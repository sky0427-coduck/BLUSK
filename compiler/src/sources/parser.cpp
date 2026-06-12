// =============================================================
//  BLUSK parser.cpp  -  Matrix / switch-case / as / simd for
// =============================================================
#include "../include/parser.h"
#include "../include/error.h"
#include <algorithm>
#include <stdexcept>

Parser::Parser(std::vector<Token>& tks) : tokens(tks), pos(0) {}

Token Parser::peek(int o) const {
    size_t idx=pos+(size_t)o;
    return idx<tokens.size()?tokens[idx]:Token{TOKEN_EOF,"",0};
}
bool  Parser::check(TokenType t,int o)      const { return peek(o).type==t; }
bool  Parser::checkVal(const std::string& v,int o)const{return peek(o).value==v;}
Token Parser::advance(){Token t=peek();if(pos<tokens.size())pos++;return t;}
bool  Parser::eat(TokenType t)            {if(check(t)){advance();return true;}return false;}
bool  Parser::eatVal(const std::string& v){if(checkVal(v)){advance();return true;}return false;}
void  Parser::skipSemicolon()             {eatVal(";");}

bool Parser::isTypeKeyword(const Token& t) const {
    return t.type==TOKEN_KW_GG||t.type==TOKEN_KW_NUM||
           t.value=="int"||t.value=="str"||t.value=="float"||
           t.value=="double"||t.value=="bool"||t.value=="string";
}
bool Parser::isCompOp(const Token& t) const {
    return t.value=="=="||t.value=="!="||t.value=="<"||
           t.value=="<="||t.value==">"||t.value==">=";
}

// ──────────────────────────────────────────────────────────────────
//  행 벡터 파싱: (1.0, 2.0, 3.0, 4.0)
// ──────────────────────────────────────────────────────────────────
std::vector<double> Parser::parseRowVector() {
    std::vector<double> row;
    eatVal("(");
    while (!checkVal(")") && peek().type!=TOKEN_EOF) {
        bool neg=false;
        if (checkVal("-")) { neg=true; advance(); }
        if (peek().type==TOKEN_NUMBER) {
            try { row.push_back((neg?-1:1)*std::stod(peek().value)); }
            catch(...) { row.push_back(0.0); }
            advance();
        }
        eatVal(",");
    }
    eatVal(")");
    return row;
}

// ──────────────────────────────────────────────────────────────────
//  최상위
// ──────────────────────────────────────────────────────────────────
ASTNode* Parser::parse() {
    ASTNode* root=new ASTNode(); root->type="ROOT";
    while(peek().type!=TOKEN_EOF) {
        ASTNode* n=parseTopLevel(); if(n) root->children.push_back(n);
    }
    return root;
}

ASTNode* Parser::parseTopLevel() {
    Token t=peek();
    if(t.type==TOKEN_KW_ROOT)       return parseRootPackage();
    if(t.type==TOKEN_KW_IMPORT)     return parseImport();
    if(t.type==TOKEN_KW_ANNOTATION) return parseAnnotation();
    if(t.type==TOKEN_KW_BLUSK)      { advance(); if(checkVal("public"))advance(); if(checkVal("class"))return parseClass(); return parseBluskMain("default"); }
    return parseStatement();
}

ASTNode* Parser::parseRootPackage() {
    ASTNode* n=new ASTNode(); n->type="ROOT_PACKAGE"; n->line=peek().line;
    advance(); if(peek().type==TOKEN_KW_PACKAGE) advance();
    std::string pkg;
    while(peek().type==TOKEN_IDENTIFIER||(peek().type==TOKEN_SYMBOL&&peek().value=="."))
        {pkg+=peek().value;advance();}
    n->value=pkg; skipSemicolon(); return n;
}

ASTNode* Parser::parseImport() {
    ASTNode* n=new ASTNode(); n->type="IMPORT"; n->line=peek().line;
    advance();
    std::string name;
    while(peek().type!=TOKEN_EOF&&!checkVal(";")) {
        std::string v=peek().value;
        if(!v.empty()&&std::isupper(v[0])&&v!="Blusk"&&v!="Math") v[0]=std::tolower(v[0]);
        name+=v; advance();
    }
    skipSemicolon();
    if(!name.empty()&&name.back()=='*'){name.pop_back();if(!name.empty()&&name.back()=='.')name.pop_back();}
    if(name=="blusk26"||name.find("blusk.26")!=std::string::npos)    name="blusk26";
    else if(name.find("Blusk.num.Math")!=std::string::npos)           name="Blusk.num.Math";
    else if(name=="math"||name.find("blusk.math")!=std::string::npos) name="Blusk.num.Math";
    else if(name=="io")     name="io";
    else if(name=="oop")    name="oop";
    else if(name=="ai")     name="ai";
    else if(name=="time")   name="time";
    else if(name=="string") name="string";
    else if(name=="collections") name="collections";
    n->value=name; return n;
}

