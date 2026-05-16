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
    uint64_t    lag     = 0;       // entries behind leader
    bool        healthy = true;

    // Heartbeat tracking
    int64_t     last_heartbeat_ms = 0; // monotonic time of last successful heartbeat
    int         consecutive_failures = 0;
};

struct ReplicationConfig {
    std::string node_id;
    NodeRole    role           = NodeRole::FOLLOWER;
    std::string leader_host;
    int         leader_port    = 9852;
    int         sync_interval_ms = 100;
    bool        sync_aof       = true;
    int         heartbeat_timeout_ms = 5000;  // mark unhealthy after this many ms without heartbeat
    int         max_consecutive_failures = 3; // remove replica after this many failed attempts
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

    // Record heartbeat from replica - updates last_heartbeat_ms
    void recordHeartbeat(const std::string& replica_id);
    // Check for timed-out replicas and mark them unhealthy
    void checkHeartbeatTimeouts();
    // Get replication lag for a specific replica
    uint64_t replicaLag(const std::string& replica_id) const;

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
    void checkHeartbeatTimeoutsLocked(); // must hold replicas_mu_
};

} // namespace cluster
} // namespace spatialdb
