#include "../include/geometry.h"
#include <cmath>
#include <stdexcept>

namespace spatialdb {
namespace geometry {

double haversineDistance(const Point& a, const Point& b) {
    double lat1 = a.lat * DEG_TO_RAD;
    double lat2 = b.lat * DEG_TO_RAD;
    double dlat = (b.lat - a.lat) * DEG_TO_RAD;
    double dlon = (b.lon - a.lon) * DEG_TO_RAD;

    double sinDlat = std::sin(dlat / 2.0);
    double sinDlon = std::sin(dlon / 2.0);

    double h = sinDlat * sinDlat +
               std::cos(lat1) * std::cos(lat2) * sinDlon * sinDlon;

    double c = 2.0 * std::atan2(std::sqrt(h), std::sqrt(1.0 - h));
    return EARTH_RADIUS_KM * c;
}

bool circleContainsPoint(const Circle& c, const Point& p) {
    return haversineDistance(c.center, p) <= c.radius_km;
}

bool pointInPolygon(const Point& p, const Polygon& poly) {
    if (!poly.isValid()) return false;

    bool inside = false;
    size_t n = poly.vertices.size();

    for (size_t i = 0, j = n - 1; i < n; j = i++) {
        const Point& vi = poly.vertices[i];
        const Point& vj = poly.vertices[j];

        bool cond1 = (vi.lon > p.lon) != (vj.lon > p.lon);
        bool cond2 = (p.lat < (vj.lat - vi.lat) * (p.lon - vi.lon) /
                                   (vj.lon - vi.lon) + vi.lat);

        if (cond1 && cond2) inside = !inside;
    }

    return inside;
}

BBox polygonBBox(const Polygon& poly) {
    if (poly.vertices.empty()) return BBox();

    BBox box;
    box.min_lat = box.max_lat = poly.vertices[0].lat;
    box.min_lon = box.max_lon = poly.vertices[0].lon;

    for (const auto& v : poly.vertices) {
        if (v.lat < box.min_lat) box.min_lat = v.lat;
        if (v.lat > box.max_lat) box.max_lat = v.lat;
        if (v.lon < box.min_lon) box.min_lon = v.lon;
        if (v.lon > box.max_lon) box.max_lon = v.lon;
    }

    return box;
}

} // namespace geometry
} // namespace spatialdb
