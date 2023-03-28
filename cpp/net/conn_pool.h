#pragma once
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <memory>
#include <atomic>

namespace spatialdb {
namespace net {

struct PooledConn {
    int         fd      = -1;
    bool        healthy = true;
    uint64_t    id      = 0;
    std::string remote_addr;
};

struct PoolConfig {
    std::string host        = "127.0.0.1";
    int         port        = 9851;
    size_t      min_size    = 2;
    size_t      max_size    = 32;
    int         timeout_ms  = 3000;
    int         idle_timeout_ms = 60000;
};

class ConnPool {
public:
    explicit ConnPool(PoolConfig cfg);
    ~ConnPool();

    std::shared_ptr<PooledConn> acquire(int timeout_ms = -1);
    void release(std::shared_ptr<PooledConn> conn);
    void invalidate(std::shared_ptr<PooledConn> conn);

    size_t idleCount()  const;
    size_t totalCount() const;
    bool   isHealthy()  const { return healthy_.load(); }

private:
    PoolConfig  cfg_;
    std::atomic<bool>   healthy_{true};
    std::atomic<size_t> total_{0};

    mutable std::mutex      mu_;
    std::condition_variable cv_;
    std::queue<std::shared_ptr<PooledConn>> idle_;

    uint64_t next_id_ = 1;

    std::shared_ptr<PooledConn> createConn();
    bool pingConn(PooledConn& conn);
};

} // namespace net
} // namespace spatialdb
