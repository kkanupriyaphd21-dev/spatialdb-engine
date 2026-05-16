#pragma once
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <memory>
#include <functional>

namespace spatialdb {
namespace net {

enum class LBStrategy {
    ROUND_ROBIN,
    LEAST_CONNECTIONS,
    RANDOM,
    IP_HASH,
};

struct HealthCheckConfig {
    int interval_ms    = 10000;  // health check interval
    int timeout_ms     = 3000;   // timeout for each health check
    int failure_threshold  = 3;  // consecutive failures before marking unhealthy
    int success_threshold  = 2;  // consecutive successes before marking healthy
};

struct Backend {
    std::string id;
    std::string host;
    int         port       = 9851;
    bool        healthy    = true;
    int         weight     = 1;
    int         active_conns = 0;

    // Health check state
    int         consecutive_failures = 0;
    int         consecutive_successes = 0;
    int64_t     last_check_ms = 0;     // monotonic time of last check
    int64_t     last_state_change_ms = 0; // when healthy/unhealthy last changed

    Backend() = default;
    Backend(std::string id, std::string host, int port, int weight = 1)
        : id(std::move(id)), host(std::move(host)), port(port), weight(weight) {}
};

class LoadBalancer {
public:
    explicit LoadBalancer(LBStrategy strategy = LBStrategy::ROUND_ROBIN,
                          HealthCheckConfig hc_cfg = {});
    ~LoadBalancer() = default;

    void addBackend(Backend backend);
    void removeBackend(const std::string& id);
    void markUnhealthy(const std::string& id);
    void markHealthy(const std::string& id);

    // Record health check result - applies thresholds before changing state
    void recordCheckResult(const std::string& id, bool success);

    const Backend* next(const std::string& client_ip = "");
    void           release(const std::string& backend_id);

    size_t activeCount()  const;
    size_t healthyCount() const;

    const HealthCheckConfig& healthCheckConfig() const { return hc_cfg_; }

private:
    LBStrategy              strategy_;
    HealthCheckConfig       hc_cfg_;
    std::atomic<size_t>     rr_counter_{0};
    mutable std::mutex      mu_;
    std::vector<Backend>    backends_;

    const Backend* roundRobin();
    const Backend* leastConnections();
    const Backend* ipHash(const std::string& ip);
    const Backend* random();
};

} // namespace net
} // namespace spatialdb