ASTNode* Parser::parseAnnotation() {
    std::string annot=peek().value; int line=peek().line; advance();
    std::transform(annot.begin(),annot.end(),annot.begin(),::tolower);

    // @simd for varname;
    if (annot=="simd") {
        if (peek().type==TOKEN_KW_FOR||checkVal("for")) {
            advance(); // for
            return parseSimdFor();
        }
    }

    if(annot=="entry") {
        eatVal("Blusk"); if(checkVal("public"))advance();
        if(checkVal("class")) return parseClass();
        return parseBluskMain("entry");
    }
    ASTNode* an=new ASTNode(); an->type="ANNOTATION"; an->value=annot; an->line=line;
    if(peek().type==TOKEN_KW_BLUSK){
        advance(); if(checkVal("public"))advance();
        ASTNode* d=checkVal("class")?parseClass():parseBluskMain(annot);
        an->children.push_back(d); return an;
    }
    if(peek().type==TOKEN_KW_GG||peek().type==TOKEN_KW_NUM||isTypeKeyword(peek()))
        { ASTNode* d=parseStatement(); if(d) an->children.push_back(d); return an; }
    return an;
}

// @simd for varname;
ASTNode* Parser::parseSimdFor() {
    ASTNode* n=new ASTNode(); n->type="SIMD_FOR"; n->line=peek().line;
    n->value=peek().value; // 변수명 (행렬/배열 이름)
    advance(); skipSemicolon();
    return n;
}

ASTNode* Parser::parseBluskMain(const std::string& annotation) {
    ASTNode* n=new ASTNode(); n->type="MAIN_BLOCK"; n->value=annotation; n->line=peek().line;
    if(!checkVal("{")&&peek().type!=TOKEN_EOF) {
        advance(); // retType
        if(peek().type==TOKEN_IDENTIFIER||peek().value=="main")
            {n->value=(annotation.empty()||annotation=="default")?peek().value:annotation;advance();}
        if(checkVal("(")) {
            int depth=0;
            while(peek().type!=TOKEN_EOF) {
                if(checkVal("("))depth++;
                if(checkVal(")")){depth--;advance();if(depth<=0)break;else continue;}
                advance();
            }
        }
    }
    n->children.push_back(parseBlock()); return n;
}

ASTNode* Parser::parseClass() {
    ASTNode* n=new ASTNode(); n->type="CLASS_DECL"; n->line=peek().line;
    advance(); n->value=peek().value; advance();
    if(checkVal(":")){advance();if(peek().type==TOKEN_KW_FORKFROM||checkVal("forkfrom"))
        {advance();ASTNode* p=new ASTNode();p->type="PARENT_CLASS";p->value=peek().value;advance();n->children.push_back(p);}}
    if(peek().type==TOKEN_KW_FORKFROM||checkVal("forkfrom"))
        {advance();ASTNode* p=new ASTNode();p->type="PARENT_CLASS";p->value=peek().value;advance();n->children.push_back(p);}
    eatVal("{");
    while(peek().type!=TOKEN_EOF&&!checkVal("}")) {
        if(peek().type==TOKEN_KW_ANNOTATION){ASTNode* a=new ASTNode();a->type="ANNOTATION";a->value=peek().value;advance();n->children.push_back(a);continue;}
        if(peek().type==TOKEN_KW_BLUSK){advance();eatVal("public");}
        Token cur=peek();
        if(isTypeKeyword(cur)&&peek(1).type==TOKEN_IDENTIFIER&&(peek(2).value==";"||peek(2).value=="=")) {
            ASTNode* m=new ASTNode();m->type="MEMBER_VAR";m->value=cur.value;advance();
            ASTNode* mn=new ASTNode();mn->type="VAR_NAME";mn->value=peek().value;advance();
            if(checkVal("=")){advance();ASTNode* iv=new ASTNode();iv->type="VAR_VALUE";iv->value=peek().value;advance();mn->children.push_back(iv);}
            skipSemicolon();m->children.push_back(mn);n->children.push_back(m);continue;
        }
        if(cur.type==TOKEN_IDENTIFIER&&cur.value==n->value){n->children.push_back(parseConstructor(n->value));continue;}
        if(cur.type==TOKEN_IDENTIFIER||isTypeKeyword(cur)||cur.type==TOKEN_KW_VOID)
            {std::string rt=cur.value;advance();if(peek().type==TOKEN_IDENTIFIER)n->children.push_back(parseMethod(rt));continue;}
        advance();
    }
    eatVal("}"); return n;
}

