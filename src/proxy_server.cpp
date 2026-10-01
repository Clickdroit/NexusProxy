#include "proxy_server.hpp"
#include "socket_utils.hpp"
#include "http_parser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <thread>
#include <vector>
#include <array>
#include <stdexcept>

// Minimal JSON parsing without external libs (hand-rolled for config.json)
namespace {

std::string json_get_str(const std::string& json, const std::string& key, const std::string& fallback = "") {
    auto pos = json.find("\"" + key + "\"");
    if (pos == std::string::npos) return fallback;
    pos = json.find(':', pos);
    if (pos == std::string::npos) return fallback;
    pos = json.find('"', pos);
    if (pos == std::string::npos) return fallback;
    auto end = json.find('"', pos + 1);
    if (end == std::string::npos) return fallback;
    return json.substr(pos + 1, end - pos - 1);
}

int json_get_int(const std::string& json, const std::string& key, int fallback = 0) {
    auto pos = json.find("\"" + key + "\"");
    if (pos == std::string::npos) return fallback;
    pos = json.find(':', pos);
    if (pos == std::string::npos) return fallback;
    pos = json.find_first_not_of(" \t\n\r", pos + 1);
    if (pos == std::string::npos) return fallback;
    return std::stoi(json.substr(pos));
}

} // anonymous namespace

namespace nexus {

ProxyConfig load_config(const std::string& path) {
    ProxyConfig cfg;
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Cannot open config: " + path);

    std::string json((std::istreambuf_iterator<char>(f)), {});
    cfg.listen_port               = json_get_int(json, "listen_port", 8080);
    cfg.strategy                  = json_get_str(json, "strategy", "round_robin");
    cfg.health_check_interval_ms  = json_get_int(json, "health_check_interval_ms", 5000);

    // Parse backends array
    auto arr_start = json.find("\"backends\"");
    if (arr_start != std::string::npos) {
        arr_start = json.find('[', arr_start);
        auto arr_end = json.find(']', arr_start);
        std::string arr = json.substr(arr_start, arr_end - arr_start + 1);

        // Iterate through each backend object
        size_t obj_start = arr.find('{');
        while (obj_start != std::string::npos) {
            auto obj_end = arr.find('}', obj_start);
            if (obj_end == std::string::npos) break;
            std::string obj = arr.substr(obj_start, obj_end - obj_start + 1);

            std::string host = json_get_str(obj, "host", "127.0.0.1");
            int port         = json_get_int(obj, "port", 80);
            int weight       = json_get_int(obj, "weight", 1);
            cfg.backends.emplace_back(host, port, weight);

            obj_start = arr.find('{', obj_end);
        }
    }

    return cfg;
}

// ─────────────────────────────────────────
// ProxyServer
// ─────────────────────────────────────────
ProxyServer::ProxyServer(const ProxyConfig& config)
    : config_(config),
      pool_(config.strategy == "least_connections"
            ? std::unique_ptr<ILoadBalancerStrategy>(new LeastConnectionsStrategy())
            : std::unique_ptr<ILoadBalancerStrategy>(new RoundRobinStrategy())),
      health_checker_(pool_, std::chrono::milliseconds(config.health_check_interval_ms))
{
    for (const auto& [host, port, weight] : config.backends)
        pool_.add_backend(host, port, weight);
}

ProxyServer::~ProxyServer() {
    stop();
}

void ProxyServer::run() {
    platform_init();
    listener_ = make_listener(config_.listen_port);
    running_.store(true);
    health_checker_.start();

    std::cout << "======================================\n";
    std::cout << "  ??? NexusProxy - C++ Load Balancer\n";
    std::cout << "======================================\n";
    std::cout << "[INFO] Listening on port " << config_.listen_port << "\n";
    std::cout << "[INFO] Strategy: " << config_.strategy << "\n";
    std::cout << "[INFO] Backends: " << config_.backends.size() << " configured\n";
    std::cout << "[INFO] Health checks every " << config_.health_check_interval_ms << "ms\n";

    while (running_.load()) {
        struct sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);
        socket_t client = accept(listener_,
                                 reinterpret_cast<struct sockaddr*>(&client_addr),
                                 &addr_len);
        if (client == INVALID_SOCKET_VALUE) continue;

        char ip_buf[INET_ADDRSTRLEN] = "unknown";
        inet_ntop(AF_INET, &client_addr.sin_addr, ip_buf, sizeof(ip_buf));
        std::string client_ip = ip_buf;

        // Each client handled in a detached thread
        std::thread([this, client, client_ip]() mutable {
            this->handle_client(client, client_ip);
        }).detach();
    }

