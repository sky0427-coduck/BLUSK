// =============================================================
//  BLUSK checker.cpp  -  single-pass analysis + narrowing warnings
//  + SWITCH/MATRIX_DECL/SIMD_FOR dedicated handling
//  + f-string {var} usage tracking (prevents false dead-code elimination)
// =============================================================
#include "../include/checker.h"
#include "../include/error.h"
#include <iostream>
#include <algorithm>

// ── Type width comparison ────────────────────────────────────────
// rank: int(0) < long(1) < float(2) < double(3)
// returns: 0 = safe (same or widening), 1 = narrowing (precision loss),
//          0 also returned for non-numeric types (str/bool) since the
//          check doesn't apply there.
int BluskChecker::typeWidthCompare(const std::string& fromType, const std::string& toType) {
    auto rank = [](const std::string& t) -> int {
        if (t=="int")    return 0;
        if (t=="long" || t=="gg" || t.empty()) return 1; // gg's integer default is long
        if (t=="float")  return 2;
        if (t=="double") return 3;
        return -100; // not a numeric type
    };
    int rf = rank(fromType), rt = rank(toType);
    if (rf == -100 || rt == -100) return 0;
    if (rf <= rt) return 0;
    return 1;
}

// ── Scope management ──────────────────────────────────────────────
void BluskChecker::pushScope() {
    scopeStack.push_back({});
    scopeDepth++;
}

