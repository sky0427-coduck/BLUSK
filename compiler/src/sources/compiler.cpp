// =============================================================
//  BLUSK compiler.cpp  -  PRINT_EXPR, loop(count,interval) patched
//  + deferred break/continue patching (fixes infinite loops)
// =============================================================
#include "../include/compiler.h"
#include "../include/error.h"
#include <cctype>
#include <sstream>

void Compiler::emit(OpCode op, uint8_t dst, uint8_t src1, uint8_t src2,
                    const std::string& str, int64_t iv, double fv) {
    bytecode.push_back({op,dst,src1,src2,str,iv,fv});
}

bool Compiler::isNumLit(const std::string& s) {
    if (s.empty()) return false;
    size_t i=(s[0]=='-')?1:0; bool dot=false;
    for(;i<s.size();i++){if(s[i]=='.'){{if(dot)return false;}dot=true;}else if(!std::isdigit(s[i]))return false;}
    return true;
}

static std::string kindTag(const std::string& typeKeyword) {
    if (typeKeyword=="int")    return "i32";
    if (typeKeyword=="long")   return "i64";
    if (typeKeyword=="float")  return "f32";
    if (typeKeyword=="double") return "f64";
    return "";
}

uint8_t Compiler::loadValue(const std::string& raw, int) {
    uint8_t r=ra.alloc();
    if(raw=="true")  {emit(OP_LOAD_BOOL,r,0,0,"",1);return r;}
    if(raw=="false") {emit(OP_LOAD_BOOL,r,0,0,"",0);return r;}
    if(raw=="nil"||raw=="null"){emit(OP_LOAD_NIL,r);return r;}
    if(isNumLit(raw)){
        if(raw.find('.')!=std::string::npos) emit(OP_LOAD_FLOAT,r,0,0,"",0,std::stod(raw));
        else                                  emit(OP_LOAD_INT,  r,0,0,"",std::stoll(raw));
        return r;
    }
    emit(OP_LOAD,r,0,0,raw); return r;
}

uint8_t Compiler::compileFString(const std::string& tmpl) {
    uint8_t r=ra.alloc(); emit(OP_LOAD_STR,r,0,0,tmpl); return r;
}

bool Compiler::tryInlineConst(const std::string& name, uint8_t dst) {
    auto it = constTable.find(name);
    if (it == constTable.end()) return false;
    const Value& v = it->second;
    if (v.isInt())   { emit(OP_LOAD_INT,  dst,0,0, v.isInt32()?"i32":"i64", v.num.i); return true; }
    if (v.isFloat()) { emit(OP_LOAD_FLOAT,dst,0,0, v.isFloat32()?"f32":"f64", 0, v.num.f); return true; }
    if (v.isString()){ emit(OP_LOAD_STR,  dst,0,0,v.str); return true; }
    if (v.isBool())  { emit(OP_LOAD_BOOL, dst,0,0,"",(int64_t)v.num.b); return true; }
    return false;
}

