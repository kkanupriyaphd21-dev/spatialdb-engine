#include "hilbert.h"
#include <cmath>
#include <stdexcept>

namespace spatialdb {
namespace spatial {

HilbertCurve::HilbertCurve(int order)
    : order_(order), n_(1u << order) {}

uint64_t HilbertCurve::xyToD(uint32_t n, uint32_t x, uint32_t y) {
    uint64_t d = 0;
    for (uint32_t s = n / 2; s > 0; s /= 2) {
        uint32_t rx = (x & s) > 0 ? 1 : 0;
        uint32_t ry = (y & s) > 0 ? 1 : 0;
        d += (uint64_t)s * s * ((3 * rx) ^ ry);

        // rotate
        if (ry == 0) {
            if (rx == 1) {
                x = s - 1 - x;
                y = s - 1 - y;
            }
            uint32_t tmp = x;
            x = y;
            y = tmp;
        }
    }
    return d;
}

void HilbertCurve::dToXY(uint32_t n, uint64_t d, uint32_t& x, uint32_t& y) {
    x = y = 0;
    for (uint32_t s = 1; s < n; s *= 2) {
        uint32_t rx = 1 & (d / 2);
        uint32_t ry = 1 & (d ^ rx);

        if (ry == 0) {
            if (rx == 1) {
                x = s - 1 - x;
                y = s - 1 - y;
            }
            uint32_t tmp = x;
            x = y;
            y = tmp;
        }

        x += s * rx;
        y += s * ry;
        d /= 4;
    }
}

uint64_t HilbertCurve::encodeLonLat(double lon, double lat) const {
    // map [-180,180] x [-90,90] to [0,n) x [0,n)
    uint32_t x = (uint32_t)((lon + 180.0) / 360.0 * n_);
    uint32_t y = (uint32_t)((lat +  90.0) / 180.0 * n_);
    x = std::min(x, n_ - 1);
    y = std::min(y, n_ - 1);
    return xyToD(n_, x, y);
}

uint64_t HilbertCurve::encode(const geometry::Point& p) const {
    return encodeLonLat(p.lon, p.lat);
}

geometry::Point HilbertCurve::decode(uint64_t d) const {
    uint32_t x, y;
    dToXY(n_, d, x, y);
    double lon = (double)x / n_ * 360.0 - 180.0;
    double lat = (double)y / n_ * 180.0 -  90.0;
    return {lat, lon};
}

} // namespace spatial
} // namespace spatialdb
