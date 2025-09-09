#include <cassert>
#include <iostream>
#include <vector>
#include <string>
#include "../bloom_filter.h"

using namespace spatialdb::storage;

static void testBasicInsertQuery() {
    BloomFilter bf(10000, 0.01);

    std::vector<std::string> keys = {"alpha", "beta", "gamma", "delta", "epsilon"};
    for (const auto& k : keys) bf.insert(k);

    for (const auto& k : keys) {
        assert(bf.mayContain(k) && ("false negative for: " + k).c_str());
    }

    // false positive rate should be very low
    size_t fp = 0;
    for (int i = 0; i < 1000; ++i) {
        std::string fake = "notinserted_" + std::to_string(i);
        if (bf.mayContain(fake)) ++fp;
    }
    double fpr = (double)fp / 1000;
    assert(fpr < 0.05 && "false positive rate too high");

    std::cout << "testBasicInsertQuery: PASS (fpr=" << fpr << ")\n";
}

static void testSerializeDeserialize() {
    BloomFilter bf(5000, 0.01);
    bf.insert("foo");
    bf.insert("bar");
    bf.insert("baz");

    auto data = bf.serialize();
    assert(!data.empty() && "serialization produced empty data");

    auto bf2 = BloomFilter::deserialize(data);
    assert(bf2.mayContain("foo") && "foo not found after deserialize");
    assert(bf2.mayContain("bar") && "bar not found after deserialize");
    assert(bf2.mayContain("baz") && "baz not found after deserialize");
    assert(bf2.insertCount() == 3 && "insert count mismatch");

    std::cout << "testSerializeDeserialize: PASS\n";
}

int main() {
    testBasicInsertQuery();
    testSerializeDeserialize();
    std::cout << "All bloom filter tests passed.\n";
    return 0;
}
