#pragma once
#include <cstdint>
#include "../include/geometry.h"

namespace spatialdb {
namespace spatial {

// Hilbert curve index for better spatial locality in storage
class HilbertCurve {
public:
    explicit HilbertCurve(int order = 16);

    uint64_t encode(const geometry::Point& p) const;
    geometry::Point decode(uint64_t d) const;

    uint64_t encodeLonLat(double lon, double lat) const;

    static uint64_t xyToD(uint32_t n, uint32_t x, uint32_t y);
    static void     dToXY(uint32_t n, uint64_t d, uint32_t& x, uint32_t& y);

private:
    int     order_;
    uint32_t n_;
};

} // namespace spatial
} // namespace spatialdb
