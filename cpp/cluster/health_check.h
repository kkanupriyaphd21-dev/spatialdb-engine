#pragma once
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>
#include <chrono>

namespace spatialdb {
namespace cluster {

enum class NodeStatus {
    HEALTHY,
    DEGRADED,
    UNREACHABLE,
    UNKNOWN,
};

struct NodeHealth {
    std::string id;
    std::string host;
    int         port       = 9851;
    NodeStatus  status     = NodeStatus::UNKNOWN;
    double      latency_ms = 0.0;
    std::string version;
    uint64_t    uptime_sec = 0;
    size_t      active_conns = 0;
    std::chrono::steady_clock::time_point last_checked;
};

using HealthChangeCallback = std::function<void(const NodeHealth& old_health,
                                                 const NodeHealth& new_health)>;

class HealthChecker {
public:
    explicit HealthChecker(int interval_ms = 5000, int timeout_ms = 1000);
    ~HealthChecker();

    void addNode(const std::string& id, const std::string& host, int port);
    void removeNode(const std::string& id);

    bool start();
    void stop();

    NodeHealth    getHealth(const std::string& id) const;
    std::vector<NodeHealth> allHealth() const;
    bool          isHealthy(const std::string& id) const;

    void onHealthChange(HealthChangeCallback cb);

private:
    int           interval_ms_;
    int           timeout_ms_;
    std::atomic<bool> running_{false};
    std::thread   check_thread_;

    mutable std::mutex                               mu_;
    std::unordered_map<std::string, NodeHealth>      nodes_;
    std::vector<HealthChangeCallback>                callbacks_;

    void checkLoop();
    NodeHealth checkNode(const NodeHealth& current);
    void notifyChange(const NodeHealth& old_h, const NodeHealth& new_h);
};

} // namespace cluster
} // namespace spatialdb
