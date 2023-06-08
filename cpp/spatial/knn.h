#pragma once
#include "../include/geometry.h"
#include "../include/index.h"
#include <vector>
#include <queue>
#include <functional>

namespace spatialdb {
namespace spatial {

struct KNNResult {
    index::IndexEntry entry;
    double distance_km;

    bool operator>(const KNNResult& other) const {
        return distance_km > other.distance_km;
    }
};

// k-nearest-neighbor search using best-first traversal
class KNNSearch {
public:
    explicit KNNSearch(const index::SpatialIndex& idx);

    std::vector<KNNResult> search(const std::string& collection,
                                   const geometry::Point& query,
                                   size_t k,
                                   double max_radius_km = 1e9) const;

    std::vector<KNNResult> searchMultiCollection(
        const std::vector<std::string>& collections,
        const geometry::Point& query,
        size_t k,
        double max_radius_km = 1e9) const;

private:
    const index::SpatialIndex& index_;

    double expandRadius(size_t k) const;
};

} // namespace spatial
} // namespace spatialdb
