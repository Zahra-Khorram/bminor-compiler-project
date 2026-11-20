#include "Lexer.h"
#include <cctype>
#include <set>

using namespace std;

namespace {

const set<string> keywords = {
    "array", "boolean", "char", "else", "for", "function", "if",
    "integer", "print", "return", "string", "void", "while"
};

const set<string> twoCharOperators = {
    "==", "!=", "<=", ">=", "&&", "||", "++", "--"
};

const string singleCharOperators = "+-*/%^=!<>";
const string validEscapes = "ntr\\\"'0";

}

Lexer::Lexer(const string& sourceCode, const string& name, bool keepCommentTokens)
    : source(sourceCode), fileName(name), position(0),
      line(1), column(1), keepComments(keepCommentTokens) {}

bool Lexer::atEnd() const {
    return position >= source.length();
}

char Lexer::currentChar() const {
    return atEnd() ? '\0' : source[position];
}

char Lexer::peekChar() const {
    return (position + 1 >= source.length()) ? '\0' : source[position + 1];
}

void Lexer::advance() {
    if (atEnd()) return;

    if (source[position] == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    position++;
}

void Lexer::skipWhitespace() {
    while (!atEnd() && isspace(static_cast<unsigned char>(currentChar()))) {
        advance();
    }
}

bool Lexer::isDelimiter(char c) const {
    static const string delimiters = ";,(){}[]:";
    return delimiters.find(c) != string::npos;
}

Token Lexer::makeToken(TokenType type, size_t start, int startLine, int startColumn) const {
    Token t;
    t.type = type;
    t.lexeme = source.substr(start, position - start);
    t.fileName = fileName;
    t.line = startLine;
    t.column = startColumn;
    t.offset = start;
    t.length = position - start;
    return t;
}

Token Lexer::invalidToken(size_t start, int l, int c, const string& message) {
    errors.push_back(fileName + ":" + to_string(l) + ":" + to_string(c) +
                     ": lexical error: " + message);
    return makeToken(TokenType::Invalid, start, l, c);
}

Token Lexer::scanWord(size_t start, int l, int c) {
    while (isalnum(static_cast<unsigned char>(currentChar())) || currentChar() == '_') {
        advance();
    }

    Token t = makeToken(TokenType::Identifier, start, l, c);

    if (t.lexeme == "true" || t.lexeme == "false") {
        t.type = TokenType::BooleanLiteral;
    } else if (keywords.count(t.lexeme)) {
        t.type = TokenType::Keyword;
    }
    return t;
}

Token Lexer::scanNumber(size_t start, int l, int c) {
    while (isdigit(static_cast<unsigned char>(currentChar()))) {
        advance();
    }

    if (isalpha(static_cast<unsigned char>(currentChar())) || currentChar() == '_') {
        while (isalnum(static_cast<unsigned char>(currentChar())) || currentChar() == '_') {
            advance();
        }
        return invalidToken(start, l, c,
            "malformed number '" + source.substr(start, position - start) + "'");
    }

    string digits = source.substr(start, position - start);
    if (digits.size() > 1 && digits[0] == '0') {
        return invalidToken(start, l, c,
            "leading zeros are not allowed in integer literal '" + digits + "'");
    }

    return makeToken(TokenType::IntegerLiteral, start, l, c);
}

Token Lexer::scanString(size_t start, int l, int c) {
    bool valid = true;
    string problem;

    advance();

    while (!atEnd() && currentChar() != '"' && currentChar() != '\n') {
        if (currentChar() == '\\') {
            advance();
            if (atEnd() || currentChar() == '\n') break;

            if (validEscapes.find(currentChar()) == string::npos && valid) {
                valid = false;
                problem = string("invalid escape sequence '\\") + currentChar() + "'";
            }
        }
        advance();
    }

    if (currentChar() == '"') {
        advance();
        if (valid) return makeToken(TokenType::StringLiteral, start, l, c);
        return invalidToken(start, l, c, problem);
    }

    return invalidToken(start, l, c, "unterminated string literal");
}

Token Lexer::scanChar(size_t start, int l, int c) {
    bool valid = true;
    string problem;
    int count = 0;

    advance();

    if (currentChar() == '\\') {
        advance();
        if (atEnd() || currentChar() == '\n' || currentChar() == '\'') {
            valid = false;
            problem = "incomplete escape sequence in character literal";
        } else {
            if (validEscapes.find(currentChar()) == string::npos) {
                valid = false;
                problem = string("invalid escape sequence '\\") + currentChar() + "'";
            }
            advance();
            count = 1;
        }
    } else if (!atEnd() && currentChar() != '\'' && currentChar() != '\n') {
        advance();
        count = 1;
    }

    if (currentChar() == '\'') {
        advance();
        if (valid && count == 0) {
            return invalidToken(start, l, c, "empty character literal");
        }
        if (valid) return makeToken(TokenType::CharacterLiteral, start, l, c);
        return invalidToken(start, l, c, problem);
    }

    while (!atEnd() && currentChar() != '\'' && currentChar() != '\n') {
        advance();
    }
    if (currentChar() == '\'') {
        advance();
        return invalidToken(start, l, c,
            "character literal must contain exactly one character");
    }
    return invalidToken(start, l, c, "unterminated character literal");
}

Token Lexer::scanOperator(size_t start, int l, int c) {
    char ch = currentChar();
    string two = string(1, ch) + string(1, peekChar());

    if (twoCharOperators.count(two)) {
        advance();
        advance();
        return makeToken(TokenType::Operator, start, l, c);
    }

    if (singleCharOperators.find(ch) != string::npos) {
        advance();
        return makeToken(TokenType::Operator, start, l, c);
    }

    advance();

    if (ch == '&' || ch == '|') {
        return invalidToken(start, l, c,
            string("'") + ch + "' is not an operator (did you mean '" + ch + ch + "'?)");
    }

    while ((static_cast<unsigned char>(currentChar()) & 0xC0) == 0x80) {
        advance();
    }
    return invalidToken(start, l, c,
        "unexpected character '" + source.substr(start, position - start) + "'");
}

Token Lexer::nextToken() {

    while (true) {
        skipWhitespace();

        size_t start = position;
        int l = line, c = column;

        if (currentChar() == '/' && peekChar() == '/') {
            while (!atEnd() && currentChar() != '\n') advance();
            if (keepComments) return makeToken(TokenType::Comment, start, l, c);
        }
        else if (currentChar() == '/' && peekChar() == '*') {
            advance();
            advance();

            bool closed = false;
            while (!atEnd()) {
                if (currentChar() == '*' && peekChar() == '/') {
                    advance();
                    advance();
                    closed = true;
                    break;
                }
                advance();
            }

            if (!closed) return invalidToken(start, l, c, "unterminated block comment");
            if (keepComments) return makeToken(TokenType::Comment, start, l, c);
        }
        else {
            break;
        }
    }

    size_t start = position;
    int l = line, c = column;

    if (atEnd()) return makeToken(TokenType::EndOfFile, start, l, c);

    char ch = currentChar();

    if (isalpha(static_cast<unsigned char>(ch)) || ch == '_') return scanWord(start, l, c);
    if (isdigit(static_cast<unsigned char>(ch)))               return scanNumber(start, l, c);
    if (ch == '"')                                             return scanString(start, l, c);
    if (ch == '\'')                                            return scanChar(start, l, c);

    if (isDelimiter(ch)) {
        advance();
        return makeToken(TokenType::Delimiter, start, l, c);
    }

    return scanOperator(start, l, c);
}

vector<Token> Lexer::tokenize() {
    vector<Token> result;
    while (true) {
        Token t = nextToken();
        result.push_back(t);
        if (t.type == TokenType::EndOfFile) break;
    }
    return result;
}
