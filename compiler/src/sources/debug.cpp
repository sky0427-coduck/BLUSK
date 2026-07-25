// =============================================================
//  BLUSK debug.cpp  -  disassembler now shows src1/src2 (was dst-only)
// =============================================================
#include "../include/debug.h"
#include "../include/error.h"
#include <iostream>
#include <iomanip>
#include <sstream>

void BluskDebugger::registerInfo(size_t instrIdx, int line, const std::string& file, const std::string& func) {
    DebugInfo di; di.instrIndex=instrIdx; di.srcLine=line; di.srcFile=file; di.funcName=func;
    infoMap_[instrIdx]=di;
}
void BluskDebugger::pushCall(const std::string& func, const std::string& file, int line, size_t pc) {
    CallRecord r; r.funcName=func; r.file=file; r.line=line; r.instrIndex=pc; callStack_.push_back(r);
}
void BluskDebugger::popCall() { if(!callStack_.empty()) callStack_.pop_back(); }
void BluskDebugger::updatePC(size_t pc) { currentPC_=pc; }
void BluskDebugger::printStackTrace() const {
    std::cerr << "\n[BLUSK] Stack trace:\n";
    for (int i=(int)callStack_.size()-1;i>=0;i--) { auto& r=callStack_[i]; std::cerr<<"  at "<<r.funcName<<" ("<<r.file<<":"<<r.line<<")\n"; }
}
void BluskDebugger::onError(const std::string& msg, size_t pc) const {
    std::cerr << "\n[BLUSK Runtime Error] " << msg << "\n";
    printStackTrace();
}
void BluskDebugger::dumpRegisters(const Value* regs, size_t count) const {
    for (size_t i=0;i<count&&i<32;i++) if(!regs[i].isNil()) std::cerr<<"  r"<<i<<" = "<<regs[i].toString()<<"\n";
}

// instrToString now prints dst/src1/src2 explicitly. The previous version
// only showed dst, which made it impossible to see which registers an
// instruction actually reads from -- critical for debugging register
// allocation bugs (e.g. OP_MATRIX_NEW reading from the wrong base register).
std::string BluskDebugger::instrToString(const Instruction& inst, size_t idx) {
    std::ostringstream oss;
    oss<<std::setw(5)<<idx<<"  op="<<(int)inst.op
       <<" dst=r"<<(int)inst.dst<<" src1=r"<<(int)inst.src1<<" src2=r"<<(int)inst.src2;
    if(!inst.strVal.empty()) oss<<" [\""<<inst.strVal<<"\"]";
    if(inst.intVal!=0) oss<<" #"<<inst.intVal;
    if(inst.floatVal!=0.0) oss<<" f"<<inst.floatVal;
    return oss.str();
}
std::string BluskDebugger::disassemble(const std::vector<Instruction>& code, const std::vector<DebugInfo>& info) {
    std::ostringstream oss; oss<<"=== BLUSK Bytecode ===\n";
    for(size_t i=0;i<code.size();i++) oss<<instrToString(code[i],i)<<"\n";
    return oss.str();
}
const DebugInfo* BluskDebugger::getInfo(size_t instrIdx) const { auto it=infoMap_.find(instrIdx); return it!=infoMap_.end()?&it->second:nullptr; }
