// =============================================================
//  BLUSK vm.cpp  -  Matrix + 실제 AVX2 SIMD + OP_CAST + switch
// =============================================================
#include "../include/vm.h"
#include "../include/checker.h"
#include "../include/module.h"
#include "../include/gc.h"
#include "../include/jit.h"
#include "../include/error.h"
#include <iostream>
#include <sstream>
#include <cmath>
#include <chrono>
#include <thread>
#include <fstream>
#include <cstring>

// ── AVX2 / SSE 헤더 ──────────────────────────────────────────────
#if defined(__AVX2__) || defined(__AVX__)
#  include <immintrin.h>
#  define BLUSK_AVX2 1
#elif defined(_M_X64) || defined(_M_IX86)
#  include <intrin.h>
   // MSVC: /arch:AVX2 플래그 필요
#  ifdef __AVX2__
#    define BLUSK_AVX2 1
#  endif
#endif

static double nowMs() {
    using namespace std::chrono;
    return (double)duration_cast<microseconds>(
        steady_clock::now().time_since_epoch()).count() / 1000.0;
}

// ──────────────────────────────────────────────────────────────────
//  실제 AVX2 행렬 곱 (zero-cost SIMD)
//  C = A * B  (rows_A x cols_A) × (rows_B x cols_B)
// ──────────────────────────────────────────────────────────────────
static void matmul_avx2(const double* A, const double* B, double* C,
                         size_t rowsA, size_t colsA, size_t colsB)
{
    std::memset(C, 0, rowsA * colsB * sizeof(double));

#if defined(BLUSK_AVX2)
    for (size_t i = 0; i < rowsA; i++) {
        for (size_t k = 0; k < colsA; k++) {
            __m256d a_ik = _mm256_set1_pd(A[i*colsA+k]);
            size_t j = 0;
            for (; j + 4 <= colsB; j += 4) {
                __m256d b = _mm256_loadu_pd(&B[k*colsB+j]);
                __m256d c = _mm256_loadu_pd(&C[i*colsB+j]);
                c = _mm256_fmadd_pd(a_ik, b, c);   // FMA: a*b + c
                _mm256_storeu_pd(&C[i*colsB+j], c);
            }
            for (; j < colsB; j++)
                C[i*colsB+j] += A[i*colsA+k] * B[k*colsB+j];
        }
    }
#else
    for (size_t i=0;i<rowsA;i++)
        for (size_t k=0;k<colsA;k++)
            for (size_t j=0;j<colsB;j++)
                C[i*colsB+j]+=A[i*colsA+k]*B[k*colsB+j];
#endif
}

// ── element-wise 연산 (AVX2) ──────────────────────────────────────
static void elmul_avx2(const double* A, const double* B, double* C, size_t n) {
#if defined(BLUSK_AVX2)
    size_t i = 0;
    for (; i+4<=n; i+=4) {
        __m256d va=_mm256_loadu_pd(&A[i]);
        __m256d vb=_mm256_loadu_pd(&B[i]);
        _mm256_storeu_pd(&C[i], _mm256_mul_pd(va,vb));
    }
    for (; i<n; i++) C[i]=A[i]*B[i];
#else
    for (size_t i=0;i<n;i++) C[i]=A[i]*B[i];
#endif
}

static void eladd_avx2(const double* A, const double* B, double* C, size_t n) {
#if defined(BLUSK_AVX2)
    size_t i=0;
    for (;i+4<=n;i+=4)
        _mm256_storeu_pd(&C[i],_mm256_add_pd(_mm256_loadu_pd(&A[i]),_mm256_loadu_pd(&B[i])));
    for (;i<n;i++) C[i]=A[i]+B[i];
#else
    for (size_t i=0;i<n;i++) C[i]=A[i]+B[i];
#endif
}

// ── @simd for: 행렬/배열 SIMD 재계산 ────────────────────────────
static void simdForMatrix(Value& val) {
    if (!val.isMatrix() || !val.mat) return;
    // 이미 연산 결과가 mat에 있으므로 추가 재계산 불필요
    // (향후: lazy evaluation 후 여기서 실제 계산 트리거)
    // 지금은 AVX2로 내부 정규화 1회 수행 예시
#if defined(BLUSK_AVX2)
    auto& m = *val.mat;
    size_t n = m.data.size();
    // no-op on already computed matrix (placeholder for future lazy eval)
    (void)n;
#endif
}

static void simdForArray(Value& val) {
    if (!val.isArr() || !val.arr) return;
    // 배열 원소를 double 버퍼로 올려 AVX2 처리 가능하게 준비
    // (향후 tensor 변환 트리거)
}

