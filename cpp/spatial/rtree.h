#pragma once
#include "../include/geometry.h"
#include "../include/index.h"
#include <vector>
#include <memory>
#include <array>
#include <algorithm>

namespace spatialdb {
namespace spatial {

static const int RTREE_MAX_ENTRIES = 8;
static const int RTREE_MIN_ENTRIES = 3;

struct RTreeNode {
    bool   is_leaf;
    geometry::BBox mbr;
    std::vector<std::shared_ptr<RTreeNode>> children;
    std::vector<index::IndexEntry>          entries;

    RTreeNode() : is_leaf(true) {}

    void updateMBR() {
        if (is_leaf) {
            if (entries.empty()) return;
            mbr.min_lat = mbr.max_lat = entries[0].point.lat;
            mbr.min_lon = mbr.max_lon = entries[0].point.lon;
            for (auto& e : entries) {
                mbr.min_lat = std::min(mbr.min_lat, e.point.lat);
                mbr.max_lat = std::max(mbr.max_lat, e.point.lat);
                mbr.min_lon = std::min(mbr.min_lon, e.point.lon);
                mbr.max_lon = std::max(mbr.max_lon, e.point.lon);
            }
        } else {
            if (children.empty()) return;
            mbr = children[0]->mbr;
            for (auto& c : children) {
                mbr.min_lat = std::min(mbr.min_lat, c->mbr.min_lat);
                mbr.max_lat = std::max(mbr.max_lat, c->mbr.max_lat);
                mbr.min_lon = std::min(mbr.min_lon, c->mbr.min_lon);
                mbr.max_lon = std::max(mbr.max_lon, c->mbr.max_lon);
            }
        }
    }
};

class RTree : public index::SpatialIndex {
public:
    RTree();
    ~RTree() override = default;

    bool insert(const index::IndexEntry& entry) override;
    bool remove(const std::string& collection, const std::string& id) override;

    std::vector<index::IndexEntry> searchBBox(
        const std::string& collection,
        const geometry::BBox& bbox) const override;

    std::vector<index::IndexEntry> searchRadius(
        const std::string& collection,
        const geometry::Circle& circle,
        size_t limit) const override;

    size_t size(const std::string& collection) const override;
    void   clear(const std::string& collection) override;

private:
    std::unordered_map<std::string, std::shared_ptr<RTreeNode>> roots_;
    std::unordered_map<std::string, size_t> counts_;

    void searchNode(const RTreeNode* node, const geometry::BBox& bbox,
                    std::vector<index::IndexEntry>& results) const;

    std::shared_ptr<RTreeNode> insertEntry(std::shared_ptr<RTreeNode> node,
                                            const index::IndexEntry& entry,
                                            std::shared_ptr<RTreeNode>& split_out);

    std::shared_ptr<RTreeNode> splitNode(std::shared_ptr<RTreeNode> node);
    int chooseSplitAxis(const std::vector<index::IndexEntry>& entries);
    geometry::BBox enlargedBBox(const geometry::BBox& box, const geometry::Point& p) const;
    double enlargementNeeded(const geometry::BBox& box, const geometry::Point& p) const;
};

} // namespace spatial
} // namespace spatialdb
