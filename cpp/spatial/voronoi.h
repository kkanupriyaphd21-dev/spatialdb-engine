#pragma once
#include "../include/geometry.h"
#include <vector>
#include <optional>

namespace spatialdb {
namespace spatial {

struct VoronoiCell {
    geometry::Point     site;
    geometry::Polygon   boundary;
    std::vector<size_t> neighbor_indices;
};

struct VoronoiDiagram {
    std::vector<VoronoiCell> cells;
    geometry::BBox           bounds;

    // Find the cell index containing point p
    std::optional<size_t> findCell(const geometry::Point& p) const;
    // Find the k nearest sites to p
    std::vector<size_t>   nearestSites(const geometry::Point& p, size_t k) const;
};

VoronoiDiagram buildVoronoi(const std::vector<geometry::Point>& sites,
                              const geometry::BBox& bounds);

} // namespace spatial
} // namespace spatialdb
