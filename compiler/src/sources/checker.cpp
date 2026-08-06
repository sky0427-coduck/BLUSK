// =============================================================
//  BLUSK checker.cpp  -  single-pass analysis + narrowing warnings
//  + SWITCH/MATRIX_DECL/SIMD_FOR dedicated handling
//  + f-string {var} usage tracking (prevents false dead-code elimination)
//  + constant folding (Checker precomputes constant expressions and
//    hands the result to the Compiler, which emits a single LOAD
//    instead of regenerating the ADD/MUL/etc. instructions)
//  + real escape-analysis RC-skip (single-owner "new Thing()" bindings
//    that never alias/escape skip all GC bookkeeping entirely)
// =============================================================
#include "../include/checker.h"
#include "../include/error.h"
#include <iostream>
#include <algorithm>
#include <cctype>

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

// ── 내장 네임스페이스 사전 스캔 ───────────────────────────────────
// "task" is always reserved -- task.sleep(...) is parsed unconditionally,
// with no import required. The rest depend on which imports are present.
void BluskChecker::scanReservedNamespaces(ASTNode* root) {
    reservedNamespaces.clear();
    reservedNamespaces.insert("task");
    if (!root) return;
    for (auto* c : root->children) {
        if (!c || c->type != "IMPORT") continue;
        const std::string& v = c->value;
        if (v == "io")                    reservedNamespaces.insert("io");
        else if (v == "Blusk.num.Math")   reservedNamespaces.insert("Math");
        else if (v == "string")           reservedNamespaces.insert("string");
        else if (v == "time")             reservedNamespaces.insert("time");
        else if (v == "collections")      reservedNamespaces.insert("collections");
    }
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
    if (reservedNamespaces.count(name)) {
        warn("Variable '" + name + "' shadows the built-in '" + name +
             "' namespace -- " + name + ".xxx(...) calls elsewhere in this "
             "file will stop working. Consider renaming this variable.", line);
    }
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

            // Try to fully evaluate the initializer now, e.g. "2 + 3" ->
            // 5, so the Compiler can emit a single LOAD instead of the
            // ADD instruction (and everything it depends on) every run.
            Value folded;
            if (tryFold(rhs, folded)) result.foldedConsts[rhs] = folded;

            // Escape analysis for RC-skip (see markSubtreeEscaped/
            // finalizeRcSkipCandidates): a fresh "new Thing()" binding
            // is a single-owner candidate. Its own constructor arguments
            // are a different story -- handing an existing variable into
            // a new object's fields means that variable itself no
            // longer has a single, provable owner.
            if (rhs->type == "NEW_EXPR") {
                objectCandidates_.insert(name);
                for (auto* arg : rhs->children) markSubtreeEscaped(arg);
            } else if (rhs->type == "VAR_REF") {
                // Aliasing: "gg b = a;" -- both names now reference the
                // same object, so neither can ever be treated as sole
                // owner again. This is whole-program, per-name analysis
                // (not per-SSA-binding), so there's no way to later
                // distinguish "b before this alias" from "b after" --
                // once a name has EVER shared an object with another
                // name, marking just one side as escaped would let a
                // later, unrelated store through the other name mark
                // the *shared object itself* rcSkip (BluskObject::rcSkip
                // is a field on the object, not the variable), silently
                // breaking the first name's RC too.
                markEscaped(rhs->value);
                markEscaped(name);
            } else {
                markSubtreeEscaped(rhs); // conservative catch-all
            }

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
                Value folded;
                if (tryFold(rhs, folded)) result.foldedConsts[rhs] = folded;

                if (rhs->type == "NEW_EXPR") {
                    objectCandidates_.insert(v);
                    for (auto* arg : rhs->children) markSubtreeEscaped(arg);
                } else if (rhs->type == "VAR_REF") {
                    // Same reasoning as VAR_DECL's aliasing case: both
                    // names must be permanently escaped.
                    markEscaped(rhs->value);
                    markEscaped(v);
                } else {
                    markSubtreeEscaped(rhs);
                }
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
        // print("fmt", arg) parses its second argument as a PRINT_ARG /
        // ARRAY_SIZE / ARRAY_GET node carrying the variable name in
        // ->value directly (not as a VAR_REF child), so plain
        // collectUsages() never sees it. Mark it used explicitly here,
        // or the compiler treats the variable as dead and skips its
        // STORE entirely -- causing a runtime "Undefined variable" crash
        // even though the variable is genuinely used in this print call.
        for (auto* c : node->children) {
            if (c->type == "PRINT_ARG" || c->type == "ARRAY_SIZE") {
                markUsed(c->value, node->line);
            } else if (c->type == "ARRAY_GET") {
                markUsed(c->value, node->line);
            }
            collectUsages(c);
        }
        return;
    }

    // THE_END_IF / THE_END_RETURN / RETURN : their condition/return
    // expression must go through collectUsages(), not the generic
    // child-recursion below -- analyze() itself never marks VAR_REF used,
    // only collectUsages() does. Without this, any variable referenced
    // only inside "the end : if (...)" or "the end : return ..." gets
    // marked dead, its STORE skipped by the compiler, and the program
    // crashes at runtime the instant that line runs.
    if (t == "THE_END_IF" || t == "THE_END_RETURN" || t == "RETURN") {
        for (auto* c : node->children) {
            if (c->type == "CONDITION" && !c->children.empty()) foldIfPossible(c->children[0]);
            else foldIfPossible(c);
        }
        return;
    }

    // ── IF / FOR / WHILE : recurse into condition and body ───────
    if (t == "IF") {
        for (auto* c : node->children) {
            if (c->type == "CONDITION") {
                if (!c->children.empty()) foldIfPossible(c->children[0]);
            }
            else if (c->type == "ELSEIF") {
                ASTNode* condNode = c->children[0];
                if (condNode && !condNode->children.empty()) foldIfPossible(condNode->children[0]);
                analyze(c->children[1]);
            }
            else if (c->type == "ELSE")   analyze(c->children[0]);
            else analyze(c);
        }
        return;
    }
    if (t == "FOR" || t == "WHILE" || t == "FOR_IN") {
        for (auto* c : node->children) {
            if (c->type == "CONDITION") {
                if (c->children.size() == 1) {
                    // WHILE-style: single wrapped expression tree, foldable.
                    foldIfPossible(c->children[0]);
                } else {
                    // C-style for's 3-token COND_LEFT/COND_OP/COND_RIGHT
                    // form isn't a real expression tree -- just track
                    // usages the normal way, same as before.
                    collectUsages(c);
                }
            }
            else if (c->type == "ARRAY_REF") markUsed(c->value, c->line); // for-in's array name
            analyze(c);
        }
        return;
    }

    // ── SWITCH : children[0] is the switch expression (needs usage
    //    tracking), the rest are CASE/DEFAULT nodes whose own children
    //    (the case body) need to be analyzed normally. ─────────────
    if (t == "SWITCH") {
        if (!node->children.empty()) foldIfPossible(node->children[0]);
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

    // METHOD_CALL statements (obj.method(args);) store the object name
    // directly in ->value rather than as a VAR_REF child, so the generic
    // recursion below would never see it as a "usage" -- the object gets
    // marked dead, its STORE gets skipped by the compiler, and the whole
    // call silently becomes a no-op (no error, just nothing happens).
    if (t == "METHOD_CALL") {
        markUsed(v, node->line);
        for (auto* c : node->children) {
            // ARG nodes carry the argument's raw token value directly
            // (not as a VAR_REF child) -- mark used, and conservatively
            // treat it as escaped since the callee could retain it.
            if (c->type == "ARG") { markUsed(c->value, node->line); markEscaped(c->value); }
            analyze(c);
        }
        return;
    }

    // ARRAY_DECL was previously never declared in scope at all, which
    // meant array names were silently exempt from dead-code elimination
    // (safe by accident, not by design) and their elements were never
    // usage-tracked or constant-folded. Declaring them properly here
    // makes arrays consistent with every other variable kind.
    if (t == "ARRAY_DECL") {
        if (!node->children.empty()) {
            ASTNode* nameNode = node->children[0];
            declareVar(nameNode->value, node->line, false, v);
            for (auto* el : nameNode->children) {
                foldIfPossible(el);
                markSubtreeEscaped(el); // stored long-term in the array
            }
        }
        return;
    }
    if (t == "ARRAY_SET") {
        markUsed(v, node->line); // v = array name
        if (node->children.size() >= 1) collectUsages(node->children[0]); // index
        if (node->children.size() >= 2) {
            collectUsages(node->children[1]);      // value being stored
            markSubtreeEscaped(node->children[1]); // escapes into the array
        }
        return;
    }
    if (t == "ARRAY_GET") {
        markUsed(v, node->line);
        if (!node->children.empty()) collectUsages(node->children[0]);
        return;
    }

    // CLASS_DECL : member variable tracking (simplified for now) ──
    if (t == "CLASS_DECL") {
        for (auto* c : node->children) analyze(c);
        return;
    }

    // Default: recurse into children
    for (auto* c : node->children) analyze(c);
}

