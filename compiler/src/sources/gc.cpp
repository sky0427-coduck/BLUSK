// =============================================================
//  BLUSK gc.cpp  -  RC + Trial Deletion Cycle GC 구현
// =============================================================
#include "../include/gc.h"
#include "../include/error.h"
#include <iostream>
#include <algorithm>

// ──────────────────────────────────────────────────────────────────
//  객체 등록
// ──────────────────────────────────────────────────────────────────
void BluskGC::trackObject(BluskObject* obj) {
    if (!obj) return;
    std::lock_guard<std::mutex> lock(mtx_);
    obj->refCount = 1;
    liveSet_.insert(obj);
    stats_.totalAllocs++;
    stats_.liveObjects++;
}

// ──────────────────────────────────────────────────────────────────
//  RC 증가
// ──────────────────────────────────────────────────────────────────
void BluskGC::incRef(BluskObject* obj) {
    if (!obj) return;
    // rcSkip 객체는 Checker가 수명 보장 → RC 연산 생략
    if (obj->rcSkip) { stats_.rcSkipSavings++; return; }
    std::lock_guard<std::mutex> lock(mtx_);
    obj->refCount++;
}

// ──────────────────────────────────────────────────────────────────
//  RC 감소 + 즉시 해제 시도
// ──────────────────────────────────────────────────────────────────
void BluskGC::decRef(BluskObject* obj) {
    if (!obj) return;
    if (obj->rcSkip) { stats_.rcSkipSavings++; return; }

    std::lock_guard<std::mutex> lock(mtx_);
    if (obj->refCount <= 0) return;
    obj->refCount--;

    if (obj->refCount == 0) {
        // 즉시 해제 가능 (순환 없음 확정)
        freeObject(obj);
    } else {
        // rc > 0이지만 외부에서 접근 불가 가능성 → 순환 후보
        candidates_.push_back(obj);
    }
}

// ──────────────────────────────────────────────────────────────────
//  즉시 해제
// ──────────────────────────────────────────────────────────────────
void BluskGC::freeObject(BluskObject* obj) {
    liveSet_.erase(obj);
    stats_.totalFrees++;
    stats_.liveObjects--;
    // fields에 담긴 Value들의 obj 참조 감소
    for (auto& [name, val] : obj->fields) {
        if (val.isObj() && val.obj) {
            BluskObject* child = val.obj.get();
            if (child != obj && !child->rcSkip) {
                child->refCount--;
                if (child->refCount == 0) freeObject(child);
            }
        }
    }
    // shared_ptr 해제는 Value 소멸자가 처리
    // 여기서는 추적 레코드만 제거
}

// ──────────────────────────────────────────────────────────────────
//  Cycle GC  -  Trial Deletion (Bacon & Rajan, 2001)
//
//  단계:
//    1. 후보 수집: liveSet에서 rc>0 && 외부 접근 의심 객체
//    2. 내부 그래프 임시 decRef  (trialDecRef)
//    3. 루트 도달 가능 객체 복원  (trialIncRef)
//    4. 남은 rc==0 → 순환 쓰레기 → 해제
// ──────────────────────────────────────────────────────────────────
void BluskGC::collectCycles() {
    std::lock_guard<std::mutex> lock(mtx_);
    if (candidates_.empty()) return;

    // 중복 제거
    std::sort(candidates_.begin(), candidates_.end());
    candidates_.erase(std::unique(candidates_.begin(), candidates_.end()), candidates_.end());

    // 1단계: 후보 확정 (liveSet에 실제 있는 것만)
    std::vector<BluskObject*> valid;
    for (BluskObject* obj : candidates_) {
        if (liveSet_.count(obj) && obj->refCount > 0)
            valid.push_back(obj);
    }
    candidates_.clear();
    if (valid.empty()) return;

    // 2단계: 후보들 내부 참조만 임시 감산
    std::unordered_set<BluskObject*> visited;
    for (BluskObject* obj : valid)
        trialDecRef(obj, visited);

    // 3단계: 외부 루트에서 도달 가능한 것 복원
    visited.clear();
    for (BluskObject* obj : valid)
        if (obj->refCount > 0)
            trialIncRef(obj, visited);

    // 4단계: rc==0 남은 것 = 순환 쓰레기
    for (BluskObject* obj : valid) {
        if (obj->refCount == 0 && liveSet_.count(obj)) {
            freeObject(obj);
            stats_.cycleFrees++;
        }
    }
}

// ──────────────────────────────────────────────────────────────────
//  Trial Decrement: 후보 내부 참조만 임시 감산
// ──────────────────────────────────────────────────────────────────
void BluskGC::trialDecRef(BluskObject* obj,
                           std::unordered_set<BluskObject*>& visited)
{
    if (!obj || visited.count(obj)) return;
    visited.insert(obj);
    for (auto& [name, val] : obj->fields) {
        if (val.isObj() && val.obj) {
            BluskObject* child = val.obj.get();
            if (child && !child->rcSkip) {
                child->refCount--;
                trialDecRef(child, visited);
            }
        }
    }
}

// ──────────────────────────────────────────────────────────────────
//  Trial Increment: 외부 루트 도달 가능 복원
// ──────────────────────────────────────────────────────────────────
void BluskGC::trialIncRef(BluskObject* obj,
                           std::unordered_set<BluskObject*>& visited)
{
    if (!obj || visited.count(obj)) return;
    visited.insert(obj);
    obj->refCount++;
    for (auto& [name, val] : obj->fields) {
        if (val.isObj() && val.obj) {
            BluskObject* child = val.obj.get();
            if (child && !child->rcSkip)
                trialIncRef(child, visited);
        }
    }
}

// ──────────────────────────────────────────────────────────────────
//  통계 출력
// ──────────────────────────────────────────────────────────────────
void BluskGC::printStats() const {
    std::cerr << "\n[BLUSK GC Stats]\n"
              << "  Total allocs : " << stats_.totalAllocs   << "\n"
              << "  Total frees  : " << stats_.totalFrees    << "\n"
              << "  Cycle frees  : " << stats_.cycleFrees    << "\n"
              << "  RC-skip saves: " << stats_.rcSkipSavings << "\n"
              << "  Live objects : " << stats_.liveObjects   << "\n\n";
}

// ──────────────────────────────────────────────────────────────────
//  종료 시 전체 해제
// ──────────────────────────────────────────────────────────────────
void BluskGC::shutdown() {
    std::lock_guard<std::mutex> lock(mtx_);
    if (!liveSet_.empty()) {
        std::cerr << "[BLUSK GC] Shutdown: " << liveSet_.size()
                  << " objects still live (potential leak)\n";
    }
    liveSet_.clear();
    candidates_.clear();
}
