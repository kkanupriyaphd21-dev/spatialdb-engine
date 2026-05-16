#include "conn_pool.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <stdexcept>
#include <chrono>
#include <iostream>

namespace spatialdb {
namespace net {

ConnPool::ConnPool(PoolConfig cfg) : cfg_(std::move(cfg)) {
    for (size_t i = 0; i < cfg_.min_size; ++i) {
        try {
            auto conn = createConn();
            if (conn) {
                conn->last_used = (uint64_t)std::chrono::system_clock::now()
                                      .time_since_epoch().count() / 1000000000ULL;
                std::lock_guard<std::mutex> lock(mu_);
                idle_.push(conn);
                ++total_;
            }
        } catch (...) {
            // prefill best-effort
        }
    }
}

ConnPool::~ConnPool() {
    std::lock_guard<std::mutex> lock(mu_);
    while (!idle_.empty()) {
        auto c = idle_.front(); idle_.pop();
        if (c && c->fd >= 0) close(c->fd);
    }
    total_.store(0);
}

std::shared_ptr<PooledConn> ConnPool::createConn() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return nullptr;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(cfg_.port);
    if (inet_pton(AF_INET, cfg_.host.c_str(), &addr.sin_addr) <= 0) {
        close(fd);
        return nullptr;
    }

    struct timeval tv;
    tv.tv_sec  = cfg_.timeout_ms / 1000;
    tv.tv_usec = (cfg_.timeout_ms % 1000) * 1000;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(fd);
        return nullptr;
    }

    auto conn = std::make_shared<PooledConn>();
    conn->fd         = fd;
    conn->healthy    = true;
    conn->id         = next_id_++;
    conn->remote_addr = cfg_.host + ":" + std::to_string(cfg_.port);
    conn->last_used  = (uint64_t)std::chrono::system_clock::now()
                           .time_since_epoch().count() / 1000000000ULL;
    return conn;
}

bool ConnPool::pingConn(PooledConn& conn) {
    const char* ping = "*1\r\n$4\r\nPING\r\n";
    ssize_t n = send(conn.fd, ping, strlen(ping), 0);
    if (n <= 0) return false;

    char buf[32];
    n = recv(conn.fd, buf, sizeof(buf) - 1, 0);
    if (n <= 0) return false;
    buf[n] = '\0';
    return strncmp(buf, "+PONG", 5) == 0;
}

std::shared_ptr<PooledConn> ConnPool::acquire(int timeout_ms) {
    std::unique_lock<std::mutex> lock(mu_);

    auto deadline = std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(timeout_ms < 0 ? cfg_.timeout_ms : timeout_ms);

    while (true) {
        // Evict idle connections that exceed idle timeout
        evictIdleLocked();

        if (!idle_.empty()) {
            auto conn = idle_.front(); idle_.pop();
            // Health check: ping the connection before returning
            if (conn->healthy && pingConn(*conn)) {
                conn->last_used = (uint64_t)std::chrono::system_clock::now()
                                      .time_since_epoch().count() / 1000000000ULL;
                return conn;
            }
            // Connection is dead, clean it up
            if (conn->fd >= 0) close(conn->fd);
            --total_;
            continue;
        }

        if (total_.load() >= cfg_.max_size) {
            if (cv_.wait_until(lock, deadline) == std::cv_status::timeout) {
                return nullptr;
            }
            continue;
        }

        // Create new connection outside the lock
        lock.unlock();
        auto conn = createConn();
        if (conn) {
            lock.lock();
            ++total_;
        }
        return conn;
    }
}

void ConnPool::release(std::shared_ptr<PooledConn> conn) {
    if (!conn || !conn->healthy) {
        if (conn) {
            if (conn->fd >= 0) close(conn->fd);
            std::lock_guard<std::mutex> lock(mu_);
            --total_;
        }
        return;
    }
    conn->last_used = (uint64_t)std::chrono::system_clock::now()
                          .time_since_epoch().count() / 1000000000ULL;
    std::lock_guard<std::mutex> lock(mu_);
    idle_.push(conn);
    cv_.notify_one();
}

void ConnPool::invalidate(std::shared_ptr<PooledConn> conn) {
    if (!conn) return;
    conn->healthy = false;
    if (conn->fd >= 0) { close(conn->fd); conn->fd = -1; }
    std::lock_guard<std::mutex> lock(mu_);
    --total_;
    cv_.notify_one();
}

size_t ConnPool::idleCount() const {
    std::lock_guard<std::mutex> lock(mu_);
    return idle_.size();
}

size_t ConnPool::totalCount() const {
    return total_.load();
}

void ConnPool::evictIdleLocked() {
    // Must be called with mu_ held
    uint64_t now = (uint64_t)std::chrono::system_clock::now()
                       .time_since_epoch().count() / 1000000000ULL;
    uint64_t max_idle_sec = cfg_.idle_timeout_ms / 1000;

    std::queue<std::shared_ptr<PooledConn>> kept;
    while (!idle_.empty()) {
        auto conn = idle_.front(); idle_.pop();
        if (now - conn->last_used > max_idle_sec) {
            if (conn->fd >= 0) close(conn->fd);
            --total_;
        } else {
            kept.push(conn);
        }
    }
    idle_ = std::move(kept);
}

} // namespace net
} // namespace spatialdb
