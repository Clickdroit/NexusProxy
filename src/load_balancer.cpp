#include "load_balancer.hpp"
#include <stdexcept>
#include <limits>

namespace nexus {

// ─────────────────────────────────────────
// RoundRobinStrategy
// ─────────────────────────────────────────
BackendServer* RoundRobinStrategy::select(
    const std::vector<std::shared_ptr<BackendServer>>& backends)
{
    if (backends.empty()) return nullptr;

    const size_t n = backends.size();
    for (size_t i = 0; i < n; ++i) {
        size_t idx = counter_.fetch_add(1, std::memory_order_relaxed) % n;
        BackendServer* b = backends[idx].get();
        if (b->is_alive.load(std::memory_order_relaxed)) return b;
    }
    return nullptr; // All backends down
}

// ─────────────────────────────────────────
// LeastConnectionsStrategy
// ─────────────────────────────────────────
BackendServer* LeastConnectionsStrategy::select(
    const std::vector<std::shared_ptr<BackendServer>>& backends)
{
    BackendServer* best = nullptr;
    int min_conn = std::numeric_limits<int>::max();

    for (const auto& sp : backends) {
        if (!sp->is_alive.load(std::memory_order_relaxed)) continue;
        int conn = sp->active_connections.load(std::memory_order_relaxed);
        if (conn < min_conn) {
            min_conn = conn;
            best = sp.get();
        }
    }
    return best;
}

// ─────────────────────────────────────────
// BackendPool
// ─────────────────────────────────────────
BackendPool::BackendPool(std::unique_ptr<ILoadBalancerStrategy> strategy)
    : strategy_(std::move(strategy)) {}

void BackendPool::add_backend(const std::string& host, int port, int weight) {
    std::unique_lock lock(mutex_);
    backends_.push_back(std::make_shared<BackendServer>(host, port, weight));
}

BackendServer* BackendPool::next_healthy() {
    std::shared_lock lock(mutex_);
    return strategy_->select(backends_);
}

const std::vector<std::shared_ptr<BackendServer>>& BackendPool::backends() const {
    return backends_;
}

void BackendPool::mark_alive(BackendServer* b, bool alive) {
    if (b) b->is_alive.store(alive, std::memory_order_relaxed);
}

} // namespace nexus
