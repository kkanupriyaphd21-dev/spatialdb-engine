#pragma once
#include "../include/geometry.h"
#include <string>
#include <vector>
#include <cstdint>

namespace spatialdb {
namespace spatial {

// S2-style cell ID encoding for hierarchical spatial indexing.
// Implements a subset of the S2 geometry library interface.
class S2CellID {
public:
    static const int MAX_LEVEL = 30;

    explicit S2CellID(uint64_t id = 0) : id_(id) {}

    static S2CellID fromLatLon(double lat, double lon, int level = MAX_LEVEL);
    static S2CellID fromPoint(const geometry::Point& p, int level = MAX_LEVEL);

    uint64_t id()    const { return id_; }
    int      level() const;
    bool     isValid() const;

    S2CellID parent(int level) const;
    S2CellID parent()          const { return parent(level() - 1); }

    std::vector<S2CellID> children() const;
    std::vector<S2CellID> neighbors() const;

    geometry::BBox   toBBox()  const;
    geometry::Point  toPoint() const;

    bool contains(const S2CellID& other) const;
    bool operator==(const S2CellID& o) const { return id_ == o.id_; }
    bool operator< (const S2CellID& o) const { return id_ <  o.id_; }

    std::string toToken() const;
    static S2CellID fromToken(const std::string& token);

    static std::vector<S2CellID> coverBBox(const geometry::BBox& bbox, int level);
    static std::vector<S2CellID> coverCircle(const geometry::Circle& c, int level);

private:
    uint64_t id_;

    static uint64_t faceUVToST(int face, double u, double v, double& s, double& t);
    static void latLonToFaceUV(double lat_rad, double lon_rad,
                                int& face, double& u, double& v);
};

} // namespace spatial
} // namespace spatialdb