ASTNode* Parser::parseConstructor(const std::string& cn) {
    ASTNode* n=new ASTNode();n->type="CONSTRUCTOR";n->value=cn;n->line=peek().line;
    advance();parseParamList(n);n->children.push_back(parseBlock());return n;
}
ASTNode* Parser::parseMethod(const std::string& rt) {
    ASTNode* n=new ASTNode();n->type="METHOD";n->value=rt;n->line=peek().line;
    ASTNode* mn=new ASTNode();mn->type="METHOD_NAME";mn->value=peek().value;advance();
    n->children.push_back(mn);parseParamList(n);n->children.push_back(parseBlock());return n;
}
void Parser::parseParamList(ASTNode* n) {
    eatVal("(");
    while(!checkVal(")")&&peek().type!=TOKEN_EOF) {
        if(isTypeKeyword(peek())||peek().type==TOKEN_IDENTIFIER) {
            ASTNode* p=new ASTNode();p->type="PARAM";p->value=peek().value;advance();
            if(peek().type==TOKEN_IDENTIFIER){ASTNode* pn=new ASTNode();pn->type="PARAM_NAME";pn->value=peek().value;advance();p->children.push_back(pn);}
            n->children.push_back(p);
        }
        if(!eatVal(",")) break;
    }
    eatVal(")");
}
ASTNode* Parser::parseBlock() {
    ASTNode* n=new ASTNode();n->type="BLOCK";n->line=peek().line;eatVal("{");
    while(peek().type!=TOKEN_EOF&&!checkVal("}")){ASTNode* s=parseStatement();if(s)n->children.push_back(s);}
    eatVal("}");return n;
}

// ──────────────────────────────────────────────────────────────────
//  문장
// ──────────────────────────────────────────────────────────────────
ASTNode* Parser::parseStatement() {
    Token t=peek();
    if(t.value==";")            {advance();return nullptr;}
    if(t.type==TOKEN_KW_ANNOTATION) return parseAnnotation();
    if(t.type==TOKEN_KW_ROOT)   return parseRootPackage();
    if(t.type==TOKEN_KW_IMPORT) return parseImport();
    if(t.type==TOKEN_KW_BLUSK)  {advance();if(checkVal("public"))advance();if(checkVal("class"))return parseClass();return parseBluskMain("default");}
    if(t.type==TOKEN_KW_THE_END)return parseTheEnd();
    if(t.value=="print")        return parsePrint();
    if(t.value=="switch")       return parseSwitch();
    if(t.type==TOKEN_KW_IF)     return parseIf();
    if(t.type==TOKEN_KW_FOR)    return parseFor();
    if(t.type==TOKEN_KW_WHILE)  return parseWhile();
    if(t.type==TOKEN_KW_LOOP)   return parseLoop();
    if(t.type==TOKEN_KW_BREAK)  {advance();skipSemicolon();ASTNode* n=new ASTNode();n->type="BREAK";return n;}
    if(t.type==TOKEN_KW_CONTINUE){advance();skipSemicolon();ASTNode* n=new ASTNode();n->type="CONTINUE";return n;}
    if(t.type==TOKEN_KW_RETURN) return parseReturn();

    // Matrix 선언
    if(t.type==TOKEN_KW_MATRIX) return parseMatrixDecl();

    if(t.type==TOKEN_KW_GG||t.type==TOKEN_KW_NUM)
        {return peek(1).value=="["?parseArrayDecl(t.value):parseVarDecl(t.value);}
    if(isTypeKeyword(t))
        {return peek(1).value=="["?parseArrayDecl(t.value):parseVarDecl(t.value);}
    if(t.value=="task"&&peek(1).value=="."&&peek(2).value=="sleep") return parseTaskSleep();
    if(t.value=="io"&&peek(1).value=="."&&peek(2).value=="read") {advance();advance();return parseIORead();}

    if(t.type==TOKEN_IDENTIFIER) {
        std::string name=t.value;
        if(peek(1).value=="[") {
            advance();advance();
            ASTNode* idx=parseExpr();eatVal("]");
            if(checkVal("=")) {
                advance();ASTNode* n=new ASTNode();n->type="ARRAY_SET";n->value=name;n->line=t.line;
                n->children.push_back(idx);n->children.push_back(parseExpr());skipSemicolon();return n;
            }
            ASTNode* n=new ASTNode();n->type="ARRAY_GET";n->value=name;n->line=t.line;
            n->children.push_back(idx);skipSemicolon();return n;
        }
        if(peek(1).value==".") {
            std::string method=peek(2).value;
            if(method=="load"||method=="ask"||method=="think"||method=="learn"||method=="save"||method=="status")
                {advance();advance();advance();return parseAICall(name,method);}
            return parseMethodCall(name);
        }
        if(peek(1).value=="=") {
            advance();advance();
            ASTNode* n=new ASTNode();n->type="ASSIGN";n->value=name;n->line=t.line;
            n->children.push_back(parseExpr());skipSemicolon();return n;
        }
        if(peek(1).type==TOKEN_IDENTIFIER) return parseVarDecl(name);
        advance();skipSemicolon();return nullptr;
    }
    advance();return nullptr;
}

