#include "../include/geometry.h"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace spatialdb {
namespace geometry {

// Compute signed area of polygon using shoelace formula
static double signedArea(const Polygon& poly) {
    double area = 0.0;
    size_t n = poly.vertices.size();
    for (size_t i = 0; i < n; ++i) {
        const Point& cur  = poly.vertices[i];
        const Point& next = poly.vertices[(i + 1) % n];
        area += cur.lat  * next.lon;
        area -= next.lat * cur.lon;
    }
    return area / 2.0;
}

bool isClockwise(const Polygon& poly) {
    return signedArea(poly) < 0.0;
}

Polygon makeConvexHull(std::vector<Point> points) {
    size_t n = points.size();
    if (n < 3) {
        Polygon p;
        for (auto& pt : points) p.addVertex(pt);
        return p;
    }

    // sort by lat, then lon
    std::sort(points.begin(), points.end(), [](const Point& a, const Point& b) {
        return a.lat < b.lat || (a.lat == b.lat && a.lon < b.lon);
    });

    std::vector<Point> hull;

    // lower hull
    for (size_t i = 0; i < n; ++i) {
        while (hull.size() >= 2) {
            const Point& a = hull[hull.size()-2];
            const Point& b = hull[hull.size()-1];
            const Point& c = points[i];
            double cross = (b.lat - a.lat) * (c.lon - a.lon) -
                           (b.lon - a.lon) * (c.lat - a.lat);
            if (cross <= 0) hull.pop_back();
            else break;
        }
        hull.push_back(points[i]);
    }

    // upper hull
    size_t lower_size = hull.size();
    for (int i = (int)n - 2; i >= 0; --i) {
        while (hull.size() > lower_size) {
            const Point& a = hull[hull.size()-2];
            const Point& b = hull[hull.size()-1];
            const Point& c = points[i];
            double cross = (b.lat - a.lat) * (c.lon - a.lon) -
                           (b.lon - a.lon) * (c.lat - a.lat);
            if (cross <= 0) hull.pop_back();
            else break;
        }
        hull.push_back(points[i]);
    }

    hull.pop_back();

    Polygon result;
    for (auto& pt : hull) result.addVertex(pt);
    return result;
}

double polygonPerimeter(const Polygon& poly) {
    double peri = 0.0;
    size_t n = poly.vertices.size();
    for (size_t i = 0; i < n; ++i) {
        peri += haversineDistance(poly.vertices[i], poly.vertices[(i+1) % n]);
    }
    return peri;
}

} // namespace geometry
} // namespace spatialdb
