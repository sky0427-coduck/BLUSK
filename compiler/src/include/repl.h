// =============================================================
//  BLUSK repl.h  -  대화형 실행 (Read-Eval-Print Loop)
// =============================================================
#pragma once
#include "lexer.h"
#include "parser.h"
#include "checker.h"
#include "compiler.h"
#include "vm.h"
#include <string>
#include <vector>

// ------------------------------------------------------------------
//  ReplSession : 상태를 유지하며 줄 단위로 실행
//  - 변수/객체 상태 세션 간 유지
//  - 멀티라인 입력 지원 ({ 열리면 } 닫힐 때까지 누적)
//  - :help, :quit, :vars, :dis 명령어
// ------------------------------------------------------------------
class ReplSession {
public:
    ReplSession();
    ~ReplSession() = default;

    // REPL 시작 (루프)
    void run();

    // 단일 입력 처리 (테스트용)
    bool evalLine(const std::string& line);

private:
    SVM         svm_;
    Compiler    compiler_;
    BluskChecker checker_;

    std::string  multilineBuffer_;
    int          braceDepth_ = 0;
    bool         running_    = true;
    int          lineNum_    = 1;

    // 세션 공유 바이트코드 (변수 유지)
    // SVM은 내부적으로 globals_ 유지함

    // ── REPL 명령어 처리 ─────────────────────────────────────────
    bool handleCommand(const std::string& cmd);

    // ── 입력 완성 여부 판단 ──────────────────────────────────────
    bool isComplete(const std::string& src);

    // ── 소스 → 실행 파이프라인 ───────────────────────────────────
    bool evalSource(const std::string& src);

    // ── 입력 프롬프트 출력 ───────────────────────────────────────
    void prompt() const;
    void promptCont() const;

    // ── 도움말 출력 ──────────────────────────────────────────────
    void printHelp() const;

    // ── REPL 래퍼 (import blusk26 자동 추가) ─────────────────────
    std::string wrapSource(const std::string& src) const;
};