// ──────────────────────────────────────────────────────────────────
//  Matrix mat = {(1,2,3,4) * (2,3,7,8)};
//
//  AST: MATRIX_DECL
//         VAR_NAME "mat"
//           MATRIX_ROW [1,2,3,4]  (value = "1,2,3,4")
//           MATRIX_OP  "*"
//           MATRIX_ROW [2,3,7,8]
// ──────────────────────────────────────────────────────────────────
ASTNode* Parser::parseMatrixDecl() {
    ASTNode* n=new ASTNode(); n->type="MATRIX_DECL"; n->line=peek().line;
    advance(); // Matrix
    ASTNode* nm=new ASTNode(); nm->type="VAR_NAME"; nm->value=peek().value; advance();
    if(checkVal("=")) {
        advance(); eatVal("{");
        // 첫 번째 행 벡터
        while(!checkVal("}")&&peek().type!=TOKEN_EOF) {
            if(checkVal("(")) {
                auto row=parseRowVector();
                ASTNode* rn=new ASTNode(); rn->type="MATRIX_ROW";
                std::string rv;
                for(size_t i=0;i<row.size();i++){if(i)rv+=",";rv+=std::to_string(row[i]);}
                rn->value=rv; nm->children.push_back(rn);
            } else if(checkVal("*")||checkVal("+")||checkVal("-")) {
                ASTNode* op=new ASTNode(); op->type="MATRIX_OP"; op->value=peek().value;
                advance(); nm->children.push_back(op);
            } else advance();
        }
        eatVal("}");
    }
    n->children.push_back(nm); skipSemicolon(); return n;
}

// ──────────────────────────────────────────────────────────────────
//  switch(a) { case 0 -> expr, case 1 -> expr, default -> expr; }
// ──────────────────────────────────────────────────────────────────
ASTNode* Parser::parseSwitch() {
    ASTNode* n=new ASTNode(); n->type="SWITCH"; n->line=peek().line;
    advance(); // switch
    eatVal("(");
    ASTNode* expr=parseExpr(); eatVal(")");
    n->children.push_back(expr);
    eatVal("{");
    while(!checkVal("}")&&peek().type!=TOKEN_EOF) {
        if(checkVal("case")) {
            advance();
            ASTNode* c=new ASTNode(); c->type="CASE"; c->line=peek().line;
            // case 값
            c->value=peek().value; advance();
            eatVal("->"); // 화살표
            // 케이스 본체: 표현식 or 블록
            if(checkVal("{")) c->children.push_back(parseBlock());
            else              c->children.push_back(parseExpr());
            n->children.push_back(c);
            eatVal(",");
        } else if(checkVal("default")) {
            advance();
            ASTNode* d=new ASTNode(); d->type="DEFAULT"; d->line=peek().line;
            eatVal("->");
            if(checkVal("{")) d->children.push_back(parseBlock());
            else              d->children.push_back(parseExpr());
            n->children.push_back(d);
            eatVal(","); eatVal(";");
        } else advance();
    }
    eatVal("}"); skipSemicolon(); return n;
}

