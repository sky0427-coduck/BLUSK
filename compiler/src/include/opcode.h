// =============================================================
//  BLUSK opcode.h  -  Matrix / SIMD opcode 추가
// =============================================================
#pragma once
#include <string>
#include <cstdint>

enum OpCode : uint16_t {
    OP_LOAD_INT, OP_LOAD_FLOAT, OP_LOAD_STR,
    OP_LOAD_BOOL, OP_LOAD_NIL, OP_MOVE,
    OP_STORE, OP_LOAD,
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD, OP_POW, OP_NEG,
    OP_CMP_EQ, OP_CMP_NE, OP_CMP_LT, OP_CMP_LE, OP_CMP_GT, OP_CMP_GE,
    OP_AND, OP_OR, OP_NOT,
    OP_JUMP, OP_JUMP_IF, OP_JUMP_IFNOT,
    OP_BREAK, OP_CONTINUE, OP_RETURN, OP_HALT_IF,
    OP_PRINT, OP_PRINT_FMT, OP_READ,
    OP_NEW, OP_CALL, OP_SET_FIELD, OP_GET_FIELD,
    OP_ARRAY_NEW, OP_ARRAY_GET, OP_ARRAY_SET, OP_SIZE,
    OP_MATH_SQRT, OP_MATH_ABS,   OP_MATH_CEIL,  OP_MATH_FLOOR,
    OP_MATH_ROUND,OP_MATH_SIN,   OP_MATH_COS,   OP_MATH_TAN,
    OP_MATH_ASIN, OP_MATH_ACOS,  OP_MATH_ATAN,
    OP_MATH_EXP,  OP_MATH_LOG,   OP_MATH_LOG10,
    OP_MATH_PI,   OP_MATH_E,

    // ── Tensor / SIMD ─────────────────────────────────────────
    OP_TENSOR_NEW,   // tensor<N> 생성
    OP_TENSOR_INIT,  // 배열 → tensor 변환
    OP_SIMD_ADD,     // element-wise +
    OP_SIMD_MUL,     // element-wise *
    OP_SIMD_FMADD,   // a*b + c (FMA)
    OP_TENSOR_SUM,   OP_TENSOR_DOT,  OP_TENSOR_NORM,
    OP_TENSOR_MEAN,  OP_TENSOR_STD,  OP_INVSQRT,
    OP_MEM_PREFETCH,

    // ── Matrix (신규) ─────────────────────────────────────────
    // strVal = "rows,cols"  intVal = rows*cols 원소 수
    // src1 = 첫 번째 원소 레지스터
    OP_MATRIX_NEW,   // Matrix 생성: reg[dst] = Matrix(rows, cols, data...)
    OP_MATRIX_MUL,   // reg[dst] = reg[src1] @ reg[src2]  (행렬 곱)
    OP_MATRIX_ADD,   // reg[dst] = reg[src1] + reg[src2]  (element-wise)
    OP_MATRIX_ELMUL, // reg[dst] = reg[src1] * reg[src2]  (Hadamard)
    OP_MATRIX_T,     // reg[dst] = transpose(reg[src1])
    OP_MATRIX_PRINT, // print(reg[src1])  → 행렬 포맷 출력
    OP_SIMD_FOR,     // @simd for varname — SIMD 가속 재계산

    // ── as 타입 캐스팅 (신규) ────────────────────────────────
    // strVal = "int" / "float" / "str" / "bool"
    OP_CAST,

    // ── stdlib NativeCall ─────────────────────────────────────
    OP_NATIVE_CALL,
    OP_SLEEP, OP_PERF_NOW,
    OP_AI_LOAD, OP_AI_ASK, OP_AI_LEARN, OP_AI_SAVE, OP_AI_STATUS,
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
