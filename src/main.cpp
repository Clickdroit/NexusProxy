#include "proxy_server.hpp"
#include <iostream>
#include <csignal>
#include <stdexcept>

static nexus::ProxyServer* g_server = nullptr;

void signal_handler(int) {
    std::cout << "\n[NexusProxy] Shutting down gracefully...\n";
    if (g_server) g_server->stop();
}

int main(int argc, char* argv[]) {
    std::string config_path = "config.json";
    if (argc >= 2) config_path = argv[1];

    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    try {
        nexus::ProxyConfig config = nexus::load_config(config_path);
        nexus::ProxyServer server(config);
        g_server = &server;
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << "\n";
        return 1;
    }

    return 0;
}