ASTNode* Parser::parseVarDecl(const std::string& kw) {
    ASTNode* n=new ASTNode();n->type="VAR_DECL";n->value=kw;n->line=peek().line;advance();
    ASTNode* nm=new ASTNode();nm->type="VAR_NAME";nm->value=peek().value;advance();
    if(checkVal("=")) {
        advance();
        if(checkVal("new")){
            advance();ASTNode* ne=new ASTNode();ne->type="NEW_EXPR";ne->value=peek().value;advance();
            eatVal("(");
            while(!checkVal(")")&&peek().type!=TOKEN_EOF){
                ASTNode* a;
                if(peek().type==TOKEN_FSTRING){a=new ASTNode();a->type="FSTRING_ARG";a->value=peek().value;advance();}
                else if(peek().type==TOKEN_STRING){a=new ASTNode();a->type="STRING_ARG";a->value=peek().value;advance();}
                else a=parseExpr();
                ne->children.push_back(a);if(!eatVal(","))break;
            }
            eatVal(")");nm->children.push_back(ne);
        } else if(peek().type==TOKEN_FSTRING) {nm->children.push_back(parseFString(peek().value));advance();}
        else if(peek().type==TOKEN_STRING) {ASTNode* v=new ASTNode();v->type="VAR_VALUE_STR";v->value=peek().value;advance();nm->children.push_back(v);}
        else nm->children.push_back(parseExpr());
    }
    n->children.push_back(nm);skipSemicolon();return n;
}

ASTNode* Parser::parseArrayDecl(const std::string& kw) {
    ASTNode* n=new ASTNode();n->type="ARRAY_DECL";n->value=kw;n->line=peek().line;
    advance();eatVal("[");eatVal("]");
    ASTNode* nm=new ASTNode();nm->type="VAR_NAME";nm->value=peek().value;advance();
    if(checkVal("=")) {
        advance();eatVal("{");
        while(!checkVal("}")&&peek().type!=TOKEN_EOF) {
            ASTNode* el;
            if(peek().type==TOKEN_STRING||peek().type==TOKEN_FSTRING)
                {el=new ASTNode();el->type="ARRAY_ELEM";el->value=peek().value;advance();}
            else {
                ASTNode* expr=parseExpr();
                el=new ASTNode();el->type=expr->type;el->value=expr->value;
                for(auto* c:expr->children)el->children.push_back(c);
                expr->children.clear();delete expr;
            }
            nm->children.push_back(el);eatVal(",");
        }
        eatVal("}");
    }
    n->children.push_back(nm);skipSemicolon();return n;
}

ASTNode* Parser::parsePrint() {
    ASTNode* n=new ASTNode();n->line=peek().line;advance();eatVal("(");
    if(peek().type==TOKEN_FSTRING){n->type="PRINT_FSTR";n->value=peek().value;advance();eatVal(")");skipSemicolon();return n;}
    n->type="PRINT";
    if(peek().type==TOKEN_STRING){n->value=peek().value;advance();}
    if(checkVal(",")) {
        advance();Token at=peek();ASTNode* a=new ASTNode();a->line=at.line;
        std::string fn=at.value;advance();
        if(checkVal(".")){advance();a->type="ARRAY_SIZE";a->value=peek().value;advance();}
        else if(checkVal("[")) {
            advance();ASTNode* ix=new ASTNode();ix->type="ARRAY_IDX";ix->value=peek().value;advance();eatVal("]");
            a->type="ARRAY_GET";a->value=fn;a->children.push_back(ix);
        } else {a->type="PRINT_ARG";a->value=fn;}
        n->children.push_back(a);
    }
    eatVal(")");skipSemicolon();return n;
}

ASTNode* Parser::parseTheEnd() {
    ASTNode* n=new ASTNode();n->line=peek().line;advance();eatVal("end");
    if(checkVal(":")) {
        advance();
        if(peek().type==TOKEN_KW_IF||checkVal("if")){advance();n->type="THE_END_IF";n->children.push_back(parseCondition());skipSemicolon();return n;}
        if(checkVal("return")){advance();n->type="THE_END_RETURN";n->children.push_back(parseExpr());skipSemicolon();return n;}
    }
    n->type="THE_END";skipSemicolon();return n;
}

ASTNode* Parser::parseReturn() {
    ASTNode* n=new ASTNode();n->type="RETURN";n->line=peek().line;advance();
    if(!checkVal(";")&&peek().type!=TOKEN_EOF&&!checkVal("}"))n->children.push_back(parseExpr());
    skipSemicolon();return n;
}

ASTNode* Parser::parseCondition() {
    eatVal("(");
    ASTNode* expr=parseExpr();
    eatVal(")");
    ASTNode* cond=new ASTNode();cond->type="CONDITION";cond->line=expr->line;
    cond->children.push_back(expr);return cond;
}

