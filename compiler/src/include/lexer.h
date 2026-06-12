// =============================================================
//  BLUSK lexer.h  -  Matrix, as, in 키워드 추가
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

    // ── 신규 ─────────────────────────────────────────────────
    TOKEN_KW_MATRIX,  // Matrix 타입
    TOKEN_KW_AS,      // as 캐스팅
    TOKEN_KW_IN,      // in (for x in arr)
    TOKEN_KW_NULL,    // null (nil 별칭)

    // 논리 연산자
    TOKEN_AND,   // &&
    TOKEN_OR,    // ||
    TOKEN_NOT,   // !
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
