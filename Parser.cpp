#include "Parser.h"
#include <algorithm>
#include <iostream>
#include <set>

using namespace std;

namespace {

const vector<vector<string>> binaryLevels = {
    {"||"},
    {"&&"},
    {"==", "!="},
    {"<", "<=", ">", ">="},
    {"+", "-"},
    {"*", "/", "%"}
};

const set<string> basicTypes = {"integer", "boolean", "char", "string", "void"};

}

Parser::Parser(const vector<Token>& tokenList) : position(0), lastErrorOffset(-1) {
    for (const Token& t : tokenList) {
        if (t.type == TokenType::Comment) continue;
        tokens.push_back(t);
    }
    if (tokens.empty() || tokens.back().type != TokenType::EndOfFile) {
        Token eof;
        eof.type = TokenType::EndOfFile;
        if (!tokens.empty()) {
            eof.fileName = tokens.back().fileName;
            eof.line = tokens.back().line;
            eof.column = tokens.back().column;
            eof.offset = tokens.back().offset + tokens.back().length;
        }
        tokens.push_back(eof);
    }
}

Token Parser::currentToken() const {
    return tokens[min(position, tokens.size() - 1)];
}

Token Parser::peekToken(size_t n) const {
    return tokens[min(position + n, tokens.size() - 1)];
}

Token Parser::advance() {
    Token token = currentToken();
    if (position < tokens.size() - 1) position++;
    return token;
}

bool Parser::isAtEnd() const {
    return currentToken().type == TokenType::EndOfFile;
}

bool Parser::check(TokenType type, const string& lexeme) const {
    Token t = currentToken();
    return t.type == type && (lexeme.empty() || t.lexeme == lexeme);
}

bool Parser::match(TokenType type, const string& lexeme) {
    if (check(type, lexeme)) {
        advance();
        return true;
    }
    return false;
}

Token Parser::expect(TokenType type, const string& lexeme, const string& what) {
    if (check(type, lexeme)) return advance();
    fail(currentToken(), "expected " + what + ", got " + describe(currentToken()));
}

bool Parser::expectSoft(TokenType type, const string& lexeme, const string& what) {
    if (match(type, lexeme)) return true;
    report(currentToken(), "expected " + what + ", got " + describe(currentToken()));
    return false;
}

string Parser::describe(const Token& t) const {
    if (t.type == TokenType::EndOfFile) return "end of file";
    return "'" + t.lexeme + "'";
}

void Parser::report(const Token& at, const string& message) {

    if (at.type == TokenType::Invalid) return;
    if (position > 0 && tokens[position - 1].type == TokenType::Invalid &&
        at.offset == currentToken().offset) return;

    if (static_cast<long>(at.offset) == lastErrorOffset) return;
    lastErrorOffset = static_cast<long>(at.offset);

    errors.push_back(at.fileName + ":" + to_string(at.line) + ":" + to_string(at.column) +
                     ": syntax error: " + message);
}

void Parser::fail(const Token& at, const string& message) {
    report(at, message);
    throw ParseError();
}

bool Parser::isStatementStart(const Token& t) const {
    if (t.type != TokenType::Keyword) return false;
    return t.lexeme == "if" || t.lexeme == "while" || t.lexeme == "for" ||
           t.lexeme == "return" || t.lexeme == "print";
}

void Parser::synchronize() {
    while (!isAtEnd()) {
        Token t = currentToken();

        if (t.type == TokenType::Delimiter && t.lexeme == ";") {
            advance();
            return;
        }
        if (t.type == TokenType::Delimiter && t.lexeme == "}") return;
        if (isStatementStart(t)) return;

        if (t.type == TokenType::Identifier &&
            peekToken().type == TokenType::Delimiter && peekToken().lexeme == ":") {
            return;
        }
        advance();
    }
}

NodePtr Parser::makeNode(NodeKind kind, const Token& at, const string& text) const {
    return make_unique<Node>(kind, text, at.line, at.column, at.offset);
}

void Parser::printErrors() const {
    for (const string& e : errors) cerr << e << "\n";
}

