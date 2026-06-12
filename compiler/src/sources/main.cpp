// =============================================================
//  BLUSK main.cpp  -  진입점 (0.7 alpha 완전판)
//
//  실행 모드:
//    blusk <file.blusk>        — 파일 실행
//    blusk --repl              — 대화형 모드
//    blusk --dis <file.blusk>  — 바이트코드 디스어셈블
//    blusk --version           — 버전 출력
//    blusk --meta              — bluskmeta.json 출력
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
#include "../include/module.h"
#include "../include/error.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <string>

#define BLUSK_VERSION "0.7-alpha"

static void printVersion() {
    std::cout << "BLUSK " << BLUSK_VERSION << "\n"
              << "  VM       : SVM (Register VM, 256 regs)\n"
              << "  GC       : RC + Trial Deletion Cycle GC\n"
              << "  Checker  : Static RC-skip + Dead-code + Borrow\n"
              << "  Native   : LLVM AOT (stub, 1.0 target)\n"
              << "  JIT      : Hot-path detection (stub, 1.0 target)\n"
              << "  AI       : Saturday (llama.cpp MIT)\n"
              << "  Targets  : x86_64 / ARM64 / x32 | Win/Linux/macOS\n";
}

static void printUsage() {
    std::cerr << "Usage:\n"
              << "  blusk <file.blusk>        Run a BLUSK source file\n"
              << "  blusk --repl              Interactive REPL\n"
              << "  blusk --dis <file.blusk>  Disassemble bytecode\n"
              << "  blusk --version           Show version info\n"
              << "  blusk --meta              Show bluskmeta.json\n"
              << "  blusk --debug <file>      Run with debug output\n";
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

    // 3. Checker
    BluskChecker checker;
    CheckResult cr = checker.check(outRoot, filename);
    if (cr.hasError) {
        std::cerr << "[BLUSK] Build stopped: Checker found "
                  << cr.errorCount << " error(s).\n";
        return false;
    }

    // 4. @native 함수 등록
    NativeCompiler::instance().compileAll();

    // 5. Compile
    Compiler compiler;
    compiler.setCheckerResult(cr);
    outCode = compiler.compile(outRoot, filename);
    outClasses = compiler.getClassTable();

    if (outCode.empty()) { std::cerr << "[BLUSK] No bytecode generated.\n"; return false; }

    if (verbose)
        std::cerr << "[BLUSK] Compiled " << outCode.size() << " instructions.\n";

    return true;
}

// ──────────────────────────────────────────────────────────────────
//  main
// ──────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {

    // ── 인자 없음 → 사용법 ───────────────────────────────────────
    if (argc < 2) { printUsage(); return 1; }

    std::string arg1 = argv[1];

    // ── --version ────────────────────────────────────────────────
    if (arg1 == "--version" || arg1 == "-v") { printVersion(); return 0; }

    // ── --meta ───────────────────────────────────────────────────
    if (arg1 == "--meta") {
        BluskMeta meta = MetaHandler::load("bluskmeta.json");
        MetaHandler::print(meta);
        return 0;
    }

    // ── --repl ───────────────────────────────────────────────────
    if (arg1 == "--repl" || arg1 == "-i") {
        ReplSession repl;
        repl.run();
        return 0;
    }

    // ── --dis ────────────────────────────────────────────────────
    if (arg1 == "--dis" && argc >= 3) {
        std::string filename = argv[2];
        std::ifstream f(filename);
        if (!f.is_open()) { BluskError::fatal("Cannot open: " + filename); return 1; }
        std::ostringstream ss; ss << f.rdbuf();

        std::vector<Instruction> code;
        std::unordered_map<std::string, ASTNode*> classes;
        ASTNode* root = nullptr;
        if (!pipeline(ss.str(), filename, code, classes, root)) {
            delete root; return 1;
        }
        std::cout << BluskDebugger::disassemble(code);
        delete root;
        return 0;
    }

    // ── --debug ──────────────────────────────────────────────────
    bool debugMode = false;
    std::string filename = arg1;
    if (arg1 == "--debug" && argc >= 3) {
        debugMode = true;
        filename  = argv[2];
        BluskDebugger::instance().enabled = true;
    }

    // ── 파일 실행 ────────────────────────────────────────────────
    std::ifstream file(filename);
    if (!file.is_open()) {
        BluskError::fatal("Cannot open file: " + filename);
        return 1;
    }
    std::ostringstream ss; ss << file.rdbuf();

    // bluskmeta.json 로드 (있으면)
    BluskMeta meta = MetaHandler::load("bluskmeta.json");
    if (debugMode) MetaHandler::print(meta);

    std::cerr << "[BLUSK] " << BLUSK_VERSION
              << " | Compiling: " << filename << "\n";

    // JIT 설정
    JITConfig jitCfg;
    jitCfg.enabled   = true;
    jitCfg.verbose   = debugMode;
    JITCompiler::instance().configure(jitCfg);

    // 파이프라인
    std::vector<Instruction> bytecode;
    std::unordered_map<std::string, ASTNode*> classTable;
    ASTNode* root = nullptr;

    if (!pipeline(ss.str(), filename, bytecode, classTable, root, true)) {
        delete root; return 1;
    }

    std::cerr << "[BLUSK] Running...\n\n";

    // SVM 실행
    SVM svm;
    svm.setFilename(filename);
    for (auto& [name, node] : classTable)
        svm.registerClass(node);

    svm.run(bytecode);

    std::cerr << "\n[BLUSK] Done.\n";

    // 종료 정리
    if (debugMode) {
        BluskGC::instance().printStats();
        JITCompiler::instance().printStats();
        BluskDebugger::instance().printStackTrace();
    }

    BluskGC::instance().collectCycles();
    BluskGC::instance().shutdown();

    delete root;
    return 0;
}