// ──────────────────────────────────────────────────────────────────
//  SVM 메서드들
// ──────────────────────────────────────────────────────────────────
void SVM::registerClass(ASTNode* n) { classes[n->value]=n; }

std::string SVM::getParentClass(const std::string& cn) {
    if (!classes.count(cn)) return "";
    for (auto* c:classes[cn]->children) if (c->type=="PARENT_CLASS") return c->value;
    return "";
}

ASTNode* SVM::findMethod(const std::string& cn, const std::string& mn) {
    if (!classes.count(cn)) return nullptr;
    for (auto* c:classes[cn]->children)
        if (c->type=="METHOD"&&!c->children.empty()&&c->children[0]->value==mn) return c;
    std::string p=getParentClass(cn);
    return p.empty()?nullptr:findMethod(p,mn);
}

static Value resolveVal(const std::string& name,
                         std::unordered_map<std::string,Value>& loc,
                         BluskObject& obj,
                         std::unordered_map<std::string,Value>& globals)
{
    auto it=loc.find(name);   if(it!=loc.end())   return it->second;
    auto i2=obj.fields.find(name); if(i2!=obj.fields.end()) return i2->second;
    auto i3=globals.find(name);    if(i3!=globals.end())    return i3->second;
    if(name.empty()) return Value::Nil();
    bool dot=false,num=true; size_t s=(name[0]=='-')?1:0;
    for(size_t i=s;i<name.size();i++){
        if(name[i]=='.'){if(dot){num=false;break;}dot=true;}
        else if(!std::isdigit(name[i])){num=false;break;}
    }
    if(num&&s<name.size()) return dot?Value::Float(std::stod(name)):Value::Int(std::stoll(name));
    return Value::String(name);
}

static bool evalCond(ASTNode* c,
                     std::unordered_map<std::string,Value>& loc,
                     BluskObject& obj,
                     std::unordered_map<std::string,Value>& g)
{
    if(!c||c->children.size()<3) return false;
    Value l=resolveVal(c->children[0]->value,loc,obj,g);
    Value r=resolveVal(c->children[2]->value,loc,obj,g);
    const std::string& op=c->children[1]->value;
    if(op=="==")return l==r; if(op=="!=")return l!=r;
    if(op=="<") return l<r;  if(op=="<=")return l<=r;
    if(op==">") return l>r;  if(op==">=")return l>=r;
    return false;
}

void SVM::runBlock(ASTNode* body,BluskObject& obj,std::unordered_map<std::string,Value>& local) {
    if(!body||body->type!="BLOCK") return;
    for(auto* s:body->children){
        if(!s) continue;
        if(s->type=="PRINT"){
            if(s->children.empty()) std::cout<<s->value<<"\n";
            else {
                Value v=resolveVal(s->children[0]->value,local,obj,globals);
                std::string fmt=s->value,res;
                for(size_t i=0;i<fmt.size();i++){
                    if(fmt[i]=='%'&&i+1<fmt.size()&&(fmt[i+1]=='d'||fmt[i+1]=='s'))
                    {res+=v.toString();i++;}else res+=fmt[i];
                }
                std::cout<<res<<"\n";
            }
        }
        else if(s->type=="VAR_DECL"&&!s->children.empty()){
            auto* nn=s->children[0];
            if(!nn->children.empty()){Value val=resolveVal(nn->children[0]->value,local,obj,globals);local[nn->value]=val;obj.fields[nn->value]=val;}
        }
        else if(s->type=="ASSIGN"){
            Value val=resolveVal(s->children[0]->value,local,obj,globals);
            local[s->value]=val;obj.fields[s->value]=val;
        }
        else if(s->type=="IF"){
            bool done=false;
            if(s->children.size()>=2&&evalCond(s->children[0],local,obj,globals))
                {runBlock(s->children[1],obj,local);done=true;}
            if(!done)for(size_t ci=2;ci<s->children.size();ci++){
                auto* b=s->children[ci];
                if(b->type=="ELSEIF"&&evalCond(b->children[0],local,obj,globals))
                    {runBlock(b->children[1],obj,local);done=true;break;}
                else if(b->type=="ELSE"){runBlock(b->children[0],obj,local);break;}
            }
        }
        else if(s->type=="WHILE"){
            while(s->children.size()>=2&&evalCond(s->children[0],local,obj,globals))
                runBlock(s->children[1],obj,local);
        }
        else if(s->type=="FOR"&&s->children.size()>=4){
            auto* init=s->children[0];
            if(!init->children.empty()){auto* nn=init->children[0];
                if(!nn->children.empty())local[nn->value]=resolveVal(nn->children[0]->value,local,obj,globals);}
            auto* step=s->children[2];
            while(evalCond(s->children[1],local,obj,globals)){
                runBlock(s->children[3],obj,local);
                Value sv=resolveVal(step->value,local,obj,globals);
                Value nv=(step->line==-1)?sv-Value::Int(1):sv+Value::Int(1);
                local[step->value]=nv;obj.fields[step->value]=nv;
            }
        }
        else if(s->type=="TASK_SLEEP"){
            int64_t ms=0;try{ms=std::stoll(s->value)*1000;}catch(...){}
            std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        }
    }
}

