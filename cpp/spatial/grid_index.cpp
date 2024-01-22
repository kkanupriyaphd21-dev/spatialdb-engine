#include "grid_index.h"
#include <algorithm>

namespace spatialdb {
namespace spatial {

GridIndex::GridIndex(double cell_size_km)
    : cell_size_km_(cell_size_km)
{
    lat_step_ = cell_size_km / geometry::EARTH_RADIUS_KM * (180.0 / M_PI);
    lon_step_ = lat_step_; // simplified — use equator approx
}

GridIndex::CellKey GridIndex::cellFor(const std::string& col, const geometry::Point& p) const {
    return {(int)std::floor(p.lat / lat_step_),
            (int)std::floor(p.lon / lon_step_),
            col};
}

bool GridIndex::insert(const index::IndexEntry& entry) {
    auto key = cellFor(entry.collection, entry.point);
    cells_[key].push_back(entry);
    counts_[entry.collection]++;
    return true;
}

bool GridIndex::remove(const std::string& collection, const std::string& id) {
    for (auto& [key, entries] : cells_) {
        if (key.collection != collection) continue;
        auto it = std::remove_if(entries.begin(), entries.end(),
            [&id](const index::IndexEntry& e) { return e.id == id; });
        if (it != entries.end()) {
            entries.erase(it, entries.end());
            counts_[collection]--;
            return true;
        }
    }
    return false;
}

std::vector<GridIndex::CellKey> GridIndex::cellsForBBox(
    const std::string& col, const geometry::BBox& bbox) const
{
    int lat_min = (int)std::floor(bbox.min_lat / lat_step_);
    int lat_max = (int)std::floor(bbox.max_lat / lat_step_);
    int lon_min = (int)std::floor(bbox.min_lon / lon_step_);
    int lon_max = (int)std::floor(bbox.max_lon / lon_step_);

    std::vector<CellKey> keys;
    for (int la = lat_min; la <= lat_max; ++la) {
        for (int lo = lon_min; lo <= lon_max; ++lo) {
            keys.push_back({la, lo, col});
        }
    }
    return keys;
}

std::vector<index::IndexEntry> GridIndex::searchBBox(
    const std::string& collection, const geometry::BBox& bbox) const
{
    std::vector<index::IndexEntry> results;
    for (const auto& key : cellsForBBox(collection, bbox)) {
        auto it = cells_.find(key);
        if (it == cells_.end()) continue;
        for (const auto& e : it->second) {
            if (bbox.contains(e.point)) results.push_back(e);
        }
    }
    return results;
}

std::vector<index::IndexEntry> GridIndex::searchRadius(
    const std::string& collection,
    const geometry::Circle& circle,
    size_t limit) const
{
    double lat_delta = circle.radius_km / geometry::EARTH_RADIUS_KM * (180.0 / M_PI);
    geometry::BBox bbox {
        circle.center.lat - lat_delta, circle.center.lon - lat_delta,
        circle.center.lat + lat_delta, circle.center.lon + lat_delta
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

size_t GridIndex::size(const std::string& collection) const {
    auto it = counts_.find(collection);
    return it == counts_.end() ? 0 : it->second;
}

void GridIndex::clear(const std::string& collection) {
    for (auto it = cells_.begin(); it != cells_.end(); ) {
        if (it->first.collection == collection) it = cells_.erase(it);
        else ++it;
    }
    counts_.erase(collection);
}

} // namespace spatial
} // namespace spatialdb
