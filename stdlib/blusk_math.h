// =============================================================
//  BLUSK stdlib/blusk_math.h
// =============================================================
#pragma once
#include "../compiler/src/include/value.h"
#include <vector>

namespace BluskStd::Math {
    Value sqrt   (const std::vector<Value>& args);
    Value pow    (const std::vector<Value>& args);
    Value abs    (const std::vector<Value>& args);
    Value ceil   (const std::vector<Value>& args);
    Value floor  (const std::vector<Value>& args);
    Value round  (const std::vector<Value>& args);
    Value sin    (const std::vector<Value>& args);
    Value cos    (const std::vector<Value>& args);
    Value tan    (const std::vector<Value>& args);
    Value asin   (const std::vector<Value>& args);
    Value acos   (const std::vector<Value>& args);
    Value atan   (const std::vector<Value>& args);
    Value atan2  (const std::vector<Value>& args);
    Value exp    (const std::vector<Value>& args);
    Value log    (const std::vector<Value>& args);
    Value log10  (const std::vector<Value>& args);
    Value log2   (const std::vector<Value>& args);
    Value min    (const std::vector<Value>& args);
    Value max    (const std::vector<Value>& args);
    Value clamp  (const std::vector<Value>& args); // clamp(v, lo, hi)
    Value lerp   (const std::vector<Value>& args); // lerp(a, b, t)
    Value pi     (const std::vector<Value>& args); // PI 상수
    Value e      (const std::vector<Value>& args); // e 상수
    Value random (const std::vector<Value>& args); // random() -> [0,1)
    Value randInt(const std::vector<Value>& args); // rand_int(lo, hi)
    Value invsqrt(const std::vector<Value>& args); // 1/sqrt(x) 고속
    Value gcd    (const std::vector<Value>& args);
    Value lcm    (const std::vector<Value>& args);
    Value isPrime(const std::vector<Value>& args);
}