void SVM::runConstructor(const std::string& cn,BluskObject& obj,const std::vector<Value>& args){
    if(!classes.count(cn)) return;
    std::string p=getParentClass(cn); if(!p.empty())runConstructor(p,obj,{});
    for(auto* c:classes[cn]->children) if(c->type=="CONSTRUCTOR"){
        std::unordered_map<std::string,Value> local; int idx=0;
        for(auto* param:c->children)
            if(param->type=="PARAM"&&idx<(int)args.size())
                for(auto* pc:param->children)
                    if(pc->type=="PARAM_NAME"){local[pc->value]=args[idx++];break;}
        for(auto* bc:c->children) if(bc->type=="BLOCK")runBlock(bc,obj,local);
        break;
    }
}

void SVM::callMethod(const std::string& on,const std::string& mn,const std::vector<Value>& args,uint8_t dst){
    if(!objects.count(on)){BluskError::report("Undefined object '"+on+"'","runtime",0);return;}
    auto& obj=*objects[on];
    auto* m=findMethod(obj.className,mn);
    if(!m){BluskError::report("Method '"+mn+"' not found","runtime",0);return;}
    std::unordered_map<std::string,Value> local; int idx=0;
    for(auto* c:m->children)
        if(c->type=="PARAM"&&idx<(int)args.size())
            for(auto* pc:c->children)
                if(pc->type=="PARAM_NAME"){local[pc->value]=args[idx++];break;}
    runBlock(m->children.back(),obj,local);
}

std::string SVM::resolveFmtString(const std::string& tmpl){
    std::string res;
    for(size_t i=0;i<tmpl.size();i++){
        if(tmpl[i]=='{'){
            size_t e=tmpl.find('}',i+1);
            if(e==std::string::npos){res+='{';continue;}
            std::string vn=tmpl.substr(i+1,e-i-1);
            res+=globals.count(vn)?globals[vn].toString():vn;
            i=e;
        }else res+=tmpl[i];
    }
    return res;
}

