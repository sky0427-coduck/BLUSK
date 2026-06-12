// =============================================================
//  BLUSK string_table.h  -  문자열 인터닝
//  동일 내용의 문자열은 단일 포인터로 공유 → 비교 O(1)
// =============================================================
#pragma once
#include <string>
#include <unordered_set>
#include <mutex>
#include <cstdint>

// ------------------------------------------------------------------
//  InternedString : 인터닝된 문자열 핸들
//  내부적으로 전역 테이블의 포인터를 가리킴
//  → 비교가 포인터 비교 1회로 끝남
// ------------------------------------------------------------------
class InternedString {
public:
    InternedString() : ptr_(nullptr) {}
    explicit InternedString(const std::string& s);
    explicit InternedString(const char* s);

    const std::string& str()  const { return *ptr_; }
    const char*        c_str()const { return ptr_->c_str(); }
    bool               empty()const { return !ptr_ || ptr_->empty(); }

    // ── 포인터 비교 (O(1)) ────────────────────────────────────
    bool operator==(const InternedString& o) const { return ptr_ == o.ptr_; }
    bool operator!=(const InternedString& o) const { return ptr_ != o.ptr_; }
    bool operator< (const InternedString& o) const { return ptr_ <  o.ptr_; }

    // std::string과 비교
    bool operator==(const std::string& s) const { return ptr_ && *ptr_ == s; }

private:
    const std::string* ptr_;
};

// ------------------------------------------------------------------
//  StringTable : 전역 문자열 풀
// ------------------------------------------------------------------
class StringTable {
public:
    static StringTable& instance() {
        static StringTable table;
        return table;
    }

    // 인터닝: 동일 내용이면 기존 포인터 반환
    const std::string* intern(const std::string& s);
    const std::string* intern(const char* s);

    size_t size() const { return pool_.size(); }
    void   clear();

private:
    StringTable() = default;
    std::unordered_set<std::string> pool_;
    std::mutex mtx_;
};

// ------------------------------------------------------------------
//  해시 지원 (unordered_map key로 사용)
// ------------------------------------------------------------------
namespace std {
    template<>
    struct hash<InternedString> {
        size_t operator()(const InternedString& s) const {
            return std::hash<const void*>{}(static_cast<const void*>(&s.str()));
        }
    };
}
