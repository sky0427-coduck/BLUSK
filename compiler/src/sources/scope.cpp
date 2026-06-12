// =============================================================
//  BLUSK scope.cpp
// =============================================================
#include "../include/scope.h"
#include "../include/error.h"

// ── Scope ────────────────────────────────────────────────────────

void Scope::declare(const std::string& name, const Value& val,
                    bool isConst, bool rcSkip, int line)
{
    ScopeVar sv;
    sv.val     = val;
    sv.isConst = isConst;
    sv.rcSkip  = rcSkip;
    sv.declLine= line;
    vars_[name] = sv;
}

std::optional<Value> Scope::get(const std::string& name) const {
    auto it = vars_.find(name);
    if (it != vars_.end()) return it->second.val;
    if (parent_) return parent_->get(name);
    return std::nullopt;
}

ScopeVar* Scope::getInfo(const std::string& name) {
    auto it = vars_.find(name);
    if (it != vars_.end()) return &it->second;
    if (parent_) return parent_->getInfo(name);
    return nullptr;
}

const ScopeVar* Scope::getInfo(const std::string& name) const {
    auto it = vars_.find(name);
    if (it != vars_.end()) return &it->second;
    if (parent_) return parent_->getInfo(name);
    return nullptr;
}

bool Scope::assign(const std::string& name, const Value& val) {
    auto it = vars_.find(name);
    if (it != vars_.end()) {
        if (it->second.isConst) {
            BluskError::report("Cannot reassign constant '" + name + "'", "scope", 0);
            return false;
        }
        it->second.val = val;
        return true;
    }
    if (parent_) return parent_->assign(name, val);
    return false;
}

bool Scope::hasLocal(const std::string& name) const {
    return vars_.count(name) > 0;
}

// ── ScopeStack ───────────────────────────────────────────────────

void ScopeStack::push() {
    if (stack_.empty()) {
        stack_.emplace_back(nullptr);
    } else {
        // 이전 스코프를 부모로 연결
        // vector 재할당으로 포인터 무효화 방지: 인덱스 기반
        stack_.emplace_back(&stack_.back());
    }
}

void ScopeStack::pop() {
    if (stack_.size() > 1) stack_.pop_back();
    // 전역 스코프는 유지
}

void ScopeStack::declare(const std::string& name, const Value& val,
                         bool isConst, bool rcSkip, int line)
{
    if (stack_.empty()) return;
    stack_.back().declare(name, val, isConst, rcSkip, line);
}

std::optional<Value> ScopeStack::get(const std::string& name) const {
    // 역순 탐색 (현재 스코프 → 전역)
    for (int i = (int)stack_.size()-1; i >= 0; i--) {
        if (stack_[i].hasLocal(name))
            return stack_[i].get(name);
    }
    return std::nullopt;
}

ScopeVar* ScopeStack::getInfo(const std::string& name) {
    for (int i = (int)stack_.size()-1; i >= 0; i--) {
        auto* sv = const_cast<Scope&>(stack_[i]).getInfo(name);
        if (sv && stack_[i].hasLocal(name)) return sv;
    }
    return nullptr;
}

bool ScopeStack::assign(const std::string& name, const Value& val) {
    for (int i = (int)stack_.size()-1; i >= 0; i--) {
        if (stack_[i].hasLocal(name))
            return const_cast<Scope&>(stack_[i]).assign(name, val);
    }
    return false;
}

void ScopeStack::setGlobal(const std::string& name, const Value& val,
                            bool isConst, bool rcSkip) {
    if (stack_.empty()) return;
    stack_.front().declare(name, val, isConst, rcSkip, 0);
}

std::optional<Value> ScopeStack::getGlobal(const std::string& name) const {
    if (stack_.empty()) return std::nullopt;
    return stack_.front().get(name);
}

const std::unordered_map<std::string, ScopeVar>* ScopeStack::currentLocals() const {
    if (stack_.empty()) return nullptr;
    return &stack_.back().locals();
}
