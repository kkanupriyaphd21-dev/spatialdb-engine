#include "wal.h"
#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>

namespace spatialdb {
namespace storage {

WAL::WAL(const std::string& path, size_t sync_every)
    : path_(path), sync_every_(sync_every)
{
    file_.open(path_, std::ios::binary | std::ios::app);
    if (!file_.is_open()) {
        throw std::runtime_error("WAL: failed to open file: " + path_);
    }
}

WAL::~WAL() {
    if (file_.is_open()) {
        file_.flush();
        syncToDisk();
        file_.close();
    }
}

bool WAL::writeEntry(const WalEntry& e) {
    uint8_t op = static_cast<uint8_t>(e.op);
    file_.write(reinterpret_cast<const char*>(&op), 1);

    uint16_t col_len = (uint16_t)e.collection.size();
    file_.write(reinterpret_cast<const char*>(&col_len), 2);
    file_.write(e.collection.data(), col_len);

    uint16_t id_len = (uint16_t)e.id.size();
    file_.write(reinterpret_cast<const char*>(&id_len), 2);
    file_.write(e.id.data(), id_len);

    file_.write(reinterpret_cast<const char*>(&e.lat), 8);
    file_.write(reinterpret_cast<const char*>(&e.lon), 8);
    file_.write(reinterpret_cast<const char*>(&e.timestamp), 8);

    return file_.good();
}

bool WAL::append(const WalEntry& entry) {
    std::lock_guard<std::mutex> lock(mu_);
    if (!writeEntry(entry)) return false;
    ++pending_;
    if (pending_ >= sync_every_) {
        file_.flush();
        if (!syncToDisk()) return false;
        pending_ = 0;
    }
    return true;
}

bool WAL::flush() {
    std::lock_guard<std::mutex> lock(mu_);
    file_.flush();
    bool ok = syncToDisk();
    pending_ = 0;
    return ok && file_.good();
}

bool WAL::readEntry(std::ifstream& in, WalEntry& out) {
    uint8_t op;
    if (!in.read(reinterpret_cast<char*>(&op), 1)) return false;
    out.op = static_cast<WalOpType>(op);

    uint16_t col_len;
    if (!in.read(reinterpret_cast<char*>(&col_len), 2)) return false;
    if (col_len > 1024) return false;
    out.collection.resize(col_len);
    if (!in.read(out.collection.data(), col_len)) return false;

    uint16_t id_len;
    if (!in.read(reinterpret_cast<char*>(&id_len), 2)) return false;
    if (id_len > 1024) return false;
    out.id.resize(id_len);
    if (!in.read(out.id.data(), id_len)) return false;

    if (!in.read(reinterpret_cast<char*>(&out.lat), 8)) return false;
    if (!in.read(reinterpret_cast<char*>(&out.lon), 8)) return false;
    if (!in.read(reinterpret_cast<char*>(&out.timestamp), 8)) return false;

    return true;
}

bool WAL::replay(std::function<void(const WalEntry&)> handler) {
    std::ifstream in(path_, std::ios::binary);
    if (!in.is_open()) return false;

    WalEntry entry;
    while (readEntry(in, entry)) {
        handler(entry);
    }

    return true;
}

bool WAL::truncate() {
    std::lock_guard<std::mutex> lock(mu_);
    file_.close();
    file_.open(path_, std::ios::binary | std::ios::trunc);
    return file_.is_open();
}

bool WAL::syncToDisk() {
    if (!file_.is_open()) return false;
    file_.flush();

#ifdef _WIN32
    return true;
#else
    int file_desc = open(path_.c_str(), O_RDONLY);
    if (file_desc < 0) return false;
    bool result = (fsync(file_desc) == 0);
    close(file_desc);
    return result;
#endif
}

} // namespace storage
} // namespace spatialdb
