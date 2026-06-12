// =============================================================
//  BLUSK repl.cpp
// =============================================================
#include "../include/repl.h"
#include "../include/debug.h"
#include "../include/error.h"
#include <iostream>
#include <sstream>
#include <algorithm>

ReplSession::ReplSession() {
    std::cerr << "\n  BLUSK " << "0.7-alpha" << "  REPL\n";
    std::cerr << "  Type :help for commands, :quit to exit\n\n";
}

void ReplSession::run() {
    running_ = true;
    while (running_) {
        prompt();
        std::string line;
        if (!std::getline(std::cin, line)) break; // EOF

        // REPL 명령어
        if (!line.empty() && line[0] == ':') {
            if (!handleCommand(line)) continue;
            continue;
        }

        // 멀티라인 누적
        multilineBuffer_ += line + "\n";
        for (char c : line) {
            if (c == '{') braceDepth_++;
            if (c == '}') braceDepth_--;
        }

        if (braceDepth_ > 0) {
            // 아직 블록이 열려있음
            promptCont();
            continue;
        }

        // 완성된 입력 실행
        std::string src = multilineBuffer_;
        multilineBuffer_.clear();
        braceDepth_ = 0;

        if (!src.empty() && src != "\n") evalSource(src);
    }
}

bool ReplSession::evalLine(const std::string& line) {
    return evalSource(wrapSource(line));
}

// ──────────────────────────────────────────────────────────────────
//  소스 → 실행
// ──────────────────────────────────────────────────────────────────
bool ReplSession::evalSource(const std::string& src) {
    // REPL 전용 래퍼: import blusk26; import io; @entry 자동 추가
    std::string wrapped = wrapSource(src);

    // 1. Lex
    Lexer lex(wrapped);
    std::vector<Token> tokens;
    while (true) {
        Token t = lex.nextToken();
        tokens.push_back(t);
        if (t.type == TOKEN_EOF) break;
    }

    // 2. Parse
    Parser parser(tokens);
    ASTNode* root = nullptr;
    try { root = parser.parse(); }
    catch (...) {
        std::cerr << "[REPL] Parse error\n";
        return false;
    }
    if (!root) return false;

    // 3. Checker (경고만, 에러 시 계속 진행)
    CheckResult cr = checker_.check(root, "<repl>");
    // REPL에서는 에러가 있어도 최대한 실행 시도

    // 4. Compile
    compiler_.setCheckerResult(cr);
    std::vector<Instruction> bytecode = compiler_.compile(root, "<repl>");
    delete root;

    if (bytecode.empty()) return false;

    // 5. SVM 실행 (globals 유지됨)
    svm_.run(bytecode);
    lineNum_++;
    return true;
}

// ──────────────────────────────────────────────────────────────────
//  래퍼: REPL 입력을 완전한 BLUSK 프로그램으로 변환
// ──────────────────────────────────────────────────────────────────
std::string ReplSession::wrapSource(const std::string& src) const {
    // 이미 import blusk26이 있으면 그대로
    if (src.find("import blusk26") != std::string::npos) return src;
    // @entry main이 있으면 그대로
    if (src.find("@entry") != std::string::npos) return src;

    // 단순 표현식/문장이면 @entry main으로 감싸기
    std::ostringstream oss;
    oss << "import blusk26;\nimport io;\n"
        << "@entry\nBlusk public void main() {\n"
        << src << "\nthe end;\n}\n";
    return oss.str();
}

// ──────────────────────────────────────────────────────────────────
//  REPL 명령어
// ──────────────────────────────────────────────────────────────────
bool ReplSession::handleCommand(const std::string& cmd) {
    std::string c = cmd;
    // 앞뒤 공백 제거
    while (!c.empty() && std::isspace(c.front())) c.erase(c.begin());
    while (!c.empty() && std::isspace(c.back()))  c.pop_back();

    if (c == ":quit" || c == ":q" || c == ":exit") {
        std::cerr << "[REPL] Goodbye.\n";
        running_ = false;
        return false;
    }
    if (c == ":help" || c == ":h") { printHelp(); return false; }
    if (c == ":clear") {
        multilineBuffer_.clear(); braceDepth_ = 0;
        std::cerr << "[REPL] Buffer cleared.\n";
        return false;
    }
    if (c == ":dis") {
        std::cerr << "[REPL] :dis — disassemble last compiled bytecode\n";
        // TODO: 마지막 bytecode 저장 후 출력
        return false;
    }
    if (c == ":gc") {
        BluskGC::instance().collectCycles();
        BluskGC::instance().printStats();
        return false;
    }
    std::cerr << "[REPL] Unknown command: " << c << " (try :help)\n";
    return false;
}

void ReplSession::prompt()     const { std::cout << "blusk> " << std::flush; }
void ReplSession::promptCont() const { std::cout << "  ... " << std::flush; }

void ReplSession::printHelp() const {
    std::cerr << "\n  BLUSK REPL Commands:\n"
              << "  :help   — show this message\n"
              << "  :quit   — exit REPL\n"
              << "  :clear  — clear current buffer\n"
              << "  :gc     — trigger GC + print stats\n"
              << "  :dis    — disassemble last bytecode\n\n"
              << "  Tip: multi-line input is supported\n"
              << "       open { and close } to submit a block\n\n";
}

bool ReplSession::isComplete(const std::string& src) {
    int depth = 0;
    for (char c : src) {
        if (c == '{') depth++;
        if (c == '}') depth--;
    }
    return depth <= 0;
}
