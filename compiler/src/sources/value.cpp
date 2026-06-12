// =============================================================
//  BLUSK value.cpp  -  BluskMatrix + castTo 구현
// =============================================================
#include "../include/value.h"
#include "../include/error.h"
#include <sstream>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <numeric>

// ──────────────────────────────────────────────────────────────────
//  BluskMatrix 연산
// ──────────────────────────────────────────────────────────────────
BluskMatrix BluskMatrix::matmul(const BluskMatrix& rhs) const {
    if (cols != rhs.rows) {
        BluskError::report("Matrix dimension mismatch for matmul", "runtime", 0);
        return {};
    }
    BluskMatrix result(rows, rhs.cols);
    for (size_t i=0; i<rows; i++)
        for (size_t k=0; k<cols; k++)
            for (size_t j=0; j<rhs.cols; j++)
                result.at(i,j) += at(i,k) * rhs.at(k,j);
    return result;
}

BluskMatrix BluskMatrix::add(const BluskMatrix& rhs) const {
    if (rows!=rhs.rows||cols!=rhs.cols) {
        BluskError::report("Matrix dimension mismatch for add", "runtime", 0); return {};
    }
    BluskMatrix result(rows, cols);
    for (size_t i=0; i<data.size(); i++) result.data[i]=data[i]+rhs.data[i];
    return result;
}

BluskMatrix BluskMatrix::elmul(const BluskMatrix& rhs) const {
    // element-wise (Hadamard)
    // 두 벡터가 행 하나씩이면 element-wise
    if (rows!=rhs.rows||cols!=rhs.cols) {
        // 크기 다르면 외적(outer product) 처리
        if (cols==rhs.cols) {
            BluskMatrix result(rows+rhs.rows, cols);
            for (size_t i=0;i<rows;i++) for(size_t j=0;j<cols;j++) result.at(i,j)=at(i,j);
            for (size_t i=0;i<rhs.rows;i++) for(size_t j=0;j<rhs.cols;j++) result.at(rows+i,j)=rhs.at(i,j);
            return result;
        }
        BluskError::report("Matrix dimension mismatch for element-wise mul", "runtime", 0);
        return {};
    }
    BluskMatrix result(rows, cols);
    for (size_t i=0; i<data.size(); i++) result.data[i]=data[i]*rhs.data[i];
    return result;
}

BluskMatrix BluskMatrix::transpose() const {
    BluskMatrix result(cols, rows);
    for (size_t i=0;i<rows;i++)
        for (size_t j=0;j<cols;j++)
            result.at(j,i)=at(i,j);
    return result;
}

std::string BluskMatrix::toString() const {
    std::ostringstream oss;
    oss << "Matrix(" << rows << "x" << cols << ")[\n";
    for (size_t i=0;i<rows;i++) {
        oss << "  [";
        for (size_t j=0;j<cols;j++) {
            if (j>0) oss << ", ";
            oss << std::fixed << std::setprecision(2) << at(i,j);
        }
        oss << "]\n";
    }
    oss << "]";
    return oss.str();
}

// ──────────────────────────────────────────────────────────────────
//  타입 변환
// ──────────────────────────────────────────────────────────────────
double Value::toDouble() const {
    switch(type) {
    case VType::INT:    return (double)num.i;
    case VType::FLOAT:  return num.f;
    case VType::BOOL:   return num.b?1.0:0.0;
    case VType::STRING: try{return std::stod(str);}catch(...){return 0.0;}
    default: return 0.0;
    }
}

int64_t Value::toInt() const {
    switch(type) {
    case VType::INT:    return num.i;
    case VType::FLOAT:  return (int64_t)num.f;
    case VType::BOOL:   return num.b?1:0;
    case VType::STRING: try{return std::stoll(str);}catch(...){return 0;}
    default: return 0;
    }
}

