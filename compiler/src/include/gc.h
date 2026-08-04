// =============================================================
//  BLUSK gc.h  -  런타임 GC
//  RC (Reference Counting) + Trial Deletion Cycle Collector
// =============================================================
#pragma once
#include "value.h"
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <cstdint>

// ------------------------------------------------------------------
//  GC 통계
// ------------------------------------------------------------------
struct GCStats {
    uint64_t totalAllocs   = 0;
    uint64_t totalFrees    = 0;
    uint64_t cycleFrees    = 0;
    uint64_t rcSkipSavings = 0; // RC 생략으로 절약한 연산 수
    size_t   liveObjects   = 0;
};

// ------------------------------------------------------------------
//  BluskGC  -  싱글턴 GC
//
//  설계:
//    - RC (즉시 해제) : rcSkip=false 객체
//    - rcSkip=true    : RC 연산 스킵 (Checker 보장)
//    - Cycle GC       : 주기적으로 후보 집합 탐색
//                       Trial Deletion (Bacon & Rajan, 2001)
// ------------------------------------------------------------------
class BluskGC {
public:
    static BluskGC& instance() {
        static BluskGC gc;
        return gc;
    }

    // ── 객체 등록 (new 시 호출) ───────────────────────────────────
    void trackObject(BluskObject* obj);

    // ── 이미 추적 중인 객체인지 조회 ─────────────────────────────
    // Lets a caller distinguish "this object has never been bound to any
    // variable yet" (needs trackObject, refCount starts at 1) from "this
    // object is already owned somewhere else" (needs incRef instead --
    // trackObject would wrongly reset its refCount back to 1).
    bool isTracked(BluskObject* obj) const;

    // ── RC 증가 ──────────────────────────────────────────────────
    void incRef(BluskObject* obj);

    // ── RC 감소 + 즉시 해제 시도 ─────────────────────────────────
    void decRef(BluskObject* obj);

    // ── Cycle GC 트리거 (주기적 or 수동 호출) ───────────────────
    void collectCycles();

    // ── 통계 ─────────────────────────────────────────────────────
    const GCStats& stats() const { return stats_; }
    void printStats() const;

    // ── 전체 해제 (프로그램 종료 시) ─────────────────────────────
    void shutdown();

private:
    BluskGC() = default;
    ~BluskGC() { shutdown(); }
    BluskGC(const BluskGC&) = delete;

    // 살아있는 전체 객체 (weak ownership, shared_ptr 별도)
    std::unordered_set<BluskObject*> liveSet_;

    // Cycle GC 후보 (rc > 0 이지만 외부 루트 없는 것)
    std::vector<BluskObject*> candidates_;

    std::mutex mutable mtx_;
    GCStats    stats_;

    // ── Trial Deletion 단계들 ────────────────────────────────────
    void markCandidates();         // 1단계: 후보 수집
    void scanCandidates();         // 2단계: 내부 그래프 임시 감산
    void collectWhite();           // 3단계: rc==0 → 순환 쓰레기 해제

    // 내부 탐색 헬퍼
    void trialDecRef(BluskObject* obj, std::unordered_set<BluskObject*>& visited);
    void trialIncRef(BluskObject* obj, std::unordered_set<BluskObject*>& visited);
    bool isRootReachable(BluskObject* obj);

    // 즉시 해제
    void freeObject(BluskObject* obj);
};

// ------------------------------------------------------------------
//  편의 매크로
// ------------------------------------------------------------------
#define GC_TRACK(obj)   BluskGC::instance().trackObject(obj)
#define GC_INCREF(obj)  do { if(obj && !obj->rcSkip) BluskGC::instance().incRef(obj); } while(0)
#define GC_DECREF(obj)  do { if(obj && !obj->rcSkip) BluskGC::instance().decRef(obj); } while(0)
#define GC_COLLECT()    BluskGC::instance().collectCycles()
