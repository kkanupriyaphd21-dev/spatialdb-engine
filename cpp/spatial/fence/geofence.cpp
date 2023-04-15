#include "geofence.h"
#include <algorithm>

namespace spatialdb {
namespace spatial {
namespace fence {

GeoFenceManager::GeoFenceManager() {}
GeoFenceManager::~GeoFenceManager() {}

bool GeoFenceManager::addFence(Fence fence) {
    fences_[fence.id] = std::move(fence);
    return true;
}

bool GeoFenceManager::removeFence(const std::string& id) {
    return fences_.erase(id) > 0;
}

bool GeoFenceManager::hasFence(const std::string& id) const {
    return fences_.count(id) > 0;
}

bool GeoFenceManager::pointInFence(const Fence& f, const geometry::Point& p) const {
    switch (f.type) {
        case FenceType::CIRCLE:
            return geometry::circleContainsPoint(f.circle, p);
        case FenceType::POLYGON:
            return geometry::pointInPolygon(p, f.polygon);
        case FenceType::BBOX:
            return f.bbox.contains(p);
    }
    return false;
}

FenceEvent GeoFenceManager::classifyTransition(bool was_inside, bool is_inside) const {
    if (!was_inside && is_inside)  return FenceEvent::ENTER;
    if (was_inside  && !is_inside) return FenceEvent::EXIT;
    return FenceEvent::CROSS;
}

std::vector<FenceMatch> GeoFenceManager::test(
    const std::string& object_id,
    const geometry::Point& prev_pos,
    const geometry::Point& new_pos,
    const std::string& collection) const
{
    std::vector<FenceMatch> matches;

    for (const auto& [id, fence] : fences_) {
        if (!fence.collection.empty() && fence.collection != collection) continue;

        bool was_inside = pointInFence(fence, prev_pos);
        bool is_inside  = pointInFence(fence, new_pos);

        if (was_inside == is_inside && !is_inside) continue;

        FenceEvent event = classifyTransition(was_inside, is_inside);

        std::string event_name;
        switch (event) {
            case FenceEvent::ENTER: event_name = "enter"; break;
            case FenceEvent::EXIT:  event_name = "exit";  break;
            case FenceEvent::CROSS: event_name = "cross"; break;
        }

        bool should_fire = fence.detect_events.empty() ||
            std::find(fence.detect_events.begin(), fence.detect_events.end(),
                      event_name) != fence.detect_events.end();

        if (!should_fire) continue;

        FenceMatch m;
        m.fence_id  = id;
        m.object_id = object_id;
        m.event     = event;
        m.point     = new_pos;
        if (fence.type == FenceType::CIRCLE) {
            m.distance_km = geometry::haversineDistance(fence.circle.center, new_pos);
        }
        matches.push_back(std::move(m));

        if (callback_) callback_(matches.back());
    }

    return matches;
}

void GeoFenceManager::setCallback(FenceCallback cb) {
    callback_ = std::move(cb);
}

} // namespace fence
} // namespace spatial
} // namespace spatialdb
