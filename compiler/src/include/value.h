// =============================================================
//  BLUSK value.h  -  int/long/float/double 실제 구분 추가
// =============================================================
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cstdint>
#include <ostream>
#include <limits>

// ------------------------------------------------------------------
//  VType : 최상위 값 종류
// ------------------------------------------------------------------
enum class VType : uint8_t {
    NIL=0, BOOL, INT, FLOAT, STRING, OBJECT, ARRAY, TENSOR, MATRIX,
};

// ------------------------------------------------------------------
//  NumKind : INT/FLOAT 내부에서 실제 정밀도/범위 구분
//
//  gg          → 타입 추론. 정수면 NK_INT(=long 취급), 실수면 NK_DOUBLE
//  int         → 32bit 정수, 범위 [-2147483648, 2147483647]
//  long        → 64bit 정수 (gg의 정수 기본값과 동일)
//  float       → 32bit 실수 (정밀도 ~7자리)
//  double      → 64bit 실수 (정밀도 ~15자리, gg의 실수 기본값과 동일)
// ------------------------------------------------------------------
enum class NumKind : uint8_t {
    NK_NONE = 0,
    NK_INT32,    // int
    NK_INT64,    // long / gg(정수)
    NK_FLOAT32,  // float
    NK_FLOAT64,  // double / gg(실수)
};

// 32bit 정수 범위로 클램프 + 오버플로 경고용 체크
inline bool fitsInt32(int64_t v) {
    return v >= std::numeric_limits<int32_t>::min() &&
           v <= std::numeric_limits<int32_t>::max();
}

struct Value;
struct BluskObject;

// ------------------------------------------------------------------
//  BluskMatrix
// ------------------------------------------------------------------
struct BluskMatrix {
    size_t rows = 0, cols = 0;
    std::vector<double> data;

    BluskMatrix() = default;
    BluskMatrix(size_t r, size_t c, double init=0.0)
        : rows(r), cols(c), data(r*c, init) {}

    double& at(size_t r, size_t c)       { return data[r*cols+c]; }
    double  at(size_t r, size_t c) const { return data[r*cols+c]; }

    BluskMatrix matmul(const BluskMatrix& rhs) const;
    BluskMatrix add(const BluskMatrix& rhs) const;
    BluskMatrix elmul(const BluskMatrix& rhs) const;
    BluskMatrix transpose() const;
    std::string toString() const;
};

// ------------------------------------------------------------------
//  BluskObject
// ------------------------------------------------------------------
struct BluskObject {
    std::string className;
    std::unordered_map<std::string, Value> fields;
    int  refCount = 0;
    bool rcSkip   = false;
};

// ------------------------------------------------------------------
//  Value
// ------------------------------------------------------------------
struct Value {
    VType   type    = VType::NIL;
    NumKind numKind = NumKind::NK_NONE;  // INT/FLOAT일 때만 의미 있음

    union Num {
        int64_t i = 0;   // INT 계열 저장 (int32도 64bit 슬롯에 저장, numKind로 구분)
        double  f;       // FLOAT 계열 저장 (float32도 double 슬롯에 저장)
        bool    b;
    } num;

    std::string                          str;
    std::shared_ptr<BluskObject>         obj;
    std::shared_ptr<std::vector<Value>>  arr;
    std::shared_ptr<std::vector<double>> tensor;
    size_t tensorSize = 0;
    std::shared_ptr<BluskMatrix>         mat;

    bool rcSkip = false;

    // ── 팩토리 ─────────────────────────────────────────────────
    static Value Nil()  { return Value{}; }
    static Value Bool(bool v) { Value r; r.type=VType::BOOL; r.num.b=v; return r; }

    // gg / 기존 코드 호환: 기본은 64bit 정수, 64bit 실수
    static Value Int(int64_t v)   { Value r; r.type=VType::INT;   r.numKind=NumKind::NK_INT64;   r.num.i=v; return r; }
    static Value Float(double v)  { Value r; r.type=VType::FLOAT; r.numKind=NumKind::NK_FLOAT64;  r.num.f=v; return r; }

    // ── 명시적 타입 팩토리 (int/long/float/double) ─────────────
    static Value Int32(int64_t v) {
        Value r; r.type=VType::INT; r.numKind=NumKind::NK_INT32;
        // 32bit 범위로 wrap (오버플로 시 C 표준 정수 래핑과 동일하게)
        r.num.i = (int64_t)(int32_t)v;
        return r;
    }
    static Value Int64(int64_t v) { Value r; r.type=VType::INT; r.numKind=NumKind::NK_INT64; r.num.i=v; return r; }
    static Value Float32(double v) {
        Value r; r.type=VType::FLOAT; r.numKind=NumKind::NK_FLOAT32;
        r.num.f = (double)(float)v;  // float 정밀도로 절삭
        return r;
    }
    static Value Float64(double v) { Value r; r.type=VType::FLOAT; r.numKind=NumKind::NK_FLOAT64; r.num.f=v; return r; }

    static Value String(const std::string& v) { Value r; r.type=VType::STRING; r.str=v; return r; }

