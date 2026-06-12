// =============================================================
//  BLUSK native.h  -  @native LLVM AOT 컴파일러
//
//  @native 어노테이션이 붙은 함수를
//  LLVM IR → 네이티브 기계어로 AOT 컴파일한다.
//
//  0.7 alpha: LLVM 연결 stub + 인터페이스 확정
//  1.0 target: 실제 LLVM IR emit
// =============================================================
#pragma once
#include "ast.h"
#include "opcode.h"
#include "value.h"
#include <string>
#include <vector>
#include <unordered_set>
#include <functional>

// ------------------------------------------------------------------
//  NativeTarget : 컴파일 대상 아키텍처
// ------------------------------------------------------------------
enum class NativeTarget {
    X86_64,   // Windows/Linux/macOS x86-64
    X86_32,   // Windows x32
    ARM64,    // macOS Apple Silicon, Linux ARM64
    AUTO,     // 현재 플랫폼 자동 감지
};

// ------------------------------------------------------------------
//  NativeCompileResult
// ------------------------------------------------------------------
struct NativeCompileResult {
    bool        success = false;
    std::string errorMsg;
    std::string irDump;     // LLVM IR (디버그용)
    void*       fnPtr = nullptr;  // JIT 로드 시 함수 포인터
};

// ------------------------------------------------------------------
//  NativeCompiler
//
//  현재 상태 (0.7 alpha):
//    - @native 함수 목록 수집
//    - LLVM 없으면 stub 경고 출력
//    - 인터페이스 확정 (1.0에서 실제 emit)
//
//  LLVM_ENABLED 정의 시:
//    - 실제 LLVM IR 생성 및 JIT 로드
// ------------------------------------------------------------------
class NativeCompiler {
public:
    static NativeCompiler& instance() {
        static NativeCompiler nc;
        return nc;
    }

    // 타겟 설정
    void setTarget(NativeTarget t);
    NativeTarget target() const { return target_; }

    // @native 함수 등록 (컴파일러 패스에서 호출)
    void registerNativeFunc(const std::string& name, ASTNode* funcNode);

    // 등록된 @native 함수 목록
    const std::unordered_set<std::string>& nativeFuncs() const { return nativeFuncNames_; }

    // AOT 컴파일 (등록된 함수 전체)
    bool compileAll();

    // 단일 함수 컴파일
    NativeCompileResult compileFunction(const std::string& name, ASTNode* node);

    // 함수 포인터 조회 (컴파일 성공 시)
    void* getFuncPtr(const std::string& name) const;

    // LLVM 가용 여부
    static bool isLLVMAvailable();

    // 타겟 문자열 (triple)
    static std::string targetTriple(NativeTarget t);

private:
    NativeCompiler();
    ~NativeCompiler();
    NativeCompiler(const NativeCompiler&) = delete;

    NativeTarget target_ = NativeTarget::AUTO;
    std::unordered_set<std::string> nativeFuncNames_;
    std::unordered_map<std::string, ASTNode*> nativeNodes_;
    std::unordered_map<std::string, void*>    funcPtrs_;

#if defined(LLVM_ENABLED)
    // LLVM context, module, JIT (1.0에서 채움)
    void* llvmCtx_    = nullptr;
    void* llvmModule_ = nullptr;
    void* jitEngine_  = nullptr;
#endif

    // AST → LLVM IR 변환 (내부, 1.0에서 구현)
    std::string emitIR(const std::string& funcName, ASTNode* node);
};