uint8_t Compiler::compileExpr(ASTNode* node) {
    if (!node) { uint8_t r=ra.alloc(); emit(OP_LOAD_NIL,r); return r; }
    const std::string& t=node->type, &v=node->value;

    if (t=="NUM_LIT") {
        uint8_t r=ra.alloc();
        if(v.find('.')!=std::string::npos) emit(OP_LOAD_FLOAT,r,0,0,"",0,std::stod(v));
        else                                emit(OP_LOAD_INT,  r,0,0,"",std::stoll(v));
        return r;
    }
    if(t=="STR_LIT") {uint8_t r=ra.alloc();emit(OP_LOAD_STR,r,0,0,v);return r;}
    if(t=="BOOL_LIT"){uint8_t r=ra.alloc();emit(OP_LOAD_BOOL,r,0,0,"",(v=="true"?1:0));return r;}
    if(t=="NIL_LIT") {uint8_t r=ra.alloc();emit(OP_LOAD_NIL,r);return r;}
    if(t=="FSTRING") {return compileFString(v);}

    if(t=="VAR_REF") {
        uint8_t r=ra.alloc();
        if (!tryInlineConst(v, r)) emit(OP_LOAD,r,0,0,v);
        return r;
    }

    if(t=="CAST") {
        uint8_t src=compileExpr(node->children[0]);
        uint8_t d=ra.alloc();
        emit(OP_CAST,d,src,0,v);
        return d;
    }

    if(t=="NEG")      {uint8_t s=compileExpr(node->children[0]);uint8_t d=ra.alloc();emit(OP_NEG,d,s);return d;}
    if(t=="LOGIC_NOT"){uint8_t s=compileExpr(node->children[0]);uint8_t d=ra.alloc();emit(OP_NOT,d,s);return d;}

    if(t=="LOGIC_AND") {
        uint8_t d=ra.alloc();
        uint8_t l=compileExpr(node->children[0]); emit(OP_MOVE,d,l);
        size_t sc=bytecode.size(); emit(OP_JUMP_IFNOT,0,d,0,"",0);
        uint8_t r=compileExpr(node->children[1]); emit(OP_AND,d,d,r);
        bytecode[sc].intVal=(int64_t)bytecode.size(); return d;
    }
    if(t=="LOGIC_OR") {
        uint8_t d=ra.alloc();
        uint8_t l=compileExpr(node->children[0]); emit(OP_MOVE,d,l);
        size_t sc=bytecode.size(); emit(OP_JUMP_IF,0,d,0,"",0);
        uint8_t r=compileExpr(node->children[1]); emit(OP_OR,d,d,r);
        bytecode[sc].intVal=(int64_t)bytecode.size(); return d;
    }

    if(t=="BIN_OP") {
        uint8_t l=compileExpr(node->children[0]);
        uint8_t r=compileExpr(node->children[1]);
        uint8_t d=ra.alloc();
        if     (v=="+")  emit(OP_ADD,  d,l,r); else if(v=="-")  emit(OP_SUB,  d,l,r);
        else if(v=="*")  emit(OP_MUL,  d,l,r); else if(v=="/")  emit(OP_DIV,  d,l,r);
        else if(v=="%")  emit(OP_MOD,  d,l,r); else if(v=="**") emit(OP_POW,  d,l,r);
        else if(v=="==") emit(OP_CMP_EQ,d,l,r);else if(v=="!=") emit(OP_CMP_NE,d,l,r);
        else if(v=="<")  emit(OP_CMP_LT,d,l,r);else if(v=="<=") emit(OP_CMP_LE,d,l,r);
        else if(v==">")  emit(OP_CMP_GT,d,l,r);else if(v==">=") emit(OP_CMP_GE,d,l,r);
        return d;
    }

    if(t=="MATH_CONST"){uint8_t r=ra.alloc();if(v=="PI")emit(OP_MATH_PI,r);else emit(OP_MATH_E,r);return r;}
    if(t=="MATH_CALL") {
        uint8_t arg=node->children.empty()?ra.alloc():compileExpr(node->children[0]);
        uint8_t d=ra.alloc();
        if     (v=="sqrt")  emit(OP_MATH_SQRT, d,arg); else if(v=="abs")   emit(OP_MATH_ABS,  d,arg);
        else if(v=="ceil")  emit(OP_MATH_CEIL, d,arg); else if(v=="floor") emit(OP_MATH_FLOOR,d,arg);
        else if(v=="round") emit(OP_MATH_ROUND,d,arg); else if(v=="sin")   emit(OP_MATH_SIN,  d,arg);
        else if(v=="cos")   emit(OP_MATH_COS,  d,arg); else if(v=="tan")   emit(OP_MATH_TAN,  d,arg);
        else if(v=="asin")  emit(OP_MATH_ASIN, d,arg); else if(v=="acos")  emit(OP_MATH_ACOS, d,arg);
        else if(v=="atan")  emit(OP_MATH_ATAN, d,arg); else if(v=="exp")   emit(OP_MATH_EXP,  d,arg);
        else if(v=="log")   emit(OP_MATH_LOG,  d,arg); else if(v=="log10") emit(OP_MATH_LOG10,d,arg);
        else{BluskError::report("Unknown Math: "+v,filename,node->line);emit(OP_LOAD_NIL,d);}
        return d;
    }
    if(t=="ARRAY_GET") {
        uint8_t idx;
        if(!node->children.empty()){
            ASTNode* c=node->children[0];
            if(c->type=="ARRAY_IDX"||c->type=="NUM_LIT"){
                idx=ra.alloc();
                if(isNumLit(c->value))emit(OP_LOAD_INT,idx,0,0,"",std::stoll(c->value));
                else emit(OP_LOAD,idx,0,0,c->value);
            }else idx=compileExpr(c);
        }else{idx=ra.alloc();emit(OP_LOAD_INT,idx,0,0,"",0);}
        uint8_t d=ra.alloc();emit(OP_ARRAY_GET,d,idx,0,v);return d;
    }
    if(t=="NEW_EXPR") {
        uint8_t first=ra.current();int argc=0;
        for(auto* a:node->children){
            uint8_t r=ra.alloc();
            if(a->type=="ARG"||a->type=="STRING_ARG")emit(OP_LOAD_STR,r,0,0,a->value);
            else if(isNumLit(a->value)){if(a->value.find('.')!=std::string::npos)emit(OP_LOAD_FLOAT,r,0,0,"",0,std::stod(a->value));else emit(OP_LOAD_INT,r,0,0,"",std::stoll(a->value));}
            else emit(OP_LOAD,r,0,0,a->value);argc++;
        }
        uint8_t d=ra.alloc();emit(OP_NEW,d,first,0,v,argc);return d;
    }
    if(t=="METHOD_CALL_EXPR"){
        size_t dot=v.find('.');std::string obj=v.substr(0,dot),mth=v.substr(dot+1);
        uint8_t first=ra.current();int argc=0;
        for(auto* a:node->children){uint8_t r=ra.alloc();emit(OP_LOAD,r,0,0,a->value);argc++;}
        uint8_t d=ra.alloc();emit(OP_CALL,d,first,0,obj+"."+mth,argc);return d;
    }
    if(t=="FIELD_GET"){uint8_t d=ra.alloc();emit(OP_GET_FIELD,d,0,0,v);return d;}
    return loadValue(v,node->line);
}

