#pragma once
#include "load_balancer.hpp"
#include <chrono>
#include <thread>
#include <atomic>

namespace nexus {

/// Background health checker thread.
/// Periodically probes each backend with a TCP connect attempt.
/// Marks them alive/dead atomically in the BackendPool.
class HealthChecker {
public:
    HealthChecker(BackendPool& pool,
                  std::chrono::milliseconds interval = std::chrono::milliseconds(5000));

    void start();
    void stop();

private:
    void run();

    BackendPool& pool_;
    std::chrono::milliseconds interval_;
    std::atomic<bool> running_{false};
    std::thread thread_;
};

} // namespace nexus
