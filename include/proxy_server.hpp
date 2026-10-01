#pragma once
#include "load_balancer.hpp"
#include "health_checker.hpp"
#include <string>
#include <thread>
#include <atomic>
#include <vector>

namespace nexus {

struct ProxyConfig {
    int listen_port = 8080;
    std::string strategy = "round_robin";  // or "least_connections"
    int health_check_interval_ms = 5000;
    std::vector<std::tuple<std::string, int, int>> backends; // host, port, weight
};

/// Loads ProxyConfig from a config.json file path.
ProxyConfig load_config(const std::string& path);

/// Main proxy server: accepts TCP connections and forwards them to backends.
class ProxyServer {
public:
    explicit ProxyServer(const ProxyConfig& config);
    ~ProxyServer();

    void run(); // blocking
    void stop();

private:
    void handle_client(socket_t client_fd, const std::string& client_ip);
    void relay_tcp(socket_t client_fd, socket_t backend_fd);

    ProxyConfig config_;
    BackendPool pool_;
    HealthChecker health_checker_;
    std::atomic<bool> running_{false};
    socket_t listener_{INVALID_SOCKET_VALUE};
};

} // namespace nexus
