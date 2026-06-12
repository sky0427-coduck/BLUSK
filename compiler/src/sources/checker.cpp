// =============================================================
//  BLUSK checker.cpp  -  단일 패스 정적 분석기
// =============================================================
#include "../include/checker.h"
#include "../include/error.h"
#include <iostream>
#include <cassert>

// ──────────────────────────────────────────────────────────────────
//  리포트
// ──────────────────────────────────────────────────────────────────
void BluskChecker::error(const std::string& msg, int line) {
    BluskError::report("[Checker] " + msg, filename, line);
    result.hasError = true; result.errorCount++;
}
void BluskChecker::warn(const std::string& msg, int line) {
    BluskError::warn("[Checker] " + msg, filename, line);
    result.warnCount++;
}

// ──────────────────────────────────────────────────────────────────
//  스코프 관리
// ──────────────────────────────────────────────────────────────────
void BluskChecker::pushScope() { scopeStack.push_back({}); scopeDepth++; }

void BluskChecker::popScope(bool emitDeadWarnings) {
    if (scopeStack.empty()) return;
    // 이 스코프에서 선언됐으나 사용 안 된 변수 → dead 경고
    auto& top = scopeStack.back();
    for (auto& [name, info] : top) {
        if (!info.isUsed && emitDeadWarnings) {
            warn("Variable '" + name + "' declared but never used", info.declLine);
            result.deadVars.insert(name);
        }
        // 수명 최종 결정: 이 스코프 안에서만 살고 탈출 안 함 → STATIC → rcSkip
        if (info.isUsed && info.lifetime == Lifetime::UNKNOWN) {
            info.lifetime = Lifetime::STATIC;
            info.rcSkip   = true;
            result.rcSkipVars.insert(name);
        }
        // Cycle GC 생략 가능 여부
        if (info.cycleSkip)
            result.cycleSkipVars.insert(name);
        if (info.isConst)
            result.constVars.insert(name);
        if (info.memProtected)
            result.memProtected.insert(name);
    }
    scopeStack.pop_back();
    scopeDepth--;
}

VarInfo* BluskChecker::findVar(const std::string& name) {
    for (int i = (int)scopeStack.size()-1; i >= 0; i--) {
        auto it = scopeStack[i].find(name);
        if (it != scopeStack[i].end()) return &it->second;
    }
    return nullptr;
}

VarInfo& BluskChecker::declareVar(const std::string& name, int line, bool isConst) {
    if (scopeStack.empty()) pushScope();
    auto& top = scopeStack.back();
    if (top.count(name))
        warn("Variable '" + name + "' shadows outer declaration", line);
    VarInfo info; info.name = name; info.declLine = line; info.isConst = isConst;
    top[name] = info;
    return top[name];
}

void BluskChecker::markUsed(const std::string& name, int line) {
    VarInfo* v = findVar(name);
    if (v) { v->isUsed = true; v->refCount++; return; }
    // 키워드나 리터럴이면 조용히 패스
    if (name.empty() || std::isdigit(name[0]) || name == "true"
        || name == "false" || name == "nil") return;
}

// ──────────────────────────────────────────────────────────────────
//  참조 그래프
// ──────────────────────────────────────────────────────────────────
void BluskChecker::addRefEdge(const std::string& from, ASTNode* rhs) {
    if (!rhs) return;
    // NEW_EXPR, VAR_REF 우변에서 참조 대상 추출
    if (rhs->type == "VAR_REF" && !rhs->value.empty())
        refGraph[from].insert(rhs->value);
    if (rhs->type == "NEW_EXPR")
        for (ASTNode* child : rhs->children)
            if (child) refGraph[from].insert(child->value);
    for (ASTNode* child : rhs->children) addRefEdge(from, child);
}

bool BluskChecker::hasCycle(const std::string& start,
                             std::unordered_set<std::string>& visited,
                             std::unordered_set<std::string>& rec)
{
    visited.insert(start); rec.insert(start);
    auto it = refGraph.find(start);
    if (it != refGraph.end()) {
        for (const auto& nb : it->second) {
            if (!visited.count(nb) && hasCycle(nb, visited, rec)) return true;
            if (rec.count(nb)) return true;
        }
    }
    rec.erase(start); return false;
}

