// =============================================================
//  BLUSK ast.cpp
//  ASTNode 추가 유틸 함수 (필요 시 확장)
// =============================================================
#include "../include/ast.h"
#include <iostream>

// ── 디버그: AST 트리 출력 ─────────────────────────────────────────
static void printTree(const ASTNode* node, int depth) {
    if (!node) return;
    std::string indent(depth * 2, ' ');
    std::cerr << indent << "[" << node->type << "]";
    if (!node->value.empty()) std::cerr << " \"" << node->value << "\"";
    if (node->line > 0)       std::cerr << " (line " << node->line << ")";
    std::cerr << "\n";
    for (const ASTNode* c : node->children) printTree(c, depth + 1);
}

void ASTNode::dump() const { printTree(this, 0); }

// ── 자식에서 특정 타입 찾기 ──────────────────────────────────────
ASTNode* ASTNode::findChild(const std::string& type) const {
    for (ASTNode* c : children) if (c && c->type == type) return c;
    return nullptr;
}

// ── 특정 타입 자식 전체 수집 ─────────────────────────────────────
std::vector<ASTNode*> ASTNode::findChildren(const std::string& type) const {
    std::vector<ASTNode*> res;
    for (ASTNode* c : children) if (c && c->type == type) res.push_back(c);
    return res;
}
