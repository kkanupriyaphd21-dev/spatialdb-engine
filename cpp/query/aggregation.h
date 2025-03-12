#pragma once
#include "../include/index.h"
#include "../include/geometry.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace spatialdb {
namespace query {

struct AggBucket {
    std::string              key;
    size_t                   count    = 0;
    double                   sum_lat  = 0;
    double                   sum_lon  = 0;
    geometry::Point          centroid;
    geometry::BBox           bbox;
    std::vector<std::string> ids;
};

class Aggregator {
public:
    // Group entries by geohash prefix at given precision
    std::vector<AggBucket> groupByGeohash(
        const std::vector<index::IndexEntry>& entries,
        int precision = 5) const;

    // Group entries into a grid of cell_size_km cells
    std::vector<AggBucket> groupByGrid(
        const std::vector<index::IndexEntry>& entries,
        double cell_size_km = 10.0) const;

    // Count entries per collection
    std::unordered_map<std::string, size_t> countByCollection(
        const std::vector<index::IndexEntry>& entries) const;

    // Compute spatial statistics
    struct SpatialStats {
        size_t          count    = 0;
        geometry::Point centroid;
        geometry::BBox  bbox;
        double          std_lat  = 0;
        double          std_lon  = 0;
        double          spread_km = 0;
    };

    SpatialStats computeStats(const std::vector<index::IndexEntry>& entries) const;
};

} // namespace query
} // namespace spatialdb
