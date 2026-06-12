// =============================================================
//  BLUSK debug.cpp
// =============================================================
#include "../include/debug.h"
#include "../include/error.h"
#include <iostream>
#include <iomanip>
#include <sstream>

void BluskDebugger::registerInfo(size_t instrIdx, int line,
                                  const std::string& file,
                                  const std::string& func)
{
    DebugInfo di;
    di.instrIndex = instrIdx;
    di.srcLine    = line;
    di.srcFile    = file;
    di.funcName   = func;
    infoMap_[instrIdx] = di;
}

void BluskDebugger::pushCall(const std::string& func,
                              const std::string& file, int line, size_t pc)
{
    CallRecord r; r.funcName = func; r.file = file;
    r.line = line; r.instrIndex = pc;
    callStack_.push_back(r);
}

void BluskDebugger::popCall() {
    if (!callStack_.empty()) callStack_.pop_back();
}

void BluskDebugger::updatePC(size_t pc) { currentPC_ = pc; }

void BluskDebugger::printStackTrace() const {
    std::cerr << "\n[BLUSK] Stack trace (most recent call last):\n";
    for (int i = (int)callStack_.size()-1; i >= 0; i--) {
        const auto& r = callStack_[i];
        std::cerr << "  at " << r.funcName
                  << " (" << r.file << ":" << r.line << ")\n";
    }
    // 현재 PC 정보
    auto it = infoMap_.find(currentPC_);
    if (it != infoMap_.end()) {
        std::cerr << "  >> " << it->second.file
                  << ":" << it->second.srcLine
                  << " [pc=" << currentPC_ << "]\n";
    }
    std::cerr << "\n";
}

void BluskDebugger::onError(const std::string& msg, size_t pc) const {
    std::cerr << "\n[BLUSK Runtime Error] " << msg << "\n";
    auto it = infoMap_.find(pc);
    if (it != infoMap_.end())
        std::cerr << "  at " << it->second.srcFile
                  << ":" << it->second.srcLine << "\n";
    printStackTrace();
}

void BluskDebugger::dumpRegisters(const Value* regs, size_t count) const {
    std::cerr << "[BLUSK] Register dump:\n";
    for (size_t i = 0; i < count && i < 32; i++) {
        if (!regs[i].isNil())
            std::cerr << "  r" << std::setw(3) << i
                      << " = " << regs[i].toString() << "\n";
    }
}

// ── 디스어셈블러 ─────────────────────────────────────────────────
std::string BluskDebugger::instrToString(const Instruction& inst, size_t idx) {
    std::ostringstream oss;
    oss << std::setw(5) << idx << "  ";

    auto opName = [&]() -> std::string {
        switch(inst.op) {
        case OP_LOAD_INT:   return "LOAD_INT";
        case OP_LOAD_FLOAT: return "LOAD_FLOAT";
        case OP_LOAD_STR:   return "LOAD_STR";
        case OP_LOAD_BOOL:  return "LOAD_BOOL";
        case OP_LOAD_NIL:   return "LOAD_NIL";
        case OP_MOVE:       return "MOVE";
        case OP_STORE:      return "STORE";
        case OP_LOAD:       return "LOAD";
        case OP_ADD:        return "ADD";
        case OP_SUB:        return "SUB";
        case OP_MUL:        return "MUL";
        case OP_DIV:        return "DIV";
        case OP_MOD:        return "MOD";
        case OP_POW:        return "POW";
        case OP_NEG:        return "NEG";
        case OP_CMP_EQ:     return "CMP_EQ";
        case OP_CMP_NE:     return "CMP_NE";
        case OP_CMP_LT:     return "CMP_LT";
        case OP_CMP_LE:     return "CMP_LE";
        case OP_CMP_GT:     return "CMP_GT";
        case OP_CMP_GE:     return "CMP_GE";
        case OP_JUMP:       return "JUMP";
        case OP_JUMP_IF:    return "JUMP_IF";
        case OP_JUMP_IFNOT: return "JUMP_IFNOT";
        case OP_BREAK:      return "BREAK";
        case OP_CONTINUE:   return "CONTINUE";
        case OP_RETURN:     return "RETURN";
        case OP_PRINT:      return "PRINT";
        case OP_PRINT_FMT:  return "PRINT_FMT";
        case OP_READ:       return "READ";
        case OP_NEW:        return "NEW";
        case OP_CALL:       return "CALL";
        case OP_SET_FIELD:  return "SET_FIELD";
        case OP_GET_FIELD:  return "GET_FIELD";
        case OP_ARRAY_NEW:  return "ARRAY_NEW";
        case OP_ARRAY_GET:  return "ARRAY_GET";
        case OP_ARRAY_SET:  return "ARRAY_SET";
        case OP_SIZE:       return "SIZE";
        case OP_SLEEP:      return "SLEEP";
        case OP_PERF_NOW:   return "PERF_NOW";
        case OP_AI_LOAD:    return "AI_LOAD";
        case OP_AI_ASK:     return "AI_ASK";
        case OP_AI_LEARN:   return "AI_LEARN";
        case OP_AI_SAVE:    return "AI_SAVE";
        case OP_AI_STATUS:  return "AI_STATUS";
        case OP_HALT:       return "HALT";
        default:            return "OP_" + std::to_string(inst.op);
        }
    };

    oss << std::left << std::setw(14) << opName();
    oss << " r" << (int)inst.dst;

    if (inst.src1 != 0 || inst.src2 != 0)
        oss << ", r" << (int)inst.src1 << ", r" << (int)inst.src2;
    if (!inst.strVal.empty())
        oss << "  [\"" << inst.strVal << "\"]";
    if (inst.intVal != 0)
        oss << "  #" << inst.intVal;
    if (inst.floatVal != 0.0)
        oss << "  #" << inst.floatVal;

    return oss.str();
}

std::string BluskDebugger::disassemble(const std::vector<Instruction>& code,
                                        const std::vector<DebugInfo>& info)
{
    std::ostringstream oss;
    oss << "=== BLUSK Bytecode Disassembly ===\n";
    for (size_t i = 0; i < code.size(); i++) {
        if (i < info.size() && info[i].srcLine > 0)
            oss << "  ; line " << info[i].srcLine << "\n";
        oss << instrToString(code[i], i) << "\n";
    }
    return oss.str();
}

const DebugInfo* BluskDebugger::getInfo(size_t instrIdx) const {
    auto it = infoMap_.find(instrIdx);
    return it != infoMap_.end() ? &it->second : nullptr;
}
