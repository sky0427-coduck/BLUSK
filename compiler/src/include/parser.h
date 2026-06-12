// =============================================================
//  BLUSK parser.h  -  Matrix, switch-case, as, simd for 추가
// =============================================================
#pragma once
#include "lexer.h"
#include "ast.h"
#include <vector>
#include <string>

class Parser {
    std::vector<Token> tokens;
    size_t pos;
public:
    Parser(std::vector<Token>& tks);
    ASTNode* parse();
private:
    Token  peek(int offset=0) const;
    bool   check(TokenType t, int offset=0) const;
    bool   checkVal(const std::string& v, int offset=0) const;
    Token  advance();
    bool   eat(TokenType t);
    bool   eatVal(const std::string& v);
    void   skipSemicolon();

    ASTNode* parseTopLevel();
    ASTNode* parseRootPackage();
    ASTNode* parseImport();
    ASTNode* parseAnnotation();    // @entry @native @simd ...
    ASTNode* parseBluskMain(const std::string& annotation);
    ASTNode* parseClass();
    ASTNode* parseStatement();
    ASTNode* parsePrint();
    ASTNode* parseTheEnd();
    ASTNode* parseIf();
    ASTNode* parseFor();           // for (gg i=0; i<=3; i++) {}
    ASTNode* parseForIn();         // for (gg x in arr) {}
    ASTNode* parseWhile();
    ASTNode* parseLoop();
    ASTNode* parseSwitch();        // switch(a) { case 0 -> expr, ... }
    ASTNode* parseVarDecl(const std::string& keyword);
    ASTNode* parseArrayDecl(const std::string& keyword);
    ASTNode* parseMatrixDecl();    // Matrix mat = {(r1) * (r2)};
    ASTNode* parseMethodCall(const std::string& objName);
    ASTNode* parseIORead();
    ASTNode* parseTaskSleep();
    ASTNode* parseBlock();
    ASTNode* parseReturn();
    ASTNode* parseAICall(const std::string& varName, const std::string& method);
    ASTNode* parseSimdFor();       // @simd for varname;

    // 표현식 우선순위 체계
    ASTNode* parseExpr();
    ASTNode* parseExprOr();
    ASTNode* parseExprAnd();
    ASTNode* parseExprCmp();
    ASTNode* parseExprAdd();
    ASTNode* parseExprMul();
    ASTNode* parseExprPow();
    ASTNode* parseExprUnary();
    ASTNode* parseExprPrimary();   // as 캐스팅 포함
    ASTNode* parseFString(const std::string& raw);
    ASTNode* parseCondition();
    ASTNode* parseMathCall(const std::string& func);
    ASTNode* parseMethod(const std::string& retType);
    ASTNode* parseConstructor(const std::string& className);
    void     parseParamList(ASTNode* node);

    // 행 벡터 파싱: (1, 2, 3, 4)
    std::vector<double> parseRowVector();

    bool isTypeKeyword(const Token& t) const;
    bool isCompOp(const Token& t)      const;
};
