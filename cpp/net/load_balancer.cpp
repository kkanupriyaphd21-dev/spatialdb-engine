#include "load_balancer.h"
#include <algorithm>
#include <stdexcept>
#include <random>
#include <functional>
#include <chrono>

namespace spatialdb {
namespace net {

static int64_t nowMs() {
    return (int64_t)std::chrono::steady_clock::now()
               .time_since_epoch().count() / 1000000;
}

LoadBalancer::LoadBalancer(LBStrategy strategy, HealthCheckConfig hc_cfg)
    : strategy_(strategy), hc_cfg_(hc_cfg) {}

void LoadBalancer::addBackend(Backend backend) {
    std::lock_guard<std::mutex> lock(mu_);
    backends_.push_back(std::move(backend));
}

void LoadBalancer::removeBackend(const std::string& id) {
    std::lock_guard<std::mutex> lock(mu_);
    backends_.erase(std::remove_if(backends_.begin(), backends_.end(),
        [&id](const Backend& b) { return b.id == id; }), backends_.end());
}

void LoadBalancer::markUnhealthy(const std::string& id) {
    std::lock_guard<std::mutex> lock(mu_);
    for (auto& b : backends_) if (b.id == id) b.healthy = false;
}

void LoadBalancer::markHealthy(const std::string& id) {
    std::lock_guard<std::mutex> lock(mu_);
    for (auto& b : backends_) if (b.id == id) b.healthy = true;
}

void LoadBalancer::recordCheckResult(const std::string& id, bool success) {
    std::lock_guard<std::mutex> lock(mu_);
    auto now = nowMs();
    for (auto& b : backends_) {
        if (b.id != id) continue;
        b.last_check_ms = now;

        if (success) {
            b.consecutive_failures = 0;
            b.consecutive_successes++;
            if (!b.healthy && b.consecutive_successes >= hc_cfg_.success_threshold) {
                b.healthy = true;
                b.last_state_change_ms = now;
            }
        } else {
            b.consecutive_successes = 0;
            b.consecutive_failures++;
            if (b.healthy && b.consecutive_failures >= hc_cfg_.failure_threshold) {
                b.healthy = false;
                b.last_state_change_ms = now;
            }
        }
        return;
    }
}

const Backend* LoadBalancer::next(const std::string& client_ip) {
    switch (strategy_) {
        case LBStrategy::ROUND_ROBIN:       return roundRobin();
        case LBStrategy::LEAST_CONNECTIONS: return leastConnections();
        case LBStrategy::IP_HASH:           return ipHash(client_ip);
        case LBStrategy::RANDOM:            return random();
    }
    return roundRobin();
}

const Backend* LoadBalancer::roundRobin() {
    std::lock_guard<std::mutex> lock(mu_);
    size_t n = backends_.size();
    if (n == 0) return nullptr;

    for (size_t i = 0; i < n; ++i) {
        size_t idx = rr_counter_.fetch_add(1) % n;
        if (backends_[idx].healthy) {
            backends_[idx].active_conns++;
            return &backends_[idx];
        }
    }
    return nullptr;
}

const Backend* LoadBalancer::leastConnections() {
    std::lock_guard<std::mutex> lock(mu_);
    const Backend* best = nullptr;
    int min_conns = INT_MAX;

    for (auto& b : backends_) {
        if (!b.healthy) continue;
        int c = b.active_conns;
        if (c < min_conns) { min_conns = c; best = &b; }
    }

    if (best) const_cast<Backend*>(best)->active_conns++;
    return best;
}

const Backend* LoadBalancer::ipHash(const std::string& ip) {
    std::lock_guard<std::mutex> lock(mu_);
    if (backends_.empty()) return nullptr;

    size_t h = std::hash<std::string>{}(ip);
    size_t n = backends_.size();

    for (size_t i = 0; i < n; ++i) {
        size_t idx = (h + i) % n;
        if (backends_[idx].healthy) {
            backends_[idx].active_conns++;
            return &backends_[idx];
        }
    }
    return nullptr;
}

const Backend* LoadBalancer::random() {
    std::lock_guard<std::mutex> lock(mu_);
    std::vector<size_t> healthy;
    for (size_t i = 0; i < backends_.size(); ++i)
        if (backends_[i].healthy) healthy.push_back(i);
    if (healthy.empty()) return nullptr;

    static std::mt19937 rng(std::random_device{}());
    size_t idx = healthy[rng() % healthy.size()];
    backends_[idx].active_conns++;
    return &backends_[idx];
}

void LoadBalancer::release(const std::string& backend_id) {
    std::lock_guard<std::mutex> lock(mu_);
    for (auto& b : backends_) {
        if (b.id == backend_id && b.active_conns > 0) {
            b.active_conns--;
            return;
        }
    }
}

size_t LoadBalancer::activeCount() const {
    std::lock_guard<std::mutex> lock(mu_);
    return backends_.size();
}

size_t LoadBalancer::healthyCount() const {
    std::lock_guard<std::mutex> lock(mu_);
    size_t c = 0;
    for (const auto& b : backends_) if (b.healthy) ++c;
    return c;
}

} // namespace net
} // namespace spatialdb
