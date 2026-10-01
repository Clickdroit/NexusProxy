#pragma once
#include <string>
#include <vector>
#include <atomic>
#include <memory>
#include <shared_mutex>
#include <thread>
#include <functional>
#include "socket_utils.hpp"

namespace nexus {

struct BackendServer {
    std::string host;
    int port;
    int weight = 1;
    std::atomic<bool> is_alive{true};
    std::atomic<int> active_connections{0};

    // Non-copyable due to atomics — use shared_ptr
    BackendServer() = default;
    BackendServer(const std::string& h, int p, int w = 1)
        : host(h), port(p), weight(w) {}
};

/// Abstract interface for load balancing strategies.
class ILoadBalancerStrategy {
public:
    virtual ~ILoadBalancerStrategy() = default;
    virtual BackendServer* select(const std::vector<std::shared_ptr<BackendServer>>& backends) = 0;
};

/// Round-Robin: cycles through healthy backends in sequence.
class RoundRobinStrategy : public ILoadBalancerStrategy {
public:
    BackendServer* select(const std::vector<std::shared_ptr<BackendServer>>& backends) override;
private:
    std::atomic<size_t> counter_{0};
};

/// Least-Connections: always routes to backend with fewest active connections.
class LeastConnectionsStrategy : public ILoadBalancerStrategy {
public:
    BackendServer* select(const std::vector<std::shared_ptr<BackendServer>>& backends) override;
};

/// Manages backend server pool with thread-safe health state and load balancing.
class BackendPool {
public:
    BackendPool(std::unique_ptr<ILoadBalancerStrategy> strategy);

    void add_backend(const std::string& host, int port, int weight = 1);
    BackendServer* next_healthy();

    const std::vector<std::shared_ptr<BackendServer>>& backends() const;
    void mark_alive(BackendServer* b, bool alive);

private:
    mutable std::shared_mutex mutex_;
    std::vector<std::shared_ptr<BackendServer>> backends_;
    std::unique_ptr<ILoadBalancerStrategy> strategy_;
};

} // namespace nexus