// ──────────────────────────────────────────────────────────────────
//  표현식에서 사용 추적 (재귀)
// ──────────────────────────────────────────────────────────────────
void BluskChecker::collectUsages(ASTNode* node) {
    if (!node) return;
    const std::string& t = node->type;
    const std::string& v = node->value;

    if (t == "VAR_REF" || t == "LOAD") { markUsed(v, node->line); }

    if (t == "BIN_OP" || t == "NEG") {
        for (ASTNode* c : node->children) collectUsages(c);
        return;
    }
    if (t == "ARRAY_GET" || t == "ARRAY_SIZE") {
        markUsed(v, node->line);
        for (ASTNode* c : node->children) collectUsages(c);
        return;
    }
    if (t == "MATH_CALL" || t == "MATH_CONST") {
        for (ASTNode* c : node->children) collectUsages(c);
        return;
    }
    for (ASTNode* c : node->children) collectUsages(c);
}

// ──────────────────────────────────────────────────────────────────
//  단일 패스 분석  —  핵심 함수
// ──────────────────────────────────────────────────────────────────
void BluskChecker::analyze(ASTNode* node) {
    if (!node) return;
    const std::string& t = node->type;
    const std::string& v = node->value;

    // ── 어노테이션 ────────────────────────────────────────────────
    if (t == "ANNOTATION") {
        if (v == "native")         nextIsNative       = true;
        if (v == "vm")             nextIsVm           = true;
        if (v == "unsafe")         nextIsUnsafe       = true;
        if (v == "Newmemorycancel")nextIsNewMemCancel = true;
        return;
    }

    // ── @Newmemorycancel 마킹: 다음 함수/블록에 적용 ─────────────
    bool applyMemCancel = nextIsNewMemCancel;
    bool applyNative    = nextIsNative;
    nextIsNative = nextIsVm = nextIsUnsafe = nextIsNewMemCancel = false;

    // ── ROOT / MAIN_BLOCK ─────────────────────────────────────────
    if (t == "ROOT") {
        // import blusk26 체크
        bool hasBlusk26 = false;
        for (ASTNode* child : node->children)
            if (child && child->type == "IMPORT" && child->value == "blusk26")
                hasBlusk26 = true;
        if (!hasBlusk26)
            error("import blusk26 is required", 0);
        pushScope();
        for (ASTNode* child : node->children) analyze(child);
        popScope(false);
        return;
    }

    // ── BLOCK ─────────────────────────────────────────────────────
    if (t == "BLOCK" || t == "MAIN_BLOCK") {
        pushScope();
        for (ASTNode* child : node->children) analyze(child);
        // 스코프 내 순환 탐지
        if (!scopeStack.empty()) {
            for (auto& [name, _] : scopeStack.back()) {
                std::unordered_set<std::string> vis, rec;
                bool cycle = hasCycle(name, vis, rec);
                VarInfo* vi = findVar(name);
                if (vi) {
                    vi->cycleSkip = !cycle;
                    if (cycle) {
                        warn("Potential circular reference at '" + name
                             + "' — Cycle GC will handle at runtime", vi->declLine);
                        vi->lifetime = Lifetime::CYCLIC_RISK;
                    }
                }
            }
        }
        popScope(true);
        return;
    }

    // ── IMPORT ────────────────────────────────────────────────────
    if (t == "IMPORT") return;

    // ── CLASS_DECL ────────────────────────────────────────────────
    if (t == "CLASS_DECL") {
        pushScope();
        for (ASTNode* child : node->children) analyze(child);
        popScope(false);
        return;
    }

    // ── VAR_DECL ─────────────────────────────────────────────────
    if (t == "VAR_DECL") {
        bool isConst = (v == "num");
        if (node->children.empty()) return;
        ASTNode* nameNode = node->children[0];
        const std::string& varName = nameNode->value;

        VarInfo& info = declareVar(varName, node->line, isConst);

        // @Newmemorycancel 적용
        if (applyMemCancel) {
            info.memProtected = true;
            info.rcSkip = true;
            result.memProtected.insert(varName);
        }

        // 우변 분석 + 참조 그래프
        if (!nameNode->children.empty()) {
            ASTNode* rhs = nameNode->children[0];
            collectUsages(rhs);
            addRefEdge(varName, rhs);
            // NEW_EXPR은 힙 → 수명 분석 필요, 아니면 STATIC 후보
            if (rhs->type != "NEW_EXPR") {
                info.lifetime = Lifetime::STATIC;
                if (!applyMemCancel) info.rcSkip = true;
            }
        } else {
            info.lifetime = Lifetime::STATIC;
            info.rcSkip   = true;
        }
        return;
    }

    // ── ARRAY_DECL ────────────────────────────────────────────────
    if (t == "ARRAY_DECL") {
        if (node->children.empty()) return;
        ASTNode* nameNode = node->children[0];
        VarInfo& info = declareVar(nameNode->value, node->line, false);
        info.lifetime = Lifetime::STATIC;
        info.rcSkip   = true;
        for (ASTNode* elem : nameNode->children) collectUsages(elem);
        return;
    }

    // ── ASSIGN ────────────────────────────────────────────────────
    if (t == "ASSIGN") {
        VarInfo* vi = findVar(v);
        if (vi) {
            // 상수 재대입 방지
            if (vi->isConst)
                error("Cannot reassign constant '" + v + "'", node->line);
            // @Newmemorycancel 보호 구역 쓰기 방지
            if (vi->memProtected)
                error("@Newmemorycancel: write to protected memory '" + v + "'", node->line);
            // Borrow: MUT_BORROWED 상태면 대입 불가
            if (vi->ownerState == OwnerState::MUT_BORROWED)
                error("Cannot assign to mutably borrowed '" + v + "'", node->line);
            // 대입하면 수명이 STATIC → ESCAPES로 올라갈 수 있음
            if (!node->children.empty()) {
                addRefEdge(v, node->children[0]);
                if (!node->children.empty() && node->children[0]->type == "NEW_EXPR")
                    vi->lifetime = Lifetime::ESCAPES;
            }
        }
        for (ASTNode* child : node->children) collectUsages(child);
        return;
    }

    // ── PRINT ─────────────────────────────────────────────────────
    if (t == "PRINT") {
        for (ASTNode* child : node->children) collectUsages(child);
        // f-string 안 변수명 추출
        for (size_t i = 0; i < v.size(); i++) {
            if (v[i] == '{') {
                size_t end = v.find('}', i+1);
                if (end != std::string::npos) {
                    std::string varInFmt = v.substr(i+1, end-i-1);
                    markUsed(varInFmt, node->line);
                    i = end;
                }
            }
        }
        return;
    }

    // ── IF / ELSEIF / ELSE ────────────────────────────────────────
    if (t == "IF" || t == "ELSEIF") {
        if (!node->children.empty()) {
            // 조건
            ASTNode* cond = node->children[0];
            if (cond && cond->children.size() >= 3) {
                markUsed(cond->children[0]->value, cond->line);
                markUsed(cond->children[2]->value, cond->line);
            }
        }
        for (ASTNode* child : node->children) analyze(child);
        return;
    }
    if (t == "ELSE") {
        for (ASTNode* child : node->children) analyze(child);
        return;
    }

    // ── FOR ───────────────────────────────────────────────────────
    if (t == "FOR") {
        pushScope();
        // 초기화 변수
        if (node->children.size() >= 1) {
            ASTNode* init = node->children[0];
            if (init && init->type == "VAR_DECL" && !init->children.empty()) {
                ASTNode* nn = init->children[0];
                VarInfo& info = declareVar(nn->value, init->line, false);
                info.lifetime = Lifetime::STATIC;
                info.rcSkip   = true;
                if (!nn->children.empty()) collectUsages(nn->children[0]);
            }
        }
        // 조건
        if (node->children.size() >= 2) {
            ASTNode* cond = node->children[1];
            if (cond && cond->children.size() >= 3) {
                markUsed(cond->children[0]->value, cond->line);
                markUsed(cond->children[2]->value, cond->line);
            }
        }
        // 스텝 변수 사용
        if (node->children.size() >= 3)
            markUsed(node->children[2]->value, node->line);
        // 바디
        if (node->children.size() >= 4) analyze(node->children[3]);
        popScope(true);
        return;
    }

    // ── WHILE ─────────────────────────────────────────────────────
    if (t == "WHILE") {
        if (!node->children.empty()) {
            ASTNode* cond = node->children[0];
            if (cond && cond->children.size() >= 3) {
                markUsed(cond->children[0]->value, cond->line);
                markUsed(cond->children[2]->value, cond->line);
            }
        }
        for (size_t i = 1; i < node->children.size(); i++) analyze(node->children[i]);
        return;
    }

    // ── LOOP ──────────────────────────────────────────────────────
    if (t == "LOOP") {
        for (ASTNode* child : node->children) analyze(child);
        return;
    }

    // ── METHOD_CALL ───────────────────────────────────────────────
    if (t == "METHOD_CALL") {
        // 객체 사용 마킹
        markUsed(v, node->line);
        // 인자 사용 추적 + Borrow: MOVED 상태 인자 에러
        for (size_t i = 1; i < node->children.size(); i++) {
            ASTNode* arg = node->children[i];
            if (!arg) continue;
            VarInfo* vi = findVar(arg->value);
            if (vi && vi->ownerState == OwnerState::MOVED)
                error("Use of moved value '" + arg->value + "'", arg->line);
            markUsed(arg->value, arg->line);
        }
        // 인자로 넘긴 객체는 수명이 ESCAPES로 변경
        for (size_t i = 1; i < node->children.size(); i++) {
            if (!node->children[i]) continue;
            VarInfo* vi = findVar(node->children[i]->value);
            if (vi && vi->lifetime == Lifetime::STATIC) {
                vi->lifetime = Lifetime::ESCAPES;
                vi->rcSkip   = false;
                result.rcSkipVars.erase(vi->name);
            }
        }
        return;
    }

    // ── AI 노드 ───────────────────────────────────────────────────
    if (t == "AI_LOAD" || t == "AI_ASK" || t == "AI_LEARN"
        || t == "AI_SAVE" || t == "AI_STATUS") {
        VarInfo& info = declareVar(v, node->line, false);
        info.isUsed  = true; // AI 모델 변수는 항상 used 취급
        info.rcSkip  = false; // 힙 객체 → RC 필요
        info.lifetime= Lifetime::ESCAPES;
        for (ASTNode* child : node->children) collectUsages(child);
        return;
    }

    // ── ARRAY_SET ─────────────────────────────────────────────────
    if (t == "ARRAY_SET") {
        markUsed(v, node->line);
        for (ASTNode* child : node->children) collectUsages(child);
        return;
    }

    // ── IO_READ ───────────────────────────────────────────────────
    if (t == "IO_READ") {
        if (!node->children.empty())
            declareVar(node->children[0]->value, node->line, false);
        return;
    }

    // ── THE_END / BREAK / CONTINUE ────────────────────────────────
    if (t == "THE_END" || t == "BREAK" || t == "CONTINUE") return;

    // ── TASK_SLEEP ────────────────────────────────────────────────
    if (t == "TASK_SLEEP") { collectUsages(node); return; }

    // ── 그 외: 자식 재귀 ─────────────────────────────────────────
    for (ASTNode* child : node->children) analyze(child);
}

