// =============================================================
//  BLUSK main.cpp  -  AI 제거판 진입점
// =============================================================
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/checker.h"
#include "../include/compiler.h"
#include "../include/vm.h"
#include "../include/gc.h"
#include "../include/debug.h"
#include "../include/repl.h"
#include "../include/native.h"
#include "../include/jit.h"
#include "../include/meta.h"
#include "../include/error.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>

#define BLUSK_VERSION "0.7-alpha"

static void printVersion() {
    std::cout << "BLUSK " << BLUSK_VERSION << "\n"
              << "  VM      : SVM (Register VM, 256 regs)\n"
              << "  GC      : RC + Trial Deletion Cycle GC\n"
              << "  Checker : RC-skip / Dead-code / Borrow\n"
              << "  SIMD    : AVX2 Matrix/Tensor\n"
              << "  Native  : LLVM AOT (1.0 target)\n"
              << "  AI      : Reserved (custom engine)\n"
              << "  Self-hosting target: v1.5\n"
              << "  Platforms: x86_64 / ARM64 / x32 | Win/Linux/macOS\n";
}

static void printUsage() {
    std::cerr << "Usage:\n"
              << "  blusk <file.blusk>        Run a BLUSK source file\n"
              << "  blusk --repl              Interactive REPL\n"
              << "  blusk --dis <file.blusk>  Disassemble bytecode\n"
              << "  blusk --debug <file>      Run with debug/GC/JIT output\n"
              << "  blusk --version           Version info\n"
              << "  blusk --meta              Show bluskmeta.json\n";
}

// ──────────────────────────────────────────────────────────────────
//  소스 → 바이트코드 파이프라인
// ──────────────────────────────────────────────────────────────────
static bool pipeline(const std::string& src,
                     const std::string& filename,
                     std::vector<Instruction>& outCode,
                     std::unordered_map<std::string, ASTNode*>& outClasses,
                     ASTNode*& outRoot,
                     bool verbose = false)
{
    // 1. Lex
    Lexer lex(src);
    std::vector<Token> tokens;
    while (true) {
        Token t = lex.nextToken();
        tokens.push_back(t);
        if (t.type == TOKEN_EOF) break;
    }

    // 2. Parse
    Parser parser(tokens);
    outRoot = parser.parse();
    if (!outRoot) { std::cerr << "[BLUSK] Parse failed.\n"; return false; }

    // 3. Checker (컴파일 전 정적 분석)
    BluskChecker checker;
    CheckResult cr = checker.check(outRoot, filename);
    if (cr.hasError) {
        std::cerr << "[BLUSK] Checker: " << cr.errorCount << " error(s). Build stopped.\n";
        return false;
    }

    // 4. @native 함수 컴파일 (LLVM, 1.0 대상 — 지금은 stub)
    NativeCompiler::instance().compileAll();

    // 5. Compile → SVM 바이트코드
    Compiler compiler;
    compiler.setCheckerResult(cr);
    outCode    = compiler.compile(outRoot, filename);
    outClasses = compiler.getClassTable();

    if (outCode.empty()) {
        std::cerr << "[BLUSK] No bytecode generated.\n";
        return false;
    }
    if (verbose)
        std::cerr << "[BLUSK] Compiled " << outCode.size() << " instructions.\n";
    return true;
}

// ──────────────────────────────────────────────────────────────────
//  main
// ──────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    if (argc < 2) { printUsage(); return 1; }

    std::string arg1 = argv[1];

    // ── 버전 ───────────────────────────────────────────────────
    if (arg1 == "--version" || arg1 == "-v") { printVersion(); return 0; }

    // ── 메타 ───────────────────────────────────────────────────
    if (arg1 == "--meta") {
        BluskMeta m = MetaHandler::load("bluskmeta.json");
        MetaHandler::print(m);
        return 0;
    }

    // ── REPL ────────────────────────────────────────────────────
    if (arg1 == "--repl" || arg1 == "-i") {
        ReplSession repl;
        repl.run();
        return 0;
    }

    // ── 디스어셈블 ──────────────────────────────────────────────
    if (arg1 == "--dis" && argc >= 3) {
        std::string fn = argv[2];
        std::ifstream f(fn);
        if (!f.is_open()) { BluskError::fatal("Cannot open: " + fn); return 1; }
        std::ostringstream ss; ss << f.rdbuf();
        std::vector<Instruction> code;
        std::unordered_map<std::string, ASTNode*> cls;
        ASTNode* root = nullptr;
        if (!pipeline(ss.str(), fn, code, cls, root)) { delete root; return 1; }
        std::cout << BluskDebugger::disassemble(code);
        delete root;
        return 0;
    }

    // ── 파일 실행 (+ 선택적 debug) ──────────────────────────────
    bool debugMode = false;
    std::string filename = arg1;
    if (arg1 == "--debug" && argc >= 3) {
        debugMode = true;
        filename  = argv[2];
        BluskDebugger::instance().enabled = true;
    }

    std::ifstream file(filename);
    if (!file.is_open()) { BluskError::fatal("Cannot open: " + filename); return 1; }
    std::ostringstream ss; ss << file.rdbuf();

    // bluskmeta.json 로드 (없으면 기본값)
    BluskMeta meta = MetaHandler::load("bluskmeta.json");
    if (debugMode) MetaHandler::print(meta);

    std::cerr << "[BLUSK] " << BLUSK_VERSION << " | " << filename << "\n";

    // JIT 설정
    JITConfig jcfg;
    jcfg.enabled = true;
    jcfg.verbose = debugMode;
    JITCompiler::instance().configure(jcfg);

    // 파이프라인
    std::vector<Instruction> bytecode;
    std::unordered_map<std::string, ASTNode*> classTable;
    ASTNode* root = nullptr;

    if (!pipeline(ss.str(), filename, bytecode, classTable, root, true)) {
        delete root;
        return 1;
    }

    std::cerr << "[BLUSK] Running...\n\n";

    // SVM 실행
    SVM svm;
    svm.setFilename(filename);
    for (auto& [name, node] : classTable)
        svm.registerClass(node);
    svm.run(bytecode);

    std::cerr << "\n[BLUSK] Done.\n";

    // 디버그 통계
    if (debugMode) {
        BluskGC::instance().printStats();
        JITCompiler::instance().printStats();
        BluskDebugger::instance().printStackTrace();
    }

    // 정리
    BluskGC::instance().collectCycles();
    BluskGC::instance().shutdown();
    delete root;
    return 0;
}
