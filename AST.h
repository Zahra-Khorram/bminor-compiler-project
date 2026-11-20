#ifndef AST_H
#define AST_H

#include <cstddef>
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

enum class NodeKind {
    Program, VarDecl, FuncDecl, Param, TypeSpec,
    Block, IfStmt, WhileStmt, ForStmt, ReturnStmt, PrintStmt, ExprStmt, Empty,
    AssignExpr, BinaryExpr, UnaryExpr, PostfixExpr, CallExpr, IndexExpr, ArrayLiteral,
    Identifier, IntLiteral, BoolLiteral, StringLiteral, CharLiteral,
    Error
};

const char* nodeKindName(NodeKind kind);

struct Node;
using NodePtr = std::unique_ptr<Node>;

struct Node {
    NodeKind kind;
    std::string text;
    int line;
    int column;
    std::size_t offset;
    std::string type;
    std::vector<NodePtr> children;

    Node(NodeKind k, std::string t, int l, int c, std::size_t o)
        : kind(k), text(std::move(t)), line(l), column(c), offset(o) {}

    void add(NodePtr child) {
        if (child) children.push_back(std::move(child));
    }

    bool isLeaf() const { return children.empty(); }
};

void printAST(const Node& node, std::ostream& os, int indent = 0);

#endif
