// =============================================================
//  BLUSK native.cpp  -  @native 컴파일러 구현 (0.7 alpha stub)
// =============================================================
#include "../include/native.h"
#include "../include/error.h"
#include <iostream>

NativeCompiler::NativeCompiler() {
    // 타겟 자동 감지
#if defined(__aarch64__) || defined(_M_ARM64)
    target_ = NativeTarget::ARM64;
#elif defined(__x86_64__) || defined(_M_X64)
    target_ = NativeTarget::X86_64;
#elif defined(__i386__) || defined(_M_IX86)
    target_ = NativeTarget::X86_32;
#else
    target_ = NativeTarget::X86_64;
#endif
}

NativeCompiler::~NativeCompiler() {}

void NativeCompiler::setTarget(NativeTarget t) { target_ = t; }

void NativeCompiler::registerNativeFunc(const std::string& name, ASTNode* node) {
    nativeFuncNames_.insert(name);
    nativeNodes_[name] = node;
    std::cerr << "[Native] Registered @native function: " << name << "\n";
}

bool NativeCompiler::compileAll() {
    if (nativeFuncNames_.empty()) return true;

#if defined(LLVM_ENABLED)
    std::cerr << "[Native] Compiling " << nativeFuncNames_.size()
              << " @native functions for " << targetTriple(target_) << "\n";
    bool ok = true;
    for (const auto& name : nativeFuncNames_) {
        auto res = compileFunction(name, nativeNodes_[name]);
        if (!res.success) {
            BluskError::report("@native compile failed: " + res.errorMsg, name, 0);
            ok = false;
        }
    }
    return ok;
#else
    // LLVM 없음 → SVM fallback 경고
    std::cerr << "[Native] WARNING: LLVM not available. "
              << "@native functions will run on SVM (slower).\n"
              << "         Build with LLVM_ENABLED for native compilation.\n";
    for (const auto& name : nativeFuncNames_)
        std::cerr << "         - " << name << " -> SVM fallback\n";
    return true; // 에러는 아님, 경고만
#endif
}

NativeCompileResult NativeCompiler::compileFunction(const std::string& name,
                                                     ASTNode* node) {
    NativeCompileResult result;
#if defined(LLVM_ENABLED)
    // TODO (1.0): AST → LLVM IR → 네이티브 코드 생성
    result.irDump = emitIR(name, node);
    // 실제 JIT 로드 및 fnPtr 설정
    result.success = false; // placeholder
    result.errorMsg = "LLVM IR emit not yet implemented (planned for 1.0)";
#else
    result.success = true; // stub: 성공처럼 처리 (SVM fallback)
    result.errorMsg = "";
#endif
    return result;
}

void* NativeCompiler::getFuncPtr(const std::string& name) const {
    auto it = funcPtrs_.find(name);
    return it != funcPtrs_.end() ? it->second : nullptr;
}

bool NativeCompiler::isLLVMAvailable() {
#if defined(LLVM_ENABLED)
    return true;
#else
    return false;
#endif
}

std::string NativeCompiler::targetTriple(NativeTarget t) {
    switch (t) {
    case NativeTarget::X86_64: return "x86_64-unknown";
    case NativeTarget::X86_32: return "i686-unknown";
    case NativeTarget::ARM64:  return "aarch64-unknown";
    default:                   return "x86_64-unknown";
    }
}

std::string NativeCompiler::emitIR(const std::string& funcName, ASTNode* node) {
    // TODO (1.0): AST를 순회하며 LLVM IR 텍스트 생성
    // 현재는 placeholder
    return "; BLUSK LLVM IR for " + funcName + "\n"
           "; (IR generation planned for 1.0)\n";
}
