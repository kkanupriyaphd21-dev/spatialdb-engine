#include "filter.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace spatialdb {
namespace query {

static constexpr size_t MAX_FILTER_DEPTH = 64;
static constexpr size_t MAX_FILTER_VALUE_LEN = 4096;

bool FieldFilter::match(const std::string& actual) const {
    switch (op) {
        case FilterOp::EQ:          return actual == value;
        case FilterOp::NEQ:         return actual != value;
        case FilterOp::GT:          return actual >  value;
        case FilterOp::GTE:         return actual >= value;
        case FilterOp::LT:          return actual <  value;
        case FilterOp::LTE:         return actual <= value;
        case FilterOp::CONTAINS:    return actual.find(value) != std::string::npos;
        case FilterOp::STARTS_WITH: return actual.rfind(value, 0) == 0;
        default:                    return false;
    }
}

bool FilterNode::evaluate(const index::IndexEntry& entry) const {
    if (type == Type::LEAF) {
        // For now match against id or collection fields
        const std::string* field_val = nullptr;
        if (filter.field == "id")         field_val = &entry.id;
        if (filter.field == "collection") field_val = &entry.collection;
        if (!field_val) return true; // unknown field = pass
        return filter.match(*field_val);
    }
    if (type == Type::AND) {
        for (const auto& c : children)
            if (!c->evaluate(entry)) return false;
        return true;
    }
    if (type == Type::OR) {
        for (const auto& c : children)
            if (c->evaluate(entry)) return true;
        return false;
    }
    if (type == Type::NOT) {
        return !children.empty() && !children[0]->evaluate(entry);
    }
    return true;
}

FilterTree makeLeaf(FieldFilter f) {
    auto n = std::make_shared<FilterNode>();
    n->type   = FilterNode::Type::LEAF;
    n->filter = std::move(f);
    return n;
}

FilterTree makeAnd(std::vector<FilterTree> children) {
    auto n = std::make_shared<FilterNode>();
    n->type     = FilterNode::Type::AND;
    n->children = std::move(children);
    return n;
}

FilterTree makeOr(std::vector<FilterTree> children) {
    auto n = std::make_shared<FilterNode>();
    n->type     = FilterNode::Type::OR;
    n->children = std::move(children);
    return n;
}

FilterTree makeNot(FilterTree child) {
    auto n = std::make_shared<FilterNode>();
    n->type = FilterNode::Type::NOT;
    n->children.push_back(std::move(child));
    return n;
}

// ─── FilterParser ─────────────────────────────────────────────────────────────

FilterParser::FilterParser(std::string expr) : expr_(std::move(expr)) {}

void FilterParser::skipWS() {
    while (pos_ < expr_.size() && std::isspace((unsigned char)expr_[pos_])) ++pos_;
}

std::string FilterParser::readToken() {
    skipWS();
    size_t start = pos_;
    while (pos_ < expr_.size() && !std::isspace((unsigned char)expr_[pos_]) &&
           expr_[pos_] != '(' && expr_[pos_] != ')') ++pos_;
    return expr_.substr(start, pos_ - start);
}

FilterOp FilterParser::parseOp() {
    auto tok = readToken();
    if (tok == "==")  return FilterOp::EQ;
    if (tok == "!=")  return FilterOp::NEQ;
    if (tok == ">")   return FilterOp::GT;
    if (tok == ">=")  return FilterOp::GTE;
    if (tok == "<")   return FilterOp::LT;
    if (tok == "<=")  return FilterOp::LTE;
    if (tok == "~")   return FilterOp::CONTAINS;
    if (tok == "^")   return FilterOp::STARTS_WITH;
    throw std::runtime_error("unknown filter op: " + tok);
}

FieldFilter FilterParser::parseLeaf() {
    FieldFilter f;
    f.field = readToken();
    if (f.field.empty()) throw std::runtime_error("empty filter field");
    f.op    = parseOp();
    f.value = readToken();
    if (f.value.size() > MAX_FILTER_VALUE_LEN)
        throw std::runtime_error("filter value too long");
    return f;
}

FilterTree FilterParser::parseAtom() {
    skipWS();
    if (pos_ < expr_.size() && expr_[pos_] == '(') {
        ++pos_;
        auto node = parseExpr();
        skipWS();
        if (pos_ < expr_.size() && expr_[pos_] == ')') ++pos_;
        return node;
    }
    return makeLeaf(parseLeaf());
}

FilterTree FilterParser::parseTerm() {
    auto left = parseAtom();
    skipWS();
    std::string peek = expr_.substr(pos_, 3);
    if (peek.substr(0, 3) == "AND") {
        pos_ += 3;
        auto right = parseTerm();
        return makeAnd({left, right});
    }
    return left;
}

FilterTree FilterParser::parseExpr() {
    auto left = parseTerm();
    skipWS();
    if (pos_ < expr_.size() && expr_.substr(pos_, 2) == "OR") {
        pos_ += 2;
        auto right = parseExpr();
        return makeOr({left, right});
    }
    return left;
}

FilterTree FilterParser::parse() {
    return parseExpr();
}

} // namespace query
} // namespace spatialdb
