// =============================================================
//  BLUSK stdlib/blusk_math.cpp
// =============================================================
#include "blusk_math.h"
#include "../compiler/src/include/value.h"
#include <cmath>
#include <random>
#include <numeric>
#include <immintrin.h>

namespace BluskStd::Math {

static double arg0(const std::vector<Value>& a) { return a.empty() ? 0.0 : a[0].toDouble(); }
static double arg1(const std::vector<Value>& a) { return a.size()<2 ? 0.0 : a[1].toDouble(); }

Value sqrt   (const std::vector<Value>& a){ return Value::Float(std::sqrt(arg0(a))); }
Value pow    (const std::vector<Value>& a){ return Value::Float(std::pow(arg0(a), arg1(a))); }
Value abs    (const std::vector<Value>& a){
    if (!a.empty() && a[0].isInt()) return Value::Int(std::abs(a[0].num.i));
    return Value::Float(std::fabs(arg0(a)));
}
Value ceil   (const std::vector<Value>& a){ return Value::Int((int64_t)std::ceil(arg0(a))); }
Value floor  (const std::vector<Value>& a){ return Value::Int((int64_t)std::floor(arg0(a))); }
Value round  (const std::vector<Value>& a){ return Value::Int((int64_t)std::round(arg0(a))); }
Value sin    (const std::vector<Value>& a){ return Value::Float(std::sin(arg0(a))); }
Value cos    (const std::vector<Value>& a){ return Value::Float(std::cos(arg0(a))); }
Value tan    (const std::vector<Value>& a){ return Value::Float(std::tan(arg0(a))); }
Value asin   (const std::vector<Value>& a){ return Value::Float(std::asin(arg0(a))); }
Value acos   (const std::vector<Value>& a){ return Value::Float(std::acos(arg0(a))); }
Value atan   (const std::vector<Value>& a){ return Value::Float(std::atan(arg0(a))); }
Value atan2  (const std::vector<Value>& a){ return Value::Float(std::atan2(arg0(a), arg1(a))); }
Value exp    (const std::vector<Value>& a){ return Value::Float(std::exp(arg0(a))); }
Value log    (const std::vector<Value>& a){ return Value::Float(std::log(arg0(a))); }
Value log10  (const std::vector<Value>& a){ return Value::Float(std::log10(arg0(a))); }
Value log2   (const std::vector<Value>& a){ return Value::Float(std::log2(arg0(a))); }

Value min(const std::vector<Value>& a){
    if (a.size() < 2) return a.empty() ? Value::Nil() : a[0];
    return (a[0] < a[1]) ? a[0] : a[1];
}
Value max(const std::vector<Value>& a){
    if (a.size() < 2) return a.empty() ? Value::Nil() : a[0];
    return (a[0] > a[1]) ? a[0] : a[1];
}
Value clamp(const std::vector<Value>& a){
    if (a.size() < 3) return a.empty() ? Value::Nil() : a[0];
    const Value& v = a[0]; const Value& lo = a[1]; const Value& hi = a[2];
    if (v < lo) return lo; if (v > hi) return hi; return v;
}
Value lerp(const std::vector<Value>& a){
    if (a.size() < 3) return Value::Float(0.0);
    double va = a[0].toDouble(), vb = a[1].toDouble(), t = a[2].toDouble();
    return Value::Float(va + t * (vb - va));
}

Value pi(const std::vector<Value>&){ return Value::Float(3.14159265358979323846); }
Value e (const std::vector<Value>&){ return Value::Float(2.71828182845904523536); }

Value random(const std::vector<Value>&){
    static std::mt19937_64 rng(std::random_device{}());
    static std::uniform_real_distribution<double> dist(0.0, 1.0);
    return Value::Float(dist(rng));
}
Value randInt(const std::vector<Value>& a){
    if (a.size() < 2) return Value::Int(0);
    int64_t lo = a[0].toInt(), hi = a[1].toInt();
    if (lo > hi) std::swap(lo, hi);
    static std::mt19937_64 rng(std::random_device{}());
    std::uniform_int_distribution<int64_t> dist(lo, hi);
    return Value::Int(dist(rng));
}

Value invsqrt(const std::vector<Value>& a){
    if (a.empty()) return Value::Nil();
    double v = a[0].toDouble();
    if (v <= 0.0) return Value::Nil();
#ifdef __SSE__
    float fv = (float)v;
    __m128 xv = _mm_set_ss(fv);
    __m128 rv = _mm_rsqrt_ss(xv);
    float r0; _mm_store_ss(&r0, rv);
    double r = r0;
    r = r * (1.5 - 0.5 * v * r * r); // Newton-Raphson 1회
    return Value::Float(r);
#else
    return Value::Float(1.0 / std::sqrt(v));
#endif
}

Value gcd(const std::vector<Value>& a){
    if (a.size() < 2) return Value::Int(0);
    return Value::Int(std::gcd(a[0].toInt(), a[1].toInt()));
}
Value lcm(const std::vector<Value>& a){
    if (a.size() < 2) return Value::Int(0);
    return Value::Int(std::lcm(a[0].toInt(), a[1].toInt()));
}
Value isPrime(const std::vector<Value>& a){
    if (a.empty()) return Value::Bool(false);
    int64_t n = a[0].toInt();
    if (n < 2) return Value::Bool(false);
    if (n == 2) return Value::Bool(true);
    if (n % 2 == 0) return Value::Bool(false);
    for (int64_t i = 3; i * i <= n; i += 2)
        if (n % i == 0) return Value::Bool(false);
    return Value::Bool(true);
}

} // namespace BluskStd::Math
