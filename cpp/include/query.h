#pragma once
#include "geometry.h"
#include "index.h"
#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <optional>

namespace spatialdb {
namespace query {

enum class QueryType {
    NEARBY,
    WITHIN_BBOX,
    WITHIN_POLYGON,
    SCAN,
    COUNT,
};

enum class SortOrder {
    ASC,
    DESC,
    NONE,
};

struct QueryOptions {
    size_t      limit  = 100;
    size_t      offset = 0;
    SortOrder   sort   = SortOrder::NONE;
    bool        sparse = false;
    std::string cursor;

    std::optional<std::string> fieldFilter;
};

struct NearbyQuery {
    std::string         collection;
    geometry::Circle    circle;
    QueryOptions        opts;
};

struct BBoxQuery {
    std::string      collection;
    geometry::BBox   bbox;
    QueryOptions     opts;
};

struct QueryResult {
    std::vector<index::IndexEntry> entries;
    std::string                    next_cursor;
    size_t                         total_count = 0;
    bool                           has_more    = false;
};

class QueryEngine {
public:
    explicit QueryEngine(std::shared_ptr<index::SpatialIndex> idx);
    ~QueryEngine();

    QueryResult executeNearby(const NearbyQuery& q) const;
    QueryResult executeBBox(const BBoxQuery& q) const;
    size_t      count(const std::string& collection) const;

private:
    std::shared_ptr<index::SpatialIndex> index_;
};

} // namespace query
} // namespace spatialdb