bool Value::toBool() const {
    switch(type) {
    case VType::BOOL:   return num.b;
    case VType::INT:    return num.i!=0;
    case VType::FLOAT:  return num.f!=0.0;
    case VType::STRING: return !str.empty();
    case VType::NIL:    return false;
    default:            return true;
    }
}

std::string Value::toString() const {
    switch(type) {
    case VType::NIL:    return "nil";
    case VType::BOOL:   return num.b?"true":"false";
    case VType::INT:    return std::to_string(num.i);
    case VType::FLOAT: { std::ostringstream oss; oss<<num.f; return oss.str(); }
    case VType::STRING: return str;
    case VType::OBJECT: return obj?"[object "+obj->className+"]":"[null]";
    case VType::ARRAY:  return arr?"[array len="+std::to_string(arr->size())+"]":"[array]";
    case VType::TENSOR: return "[tensor<"+std::to_string(tensorSize)+">]";
    case VType::MATRIX: return mat?mat->toString():"[null matrix]";
    default: return "?";
    }
}

// ── as 캐스팅 ────────────────────────────────────────────────────
Value Value::castTo(const std::string& typeName) const {
    if (typeName=="int"||typeName=="INT")      return Value::Int(toInt());
    if (typeName=="float"||typeName=="FLOAT")  return Value::Float(toDouble());
    if (typeName=="str"||typeName=="string")   return Value::String(toString());
    if (typeName=="bool"||typeName=="BOOL")    return Value::Bool(toBool());
    if (typeName=="double")                    return Value::Float(toDouble());
    BluskError::report("Unknown cast type: "+typeName, "runtime", 0);
    return *this;
}

// ──────────────────────────────────────────────────────────────────
//  산술 연산자
// ──────────────────────────────────────────────────────────────────
Value Value::operator+(const Value& o) const {
    if (type==VType::STRING||o.type==VType::STRING) return Value::String(toString()+o.toString());
    if (isMatrix()&&o.isMatrix()&&mat&&o.mat) return Value::Matrix(std::make_shared<BluskMatrix>(mat->add(*o.mat)));
    if (type==VType::FLOAT||o.type==VType::FLOAT) return Value::Float(toDouble()+o.toDouble());
    if (isNum()&&o.isNum()) return Value::Int(toInt()+o.toInt());
    return Value::Nil();
}
Value Value::operator-(const Value& o) const {
    if (type==VType::FLOAT||o.type==VType::FLOAT) return Value::Float(toDouble()-o.toDouble());
    if (isNum()&&o.isNum()) return Value::Int(toInt()-o.toInt());
    return Value::Nil();
}
Value Value::operator*(const Value& o) const {
    // Matrix * Matrix = 행렬 곱
    if (isMatrix()&&o.isMatrix()&&mat&&o.mat) return Value::Matrix(std::make_shared<BluskMatrix>(mat->matmul(*o.mat)));
    if (type==VType::FLOAT||o.type==VType::FLOAT) return Value::Float(toDouble()*o.toDouble());
    if (isNum()&&o.isNum()) return Value::Int(toInt()*o.toInt());
    return Value::Nil();
}
Value Value::operator/(const Value& o) const {
    if (type==VType::FLOAT||o.type==VType::FLOAT) {
        double d=o.toDouble(); if(d==0.0){BluskError::report("Division by zero","runtime",0);return Value::Nil();}
        return Value::Float(toDouble()/d);
    }
    if (isNum()&&o.isNum()) {
        int64_t d=o.toInt(); if(d==0){BluskError::report("Division by zero","runtime",0);return Value::Nil();}
        return Value::Int(toInt()/d);
    }
    return Value::Nil();
}
Value Value::operator%(const Value& o) const {
    if (isNum()&&o.isNum()) {
        int64_t d=o.toInt(); if(d==0){BluskError::report("Modulo by zero","runtime",0);return Value::Nil();}
        return Value::Int(toInt()%d);
    }
    return Value::Nil();
}
Value Value::blusk_pow(const Value& o) const { return Value::Float(std::pow(toDouble(),o.toDouble())); }
Value Value::operator-() const {
    if(type==VType::INT)   return Value::Int(-num.i);
    if(type==VType::FLOAT) return Value::Float(-num.f);
    return Value::Nil();
}

