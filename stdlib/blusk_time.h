// =============================================================
//  BLUSK stdlib/blusk_time.h  -  시간 라이브러리
// =============================================================
#pragma once
#include "../compiler/src/include/value.h"
#include <vector>

namespace BluskStd::Time {
    Value now      (const std::vector<Value>& args); // now() -> ms since epoch
    Value perfNow  (const std::vector<Value>& args); // perf_now() -> us 정밀도
    Value sleep    (const std::vector<Value>& args); // sleep(ms)
    Value timestamp(const std::vector<Value>& args); // timestamp() -> "YYYY-MM-DD HH:MM:SS"
    Value date     (const std::vector<Value>& args); // date() -> "YYYY-MM-DD"
    Value elapsed  (const std::vector<Value>& args); // elapsed(start) -> ms since start
}
