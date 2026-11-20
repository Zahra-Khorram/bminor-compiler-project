#include "AST.h"
#include <ostream>

using namespace std;

const char* nodeKindName(NodeKind kind) {
    switch (kind) {
        case NodeKind::Program:      return "Program";
        case NodeKind::VarDecl:      return "VarDecl";
        case NodeKind::FuncDecl:     return "FuncDecl";
        case NodeKind::Param:        return "Param";
        case NodeKind::TypeSpec:     return "TypeSpec";
        case NodeKind::Block:        return "Block";
        case NodeKind::IfStmt:       return "IfStmt";
        case NodeKind::WhileStmt:    return "WhileStmt";
        case NodeKind::ForStmt:      return "ForStmt";
        case NodeKind::ReturnStmt:   return "ReturnStmt";
        case NodeKind::PrintStmt:    return "PrintStmt";
        case NodeKind::ExprStmt:     return "ExprStmt";
        case NodeKind::Empty:        return "Empty";
        case NodeKind::AssignExpr:   return "AssignExpr";
        case NodeKind::BinaryExpr:   return "BinaryExpr";
        case NodeKind::UnaryExpr:    return "UnaryExpr";
        case NodeKind::PostfixExpr:  return "PostfixExpr";
        case NodeKind::CallExpr:     return "CallExpr";
        case NodeKind::IndexExpr:    return "IndexExpr";
        case NodeKind::ArrayLiteral: return "ArrayLiteral";
        case NodeKind::Identifier:   return "Identifier";
        case NodeKind::IntLiteral:   return "IntLiteral";
        case NodeKind::BoolLiteral:  return "BoolLiteral";
        case NodeKind::StringLiteral:return "StringLiteral";
        case NodeKind::CharLiteral:  return "CharLiteral";
        case NodeKind::Error:        return "Error";
    }
    return "?";
}

void printAST(const Node& node, ostream& os, int indent) {
    os << string(indent * 2, ' ') << nodeKindName(node.kind);
    if (!node.text.empty()) os << " '" << node.text << "'";
    os << "  @" << node.line << ":" << node.column;
    if (!node.type.empty()) os << "  : " << node.type;
    os << "\n";

    for (const auto& child : node.children) {
        printAST(*child, os, indent + 1);
    }
}
