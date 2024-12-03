#include "health_check.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <chrono>
#include <iostream>
#include <cstring>

namespace spatialdb {
namespace cluster {

HealthChecker::HealthChecker(int interval_ms, int timeout_ms)
    : interval_ms_(interval_ms), timeout_ms_(timeout_ms) {}

HealthChecker::~HealthChecker() { stop(); }

void HealthChecker::addNode(const std::string& id, const std::string& host, int port) {
    std::lock_guard<std::mutex> lock(mu_);
    NodeHealth h;
    h.id   = id;
    h.host = host;
    h.port = port;
    nodes_[id] = h;
}

void HealthChecker::removeNode(const std::string& id) {
    std::lock_guard<std::mutex> lock(mu_);
    nodes_.erase(id);
}

NodeHealth HealthChecker::checkNode(const NodeHealth& current) {
    NodeHealth updated = current;
    updated.last_checked = std::chrono::steady_clock::now();

    // attempt TCP connect with timeout
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        updated.status = NodeStatus::UNREACHABLE;
        return updated;
    }

    struct timeval tv;
    tv.tv_sec  = timeout_ms_ / 1000;
    tv.tv_usec = (timeout_ms_ % 1000) * 1000;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(current.port);
    inet_pton(AF_INET, current.host.c_str(), &addr.sin_addr);

    auto t0 = std::chrono::steady_clock::now();
    int rc = connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    auto t1 = std::chrono::steady_clock::now();

    if (rc < 0) {
        updated.status = NodeStatus::UNREACHABLE;
        close(fd);
        return updated;
    }

    updated.latency_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    // send PING
    const char* ping = "*1\r\n$4\r\nPING\r\n";
    send(fd, ping, strlen(ping), 0);

    char buf[64];
    ssize_t n = recv(fd, buf, sizeof(buf)-1, 0);
    close(fd);

    if (n > 0 && buf[0] == '+') {
        updated.status = NodeStatus::HEALTHY;
    } else {
        updated.status = NodeStatus::DEGRADED;
    }

    return updated;
}

void HealthChecker::notifyChange(const NodeHealth& old_h, const NodeHealth& new_h) {
    for (auto& cb : callbacks_) cb(old_h, new_h);
}

void HealthChecker::checkLoop() {
    while (running_) {
        std::vector<std::string> ids;
        {
            std::lock_guard<std::mutex> lock(mu_);
            for (const auto& [id, _] : nodes_) ids.push_back(id);
        }

        for (const auto& id : ids) {
            if (!running_) break;

            NodeHealth current;
            {
                std::lock_guard<std::mutex> lock(mu_);
                auto it = nodes_.find(id);
                if (it == nodes_.end()) continue;
                current = it->second;
            }

            auto updated = checkNode(current);

            {
                std::lock_guard<std::mutex> lock(mu_);
                auto it = nodes_.find(id);
                if (it == nodes_.end()) continue;

                if (it->second.status != updated.status) {
                    notifyChange(it->second, updated);
                    std::cout << "[health] " << id << ": "
                              << (int)it->second.status << " -> "
                              << (int)updated.status << "\n";
                }
                it->second = updated;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms_));
    }
}

bool HealthChecker::start() {
    running_ = true;
    check_thread_ = std::thread([this]() { checkLoop(); });
    return true;
}

void HealthChecker::stop() {
    running_ = false;
    if (check_thread_.joinable()) check_thread_.join();
}

NodeHealth HealthChecker::getHealth(const std::string& id) const {
    std::lock_guard<std::mutex> lock(mu_);
    auto it = nodes_.find(id);
    if (it == nodes_.end()) return {};
    return it->second;
}

std::vector<NodeHealth> HealthChecker::allHealth() const {
    std::lock_guard<std::mutex> lock(mu_);
    std::vector<NodeHealth> out;
    for (const auto& [id, h] : nodes_) out.push_back(h);
    return out;
}

bool HealthChecker::isHealthy(const std::string& id) const {
    auto h = getHealth(id);
    return h.status == NodeStatus::HEALTHY;
}

void HealthChecker::onHealthChange(HealthChangeCallback cb) {
    std::lock_guard<std::mutex> lock(mu_);
    callbacks_.push_back(std::move(cb));
}

} // namespace cluster
} // namespace spatialdb
