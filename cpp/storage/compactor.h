#pragma once
#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include "wal.h"
#include "snapshot.h"

namespace spatialdb {
namespace storage {

struct CompactionStats {
    size_t   wal_entries_before = 0;
    size_t   wal_entries_after  = 0;
    size_t   snapshot_entries   = 0;
    uint64_t duration_ms        = 0;
    bool     success            = false;
};

class Compactor {
public:
    explicit Compactor(std::string wal_path,
                       std::string snapshot_path,
                       size_t      trigger_wal_entries = 10000);
    ~Compactor();

    bool shouldCompact() const;
    CompactionStats compact(std::function<std::vector<index::IndexEntry>()> dump_fn);

    void startBackground(std::function<std::vector<index::IndexEntry>()> dump_fn);
    void stopBackground();

    bool isRunning() const { return bg_running_.load(); }
    CompactionStats lastStats() const;

private:
    std::string  wal_path_;
    std::string  snapshot_path_;
    size_t       trigger_entries_;

    std::atomic<bool>    bg_running_{false};
    std::thread          bg_thread_;
    mutable std::mutex   stats_mu_;
    CompactionStats      last_stats_;

    void backgroundLoop(std::function<std::vector<index::IndexEntry>()> dump_fn);
};

} // namespace storage
} // namespace spatialdb
