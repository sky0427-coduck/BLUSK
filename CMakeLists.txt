cmake_minimum_required(VERSION 3.18)
project(BLUSK VERSION 0.7.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── 빌드 타입 기본값 ─────────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ── 최적화 플래그 ─────────────────────────────────────────────────
if(MSVC)
    add_compile_options(/O2 /W3 /utf-8 /EHsc)
    add_compile_options(/arch:AVX2)  # SIMD
else()
    add_compile_options(-O3 -Wall -Wextra -march=native)
    add_compile_options(-ffast-math)
endif()

# ── 소스 파일 ─────────────────────────────────────────────────────
set(BLUSK_SOURCES
        compiler/src/sources/main.cpp
        compiler/src/sources/lexer.cpp
        compiler/src/sources/parser.cpp
        compiler/src/sources/ast.cpp
        compiler/src/sources/checker.cpp
        compiler/src/sources/compiler.cpp
        compiler/src/sources/vm.cpp
        compiler/src/sources/value.cpp
        compiler/src/sources/gc.cpp
        compiler/src/sources/scope.cpp
        compiler/src/sources/string_table.cpp
        compiler/src/sources/module.cpp
        compiler/src/sources/debug.cpp
        compiler/src/sources/repl.cpp
        compiler/src/sources/native.cpp
        compiler/src/sources/jit.cpp
        compiler/src/sources/meta.cpp
    # stdlib
    stdlib/blusk_io.cpp
    stdlib/blusk_string.cpp
    stdlib/blusk_math.cpp
    stdlib/blusk_time.cpp
    stdlib/blusk_collections.cpp
)

add_executable(blusk ${BLUSK_SOURCES})

target_include_directories(blusk PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/compiler/src/include
    ${CMAKE_SOURCE_DIR}/stdlib
    ${CMAKE_SOURCE_DIR}/llama_include
)

# ── 스레드 ────────────────────────────────────────────────────────
find_package(Threads REQUIRED)
target_link_libraries(blusk PRIVATE Threads::Threads)

# ── LLVM @native 지원 (선택) ──────────────────────────────────────
option(BLUSK_LLVM "Enable LLVM backend for @native" OFF)
if(BLUSK_LLVM)
    find_package(LLVM REQUIRED CONFIG)
    message(STATUS "LLVM version: ${LLVM_PACKAGE_VERSION}")
    target_compile_definitions(blusk PRIVATE LLVM_ENABLED)
    target_include_directories(blusk PRIVATE ${LLVM_INCLUDE_DIRS})
    target_link_libraries(blusk PRIVATE ${LLVM_LIBRARIES})
endif()

# ── llama.cpp 내장 AI (선택) ──────────────────────────────────────
option(BLUSK_LLAMA "Enable embedded llama.cpp for Saturday AI" OFF)
if(BLUSK_LLAMA)
    target_compile_definitions(blusk PRIVATE LLAMA_EMBED)
    # llama.cpp는 include/ 폴더에 llama.h 놓고, lib/ 폴더에 llama.lib/libllama.a
    target_link_directories(blusk PRIVATE ${CMAKE_SOURCE_DIR}/lib)
    if(WIN32)
        target_link_libraries(blusk PRIVATE llama)
    else()
        target_link_libraries(blusk PRIVATE llama m dl)
    endif()
endif()

# ── httplib Saturday AI API 모드 (선택) ──────────────────────────
option(BLUSK_HTTP "Enable HTTP API mode for Saturday AI" OFF)
if(BLUSK_HTTP)
    find_package(OpenSSL REQUIRED)
    target_link_libraries(blusk PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

# ── 설치 ──────────────────────────────────────────────────────────
install(TARGETS blusk DESTINATION bin)

message(STATUS "")
message(STATUS "BLUSK ${CMAKE_PROJECT_VERSION} build configuration:")
message(STATUS "  Build type : ${CMAKE_BUILD_TYPE}")
message(STATUS "  LLVM       : ${BLUSK_LLVM}")
message(STATUS "  llama.cpp  : ${BLUSK_LLAMA}")
message(STATUS "  HTTP API   : ${BLUSK_HTTP}")
message(STATUS "")
