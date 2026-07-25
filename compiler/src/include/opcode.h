// =============================================================
//  BLUSK opcode.h  -  AI opcode 제거판
// =============================================================
#pragma once
#include <string>
#include <cstdint>

enum OpCode : uint16_t {
    // 로드
    OP_LOAD_INT, OP_LOAD_FLOAT, OP_LOAD_STR, OP_LOAD_BOOL, OP_LOAD_NIL, OP_MOVE,
    // 변수
    OP_STORE, OP_LOAD,
    // 산술
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD, OP_POW, OP_NEG,
    // 비교
    OP_CMP_EQ, OP_CMP_NE, OP_CMP_LT, OP_CMP_LE, OP_CMP_GT, OP_CMP_GE,
    // 논리
    OP_AND, OP_OR, OP_NOT,
    // 제어
    OP_JUMP, OP_JUMP_IF, OP_JUMP_IFNOT,
    OP_BREAK, OP_CONTINUE, OP_RETURN, OP_HALT_IF,
    // 타입 캐스팅  strVal="int"/"float"/"str"/"bool"
    OP_CAST,
    // I/O
    OP_PRINT, OP_PRINT_FMT, OP_READ,
    // OOP
    OP_NEW, OP_CALL, OP_SET_FIELD, OP_GET_FIELD,
    // 배열
    OP_ARRAY_NEW, OP_ARRAY_GET, OP_ARRAY_SET, OP_SIZE,
    // Math
    OP_MATH_SQRT, OP_MATH_ABS,   OP_MATH_CEIL,  OP_MATH_FLOOR,
    OP_MATH_ROUND, OP_MATH_SIN,  OP_MATH_COS,   OP_MATH_TAN,
    OP_MATH_ASIN,  OP_MATH_ACOS, OP_MATH_ATAN,
    OP_MATH_EXP,   OP_MATH_LOG,  OP_MATH_LOG10,
    OP_MATH_PI, OP_MATH_E,
    // Tensor
    OP_TENSOR_NEW, OP_TENSOR_INIT,
    OP_SIMD_ADD, OP_SIMD_MUL, OP_SIMD_FMADD,
    OP_TENSOR_SUM, OP_TENSOR_DOT, OP_TENSOR_NORM,
    OP_TENSOR_MEAN, OP_TENSOR_STD, OP_INVSQRT,
    OP_MEM_PREFETCH,
    // Matrix (AVX2)
    OP_MATRIX_NEW,   // strVal="rows,cols,op"
    OP_MATRIX_MUL,   // @ 행렬 곱 (AVX2 FMA)
    OP_MATRIX_ADD,   // element-wise +
    OP_MATRIX_ELMUL, // element-wise * (Hadamard)
    OP_MATRIX_T,     // 전치
    OP_MATRIX_PRINT,
    OP_SIMD_FOR,     // @simd for varname
    // stdlib
    OP_NATIVE_CALL,
    // 시간
    OP_SLEEP, OP_PERF_NOW,
    // AI — 보류 (사용자 직접 엔진 완성 후 복원)
    // OP_AI_LOAD, OP_AI_ASK, OP_AI_LEARN, OP_AI_SAVE, OP_AI_STATUS,
    OP_HALT,
};

enum StoreFlag : int64_t {
    STORE_NORMAL   = 0,
    STORE_CONST    = 1,
    STORE_RCSKIP   = 2,
    STORE_CONST_RC = 3,
};

struct Instruction {
    OpCode  op;
    uint8_t dst  = 0;
    uint8_t src1 = 0;
    uint8_t src2 = 0;
    std::string strVal;
    int64_t     intVal   = 0;
    double      floatVal = 0.0;
};
