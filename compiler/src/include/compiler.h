// =============================================================
//  BLUSK compiler.h
//  Includes: constTable (zero-cost num inlining), varTypes-based
//  auto-cast on assignment (keeps int/long/float/double widths
//  correct through arithmetic promotion), and LoopContext stack
//  for compiling break/continue directly to OP_JUMP via deferred
//  patching (fixes infinite loops when continue/break appear
//  inside nested if/switch blocks).
// =============================================================
#pragma once
#include "ast.h"
#include "opcode.h"
#include "checker.h"
#include "value.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>
#include <sstream>

class RegAllocator {
    uint8_t next = 0;
public:
    uint8_t alloc()            { return next++; }
    uint8_t current()    const { return next; }
    void    resetTo(uint8_t m) { next = m; }
    void    reset()            { next = 0; }
};

// Tracks the patch points for break/continue inside the loop currently
// being compiled. Both breakPatches and continuePatches are *deferred*
// patch lists: the JUMP target isn't known yet at the point a break/continue
// statement is compiled (it's compiled while walking the loop body, before
// the increment step or the loop's exit point even exist in the bytecode
// yet), so each JUMP is emitted with a placeholder target and its index is
// recorded here. Once the real location is known -- the increment step's
// start PC for continuePatches, the address just past the loop for
// breakPatches -- every recorded JUMP gets patched to point there.
struct LoopContext {
    std::vector<size_t> breakPatches;
    std::vector<size_t> continuePatches;
};

class Compiler {
public:
    std::vector<Instruction> compile(ASTNode* root,
                                     const std::string& filename = "unknown");
    void setCheckerResult(const CheckResult& cr) { checkerResult = cr; }
    std::unordered_map<std::string, ASTNode*>& getClassTable();

private:
    std::vector<Instruction>                  bytecode;
    std::string                               filename;
    std::unordered_map<std::string, ASTNode*> classTable;
    RegAllocator                              ra;
    CheckResult                               checkerResult;
    std::vector<LoopContext>                  loopStack;

    // zero-cost: num constant inlining table
    std::unordered_map<std::string, Value>    constTable;

    bool hasBlusk26   = false;
    bool hasIO        = false;
    bool hasOOP       = false;
    bool hasTime      = false;
    bool hasAI        = false;
    bool hasBluskMath = false;
    bool hasString    = false;
    bool hasColl      = false;

    bool isDead(const std::string& name) const {
        return checkerResult.deadVars.count(name) > 0;
    }
    int64_t storeFlag(const std::string& name, bool isConst) const {
        bool rc = checkerResult.rcSkipVars.count(name) > 0;
        if (isConst && rc) return STORE_CONST_RC;
        if (isConst)        return STORE_CONST;
        if (rc)             return STORE_RCSKIP;
        return STORE_NORMAL;
    }

    // Declared type keyword for a variable, as recorded by the Checker
    // ("int"/"long"/"float"/"double"/"gg"/"num"/...). Empty if unknown.
    std::string declaredTypeOf(const std::string& name) const {
        auto it = checkerResult.varTypes.find(name);
        return it != checkerResult.varTypes.end() ? it->second : "";
    }
    // Whether re-assigning to this variable needs an automatic cast back
    // to its declared width. Without this, "int x = x + 1;" silently
    // widens to long through arithmetic promotion (the literal 1 has no
    // type tag, so it defaults to long, and int+long promotes to long),
    // so the 32-bit wraparound never happens. Not needed for "gg"/"num",
    // whose width is inferred fresh from whatever the literal/expression
    // produces.
    static bool needsCastOnAssign(const std::string& declaredType) {
        return declaredType=="int" || declaredType=="long" ||
               declaredType=="float" || declaredType=="double";
    }

    void    emit(OpCode op, uint8_t dst=0, uint8_t src1=0, uint8_t src2=0,
                 const std::string& str="", int64_t iv=0, double fv=0.0);

    // zero-cost constant inlining attempt
    bool    tryInlineConst(const std::string& name, uint8_t dst);

    void    compileNode(ASTNode* node);
    uint8_t compileExpr(ASTNode* node);
    uint8_t compileCond(ASTNode* cond);
    uint8_t compileFString(const std::string& tmpl);
    uint8_t loadValue(const std::string& raw, int line = 0);

    static bool isNumLit(const std::string& s);
};
