#include "health_checker.hpp"
#include "socket_utils.hpp"
#include <iostream>

namespace nexus {

HealthChecker::HealthChecker(BackendPool& pool, std::chrono::milliseconds interval)
    : pool_(pool), interval_(interval) {}

void HealthChecker::start() {
    if (running_.exchange(true)) return; // Already running
    thread_ = std::thread(&HealthChecker::run, this);
}

void HealthChecker::stop() {
    running_.store(false);
    if (thread_.joinable()) thread_.join();
}

void HealthChecker::run() {
    while (running_.load()) {
        for (const auto& sp : pool_.backends()) {
            BackendServer* b = sp.get();
            socket_t probe = connect_to(b->host, b->port, 1500);

            if (probe != INVALID_SOCKET_VALUE) {
                close_socket(probe);
                if (!b->is_alive.load()) {
                    b->is_alive.store(true, std::memory_order_relaxed);
                    std::cout << "[HealthChecker] Backend " << b->host << ":"
                              << b->port << " is UP\n";
                }
            } else {
                if (b->is_alive.load()) {
                    b->is_alive.store(false, std::memory_order_relaxed);
                    std::cout << "[HealthChecker] Backend " << b->host << ":"
                              << b->port << " is DOWN\n";
                }
            }
        }
        std::this_thread::sleep_for(interval_);
    }
}

} // namespace nexus