void BluskChecker::popScope(bool emitDeadWarnings) {
    if (scopeStack.empty()) return;
    auto& top = scopeStack.back();
    for (auto& [name, info] : top) {
        if (emitDeadWarnings && !info.isUsed && !info.name.empty()) {
            result.deadVars.insert(name);
            warn("Variable '" + name + "' declared but never used", info.declLine);
        }
        if (!info.isUsed && info.refCount <= 1) {
            result.rcSkipVars.insert(name);
        }
        if (info.memProtected) result.memProtected.insert(name);
        if (info.isConst)      result.constVars.insert(name);
        if (!info.declaredType.empty()) result.varTypes[name] = info.declaredType;
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

VarInfo& BluskChecker::declareVar(const std::string& name, int line, bool isConst,
                                   const std::string& declaredType) {
    if (scopeStack.empty()) pushScope();
    VarInfo info;
    info.name        = name;
    info.declLine    = line;
    info.isConst     = isConst;
    info.declaredType= declaredType;
    info.lifetime    = Lifetime::UNKNOWN;
    if (nextIsNewMemCancel) { info.memProtected = true; nextIsNewMemCancel = false; }
    scopeStack.back()[name] = info;
    return scopeStack.back()[name];
}

void BluskChecker::markUsed(const std::string& name, int line) {
    VarInfo* v = findVar(name);
    if (v) { v->isUsed = true; v->refCount++; }
}

// ── Reference graph (cycle detection) ────────────────────────────
void BluskChecker::addRefEdge(const std::string& from, ASTNode* rhs) {
    if (!rhs) return;
    if (rhs->type=="VAR_REF" || rhs->type=="NEW_EXPR") {
        refGraph[from].insert(rhs->value);
    }
}

bool BluskChecker::hasCycle(const std::string& start,
                             std::unordered_set<std::string>& visited,
                             std::unordered_set<std::string>& rec) {
    if (rec.count(start)) return true;
    if (visited.count(start)) return false;
    visited.insert(start);
    rec.insert(start);
    if (refGraph.count(start)) {
        for (auto& next : refGraph[start]) {
            if (hasCycle(next, visited, rec)) return true;
        }
    }
    rec.erase(start);
    return false;
}

// ── Single-pass analysis ──────────────────────────────────────────
void BluskChecker::analyze(ASTNode* node) {
    if (!node) return;
    const std::string& t = node->type;
    const std::string& v = node->value;

    // ── Annotation state tracking ────────────────────────────────
    if (t == "ANNOTATION") {
        if (v=="native")            nextIsNative = true;
        else if (v=="vm")           nextIsVm = true;
        else if (v=="unsafe")       nextIsUnsafe = true;
        else if (v=="newmemorycancel" || v=="Newmemorycancel")
            nextIsNewMemCancel = true;
        for (auto* c : node->children) analyze(c);
        nextIsNative = nextIsVm = nextIsUnsafe = false;
        return;
    }

    if (t == "ROOT" || t == "MAIN_BLOCK" || t == "BLOCK") {
        bool isBlock = (t == "BLOCK");
        if (isBlock) pushScope();
        for (auto* c : node->children) analyze(c);
        if (isBlock) popScope();
        return;
    }

    // ── VAR_DECL : includes narrowing warning check ──────────────
    if (t == "VAR_DECL") {
        if (node->children.empty()) return;
        ASTNode* nameNode = node->children[0];
        const std::string& name = nameNode->value;
        bool isConst = (v == "num");
        std::string declaredType = v; // "gg"/"num"/"int"/"long"/"float"/"double"/"str"/"bool"

        declareVar(name, node->line, isConst, declaredType);

        if (!nameNode->children.empty()) {
            ASTNode* rhs = nameNode->children[0];
            collectUsages(rhs);
            addRefEdge(name, rhs);

            // narrowing warning: rhs is a variable reference, compare widths
            if (rhs->type == "VAR_REF") {
                VarInfo* srcVar = findVar(rhs->value);
                if (srcVar && !srcVar->declaredType.empty() && !declaredType.empty()) {
                    int cmp = typeWidthCompare(srcVar->declaredType, declaredType);
                    if (cmp == 1) {
                        warn("Possible precision loss: assigning '" + srcVar->declaredType +
                             "' (" + rhs->value + ") to narrower type '" + declaredType +
                             "' (" + name + ")", node->line);
                    }
                }
            }
        }
        return;
    }

    // ── ASSIGN : narrowing check applies on reassignment too ─────
    if (t == "ASSIGN") {
        VarInfo* target = findVar(v);
        if (target) {
            if (target->isConst) {
                error("Cannot reassign constant '" + v + "'", node->line);
            }
            markUsed(v, node->line);
            if (!node->children.empty()) {
                ASTNode* rhs = node->children[0];
                collectUsages(rhs);
                if (rhs->type == "VAR_REF" && !target->declaredType.empty()) {
                    VarInfo* srcVar = findVar(rhs->value);
                    if (srcVar && !srcVar->declaredType.empty()) {
                        int cmp = typeWidthCompare(srcVar->declaredType, target->declaredType);
                        if (cmp == 1) {
                            warn("Possible precision loss: assigning '" + srcVar->declaredType +
                                 "' (" + rhs->value + ") to narrower type '" +
                                 target->declaredType + "' (" + v + ")", node->line);
                        }
                    }
                }
            }
        } else {
            if (!node->children.empty()) collectUsages(node->children[0]);
        }
        return;
    }

    // ── PRINT_FSTR : parse {var} patterns inside the f-string
    //    template text directly. Without this, the Checker can't see
    //    these usages (they're embedded in a plain string, not real
    //    AST child nodes) and incorrectly marks the variable as dead,
    //    which causes the compiler to skip its STORE entirely. ──────
    if (t == "PRINT_FSTR") {
        const std::string& tmpl = v;
        for (size_t i = 0; i < tmpl.size(); i++) {
            if (tmpl[i] == '{') {
                size_t e = tmpl.find('}', i+1);
                if (e == std::string::npos) break;
                std::string varName = tmpl.substr(i+1, e-i-1);
                if (!varName.empty()) markUsed(varName, node->line);
                i = e;
            }
        }
        for (auto* c : node->children) collectUsages(c);
        return;
    }
    if (t == "PRINT") {
        for (auto* c : node->children) collectUsages(c);
        return;
    }

    // ── IF / FOR / WHILE : recurse into condition and body ───────
    if (t == "IF") {
        for (auto* c : node->children) {
            if (c->type == "CONDITION") collectUsages(c);
            else if (c->type == "ELSEIF") { collectUsages(c->children[0]); analyze(c->children[1]); }
            else if (c->type == "ELSE")   analyze(c->children[0]);
            else analyze(c);
        }
        return;
    }
    if (t == "FOR" || t == "WHILE" || t == "FOR_IN") {
        for (auto* c : node->children) {
            if (c->type == "CONDITION") collectUsages(c);
            analyze(c);
        }
        return;
    }

    // ── SWITCH : children[0] is the switch expression (needs usage
    //    tracking), the rest are CASE/DEFAULT nodes whose own children
    //    (the case body) need to be analyzed normally. ─────────────
    if (t == "SWITCH") {
        if (!node->children.empty()) collectUsages(node->children[0]);
        for (size_t ci = 1; ci < node->children.size(); ci++) {
            ASTNode* c = node->children[ci];
            for (auto* cc : c->children) analyze(cc);
        }
        return;
    }

    // ── MATRIX_DECL : declare the variable with type "Matrix" so it
    //    participates in scope/RC tracking like any other variable. ─
    if (t == "MATRIX_DECL") {
        if (!node->children.empty()) {
            ASTNode* nameNode = node->children[0];
            declareVar(nameNode->value, node->line, false, "Matrix");
        }
        return;
    }

    // ── SIMD_FOR : @simd for varname; counts as a usage of varname ──
    if (t == "SIMD_FOR") {
        markUsed(v, node->line);
        return;
    }

    // ── CLASS_DECL : member variable tracking (simplified for now) ──
    if (t == "CLASS_DECL") {
        for (auto* c : node->children) analyze(c);
        return;
    }

    // Default: recurse into children
    for (auto* c : node->children) analyze(c);
}

// ── Collect variable usages inside an expression subtree ─────────
void BluskChecker::collectUsages(ASTNode* node) {
    if (!node) return;
    if (node->type == "VAR_REF") {
        markUsed(node->value, node->line);
    }
    for (auto* c : node->children) collectUsages(c);
}

// ── Reporting ──────────────────────────────────────────────────────
void BluskChecker::error(const std::string& msg, int line) {
    BluskError::report(msg, filename, line);
    result.hasError = true;
    result.errorCount++;
}

void BluskChecker::warn(const std::string& msg, int line) {
    BluskError::warn(msg, filename, line);
    result.warnCount++;
}

// ── Public API ──────────────────────────────────────────────────────
CheckResult BluskChecker::check(ASTNode* root, const std::string& fn) {
    filename = fn;
    result = CheckResult{};
    scopeStack.clear();
    refGraph.clear();
    scopeDepth = 0;

    std::cerr << "[Checker] Analyzing: " << filename << "\n";

    pushScope();
    analyze(root);
    popScope();

    std::unordered_set<std::string> visited, rec;
    for (auto& [name, _] : refGraph) {
        if (!visited.count(name) && hasCycle(name, visited, rec)) {
            result.cycleSkipVars.insert(name);
            warn("Potential reference cycle detected involving '" + name + "'", 0);
        }
    }

    std::cerr << "[Checker] Done. errors=" << result.errorCount
              << " warnings=" << result.warnCount
              << " dead=" << result.deadVars.size()
              << " rcSkip=" << result.rcSkipVars.size()
              << " cycleSkip=" << result.cycleSkipVars.size() << "\n";

    if (!result.rcSkipVars.empty()) {
        std::cerr << "[Checker] RC-skip (no ref-counting): ";
        for (auto& n : result.rcSkipVars) std::cerr << n << " ";
        std::cerr << "\n";
    }
    if (!result.deadVars.empty()) {
        std::cerr << "[Checker] Dead vars (skipped in compile): ";
        for (auto& n : result.deadVars) std::cerr << n << " ";
        std::cerr << "\n";
    }

    return result;
}