// ──────────────────────────────────────────────────────────────────
//  Tensor 연산
// ──────────────────────────────────────────────────────────────────
Value Value::tensorAdd(const Value& o) const {
    if(!tensor||!o.tensor) return Value::Nil();
    size_t n=std::min(tensorSize,o.tensorSize);
    std::vector<double> res(n);
    for(size_t i=0;i<n;i++) res[i]=(*tensor)[i]+(*o.tensor)[i];
    return Value::Tensor(res);
}
Value Value::tensorMul(const Value& o) const {
    if(!tensor||!o.tensor) return Value::Nil();
    size_t n=std::min(tensorSize,o.tensorSize);
    std::vector<double> res(n);
    for(size_t i=0;i<n;i++) res[i]=(*tensor)[i]*(*o.tensor)[i];
    return Value::Tensor(res);
}
Value Value::tensorDot(const Value& o) const {
    if(!tensor||!o.tensor) return Value::Nil();
    size_t n=std::min(tensorSize,o.tensorSize);
    double s=0.0;
    for(size_t i=0;i<n;i++) s+=(*tensor)[i]*(*o.tensor)[i];
    return Value::Float(s);
}
Value Value::tensorSum() const {
    if(!tensor) return Value::Nil();
    double s=0.0; for(double v:*tensor) s+=v; return Value::Float(s);
}
Value Value::tensorMean() const {
    if(!tensor||tensorSize==0) return Value::Nil();
    return Value::Float(tensorSum().toDouble()/(double)tensorSize);
}
Value Value::tensorNorm() const {
    if(!tensor) return Value::Nil();
    double sq=0.0; for(double v:*tensor) sq+=v*v; return Value::Float(std::sqrt(sq));
}
Value Value::tensorStd() const {
    if(!tensor||tensorSize<2) return Value::Nil();
    double mean=tensorMean().toDouble(),var=0.0;
    for(double v:*tensor){double d=v-mean;var+=d*d;}
    return Value::Float(std::sqrt(var/(double)tensorSize));
}
Value Value::fmadd(const Value& a,const Value& b,const Value& c) {
    if(a.isTensor()&&b.isTensor()&&c.isTensor()) {
        size_t n=std::min({a.tensorSize,b.tensorSize,c.tensorSize});
        std::vector<double> res(n);
        for(size_t i=0;i<n;i++) res[i]=std::fma((*a.tensor)[i],(*b.tensor)[i],(*c.tensor)[i]);
        return Value::Tensor(res);
    }
    return Value::Float(std::fma(a.toDouble(),b.toDouble(),c.toDouble()));
}
Value Value::invsqrt(const Value& x) {
    double v=x.toDouble(); if(v<=0.0) return Value::Nil();
    return Value::Float(1.0/std::sqrt(v));
}

// ──────────────────────────────────────────────────────────────────
//  비교 연산자
// ──────────────────────────────────────────────────────────────────
bool Value::operator==(const Value& o) const {
    if(type!=o.type){if(isNum()&&o.isNum())return toDouble()==o.toDouble();return false;}
    switch(type){
    case VType::NIL:    return true;
    case VType::BOOL:   return num.b==o.num.b;
    case VType::INT:    return num.i==o.num.i;
    case VType::FLOAT:  return num.f==o.num.f;
    case VType::STRING: return str==o.str;
    case VType::OBJECT: return obj.get()==o.obj.get();
    default: return false;
    }
}
bool Value::operator<(const Value& o) const {
    if(isNum()&&o.isNum()){
        if(type==VType::FLOAT||o.type==VType::FLOAT) return toDouble()<o.toDouble();
        return toInt()<o.toInt();
    }
    if(type==VType::STRING&&o.type==VType::STRING) return str<o.str;
    return false;
}
