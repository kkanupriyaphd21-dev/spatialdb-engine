#include "s2_bridge.h"
#include <cmath>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace spatialdb {
namespace spatial {

static void latLonToXYZ(double lat_rad, double lon_rad,
                         double& x, double& y, double& z) {
    double cos_lat = std::cos(lat_rad);
    x = cos_lat * std::cos(lon_rad);
    y = cos_lat * std::sin(lon_rad);
    z = std::sin(lat_rad);
}

static int xyzToFace(double x, double y, double z) {
    double ax = std::abs(x), ay = std::abs(y), az = std::abs(z);
    if (ax > ay && ax > az) return x > 0 ? 0 : 3;
    if (ay > az)             return y > 0 ? 1 : 4;
    return z > 0 ? 2 : 5;
}

void S2CellID::latLonToFaceUV(double lat_rad, double lon_rad,
                                int& face, double& u, double& v) {
    double x, y, z;
    latLonToXYZ(lat_rad, lon_rad, x, y, z);
    face = xyzToFace(x, y, z);
    switch (face) {
        case 0: u =  y/x; v =  z/x; break;
        case 1: u = -x/y; v =  z/y; break;
        case 2: u = -x/z; v = -y/z; break;
        case 3: u =  z/x; v =  y/x; break;
        case 4: u =  z/y; v = -x/y; break;
        case 5: u = -y/z; v = -x/z; break;
        default: u = v = 0;
    }
}

S2CellID S2CellID::fromLatLon(double lat, double lon, int level) {
    double lat_rad = lat * geometry::DEG_TO_RAD;
    double lon_rad = lon * geometry::DEG_TO_RAD;

    int face; double u, v;
    latLonToFaceUV(lat_rad, lon_rad, face, u, v);

    // map u,v in [-1,1] to [0,1]
    double s = (u + 1.0) / 2.0;
    double t = (v + 1.0) / 2.0;

    uint64_t i = (uint64_t)(s * (1LL << level));
    uint64_t j = (uint64_t)(t * (1LL << level));
    i = std::min(i, (uint64_t)(1LL << level) - 1);
    j = std::min(j, (uint64_t)(1LL << level) - 1);

    uint64_t id = ((uint64_t)face << (2 * level + 1)) |
                  (i << (level + 1)) |
                  (j) |
                  1ULL;

    return S2CellID(id);
}

S2CellID S2CellID::fromPoint(const geometry::Point& p, int level) {
    return fromLatLon(p.lat, p.lon, level);
}

int S2CellID::level() const {
    if (id_ == 0) return -1;
    int lsb = __builtin_ctzll(id_);
    return (MAX_LEVEL * 2 - lsb) / 2;
}

bool S2CellID::isValid() const {
    return id_ != 0 && level() >= 0 && level() <= MAX_LEVEL;
}

S2CellID S2CellID::parent(int level) const {
    int lsb_bits = 2 * (MAX_LEVEL - level);
    uint64_t mask = ~((1ULL << lsb_bits) - 1);
    return S2CellID((id_ & mask) | (1ULL << (lsb_bits - 1)));
}

bool S2CellID::contains(const S2CellID& other) const {
    return other.id_ >= id_ && other.id_ <= id_ + (id_ & (-id_)) * 2 - 1;
}

std::string S2CellID::toToken() const {
    std::ostringstream ss;
    ss << std::hex << id_;
    return ss.str();
}

S2CellID S2CellID::fromToken(const std::string& token) {
    uint64_t id;
    std::istringstream ss(token);
    ss >> std::hex >> id;
    return S2CellID(id);
}

geometry::Point S2CellID::toPoint() const {
    // approximate center — reverse the cell ID encoding
    // simplified: extract face, i, j and convert back
    return {0.0, 0.0}; // placeholder
}

geometry::BBox S2CellID::toBBox() const {
    return {-90, -180, 90, 180}; // placeholder
}

std::vector<S2CellID> S2CellID::coverBBox(const geometry::BBox& bbox, int level) {
    std::vector<S2CellID> cells;
    // Sample grid and collect unique cell IDs
    int steps = 8;
    double dlat = (bbox.max_lat - bbox.min_lat) / steps;
    double dlon = (bbox.max_lon - bbox.min_lon) / steps;

    for (int i = 0; i <= steps; ++i) {
        for (int j = 0; j <= steps; ++j) {
            double lat = bbox.min_lat + i * dlat;
            double lon = bbox.min_lon + j * dlon;
            auto cell = fromLatLon(lat, lon, level);
            if (std::find(cells.begin(), cells.end(), cell) == cells.end())
                cells.push_back(cell);
        }
    }
    return cells;
}

std::vector<S2CellID> S2CellID::coverCircle(const geometry::Circle& c, int level) {
    double lat_delta = c.radius_km / geometry::EARTH_RADIUS_KM * (180.0 / M_PI);
    geometry::BBox bbox {
        c.center.lat - lat_delta, c.center.lon - lat_delta,
        c.center.lat + lat_delta, c.center.lon + lat_delta
    };
    return coverBBox(bbox, level);
}

} // namespace spatial
} // namespace spatialdb