NodePtr Parser::parseProgram() {
    NodePtr program = makeNode(NodeKind::Program, currentToken());

    while (!isAtEnd()) {
        size_t before = position;
        try {
            program->add(parseDeclaration());
        } catch (const ParseError&) {
            synchronize();

            if (position == before && !isAtEnd()) advance();
        }
    }
    return program;
}

NodePtr Parser::parseDeclaration() {
    Token name = expect(TokenType::Identifier, "", "a declaration (identifier)");
    expect(TokenType::Delimiter, ":", "':' after '" + name.lexeme + "'");

    if (check(TokenType::Keyword, "function")) return parseFunctionDecl(name);

    NodePtr decl = makeNode(NodeKind::VarDecl, name, name.lexeme);
    decl->add(parseType());

    if (match(TokenType::Operator, "=")) {
        decl->add(parseExpression());
    }
    expectSoft(TokenType::Delimiter, ";", "';' after declaration");
    return decl;
}

NodePtr Parser::parseFunctionDecl(const Token& name) {
    NodePtr fn = makeNode(NodeKind::FuncDecl, name, name.lexeme);

    advance();
    fn->add(parseType());
    expect(TokenType::Delimiter, "(", "'(' after function return type");
    parseParams(*fn);
    expectSoft(TokenType::Delimiter, ")", "')' after parameters");

    if (match(TokenType::Operator, "=")) {
        if (!check(TokenType::Delimiter, "{")) {
            fail(currentToken(), "expected '{' to start the function body, got " +
                                 describe(currentToken()));
        }
        fn->add(parseBlock());
    } else {
        expectSoft(TokenType::Delimiter, ";", "'=' or ';' after function header");
    }
    return fn;
}

void Parser::parseParams(Node& owner) {
    if (check(TokenType::Delimiter, ")")) return;

    do {
        Token name = expect(TokenType::Identifier, "", "parameter name");
        expect(TokenType::Delimiter, ":", "':' after parameter name");

        NodePtr param = makeNode(NodeKind::Param, name, name.lexeme);
        param->add(parseType());
        owner.add(std::move(param));
    } while (match(TokenType::Delimiter, ","));
}

NodePtr Parser::parseType() {
    Token t = currentToken();

    if (t.type == TokenType::Keyword) {
        if (basicTypes.count(t.lexeme)) {
            advance();
            return makeNode(NodeKind::TypeSpec, t, t.lexeme);
        }

        if (t.lexeme == "array") {
            advance();
            NodePtr node = makeNode(NodeKind::TypeSpec, t, "array");
            expect(TokenType::Delimiter, "[", "'[' after 'array'");

            NodePtr size;
            if (!check(TokenType::Delimiter, "]")) size = parseExpression();
            expectSoft(TokenType::Delimiter, "]", "']' in array type");

            node->add(parseType());
            node->add(std::move(size));
            return node;
        }

        if (t.lexeme == "function") {
            advance();
            NodePtr node = makeNode(NodeKind::TypeSpec, t, "function");
            node->add(parseType());
            expect(TokenType::Delimiter, "(", "'(' in function type");
            parseParams(*node);
            expectSoft(TokenType::Delimiter, ")", "')' in function type");
            return node;
        }
    }

    fail(t, "expected a type, got " + describe(t));
}

NodePtr Parser::parseBlock() {
    Token open = expect(TokenType::Delimiter, "{", "'{'");
    NodePtr block = makeNode(NodeKind::Block, open);

    while (!check(TokenType::Delimiter, "}") && !isAtEnd()) {
        size_t before = position;
        try {
            block->add(parseStatement());
        } catch (const ParseError&) {
            synchronize();
            if (position == before && !isAtEnd() && !check(TokenType::Delimiter, "}")) advance();
        }
    }
    expectSoft(TokenType::Delimiter, "}", "'}' to close the block");
    return block;
}