// Shared helper: does this raw token value look like a variable name
// (as opposed to a numeric/bool/nil literal)? Used wherever a node
// stores an operand's raw token directly in ->value instead of wrapping
// it in a real VAR_REF child (COND_LEFT/RIGHT, ARRAY_IDX, ...).
static bool looksLikeVarName(const std::string& val) {
    if (val.empty()) return false;
    if (std::isdigit((unsigned char)val[0]) || (val[0]=='-' && val.size()>1)) return false;
    if (val=="true" || val=="false" || val=="nil") return false;
    return true;
}

// ── Collect variable usages inside an expression subtree ─────────
void BluskChecker::collectUsages(ASTNode* node) {
    if (!node) return;
    if (node->type == "VAR_REF") {
        markUsed(node->value, node->line);
    }
    if (node->type == "METHOD_CALL_EXPR") {
        // ->value is "obj.method" concatenated together; the object part
        // is the actual variable usage.
        size_t dot = node->value.find('.');
        if (dot != std::string::npos) markUsed(node->value.substr(0, dot), node->line);
    }
    if (node->type == "COND_LEFT" || node->type == "COND_RIGHT") {
        // C-style for-loop conditions ("k <= n") are parsed token-by-token
        // into a flat 3-child CONDITION (COND_LEFT, COND_OP, COND_RIGHT)
        // rather than through the full expression parser, so operand
        // names live directly in ->value instead of as VAR_REF children.
        if (looksLikeVarName(node->value)) markUsed(node->value, node->line);
    }
    if (node->type == "ARRAY_IDX" && looksLikeVarName(node->value)) {
        markUsed(node->value, node->line);
    }
    for (auto* c : node->children) collectUsages(c);
}

