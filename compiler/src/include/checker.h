// =============================================================
//  BLUSK checker.h  -  BluskChecker (단일 패스, 완전 구현)
// =============================================================
#pragma once
#include "ast.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

// ------------------------------------------------------------------
//  수명 종류
// ------------------------------------------------------------------
enum class Lifetime { UNKNOWN, STATIC, ESCAPES, CYCLIC_RISK };

// ------------------------------------------------------------------
//  소유권 상태 (Borrow Checker)
// ------------------------------------------------------------------
enum class OwnerState { OWNED, BORROWED, MUT_BORROWED, MOVED, FREED };

// ------------------------------------------------------------------
//  OP_STORE intVal 인코딩 (SVM, Compiler 공용)
//
//  0 = 일반 변수
//  1 = 상수 (num 키워드)
//  2 = rcSkip  (Checker 확정 — RC 생략)
//  3 = 상수 + rcSkip
// ------------------------------------------------------------------
enum StoreFlag : int64_t {
    STORE_NORMAL   = 0,
    STORE_CONST    = 1,
    STORE_RCSKIP   = 2,
    STORE_CONST_RC = 3,
};

// ------------------------------------------------------------------
//  변수 심볼 정보
// ------------------------------------------------------------------
struct VarInfo {
    std::string name;
    int         declLine    = 0;
    bool        isConst     = false;
    bool        isUsed      = false;
    bool        rcSkip      = false;
    bool        cycleSkip   = false;
    bool        memProtected= false;
    Lifetime    lifetime    = Lifetime::UNKNOWN;
    OwnerState  ownerState  = OwnerState::OWNED;
    int         refCount    = 0;
    std::string borrowedBy;
};

// ------------------------------------------------------------------
//  Checker 결과 (Compiler에 전달)
// ------------------------------------------------------------------
struct CheckResult {
    bool hasError   = false;
    int  errorCount = 0;
    int  warnCount  = 0;

    std::unordered_set<std::string> rcSkipVars;    // RC 생략 확정
    std::unordered_set<std::string> cycleSkipVars; // Cycle GC 생략
    std::unordered_set<std::string> deadVars;       // 미사용 → 컴파일 제외
    std::unordered_set<std::string> constVars;      // 상수 목록
    std::unordered_set<std::string> memProtected;   // @Newmemorycancel 보호 구역
};

// ------------------------------------------------------------------
//  BluskChecker
// ------------------------------------------------------------------
class BluskChecker {
public:
    CheckResult check(ASTNode* root, const std::string& filename = "unknown");

private:
    std::string filename;
    CheckResult result;

    // ── 단일 플랫 변수 테이블 (scope id, name) ───────────────────
    // scope 깊이는 scopeDepth로 추적, 변수는 scopeId_name 키로 저장
    int  scopeDepth = 0;
    // varTable[name] = 현재 스코프에서 가장 가까운 VarInfo
    // 스코프 스택: vector<unordered_map>
    std::vector<std::unordered_map<std::string, VarInfo>> scopeStack;

    // 참조 그래프 (순환 탐지)
    std::unordered_map<std::string, std::unordered_set<std::string>> refGraph;

    // 어노테이션 상태 (다음 노드에 적용)
    bool nextIsNative      = false;
    bool nextIsVm          = false;
    bool nextIsUnsafe      = false;
    bool nextIsNewMemCancel= false;

    // ── 스코프 ────────────────────────────────────────────────────
    void pushScope();
    void popScope(bool emitDeadWarnings = true);
    VarInfo* findVar(const std::string& name);
    VarInfo& declareVar(const std::string& name, int line, bool isConst = false);
    void     markUsed(const std::string& name, int line = 0);

    // ── 단일 패스 분석 ────────────────────────────────────────────
    void analyze(ASTNode* node);

    // ── 표현식에서 사용되는 변수 수집 ────────────────────────────
    void collectUsages(ASTNode* node);

    // ── 수명 / RC 판단 ────────────────────────────────────────────
    void finalizeScope();  // popScope 직전에 수명 결정

    // ── 참조 그래프 ───────────────────────────────────────────────
    void addRefEdge(const std::string& from, ASTNode* rhs);
    bool hasCycle(const std::string& start,
                  std::unordered_set<std::string>& visited,
                  std::unordered_set<std::string>& rec);

    // ── 리포트 ────────────────────────────────────────────────────
    void error(const std::string& msg, int line);
    void warn (const std::string& msg, int line);
};
