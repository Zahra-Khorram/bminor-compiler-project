#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include "token.h"

class Lexer {
private:
    std::string source;
    std::string fileName;

    std::size_t position;
    int line;
    int column;
    bool keepComments;
    std::vector<std::string> errors;

    bool atEnd() const;
    char currentChar() const;
    char peekChar() const;
    void advance();
    void skipWhitespace();

    Token makeToken(TokenType type, std::size_t start, int startLine, int startColumn) const;
    Token invalidToken(std::size_t start, int startLine, int startColumn, const std::string& message);

    Token scanWord(std::size_t start, int l, int c);
    Token scanNumber(std::size_t start, int l, int c);
    Token scanString(std::size_t start, int l, int c);
    Token scanChar(std::size_t start, int l, int c);
    Token scanOperator(std::size_t start, int l, int c);

    bool isDelimiter(char c) const;

public:

    Lexer(const std::string& sourceCode, const std::string& name, bool keepCommentTokens = false);

    Token nextToken();
    std::vector<Token> tokenize();

    const std::vector<std::string>& getErrors() const { return errors; }
};

#endif