uint8_t Compiler::compileCond(ASTNode* cond) {
    if(!cond){uint8_t r=ra.alloc();emit(OP_LOAD_BOOL,r,0,0,"",0);return r;}
    if(cond->children.size()==1) return compileExpr(cond->children[0]);
    if(cond->children.size()>=3){
        uint8_t l=loadValue(cond->children[0]->value,cond->line);
        uint8_t r=loadValue(cond->children[2]->value,cond->line);
        uint8_t d=ra.alloc();
        const std::string& op=cond->children[1]->value;
        if(op=="==")emit(OP_CMP_EQ,d,l,r);else if(op=="!=")emit(OP_CMP_NE,d,l,r);
        else if(op=="<")emit(OP_CMP_LT,d,l,r);else if(op=="<=")emit(OP_CMP_LE,d,l,r);
        else if(op==">")emit(OP_CMP_GT,d,l,r);else if(op==">=")emit(OP_CMP_GE,d,l,r);
        else emit(OP_LOAD_BOOL,d,0,0,"",0);
        return d;
    }
    uint8_t r=ra.alloc();emit(OP_LOAD_BOOL,r,0,0,"",0);return r;
}

void Compiler::compileNode(ASTNode* node) {
    if(!node)return;
    const std::string& t=node->type, &v=node->value;

    if(t=="ROOT"||t=="MAIN_BLOCK"||t=="BLOCK")
        {for(auto* c:node->children)compileNode(c);return;}
    if(t=="IMPORT"){
        if(v=="blusk26")hasBlusk26=true; if(v=="io")hasIO=true;
        if(v=="oop")hasOOP=true;   if(v=="ai")hasAI=true;
        if(v=="time")hasTime=true; if(v=="Blusk.num.Math")hasBluskMath=true;
        if(v=="string")hasString=true; if(v=="collections")hasColl=true;
        return;
    }
    if(t=="ROOT_PACKAGE"||t=="CLASS_DECL"||t=="ANNOTATION"||
       t=="PARENT_CLASS"||t=="MEMBER_VAR") return;

    if(t=="THE_END")        {emit(OP_HALT);return;}
    if(t=="THE_END_IF")     {uint8_t mk=ra.current();uint8_t cr=compileCond(node->children.empty()?nullptr:node->children[0]);emit(OP_HALT_IF,0,cr);ra.resetTo(mk);return;}
    if(t=="THE_END_RETURN"||t=="RETURN"){uint8_t mk=ra.current();uint8_t vr=node->children.empty()?ra.alloc():compileExpr(node->children[0]);emit(OP_RETURN,0,vr);ra.resetTo(mk);return;}
    // break/continue compile directly to OP_JUMP using the enclosing
    // loop's LoopContext. Both are *deferred* patches: the JUMP target
    // isn't known yet at the point the statement is compiled (walking
    // the loop body, before the increment/exit point exist in the
    // bytecode), so a placeholder JUMP is emitted and its index recorded;
    // the loop's own compilation later patches it once the real target
    // (increment start for continue, past-loop-end for break) is known.
    if(t=="BREAK") {
        if (!loopStack.empty()) {
            size_t idx = bytecode.size();
            emit(OP_JUMP, 0, 0, 0, "", 0);
            loopStack.back().breakPatches.push_back(idx);
        } else {
            emit(OP_BREAK);
        }
        return;
    }
    if(t=="CONTINUE") {
        if (!loopStack.empty()) {
            size_t idx = bytecode.size();
            emit(OP_JUMP, 0, 0, 0, "", 0);
            loopStack.back().continuePatches.push_back(idx);
        } else {
            emit(OP_CONTINUE);
        }
        return;
    }

    if(t=="TASK_SLEEP"){
        uint8_t mk=ra.current();uint8_t r=ra.alloc();
        if(isNumLit(v))emit(OP_LOAD_INT,r,0,0,"",std::stoll(v));else emit(OP_LOAD,r,0,0,v);
        emit(OP_SLEEP,0,r);ra.resetTo(mk);return;
    }

    if(t=="PRINT"){
        if(!hasIO)BluskError::report("print requires 'import io'",filename,node->line);
        uint8_t mk=ra.current();
        if(node->children.empty()){uint8_t r=ra.alloc();emit(OP_LOAD_STR,r,0,0,v);emit(OP_PRINT,0,r);}
        else {
            ASTNode* arg=node->children[0];
            if(arg->type=="PRINT_EXPR") {
                // print(expr) -- variable, Matrix, call, etc. compiled directly
                uint8_t r = arg->children.empty() ? ra.alloc() : compileExpr(arg->children[0]);
                emit(OP_PRINT,0,r);
            } else {
                uint8_t fr=ra.alloc();emit(OP_LOAD_STR,fr,0,0,v);
                uint8_t ar;
                if(arg->type=="ARRAY_SIZE"){ar=ra.alloc();emit(OP_SIZE,ar,0,0,arg->value);}
                else if(arg->type=="ARRAY_GET")ar=compileExpr(arg);
                else{ar=ra.alloc();emit(OP_LOAD,ar,0,0,arg->value);}
                emit(OP_PRINT_FMT,0,fr,ar);
            }
        }
        ra.resetTo(mk);return;
    }
    if(t=="PRINT_FSTR"){
        if(!hasIO)BluskError::report("print requires 'import io'",filename,node->line);
        uint8_t mk=ra.current();uint8_t fr=compileFString(v);
        emit(OP_PRINT_FMT,0,fr,fr);ra.resetTo(mk);return;
    }

    if(t=="MATRIX_DECL") {
        if(node->children.empty())return;
        ASTNode* nm=node->children[0];
        const std::string& name=nm->value;
        if(isDead(name))return;
        uint8_t mk=ra.current();
        std::vector<std::vector<double>> rows;
        std::string op="*";
        for(auto* child:nm->children){
            if(child->type=="MATRIX_ROW"){
                std::vector<double> row;
                std::istringstream ss(child->value);
                std::string item;
                while(std::getline(ss,item,','))
                    try{row.push_back(std::stod(item));}catch(...){}
                rows.push_back(row);
            } else if(child->type=="MATRIX_OP") op=child->value;
        }
        if(rows.empty()){ra.resetTo(mk);return;}
        size_t numRows=rows.size();
        size_t numCols=rows.empty()?0:rows[0].size();
        uint8_t firstReg=ra.current();
        int totalElems=0;
        for(auto& row:rows) for(double val:row){
            uint8_t r=ra.alloc();
            emit(OP_LOAD_FLOAT,r,0,0,"",0,val);
            totalElems++;
        }
        uint8_t d=ra.alloc();
        std::string dims=std::to_string(numRows)+","+std::to_string(numCols)+","+op;
        emit(OP_MATRIX_NEW,d,firstReg,0,dims,totalElems);
        emit(OP_STORE,0,d,0,name,storeFlag(name,false));
        ra.resetTo(mk);return;
    }

    if(t=="SIMD_FOR"){
        uint8_t mk=ra.current();
        uint8_t vr=ra.alloc();emit(OP_LOAD,vr,0,0,v);
        emit(OP_SIMD_FOR,0,vr,0,v);
        ra.resetTo(mk);return;
    }

    if(t=="SWITCH") {
        uint8_t mk=ra.current();
        uint8_t switchReg=compileExpr(node->children[0]);
        std::vector<size_t> endJumps;
        size_t defaultIdx=std::string::npos;
        for(size_t ci=1;ci<node->children.size();ci++){
            ASTNode* c=node->children[ci];
            if(c->type=="CASE"){
                uint8_t caseReg=ra.alloc();
                if(isNumLit(c->value)) emit(OP_LOAD_INT,caseReg,0,0,"",std::stoll(c->value));
                else                    emit(OP_LOAD,caseReg,0,0,c->value);
                uint8_t cmpReg=ra.alloc();
                emit(OP_CMP_EQ,cmpReg,switchReg,caseReg);
                size_t skipIdx=bytecode.size();
                emit(OP_JUMP_IFNOT,0,cmpReg,0,"",0);
                if(!c->children.empty()) compileNode(c->children[0]);
                endJumps.push_back(bytecode.size());
                emit(OP_JUMP,0,0,0,"",0);
                bytecode[skipIdx].intVal=(int64_t)bytecode.size();
            } else if(c->type=="DEFAULT"){
                defaultIdx=ci;
            }
        }
        if(defaultIdx!=std::string::npos){
            ASTNode* d=node->children[defaultIdx];
            if(!d->children.empty()) compileNode(d->children[0]);
        }
        int64_t end=(int64_t)bytecode.size();
        for(size_t i:endJumps) bytecode[i].intVal=end;
        ra.resetTo(mk);return;
    }

    if(t=="FOR_IN"&&node->children.size()>=3){
        uint8_t mk=ra.current();
        const std::string& varNm=node->children[0]->value;
        const std::string& arrNm=node->children[1]->value;
        uint8_t iReg=ra.alloc(); emit(OP_LOAD_INT,iReg,0,0,"",0);
        emit(OP_STORE,0,iReg,0,"__forin_i__"+arrNm,STORE_RCSKIP);
        uint8_t lenReg=ra.alloc(); emit(OP_SIZE,lenReg,0,0,arrNm);
        emit(OP_STORE,0,lenReg,0,"__forin_len__"+arrNm,STORE_RCSKIP);
        size_t loopStart=bytecode.size();
        uint8_t ci=ra.alloc(); emit(OP_LOAD,ci,0,0,"__forin_i__"+arrNm);
        uint8_t cl=ra.alloc(); emit(OP_LOAD,cl,0,0,"__forin_len__"+arrNm);
        uint8_t cr=ra.alloc(); emit(OP_CMP_LT,cr,ci,cl);
        size_t jo=bytecode.size(); emit(OP_JUMP_IFNOT,0,cr,0,"",0);
        ra.resetTo(mk);
        uint8_t elemIdx=ra.alloc(); emit(OP_LOAD,elemIdx,0,0,"__forin_i__"+arrNm);
        uint8_t elemReg=ra.alloc(); emit(OP_ARRAY_GET,elemReg,elemIdx,0,arrNm);
        emit(OP_STORE,0,elemReg,0,varNm,STORE_RCSKIP);

        // continue jumps land here -- the increment's real start PC,
        // now knowable since the body has fully compiled.
        loopStack.push_back(LoopContext{});

        compileNode(node->children[2]);

        size_t incrementStart = bytecode.size();
        for (size_t patchIdx : loopStack.back().continuePatches)
            bytecode[patchIdx].intVal = (int64_t)incrementStart;

        uint8_t iCur=ra.alloc(); emit(OP_LOAD,iCur,0,0,"__forin_i__"+arrNm);
        uint8_t one=ra.alloc();  emit(OP_LOAD_INT,one,0,0,"",1);
        uint8_t iNew=ra.alloc(); emit(OP_ADD,iNew,iCur,one);
        emit(OP_STORE,0,iNew,0,"__forin_i__"+arrNm,STORE_RCSKIP);
        emit(OP_JUMP,0,0,0,"",(int64_t)loopStart);
        bytecode[jo].intVal=(int64_t)bytecode.size();

        for (size_t patchIdx : loopStack.back().breakPatches)
            bytecode[patchIdx].intVal = (int64_t)bytecode.size();
        loopStack.pop_back();

        ra.resetTo(mk);return;
    }

    if(t=="VAR_DECL"){
        if(node->children.empty())return;
        ASTNode* nm=node->children[0];
        const std::string& name=nm->value;
        bool isConst=(v=="num");
        if(isDead(name))return;
        uint8_t mk=ra.current();uint8_t vr;
        std::string tag = kindTag(v);

        if(nm->children.empty()){vr=ra.alloc();emit(OP_LOAD_NIL,vr);}
        else{
            ASTNode* rhs=nm->children[0];
            if(rhs->type=="VAR_VALUE_STR"){vr=ra.alloc();emit(OP_LOAD_STR,vr,0,0,rhs->value);}
            else if(!tag.empty() && rhs->type=="NUM_LIT") {
                vr=ra.alloc();
                bool isF = (tag=="f32"||tag=="f64");
                if (isF) emit(OP_LOAD_FLOAT, vr,0,0, tag, 0, std::stod(rhs->value));
                else      emit(OP_LOAD_INT,   vr,0,0, tag, std::stoll(rhs->value));
            }
            else vr=compileExpr(rhs);
        }
        // Auto-cast to the declared width if the initializer is a non-trivial
        // expression -- arithmetic promotion may have widened the result
        // (e.g. "int x = a + b;" where a/b are long would otherwise stay
        // 64-bit through the STORE).
        if (needsCastOnAssign(v) && !(nm->children.empty()) && nm->children[0]->type != "NUM_LIT") {
            uint8_t cast = ra.alloc();
            emit(OP_CAST, cast, vr, 0, v);
            vr = cast;
        }
        emit(OP_STORE,0,vr,0,name,storeFlag(name,isConst));
        ra.resetTo(mk);return;
    }

    if(t=="ASSIGN"){
        if(isDead(v))return;
        uint8_t mk=ra.current();
        uint8_t vr=node->children.empty()?ra.alloc():compileExpr(node->children[0]);
        // Auto-cast the arithmetic result back to the variable's declared
        // width on reassignment (e.g. "int x = x + 1;" stays 32-bit instead
        // of silently widening to long through operand promotion -- the
        // literal 1 has no type tag so it defaults to long, and int+long
        // promotes to long).
        std::string declType = declaredTypeOf(v);
        if (needsCastOnAssign(declType)) {
            uint8_t cast = ra.alloc();
            emit(OP_CAST, cast, vr, 0, declType);
            vr = cast;
        }
        emit(OP_STORE,0,vr,0,v,storeFlag(v,false));
        ra.resetTo(mk);return;
    }
    if(t=="ARRAY_DECL"){
        if(node->children.empty())return;
        ASTNode* nm=node->children[0];const std::string& name=nm->value;
        if(isDead(name))return;
        uint8_t mk=ra.current();uint8_t first=ra.current();int cnt=0;
        for(auto* el:nm->children){
            uint8_t r=ra.alloc();
            if(isNumLit(el->value)){if(el->value.find('.')!=std::string::npos)emit(OP_LOAD_FLOAT,r,0,0,"",0,std::stod(el->value));else emit(OP_LOAD_INT,r,0,0,"",std::stoll(el->value));}
            else emit(OP_LOAD_STR,r,0,0,el->value);cnt++;
        }
        emit(OP_ARRAY_NEW,0,first,0,name,cnt);ra.resetTo(mk);return;
    }
    if(t=="ARRAY_SET"){
        uint8_t mk=ra.current();
        uint8_t ir=loadValue(node->children[0]->value,node->line);
        uint8_t vr=node->children.size()>1?compileExpr(node->children[1]):ra.alloc();
        emit(OP_ARRAY_SET,0,ir,vr,v);ra.resetTo(mk);return;
    }
    if(t=="IO_READ"){
        std::string name=node->children.empty()?"":node->children[0]->value;
        if(isDead(name))return;uint8_t r=ra.alloc();
        emit(OP_READ,r,0,0,name);emit(OP_STORE,0,r,0,name,storeFlag(name,false));ra.resetTo(r);return;
    }
    if(t=="METHOD_CALL"){
        const std::string& meth=node->children[0]->value;
        uint8_t mk=ra.current();uint8_t first=ra.current();int argc=0;
        for(size_t i=1;i<node->children.size();i++){uint8_t r=ra.alloc();emit(OP_LOAD,r,0,0,node->children[i]->value);argc++;}
        uint8_t d=ra.alloc();emit(OP_CALL,d,first,0,v+"."+meth,argc);
        ra.resetTo(mk);return;
    }
    if(t=="IF"){
        uint8_t mk=ra.current();std::vector<size_t> ends;
        uint8_t cr=compileCond(node->children[0]);
        size_t ji=bytecode.size();emit(OP_JUMP_IFNOT,0,cr,0,"",0);
        ra.resetTo(mk);compileNode(node->children[1]);
        ends.push_back(bytecode.size());emit(OP_JUMP,0,0,0,"",0);
        bytecode[ji].intVal=(int64_t)bytecode.size();
        for(size_t ci=2;ci<node->children.size();ci++){
            ASTNode* br=node->children[ci];
            if(br->type=="ELSEIF"){
                uint8_t ec=compileCond(br->children[0]);size_t ej=bytecode.size();emit(OP_JUMP_IFNOT,0,ec,0,"",0);
                ra.resetTo(mk);compileNode(br->children[1]);ends.push_back(bytecode.size());emit(OP_JUMP,0,0,0,"",0);
                bytecode[ej].intVal=(int64_t)bytecode.size();
            }else if(br->type=="ELSE")compileNode(br->children[0]);
        }
        int64_t end=(int64_t)bytecode.size();for(size_t i:ends)bytecode[i].intVal=end;
        ra.resetTo(mk);return;
    }
    if(t=="FOR"&&node->children.size()>=4){
        uint8_t mk=ra.current();compileNode(node->children[0]);
        size_t ls=bytecode.size();
        uint8_t cr=compileCond(node->children[1]);size_t jo=bytecode.size();emit(OP_JUMP_IFNOT,0,cr,0,"",0);
        ra.resetTo(mk);

        loopStack.push_back(LoopContext{});

        compileNode(node->children[3]);

        // continue jumps land here -- the increment's real start PC,
        // now knowable since the body has fully compiled. Patching here
        // (rather than pre-computing before the body) is what makes this
        // correct: any "continue" compiled partway through the body had
        // no way to know this address yet, so it emitted a placeholder
        // JUMP now filled in.
        size_t incrementStart = bytecode.size();
        for (size_t patchIdx : loopStack.back().continuePatches)
            bytecode[patchIdx].intVal = (int64_t)incrementStart;

        ASTNode* step=node->children[2];
        uint8_t sv=ra.alloc();emit(OP_LOAD,sv,0,0,step->value);
        uint8_t one=ra.alloc();emit(OP_LOAD_INT,one,0,0,"",1);
        uint8_t nv=ra.alloc();
        if(step->line==-1)emit(OP_SUB,nv,sv,one);else emit(OP_ADD,nv,sv,one);
        emit(OP_STORE,0,nv,0,step->value,storeFlag(step->value,false));
        emit(OP_JUMP,0,0,0,"",(int64_t)ls);bytecode[jo].intVal=(int64_t)bytecode.size();

        for (size_t patchIdx : loopStack.back().breakPatches)
            bytecode[patchIdx].intVal = (int64_t)bytecode.size();
        loopStack.pop_back();

        ra.resetTo(mk);return;
    }
    if(t=="WHILE"){
        uint8_t mk=ra.current();size_t ls=bytecode.size();
        uint8_t cr=compileCond(node->children[0]);size_t jo=bytecode.size();emit(OP_JUMP_IFNOT,0,cr,0,"",0);
        ra.resetTo(mk);

        loopStack.push_back(LoopContext{});

        compileNode(node->children[1]);

        // continue jumps straight back to the condition recheck -- a
        // while loop has no separate increment step, so ls is the
        // correct continue target.
        for (size_t patchIdx : loopStack.back().continuePatches)
            bytecode[patchIdx].intVal = (int64_t)ls;

        emit(OP_JUMP,0,0,0,"",(int64_t)ls);bytecode[jo].intVal=(int64_t)bytecode.size();

        for (size_t patchIdx : loopStack.back().breakPatches)
            bytecode[patchIdx].intVal = (int64_t)bytecode.size();
        loopStack.pop_back();

        ra.resetTo(mk);return;
    }

    // ── LOOP : count-based repetition ─────────────────────────
    // loop(count, intervalSeconds) { ... }
    //   Repeats the body exactly `count` times. Sleeps `intervalSeconds`
    //   after each iteration (uses existing SLEEP/STORE/CMP/JUMP opcodes,
    //   no new opcode needed -- zero-cost reuse of the instruction set).
    if(t=="LOOP"){
        ASTNode *tot=nullptr,*ivl=nullptr,*blk=nullptr;
        for(auto* c:node->children){if(c->type=="LOOP_TOTAL")tot=c;else if(c->type=="LOOP_INTERVAL")ivl=c;else if(c->type=="BLOCK")blk=c;}

        if (tot) {
            uint8_t mk=ra.current();
            int64_t totalCount = 0;
            try { totalCount = std::stoll(tot->value); } catch(...) {}
            int64_t intervalMs = 0;
            if (ivl) { try { intervalMs = std::stoll(ivl->value) * 1000; } catch(...) {} }

            uint8_t iReg = ra.alloc();
            emit(OP_LOAD_INT, iReg, 0, 0, "", 0);
            emit(OP_STORE, 0, iReg, 0, "__loop_count_i__", STORE_RCSKIP);

            size_t loopStart = bytecode.size();

            uint8_t ci = ra.alloc(); emit(OP_LOAD, ci, 0, 0, "__loop_count_i__");
            uint8_t cn = ra.alloc(); emit(OP_LOAD_INT, cn, 0, 0, "", totalCount);
            uint8_t cr = ra.alloc(); emit(OP_CMP_LT, cr, ci, cn);
            size_t jo = bytecode.size(); emit(OP_JUMP_IFNOT, 0, cr, 0, "", 0);
            ra.resetTo(mk);

            loopStack.push_back(LoopContext{});

            if (blk) compileNode(blk); else if (!node->children.empty()) compileNode(node->children[0]);

            // continue jumps land here -- right where the per-iteration
            // sleep + increment begins, now that the body has compiled.
            size_t incrementStart = bytecode.size();
            for (size_t patchIdx : loopStack.back().continuePatches)
                bytecode[patchIdx].intVal = (int64_t)incrementStart;

            if (intervalMs > 0) {
                uint8_t sv = ra.alloc(); emit(OP_LOAD_INT, sv, 0, 0, "", intervalMs);
                emit(OP_SLEEP, 0, sv);
            }

            uint8_t iCur = ra.alloc(); emit(OP_LOAD, iCur, 0, 0, "__loop_count_i__");
            uint8_t one  = ra.alloc(); emit(OP_LOAD_INT, one, 0, 0, "", 1);
            uint8_t iNew = ra.alloc(); emit(OP_ADD, iNew, iCur, one);
            emit(OP_STORE, 0, iNew, 0, "__loop_count_i__", STORE_RCSKIP);

            emit(OP_JUMP, 0, 0, 0, "", (int64_t)loopStart);
            bytecode[jo].intVal = (int64_t)bytecode.size();

            for (size_t patchIdx : loopStack.back().breakPatches)
                bytecode[patchIdx].intVal = (int64_t)bytecode.size();
            loopStack.pop_back();

            ra.resetTo(mk);
            return;
        }

        // No count given -- fall back to unconditional repeat (legacy)
        loopStack.push_back(LoopContext{});
        size_t ls=bytecode.size();
        if(blk)compileNode(blk);else if(!node->children.empty())compileNode(node->children[0]);
        // continue in an unconditional loop just jumps back to the top.
        for (size_t patchIdx : loopStack.back().continuePatches)
            bytecode[patchIdx].intVal = (int64_t)ls;
        emit(OP_JUMP,0,0,0,"",(int64_t)ls);
        for (size_t patchIdx : loopStack.back().breakPatches)
            bytecode[patchIdx].intVal = (int64_t)bytecode.size();
        loopStack.pop_back();
        return;
    }

    for(auto* c:node->children)compileNode(c);
}

