// =============================================================
//  BLUSK checker.h  -  BluskChecker (단일 패스, 완전 구현)
//  StoreFlag는 opcode.h에서 단일 정의 (재정의 버그 수정)
// =============================================================
#pragma once
#include "ast.h"
#include "opcode.h"   // StoreFlag 정의 위치 (단일 소스)
#include "value.h"
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

    // Constant-expression values the Checker has already fully computed
    // at check time (e.g. "2 + 3" -> 5), keyed by the AST node it
    // evaluated. The Compiler looks this map up before walking an
    // expression subtree: a hit means it can emit a single LOAD of the
    // cached value instead of regenerating the ADD/MUL/etc. instructions
    // that would otherwise recompute the same result at every run. Keyed
    // by node pointer rather than variable name so it's unambiguous even
    // when the same variable is reassigned with different expressions at
    // different points in the file, and so it applies to any constant
    // sub-expression, not just whole initializers.
    std::unordered_map<const ASTNode*, Value> foldedConsts;
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

    // ── 상수 표현식 사전 계산 (constant folding) ─────────────────
    // Attempts to fully evaluate `node` at check time using only
    // literals and operators the Checker can already resolve (NUM_LIT,
    // STR_LIT, BOOL_LIT, arithmetic/logic/comparison BIN_OP, NEG,
    // LOGIC_NOT, CAST). On success, caches the result in
    // result.foldedConsts[node] and returns true; recurses into children
    // first, so a subtree like "(2+3) * x" still folds its constant
    // half even though the whole expression isn't constant. Does not
    // (yet) propagate through variable references -- only literal
    // arithmetic is folded, not "num PI = 3.14; ... PI * 2".
    bool tryFold(ASTNode* node, Value& out);

    // Convenience used at every "root expression" site (conditions,
    // returns, switch subjects, ...): tracks variable usages the normal
    // way, then attempts constant folding on the same subtree.
    void foldIfPossible(ASTNode* node);

    // ── 객체 RC-skip을 위한 이스케이프 분석 ─────────────────────────
    // A variable declared as "gg x = new Thing(...)" is a candidate for
    // real RC-skip: if it's provably never aliased into another
    // variable, passed as a constructor/method argument, stored into an
    // array, or returned, it's the *sole* owner of that object for its
    // entire lifetime, and the VM can skip all reference-counting for
    // it -- the object is never even registered with the GC (see
    // BluskGC::isTracked/incRef/decRef in gc.h), so there's zero
    // runtime bookkeeping cost. A variable that fails any of those
    // checks still runs correctly; it just keeps paying for real RC.
    //
    // This can never mark something rcSkip that's actually unsafe to
    // skip: an rcSkip object is simply never registered with the GC at
    // all, and its real memory is still safely managed by
    // std::shared_ptr underneath regardless of our bookkeeping being
    // wrong -- worst case is a missed optimization, never a
    // use-after-free or leak-at-the-C++-level.
    std::unordered_set<std::string> objectCandidates_;
    std::unordered_set<std::string> escapedVars_;

    void markEscaped(const std::string& name);
    // Recursively marks every VAR_REF found anywhere under `node` as
    // escaped -- used wherever a value is known to be handed off
    // somewhere that could outlive the current binding (constructor
    // arguments, call arguments, array elements, return values).
    void markSubtreeEscaped(ASTNode* node);
    // Runs once after the main analyze() pass: every object candidate
    // that was never marked escaped becomes a confirmed RC-skip.
    void finalizeRcSkipCandidates();

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