ASTNode* Parser::parseIf() {
    ASTNode* n=new ASTNode();n->type="IF";n->line=peek().line;advance();
    n->children.push_back(parseCondition());n->children.push_back(parseBlock());
    while(true) {
        if(peek().type==TOKEN_KW_ELSEIF||checkVal("elseif"))
            {advance();ASTNode* e=new ASTNode();e->type="ELSEIF";e->children.push_back(parseCondition());e->children.push_back(parseBlock());n->children.push_back(e);}
        else if(peek().type==TOKEN_KW_ELSE||checkVal("else")) {
            advance();
            if(peek().type==TOKEN_KW_IF||checkVal("if"))
                {advance();ASTNode* e=new ASTNode();e->type="ELSEIF";e->children.push_back(parseCondition());e->children.push_back(parseBlock());n->children.push_back(e);}
            else {ASTNode* e=new ASTNode();e->type="ELSE";e->children.push_back(parseBlock());n->children.push_back(e);break;}
        } else break;
    }
    skipSemicolon();return n;
}

ASTNode* Parser::parseFor() {
    ASTNode* n=new ASTNode();n->type="FOR";n->line=peek().line;advance();
    eatVal("(");

    // for (gg x in arr) {} — 범위 기반
    if((peek().type==TOKEN_KW_GG||isTypeKeyword(peek()))&&peek(2).type==TOKEN_KW_IN) {
        return parseForIn();
    }

    // for (gg i = 0; i <= 3; i++) {}
    ASTNode* init=new ASTNode();init->type="VAR_DECL";init->value=peek().value;advance();
    ASTNode* initNm=new ASTNode();initNm->type="VAR_NAME";initNm->value=peek().value;advance();
    if(checkVal("=")){advance();initNm->children.push_back(parseExpr());}
    init->children.push_back(initNm);skipSemicolon();n->children.push_back(init);

    // 조건
    ASTNode* cond=new ASTNode();cond->type="CONDITION";cond->line=peek().line;
    ASTNode* cl=new ASTNode();cl->type="COND_LEFT"; cl->value=peek().value;advance();
    ASTNode* co=new ASTNode();co->type="COND_OP";   co->value=peek().value;advance();
    ASTNode* cr=new ASTNode();cr->type="COND_RIGHT"; cr->value=peek().value;advance();
    skipSemicolon();
    cond->children.push_back(cl);cond->children.push_back(co);cond->children.push_back(cr);
    n->children.push_back(cond);

    // 스텝
    ASTNode* step=new ASTNode();step->type="FOR_STEP";step->value=peek().value;step->line=1;advance();
    if(checkVal("++"))       {step->line=1; advance();}
    else if(checkVal("--"))  {step->line=-1;advance();}
    else if(checkVal("="))   {advance();advance();
                              if(checkVal("+"))      {step->line=1; advance();advance();}
                              else if(checkVal("-")) {step->line=-1;advance();advance();}}
    eatVal(")");n->children.push_back(step);n->children.push_back(parseBlock());
    skipSemicolon();return n;
}

ASTNode* Parser::parseForIn() {
    // for (gg x in arr) {}  — 이미 '(' 소비됨
    ASTNode* n=new ASTNode();n->type="FOR_IN";n->line=peek().line;
    advance(); // gg/type
    ASTNode* varNm=new ASTNode();varNm->type="VAR_NAME";varNm->value=peek().value;advance();
    eat(TOKEN_KW_IN); // in
    ASTNode* arrNm=new ASTNode();arrNm->type="ARRAY_REF";arrNm->value=peek().value;advance();
    eatVal(")");
    n->children.push_back(varNm);n->children.push_back(arrNm);
    n->children.push_back(parseBlock());
    skipSemicolon();return n;
}

ASTNode* Parser::parseWhile() {
    ASTNode* n=new ASTNode();n->type="WHILE";n->line=peek().line;advance();
    n->children.push_back(parseCondition());n->children.push_back(parseBlock());skipSemicolon();return n;
}

ASTNode* Parser::parseLoop() {
    ASTNode* n=new ASTNode();n->type="LOOP";n->line=peek().line;advance();
    if(checkVal("(")) {
        advance();
        while(!checkVal(")")&&peek().type!=TOKEN_EOF) {
            std::string kw=peek().value;advance();
            if(kw=="task"){
                eatVal(".");std::string sub=peek().value;advance();
                if(checkVal("="))advance(); else eatVal(",");
                if(sub=="time")     {ASTNode* t=new ASTNode();t->type="LOOP_TOTAL";    t->value=peek().value;advance();n->children.push_back(t);}
                else if(sub=="interval"){ASTNode* iv=new ASTNode();iv->type="LOOP_INTERVAL";iv->value=peek().value;advance();n->children.push_back(iv);}
            }
            eatVal(",");
        }
        eatVal(")");
    }
    n->children.push_back(parseBlock());skipSemicolon();return n;
}

