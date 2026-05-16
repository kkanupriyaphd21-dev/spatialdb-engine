#include "../spatial/geohash.h"
#include <iostream>
#include <cmath>

using namespace spatialdb::spatial;

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) void name()
#define ASSERT_TRUE(x) do { \
    if (!(x)) { \
        std::cerr << "FAILED: " << #x << " is false" \
                  << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        tests_failed++; return; \
    } \
} while(0)
#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        std::cerr << "FAILED: " << #a << " != " << #b \
                  << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        tests_failed++; return; \
    } \
} while(0)
#define ASSERT_THROWS(expr) do { \
    bool threw = false; \
    try { expr; } catch (const std::exception&) { threw = true; } \
    if (!threw) { \
        std::cerr << "FAILED: expected exception at " << __FILE__ << ":" << __LINE__ << std::endl; \
        tests_failed++; return; \
    } \
} while(0)
#define RUN_TEST(name) do { \
    std::cout << "  Running " << #name << "... "; \
    try { name(); } catch (const std::exception& e) { \
        std::cerr << "CRASH: " << e.what() << std::endl; \
        tests_failed++; \
        continue; \
    } \
    std::cout << "OK" << std::endl; \
    tests_passed++; \
} while(0)

TEST(Geohash_ValidEncode) {
    auto h = Geohash::encode(40.7128, -74.0060, 6);
    ASSERT_EQ(h.size(), 6u);
}

TEST(Geohash_ZeroPrecision) {
    ASSERT_THROWS(Geohash::encode(40.0, -74.0, 0));
}

TEST(Geohash_NegativePrecision) {
    ASSERT_THROWS(Geohash::encode(40.0, -74.0, -5));
}

TEST(Geohash_LargePrecision) {
    // Should clamp to 12, not crash
    auto h = Geohash::encode(40.0, -74.0, 100);
    ASSERT_EQ(h.size(), 12u);
}

TEST(Geohash_LatOutOfRange) {
    ASSERT_THROWS(Geohash::encode(91.0, 0.0, 5));
    ASSERT_THROWS(Geohash::encode(-91.0, 0.0, 5));
}

TEST(Geohash_LonOutOfRange) {
    ASSERT_THROWS(Geohash::encode(0.0, 181.0, 5));
    ASSERT_THROWS(Geohash::encode(0.0, -181.0, 5));
}

TEST(Geohash_EmptyHashDecode) {
    ASSERT_THROWS(Geohash::decodeBBox(""));
}

TEST(Geohash_InvalidChar) {
    ASSERT_THROWS(Geohash::decodeBBox("dr5x!")); // '!' is not valid base32
}

TEST(Geohash_EmptyNeighbors) {
    ASSERT_THROWS(Geohash::neighbors(""));
}

TEST(Geohash_RoundTrip) {
    double lat = 40.7128, lon = -74.0060;
    auto h = Geohash::encode(lat, lon, 8);
    auto pt = Geohash::decode(h);
    // Should be within ~0.001 degrees
    ASSERT_TRUE(std::abs(pt.lat - lat) < 0.01);
    ASSERT_TRUE(std::abs(pt.lon - lon) < 0.01);
}

TEST(Geohash_Parent) {
    auto h = Geohash::encode(40.0, -74.0, 6);
    auto p = Geohash::parent(h);
    ASSERT_EQ(p.size(), 5u);
    ASSERT_EQ(Geohash::parent("x"), "");
}

TEST(Geohash_BBoxPrecision) {
    ASSERT_THROWS(Geohash::hashesForBBox({40.0, -74.0, 41.0, -73.0}, 0));
    ASSERT_THROWS(Geohash::hashesForBBox({40.0, -74.0, 41.0, -73.0}, -1));
}

int main() {
    std::cout << "Geohash Validation Tests" << std::endl;
    std::cout << "========================" << std::endl;

    RUN_TEST(Geohash_ValidEncode);
    RUN_TEST(Geohash_ZeroPrecision);
    RUN_TEST(Geohash_NegativePrecision);
    RUN_TEST(Geohash_LargePrecision);
    RUN_TEST(Geohash_LatOutOfRange);
    RUN_TEST(Geohash_LonOutOfRange);
    RUN_TEST(Geohash_EmptyHashDecode);
    RUN_TEST(Geohash_InvalidChar);
    RUN_TEST(Geohash_EmptyNeighbors);
    RUN_TEST(Geohash_RoundTrip);
    RUN_TEST(Geohash_Parent);
    RUN_TEST(Geohash_BBoxPrecision);

    std::cout << std::endl;
    std::cout << "Results: " << tests_passed << " passed, "
              << tests_failed << " failed" << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
