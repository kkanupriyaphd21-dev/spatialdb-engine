#pragma once
#include "../../include/geometry.h"
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <optional>

namespace spatialdb {
namespace spatial {
namespace trajectory {

struct TrajectoryPoint {
    geometry::Point point;
    uint64_t        timestamp_ms;
    double          speed_kmh   = 0.0;
    double          heading_deg = 0.0;
};

struct TrajectoryStats {
    size_t   point_count   = 0;
    double   total_dist_km = 0;
    double   avg_speed_kmh = 0;
    double   max_speed_kmh = 0;
    uint64_t duration_ms   = 0;
    geometry::BBox bbox;
};

class TrajectoryTracker {
public:
    explicit TrajectoryTracker(size_t max_points_per_object = 1000);

    void update(const std::string& object_id,
                double lat, double lon,
                uint64_t timestamp_ms);

    std::vector<TrajectoryPoint> getTrajectory(const std::string& object_id,
                                                size_t limit = 0) const;

    std::optional<TrajectoryPoint> getLastPoint(const std::string& object_id) const;
    TrajectoryStats                getStats(const std::string& object_id) const;

    // Predict next position based on current velocity
    std::optional<geometry::Point> predictPosition(
        const std::string& object_id, uint64_t future_ms) const;

    void   clear(const std::string& object_id);
    size_t objectCount() const;

private:
    size_t max_points_;
    mutable std::mutex mu_;
    std::unordered_map<std::string, std::deque<TrajectoryPoint>> tracks_;

    double computeHeading(const geometry::Point& from,
                           const geometry::Point& to) const;
    double computeSpeed(const geometry::Point& from,
                         const geometry::Point& to,
                         uint64_t dt_ms) const;
};

} // namespace trajectory
} // namespace spatial
} // namespace spatialdb
