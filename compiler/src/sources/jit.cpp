// =============================================================
//  BLUSK jit.cpp
// =============================================================
#include "../include/jit.h"
#include "../include/native.h"
#include "../include/error.h"
#include <iostream>
#include <algorithm>

// ──────────────────────────────────────────────────────────────────
//  Back-edge 감지 (루프 OP_JUMP가 뒤로 점프할 때)
// ──────────────────────────────────────────────────────────────────
void JITCompiler::onBackEdge(size_t pc) {
    if (!config_.enabled) return;

    auto& cnt = backEdgeCount_[pc];
    cnt++;

    if (cnt == config_.hotThreshold && !hotSpots_.count(pc)) {
        HotSpot spot;
        spot.startPC  = pc;
        spot.endPC    = pc + config_.traceLen;
        spot.hitCount = cnt;
        hotSpots_[pc] = spot;

        if (config_.verbose)
            std::cerr << "[JIT] Hot loop detected at pc=" << pc
                      << " (hits=" << cnt << ")\n";

        // 0.7: 감지만 하고 컴파일은 stub
        // 1.0: doCompile() 호출
    }

    if (hotSpots_.count(pc))
        hotSpots_[pc].hitCount = cnt;
}

// ──────────────────────────────────────────────────────────────────
//  함수 호출 카운트
// ──────────────────────────────────────────────────────────────────
void JITCompiler::onFuncCall(const std::string& name) {
    if (!config_.enabled) return;

    auto& cnt = funcCallCount_[name];
    cnt++;

    if (cnt == config_.hotThreshold && config_.verbose)
        std::cerr << "[JIT] Hot function: " << name
                  << " (calls=" << cnt << ") — JIT promotion candidate\n";
}

// ──────────────────────────────────────────────────────────────────
//  hot 여부
// ──────────────────────────────────────────────────────────────────
bool JITCompiler::isHot(size_t pc) const {
    auto it = backEdgeCount_.find(pc);
    return it != backEdgeCount_.end() && it->second >= config_.hotThreshold;
}

bool JITCompiler::isFuncHot(const std::string& name) const {
    auto it = funcCallCount_.find(name);
    return it != funcCallCount_.end() && it->second >= config_.hotThreshold;
}

// ──────────────────────────────────────────────────────────────────
//  JIT 컴파일 시도
// ──────────────────────────────────────────────────────────────────
bool JITCompiler::tryCompile(size_t startPC, size_t endPC,
                              const std::vector<Instruction>& bytecode)
{
    if (!config_.enabled) return false;
    if (!isHot(startPC)) return false;

    auto& spot = hotSpots_[startPC];
    if (spot.compiled) return true;

    spot.endPC = std::min(endPC, bytecode.size());
    return doCompile(spot, bytecode);
}

bool JITCompiler::doCompile(HotSpot& spot,
                             const std::vector<Instruction>& code)
{
#if defined(LLVM_ENABLED)
    // TODO (1.0): bytecode[startPC..endPC] → LLVM IR → 네이티브 코드
    // spot.codePtr = ...;
    // spot.compiled = true;
    spot.compiled = false;
    return false; // 아직 미구현
#else
    if (config_.verbose)
        std::cerr << "[JIT] Compilation skipped (LLVM not available). "
                  << "SVM continues.\n";
    spot.compiled = false;
    return false;
#endif
}

// ──────────────────────────────────────────────────────────────────
//  컴파일된 코드 실행
// ──────────────────────────────────────────────────────────────────
bool JITCompiler::executeIfCompiled(size_t pc) {
    auto it = hotSpots_.find(pc);
    if (it == hotSpots_.end() || !it->second.compiled) return false;
    if (!it->second.codePtr) return false;

    // 1.0: 네이티브 함수 포인터 호출
    // using JitFn = void(*)();
    // ((JitFn)it->second.codePtr)();
    return false; // placeholder
}

// ──────────────────────────────────────────────────────────────────
//  통계
// ──────────────────────────────────────────────────────────────────
void JITCompiler::printStats() const {
    std::cerr << "\n[BLUSK JIT Stats]\n"
              << "  Enabled       : " << (config_.enabled ? "yes" : "no") << "\n"
              << "  Hot threshold : " << config_.hotThreshold << "\n"
              << "  Back-edges    : " << backEdgeCount_.size() << " tracked\n"
              << "  Hot funcs     : " << funcCallCount_.size() << " tracked\n"
              << "  Hot spots     : " << hotSpots_.size() << "\n";

    int compiled = 0;
    for (auto& [_, hs] : hotSpots_) if (hs.compiled) compiled++;
    std::cerr << "  JIT compiled  : " << compiled << "\n";

    // 가장 많이 호출된 함수 top 5
    std::vector<std::pair<std::string,uint32_t>> sorted(
        funcCallCount_.begin(), funcCallCount_.end());
    std::sort(sorted.begin(), sorted.end(),
              [](auto& a, auto& b){ return a.second > b.second; });
    if (!sorted.empty()) {
        std::cerr << "  Top functions :\n";
        for (size_t i = 0; i < std::min(sorted.size(), (size_t)5); i++)
            std::cerr << "    " << sorted[i].first
                      << " (" << sorted[i].second << " calls)\n";
    }
    std::cerr << "\n";
}

void JITCompiler::reset() {
    backEdgeCount_.clear();
    funcCallCount_.clear();
    hotSpots_.clear();
}
