// =============================================================
//  BLUSK value.h  -  VType::MATRIX 추가
// =============================================================
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cstdint>
#include <ostream>

enum class VType : uint8_t {
    NIL=0, BOOL, INT, FLOAT, STRING, OBJECT, ARRAY, TENSOR,
    MATRIX,   // ← 신규: 2D double 행렬
};

struct Value;
struct BluskObject;

// ------------------------------------------------------------------
//  BluskMatrix : 2D 행렬 (rows × cols)
// ------------------------------------------------------------------
struct BluskMatrix {
    size_t rows = 0, cols = 0;
    std::vector<double> data; // row-major

    BluskMatrix() = default;
    BluskMatrix(size_t r, size_t c, double init=0.0)
        : rows(r), cols(c), data(r*c, init) {}

    double& at(size_t r, size_t c)             { return data[r*cols+c]; }
    double  at(size_t r, size_t c) const       { return data[r*cols+c]; }

    // 행렬 곱 (this @ rhs)
    BluskMatrix matmul(const BluskMatrix& rhs) const;
    // element-wise
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
    VType type = VType::NIL;

    union Num { int64_t i=0; double f; bool b; } num;

    std::string                          str;
    std::shared_ptr<BluskObject>         obj;
    std::shared_ptr<std::vector<Value>>  arr;
    std::shared_ptr<std::vector<double>> tensor;
    size_t tensorSize = 0;
    std::shared_ptr<BluskMatrix>         mat;  // ← 신규

    bool rcSkip = false;

    // ── 팩토리 ─────────────────────────────────────────────────
    static Value Nil()                        { return Value{}; }
    static Value Bool(bool v)                 { Value r; r.type=VType::BOOL;   r.num.b=v; return r; }
    static Value Int(int64_t v)               { Value r; r.type=VType::INT;    r.num.i=v; return r; }
    static Value Float(double v)              { Value r; r.type=VType::FLOAT;  r.num.f=v; return r; }
    static Value String(const std::string& v) { Value r; r.type=VType::STRING; r.str=v;   return r; }

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
    static Value Object(std::shared_ptr<BluskObject> o) {
        Value r; r.type=VType::OBJECT; r.obj=o; return r;
    }
    static Value Matrix(std::shared_ptr<BluskMatrix> m) {
        Value r; r.type=VType::MATRIX; r.mat=m; return r;
    }
    static Value Matrix(size_t rows, size_t cols, const std::vector<double>& data) {
        auto m=std::make_shared<BluskMatrix>(rows, cols);
        m->data=data;
        Value r; r.type=VType::MATRIX; r.mat=m; return r;
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

    // ── 변환 ───────────────────────────────────────────────────
    double      toDouble() const;
    int64_t     toInt()    const;
    bool        toBool()   const;
    std::string toString() const;

    // ── as 캐스팅 ──────────────────────────────────────────────
    Value castTo(const std::string& typeName) const;

    // ── 산술 ───────────────────────────────────────────────────
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
    Value tensorNorm()               const;
    Value tensorSum()                const;
    Value tensorMean()               const;
    Value tensorStd()                const;
    static Value fmadd(const Value& a, const Value& b, const Value& c);
    static Value invsqrt(const Value& x);

    // ── 비교 ───────────────────────────────────────────────────
    bool operator==(const Value& o) const;
    bool operator!=(const Value& o) const { return !(*this==o); }
    bool operator< (const Value& o) const;
    bool operator<=(const Value& o) const { return *this<o||*this==o; }
    bool operator> (const Value& o) const { return !(*this<=o); }
    bool operator>=(const Value& o) const { return !(*this<o); }
};

inline std::ostream& operator<<(std::ostream& os, const Value& v) {
    return os << v.toString();
}
