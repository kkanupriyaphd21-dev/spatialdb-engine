#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <variant>
#include "../include/index.h"

namespace spatialdb {
namespace query {

enum class FilterOp {
    EQ, NEQ, GT, GTE, LT, LTE, CONTAINS, STARTS_WITH,
};

struct FieldFilter {
    std::string field;
    FilterOp    op;
    std::string value;

    bool match(const std::string& actual) const;
};

struct FilterNode {
    enum class Type { LEAF, AND, OR, NOT };

    Type type = Type::LEAF;
    FieldFilter filter;
    std::vector<std::shared_ptr<FilterNode>> children;

    bool evaluate(const index::IndexEntry& entry) const;
};

using FilterTree = std::shared_ptr<FilterNode>;

FilterTree makeLeaf(FieldFilter f);
FilterTree makeAnd(std::vector<FilterTree> children);
FilterTree makeOr(std::vector<FilterTree> children);
FilterTree makeNot(FilterTree child);

class FilterParser {
public:
    explicit FilterParser(std::string expr);
    FilterTree parse();

private:
    std::string expr_;
    size_t      pos_ = 0;

    FilterTree parseExpr();
    FilterTree parseTerm();
    FilterTree parseAtom();
    FieldFilter parseLeaf();
    std::string readToken();
    void skipWS();
    FilterOp parseOp();
};

} // namespace query
} // namespace spatialdb