// ──────────────────────────────────────────────────────────────────
//  SVM 메인 실행 루프
// ──────────────────────────────────────────────────────────────────
void SVM::run(const std::vector<Instruction>& bc){
    size_t pc=0; const size_t sz=bc.size();
    double loopStart=0,loopLimit=0; bool timedLoop=false;

    while(pc<sz){
        const Instruction& I=bc[pc++];
        if(I.op==OP_JUMP&&(size_t)I.intVal<pc)
            JITCompiler::instance().onBackEdge((size_t)I.intVal);

        switch(I.op){
        // ── 로드 ─────────────────────────────────────────────────
        case OP_LOAD_INT:   regs[I.dst]=Value::Int(I.intVal);     break;
        case OP_LOAD_FLOAT: regs[I.dst]=Value::Float(I.floatVal); break;
        case OP_LOAD_STR:   regs[I.dst]=Value::String(I.strVal);  break;
        case OP_LOAD_BOOL:  regs[I.dst]=Value::Bool(I.intVal!=0); break;
        case OP_LOAD_NIL:   regs[I.dst]=Value::Nil();             break;
        case OP_MOVE:       regs[I.dst]=regs[I.src1];             break;

        // ── 변수 ─────────────────────────────────────────────────
        case OP_STORE: {
            bool isC=(I.intVal==STORE_CONST||I.intVal==STORE_CONST_RC);
            bool rcS=(I.intVal==STORE_RCSKIP||I.intVal==STORE_CONST_RC);
            if(isC&&constants.count(I.strVal))
                {BluskError::report("Cannot reassign constant '"+I.strVal+"'","runtime",0);break;}
            Value val=regs[I.src1];
            if(rcS){val.rcSkip=true;if(val.isObj()&&val.obj)val.obj->rcSkip=true;}
            if(!rcS&&val.isObj()&&val.obj) GC_TRACK(val.obj.get());
            globals[I.strVal]=val;
            if(isC) constants[I.strVal]=true;
            break;
        }
        case OP_LOAD: {
            if(globals.count(I.strVal)) {regs[I.dst]=globals[I.strVal];break;}
            if(objects.count(I.strVal)){regs[I.dst]=Value::Object(objects[I.strVal]);break;}
            BluskError::report("Undefined variable '"+I.strVal+"'","runtime",0);
            regs[I.dst]=Value::Nil();break;
        }

        // ── 산술 ─────────────────────────────────────────────────
        case OP_ADD: regs[I.dst]=regs[I.src1]+regs[I.src2]; break;
        case OP_SUB: regs[I.dst]=regs[I.src1]-regs[I.src2]; break;
        case OP_MUL: regs[I.dst]=regs[I.src1]*regs[I.src2]; break;
        case OP_DIV: regs[I.dst]=regs[I.src1]/regs[I.src2]; break;
        case OP_MOD: regs[I.dst]=regs[I.src1]%regs[I.src2]; break;
        case OP_POW: regs[I.dst]=regs[I.src1].blusk_pow(regs[I.src2]); break;
        case OP_NEG: regs[I.dst]=-regs[I.src1]; break;

        // ── 비교 ─────────────────────────────────────────────────
        case OP_CMP_EQ: regs[I.dst]=Value::Bool(regs[I.src1]==regs[I.src2]); break;
        case OP_CMP_NE: regs[I.dst]=Value::Bool(regs[I.src1]!=regs[I.src2]); break;
        case OP_CMP_LT: regs[I.dst]=Value::Bool(regs[I.src1]< regs[I.src2]); break;
        case OP_CMP_LE: regs[I.dst]=Value::Bool(regs[I.src1]<=regs[I.src2]); break;
        case OP_CMP_GT: regs[I.dst]=Value::Bool(regs[I.src1]> regs[I.src2]); break;
        case OP_CMP_GE: regs[I.dst]=Value::Bool(regs[I.src1]>=regs[I.src2]); break;

        // ── 논리 ─────────────────────────────────────────────────
        case OP_AND: regs[I.dst]=Value::Bool(regs[I.src1].toBool()&&regs[I.src2].toBool()); break;
        case OP_OR:  regs[I.dst]=Value::Bool(regs[I.src1].toBool()||regs[I.src2].toBool()); break;
        case OP_NOT: regs[I.dst]=Value::Bool(!regs[I.src1].toBool()); break;

        // ── 제어 ─────────────────────────────────────────────────
        case OP_JUMP:       pc=(size_t)I.intVal; break;
        case OP_JUMP_IF:    if( regs[I.src1].toBool()) pc=(size_t)I.intVal; break;
        case OP_JUMP_IFNOT: if(!regs[I.src1].toBool()) pc=(size_t)I.intVal; break;
        case OP_BREAK:
            for(size_t j=pc;j<sz;j++) if(bc[j].op==OP_JUMP){pc=j+1;break;}
            timedLoop=false; break;
        case OP_CONTINUE:
            for(size_t j=pc;j<sz;j++) if(bc[j].op==OP_JUMP){pc=(size_t)bc[j].intVal;break;}
            break;
        case OP_RETURN:  return;
        case OP_HALT_IF: if(regs[I.src1].toBool()) return; break;

        // ── as 캐스팅 ─────────────────────────────────────────────
        case OP_CAST:
            regs[I.dst] = regs[I.src1].castTo(I.strVal);
            break;

        // ── I/O ──────────────────────────────────────────────────
        case OP_PRINT: std::cout<<regs[I.src1].toString()<<"\n"; break;
        case OP_PRINT_FMT: {
            if(I.src1==I.src2){std::cout<<resolveFmtString(regs[I.src1].toString())<<"\n";break;}
            std::string fmt=regs[I.src1].toString(),val=regs[I.src2].toString();
            if(fmt.find('{')!=std::string::npos){std::cout<<resolveFmtString(fmt)<<"\n";break;}
            std::string res;
            for(size_t i=0;i<fmt.size();i++){
                if(fmt[i]=='%'&&i+1<fmt.size()&&(fmt[i+1]=='d'||fmt[i+1]=='s'))
                {res+=val;i++;}else res+=fmt[i];
            }
            std::cout<<res<<"\n";break;
        }
        case OP_READ: {
            std::string in;std::getline(std::cin,in);
            regs[I.dst]=Value::String(in);
            if(!I.strVal.empty()) globals[I.strVal]=regs[I.dst];break;
        }

        // ── OOP ──────────────────────────────────────────────────
        case OP_NEW: {
            auto obj=std::make_shared<BluskObject>();obj->className=I.strVal;
            std::vector<Value> args;
            for(int j=0;j<(int)I.intVal;j++) args.push_back(regs[I.src1+j]);
            runConstructor(I.strVal,*obj,args); GC_TRACK(obj.get());
            regs[I.dst]=Value::Object(obj);break;
        }
        case OP_CALL: {
            size_t dot=I.strVal.find('.');if(dot==std::string::npos)break;
            std::string on=I.strVal.substr(0,dot),mn=I.strVal.substr(dot+1);
            std::vector<Value> args;
            for(int j=0;j<(int)I.intVal;j++) args.push_back(regs[I.src1+j]);
            JITCompiler::instance().onFuncCall(mn);
            if(!objects.count(on)&&globals.count(on)&&globals[on].isObj()) objects[on]=globals[on].obj;
            callMethod(on,mn,args,I.dst);break;
        }
        case OP_SET_FIELD:{
            size_t d=I.strVal.find('.');std::string on=I.strVal.substr(0,d),fn=I.strVal.substr(d+1);
            if(objects.count(on))objects[on]->fields[fn]=regs[I.src1];break;
        }
        case OP_GET_FIELD:{
            size_t d=I.strVal.find('.');std::string on=I.strVal.substr(0,d),fn=I.strVal.substr(d+1);
            regs[I.dst]=(objects.count(on)&&objects[on]->fields.count(fn))?objects[on]->fields[fn]:Value::Nil();break;
        }

        // ── 배열 ─────────────────────────────────────────────────
        case OP_ARRAY_NEW:
            arrays[I.strVal].clear();
            for(int j=0;j<(int)I.intVal;j++) arrays[I.strVal].push_back(regs[I.src1+j]);break;
        case OP_ARRAY_GET:{
            int64_t idx=regs[I.src1].toInt();
            if(!arrays.count(I.strVal)){BluskError::report("Undefined array '"+I.strVal+"'","runtime",0);regs[I.dst]=Value::Nil();break;}
            auto& arr=arrays[I.strVal];
            if(idx<0||idx>=(int64_t)arr.size()){BluskError::report("Array index out of bounds","runtime",0);regs[I.dst]=Value::Nil();break;}
            regs[I.dst]=arr[(size_t)idx];break;
        }
        case OP_ARRAY_SET:{
            int64_t idx=regs[I.src1].toInt();
            if(!arrays.count(I.strVal)||idx<0||idx>=(int64_t)arrays[I.strVal].size())
                {BluskError::report("Array index out of bounds","runtime",0);break;}
            arrays[I.strVal][(size_t)idx]=regs[I.src2];break;
        }
        case OP_SIZE:
            regs[I.dst]=arrays.count(I.strVal)?Value::Int((int64_t)arrays[I.strVal].size()):Value::Int(0);break;

        // ── Math ─────────────────────────────────────────────────
        case OP_MATH_PI: regs[I.dst]=Value::Float(3.14159265358979323846);break;
        case OP_MATH_E:  regs[I.dst]=Value::Float(2.71828182845904523536);break;
        #define M1(op,fn) case op: regs[I.dst]=Value::Float(fn(regs[I.src1].toDouble()));break;
        M1(OP_MATH_SQRT,std::sqrt) M1(OP_MATH_SIN,std::sin)   M1(OP_MATH_COS,std::cos)
        M1(OP_MATH_TAN,std::tan)   M1(OP_MATH_ASIN,std::asin) M1(OP_MATH_ACOS,std::acos)
        M1(OP_MATH_ATAN,std::atan) M1(OP_MATH_EXP,std::exp)   M1(OP_MATH_LOG,std::log)
        M1(OP_MATH_LOG10,std::log10)
        #undef M1
        case OP_MATH_ABS:{
            Value v=regs[I.src1];
            regs[I.dst]=v.isFloat()?Value::Float(std::fabs(v.num.f)):Value::Int(std::abs(v.num.i));break;
        }
        case OP_MATH_CEIL:  regs[I.dst]=Value::Int((int64_t)std::ceil (regs[I.src1].toDouble()));break;
        case OP_MATH_FLOOR: regs[I.dst]=Value::Int((int64_t)std::floor(regs[I.src1].toDouble()));break;
        case OP_MATH_ROUND: regs[I.dst]=Value::Int((int64_t)std::round(regs[I.src1].toDouble()));break;

        // ── Matrix (실제 AVX2 SIMD) ───────────────────────────────
        case OP_MATRIX_NEW: {
            // strVal = "rows,cols,op"  intVal = 총 원소 수  src1 = 첫 원소 reg
            std::string dimsStr=I.strVal;
            size_t c1=dimsStr.find(','), c2=dimsStr.find(',',c1+1);
            size_t rows=std::stoull(dimsStr.substr(0,c1));
            size_t cols=std::stoull(dimsStr.substr(c1+1,c2-c1-1));
            std::string op=dimsStr.substr(c2+1);

            int totalElems=(int)I.intVal;
            int elemsPerRow=(rows>0)?(int)(totalElems/rows):totalElems;

            // 행 분리: src1 ~ src1+totalElems-1 레지스터에서 원소 수집
            std::vector<std::vector<double>> rowVecs;
            for(size_t r=0;r<rows;r++){
                std::vector<double> row;
                for(int e=0;e<elemsPerRow;e++)
                    row.push_back(regs[I.src1+r*elemsPerRow+e].toDouble());
                rowVecs.push_back(row);
            }

            // op에 따라 행렬 연산 수행 (AVX2 사용)
            auto result=std::make_shared<BluskMatrix>(rows,cols);
            if(rows==2 && op=="*") {
                // 두 행 벡터 element-wise 곱 (Hadamard) → 1×cols 결과
                if(rowVecs[0].size()==rowVecs[1].size()) {
                    size_t n=rowVecs[0].size();
                    result=std::make_shared<BluskMatrix>(1,n);
                    elmul_avx2(rowVecs[0].data(),rowVecs[1].data(),result->data.data(),n);
                }
            } else if(rows==2 && op=="+") {
                size_t n=rowVecs[0].size();
                result=std::make_shared<BluskMatrix>(1,n);
                eladd_avx2(rowVecs[0].data(),rowVecs[1].data(),result->data.data(),n);
            } else {
                // 단순 행렬 구성 (연산 없음)
                for(size_t r=0;r<rows;r++)
                    for(size_t c=0;c<cols&&c<rowVecs[r].size();c++)
                        result->at(r,c)=rowVecs[r][c];
            }
            regs[I.dst]=Value::Matrix(result);
            break;
        }

        case OP_MATRIX_MUL: {
            // reg[dst] = reg[src1] @ reg[src2]
            if(!regs[I.src1].isMatrix()||!regs[I.src2].isMatrix())
                {BluskError::report("Matrix mul: both operands must be Matrix","runtime",0);regs[I.dst]=Value::Nil();break;}
            auto& A=*regs[I.src1].mat;
            auto& B=*regs[I.src2].mat;
            if(A.cols!=B.rows){BluskError::report("Matrix dim mismatch","runtime",0);regs[I.dst]=Value::Nil();break;}
            auto C=std::make_shared<BluskMatrix>(A.rows,B.cols);
            matmul_avx2(A.data.data(),B.data.data(),C->data.data(),A.rows,A.cols,B.cols);
            regs[I.dst]=Value::Matrix(C);break;
        }

        case OP_MATRIX_ADD: {
            if(!regs[I.src1].isMatrix()||!regs[I.src2].isMatrix())
                {regs[I.dst]=Value::Nil();break;}
            auto& A=*regs[I.src1].mat; auto& B=*regs[I.src2].mat;
            auto C=std::make_shared<BluskMatrix>(A.rows,A.cols);
            eladd_avx2(A.data.data(),B.data.data(),C->data.data(),A.data.size());
            regs[I.dst]=Value::Matrix(C);break;
        }

        case OP_MATRIX_ELMUL: {
            if(!regs[I.src1].isMatrix()||!regs[I.src2].isMatrix())
                {regs[I.dst]=Value::Nil();break;}
            auto& A=*regs[I.src1].mat; auto& B=*regs[I.src2].mat;
            auto C=std::make_shared<BluskMatrix>(A.rows,A.cols);
            elmul_avx2(A.data.data(),B.data.data(),C->data.data(),A.data.size());
            regs[I.dst]=Value::Matrix(C);break;
        }

        case OP_MATRIX_T: {
            if(!regs[I.src1].isMatrix()){regs[I.dst]=Value::Nil();break;}
            regs[I.dst]=Value::Matrix(std::make_shared<BluskMatrix>(regs[I.src1].mat->transpose()));break;
        }

        case OP_MATRIX_PRINT: {
            if(regs[I.src1].isMatrix()&&regs[I.src1].mat)
                std::cout<<regs[I.src1].mat->toString()<<"\n";
            else std::cout<<regs[I.src1].toString()<<"\n";
            break;
        }

        // ── @simd for varname ─────────────────────────────────────
        case OP_SIMD_FOR: {
            // src1 = 변수 레지스터
            Value& val=regs[I.src1];
            if(val.isMatrix()) simdForMatrix(val);
            else if(val.isArr()) simdForArray(val);
            // globals에 반영
            if(!I.strVal.empty()) globals[I.strVal]=val;
            break;
        }

        // ── NativeCall ────────────────────────────────────────────
        case OP_NATIVE_CALL: {
            auto* fn=ModuleLoader(filename).resolve(I.strVal);
            if(fn){std::vector<Value> a;for(int j=0;j<(int)I.intVal;j++)a.push_back(regs[I.src1+j]);regs[I.dst]=(*fn)(a);}
            else  {BluskError::report("Native fn not found: "+I.strVal,"runtime",0);regs[I.dst]=Value::Nil();}
            break;
        }

        // ── 시간 ─────────────────────────────────────────────────
        case OP_SLEEP:
            std::this_thread::sleep_for(std::chrono::milliseconds(regs[I.src1].toInt()));break;
        case OP_PERF_NOW:
            if(I.strVal=="__loop_start__"){loopStart=nowMs();loopLimit=(double)I.intVal;timedLoop=true;regs[I.dst]=Value::Float(loopStart);}
            else regs[I.dst]=Value::Float(nowMs());
            break;

        // ── Saturday AI ──────────────────────────────────────────
        case OP_AI_LOAD:  satLoad(I.strVal,regs[I.src1].toString());break;
        case OP_AI_ASK:   regs[I.dst]=Value::String(satAsk(I.strVal,regs[I.src1].toString(),(float)I.floatVal,(int)I.intVal));break;
        case OP_AI_LEARN: satLearn(I.strVal,regs[I.src1].toString());break;
        case OP_AI_SAVE:  satSave(I.strVal,regs[I.src1].toString());break;
        case OP_AI_STATUS:
            if(!aiModels.count(I.strVal)){std::cout<<"[Saturday] '"<<I.strVal<<"' not loaded\n";regs[I.dst]=Value::String("not_loaded");}
            else{auto& m=aiModels[I.strVal];std::cout<<"[Saturday] "<<m.name<<" v"<<m.version<<" loaded="<<(m.loaded?"yes":"no")<<"\n";regs[I.dst]=Value::String("ok");}
            break;

        case OP_HALT: return;
        default: break;
        }
        if(timedLoop&&nowMs()-loopStart>=loopLimit){timedLoop=false;return;}
    }
}