NodePtr Parser::parseStatement() {
    Token t = currentToken();

    if (t.type == TokenType::Identifier &&
        peekToken().type == TokenType::Delimiter && peekToken().lexeme == ":") {
        return parseDeclaration();
    }

    if (t.type == TokenType::Delimiter && t.lexeme == "{") return parseBlock();

    if (t.type == TokenType::Delimiter && t.lexeme == ";") {
        advance();
        return makeNode(NodeKind::Empty, t);
    }

    if (t.type == TokenType::Keyword) {
        if (t.lexeme == "if")     return parseIf();
        if (t.lexeme == "while")  return parseWhile();
        if (t.lexeme == "for")    return parseFor();
        if (t.lexeme == "return") return parseReturn();
        if (t.lexeme == "print")  return parsePrint();
    }

    NodePtr stmt = makeNode(NodeKind::ExprStmt, t);
    stmt->add(parseExpression());
    expectSoft(TokenType::Delimiter, ";", "';' after expression");
    return stmt;
}

NodePtr Parser::parseIf() {
    NodePtr node = makeNode(NodeKind::IfStmt, advance());

    expect(TokenType::Delimiter, "(", "'(' after 'if'");
    node->add(parseExpression());
    expectSoft(TokenType::Delimiter, ")", "')' after condition");

    node->add(parseStatement());

    if (match(TokenType::Keyword, "else")) node->add(parseStatement());
    return node;
}

NodePtr Parser::parseWhile() {
    NodePtr node = makeNode(NodeKind::WhileStmt, advance());

    expect(TokenType::Delimiter, "(", "'(' after 'while'");
    node->add(parseExpression());
    expectSoft(TokenType::Delimiter, ")", "')' after condition");
    node->add(parseStatement());
    return node;
}

NodePtr Parser::parseFor() {
    NodePtr node = makeNode(NodeKind::ForStmt, advance());

    expect(TokenType::Delimiter, "(", "'(' after 'for'");

    node->add(check(TokenType::Delimiter, ";") ? makeNode(NodeKind::Empty, currentToken())
                                               : parseExpression());
    expectSoft(TokenType::Delimiter, ";", "';' after for-initializer");

    node->add(check(TokenType::Delimiter, ";") ? makeNode(NodeKind::Empty, currentToken())
                                               : parseExpression());
    expectSoft(TokenType::Delimiter, ";", "';' after for-condition");

    node->add(check(TokenType::Delimiter, ")") ? makeNode(NodeKind::Empty, currentToken())
                                               : parseExpression());
    expectSoft(TokenType::Delimiter, ")", "')' after for-clauses");

    node->add(parseStatement());
    return node;
}

NodePtr Parser::parseReturn() {
    NodePtr node = makeNode(NodeKind::ReturnStmt, advance());
    if (!check(TokenType::Delimiter, ";")) node->add(parseExpression());
    expectSoft(TokenType::Delimiter, ";", "';' after return");
    return node;
}

NodePtr Parser::parsePrint() {
    NodePtr node = makeNode(NodeKind::PrintStmt, advance());
    do {
        node->add(parseExpression());
    } while (match(TokenType::Delimiter, ","));
    expectSoft(TokenType::Delimiter, ";", "';' after print");
    return node;
}

NodePtr Parser::parseExpression() {
    return parseAssignment();
}

NodePtr Parser::parseAssignment() {
    NodePtr left = parseBinary(0);

    if (check(TokenType::Operator, "=")) {
        Token op = advance();

        if (left->kind != NodeKind::Identifier && left->kind != NodeKind::IndexExpr) {
            report(op, "invalid assignment target");
        }

        NodePtr node = makeNode(NodeKind::AssignExpr, op, "=");
        node->line = left->line;
        node->column = left->column;
        node->offset = left->offset;
        node->add(std::move(left));
        node->add(parseAssignment());
        return node;
    }
    return left;
}

NodePtr Parser::parseBinary(size_t level) {
    if (level == binaryLevels.size()) return parseExponent();

    NodePtr left = parseBinary(level + 1);

    while (true) {
        Token t = currentToken();
        const auto& ops = binaryLevels[level];
        if (t.type != TokenType::Operator ||
            find(ops.begin(), ops.end(), t.lexeme) == ops.end()) break;

        advance();
        NodePtr right = parseBinary(level + 1);

        NodePtr node = makeNode(NodeKind::BinaryExpr, t, t.lexeme);
        node->line = left->line;
        node->column = left->column;
        node->offset = left->offset;
        node->add(std::move(left));
        node->add(std::move(right));
        left = std::move(node);
    }
    return left;
}

