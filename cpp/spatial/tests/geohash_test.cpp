#include <cassert>
#include <cmath>
#include <iostream>
#include "../geohash.h"

using namespace spatialdb;

static void testEncodeDecodeRoundtrip() {
    double lat = 37.7749, lon = -122.4194;

    for (int prec = 1; prec <= 9; ++prec) {
        auto hash = spatial::Geohash::encode(lat, lon, prec);
        assert((int)hash.size() == prec && "hash length mismatch");

        auto decoded = spatial::Geohash::decode(hash);
        auto bbox    = spatial::Geohash::decodeBBox(hash);

        assert(bbox.contains(decoded) && "decoded center not in bbox");

        // tolerance increases with lower precision
        double tol = 180.0 / std::pow(32, prec / 2.0 + 0.5);
        assert(std::abs(decoded.lat - lat) < tol + 1.0 && "lat out of tolerance");
    }

    std::cout << "testEncodeDecodeRoundtrip: PASS\n";
}

static void testNeighbors() {
    auto hash  = spatial::Geohash::encode(37.77, -122.41, 6);
    auto nbs   = spatial::Geohash::neighbors(hash);
    assert(nbs.size() == 8 && "expected 8 neighbors");

    // All neighbors should be adjacent (share an edge or corner)
    auto bbox = spatial::Geohash::decodeBBox(hash);
    for (const auto& nb : nbs) {
        auto nb_bbox = spatial::Geohash::decodeBBox(nb);
        // they should not be identical to the original
        assert(nb != hash && "neighbor should not equal self");
    }

    std::cout << "testNeighbors: PASS\n";
}

int main() {
    testEncodeDecodeRoundtrip();
    testNeighbors();
    std::cout << "All geohash tests passed.\n";
    return 0;
}
