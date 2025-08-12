#include <cassert>
#include <iostream>
#include <random>
#include "../rtree.h"
#include "../../include/geometry.h"

using namespace spatialdb;

static void testBasicInsertSearch() {
    spatial::RTree tree;

    tree.insert({"bus_1", {37.77,  -122.41}, "buses", 1000});
    tree.insert({"bus_2", {37.78,  -122.42}, "buses", 1001});
    tree.insert({"bus_3", {40.71,  -74.00},  "buses", 1002});

    geometry::BBox bbox{37.0, -123.0, 38.0, -122.0};
    auto results = tree.searchBBox("buses", bbox);
    assert(results.size() == 2 && "expected 2 results in SF bbox");

    geometry::Circle circle{{37.77, -122.41}, 5.0};
    auto nearby = tree.searchRadius("buses", circle, 10);
    assert(nearby.size() == 2 && "expected 2 results within 5km");

    std::cout << "testBasicInsertSearch: PASS\n";
}

static void testBulkLoad() {
    spatial::RTree tree;
    std::mt19937_64 rng(99);
    std::uniform_real_distribution<double> lat(-90, 90);
    std::uniform_real_distribution<double> lon(-180, 180);

    std::vector<index::IndexEntry> entries;
    for (int i = 0; i < 10000; ++i) {
        entries.push_back({"obj_" + std::to_string(i),
                           {lat(rng), lon(rng)}, "test", (uint64_t)i});
    }
    tree.bulkLoad("test", entries);
    assert(tree.size("test") == 10000 && "bulk load size mismatch");

    geometry::Circle circle{{0.0, 0.0}, 1000.0};
    auto results = tree.searchRadius("test", circle, 5000);
    assert(!results.empty() && "expected results in large radius search");

    std::cout << "testBulkLoad: PASS\n";
}

static void testRemoveAndClear() {
    spatial::RTree tree;
    tree.insert({"a", {1.0, 2.0}, "col", 0});
    tree.insert({"b", {3.0, 4.0}, "col", 1});

    bool removed = tree.remove("col", "a");
    assert(removed && "expected remove to succeed");
    assert(tree.size("col") == 1 && "expected 1 entry after remove");

    tree.clear("col");
    assert(tree.size("col") == 0 && "expected empty after clear");

    std::cout << "testRemoveAndClear: PASS\n";
}

int main() {
    testBasicInsertSearch();
    testBulkLoad();
    testRemoveAndClear();
    std::cout << "All R-tree tests passed.\n";
    return 0;
}
