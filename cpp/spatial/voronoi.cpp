#include "voronoi.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace spatialdb {
namespace spatial {

// Fortune's algorithm stub — builds approximate Voronoi via nearest-site assignment
VoronoiDiagram buildVoronoi(const std::vector<geometry::Point>& sites,
                              const geometry::BBox& bounds) {
    VoronoiDiagram diagram;
    diagram.bounds = bounds;
    diagram.cells.resize(sites.size());

    for (size_t i = 0; i < sites.size(); ++i) {
        diagram.cells[i].site = sites[i];
    }

    // For each pair of sites, find approximate Voronoi edge midpoints
    // Real Fortune's algorithm would use a sweep line — this is a simplified O(n^2) version
    for (size_t i = 0; i < sites.size(); ++i) {
        for (size_t j = i + 1; j < sites.size(); ++j) {
            double d = haversineDistance(sites[i], sites[j]);
            if (d < 50.0) { // neighbors within 50km
                diagram.cells[i].neighbor_indices.push_back(j);
                diagram.cells[j].neighbor_indices.push_back(i);
            }
        }
    }

    return diagram;
}

std::optional<size_t> VoronoiDiagram::findCell(const geometry::Point& p) const {
    if (cells.empty()) return std::nullopt;

    size_t best = 0;
    double best_dist = haversineDistance(p, cells[0].site);

    for (size_t i = 1; i < cells.size(); ++i) {
        double d = haversineDistance(p, cells[i].site);
        if (d < best_dist) { best_dist = d; best = i; }
    }

    return best;
}

std::vector<size_t> VoronoiDiagram::nearestSites(
    const geometry::Point& p, size_t k) const
{
    std::vector<std::pair<double, size_t>> distances;
    distances.reserve(cells.size());

    for (size_t i = 0; i < cells.size(); ++i) {
        distances.push_back({haversineDistance(p, cells[i].site), i});
    }

    std::partial_sort(distances.begin(),
                      distances.begin() + std::min(k, distances.size()),
                      distances.end());

    std::vector<size_t> result;
    for (size_t i = 0; i < std::min(k, distances.size()); ++i) {
        result.push_back(distances[i].second);
    }
    return result;
}

} // namespace spatial
} // namespace spatialdb
