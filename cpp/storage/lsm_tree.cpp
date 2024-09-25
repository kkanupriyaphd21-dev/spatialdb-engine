#include "lsm_tree.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <cstring>

namespace spatialdb {
namespace storage {

// ─── MemTable ──────────────────────────────────────────────────────────────

MemTable::MemTable(size_t max_size) : max_size_bytes_(max_size) {}

bool MemTable::put(std::string key, std::string value, uint64_t seq) {
    std::lock_guard<std::mutex> lock(mu_);
    size_bytes_ += key.size() + value.size();
    entries_[key] = {key, value, false, seq};
    return true;
}

bool MemTable::del(const std::string& key, uint64_t seq) {
    std::lock_guard<std::mutex> lock(mu_);
    entries_[key] = {key, "", true, seq};
    return true;
}

std::optional<LSMEntry> MemTable::get(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mu_);
    auto it = entries_.find(key);
    if (it == entries_.end()) return std::nullopt;
    return it->second;
}

std::vector<LSMEntry> MemTable::flush() const {
    std::lock_guard<std::mutex> lock(mu_);
    std::vector<LSMEntry> out;
    out.reserve(entries_.size());
    for (const auto& [k, v] : entries_) out.push_back(v);
    return out;
}

void MemTable::clear() {
    std::lock_guard<std::mutex> lock(mu_);
    entries_.clear();
    size_bytes_ = 0;
}

// ─── SSTable ──────────────────────────────────────────────────────────────

SSTable::SSTable(std::string path, std::vector<LSMEntry> entries)
    : path_(std::move(path))
{
    std::sort(entries.begin(), entries.end(), [](const LSMEntry& a, const LSMEntry& b) {
        return a.key < b.key;
    });
    entries_ = std::move(entries);
    if (!entries_.empty()) {
        min_key_ = entries_.front().key;
        max_key_ = entries_.back().key;
    }
    write(entries_);
    loaded_ = true;
}

SSTable::SSTable(std::string path) : path_(std::move(path)) {}

bool SSTable::write(const std::vector<LSMEntry>& entries) {
    std::ofstream f(path_, std::ios::binary);
    if (!f) return false;

    uint64_t count = entries.size();
    f.write(reinterpret_cast<const char*>(&count), 8);

    for (const auto& e : entries) {
        uint16_t klen = e.key.size();
        uint32_t vlen = e.value.size();
        uint8_t  del  = e.deleted ? 1 : 0;
        f.write(reinterpret_cast<const char*>(&klen), 2);
        f.write(e.key.data(), klen);
        f.write(reinterpret_cast<const char*>(&del), 1);
        f.write(reinterpret_cast<const char*>(&vlen), 4);
        if (!e.deleted) f.write(e.value.data(), vlen);
        f.write(reinterpret_cast<const char*>(&e.seq_num), 8);
    }
    return f.good();
}

bool SSTable::load() {
    std::ifstream f(path_, std::ios::binary);
    if (!f) return false;

    uint64_t count;
    if (!f.read(reinterpret_cast<char*>(&count), 8)) return false;

    entries_.clear();
    entries_.reserve(count);

    for (uint64_t i = 0; i < count; ++i) {
        LSMEntry e;
        uint16_t klen; uint32_t vlen; uint8_t del;
        if (!f.read(reinterpret_cast<char*>(&klen), 2)) break;
        e.key.resize(klen);
        if (!f.read(e.key.data(), klen)) break;
        if (!f.read(reinterpret_cast<char*>(&del), 1)) break;
        e.deleted = del;
        if (!f.read(reinterpret_cast<char*>(&vlen), 4)) break;
        if (!e.deleted) {
            e.value.resize(vlen);
            if (!f.read(e.value.data(), vlen)) break;
        }
        if (!f.read(reinterpret_cast<char*>(&e.seq_num), 8)) break;
        entries_.push_back(std::move(e));
    }

    if (!entries_.empty()) {
        min_key_ = entries_.front().key;
        max_key_ = entries_.back().key;
    }
    loaded_ = true;
    return true;
}

std::optional<LSMEntry> SSTable::get(const std::string& key) const {
    if (!loaded_ || key < min_key_ || key > max_key_) return std::nullopt;
    auto it = std::lower_bound(entries_.begin(), entries_.end(), key,
        [](const LSMEntry& e, const std::string& k) { return e.key < k; });
    if (it != entries_.end() && it->key == key) return *it;
    return std::nullopt;
}

bool SSTable::mayContain(const std::string& key) const {
    return key >= min_key_ && key <= max_key_;
}

std::vector<LSMEntry> SSTable::scan(const std::string& from, const std::string& to) const {
    std::vector<LSMEntry> result;
    auto it = std::lower_bound(entries_.begin(), entries_.end(), from,
        [](const LSMEntry& e, const std::string& k) { return e.key < k; });
    while (it != entries_.end() && it->key <= to) {
        if (!it->deleted) result.push_back(*it);
        ++it;
    }
    return result;
}

// ─── LSMTree ──────────────────────────────────────────────────────────────

LSMTree::LSMTree(std::string dir, size_t memtable_size)
    : dir_(std::move(dir)), memtable_(memtable_size) {}

LSMTree::~LSMTree() {}

bool LSMTree::put(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mu_);
    memtable_.put(key, value, seq_++);
    if (memtable_.isFull()) flushMemTable();
    return true;
}

bool LSMTree::del(const std::string& key) {
    std::lock_guard<std::mutex> lock(mu_);
    memtable_.del(key, seq_++);
    return true;
}

std::optional<std::string> LSMTree::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(mu_);

    auto mem_entry = memtable_.get(key);
    if (mem_entry) {
        if (mem_entry->deleted) return std::nullopt;
        return mem_entry->value;
    }

    for (auto it = sstables_.rbegin(); it != sstables_.rend(); ++it) {
        auto e = (*it)->get(key);
        if (e) {
            if (e->deleted) return std::nullopt;
            return e->value;
        }
    }

    return std::nullopt;
}

std::string LSMTree::newSSTablePath() {
    return dir_ + "/sst_" + std::to_string(seq_) + ".sst";
}

void LSMTree::flushMemTable() {
    auto entries = memtable_.flush();
    if (entries.empty()) return;

    auto path = newSSTablePath();
    auto sst = std::make_shared<SSTable>(path, std::move(entries));
    sstables_.push_back(sst);
    memtable_.clear();
    std::cout << "Flushed memtable -> " << path << "\n";
}

void LSMTree::compact() {
    std::lock_guard<std::mutex> lock(mu_);
    if (sstables_.size() < 2) return;

    std::map<std::string, LSMEntry> merged;
    for (auto& sst : sstables_) {
        auto entries = sst->scan("", "\xff\xff\xff\xff\xff");
        for (auto& e : entries) {
            auto it = merged.find(e.key);
            if (it == merged.end() || it->second.seq_num < e.seq_num)
                merged[e.key] = e;
        }
    }

    std::vector<LSMEntry> compacted;
    for (auto& [k, v] : merged) {
        if (!v.deleted) compacted.push_back(v);
    }

    sstables_.clear();
    if (!compacted.empty()) {
        auto path = newSSTablePath();
        sstables_.push_back(std::make_shared<SSTable>(path, std::move(compacted)));
    }
    std::cout << "Compaction done: " << merged.size() << " entries merged\n";
}

} // namespace storage
} // namespace spatialdb
