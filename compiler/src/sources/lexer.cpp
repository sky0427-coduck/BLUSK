// =============================================================
//  BLUSK lexer.cpp  -  Matrix, as, in, null 추가
// =============================================================
#include "../include/lexer.h"
#include <cctype>
#include <unordered_map>

Lexer::Lexer(const std::string& source) : src(source), pos(0), line(1) {}

char Lexer::peek()             { return pos < src.size() ? src[pos] : '\0'; }
char Lexer::peekAt(int offset) { size_t p=pos+offset; return p<src.size()?src[p]:'\0'; }
void Lexer::advance() { if(pos<src.size()){ if(src[pos]=='\n')line++; pos++; } }

TokenType Lexer::classifyKeyword(const std::string& s) {
    static const std::unordered_map<std::string, TokenType> kw = {
        {"root",     TOKEN_KW_ROOT},    {"package",  TOKEN_KW_PACKAGE},
        {"import",   TOKEN_KW_IMPORT},  {"forkfrom", TOKEN_KW_FORKFROM},
        {"the",      TOKEN_KW_THE_END}, {"Blusk",    TOKEN_KW_BLUSK},
        {"public",   TOKEN_KW_PUBLIC},  {"void",     TOKEN_KW_VOID},
        {"if",       TOKEN_KW_IF},      {"else",     TOKEN_KW_ELSE},
        {"elseif",   TOKEN_KW_ELSEIF},  {"for",      TOKEN_KW_FOR},
        {"while",    TOKEN_KW_WHILE},   {"loop",     TOKEN_KW_LOOP},
        {"break",    TOKEN_KW_BREAK},   {"continue", TOKEN_KW_CONTINUE},
        {"return",   TOKEN_KW_RETURN},  {"new",      TOKEN_KW_NEW},
        {"gg",       TOKEN_KW_GG},      {"num",      TOKEN_KW_NUM},
        {"true",     TOKEN_KW_TRUE},    {"false",    TOKEN_KW_FALSE},
        {"nil",      TOKEN_KW_NIL},     {"null",     TOKEN_KW_NULL},
        {"simd",     TOKEN_KW_SIMD},    {"tensor",   TOKEN_KW_TENSOR},
        {"Matrix",   TOKEN_KW_MATRIX},  {"as",       TOKEN_KW_AS},
        {"in",       TOKEN_KW_IN},
    };
    auto it = kw.find(s);
    return it != kw.end() ? it->second : TOKEN_IDENTIFIER;
}

Token Lexer::identifier() {
    std::string val; int sl = line;
    while (std::isalnum(peek()) || peek()=='_' || peek()=='.') {
        if (peek()=='.') { if (!std::isalpha(peekAt(1)) && peekAt(1)!='_') break; }
        val += peek(); advance();
    }
    return { classifyKeyword(val), val, sl };
}

Token Lexer::number() {
    std::string val; int sl = line; bool hasDot = false;
    if (peek()=='0' && (peekAt(1)=='x'||peekAt(1)=='X')) {
        val+=peek(); advance(); val+=peek(); advance();
        while (std::isxdigit(peek())) { val+=peek(); advance(); }
        return { TOKEN_NUMBER, val, sl };
    }
    if (peek()=='0' && (peekAt(1)=='b'||peekAt(1)=='B')) {
        val+=peek(); advance(); val+=peek(); advance();
        while (peek()=='0'||peek()=='1') { val+=peek(); advance(); }
        return { TOKEN_NUMBER, val, sl };
    }
    while (std::isdigit(peek()) || (peek()=='.'&&!hasDot&&std::isdigit(peekAt(1)))) {
        if (peek()=='.') hasDot=true; val+=peek(); advance();
    }
    return { TOKEN_NUMBER, val, sl };
}

Token Lexer::string_() {
    int sl = line; bool isFStr = false;
    if (peek()=='f' && peekAt(1)=='"') { isFStr=true; advance(); }
    advance();
    std::string val;
    while (peek()!='"' && peek()!='\0') {
        if (peek()=='\\') {
            advance();
            switch(peek()) {
            case 'n': val+='\n'; break; case 't': val+='\t'; break;
            case '"': val+='"';  break; case '\\':val+='\\'; break;
            default:  val+=peek(); break;
            }
        } else val+=peek();
        advance();
    }
    advance();
    return { isFStr ? TOKEN_FSTRING : TOKEN_STRING, val, sl };
}

Token Lexer::annotation() {
    int sl = line; advance();
    std::string val;
    while (std::isalnum(peek())||peek()=='_') { val+=peek(); advance(); }
    return { TOKEN_KW_ANNOTATION, val, sl };
}

Token Lexer::lineComment()  { while(peek()!='\n'&&peek()!='\0') advance(); return nextToken(); }
Token Lexer::blockComment() {
    advance(); advance();
    while (peek()!='\0') { if(peek()=='*'&&peekAt(1)=='/'){advance();advance();break;} advance(); }
    return nextToken();
}

Token Lexer::nextToken() {
    while (std::isspace(peek())) advance();
    char c = peek(); if (c=='\0') return { TOKEN_EOF, "", line };
    int sl = line;

    if (c=='/'&&peekAt(1)=='/') { advance(); advance(); return lineComment(); }
    if (c=='/'&&peekAt(1)=='*') return blockComment();
    if (c=='f'&&peekAt(1)=='"') return string_();
    if (std::isalpha(c)||c=='_') return identifier();
    if (std::isdigit(c)) return number();
    if (c=='"') return string_();
    if (c=='@') return annotation();

    advance();
    if (c=='*'&&peek()=='*') { advance(); return { TOKEN_SYMBOL, "**", sl }; }
    if (c=='<'&&peek()=='=') { advance(); return { TOKEN_SYMBOL, "<=", sl }; }
    if (c=='>'&&peek()=='=') { advance(); return { TOKEN_SYMBOL, ">=", sl }; }
    if (c=='='&&peek()=='=') { advance(); return { TOKEN_SYMBOL, "==", sl }; }
    if (c=='+'&&peek()=='+') { advance(); return { TOKEN_SYMBOL, "++", sl }; }
    if (c=='-'&&peek()=='-') { advance(); return { TOKEN_SYMBOL, "--", sl }; }
    if (c=='-'&&peek()=='>') { advance(); return { TOKEN_SYMBOL, "->", sl }; }
    if (c=='.'&&peek()=='.') { advance(); return { TOKEN_SYMBOL, "..", sl }; }
    if (c=='&'&&peek()=='&') { advance(); return { TOKEN_AND, "&&", sl }; }
    if (c=='|'&&peek()=='|') { advance(); return { TOKEN_OR,  "||", sl }; }
    if (c=='!'&&peek()=='=') { advance(); return { TOKEN_SYMBOL, "!=", sl }; }
    if (c=='!') return { TOKEN_NOT, "!", sl };

    return { TOKEN_SYMBOL, std::string(1,c), sl };
}