// ── Constant folding: evaluate an expression at check time ─────────
// Bottom-up: children are folded first, so a partially-constant tree
// like "(2+3) * x" still gets its constant half cached even though the
// whole expression can't be. Bails out (returns false) the moment it
// hits anything it can't fully resolve -- variable references (no
// constant-propagation yet), object construction, method calls, array
// access, etc. -- since those genuinely can't be known until runtime.
bool BluskChecker::tryFold(ASTNode* node, Value& out) {
    if (!node) return false;
    const std::string& t = node->type;
    const std::string& v = node->value;

    if (t == "NUM_LIT") {
        try {
            if (v.find('.') != std::string::npos) out = Value::Float(std::stod(v));
            else                                    out = Value::Int(std::stoll(v));
        } catch (...) { return false; }
        return true;
    }
    if (t == "STR_LIT")  { out = Value::String(v); return true; }
    if (t == "BOOL_LIT") { out = Value::Bool(v == "true"); return true; }
    if (t == "NIL_LIT")  { out = Value::Nil(); return true; }

    if (t == "NEG") {
        if (node->children.empty()) return false;
        Value inner;
        if (!tryFold(node->children[0], inner)) return false;
        out = -inner;
        return true;
    }
    if (t == "LOGIC_NOT") {
        if (node->children.empty()) return false;
        Value inner;
        if (!tryFold(node->children[0], inner)) return false;
        out = Value::Bool(!inner.toBool());
        return true;
    }
    if (t == "CAST") {
        if (node->children.empty()) return false;
        Value inner;
        if (!tryFold(node->children[0], inner)) return false;
        out = inner.castTo(v);
        return true;
    }
    if (t == "BIN_OP" || t == "LOGIC_AND" || t == "LOGIC_OR") {
        if (node->children.size() < 2) return false;
        Value l, r;
        if (!tryFold(node->children[0], l)) return false;
        if (!tryFold(node->children[1], r)) return false;

        if (t == "LOGIC_AND") { out = Value::Bool(l.toBool() && r.toBool()); return true; }
        if (t == "LOGIC_OR")  { out = Value::Bool(l.toBool() || r.toBool()); return true; }

        // Division/modulo by a literal zero: don't fold it away here --
        // let it reach the compiler/runtime and raise the same
        // "Division by zero" error it always would, at the same point
        // in execution a developer would expect to see it.
        if ((v == "/" || v == "%") && r.isNum() && r.toDouble() == 0.0) return false;

        if      (v=="+")  out = l + r;              else if (v=="-")  out = l - r;
        else if (v=="*")  out = l * r;              else if (v=="/")  out = l / r;
        else if (v=="%")  out = l % r;              else if (v=="**") out = l.blusk_pow(r);
        else if (v=="==") out = Value::Bool(l == r); else if (v=="!=") out = Value::Bool(l != r);
        else if (v=="<")  out = Value::Bool(l <  r); else if (v=="<=") out = Value::Bool(l <= r);
        else if (v==">")  out = Value::Bool(l >  r); else if (v==">=") out = Value::Bool(l >= r);
        else return false;
        return true;
    }

    // FSTRING, VAR_REF, NEW_EXPR, METHOD_CALL_EXPR, ARRAY_GET, MATH_CALL,
    // FIELD_GET, etc. all depend on something not known until runtime
    // (or, for VAR_REF, would need full constant propagation, which this
    // pass doesn't attempt yet) -- not foldable.
    return false;
}

