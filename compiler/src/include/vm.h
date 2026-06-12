// =============================================================
//  BLUSK vm.h  -  SVM (AI 기능 제거판)
//  llama.cpp 제거 / Saturday AI 보류
// =============================================================
#pragma once
#include "opcode.h"
#include "value.h"
#include "ast.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <array>
#include <memory>

#if __has_include("httplib.h")
#define SATURDAY_HTTP
#include "httplib.h"
#endif

constexpr size_t REG_COUNT = 256;

struct CallFrame {
    std::array<Value, REG_COUNT> regs{};
    size_t      returnPC   = 0;
    std::string objectName;
};

// ------------------------------------------------------------------
//  SVM : BLUSK Register Virtual Machine
// ------------------------------------------------------------------
class SVM {
    std::array<Value, REG_COUNT> regs{};
    std::unordered_map<std::string, Value>                        globals;
    std::unordered_map<std::string, bool>                         constants;
    std::vector<CallFrame>                                        callStack;
    std::unordered_map<std::string, std::shared_ptr<BluskObject>> objects;
    std::unordered_map<std::string, ASTNode*>                     classes;
    std::unordered_map<std::string, std::vector<Value>>           arrays;
    std::string filename = "unknown";

public:
    void setFilename(const std::string& f) { filename = f; }
    void registerClass(ASTNode* classNode);
    void run(const std::vector<Instruction>& bytecode);

private:
    Value& reg(uint8_t i) { return regs[i]; }

    std::string getParentClass(const std::string& cn);
    ASTNode*    findMethod(const std::string& cn, const std::string& mn);
    void runConstructor(const std::string& cn, BluskObject& obj,
                        const std::vector<Value>& args);
    void runBlock(ASTNode* body, BluskObject& obj,
                  std::unordered_map<std::string, Value>& local);
    void callMethod(const std::string& on, const std::string& mn,
                    const std::vector<Value>& args, uint8_t dstReg);
    std::string resolveFmtString(const std::string& tmpl);
};
