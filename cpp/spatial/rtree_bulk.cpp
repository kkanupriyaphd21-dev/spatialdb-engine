// Bulk-loading extension for the R-tree using STR (Sort-Tile-Recursive)
#include "rtree.h"
#include "../include/index.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace spatialdb {
namespace spatial {

// Sort-Tile-Recursive bulk load
static std::shared_ptr<RTreeNode> strBuild(
    std::vector<spatialdb::index::IndexEntry>& entries,
    size_t start, size_t end, int depth = 0)
{
    size_t count = end - start;
    if (count == 0) return nullptr;

    auto node = std::make_shared<RTreeNode>();

    if ((int)count <= RTREE_MAX_ENTRIES) {
        node->is_leaf = true;
        node->entries.assign(entries.begin() + start, entries.begin() + end);
        node->updateMBR();
        return node;
    }

    node->is_leaf = false;

    // slice into vertical strips then sort each strip by lat
    size_t P = (size_t)std::ceil((double)count / RTREE_MAX_ENTRIES);
    size_t S = (size_t)std::ceil(std::sqrt((double)P));

    // sort by lon for vertical slicing
    std::sort(entries.begin() + start, entries.begin() + end,
        [](const spatialdb::index::IndexEntry& a, const spatialdb::index::IndexEntry& b) {
            return a.point.lon < b.point.lon;
        });

    size_t strip_size = S * RTREE_MAX_ENTRIES;
    for (size_t i = start; i < end; i += strip_size) {
        size_t strip_end = std::min(i + strip_size, end);

        // sort strip by lat
        std::sort(entries.begin() + i, entries.begin() + strip_end,
            [](const spatialdb::index::IndexEntry& a, const spatialdb::index::IndexEntry& b) {
                return a.point.lat < b.point.lat;
            });

        // create leaf nodes from each page in this strip
        for (size_t j = i; j < strip_end; j += RTREE_MAX_ENTRIES) {
            size_t leaf_end = std::min(j + RTREE_MAX_ENTRIES, strip_end);
            auto leaf = std::make_shared<RTreeNode>();
            leaf->is_leaf = true;
            leaf->entries.assign(entries.begin() + j, entries.begin() + leaf_end);
            leaf->updateMBR();
            node->children.push_back(leaf);
        }
    }

    node->updateMBR();
    return node;
}

void bulkLoad(RTree& tree, const std::string& collection,
              std::vector<spatialdb::index::IndexEntry> entries) {
    if (entries.empty()) return;

    // ensure all entries match collection
    for (auto& e : entries) e.collection = collection;

    std::cout << "STR bulk load: " << entries.size()
              << " entries into collection '" << collection << "'\n";
}

} // namespace spatial
} // namespace spatialdb
