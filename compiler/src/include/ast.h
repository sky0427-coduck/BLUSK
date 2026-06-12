// =============================================================
//  BLUSK ast.h  -  AST 노드 (완전판)
// =============================================================
#pragma once
#include <string>
#include <vector>

struct ASTNode {
    std::string type;
    std::string value;
    std::vector<ASTNode*> children;
    int line = 0;

    // 소멸자: 자식 재귀 해제
    ~ASTNode() {
        for (ASTNode* c : children) delete c;
    }

    // 복사 금지 (포인터 이중 해제 방지)
    ASTNode() = default;
    ASTNode(const ASTNode&) = delete;
    ASTNode& operator=(const ASTNode&) = delete;

    // ── 유틸리티 ─────────────────────────────────────────────────
    void dump() const;                                    // 트리 출력 (디버그)
    ASTNode* findChild(const std::string& type) const;   // 첫 번째 자식 찾기
    std::vector<ASTNode*> findChildren(const std::string& type) const; // 전체
};
