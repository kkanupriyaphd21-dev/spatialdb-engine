#pragma once
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <functional>
#include <memory>
#include "../storage/wal.h"

namespace spatialdb {
namespace cluster {

enum class NodeRole { LEADER, FOLLOWER, CANDIDATE };

struct ReplicaInfo {
    std::string id;
    std::string host;
    int         port    = 9852;
    uint64_t    lag     = 0;
    bool        healthy = true;
};

struct ReplicationConfig {
    std::string node_id;
    NodeRole    role           = NodeRole::FOLLOWER;
    std::string leader_host;
    int         leader_port    = 9852;
    int         sync_interval_ms = 100;
    bool        sync_aof       = true;
};

class ReplicationManager {
public:
    explicit ReplicationManager(ReplicationConfig cfg);
    ~ReplicationManager();

    bool start();
    void stop();

    bool isLeader()   const { return role_ == NodeRole::LEADER; }
    NodeRole role()   const { return role_; }
    uint64_t offset() const { return replicated_offset_.load(); }

    void enqueueEntry(const storage::WalEntry& entry);
    bool waitForQuorum(uint64_t offset, int timeout_ms = 1000);

    void addReplica(ReplicaInfo info);
    void removeReplica(const std::string& id);
    std::vector<ReplicaInfo> replicas() const;

private:
    ReplicationConfig       cfg_;
    NodeRole                role_;
    std::atomic<uint64_t>   replicated_offset_{0};
    std::atomic<bool>       running_{false};

    mutable std::mutex              replicas_mu_;
    std::vector<ReplicaInfo>        replicas_;

    std::mutex              queue_mu_;
    std::queue<storage::WalEntry> entry_queue_;
    std::thread             replication_thread_;

    void replicationLoop();
    bool sendEntriesToReplica(ReplicaInfo& replica,
                               const std::vector<storage::WalEntry>& entries);
};

} // namespace cluster
} // namespace spatialdb
