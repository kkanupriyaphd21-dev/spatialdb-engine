#include "snapshot.h"
#include <cstdio>
#include <cstring>
#include <ctime>
#include <stdexcept>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <fileapi.h>
#else
#include <unistd.h>
#include <fcntl.h>
#endif

namespace spatialdb {
namespace storage {

// Simple CRC32 table
static uint32_t CRC_TABLE[256];
static bool CRC_INIT = false;

static void initCRC() {
    if (CRC_INIT) return;
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t c = i;
        for (int j = 0; j < 8; ++j)
            c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        CRC_TABLE[i] = c;
    }
    CRC_INIT = true;
}

uint32_t SnapshotWriter::updateCRC(uint32_t crc, const void* data, size_t len) {
    initCRC();
    const uint8_t* p = static_cast<const uint8_t*>(data);
    crc = ~crc;
    for (size_t i = 0; i < len; ++i)
        crc = CRC_TABLE[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);
    return ~crc;
}

SnapshotWriter::SnapshotWriter(const std::string& path)
    : path_(path), tmp_path_(path + ".tmp") {}

SnapshotWriter::~SnapshotWriter() {
    if (fp_) { fclose(fp_); fp_ = nullptr; }
}

bool SnapshotWriter::begin(uint64_t timestamp) {
    fp_ = fopen(tmp_path_.c_str(), "wb");
    if (!fp_) return false;
    ts_      = timestamp;
    written_ = 0;
    crc_     = 0;
    return writeHeader();
}

bool SnapshotWriter::writeHeader() {
    SnapshotHeader h;
    h.timestamp = ts_;
    h.entry_count = 0;
    h.checksum = 0;
    return fwrite(&h, sizeof(h), 1, fp_) == 1;
}

bool SnapshotWriter::writeEntry(const index::IndexEntry& entry) {
    uint16_t id_len  = (uint16_t)entry.id.size();
    uint16_t col_len = (uint16_t)entry.collection.size();

    if (fwrite(&id_len, 2, 1, fp_) != 1) return false;
    if (fwrite(entry.id.data(), 1, id_len, fp_) != id_len) return false;
    if (fwrite(&col_len, 2, 1, fp_) != 1) return false;
    if (fwrite(entry.collection.data(), 1, col_len, fp_) != col_len) return false;
    if (fwrite(&entry.point.lat, 8, 1, fp_) != 1) return false;
    if (fwrite(&entry.point.lon, 8, 1, fp_) != 1) return false;
    if (fwrite(&entry.timestamp, 8, 1, fp_) != 1) return false;

    // Update CRC with entry data
    crc_ = updateCRC(crc_, &id_len, 2);
    crc_ = updateCRC(crc_, entry.id.data(), id_len);
    crc_ = updateCRC(crc_, &col_len, 2);
    crc_ = updateCRC(crc_, entry.collection.data(), col_len);
    crc_ = updateCRC(crc_, &entry.point.lat, 8);
    crc_ = updateCRC(crc_, &entry.point.lon, 8);
    crc_ = updateCRC(crc_, &entry.timestamp, 8);

    ++written_;
    return true;
}

bool SnapshotWriter::finalizeHeader() {
    if (fseek(fp_, 0, SEEK_SET) != 0) return false;
    SnapshotHeader h;
    h.timestamp   = ts_;
    h.entry_count = (uint64_t)written_;
    h.checksum    = crc_;
    if (fwrite(&h, sizeof(h), 1, fp_) != 1) return false;
    return fflush(fp_) == 0;
}

// fsync the file descriptor to ensure data is flushed to disk
static int fsyncFile(FILE* fp) {
    fflush(fp);
#ifdef _WIN32
    HANDLE h = (HANDLE)_get_osfhandle(_fileno(fp));
    if (h == INVALID_HANDLE_VALUE) return -1;
    return FlushFileBuffers(h) ? 0 : -1;
#else
    return fsync(fileno(fp));
#endif
}

bool SnapshotWriter::commit() {
    if (!fp_) return false;
    if (!finalizeHeader()) return false;
    if (fsyncFile(fp_) != 0) return false;
    fclose(fp_); fp_ = nullptr;

    // Atomic rename ensures crash-safe durability
    return rename(tmp_path_.c_str(), path_.c_str()) == 0;
}

void SnapshotWriter::abort() {
    if (fp_) { fclose(fp_); fp_ = nullptr; }
    remove(tmp_path_.c_str());
}

// ─── SnapshotReader ───────────────────────────────────────────────────────────

SnapshotReader::SnapshotReader(const std::string& path) : path_(path) {}

bool SnapshotReader::readEntry(FILE* fp, index::IndexEntry& out) {
    uint16_t id_len;
    if (fread(&id_len, 2, 1, fp) != 1) return false;
    if (id_len > 1024) return false; // sanity check
    out.id.resize(id_len);
    if (fread(out.id.data(), 1, id_len, fp) != id_len) return false;

    uint16_t col_len;
    if (fread(&col_len, 2, 1, fp) != 1) return false;
    if (col_len > 1024) return false; // sanity check
    out.collection.resize(col_len);
    if (fread(out.collection.data(), 1, col_len, fp) != col_len) return false;

    if (fread(&out.point.lat, 8, 1, fp) != 1) return false;
    if (fread(&out.point.lon, 8, 1, fp) != 1) return false;
    if (fread(&out.timestamp, 8, 1, fp) != 1) return false;

    return true;
}

bool SnapshotReader::load(std::function<void(const index::IndexEntry&)> handler) {
    FILE* fp = fopen(path_.c_str(), "rb");
    if (!fp) return false;

    if (fread(&header_, sizeof(header_), 1, fp) != 1) {
        fclose(fp);
        return false;
    }

    if (header_.magic != 0x53504442) {
        fclose(fp);
        return false;
    }

    index::IndexEntry entry;
    size_t loaded = 0;
    while (loaded < header_.entry_count && readEntry(fp, entry)) {
        handler(entry);
        ++loaded;
    }

    fclose(fp);
    std::cout << "Snapshot loaded: " << loaded << " entries\n";
    return true;
}

} // namespace storage
} // namespace spatialdb