std::vector<Instruction> Compiler::compile(ASTNode* root,const std::string& fn){
    bytecode.clear();ra.reset();constTable.clear();
    hasBlusk26=hasIO=hasOOP=hasTime=hasAI=hasBluskMath=hasString=hasColl=false;
    filename=fn;
    for(auto* c:root->children)if(c&&c->type=="IMPORT"){
        auto& vv=c->value;
        if(vv=="blusk26")hasBlusk26=true; if(vv=="io")hasIO=true;
        if(vv=="oop")hasOOP=true;   if(vv=="ai")hasAI=true;
        if(vv=="time")hasTime=true; if(vv=="Blusk.num.Math")hasBluskMath=true;
        if(vv=="string")hasString=true; if(vv=="collections")hasColl=true;
    }
    if(!hasBlusk26){BluskError::fatal("import blusk26 is required!");return bytecode;}
    for(auto* c:root->children)if(c&&c->type=="CLASS_DECL")classTable[c->value]=c;
    for(auto* c:root->children)
        if(c&&c->type=="MAIN_BLOCK"&&c->value=="entry")
            {compileNode(c);emit(OP_HALT);return bytecode;}
    for(auto* c:root->children)
        if(c&&c->type=="ANNOTATION"&&!c->children.empty()&&c->children[0]->type=="MAIN_BLOCK")
            {compileNode(c->children[0]);emit(OP_HALT);return bytecode;}
    for(auto* c:root->children)
        if(c&&c->type=="MAIN_BLOCK")
            {compileNode(c);emit(OP_HALT);return bytecode;}
    compileNode(root);emit(OP_HALT);return bytecode;
}

std::unordered_map<std::string,ASTNode*>& Compiler::getClassTable(){return classTable;}