// Saturday AI 구현 (기존 동일)
static std::string dS(double v){std::ostringstream o;o<<v;return o.str();}

void SVM::satLoad(const std::string& vn,const std::string& path){
    SaturdayModel m;m.name=vn;m.modelPath=path;
    if(path.size()>4&&path.substr(0,4)=="api:"){
        std::string rest=path.substr(4);size_t c=rest.find(':');
        if(c==std::string::npos){std::cout<<"[Saturday] Error: api:provider:model\n";return;}
        m.apiProvider=rest.substr(0,c);m.apiModel=rest.substr(c+1);
        if(m.apiProvider=="openai"){m.mode=MODE_API_OPENAI;const char*k=std::getenv("OPENAI_API_KEY");m.apiKey=k?k:"";}
        else if(m.apiProvider=="anthropic"){m.mode=MODE_API_ANTHROPIC;const char*k=std::getenv("ANTHROPIC_API_KEY");m.apiKey=k?k:"";}
        m.loaded=true;
    }else if(path.size()>5&&path.substr(path.size()-5)==".gguf"){
#if defined(LLAMA_EMBED)
        m.mode=MODE_LLAMA;llama_backend_init();
        llama_model_params mp=llama_model_default_params();
        m.llamaModel=llama_load_model_from_file(path.c_str(),mp);
        if(!m.llamaModel){std::cout<<"[Saturday] Error: cannot load gguf\n";return;}
        llama_context_params cp=llama_context_default_params();cp.n_ctx=2048;
        m.llamaCtx=llama_new_context_with_model(m.llamaModel,cp);m.loaded=true;
#else
        m.mode=MODE_STUB;m.loaded=true;
#endif
    }else{
        m.mode=MODE_STUB;
        std::ifstream f(path);
        if(f.good()){std::string ln;
            if(std::getline(f,ln))try{m.version=std::stoi(ln);}catch(...){}
            if(std::getline(f,ln))try{m.defaultTemperature=std::stof(ln);}catch(...){}
            if(std::getline(f,ln))try{m.defaultSentences=std::stoi(ln);}catch(...){}
        }
        m.loaded=true;
    }
    std::cout<<"[Saturday] Loaded '"<<vn<<"'\n";
    aiModels[vn]=m;globals[vn]=Value::String("__saturday__"+vn);
}

