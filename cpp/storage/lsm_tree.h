#pragma once
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <mutex>
#include <optional>
#include <functional>

namespace spatialdb {
namespace storage {

struct LSMEntry {
    std::string key;
    std::string value;
    bool        deleted   = false;
    uint64_t    seq_num   = 0;
};

class MemTable {
public:
    explicit MemTable(size_t max_size_bytes = 64 * 1024 * 1024);

    bool put(std::string key, std::string value, uint64_t seq);
    bool del(const std::string& key, uint64_t seq);
    std::optional<LSMEntry> get(const std::string& key) const;

    bool    isFull()    const { return size_bytes_ >= max_size_bytes_; }
    size_t  sizeBytes() const { return size_bytes_; }
    size_t  entryCount() const { return entries_.size(); }

    std::vector<LSMEntry> flush() const;
    void clear();

private:
    mutable std::mutex   mu_;
    std::map<std::string, LSMEntry> entries_;
    size_t               size_bytes_     = 0;
    size_t               max_size_bytes_;
};

struct SSTableBlock {
    std::string min_key;
    std::string max_key;
    std::vector<LSMEntry> entries;
    uint64_t    offset = 0;
    uint32_t    checksum = 0;
};

class SSTable {
public:
    SSTable(std::string path, std::vector<LSMEntry> entries);
    explicit SSTable(std::string path);

    bool load();
    std::optional<LSMEntry> get(const std::string& key) const;
    std::vector<LSMEntry>   scan(const std::string& from, const std::string& to) const;

    const std::string& path()    const { return path_; }
    const std::string& minKey()  const { return min_key_; }
    const std::string& maxKey()  const { return max_key_; }
    size_t             entryCount() const { return entries_.size(); }
    bool               mayContain(const std::string& key) const;

private:
    std::string           path_;
    std::string           min_key_;
    std::string           max_key_;
    std::vector<LSMEntry> entries_; // sorted by key
    bool                  loaded_ = false;

    bool write(const std::vector<LSMEntry>& entries);
};

struct CompactionStats {
    size_t sstables_merged = 0;
    size_t entries_read    = 0;
    size_t entries_written = 0;
    size_t entries_deleted = 0; // tombstones removed
    size_t bytes_before    = 0;
    size_t bytes_after     = 0;
    int64_t duration_ms    = 0;
    bool   skipped         = false; // true if compaction was skipped (nothing to do)
};

class LSMTree {
public:
    explicit LSMTree(std::string dir, size_t memtable_size = 64 * 1024 * 1024);
    ~LSMTree();

    bool put(const std::string& key, const std::string& value);
    bool del(const std::string& key);
    std::optional<std::string> get(const std::string& key);

    CompactionStats compact();
    size_t sstableCount() const { return sstables_.size(); }
    size_t memtableSize() const { return memtable_.sizeBytes(); }

private:
    std::string                                  dir_;
    MemTable                                     memtable_;
    std::vector<std::shared_ptr<SSTable>>        sstables_;
    mutable std::mutex                           mu_;
    uint64_t                                     seq_ = 1;

    void flushMemTable();
    std::string newSSTablePath();
};

} // namespace storage
} // namespace spatialdb
