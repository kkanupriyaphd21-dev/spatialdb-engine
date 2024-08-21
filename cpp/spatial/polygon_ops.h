#pragma once
#include "../include/geometry.h"
#include <vector>

namespace spatialdb {
namespace spatial {

// Sutherland-Hodgman polygon clipping
geometry::Polygon clipPolygonByBBox(const geometry::Polygon& poly,
                                     const geometry::BBox& clip);

// Compute polygon area in square km using haversine
double polygonAreaKm2(const geometry::Polygon& poly);

// Simplify polygon using Ramer-Douglas-Peucker
geometry::Polygon simplifyPolygon(const geometry::Polygon& poly,
                                   double epsilon_km = 0.01);

// Check if two polygons overlap
bool polygonsOverlap(const geometry::Polygon& a, const geometry::Polygon& b);

// Compute centroid
geometry::Point polygonCentroid(const geometry::Polygon& poly);

// Buffering: expand polygon outward by dist_km
geometry::Polygon bufferPolygon(const geometry::Polygon& poly, double dist_km);

} // namespace spatial
} // namespace spatialdb
