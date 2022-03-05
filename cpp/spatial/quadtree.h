#pragma once
#include "../include/geometry.h"
#include "../include/index.h"
#include <memory>
#include <array>
#include <vector>

namespace spatialdb {
namespace spatial {

class QuadTree : public index::SpatialIndex {
public:
    explicit QuadTree(const geometry::BBox& world_bounds, int max_depth = 16);
    ~QuadTree() override = default;

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
    static const int MAX_LEAF_ENTRIES = 16;

    struct Node {
        geometry::BBox                   bounds;
        std::vector<index::IndexEntry>   entries;
        std::array<std::unique_ptr<Node>, 4> children;
        bool is_leaf = true;
        int  depth   = 0;

        explicit Node(const geometry::BBox& b, int d = 0) : bounds(b), depth(d) {}

        int quadrant(const geometry::Point& p) const {
            double mid_lat = (bounds.min_lat + bounds.max_lat) / 2.0;
            double mid_lon = (bounds.min_lon + bounds.max_lon) / 2.0;
            if (p.lat >= mid_lat) return p.lon >= mid_lon ? 0 : 1;
            else                  return p.lon >= mid_lon ? 2 : 3;
        }

        geometry::BBox childBounds(int q) const {
            double mid_lat = (bounds.min_lat + bounds.max_lat) / 2.0;
            double mid_lon = (bounds.min_lon + bounds.max_lon) / 2.0;
            switch (q) {
                case 0: return {mid_lat, mid_lon, bounds.max_lat, bounds.max_lon};
                case 1: return {mid_lat, bounds.min_lon, bounds.max_lat, mid_lon};
                case 2: return {bounds.min_lat, mid_lon, mid_lat, bounds.max_lon};
                case 3: return {bounds.min_lat, bounds.min_lon, mid_lat, mid_lon};
                default: return bounds;
            }
        }
    };

    std::unordered_map<std::string, std::unique_ptr<Node>> roots_;
    std::unordered_map<std::string, size_t> counts_;
    geometry::BBox world_;
    int max_depth_;

    bool insertNode(Node* node, const index::IndexEntry& entry);
    void subdivide(Node* node);
    void searchNode(const Node* node, const geometry::BBox& bbox,
                    std::vector<index::IndexEntry>& out) const;
};

} // namespace spatial
} // namespace spatialdb