void BluskChecker::foldIfPossible(ASTNode* node) {
    collectUsages(node);
    Value folded;
    if (tryFold(node, folded)) result.foldedConsts[node] = folded;
}

void BluskChecker::markEscaped(const std::string& name) {
    escapedVars_.insert(name);
}

void BluskChecker::markSubtreeEscaped(ASTNode* node) {
    if (!node) return;
    if (node->type == "VAR_REF") markEscaped(node->value);
    // ARG nodes (statement-form method calls) and ARRAY_IDX/ARRAY_GET
    // (index expressions) carry a variable name directly in ->value
    // rather than as a VAR_REF child -- same pattern as everywhere else
    // in this file, so the same explicit check is needed here too.
    if (node->type == "ARG" || node->type == "PRINT_ARG") markEscaped(node->value);
    for (auto* c : node->children) markSubtreeEscaped(c);
}

void BluskChecker::finalizeRcSkipCandidates() {
    for (auto& name : objectCandidates_) {
        if (!escapedVars_.count(name)) result.rcSkipVars.insert(name);
    }
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
    objectCandidates_.clear();
    escapedVars_.clear();

    std::cerr << "[Checker] Analyzing: " << filename << "\n";

    scanReservedNamespaces(root);

    pushScope();
    analyze(root);
    popScope();

    finalizeRcSkipCandidates();

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
