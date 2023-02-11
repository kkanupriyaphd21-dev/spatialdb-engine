#pragma once
#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include "../include/index.h"

namespace spatialdb {
namespace storage {

struct SnapshotHeader {
    uint32_t magic     = 0x53504442; // "SPDB"
    uint32_t version   = 1;
    uint64_t timestamp = 0;
    uint64_t entry_count = 0;
    uint32_t checksum  = 0;
};

class SnapshotWriter {
public:
    explicit SnapshotWriter(const std::string& path);
    ~SnapshotWriter();

    bool begin(uint64_t timestamp);
    bool writeEntry(const index::IndexEntry& entry);
    bool commit();
    void abort();

    size_t entriesWritten() const { return written_; }

private:
    std::string path_;
    std::string tmp_path_;
    FILE*       fp_     = nullptr;
    size_t      written_ = 0;
    uint64_t    ts_     = 0;
    uint32_t    crc_    = 0;

    bool writeHeader();
    bool finalizeHeader();
    uint32_t updateCRC(uint32_t crc, const void* data, size_t len);
};

class SnapshotReader {
public:
    explicit SnapshotReader(const std::string& path);
    bool load(std::function<void(const index::IndexEntry&)> handler);
    SnapshotHeader header() const { return header_; }

private:
    std::string     path_;
    SnapshotHeader  header_;
    bool readEntry(FILE* fp, index::IndexEntry& out);
};

} // namespace storage
} // namespace spatialdb