std::string SVM::satAsk(const std::string& vn,const std::string& prompt,float temp,int sents){
    if(!aiModels.count(vn)||!aiModels[vn].loaded) return "[error: not loaded]";
    auto& m=aiModels[vn];
    float t=(temp<=0.f)?m.defaultTemperature:temp;int s=(sents<=0)?m.defaultSentences:sents;
    m.memory.push_back("user: "+prompt);if(m.memory.size()>20)m.memory.erase(m.memory.begin());
    std::string res;
    switch(m.mode){
        case MODE_LLAMA:res=satAskLlama(m,prompt,t,s);break;
        case MODE_API_OPENAI:case MODE_API_ANTHROPIC:res=satAskAPI(m,prompt,t,s);break;
        default:res=satAskStub(m,prompt,t,s);break;
    }
    m.memory.push_back("saturday: "+res);std::cout<<res<<"\n";return res;
}
std::string SVM::satAskStub(SaturdayModel& m,const std::string& p,float t,int s){
    return "[Saturday-stub v"+std::to_string(m.version)+"] temp="+dS(t)+" sents="+std::to_string(s)+" | "+p;
}
std::string SVM::satAskLlama(SaturdayModel& m,const std::string& p,float t,int s){
#if defined(LLAMA_EMBED)
    if(!m.llamaModel||!m.llamaCtx)return "[error: llama not init]";
    std::vector<llama_token>toks(512);
    int n=llama_tokenize(m.llamaModel,p.c_str(),(int)p.size(),toks.data(),(int)toks.size(),true,false);
    if(n<0)return "[error: tokenize]";toks.resize(n);
    llama_batch batch=llama_batch_get_one(toks.data(),n);
    if(llama_decode(m.llamaCtx,batch))return "[error: decode]";
    auto* samp=llama_sampler_chain_init(llama_sampler_chain_default_params());
    llama_sampler_chain_add(samp,llama_sampler_init_temp(t));
    llama_sampler_chain_add(samp,llama_sampler_init_dist(0));
    std::string res;
    for(int k=0;k<s*50;k++){
        llama_token tok=llama_sampler_sample(samp,m.llamaCtx,-1);
        if(tok==llama_token_eos(m.llamaModel))break;
        char buf[64]={};llama_token_to_piece(m.llamaModel,tok,buf,sizeof(buf),0,false);
        res+=buf;llama_batch next=llama_batch_get_one(&tok,1);llama_decode(m.llamaCtx,next);
    }
    llama_sampler_free(samp);return res;
#else
    return "[error: no LLAMA_EMBED]";
#endif
}
std::string SVM::satAskAPI(SaturdayModel& m,const std::string& p,float t,int s){
    if(m.apiKey.empty())return "[error: API key not set ("+m.apiProvider+")]";
#if defined(SATURDAY_HTTP)
    std::string host,path,body;httplib::Headers hdrs;
    if(m.mode==MODE_API_OPENAI){
        host="api.openai.com";path="/v1/chat/completions";
        body=R"({"model":")" +m.apiModel+R"(","max_tokens":)" +std::to_string(s*80)+R"(,"temperature":)" +dS(t)+R"(,"messages":[{"role":"user","content":")" +p+R"("}]})";
        hdrs={{"Authorization","Bearer "+m.apiKey},{"Content-Type","application/json"}};
    }else{
        host="api.anthropic.com";path="/v1/messages";
        body=R"({"model":")" +m.apiModel+R"(","max_tokens":)" +std::to_string(s*80)+R"(,"messages":[{"role":"user","content":")" +p+R"("}]})";
        hdrs={{"x-api-key",m.apiKey},{"anthropic-version","2023-06-01"},{"Content-Type","application/json"}};
    }
    httplib::SSLClient cli(host);cli.set_connection_timeout(10);cli.set_read_timeout(30);
    auto res=cli.Post(path,hdrs,body,"application/json");
    if(!res)return "[error: HTTP failed]";
    if(res->status!=200)return "[error: HTTP "+std::to_string(res->status)+"]";
    const std::string& rb=res->body;
    size_t p2=rb.find(m.mode==MODE_API_OPENAI?"\"content\":":"\"text\":");
    if(p2==std::string::npos)return rb;
    p2=rb.find('"',p2+10)+1;return rb.substr(p2,rb.find('"',p2)-p2);
#else
    return "[error: no httplib]";
#endif
}
void SVM::satLearn(const std::string& vn,const std::string& path){
    if(!aiModels.count(vn)||!aiModels[vn].loaded){std::cout<<"[Saturday] Not loaded\n";return;}
    std::ifstream f(path);if(!f.good()){std::cout<<"[Saturday] Data not found: "<<path<<"\n";return;}
    aiModels[vn].version++;std::cout<<"[Saturday] Learned v"<<aiModels[vn].version<<"\n";
}
void SVM::satSave(const std::string& vn,const std::string& path){
    if(!aiModels.count(vn)||!aiModels[vn].loaded){std::cout<<"[Saturday] Not loaded\n";return;}
    auto& m=aiModels[vn];std::ofstream f(path);
    if(!f.is_open()){std::cout<<"[Saturday] Cannot write\n";return;}
    f<<m.version<<"\n"<<m.defaultTemperature<<"\n"<<m.defaultSentences<<"\n";
    for(auto& mem:m.memory)f<<mem<<"\n";
    std::cout<<"[Saturday] Saved v"<<m.version<<"\n";
}
