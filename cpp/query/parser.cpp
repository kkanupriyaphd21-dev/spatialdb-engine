#include "parser.h"
#include <cctype>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <cmath>

namespace spatialdb {
namespace query {

// Validation constants
static constexpr double MIN_LAT = -90.0;
static constexpr double MAX_LAT = 90.0;
static constexpr double MIN_LON = -180.0;
static constexpr double MAX_LON = 180.0;
static constexpr double MIN_RADIUS = 0.0;
static constexpr double MAX_RADIUS = 20037508.34; // ~half Earth circumference in meters
static constexpr size_t MIN_KNN_LIMIT = 1;
static constexpr size_t MAX_KNN_LIMIT = 10000;

static void validateCoordinate(double lat, double lon, const std::string& ctx) {
    if (std::isnan(lat) || std::isnan(lon)) {
        throw std::runtime_error("NaN coordinate in " + ctx);
    }
    if (std::isinf(lat) || std::isinf(lon)) {
        throw std::runtime_error("infinite coordinate in " + ctx);
    }
    if (lat < MIN_LAT || lat > MAX_LAT) {
        throw std::runtime_error("latitude " + std::to_string(lat) + " out of range [" +
                                 std::to_string(MIN_LAT) + ", " + std::to_string(MAX_LAT) + "] in " + ctx);
    }
    if (lon < MIN_LON || lon > MAX_LON) {
        throw std::runtime_error("longitude " + std::to_string(lon) + " out of range [" +
                                 std::to_string(MIN_LON) + ", " + std::to_string(MAX_LON) + "] in " + ctx);
    }
}

static void validateRadius(double radius, const std::string& ctx) {
    if (std::isnan(radius) || std::isinf(radius)) {
        throw std::runtime_error("invalid radius in " + ctx);
    }
    if (radius < MIN_RADIUS) {
        throw std::runtime_error("radius must be non-negative, got " + std::to_string(radius) + " in " + ctx);
    }
    if (radius > MAX_RADIUS) {
        throw std::runtime_error("radius " + std::to_string(radius) + " exceeds maximum " +
                                 std::to_string(MAX_RADIUS) + " in " + ctx);
    }
}

static void validateLimit(size_t limit, const std::string& ctx) {
    if (limit < MIN_KNN_LIMIT) {
        throw std::runtime_error("limit must be at least " + std::to_string(MIN_KNN_LIMIT) + " in " + ctx);
    }
    if (limit > MAX_KNN_LIMIT) {
        throw std::runtime_error("limit " + std::to_string(limit) + " exceeds maximum " +
                                 std::to_string(MAX_KNN_LIMIT) + " in " + ctx);
    }
}

static void validateBBox(double min_lat, double min_lon, double max_lat, double max_lon, const std::string& ctx) {
    validateCoordinate(min_lat, min_lon, ctx);
    validateCoordinate(max_lat, max_lon, ctx);
    if (min_lat > max_lat) {
        throw std::runtime_error("min_lat " + std::to_string(min_lat) + " > max_lat " +
                                 std::to_string(max_lat) + " in " + ctx);
    }
    if (min_lon > max_lon) {
        throw std::runtime_error("min_lon " + std::to_string(min_lon) + " > max_lon " +
                                 std::to_string(max_lon) + " in " + ctx);
    }
}

// ─── Lexer ───────────────────────────────────────────────────────────────────

Lexer::Lexer(std::string input) : input_(std::move(input)) {}

void Lexer::skipWhitespace() {
    while (pos_ < input_.size()) {
        if (input_[pos_] == '\n') { ++line_; ++pos_; }
        else if (std::isspace((unsigned char)input_[pos_])) { ++pos_; }
        else break;
    }
}

bool Lexer::isKeywordChar(char c) {
    return std::isalnum((unsigned char)c) || c == '_' || c == '.';
}

Token Lexer::readNumber() {
    size_t start = pos_;
    bool has_dot = false;
    if (pos_ < input_.size() && input_[pos_] == '-') ++pos_;
    while (pos_ < input_.size() && (std::isdigit((unsigned char)input_[pos_]) || input_[pos_] == '.')) {
        if (input_[pos_] == '.') {
            if (has_dot) break;
            has_dot = true;
        }
        ++pos_;
    }
    return {TokenType::NUMBER, input_.substr(start, pos_ - start), line_};
}

Token Lexer::readString() {
    ++pos_; // skip opening quote
    size_t start = pos_;
    while (pos_ < input_.size() && input_[pos_] != '"') {
        if (input_[pos_] == '\\') ++pos_;
        ++pos_;
    }
    auto val = input_.substr(start, pos_ - start);
    if (pos_ < input_.size()) ++pos_; // skip closing quote
    return {TokenType::STRING, val, line_};
}

Token Lexer::readKeyword() {
    size_t start = pos_;
    while (pos_ < input_.size() && isKeywordChar(input_[pos_])) ++pos_;
    auto val = input_.substr(start, pos_ - start);
    std::transform(val.begin(), val.end(), val.begin(), ::toupper);
    return {TokenType::KEYWORD, val, line_};
}

Token Lexer::readToken() {
    skipWhitespace();
    if (pos_ >= input_.size()) return {TokenType::EOF_TOKEN, "", line_};

    char c = input_[pos_];

    if (c == '-' || std::isdigit((unsigned char)c)) return readNumber();
    if (c == '"')  return readString();
    if (c == '(') { ++pos_; return {TokenType::LPAREN,  "(", line_}; }
    if (c == ')') { ++pos_; return {TokenType::RPAREN,  ")", line_}; }
    if (c == ',') { ++pos_; return {TokenType::COMMA,   ",", line_}; }
    if (isKeywordChar(c)) return readKeyword();

    ++pos_;
    return {TokenType::UNKNOWN, std::string(1, c), line_};
}

Token Lexer::peek() {
    if (!lookahead_) lookahead_ = readToken();
    return *lookahead_;
}

Token Lexer::next() {
    if (lookahead_) {
        auto t = *lookahead_;
        lookahead_.reset();
        return t;
    }
    return readToken();
}

// ─── QueryParser ─────────────────────────────────────────────────────────────

QueryParser::QueryParser(std::string input) : lexer_(std::move(input)) {}

double QueryParser::expectNumber(const std::string& ctx) {
    auto t = lexer_.next();
    if (t.type != TokenType::NUMBER)
        throw std::runtime_error("expected number in " + ctx + ", got: " + t.value);
    try {
        return std::stod(t.value);
    } catch (const std::exception& e) {
        throw std::runtime_error("invalid number '" + t.value + "' in " + ctx);
    }
}

std::string QueryParser::expectString(const std::string& ctx) {
    auto t = lexer_.next();
    if (t.type != TokenType::STRING && t.type != TokenType::IDENTIFIER && t.type != TokenType::KEYWORD)
        throw std::runtime_error("expected string in " + ctx + ", got: " + t.value);
    return t.value;
}

std::string QueryParser::expectIdentifier(const std::string& ctx) {
    auto t = lexer_.next();
    if (t.type != TokenType::IDENTIFIER && t.type != TokenType::KEYWORD)
        throw std::runtime_error("expected identifier in " + ctx + ", got: " + t.value);
    return t.value;
}

void QueryParser::expectKeyword(const std::string& kw) {
    auto t = lexer_.next();
    std::string v = t.value;
    std::transform(v.begin(), v.end(), v.begin(), ::toupper);
    if (v != kw)
        throw std::runtime_error("expected keyword " + kw + ", got: " + t.value);
}

void QueryParser::expectComma() {
    auto t = lexer_.next();
    if (t.type != TokenType::COMMA)
        throw std::runtime_error("expected comma, got: " + t.value);
}

NearbyQuery QueryParser::parseNearby() {
    // NEARBY collection lat lon radius [LIMIT n]
    NearbyQuery q;
    q.collection = expectString("NEARBY collection");
    double lat   = expectNumber("NEARBY lat");
    double lon   = expectNumber("NEARBY lon");
    double dist  = expectNumber("NEARBY radius");

    validateCoordinate(lat, lon, "NEARBY");
    validateRadius(dist, "NEARBY");

    q.circle = {geometry::Point(lat, lon), dist};

    auto t = lexer_.peek();
    if (t.type == TokenType::KEYWORD && t.value == "LIMIT") {
        lexer_.next();
        size_t limit = (size_t)expectNumber("LIMIT");
        validateLimit(limit, "NEARBY LIMIT");
        q.opts.limit = limit;
    }

    return q;
}

BBoxQuery QueryParser::parseBBox() {
    // WITHIN collection min_lat min_lon max_lat max_lon [LIMIT n]
    BBoxQuery q;
    q.collection = expectString("WITHIN collection");

    double min_lat = expectNumber("bbox min_lat");
    double min_lon = expectNumber("bbox min_lon");
    double max_lat = expectNumber("bbox max_lat");
    double max_lon = expectNumber("bbox max_lon");

    validateBBox(min_lat, min_lon, max_lat, max_lon, "WITHIN");

    q.bbox = {min_lat, min_lon, max_lat, max_lon};

    auto t = lexer_.peek();
    if (t.type == TokenType::KEYWORD && t.value == "LIMIT") {
        lexer_.next();
        size_t limit = (size_t)expectNumber("LIMIT");
        validateLimit(limit, "WITHIN LIMIT");
        q.opts.limit = limit;
    }

    return q;
}

} // namespace query
} // namespace spatialdb
