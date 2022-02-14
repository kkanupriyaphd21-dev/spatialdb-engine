#include "geohash.h"
#include <stdexcept>
#include <cmath>
#include <queue>
#include <unordered_set>

namespace spatialdb {
namespace spatial {

const char Geohash::BASE32[] = "0123456789bcdefghjkmnpqrstuvwxyz";

int Geohash::charToIdx(char c) {
    for (int i = 0; i < 32; ++i) {
        if (BASE32[i] == c) return i;
    }
    throw std::invalid_argument(std::string("invalid geohash char: ") + c);
}

std::string Geohash::encode(double lat, double lon, int precision) {
    double lat_min = -90.0,  lat_max = 90.0;
    double lon_min = -180.0, lon_max = 180.0;

    std::string result;
    result.reserve(precision);

    int bits = 0, bit = 0;
    bool use_lon = true;

    while ((int)result.size() < precision) {
        if (use_lon) {
            double mid = (lon_min + lon_max) / 2.0;
            if (lon >= mid) { bit = (bit << 1) | 1; lon_min = mid; }
            else            { bit = (bit << 1) | 0; lon_max = mid; }
        } else {
            double mid = (lat_min + lat_max) / 2.0;
            if (lat >= mid) { bit = (bit << 1) | 1; lat_min = mid; }
            else            { bit = (bit << 1) | 0; lat_max = mid; }
        }
        use_lon = !use_lon;
        ++bits;

        if (bits == 5) {
            result += BASE32[bit];
            bits = 0;
            bit  = 0;
        }
    }

    return result;
}

geometry::BBox Geohash::decodeBBox(const std::string& hash) {
    double lat_min = -90.0,  lat_max = 90.0;
    double lon_min = -180.0, lon_max = 180.0;
    bool use_lon = true;

    for (char c : hash) {
        int val = charToIdx(c);
        for (int i = 4; i >= 0; --i) {
            int bit = (val >> i) & 1;
            if (use_lon) {
                double mid = (lon_min + lon_max) / 2.0;
                if (bit) lon_min = mid; else lon_max = mid;
            } else {
                double mid = (lat_min + lat_max) / 2.0;
                if (bit) lat_min = mid; else lat_max = mid;
            }
            use_lon = !use_lon;
        }
    }

    return {lat_min, lon_min, lat_max, lon_max};
}

geometry::Point Geohash::decode(const std::string& hash) {
    auto box = decodeBBox(hash);
    return {(box.min_lat + box.max_lat) / 2.0,
            (box.min_lon + box.max_lon) / 2.0};
}

std::vector<std::string> Geohash::neighbors(const std::string& hash) {
    auto box = decodeBBox(hash);
    int prec = (int)hash.size();
    double lat_step = (box.max_lat - box.min_lat);
    double lon_step = (box.max_lon - box.min_lon);
    double clat = (box.min_lat + box.max_lat) / 2.0;
    double clon = (box.min_lon + box.max_lon) / 2.0;

    return {
        encode(clat + lat_step, clon,            prec), // N
        encode(clat + lat_step, clon + lon_step, prec), // NE
        encode(clat,            clon + lon_step, prec), // E
        encode(clat - lat_step, clon + lon_step, prec), // SE
        encode(clat - lat_step, clon,            prec), // S
        encode(clat - lat_step, clon - lon_step, prec), // SW
        encode(clat,            clon - lon_step, prec), // W
        encode(clat + lat_step, clon - lon_step, prec), // NW
    };
}

std::string Geohash::parent(const std::string& hash) {
    if (hash.size() <= 1) return "";
    return hash.substr(0, hash.size() - 1);
}

std::vector<std::string> Geohash::hashesForBBox(const geometry::BBox& bbox, int precision) {
    std::vector<std::string> result;
    std::unordered_set<std::string> visited;
    std::queue<std::string> q;

    auto center_hash = encode((bbox.min_lat + bbox.max_lat) / 2.0,
                               (bbox.min_lon + bbox.max_lon) / 2.0, precision);
    q.push(center_hash);
    visited.insert(center_hash);

    while (!q.empty()) {
        auto h = q.front(); q.pop();
        auto box = decodeBBox(h);
        if (!box.intersects(bbox)) continue;

        result.push_back(h);
        for (auto& nb : neighbors(h)) {
            if (visited.insert(nb).second) {
                q.push(nb);
            }
        }

        if (result.size() > 5000) break; // safety limit
    }

    return result;
}

std::vector<std::string> Geohash::hashesForRadius(const geometry::Circle& circle, int precision) {
    geometry::BBox bbox {
        circle.center.lat - circle.radius_km / geometry::EARTH_RADIUS_KM * (180.0 / M_PI),
        circle.center.lon - circle.radius_km / geometry::EARTH_RADIUS_KM * (180.0 / M_PI),
        circle.center.lat + circle.radius_km / geometry::EARTH_RADIUS_KM * (180.0 / M_PI),
        circle.center.lon + circle.radius_km / geometry::EARTH_RADIUS_KM * (180.0 / M_PI),
    };
    return hashesForBBox(bbox, precision);
}

} // namespace spatial
} // namespace spatialdb
