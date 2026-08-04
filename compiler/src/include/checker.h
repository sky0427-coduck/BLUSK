// =============================================================
//  BLUSK checker.h  -  BluskChecker (단일 패스, 완전 구현)
//  StoreFlag는 opcode.h에서 단일 정의 (재정의 버그 수정)
// =============================================================
#pragma once
#include "ast.h"
#include "opcode.h"   // StoreFlag 정의 위치 (단일 소스)
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

    // ── 타입 시스템 (narrowing 경고용) ───────────────────────────
    // "gg"/"num"/"int"/"long"/"float"/"double"/"str"/"bool" 등
    // VAR_DECL 키워드 그대로 저장. 비어있으면 추론(gg)됨.
    std::string declaredType;
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

    // Declared type keyword per variable ("int"/"long"/"float"/"double"/
    // "gg"/"num"/...). Used by the compiler to auto-cast arithmetic
    // results back to the declared width on assignment, e.g.
    // "int x = x + 1;" stays 32-bit instead of silently widening.
    std::unordered_map<std::string, std::string> varTypes;
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

    int  scopeDepth = 0;
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
    VarInfo& declareVar(const std::string& name, int line, bool isConst = false,
                        const std::string& declaredType = "");
    void     markUsed(const std::string& name, int line = 0);

    // ── 내장 네임스페이스 사전 스캔 ───────────────────────────────
    // Runs once, before the main analyze() pass, over the top-level
    // IMPORT nodes. Populates reservedNamespaces with the bare
    // identifiers ("Math", "io", "string", "time", "collections") that
    // the parser treats specially when followed by "." -- e.g. "Math"
    // immediately before "." triggers Math-call parsing regardless of
    // whether the user has also declared a variable named "Math". This
    // lets declareVar() warn about the shadowing before it silently
    // breaks those built-in calls elsewhere in the file.
    std::unordered_set<std::string> reservedNamespaces;
    void scanReservedNamespaces(ASTNode* root);

    // ── 단일 패스 분석 ────────────────────────────────────────────
    void analyze(ASTNode* node);

    // ── 표현식에서 사용되는 변수 수집 ────────────────────────────
    void collectUsages(ASTNode* node);

    // ── 타입 폭 비교 (narrowing 경고용) ──────────────────────────
    // 반환값: 0=동일/문제없음, 1=narrowing(정밀도 손실 가능), -1=타입불일치
    static int typeWidthCompare(const std::string& fromType, const std::string& toType);

    // ── 수명 / RC 판단 ────────────────────────────────────────────
    void finalizeScope();

    // ── 참조 그래프 ───────────────────────────────────────────────
    void addRefEdge(const std::string& from, ASTNode* rhs);
    bool hasCycle(const std::string& start,
                  std::unordered_set<std::string>& visited,
                  std::unordered_set<std::string>& rec);

    // ── 리포트 ────────────────────────────────────────────────────
    void error(const std::string& msg, int line);
    void warn (const std::string& msg, int line);
};
