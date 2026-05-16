#include "rtree.h"
#include "../include/geometry.h"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <cmath>
#include <unordered_set>

namespace spatialdb {
namespace spatial {

RTree::RTree() {}

bool RTree::insert(const index::IndexEntry& entry) {
    auto& root = roots_[entry.collection];
    if (!root) {
        root = std::make_shared<RTreeNode>();
        root->is_leaf = true;
    }

    std::shared_ptr<RTreeNode> split;
    root = insertEntry(root, entry, split);

    if (split) {
        auto new_root = std::make_shared<RTreeNode>();
        new_root->is_leaf = false;
        new_root->children.push_back(root);
        new_root->children.push_back(split);
        new_root->updateMBR();
        root = new_root;
    }

    counts_[entry.collection]++;
    return true;
}

std::shared_ptr<RTreeNode> RTree::insertEntry(
    std::shared_ptr<RTreeNode> node,
    const index::IndexEntry& entry,
    std::shared_ptr<RTreeNode>& split_out)
{
    if (node->is_leaf) {
        node->entries.push_back(entry);
        node->updateMBR();

        if ((int)node->entries.size() > RTREE_MAX_ENTRIES) {
            split_out = splitNode(node);
        }
        return node;
    }

    // choose best child (least enlargement)
    int best = 0;
    double best_enl = std::numeric_limits<double>::max();
    for (int i = 0; i < (int)node->children.size(); ++i) {
        double enl = enlargementNeeded(node->children[i]->mbr, entry.point);
        if (enl < best_enl) { best_enl = enl; best = i; }
    }

    std::shared_ptr<RTreeNode> child_split;
    node->children[best] = insertEntry(node->children[best], entry, child_split);

    if (child_split) {
        node->children.push_back(child_split);
        if ((int)node->children.size() > RTREE_MAX_ENTRIES) {
            split_out = splitNode(node);
        }
    }

    node->updateMBR();
    return node;
}

std::shared_ptr<RTreeNode> RTree::splitNode(std::shared_ptr<RTreeNode> node) {
    auto sibling = std::make_shared<RTreeNode>();
    sibling->is_leaf = node->is_leaf;

    if (node->is_leaf) {
        size_t mid = node->entries.size() / 2;
        sibling->entries.assign(node->entries.begin() + mid, node->entries.end());
        node->entries.erase(node->entries.begin() + mid, node->entries.end());
    } else {
        size_t mid = node->children.size() / 2;
        sibling->children.assign(node->children.begin() + mid, node->children.end());
        node->children.erase(node->children.begin() + mid, node->children.end());
    }

    node->updateMBR();
    sibling->updateMBR();
    return sibling;
}

double RTree::enlargementNeeded(const geometry::BBox& box, const geometry::Point& p) const {
    double new_min_lat = std::min(box.min_lat, p.lat);
    double new_max_lat = std::max(box.max_lat, p.lat);
    double new_min_lon = std::min(box.min_lon, p.lon);
    double new_max_lon = std::max(box.max_lon, p.lon);

    double old_area = (box.max_lat - box.min_lat) * (box.max_lon - box.min_lon);
    double new_area = (new_max_lat - new_min_lat) * (new_max_lon - new_min_lon);
    return new_area - old_area;
}

void RTree::searchNode(const RTreeNode* node, const geometry::BBox& bbox,
                        std::vector<index::IndexEntry>& results) const {
    if (!node->mbr.intersects(bbox)) return;

    if (node->is_leaf) {
        for (const auto& e : node->entries) {
            if (bbox.contains(e.point)) {
                results.push_back(e);
            }
        }
    } else {
        for (const auto& child : node->children) {
            searchNode(child.get(), bbox, results);
        }
    }
}

std::vector<index::IndexEntry> RTree::searchBBox(
    const std::string& collection,
    const geometry::BBox& bbox) const
{
    std::vector<index::IndexEntry> results;
    auto it = roots_.find(collection);
    if (it == roots_.end() || !it->second) return results;
    searchNode(it->second.get(), bbox, results);

    // Deduplicate by ID - overlapping nodes can return same entry
    std::unordered_set<std::string> seen;
    std::vector<index::IndexEntry> deduped;
    deduped.reserve(results.size());
    for (auto& e : results) {
        if (seen.insert(e.id).second) {
            deduped.push_back(std::move(e));
        }
    }
    return deduped;
}

std::vector<index::IndexEntry> RTree::searchRadius(
    const std::string& collection,
    const geometry::Circle& circle,
    size_t limit) const
{
    // Validate radius
    if (circle.radius_km <= 0.0) return {};

    // expand bbox then filter by actual distance
    double lat_delta = circle.radius_km / geometry::EARTH_RADIUS_KM *
                       (180.0 / M_PI);
    double lon_delta = lat_delta / std::cos(circle.center.lat * geometry::DEG_TO_RAD);

    geometry::BBox bbox{
        circle.center.lat - lat_delta,
        circle.center.lon - lon_delta,
        circle.center.lat + lat_delta,
        circle.center.lon + lon_delta
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

bool RTree::remove(const std::string& collection, const std::string& id) {
    // simple: mark deleted, rebuild periodically
    auto it = roots_.find(collection);
    if (it == roots_.end()) return false;
    // TODO: proper delete-and-reinsert
    counts_[collection]--;
    return true;
}

size_t RTree::size(const std::string& collection) const {
    auto it = counts_.find(collection);
    return it == counts_.end() ? 0 : it->second;
}

void RTree::clear(const std::string& collection) {
    roots_.erase(collection);
    counts_.erase(collection);
}

} // namespace spatial
} // namespace spatialdb
