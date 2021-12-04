#pragma once
#include "geometry.h"
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>

namespace spatialdb {
namespace index {

struct IndexEntry {
    std::string id;
    geometry::Point point;
    std::string collection;
    uint64_t timestamp;

    IndexEntry() = default;
    IndexEntry(std::string id, geometry::Point pt, std::string col, uint64_t ts)
        : id(std::move(id)), point(pt), collection(std::move(col)), timestamp(ts) {}
};

class SpatialIndex {
public:
    virtual ~SpatialIndex() = default;
    virtual bool insert(const IndexEntry& entry) = 0;
    virtual bool remove(const std::string& collection, const std::string& id) = 0;
    virtual std::vector<IndexEntry> searchBBox(const std::string& collection,
                                                const geometry::BBox& bbox) const = 0;
    virtual std::vector<IndexEntry> searchRadius(const std::string& collection,
                                                  const geometry::Circle& circle,
                                                  size_t limit) const = 0;
    virtual size_t size(const std::string& collection) const = 0;
    virtual void clear(const std::string& collection) = 0;
};

} // namespace index
} // namespace spatialdb