    close_socket(listener_);
    platform_cleanup();
}

void ProxyServer::stop() {
    running_.store(false);
    health_checker_.stop();
    if (listener_ != INVALID_SOCKET_VALUE) {
        close_socket(listener_);
        listener_ = INVALID_SOCKET_VALUE;
    }
}

void ProxyServer::handle_client(socket_t client_fd, const std::string& client_ip) {
    BackendServer* backend = pool_.next_healthy();
    if (!backend) {
        // 502 Bad Gateway
        const std::string response =
            "HTTP/1.1 502 Bad Gateway\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n\r\n"
            "502 Bad Gateway: No healthy backends available.";
        send(client_fd, response.c_str(), static_cast<int>(response.size()), 0);
        close_socket(client_fd);
        return;
    }

    socket_t backend_fd = connect_to(backend->host, backend->port);
    if (backend_fd == INVALID_SOCKET_VALUE) {
        pool_.mark_alive(backend, false);
        const std::string response =
            "HTTP/1.1 503 Service Unavailable\r\n"
            "Connection: close\r\n\r\n"
            "503 Backend temporarily unavailable.";
        send(client_fd, response.c_str(), static_cast<int>(response.size()), 0);
        close_socket(client_fd);
        return;
    }

    // Read initial request from client to rewrite proxy headers
    std::array<char, 8192> buf{};
    int n = recv(client_fd, buf.data(), static_cast<int>(buf.size()) - 1, 0);
    if (n > 0) {
        std::string raw_req(buf.data(), n);
        std::string rewritten = rewrite_proxy_headers(raw_req, client_ip,
                                                       backend->host, backend->port);
        send(backend_fd, rewritten.c_str(), static_cast<int>(rewritten.size()), 0);
    }

    backend->active_connections.fetch_add(1, std::memory_order_relaxed);

    // Relay bidirectionally until both sides close
    relay_tcp(client_fd, backend_fd);

    backend->active_connections.fetch_sub(1, std::memory_order_relaxed);
    close_socket(backend_fd);
    close_socket(client_fd);
}

void ProxyServer::relay_tcp(socket_t client_fd, socket_t backend_fd) {
    std::array<char, 65536> buf{};
    bool client_open  = true;
    bool backend_open = true;

    while (client_open || backend_open) {
        fd_set rfds;
        FD_ZERO(&rfds);
        if (client_open)  FD_SET(client_fd,  &rfds);
        if (backend_open) FD_SET(backend_fd, &rfds);

        int max_fd = static_cast<int>(std::max(client_fd, backend_fd)) + 1;
        struct timeval tv { .tv_sec = 30, .tv_usec = 0 };

        if (select(max_fd, &rfds, nullptr, nullptr, &tv) <= 0) break;

        // Client → Backend
        if (client_open && FD_ISSET(client_fd, &rfds)) {
            int n = recv(client_fd, buf.data(), static_cast<int>(buf.size()), 0);
            if (n <= 0) {
                client_open = false;
#ifndef _WIN32
                shutdown(backend_fd, SHUT_WR);
#endif
            } else {
                send(backend_fd, buf.data(), n, 0);
            }
        }

        // Backend → Client
        if (backend_open && FD_ISSET(backend_fd, &rfds)) {
            int n = recv(backend_fd, buf.data(), static_cast<int>(buf.size()), 0);
            if (n <= 0) {
                backend_open = false;
#ifndef _WIN32
                shutdown(client_fd, SHUT_WR);
#endif
            } else {
                send(client_fd, buf.data(), n, 0);
            }
        }
    }
}

} // namespace nexus
