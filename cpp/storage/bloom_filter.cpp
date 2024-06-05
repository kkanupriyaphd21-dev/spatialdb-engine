#include "bloom_filter.h"
#include <cstring>
#include <stdexcept>

namespace spatialdb {
namespace storage {

BloomFilter::BloomFilter(size_t expected_items, double fpr) {
    double ln2 = std::log(2.0);
    size_t m = (size_t)(-expected_items * std::log(fpr) / (ln2 * ln2));
    m = std::max(m, (size_t)64);
    num_hashes_ = std::max(1, (int)(m / expected_items * ln2));
    bits_.assign(m, false);
}

BloomFilter::BloomFilter(size_t num_bits, int num_hashes)
    : num_hashes_(num_hashes)
{
    bits_.assign(num_bits, false);
}

// FNV-1a
uint64_t BloomFilter::hash1(const std::string& key) const {
    uint64_t h = 14695981039346656037ULL;
    for (unsigned char c : key) {
        h ^= c;
        h *= 1099511628211ULL;
    }
    return h;
}

// djb2
uint64_t BloomFilter::hash2(const std::string& key) const {
    uint64_t h = 5381;
    for (unsigned char c : key) {
        h = ((h << 5) + h) + c;
    }
    return h;
}

// Double hashing: h(i) = h1(k) + i * h2(k)
size_t BloomFilter::bitIndex(int i, const std::string& key) const {
    uint64_t h = (hash1(key) + (uint64_t)i * hash2(key)) % bits_.size();
    return (size_t)h;
}

void BloomFilter::insert(const std::string& key) {
    for (int i = 0; i < num_hashes_; ++i) {
        bits_[bitIndex(i, key)] = true;
    }
    ++insert_count_;
}

bool BloomFilter::mayContain(const std::string& key) const {
    for (int i = 0; i < num_hashes_; ++i) {
        if (!bits_[bitIndex(i, key)]) return false;
    }
    return true;
}

void BloomFilter::clear() {
    std::fill(bits_.begin(), bits_.end(), false);
    insert_count_ = 0;
}

double BloomFilter::estimatedFPR() const {
    double k = num_hashes_;
    double m = (double)bits_.size();
    double n = (double)insert_count_;
    return std::pow(1.0 - std::exp(-k * n / m), k);
}

std::vector<uint8_t> BloomFilter::serialize() const {
    std::vector<uint8_t> out;
    // header: num_bits (8 bytes), num_hashes (4 bytes), insert_count (8 bytes)
    uint64_t nb = bits_.size();
    uint32_t nh = num_hashes_;
    uint64_t ic = insert_count_;
    auto push = [&](const void* d, size_t n) {
        const uint8_t* p = static_cast<const uint8_t*>(d);
        out.insert(out.end(), p, p + n);
    };
    push(&nb, 8); push(&nh, 4); push(&ic, 8);

    // pack bits into bytes
    for (size_t i = 0; i < bits_.size(); i += 8) {
        uint8_t byte = 0;
        for (int b = 0; b < 8 && i + b < bits_.size(); ++b) {
            if (bits_[i + b]) byte |= (1 << b);
        }
        out.push_back(byte);
    }
    return out;
}

BloomFilter BloomFilter::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 20) throw std::runtime_error("bloom: data too short");
    uint64_t nb; uint32_t nh; uint64_t ic;
    memcpy(&nb, data.data() + 0, 8);
    memcpy(&nh, data.data() + 8, 4);
    memcpy(&ic, data.data() + 12, 8);

    BloomFilter bf((size_t)nb, (int)nh);
    bf.insert_count_ = ic;

    for (size_t i = 0; i < nb && 20 + i/8 < data.size(); ++i) {
        uint8_t byte = data[20 + i/8];
        bf.bits_[i] = (byte >> (i % 8)) & 1;
    }
    return bf;
}

} // namespace storage
} // namespace spatialdb
