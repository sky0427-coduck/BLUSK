// =============================================================
//  BLUSK compiler.h  -  constTable, tryInlineConst 추가
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

    // ── zero-cost: num 상수 인라이닝 테이블 ─────────────────────
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

    void    emit(OpCode op, uint8_t dst=0, uint8_t src1=0, uint8_t src2=0,
                 const std::string& str="", int64_t iv=0, double fv=0.0);

    // zero-cost 상수 인라이닝 시도
    bool    tryInlineConst(const std::string& name, uint8_t dst);

    void    compileNode(ASTNode* node);
    uint8_t compileExpr(ASTNode* node);
    uint8_t compileCond(ASTNode* cond);
    uint8_t compileFString(const std::string& tmpl);
    uint8_t loadValue(const std::string& raw, int line = 0);

    static bool isNumLit(const std::string& s);
};
