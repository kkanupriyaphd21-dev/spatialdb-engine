#include "tracker.h"
#include <cmath>
#include <algorithm>

namespace spatialdb {
namespace spatial {
namespace trajectory {

TrajectoryTracker::TrajectoryTracker(size_t max_points)
    : max_points_(max_points) {}

double TrajectoryTracker::computeHeading(const geometry::Point& from,
                                          const geometry::Point& to) const {
    double dlat = (to.lat - from.lat) * geometry::DEG_TO_RAD;
    double dlon = (to.lon - from.lon) * geometry::DEG_TO_RAD;
    double lat1 = from.lat * geometry::DEG_TO_RAD;
    double lat2 = to.lat   * geometry::DEG_TO_RAD;

    double x = std::sin(dlon) * std::cos(lat2);
    double y = std::cos(lat1) * std::sin(lat2) -
               std::sin(lat1) * std::cos(lat2) * std::cos(dlon);

    double bearing = std::atan2(x, y) * 180.0 / M_PI;
    return std::fmod(bearing + 360.0, 360.0);
}

double TrajectoryTracker::computeSpeed(const geometry::Point& from,
                                        const geometry::Point& to,
                                        uint64_t dt_ms) const {
    if (dt_ms == 0) return 0.0;
    double dist_km = geometry::haversineDistance(from, to);
    double hours   = dt_ms / 3600000.0;
    return dist_km / hours;
}

void TrajectoryTracker::update(const std::string& object_id,
                                 double lat, double lon,
                                 uint64_t timestamp_ms) {
    std::lock_guard<std::mutex> lock(mu_);
    auto& track = tracks_[object_id];

    TrajectoryPoint pt;
    pt.point        = {lat, lon};
    pt.timestamp_ms = timestamp_ms;

    if (!track.empty()) {
        const auto& prev = track.back();
        uint64_t dt = timestamp_ms > prev.timestamp_ms ?
                      timestamp_ms - prev.timestamp_ms : 0;
        pt.speed_kmh   = computeSpeed(prev.point, pt.point, dt);
        pt.heading_deg = computeHeading(prev.point, pt.point);
    }

    track.push_back(pt);
    if (track.size() > max_points_) track.pop_front();
}

std::vector<TrajectoryPoint> TrajectoryTracker::getTrajectory(
    const std::string& object_id, size_t limit) const
{
    std::lock_guard<std::mutex> lock(mu_);
    auto it = tracks_.find(object_id);
    if (it == tracks_.end()) return {};

    const auto& track = it->second;
    size_t take = limit == 0 ? track.size() : std::min(limit, track.size());

    return std::vector<TrajectoryPoint>(track.end() - take, track.end());
}

std::optional<TrajectoryPoint> TrajectoryTracker::getLastPoint(
    const std::string& object_id) const
{
    std::lock_guard<std::mutex> lock(mu_);
    auto it = tracks_.find(object_id);
    if (it == tracks_.end() || it->second.empty()) return std::nullopt;
    return it->second.back();
}

TrajectoryStats TrajectoryTracker::getStats(const std::string& object_id) const {
    std::lock_guard<std::mutex> lock(mu_);
    auto it = tracks_.find(object_id);
    if (it == tracks_.end()) return {};

    const auto& track = it->second;
    TrajectoryStats stats;
    stats.point_count = track.size();
    if (track.empty()) return stats;

    stats.bbox.min_lat = stats.bbox.max_lat = track[0].point.lat;
    stats.bbox.min_lon = stats.bbox.max_lon = track[0].point.lon;

    for (size_t i = 1; i < track.size(); ++i) {
        stats.total_dist_km += geometry::haversineDistance(
            track[i-1].point, track[i].point);
        stats.max_speed_kmh = std::max(stats.max_speed_kmh, track[i].speed_kmh);
        stats.bbox.min_lat  = std::min(stats.bbox.min_lat, track[i].point.lat);
        stats.bbox.max_lat  = std::max(stats.bbox.max_lat, track[i].point.lat);
        stats.bbox.min_lon  = std::min(stats.bbox.min_lon, track[i].point.lon);
        stats.bbox.max_lon  = std::max(stats.bbox.max_lon, track[i].point.lon);
    }

    stats.duration_ms  = track.back().timestamp_ms - track.front().timestamp_ms;
    double hours       = stats.duration_ms / 3600000.0;
    stats.avg_speed_kmh = hours > 0 ? stats.total_dist_km / hours : 0;

    return stats;
}

std::optional<geometry::Point> TrajectoryTracker::predictPosition(
    const std::string& object_id, uint64_t future_ms) const
{
    auto last = getLastPoint(object_id);
    if (!last) return std::nullopt;

    if (last->speed_kmh <= 0) return last->point;

    double dist_km   = last->speed_kmh * (future_ms / 3600000.0);
    double bearing   = last->heading_deg * geometry::DEG_TO_RAD;
    double lat_rad   = last->point.lat * geometry::DEG_TO_RAD;
    double lon_rad   = last->point.lon * geometry::DEG_TO_RAD;
    double d_over_r  = dist_km / geometry::EARTH_RADIUS_KM;

    double new_lat = std::asin(std::sin(lat_rad) * std::cos(d_over_r) +
                                std::cos(lat_rad) * std::sin(d_over_r) * std::cos(bearing));
    double new_lon = lon_rad + std::atan2(
        std::sin(bearing) * std::sin(d_over_r) * std::cos(lat_rad),
        std::cos(d_over_r) - std::sin(lat_rad) * std::sin(new_lat));

    return geometry::Point{new_lat / geometry::DEG_TO_RAD,
                            new_lon / geometry::DEG_TO_RAD};
}

void TrajectoryTracker::clear(const std::string& object_id) {
    std::lock_guard<std::mutex> lock(mu_);
    tracks_.erase(object_id);
}

size_t TrajectoryTracker::objectCount() const {
    std::lock_guard<std::mutex> lock(mu_);
    return tracks_.size();
}

} // namespace trajectory
} // namespace spatial
} // namespace spatialdb
