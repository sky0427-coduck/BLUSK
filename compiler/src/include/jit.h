// =============================================================
//  BLUSK jit.h  -  JIT 컴파일러 (hot path 감지 + 최적화)
//
//  설계:
//    Tier 0 : SVM 인터프리터 (기본)
//    Tier 1 : hot path 감지 (호출 횟수 > threshold)
//    Tier 2 : @native AOT 또는 LLVM JIT로 승격
//
//  0.7 alpha: hot path 감지 + 카운터 시스템
//  1.0 target: 실제 JIT 코드 생성
// =============================================================
#pragma once
#include "opcode.h"
#include "value.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

// ------------------------------------------------------------------
//  JIT 설정
// ------------------------------------------------------------------
struct JITConfig {
    uint32_t hotThreshold  = 1000;  // 이 횟수 이상 호출되면 hot
    uint32_t traceLen      = 64;    // 트레이스 최대 명령어 수
    bool     enabled       = true;
    bool     verbose       = false; // 승격 시 로그 출력
};

// ------------------------------------------------------------------
//  HotSpot : hot path 정보
// ------------------------------------------------------------------
struct HotSpot {
    size_t   startPC   = 0;
    size_t   endPC     = 0;
    uint32_t hitCount  = 0;
    bool     compiled  = false; // JIT 완료 여부
    void*    codePtr   = nullptr; // 네이티브 코드 포인터 (1.0)
};

// ------------------------------------------------------------------
//  JITCompiler
// ------------------------------------------------------------------
class JITCompiler {
public:
    static JITCompiler& instance() {
        static JITCompiler jit;
        return jit;
    }

    // 설정
    void configure(const JITConfig& cfg) { config_ = cfg; }
    const JITConfig& config() const { return config_; }

    // ── 호출 카운터 (SVM 루프에서 매 JUMP마다 체크) ──────────────
    void onBackEdge(size_t pc);      // 루프 back-edge 감지
    void onFuncCall(const std::string& name); // 함수 호출 카운트

    // ── hot 여부 조회 ─────────────────────────────────────────────
    bool isHot(size_t pc) const;
    bool isFuncHot(const std::string& name) const;

    // ── JIT 컴파일 시도 ──────────────────────────────────────────
    bool tryCompile(size_t startPC, size_t endPC,
                    const std::vector<Instruction>& bytecode);

    // ── 컴파일된 코드 실행 (있으면 true 반환, 없으면 false) ──────
    bool executeIfCompiled(size_t pc);

    // ── hot spot 목록 ────────────────────────────────────────────
    const std::unordered_map<size_t, HotSpot>& hotSpots() const { return hotSpots_; }

    // ── 통계 출력 ─────────────────────────────────────────────────
    void printStats() const;

    // ── 초기화 ───────────────────────────────────────────────────
    void reset();

private:
    JITCompiler() = default;
    JITConfig config_;

    // PC → 호출 카운터
    std::unordered_map<size_t, uint32_t>   backEdgeCount_;
    std::unordered_map<std::string, uint32_t> funcCallCount_;

    // 확정된 hot spot
    std::unordered_map<size_t, HotSpot> hotSpots_;

    // ── 내부 컴파일 (1.0에서 LLVM 연결) ─────────────────────────
    bool doCompile(HotSpot& spot, const std::vector<Instruction>& code);
};
