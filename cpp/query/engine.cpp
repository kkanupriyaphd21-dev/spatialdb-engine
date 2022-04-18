#include "../include/query.h"
#include "../include/geometry.h"
#include <algorithm>
#include <stdexcept>

namespace spatialdb {
namespace query {

QueryEngine::QueryEngine(std::shared_ptr<index::SpatialIndex> idx)
    : index_(std::move(idx)) {}

QueryEngine::~QueryEngine() = default;

QueryResult QueryEngine::executeNearby(const NearbyQuery& q) const {
    auto entries = index_->searchRadius(q.collection, q.circle, q.opts.limit + q.opts.offset);

    // sort by distance
    std::sort(entries.begin(), entries.end(), [&](const index::IndexEntry& a,
                                                   const index::IndexEntry& b) {
        double da = geometry::haversineDistance(q.circle.center, a.point);
        double db = geometry::haversineDistance(q.circle.center, b.point);
        return da < db;
    });

    QueryResult result;
    result.total_count = entries.size();

    size_t start = std::min(q.opts.offset, entries.size());
    size_t end   = std::min(start + q.opts.limit, entries.size());

    result.entries.assign(entries.begin() + start, entries.begin() + end);
    result.has_more = end < entries.size();

    return result;
}

QueryResult QueryEngine::executeBBox(const BBoxQuery& q) const {
    auto entries = index_->searchBBox(q.collection, q.bbox);

    if (q.opts.sort == SortOrder::ASC) {
        std::sort(entries.begin(), entries.end(), [](const index::IndexEntry& a,
                                                     const index::IndexEntry& b) {
            return a.timestamp < b.timestamp;
        });
    } else if (q.opts.sort == SortOrder::DESC) {
        std::sort(entries.begin(), entries.end(), [](const index::IndexEntry& a,
                                                     const index::IndexEntry& b) {
            return a.timestamp > b.timestamp;
        });
    }

    QueryResult result;
    result.total_count = entries.size();

    size_t start = std::min(q.opts.offset, entries.size());
    size_t end   = std::min(start + q.opts.limit, entries.size());
    result.entries.assign(entries.begin() + start, entries.begin() + end);
    result.has_more = end < entries.size();

    return result;
}

size_t QueryEngine::count(const std::string& collection) const {
    return index_->size(collection);
}

} // namespace query
} // namespace spatialdb
