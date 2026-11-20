#ifndef TOKEN_H
#define TOKEN_H

#include <cstddef>
#include <string>

enum class TokenType {
    Keyword,
    Identifier,
    IntegerLiteral,
    BooleanLiteral,
    StringLiteral,
    CharacterLiteral,
    Operator,
    Delimiter,
    Comment,
    EndOfFile,
    Invalid
};

struct Token {
    TokenType type = TokenType::Invalid;
    std::string lexeme;
    std::string fileName;
    int line = 1;
    int column = 1;
    std::size_t offset = 0;
    std::size_t length = 0;
};

inline const char* tokenTypeName(TokenType t) {
    switch (t) {
        case TokenType::Keyword:          return "KEYWORD";
        case TokenType::Identifier:       return "IDENT";
        case TokenType::IntegerLiteral:   return "INT";
        case TokenType::BooleanLiteral:   return "BOOL";
        case TokenType::StringLiteral:    return "STRING";
        case TokenType::CharacterLiteral: return "CHAR";
        case TokenType::Operator:         return "OP";
        case TokenType::Delimiter:        return "DELIM";
        case TokenType::Comment:          return "COMMENT";
        case TokenType::EndOfFile:        return "EOF";
        case TokenType::Invalid:          return "INVALID";
    }
    return "?";
}

#endif
