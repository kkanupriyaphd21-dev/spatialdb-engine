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

struct Backend {
    std::string id;
    std::string host;
    int         port       = 9851;
    bool        healthy    = true;
    int         weight     = 1;
    int active_conns{0};

    Backend() = default;
    Backend(std::string id, std::string host, int port, int weight = 1)
        : id(std::move(id)), host(std::move(host)), port(port), weight(weight) {}
    Backend(const Backend& o)
        : id(o.id), host(o.host), port(o.port), healthy(o.healthy), weight(o.weight), active_conns(o.active_conns) {}
};

class LoadBalancer {
public:
    explicit LoadBalancer(LBStrategy strategy = LBStrategy::ROUND_ROBIN);
    ~LoadBalancer() = default;

    void addBackend(Backend backend);
    void removeBackend(const std::string& id);
    void markUnhealthy(const std::string& id);
    void markHealthy(const std::string& id);

    const Backend* next(const std::string& client_ip = "");
    void           release(const std::string& backend_id);

    size_t activeCount()  const;
    size_t healthyCount() const;

private:
    LBStrategy              strategy_;
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