ASTNode* Parser::parseMethodCall(const std::string& on) {
    ASTNode* n=new ASTNode();n->type="METHOD_CALL";n->value=on;n->line=peek().line;
    advance();eatVal(".");
    ASTNode* mn=new ASTNode();mn->type="CALL_METHOD";mn->value=peek().value;advance();
    n->children.push_back(mn);eatVal("(");
    while(!checkVal(")")&&peek().type!=TOKEN_EOF)
        {ASTNode* a=new ASTNode();a->type="ARG";a->value=peek().value;advance();n->children.push_back(a);if(!eatVal(","))break;}
    eatVal(")");skipSemicolon();return n;
}

ASTNode* Parser::parseTaskSleep() {
    ASTNode* n=new ASTNode();n->type="TASK_SLEEP";n->line=peek().line;
    advance();eatVal(".");eatVal("sleep");eatVal("(");
    n->value=peek().value;advance();eatVal(")");skipSemicolon();return n;
}

ASTNode* Parser::parseIORead() {
    ASTNode* n=new ASTNode();n->type="IO_READ";n->line=peek().line;eatVal("(");
    if(peek().type==TOKEN_STRING){n->value=peek().value;advance();}
    eatVal(",");eatVal("com");
    ASTNode* v=new ASTNode();v->type="READ_TARGET";v->value=peek().value;advance();
    n->children.push_back(v);eatVal(")");skipSemicolon();return n;
}

ASTNode* Parser::parseAICall(const std::string& vn,const std::string& method) {
    ASTNode* n=new ASTNode();n->value=vn;n->line=peek().line;
    if(method=="load")              n->type="AI_LOAD";
    else if(method=="ask"||method=="think") n->type="AI_ASK";
    else if(method=="learn")        n->type="AI_LEARN";
    else if(method=="save")         n->type="AI_SAVE";
    else                            n->type="AI_STATUS";
    eatVal("(");
    if(n->type=="AI_ASK") {
        auto add=[&](const std::string& t){ASTNode* a=new ASTNode();a->type=t;a->value=peek().value;advance();n->children.push_back(a);};
        if(!checkVal(")"))add("AI_PROMPT");
        if(checkVal(","))  {advance();add("AI_TEMPERATURE");}
        if(checkVal(","))  {advance();add("AI_SENTENCES");}
        if(checkVal(","))  {advance();add("AI_RESULT_VAR");}
    } else if(!checkVal(")")) {
        ASTNode* a=new ASTNode();a->type="AI_ARG";a->value=peek().value;advance();n->children.push_back(a);
    }
    eatVal(")");skipSemicolon();return n;
}

ASTNode* Parser::parseFString(const std::string& raw) {
    ASTNode* n=new ASTNode();n->type="FSTRING";n->value=raw;return n;
}

// ──────────────────────────────────────────────────────────────────
//  표현식 우선순위
// ──────────────────────────────────────────────────────────────────
ASTNode* Parser::parseExpr()    { return parseExprOr(); }

ASTNode* Parser::parseExprOr() {
    ASTNode* left=parseExprAnd();
    while(peek().type==TOKEN_OR){
        int line=peek().line;advance();
        ASTNode* n=new ASTNode();n->type="LOGIC_OR";n->value="||";n->line=line;
        n->children.push_back(left);n->children.push_back(parseExprAnd());left=n;
    }
    return left;
}

ASTNode* Parser::parseExprAnd() {
    ASTNode* left=parseExprCmp();
    while(peek().type==TOKEN_AND){
        int line=peek().line;advance();
        ASTNode* n=new ASTNode();n->type="LOGIC_AND";n->value="&&";n->line=line;
        n->children.push_back(left);n->children.push_back(parseExprCmp());left=n;
    }
    return left;
}

ASTNode* Parser::parseExprCmp() {
    ASTNode* left=parseExprAdd();
    while(isCompOp(peek())){
        std::string op=peek().value;int line=peek().line;advance();
        ASTNode* n=new ASTNode();n->type="BIN_OP";n->value=op;n->line=line;
        n->children.push_back(left);n->children.push_back(parseExprAdd());left=n;
    }
    return left;
}

ASTNode* Parser::parseExprAdd() {
    ASTNode* left=parseExprMul();
    while(checkVal("+")||checkVal("-")){
        std::string op=peek().value;int line=peek().line;advance();
        ASTNode* n=new ASTNode();n->type="BIN_OP";n->value=op;n->line=line;
        n->children.push_back(left);n->children.push_back(parseExprMul());left=n;
    }
    return left;
}

