// =============================================================
//  BLUSK scope.h  -  독립 스코프 관리자
//  Checker와 VM 양쪽에서 사용 가능한 범용 스코프
// =============================================================
#pragma once
#include "value.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

// ------------------------------------------------------------------
//  ScopeVar : 스코프 내 변수 정보
// ------------------------------------------------------------------
struct ScopeVar {
    Value       val;
    bool        isConst   = false;
    bool        rcSkip    = false;
    int         declLine  = 0;
    std::string typeName;   // 향후 명시적 타입 지원용
};

// ------------------------------------------------------------------
//  Scope : 단일 스코프 (블록 하나)
// ------------------------------------------------------------------
class Scope {
public:
    explicit Scope(Scope* parent = nullptr) : parent_(parent) {}

    // 변수 선언
    void declare(const std::string& name, const Value& val,
                 bool isConst = false, bool rcSkip = false, int line = 0);

    // 변수 조회 (부모 스코프까지 탐색)
    std::optional<Value> get(const std::string& name) const;

    // 변수 정보 조회
    ScopeVar*       getInfo(const std::string& name);
    const ScopeVar* getInfo(const std::string& name) const;

    // 변수 대입 (가장 가까운 스코프에서 탐색 후 수정)
    bool assign(const std::string& name, const Value& val);

    // 이 스코프에만 있는지
    bool hasLocal(const std::string& name) const;

    // 모든 로컬 변수 반환
    const std::unordered_map<std::string, ScopeVar>& locals() const { return vars_; }

    Scope* parent() { return parent_; }

private:
    std::unordered_map<std::string, ScopeVar> vars_;
    Scope* parent_ = nullptr;
};

// ------------------------------------------------------------------
//  ScopeStack : 스코프 스택 관리자
//  VM과 Checker 모두 이걸 쓰도록 통일
// ------------------------------------------------------------------
class ScopeStack {
public:
    ScopeStack()  { push(); }   // 전역 스코프 자동 생성
    ~ScopeStack() { while (!stack_.empty()) pop(); }

    // 스코프 진입/탈출
    void push();
    void pop();

    // 현재 스코프에 변수 선언
    void declare(const std::string& name, const Value& val,
                 bool isConst = false, bool rcSkip = false, int line = 0);

    // 변수 조회 (스코프 체인 전체)
    std::optional<Value> get(const std::string& name) const;
    ScopeVar*            getInfo(const std::string& name);

    // 변수 대입
    bool assign(const std::string& name, const Value& val);

    // 현재 깊이
    int depth() const { return (int)stack_.size(); }

    // 전역 스코프에 직접 설정
    void setGlobal(const std::string& name, const Value& val,
                   bool isConst = false, bool rcSkip = false);

    std::optional<Value> getGlobal(const std::string& name) const;

    // 현재 스코프 로컬 전체
    const std::unordered_map<std::string, ScopeVar>* currentLocals() const;

private:
    std::vector<Scope> stack_;
};