    static Value Array(size_t reserve=0) {
        Value r; r.type=VType::ARRAY;
        r.arr=std::make_shared<std::vector<Value>>();
        if(reserve) r.arr->reserve(reserve); return r;
    }
    static Value Tensor(const std::vector<double>& d) {
        Value r; r.type=VType::TENSOR;
        r.tensor=std::make_shared<std::vector<double>>(d);
        r.tensorSize=d.size(); return r;
    }
    static Value Tensor(size_t n, double iv=0.0) {
        Value r; r.type=VType::TENSOR;
        r.tensor=std::make_shared<std::vector<double>>(n,iv);
        r.tensorSize=n; return r;
    }
    static Value Object(std::shared_ptr<BluskObject> o) { Value r; r.type=VType::OBJECT; r.obj=o; return r; }
    static Value Matrix(std::shared_ptr<BluskMatrix> m) { Value r; r.type=VType::MATRIX; r.mat=m; return r; }
    static Value Matrix(size_t rows, size_t cols, const std::vector<double>& data) {
        auto m=std::make_shared<BluskMatrix>(rows, cols); m->data=data;
        Value r; r.type=VType::MATRIX; r.mat=m; return r;
    }

    // ── 타입 키워드 → 팩토리 라우팅 ──────────────────────────────
    // VAR_DECL의 키워드(int/long/float/double/gg)에 따라 적절한 Value 생성
    static Value FromTypeKeyword(const std::string& kw, double rawNum, bool isFloatLit) {
        if (kw=="int")    return Value::Int32(  (int64_t)rawNum );
        if (kw=="long")   return Value::Int64(  (int64_t)rawNum );
        if (kw=="float")  return Value::Float32( rawNum );
        if (kw=="double") return Value::Float64( rawNum );
        // gg: 리터럴이 실수면 double, 정수면 long (기존 동작 유지)
        return isFloatLit ? Value::Float64(rawNum) : Value::Int64((int64_t)rawNum);
    }

    // ── 타입 확인 ──────────────────────────────────────────────
    bool isNil()    const { return type==VType::NIL; }
    bool isBool()   const { return type==VType::BOOL; }
    bool isInt()    const { return type==VType::INT; }
    bool isFloat()  const { return type==VType::FLOAT; }
    bool isNum()    const { return type==VType::INT||type==VType::FLOAT; }
    bool isString() const { return type==VType::STRING; }
    bool isObj()    const { return type==VType::OBJECT; }
    bool isArr()    const { return type==VType::ARRAY; }
    bool isTensor() const { return type==VType::TENSOR; }
    bool isMatrix() const { return type==VType::MATRIX; }

    bool isInt32()   const { return type==VType::INT   && numKind==NumKind::NK_INT32; }
    bool isInt64()   const { return type==VType::INT   && numKind==NumKind::NK_INT64; }
    bool isFloat32() const { return type==VType::FLOAT && numKind==NumKind::NK_FLOAT32; }
    bool isFloat64() const { return type==VType::FLOAT && numKind==NumKind::NK_FLOAT64; }

    // 타입명 문자열 (디버그/에러메시지용)
    std::string typeName() const {
        switch(type) {
        case VType::NIL: return "nil";
        case VType::BOOL: return "bool";
        case VType::INT:  return numKind==NumKind::NK_INT32 ? "int" : "long";
        case VType::FLOAT:return numKind==NumKind::NK_FLOAT32 ? "float" : "double";
        case VType::STRING: return "str";
        case VType::OBJECT: return "object";
        case VType::ARRAY: return "array";
        case VType::TENSOR: return "tensor";
        case VType::MATRIX: return "Matrix";
        default: return "?";
        }
    }

    // ── 변환 ───────────────────────────────────────────────────
    double      toDouble() const;
    int64_t     toInt()    const;
    bool        toBool()   const;
    std::string toString() const;

    // ── as 캐스팅 (int/long/float/double/str/bool 전부 지원) ────
    Value castTo(const std::string& typeName) const;

    // ── 산술 (정밀도 승격 규칙 적용) ─────────────────────────────
    Value operator+(const Value& o) const;
    Value operator-(const Value& o) const;
    Value operator*(const Value& o) const;
    Value operator/(const Value& o) const;
    Value operator%(const Value& o) const;
    Value blusk_pow(const Value& o) const;
    Value operator-() const;

    // ── tensor ─────────────────────────────────────────────────
    Value tensorAdd(const Value& o)  const;
    Value tensorMul(const Value& o)  const;
    Value tensorDot(const Value& o)  const;
    Value tensorNorm()                const;
    Value tensorSum()                 const;
    Value tensorMean()                const;
    Value tensorStd()                 const;
    static Value fmadd(const Value& a, const Value& b, const Value& c);
    static Value invsqrt(const Value& x);

    // ── 비교 ───────────────────────────────────────────────────
    bool operator==(const Value& o) const;
    bool operator!=(const Value& o) const { return !(*this==o); }
    bool operator< (const Value& o) const;
    bool operator<=(const Value& o) const { return *this<o||*this==o; }
    bool operator> (const Value& o) const { return !(*this<=o); }
    bool operator>=(const Value& o) const { return !(*this<o); }

private:
    // 산술 결과의 NumKind 결정: 더 넓은 타입으로 승격
    // int32+int32=int32, int32+long=long, float32+float32=float32,
    // float32+double=double, int*float=float(승격된 쪽)
    static NumKind promote(NumKind a, NumKind b);
};

inline std::ostream& operator<<(std::ostream& os, const Value& v) {
    return os << v.toString();
}
