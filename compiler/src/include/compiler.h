// =============================================================
//  BLUSK compiler.h
//  Includes: constTable (zero-cost num inlining), varTypes-based
//  auto-cast on assignment (keeps int/long/float/double widths
//  correct through arithmetic promotion), and LoopContext stack
//  for compiling break/continue directly to OP_JUMP (no runtime
//  "find the nearest JUMP" guessing, which breaks inside nested
//  if/switch blocks).
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
// being compiled. continueTarget is the PC where the loop's
// increment/condition-recheck step begins -- known immediately since
// nothing else is emitted between the body and the step. breakPatches
// holds the bytecode indices of JUMP instructions emitted for "break"
// statements; these get patched to point past the loop once its end PC
// is known, after the body finishes compiling.
struct LoopContext {
    size_t continueTarget = 0;
    std::vector<size_t> breakPatches;
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
