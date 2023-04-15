#pragma once
#include "../../include/geometry.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

namespace spatialdb {
namespace spatial {
namespace fence {

enum class FenceEvent {
    ENTER,
    EXIT,
    CROSS,
};

enum class FenceType {
    CIRCLE,
    POLYGON,
    BBOX,
};

struct Fence {
    std::string id;
    std::string collection;
    FenceType   type;

    geometry::Circle  circle;
    geometry::Polygon polygon;
    geometry::BBox    bbox;

    std::vector<std::string> detect_events; // "enter", "exit", "cross"
};

struct FenceMatch {
    std::string fence_id;
    std::string object_id;
    FenceEvent  event;
    geometry::Point  point;
    double      distance_km = 0.0;
};

using FenceCallback = std::function<void(const FenceMatch&)>;

class GeoFenceManager {
public:
    GeoFenceManager();
    ~GeoFenceManager();

    bool addFence(Fence fence);
    bool removeFence(const std::string& id);
    bool hasFence(const std::string& id) const;

    std::vector<FenceMatch> test(const std::string& object_id,
                                  const geometry::Point& prev_pos,
                                  const geometry::Point& new_pos,
                                  const std::string& collection) const;

    void setCallback(FenceCallback cb);
    size_t fenceCount() const { return fences_.size(); }

private:
    std::unordered_map<std::string, Fence> fences_;
    FenceCallback                           callback_;

    bool pointInFence(const Fence& f, const geometry::Point& p) const;
    FenceEvent classifyTransition(bool was_inside, bool is_inside) const;
};

} // namespace fence
} // namespace spatial
} // namespace spatialdb
