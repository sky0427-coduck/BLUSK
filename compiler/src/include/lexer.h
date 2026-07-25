// =============================================================
//  BLUSK lexer.h  -  long 키워드 추가
// =============================================================
#pragma once
#include <string>

enum TokenType {
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_FSTRING,
    TOKEN_SYMBOL,
    TOKEN_EOF,

    TOKEN_KW_ROOT,      TOKEN_KW_PACKAGE,   TOKEN_KW_IMPORT,
    TOKEN_KW_FORKFROM,  TOKEN_KW_THE_END,   TOKEN_KW_BLUSK,
    TOKEN_KW_PUBLIC,    TOKEN_KW_VOID,      TOKEN_KW_IF,
    TOKEN_KW_ELSE,      TOKEN_KW_ELSEIF,    TOKEN_KW_FOR,
    TOKEN_KW_WHILE,     TOKEN_KW_LOOP,      TOKEN_KW_BREAK,
    TOKEN_KW_CONTINUE,  TOKEN_KW_RETURN,    TOKEN_KW_NEW,
    TOKEN_KW_GG,        TOKEN_KW_NUM,       TOKEN_KW_TRUE,
    TOKEN_KW_FALSE,     TOKEN_KW_NIL,       TOKEN_KW_SIMD,
    TOKEN_KW_TENSOR,    TOKEN_KW_ANNOTATION,
    TOKEN_KW_MATRIX,    TOKEN_KW_AS,        TOKEN_KW_IN,
    TOKEN_KW_NULL,

    // ── 명시적 숫자 타입 키워드 ──────────────────────────────
    TOKEN_KW_INT,     // int   (32bit 정수)
    TOKEN_KW_LONG,    // long  (64bit 정수)
    TOKEN_KW_FLOAT,   // float (32bit 실수)
    TOKEN_KW_DOUBLE,  // double(64bit 실수)
    TOKEN_KW_BOOL,    // bool
    TOKEN_KW_STR,     // str

    // 논리 연산자
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,
};

struct Token {
    TokenType   type;
    std::string value;
    int         line = 1;
};

class Lexer {
    std::string src;
    size_t      pos;
    int         line;
public:
    Lexer(const std::string& source);
    Token nextToken();
private:
    char peek();
    char peekAt(int offset);
    void advance();
    Token identifier();
    Token number();
    Token string_();
    Token annotation();
    Token lineComment();
    Token blockComment();
    static TokenType classifyKeyword(const std::string& s);
};
