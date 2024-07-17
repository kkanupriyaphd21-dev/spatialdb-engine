#include "benchmark.h"
#include "../spatial/rtree.h"
#include "../spatial/geohash.h"
#include "../spatial/grid_index.h"
#include "../include/geometry.h"
#include <random>
#include <iostream>

using namespace spatialdb;

static std::vector<index::IndexEntry> generateEntries(size_t n) {
    std::mt19937_64 rng(42);
    std::uniform_real_distribution<double> lat_dist(-90.0, 90.0);
    std::uniform_real_distribution<double> lon_dist(-180.0, 180.0);

    std::vector<index::IndexEntry> entries;
    entries.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        entries.push_back({"obj_" + std::to_string(i),
                           {lat_dist(rng), lon_dist(rng)},
                           "vehicles",
                           (uint64_t)i});
    }
    return entries;
}

int main(int argc, char** argv) {
    auto& bm = bench::Benchmark::global();
    auto entries = generateEntries(100000);

    // R-tree insert
    auto rtree = std::make_shared<spatial::RTree>();
    bm.add("rtree/insert_100k",
        [&]() { rtree = std::make_shared<spatial::RTree>(); },
        [&]() {
            for (auto& e : entries) rtree->insert(e);
        }, 5);

    // R-tree NEARBY search
    bm.add("rtree/search_radius_1km",
        [&]() {
            rtree = std::make_shared<spatial::RTree>();
            for (auto& e : entries) rtree->insert(e);
        },
        [&]() {
            geometry::Circle c{{37.77, -122.41}, 1.0};
            rtree->searchRadius("vehicles", c, 100);
        }, 10000);

    // Geohash encode
    bm.add("geohash/encode_p9",
        nullptr,
        [&]() {
            spatial::Geohash::encode(37.77, -122.41, 9);
        }, 1000000);

    // GridIndex insert
    auto grid = std::make_shared<spatial::GridIndex>(0.5);
    bm.add("grid/insert_100k",
        [&]() { grid = std::make_shared<spatial::GridIndex>(0.5); },
        [&]() {
            for (auto& e : entries) grid->insert(e);
        }, 5);

    bm.run();
    return 0;
}