ASTNode* Parser::parseExprMul() {
    ASTNode* left=parseExprPow();
    while(checkVal("*")||checkVal("/")||checkVal("%")){
        std::string op=peek().value;int line=peek().line;advance();
        ASTNode* n=new ASTNode();n->type="BIN_OP";n->value=op;n->line=line;
        n->children.push_back(left);n->children.push_back(parseExprPow());left=n;
    }
    return left;
}

ASTNode* Parser::parseExprPow() {
    ASTNode* left=parseExprUnary();
    while(checkVal("**")){
        int line=peek().line;advance();
        ASTNode* n=new ASTNode();n->type="BIN_OP";n->value="**";n->line=line;
        n->children.push_back(left);n->children.push_back(parseExprUnary());left=n;
    }
    return left;
}

ASTNode* Parser::parseExprUnary() {
    if(checkVal("-")){int line=peek().line;advance();ASTNode* n=new ASTNode();n->type="NEG";n->line=line;n->children.push_back(parseExprPrimary());return n;}
    if(peek().type==TOKEN_NOT){int line=peek().line;advance();ASTNode* n=new ASTNode();n->type="LOGIC_NOT";n->value="!";n->line=line;n->children.push_back(parseExprPrimary());return n;}
    return parseExprPrimary();
}

ASTNode* Parser::parseExprPrimary() {
    Token t=peek();
    if(checkVal("(")) {advance();ASTNode* inner=parseExpr();eatVal(")");
        // as 캐스팅: (expr) as int
        if(peek().type==TOKEN_KW_AS) {
            advance();
            ASTNode* n=new ASTNode();n->type="CAST";n->value=peek().value;advance();
            n->children.push_back(inner);return n;
        }
        return inner;
    }
    if(t.type==TOKEN_FSTRING){ASTNode* n=parseFString(t.value);advance();return n;}
    if(t.type==TOKEN_STRING) {ASTNode* n=new ASTNode();n->type="STR_LIT";n->value=t.value;advance();return n;}
    if(t.value=="Math"&&peek(1).value==".") {advance();eatVal(".");std::string f=peek().value;advance();return parseMathCall(f);}
    if(t.type==TOKEN_NUMBER) {ASTNode* n=new ASTNode();n->type="NUM_LIT";n->value=t.value;advance();
        // as 캐스팅: 3 as float
        if(peek().type==TOKEN_KW_AS){advance();ASTNode* c=new ASTNode();c->type="CAST";c->value=peek().value;advance();c->children.push_back(n);return c;}
        return n;
    }
    if(t.type==TOKEN_KW_TRUE) {advance();ASTNode* n=new ASTNode();n->type="BOOL_LIT";n->value="true"; return n;}
    if(t.type==TOKEN_KW_FALSE){advance();ASTNode* n=new ASTNode();n->type="BOOL_LIT";n->value="false";return n;}
    if(t.type==TOKEN_KW_NIL||t.type==TOKEN_KW_NULL){advance();ASTNode* n=new ASTNode();n->type="NIL_LIT";n->value="nil";return n;}

    if(t.type==TOKEN_IDENTIFIER) {
        std::string name=t.value;advance();
        if(checkVal("[")) {advance();ASTNode* n=new ASTNode();n->type="ARRAY_GET";n->value=name;n->children.push_back(parseExpr());eatVal("]");return n;}
        if(checkVal(".")) {
            advance();std::string method=peek().value;advance();
            if(checkVal("(")){eatVal("(");ASTNode* n=new ASTNode();n->type="METHOD_CALL_EXPR";n->value=name+"."+method;
                while(!checkVal(")")&&peek().type!=TOKEN_EOF){n->children.push_back(parseExpr());eatVal(",");}eatVal(")");return n;}
            ASTNode* n=new ASTNode();n->type="FIELD_GET";n->value=name+"."+method;return n;
        }
        ASTNode* n=new ASTNode();n->type="VAR_REF";n->value=name;
        // as 캐스팅: x as int
        if(peek().type==TOKEN_KW_AS){advance();ASTNode* c=new ASTNode();c->type="CAST";c->value=peek().value;advance();c->children.push_back(n);return c;}
        return n;
    }
    ASTNode* n=new ASTNode();n->type="VAR_REF";n->value=t.value;advance();return n;
}

ASTNode* Parser::parseMathCall(const std::string& func) {
    if(func=="PI"||func=="E"||func=="I"){ASTNode* n=new ASTNode();n->type="MATH_CONST";n->value=func;return n;}
    ASTNode* n=new ASTNode();n->type="MATH_CALL";n->value=func;
    eatVal("(");
    while(!checkVal(")")&&peek().type!=TOKEN_EOF){n->children.push_back(parseExpr());if(!eatVal(","))break;}
    eatVal(")");return n;
}