// ──────────────────────────────────────────────────────────────────
//  진입점
// ──────────────────────────────────────────────────────────────────
CheckResult BluskChecker::check(ASTNode* root, const std::string& fn) {
    if (!root) return result;
    filename = fn; result = {};
    scopeDepth = 0; scopeStack.clear(); refGraph.clear();
    nextIsNative = nextIsVm = nextIsUnsafe = nextIsNewMemCancel = false;

    std::cerr << "[Checker] Analyzing: " << fn << "\n";

    analyze(root);

    // ── 순환 가능성 최종 정리 ────────────────────────────────────
    // rcSkipVars에서 CYCLIC_RISK인 것 제거
    for (const auto& [name, neighbors] : refGraph) {
        std::unordered_set<std::string> vis, rec;
        if (hasCycle(name, vis, rec)) {
            result.rcSkipVars.erase(name);
            result.cycleSkipVars.erase(name);
        } else {
            result.cycleSkipVars.insert(name);
        }
    }

    // ── 요약 출력 ────────────────────────────────────────────────
    std::cerr << "[Checker] Done."
              << " errors="    << result.errorCount
              << " warnings="  << result.warnCount
              << " dead="      << result.deadVars.size()
              << " rcSkip="    << result.rcSkipVars.size()
              << " cycleSkip=" << result.cycleSkipVars.size()
              << " memProt="   << result.memProtected.size()
              << "\n";

    if (!result.rcSkipVars.empty()) {
        std::cerr << "[Checker] RC-skip (no ref-counting):";
        for (auto& n : result.rcSkipVars) std::cerr << " " << n;
        std::cerr << "\n";
    }
    if (!result.deadVars.empty()) {
        std::cerr << "[Checker] Dead vars (skipped in compile):";
        for (auto& n : result.deadVars) std::cerr << " " << n;
        std::cerr << "\n";
    }
    if (!result.memProtected.empty()) {
        std::cerr << "[Checker] @Newmemorycancel regions:";
        for (auto& n : result.memProtected) std::cerr << " " << n;
        std::cerr << "\n";
    }

    return result;
}
