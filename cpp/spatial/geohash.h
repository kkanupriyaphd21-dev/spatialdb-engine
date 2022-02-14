#pragma once
#include <string>
#include "../include/geometry.h"

namespace spatialdb {
namespace spatial {

class Geohash {
public:
    static std::string encode(double lat, double lon, int precision = 9);
    static geometry::Point decode(const std::string& hash);
    static geometry::BBox  decodeBBox(const std::string& hash);
    static std::vector<std::string> neighbors(const std::string& hash);
    static std::string parent(const std::string& hash);
    static int precision(const std::string& hash) { return (int)hash.size(); }

    static std::vector<std::string> hashesForBBox(const geometry::BBox& bbox, int precision);
    static std::vector<std::string> hashesForRadius(const geometry::Circle& circle, int precision);

private:
    static const char BASE32[];
    static int charToIdx(char c);
};

} // namespace spatial
} // namespace spatialdb
