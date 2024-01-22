#pragma once
#include "../include/geometry.h"
#include "../include/index.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <cmath>

namespace spatialdb {
namespace spatial {

// Fixed-resolution grid index for O(1) average insert/lookup
class GridIndex : public index::SpatialIndex {
public:
    // cell_size_km controls resolution — smaller = faster point lookup, more memory
    explicit GridIndex(double cell_size_km = 1.0);
    ~GridIndex() override = default;

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

    double cellSizeKm() const { return cell_size_km_; }

private:
    double cell_size_km_;
    double lat_step_;
    double lon_step_;

    struct CellKey {
        int lat_idx;
        int lon_idx;
        std::string collection;

        bool operator==(const CellKey& o) const {
            return lat_idx == o.lat_idx && lon_idx == o.lon_idx &&
                   collection == o.collection;
        }
    };

    struct CellKeyHash {
        size_t operator()(const CellKey& k) const {
            size_t h = std::hash<int>{}(k.lat_idx);
            h ^= std::hash<int>{}(k.lon_idx) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<std::string>{}(k.collection) + 0x9e3779b9 + (h << 6) + (h >> 2);
            return h;
        }
    };

    std::unordered_map<CellKey, std::vector<index::IndexEntry>, CellKeyHash> cells_;
    std::unordered_map<std::string, size_t> counts_;

    CellKey cellFor(const std::string& collection, const geometry::Point& p) const;
    std::vector<CellKey> cellsForBBox(const std::string& collection,
                                       const geometry::BBox& bbox) const;
};

} // namespace spatial
} // namespace spatialdb
