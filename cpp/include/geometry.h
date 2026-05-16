#pragma once
#include <vector>
#include <string>
#include <cmath>

namespace spatialdb {
namespace geometry {

const double EARTH_RADIUS_KM = 6371.0;
const double DEG_TO_RAD = M_PI / 180.0;

struct Point {
    double lat;
    double lon;

    Point() : lat(0.0), lon(0.0) {}
    Point(double lat, double lon) : lat(lat), lon(lon) {}

    bool operator==(const Point& other) const {
        return lat == other.lat && lon == other.lon;
    }
};

struct BBox {
    double min_lat, min_lon;
    double max_lat, max_lon;

    BBox() : min_lat(0), min_lon(0), max_lat(0), max_lon(0) {}
    BBox(double min_lat, double min_lon, double max_lat, double max_lon)
        : min_lat(min_lat), min_lon(min_lon), max_lat(max_lat), max_lon(max_lon) {}

    bool contains(const Point& p) const {
        return p.lat >= min_lat && p.lat <= max_lat &&
               p.lon >= min_lon && p.lon <= max_lon;
    }

    bool intersects(const BBox& other) const {
        return !(other.min_lat > max_lat || other.max_lat < min_lat ||
                 other.min_lon > max_lon || other.max_lon < min_lon);
    }

    double area() const {
        return (max_lat - min_lat) * (max_lon - min_lon);
    }
};

struct Circle {
    Point  center;
    double radius_km;

    Circle() : center(0.0, 0.0), radius_km(0.0) {}
    Circle(Point c, double r) : center(c), radius_km(r) {}
};

struct Polygon {
    std::vector<Point> vertices;

    void addVertex(const Point& p) { vertices.push_back(p); }
    bool isValid() const { return vertices.size() >= 3; }
};

double haversineDistance(const Point& a, const Point& b);
bool   pointInPolygon(const Point& p, const Polygon& poly);
bool   circleContainsPoint(const Circle& c, const Point& p);
BBox   polygonBBox(const Polygon& poly);

} // namespace geometry
} // namespace spatialdb
