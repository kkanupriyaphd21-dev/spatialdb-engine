#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <cmath>

namespace spatialdb {
namespace storage {

class BloomFilter {
public:
    explicit BloomFilter(size_t expected_items, double false_positive_rate = 0.01);
    explicit BloomFilter(size_t num_bits, int num_hashes);

    void   insert(const std::string& key);
    bool   mayContain(const std::string& key) const;
    void   clear();
    double estimatedFPR() const;

    size_t numBits()   const { return bits_.size(); }
    int    numHashes() const { return num_hashes_; }
    size_t insertCount() const { return insert_count_; }

    std::vector<uint8_t> serialize() const;
    static BloomFilter   deserialize(const std::vector<uint8_t>& data);

private:
    std::vector<bool> bits_;
    int               num_hashes_;
    size_t            insert_count_ = 0;

    uint64_t hash1(const std::string& key) const;
    uint64_t hash2(const std::string& key) const;
    size_t   bitIndex(int i, const std::string& key) const;
};

} // namespace storage
} // namespace spatialdb
