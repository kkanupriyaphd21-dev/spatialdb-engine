#include "knn.h"
#include <algorithm>
#include <cmath>

namespace spatialdb {
namespace spatial {

KNNSearch::KNNSearch(const index::SpatialIndex& idx) : index_(idx) {}

double KNNSearch::expandRadius(size_t k) const {
    // Start with a radius proportional to sqrt(k), expand if needed
    return std::max(1.0, std::sqrt((double)k) * 0.5);
}

std::vector<KNNResult> KNNSearch::search(
    const std::string& collection,
    const geometry::Point& query,
    size_t k,
    double max_radius_km) const
{
    double radius = expandRadius(k);
    std::vector<KNNResult> results;

    // expand search radius until we have k results
    for (int attempt = 0; attempt < 8; ++attempt) {
        geometry::Circle circle{query, std::min(radius, max_radius_km)};
        auto candidates = index_.searchRadius(collection, circle, k * 4);

        results.clear();
        for (auto& c : candidates) {
            double d = geometry::haversineDistance(query, c.point);
            if (d <= max_radius_km) {
                results.push_back({c, d});
            }
        }

        if (results.size() >= k || radius >= max_radius_km) break;
        radius *= 2.0;
    }

    // sort by distance, take top-k
    std::sort(results.begin(), results.end(), [](const KNNResult& a, const KNNResult& b) {
        return a.distance_km < b.distance_km;
    });

    if (results.size() > k) results.resize(k);
    return results;
}

std::vector<KNNResult> KNNSearch::searchMultiCollection(
    const std::vector<std::string>& collections,
    const geometry::Point& query,
    size_t k,
    double max_radius_km) const
{
    std::vector<KNNResult> all;
    for (const auto& col : collections) {
        auto r = search(col, query, k, max_radius_km);
        all.insert(all.end(), r.begin(), r.end());
    }

    std::sort(all.begin(), all.end(), [](const KNNResult& a, const KNNResult& b) {
        return a.distance_km < b.distance_km;
    });

    if (all.size() > k) all.resize(k);
    return all;
}

} // namespace spatial
} // namespace spatialdb
