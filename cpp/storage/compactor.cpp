#include "compactor.h"
#include <chrono>
#include <iostream>

namespace spatialdb {
namespace storage {

Compactor::Compactor(std::string wal_path, std::string snapshot_path, size_t trigger)
    : wal_path_(std::move(wal_path)),
      snapshot_path_(std::move(snapshot_path)),
      trigger_entries_(trigger) {}

Compactor::~Compactor() {
    stopBackground();
}

bool Compactor::shouldCompact() const {
    // count WAL entries by reading header info
    std::ifstream f(wal_path_, std::ios::binary | std::ios::ate);
    if (!f.is_open()) return false;
    size_t sz = f.tellg();
    // rough heuristic: avg entry ~40 bytes
    return sz / 40 >= trigger_entries_;
}

CompactionStats Compactor::compact(
    std::function<std::vector<index::IndexEntry>()> dump_fn)
{
    CompactionStats stats;
    auto t0 = std::chrono::steady_clock::now();

    auto entries = dump_fn();
    stats.snapshot_entries = entries.size();

    SnapshotWriter writer(snapshot_path_);
    uint64_t now = (uint64_t)std::chrono::system_clock::now()
                       .time_since_epoch().count();

    if (!writer.begin(now)) return stats;

    for (const auto& e : entries) {
        writer.writeEntry(e);
    }

    if (!writer.commit()) return stats;

    // truncate WAL after successful snapshot
    WAL wal(wal_path_);
    wal.truncate();

    auto t1 = std::chrono::steady_clock::now();
    stats.duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    stats.success = true;

    std::cout << "Compaction done: " << entries.size() << " entries in "
              << stats.duration_ms << "ms\n";

    std::lock_guard<std::mutex> lock(stats_mu_);
    last_stats_ = stats;
    return stats;
}

void Compactor::backgroundLoop(std::function<std::vector<index::IndexEntry>()> dump_fn) {
    while (bg_running_) {
        std::this_thread::sleep_for(std::chrono::seconds(30));
        if (!bg_running_) break;
        if (shouldCompact()) {
            compact(dump_fn);
        }
    }
}

void Compactor::startBackground(std::function<std::vector<index::IndexEntry>()> dump_fn) {
    bg_running_ = true;
    bg_thread_ = std::thread([this, dump_fn]() { backgroundLoop(dump_fn); });
}

void Compactor::stopBackground() {
    bg_running_ = false;
    if (bg_thread_.joinable()) bg_thread_.join();
}

CompactionStats Compactor::lastStats() const {
    std::lock_guard<std::mutex> lock(stats_mu_);
    return last_stats_;
}

} // namespace storage
} // namespace spatialdb
