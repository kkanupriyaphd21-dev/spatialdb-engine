#include "quadtree.h"
#include <stdexcept>

namespace spatialdb {
namespace spatial {

QuadTree::QuadTree(const geometry::BBox& world_bounds, int max_depth)
    : world_(world_bounds), max_depth_(max_depth) {}

bool QuadTree::insert(const index::IndexEntry& entry) {
    auto& root = roots_[entry.collection];
    if (!root) {
        root = std::make_unique<Node>(world_, 0);
    }
    if (insertNode(root.get(), entry)) {
        counts_[entry.collection]++;
        return true;
    }
    return false;
}

bool QuadTree::insertNode(Node* node, const index::IndexEntry& entry) {
    if (!node->bounds.contains(entry.point)) return false;

    if (node->is_leaf) {
        node->entries.push_back(entry);
        if ((int)node->entries.size() > MAX_LEAF_ENTRIES && node->depth < max_depth_) {
            subdivide(node);
        }
        return true;
    }

    int q = node->quadrant(entry.point);
    if (!node->children[q]) {
        node->children[q] = std::make_unique<Node>(node->childBounds(q), node->depth + 1);
    }
    return insertNode(node->children[q].get(), entry);
}

void QuadTree::subdivide(Node* node) {
    node->is_leaf = false;
    auto entries = std::move(node->entries);
    node->entries.clear();

    for (auto& e : entries) {
        int q = node->quadrant(e.point);
        if (!node->children[q]) {
            node->children[q] = std::make_unique<Node>(node->childBounds(q), node->depth + 1);
        }
        node->children[q]->entries.push_back(e);
    }
}

void QuadTree::searchNode(const Node* node, const geometry::BBox& bbox,
                           std::vector<index::IndexEntry>& out) const {
    if (!node->bounds.intersects(bbox)) return;

    if (node->is_leaf) {
        for (const auto& e : node->entries) {
            if (bbox.contains(e.point)) out.push_back(e);
        }
        return;
    }

    for (const auto& child : node->children) {
        if (child) searchNode(child.get(), bbox, out);
    }
}

std::vector<index::IndexEntry> QuadTree::searchBBox(
    const std::string& collection, const geometry::BBox& bbox) const
{
    std::vector<index::IndexEntry> results;
    auto it = roots_.find(collection);
    if (it == roots_.end()) return results;
    searchNode(it->second.get(), bbox, results);
    return results;
}

std::vector<index::IndexEntry> QuadTree::searchRadius(
    const std::string& collection, const geometry::Circle& circle, size_t limit) const
{
    double lat_delta = circle.radius_km / geometry::EARTH_RADIUS_KM * (180.0 / M_PI);
    double lon_delta = lat_delta;
    geometry::BBox bbox {
        circle.center.lat - lat_delta, circle.center.lon - lon_delta,
        circle.center.lat + lat_delta, circle.center.lon + lon_delta
    };
    auto candidates = searchBBox(collection, bbox);
    std::vector<index::IndexEntry> results;
    for (auto& c : candidates) {
        if (geometry::circleContainsPoint(circle, c.point)) {
            results.push_back(c);
            if (results.size() >= limit) break;
        }
    }
    return results;
}

bool QuadTree::remove(const std::string& collection, const std::string& id) {
    counts_[collection]--;
    return true;
}

size_t QuadTree::size(const std::string& collection) const {
    auto it = counts_.find(collection);
    return it == counts_.end() ? 0 : it->second;
}

void QuadTree::clear(const std::string& collection) {
    roots_.erase(collection);
    counts_.erase(collection);
}

} // namespace spatial
} // namespace spatialdb
