// =============================================================
//  BLUSK debug.h  -  스택 트레이스 + 디버그 정보
// =============================================================
#pragma once
#include "opcode.h"
#include "value.h"
#include <string>
#include <vector>
#include <unordered_map>

// ------------------------------------------------------------------
//  DebugInfo : 바이트코드 위치 → 소스 위치 매핑
// ------------------------------------------------------------------
struct DebugInfo {
    size_t      instrIndex = 0;  // bytecode 인덱스
    int         srcLine    = 0;  // 소스 줄 번호
    std::string srcFile;         // 소스 파일명
    std::string funcName;        // 현재 함수/메서드 이름
};

// ------------------------------------------------------------------
//  CallRecord : 콜 스택 한 프레임
// ------------------------------------------------------------------
struct CallRecord {
    std::string funcName;
    std::string file;
    int         line = 0;
    size_t      instrIndex = 0;
};

// ------------------------------------------------------------------
//  BluskDebugger
// ------------------------------------------------------------------
class BluskDebugger {
public:
    static BluskDebugger& instance() {
        static BluskDebugger d;
        return d;
    }

    // 활성화 여부
    bool enabled = false;

    // ── DebugInfo 등록 (컴파일 시) ───────────────────────────────
    void registerInfo(size_t instrIdx, int line,
                      const std::string& file, const std::string& func = "");

    // ── 콜 스택 관리 (런타임) ────────────────────────────────────
    void pushCall(const std::string& func, const std::string& file, int line, size_t pc);
    void popCall();

    // ── 현재 실행 위치 업데이트 ──────────────────────────────────
    void updatePC(size_t pc);

    // ── 스택 트레이스 출력 ───────────────────────────────────────
    void printStackTrace() const;

    // ── 런타임 에러 발생 시 자동 트레이스 ───────────────────────
    void onError(const std::string& msg, size_t pc) const;

    // ── 변수 덤프 (디버그 모드) ──────────────────────────────────
    void dumpRegisters(const Value* regs, size_t count) const;

    // ── 바이트코드 디스어셈블러 ──────────────────────────────────
    static std::string disassemble(const std::vector<Instruction>& code,
                                   const std::vector<DebugInfo>& info = {});
    static std::string instrToString(const Instruction& inst, size_t idx);

    // ── 소스 위치 조회 ────────────────────────────────────────────
    const DebugInfo* getInfo(size_t instrIdx) const;

private:
    BluskDebugger() = default;
    std::unordered_map<size_t, DebugInfo> infoMap_;
    std::vector<CallRecord> callStack_;
    size_t currentPC_ = 0;
};

// 편의 매크로
#define DBG_PUSH(fn, file, line, pc) \
    do { if(BluskDebugger::instance().enabled) \
         BluskDebugger::instance().pushCall(fn,file,line,pc); } while(0)
#define DBG_POP() \
    do { if(BluskDebugger::instance().enabled) \
         BluskDebugger::instance().popCall(); } while(0)
#define DBG_PC(pc) \
    do { if(BluskDebugger::instance().enabled) \
         BluskDebugger::instance().updatePC(pc); } while(0)
