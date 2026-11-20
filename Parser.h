#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include "token.h"
#include "AST.h"

class Parser {
private:
    struct ParseError {};

    std::vector<Token> tokens;
    std::size_t position;
    std::vector<std::string> errors;
    long lastErrorOffset;

    Token currentToken() const;
    Token peekToken(std::size_t n = 1) const;
    Token advance();
    bool isAtEnd() const;
    bool check(TokenType type, const std::string& lexeme = "") const;
    bool match(TokenType type, const std::string& lexeme = "");
    Token expect(TokenType type, const std::string& lexeme, const std::string& what);
    bool expectSoft(TokenType type, const std::string& lexeme, const std::string& what);

    std::string describe(const Token& t) const;
    void report(const Token& at, const std::string& message);
    [[noreturn]] void fail(const Token& at, const std::string& message);
    void synchronize();
    bool isStatementStart(const Token& t) const;

    NodePtr makeNode(NodeKind kind, const Token& at, const std::string& text = "") const;

    NodePtr parseDeclaration();
    NodePtr parseFunctionDecl(const Token& name);
    void parseParams(Node& owner);
    NodePtr parseType();

    NodePtr parseBlock();
    NodePtr parseStatement();
    NodePtr parseIf();
    NodePtr parseWhile();
    NodePtr parseFor();
    NodePtr parseReturn();
    NodePtr parsePrint();

    NodePtr parseExpression();
    NodePtr parseAssignment();
    NodePtr parseBinary(std::size_t level);
    NodePtr parseExponent();
    NodePtr parseUnary();
    NodePtr parsePostfix();
    NodePtr parsePrimary();

public:

    Parser(const std::vector<Token>& tokenList);

    NodePtr parseProgram();

    bool hasErrors() const { return !errors.empty(); }
    const std::vector<std::string>& getErrors() const { return errors; }
    void printErrors() const;
};

#endif
