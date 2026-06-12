// =============================================================
//  BLUSK string_table.cpp
// =============================================================
#include "../include/string_table.h"

const std::string* StringTable::intern(const std::string& s) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto [it, _] = pool_.insert(s);
    return &(*it);
}

const std::string* StringTable::intern(const char* s) {
    return intern(std::string(s));
}

void StringTable::clear() {
    std::lock_guard<std::mutex> lock(mtx_);
    pool_.clear();
}

// ── InternedString 생성자 ────────────────────────────────────────
InternedString::InternedString(const std::string& s)
    : ptr_(StringTable::instance().intern(s)) {}

InternedString::InternedString(const char* s)
    : ptr_(StringTable::instance().intern(s)) {}
