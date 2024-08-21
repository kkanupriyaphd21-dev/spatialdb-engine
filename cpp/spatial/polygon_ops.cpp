#include "polygon_ops.h"
#include <cmath>
#include <algorithm>

namespace spatialdb {
namespace spatial {

geometry::Polygon clipPolygonByBBox(const geometry::Polygon& poly,
                                     const geometry::BBox& clip) {
    if (poly.vertices.empty()) return poly;

    auto inside = [&](const geometry::Point& p, int edge) -> bool {
        switch (edge) {
            case 0: return p.lat >= clip.min_lat;
            case 1: return p.lat <= clip.max_lat;
            case 2: return p.lon >= clip.min_lon;
            case 3: return p.lon <= clip.max_lon;
        }
        return false;
    };

    auto intersect = [&](const geometry::Point& a, const geometry::Point& b, int edge) {
        double t;
        switch (edge) {
            case 0: t = (clip.min_lat - a.lat) / (b.lat - a.lat); break;
            case 1: t = (clip.max_lat - a.lat) / (b.lat - a.lat); break;
            case 2: t = (clip.min_lon - a.lon) / (b.lon - a.lon); break;
            case 3: t = (clip.max_lon - a.lon) / (b.lon - a.lon); break;
            default: t = 0;
        }
        return geometry::Point{a.lat + t*(b.lat-a.lat), a.lon + t*(b.lon-a.lon)};
    };

    std::vector<geometry::Point> output = poly.vertices;

    for (int edge = 0; edge < 4; ++edge) {
        if (output.empty()) break;
        std::vector<geometry::Point> input = output;
        output.clear();

        for (size_t i = 0; i < input.size(); ++i) {
            const auto& cur  = input[i];
            const auto& prev = input[(i + input.size() - 1) % input.size()];

            if (inside(cur, edge)) {
                if (!inside(prev, edge))
                    output.push_back(intersect(prev, cur, edge));
                output.push_back(cur);
            } else if (inside(prev, edge)) {
                output.push_back(intersect(prev, cur, edge));
            }
        }
    }

    geometry::Polygon result;
    for (auto& v : output) result.addVertex(v);
    return result;
}

double polygonAreaKm2(const geometry::Polygon& poly) {
    if (poly.vertices.size() < 3) return 0.0;
    double area = 0.0;
    size_t n = poly.vertices.size();
    for (size_t i = 0; i < n; ++i) {
        const auto& a = poly.vertices[i];
        const auto& b = poly.vertices[(i+1) % n];
        area += (b.lon - a.lon) * geometry::DEG_TO_RAD *
                (2 + std::sin(a.lat * geometry::DEG_TO_RAD) +
                     std::sin(b.lat * geometry::DEG_TO_RAD));
    }
    return std::abs(area) * geometry::EARTH_RADIUS_KM * geometry::EARTH_RADIUS_KM / 2.0;
}

static double pointToSegmentDist(const geometry::Point& p,
                                   const geometry::Point& a,
                                   const geometry::Point& b) {
    double dx = b.lat - a.lat, dy = b.lon - a.lon;
    double len2 = dx*dx + dy*dy;
    if (len2 == 0) return geometry::haversineDistance(p, a);
    double t = std::max(0.0, std::min(1.0, ((p.lat-a.lat)*dx + (p.lon-a.lon)*dy) / len2));
    geometry::Point proj{a.lat + t*dx, a.lon + t*dy};
    return geometry::haversineDistance(p, proj);
}

geometry::Polygon simplifyPolygon(const geometry::Polygon& poly, double epsilon_km) {
    if (poly.vertices.size() <= 2) return poly;

    // RDP recursive
    std::function<std::vector<geometry::Point>(size_t, size_t)> rdp;
    rdp = [&](size_t start, size_t end) -> std::vector<geometry::Point> {
        if (end - start < 2) return {poly.vertices[start]};

        double max_dist = 0;
        size_t max_idx  = start;
        for (size_t i = start + 1; i < end; ++i) {
            double d = pointToSegmentDist(poly.vertices[i],
                                           poly.vertices[start],
                                           poly.vertices[end]);
            if (d > max_dist) { max_dist = d; max_idx = i; }
        }

        if (max_dist > epsilon_km) {
            auto left  = rdp(start, max_idx);
            auto right = rdp(max_idx, end);
            left.insert(left.end(), right.begin(), right.end());
            return left;
        }
        return {poly.vertices[start]};
    };

    auto simplified = rdp(0, poly.vertices.size() - 1);
    simplified.push_back(poly.vertices.back());

    geometry::Polygon result;
    for (auto& v : simplified) result.addVertex(v);
    return result;
}

geometry::Point polygonCentroid(const geometry::Polygon& poly) {
    if (poly.vertices.empty()) return {};
    double lat_sum = 0, lon_sum = 0;
    for (const auto& v : poly.vertices) { lat_sum += v.lat; lon_sum += v.lon; }
    size_t n = poly.vertices.size();
    return {lat_sum / n, lon_sum / n};
}

bool polygonsOverlap(const geometry::Polygon& a, const geometry::Polygon& b) {
    // Quick test: check if any vertex of A is inside B and vice versa
    for (const auto& v : a.vertices)
        if (geometry::pointInPolygon(v, b)) return true;
    for (const auto& v : b.vertices)
        if (geometry::pointInPolygon(v, a)) return true;
    return false;
}

geometry::Polygon bufferPolygon(const geometry::Polygon& poly, double dist_km) {
    if (poly.vertices.empty()) return poly;
    auto center = polygonCentroid(poly);
    double scale = 1.0 + dist_km / 100.0; // simplified radial expansion

    geometry::Polygon result;
    for (const auto& v : poly.vertices) {
        double dlat = (v.lat - center.lat) * scale;
        double dlon = (v.lon - center.lon) * scale;
        result.addVertex({center.lat + dlat, center.lon + dlon});
    }
    return result;
}

} // namespace spatial
} // namespace spatialdb
