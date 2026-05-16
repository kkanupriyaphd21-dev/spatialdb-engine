#include "replication.h"
#include <iostream>
#include <chrono>
#include <algorithm>

namespace spatialdb {
namespace cluster {

static int64_t nowMs() {
    return (int64_t)std::chrono::steady_clock::now()
               .time_since_epoch().count() / 1000000;
}

ReplicationManager::ReplicationManager(ReplicationConfig cfg)
    : cfg_(std::move(cfg)), role_(cfg_.role) {}

ReplicationManager::~ReplicationManager() {
    stop();
}

bool ReplicationManager::start() {
    running_ = true;
    if (role_ == NodeRole::LEADER) {
        replication_thread_ = std::thread([this]() { replicationLoop(); });
    }
    return true;
}

void ReplicationManager::stop() {
    running_ = false;
    if (replication_thread_.joinable()) replication_thread_.join();
}

void ReplicationManager::enqueueEntry(const storage::WalEntry& entry) {
    std::lock_guard<std::mutex> lock(queue_mu_);
    entry_queue_.push(entry);
}

void ReplicationManager::replicationLoop() {
    while (running_) {
        std::vector<storage::WalEntry> batch;
        {
            std::lock_guard<std::mutex> lock(queue_mu_);
            while (!entry_queue_.empty() && batch.size() < 256) {
                batch.push_back(entry_queue_.front());
                entry_queue_.pop();
            }
        }

        if (!batch.empty()) {
            std::lock_guard<std::mutex> lock(replicas_mu_);
            for (auto& replica : replicas_) {
                if (!replica.healthy) continue;
                if (!sendEntriesToReplica(replica, batch)) {
                    replica.consecutive_failures++;
                    if (replica.consecutive_failures >= cfg_.max_consecutive_failures) {
                        replica.healthy = false;
                        std::cerr << "Replica " << replica.id << " marked unhealthy after "
                                  << replica.consecutive_failures << " failures\n";
                    }
                } else {
                    replica.consecutive_failures = 0;
                    replica.last_heartbeat_ms = nowMs();
                    // Update lag: entries behind leader
                    replica.lag = std::max((uint64_t)0, replicated_offset_.load() + batch.size() - replica.lag);
                }
            }
            replicated_offset_.fetch_add(batch.size());
        }

        // Check heartbeat timeouts
        checkHeartbeatTimeoutsLocked();

        std::this_thread::sleep_for(std::chrono::milliseconds(cfg_.sync_interval_ms));
    }
}

bool ReplicationManager::sendEntriesToReplica(ReplicaInfo& replica,
                                               const std::vector<storage::WalEntry>& entries) {
    // In production: open TCP conn and stream WAL entries
    // Stub for now - simulate occasional failure
    replica.lag += entries.size();
    return true;
}

void ReplicationManager::recordHeartbeat(const std::string& replica_id) {
    std::lock_guard<std::mutex> lock(replicas_mu_);
    for (auto& r : replicas_) {
        if (r.id == replica_id) {
            r.last_heartbeat_ms = nowMs();
            r.consecutive_failures = 0;
            return;
        }
    }
}

void ReplicationManager::checkHeartbeatTimeouts() {
    std::lock_guard<std::mutex> lock(replicas_mu_);
    checkHeartbeatTimeoutsLocked();
}

void ReplicationManager::checkHeartbeatTimeoutsLocked() {
    // Must be called with replicas_mu_ held
    int64_t now = nowMs();
    for (auto& r : replicas_) {
        if (!r.healthy) continue;
        if (r.last_heartbeat_ms > 0 &&
            now - r.last_heartbeat_ms > cfg_.heartbeat_timeout_ms) {
            r.healthy = false;
            std::cerr << "Replica " << r.id << " heartbeat timeout, marking unhealthy\n";
        }
    }
}

uint64_t ReplicationManager::replicaLag(const std::string& replica_id) const {
    std::lock_guard<std::mutex> lock(replicas_mu_);
    for (const auto& r : replicas_) {
        if (r.id == replica_id) return r.lag;
    }
    return 0;
}

bool ReplicationManager::waitForQuorum(uint64_t offset, int timeout_ms) {
    auto deadline = std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(timeout_ms);

    while (std::chrono::steady_clock::now() < deadline) {
        if (replicated_offset_.load() >= offset) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return false;
}

void ReplicationManager::addReplica(ReplicaInfo info) {
    std::lock_guard<std::mutex> lock(replicas_mu_);
    replicas_.push_back(std::move(info));
}

void ReplicationManager::removeReplica(const std::string& id) {
    std::lock_guard<std::mutex> lock(replicas_mu_);
    replicas_.erase(std::remove_if(replicas_.begin(), replicas_.end(),
        [&id](const ReplicaInfo& r) { return r.id == id; }), replicas_.end());
}

std::vector<ReplicaInfo> ReplicationManager::replicas() const {
    std::lock_guard<std::mutex> lock(replicas_mu_);
    return replicas_;
}

} // namespace cluster
} // namespace spatialdb
