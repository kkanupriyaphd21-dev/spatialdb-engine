#pragma once
#include <string>
#include <vector>
#include <variant>
#include <optional>
#include "../include/geometry.h"
#include "../include/query.h"

namespace spatialdb {
namespace query {

enum class TokenType {
    KEYWORD, IDENTIFIER, NUMBER, STRING, COMMA,
    LPAREN, RPAREN, EOF_TOKEN, UNKNOWN,
};

struct Token {
    TokenType   type;
    std::string value;
    int         line = 0;
};

class Lexer {
public:
    explicit Lexer(std::string input);
    Token next();
    Token peek();

private:
    std::string input_;
    size_t      pos_ = 0;
    int         line_ = 1;
    std::optional<Token> lookahead_;

    Token readToken();
    Token readNumber();
    Token readString();
    Token readKeyword();
    void  skipWhitespace();
    bool  isKeywordChar(char c);
};

class QueryParser {
public:
    explicit QueryParser(std::string input);

    NearbyQuery parseNearby();
    BBoxQuery   parseBBox();

private:
    Lexer lexer_;

    double      expectNumber(const std::string& ctx);
    std::string expectString(const std::string& ctx);
    std::string expectIdentifier(const std::string& ctx);
    void        expectKeyword(const std::string& kw);
    void        expectComma();
};

} // namespace query
} // namespace spatialdb
