#include "aggregation.h"
#include "../spatial/geohash.h"
#include <cmath>
#include <algorithm>

namespace spatialdb {
namespace query {

std::vector<AggBucket> Aggregator::groupByGeohash(
    const std::vector<index::IndexEntry>& entries, int precision) const
{
    std::unordered_map<std::string, AggBucket> buckets;

    for (const auto& e : entries) {
        auto hash = spatial::Geohash::encode(e.point.lat, e.point.lon, precision);
        auto& b = buckets[hash];
        if (b.key.empty()) {
            b.key = hash;
            auto box = spatial::Geohash::decodeBBox(hash);
            b.bbox = box;
        }
        b.count++;
        b.sum_lat += e.point.lat;
        b.sum_lon += e.point.lon;
        b.ids.push_back(e.id);
    }

    std::vector<AggBucket> result;
    result.reserve(buckets.size());
    for (auto& [k, v] : buckets) {
        if (v.count > 0) {
            v.centroid = {v.sum_lat / v.count, v.sum_lon / v.count};
        }
        result.push_back(std::move(v));
    }

    std::sort(result.begin(), result.end(), [](const AggBucket& a, const AggBucket& b) {
        return a.count > b.count;
    });

    return result;
}

std::vector<AggBucket> Aggregator::groupByGrid(
    const std::vector<index::IndexEntry>& entries, double cell_size_km) const
{
    double lat_step = cell_size_km / geometry::EARTH_RADIUS_KM * (180.0 / M_PI);
    double lon_step = lat_step;

    std::unordered_map<std::string, AggBucket> buckets;

    for (const auto& e : entries) {
        int lat_idx = (int)std::floor(e.point.lat / lat_step);
        int lon_idx = (int)std::floor(e.point.lon / lon_step);
        std::string key = std::to_string(lat_idx) + ":" + std::to_string(lon_idx);

        auto& b = buckets[key];
        if (b.key.empty()) {
            b.key = key;
            b.bbox = {
                lat_idx * lat_step, lon_idx * lon_step,
                (lat_idx + 1) * lat_step, (lon_idx + 1) * lon_step
            };
        }
        b.count++;
        b.sum_lat += e.point.lat;
        b.sum_lon += e.point.lon;
    }

    std::vector<AggBucket> result;
    for (auto& [k, v] : buckets) {
        if (v.count > 0) v.centroid = {v.sum_lat / v.count, v.sum_lon / v.count};
        result.push_back(std::move(v));
    }

    std::sort(result.begin(), result.end(), [](const AggBucket& a, const AggBucket& b) {
        return a.count > b.count;
    });
    return result;
}

std::unordered_map<std::string, size_t> Aggregator::countByCollection(
    const std::vector<index::IndexEntry>& entries) const
{
    std::unordered_map<std::string, size_t> counts;
    for (const auto& e : entries) counts[e.collection]++;
    return counts;
}

Aggregator::SpatialStats Aggregator::computeStats(
    const std::vector<index::IndexEntry>& entries) const
{
    SpatialStats s;
    if (entries.empty()) return s;

    s.count = entries.size();
    s.bbox.min_lat = s.bbox.max_lat = entries[0].point.lat;
    s.bbox.min_lon = s.bbox.max_lon = entries[0].point.lon;

    double sum_lat = 0, sum_lon = 0;
    for (const auto& e : entries) {
        sum_lat += e.point.lat;
        sum_lon += e.point.lon;
        s.bbox.min_lat = std::min(s.bbox.min_lat, e.point.lat);
        s.bbox.max_lat = std::max(s.bbox.max_lat, e.point.lat);
        s.bbox.min_lon = std::min(s.bbox.min_lon, e.point.lon);
        s.bbox.max_lon = std::max(s.bbox.max_lon, e.point.lon);
    }
    s.centroid = {sum_lat / s.count, sum_lon / s.count};

    double var_lat = 0, var_lon = 0;
    for (const auto& e : entries) {
        var_lat += std::pow(e.point.lat - s.centroid.lat, 2);
        var_lon += std::pow(e.point.lon - s.centroid.lon, 2);
    }
    s.std_lat  = std::sqrt(var_lat / s.count);
    s.std_lon  = std::sqrt(var_lon / s.count);
    s.spread_km = geometry::haversineDistance(
        {s.bbox.min_lat, s.bbox.min_lon},
        {s.bbox.max_lat, s.bbox.max_lon});

    return s;
}

} // namespace query
} // namespace spatialdb