NodePtr Parser::parseExponent() {
    NodePtr left = parseUnary();

    if (check(TokenType::Operator, "^")) {
        Token op = advance();
        NodePtr node = makeNode(NodeKind::BinaryExpr, op, "^");
        node->line = left->line;
        node->column = left->column;
        node->offset = left->offset;
        node->add(std::move(left));
        node->add(parseExponent());
        return node;
    }
    return left;
}

NodePtr Parser::parseUnary() {
    if (check(TokenType::Operator, "-") || check(TokenType::Operator, "!")) {
        Token op = advance();
        NodePtr node = makeNode(NodeKind::UnaryExpr, op, op.lexeme);
        node->add(parseUnary());
        return node;
    }
    return parsePostfix();
}

NodePtr Parser::parsePostfix() {
    NodePtr expr = parsePrimary();

    while (true) {
        if (check(TokenType::Delimiter, "[")) {
            advance();
            NodePtr node = makeNode(NodeKind::IndexExpr, currentToken());
            node->line = expr->line;
            node->column = expr->column;
            node->offset = expr->offset;
            node->add(std::move(expr));
            node->add(parseExpression());
            expectSoft(TokenType::Delimiter, "]", "']' after index");
            expr = std::move(node);
        }
        else if (check(TokenType::Delimiter, "(")) {
            advance();
            NodePtr node = makeNode(NodeKind::CallExpr, currentToken());
            node->line = expr->line;
            node->column = expr->column;
            node->offset = expr->offset;
            node->add(std::move(expr));

            if (!check(TokenType::Delimiter, ")")) {
                do {
                    node->add(parseExpression());
                } while (match(TokenType::Delimiter, ","));
            }
            expectSoft(TokenType::Delimiter, ")", "')' after arguments");
            expr = std::move(node);
        }
        else if (check(TokenType::Operator, "++") || check(TokenType::Operator, "--")) {
            Token op = advance();
            NodePtr node = makeNode(NodeKind::PostfixExpr, op, op.lexeme);
            node->line = expr->line;
            node->column = expr->column;
            node->offset = expr->offset;
            node->add(std::move(expr));
            expr = std::move(node);
        }
        else {
            break;
        }
    }
    return expr;
}

NodePtr Parser::parsePrimary() {
    Token t = currentToken();

    switch (t.type) {
        case TokenType::IntegerLiteral:   advance(); return makeNode(NodeKind::IntLiteral, t, t.lexeme);
        case TokenType::BooleanLiteral:   advance(); return makeNode(NodeKind::BoolLiteral, t, t.lexeme);
        case TokenType::StringLiteral:    advance(); return makeNode(NodeKind::StringLiteral, t, t.lexeme);
        case TokenType::CharacterLiteral: advance(); return makeNode(NodeKind::CharLiteral, t, t.lexeme);
        case TokenType::Identifier:       advance(); return makeNode(NodeKind::Identifier, t, t.lexeme);
        default: break;
    }

    if (t.type == TokenType::Invalid) {
        advance();
        return makeNode(NodeKind::Error, t, t.lexeme);
    }

    if (t.type == TokenType::Delimiter && t.lexeme == "(") {
        advance();
        NodePtr inner = parseExpression();
        expectSoft(TokenType::Delimiter, ")", "')' after expression");
        return inner;
    }

    if (t.type == TokenType::Delimiter && t.lexeme == "{") {
        advance();
        NodePtr list = makeNode(NodeKind::ArrayLiteral, t);
        if (!check(TokenType::Delimiter, "}")) {
            do {
                list->add(parseExpression());
            } while (match(TokenType::Delimiter, ","));
        }
        expectSoft(TokenType::Delimiter, "}", "'}' to close array literal");
        return list;
    }

    fail(t, "expected expression, got " + describe(t));
}
