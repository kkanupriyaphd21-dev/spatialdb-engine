#include "parser.h"
#include <cctype>
#include <stdexcept>
#include <sstream>
#include <algorithm>

namespace spatialdb {
namespace query {

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
    return std::stod(t.value);
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

    q.circle = {geometry::Point(lat, lon), dist};

    auto t = lexer_.peek();
    if (t.type == TokenType::KEYWORD && t.value == "LIMIT") {
        lexer_.next();
        q.opts.limit = (size_t)expectNumber("LIMIT");
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

    q.bbox = {min_lat, min_lon, max_lat, max_lon};

    auto t = lexer_.peek();
    if (t.type == TokenType::KEYWORD && t.value == "LIMIT") {
        lexer_.next();
        q.opts.limit = (size_t)expectNumber("LIMIT");
    }

    return q;
}

} // namespace query
} // namespace spatialdb
