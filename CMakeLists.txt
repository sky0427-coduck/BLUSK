cmake_minimum_required(VERSION 3.18)
project(BLUSK VERSION 0.7.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ── 최적화 플래그 ─────────────────────────────────────────────────
if(MSVC)
    add_compile_options(/O2 /W3 /utf-8 /EHsc /arch:AVX2)
else()
    add_compile_options(-O3 -Wall -Wextra -march=native -ffast-math)
endif()

# ── include/source 경로 ───────────────────────────────────────────
set(INC  ${CMAKE_SOURCE_DIR}/compiler/src/include)
set(SRC  ${CMAKE_SOURCE_DIR}/compiler/src/sources)
set(STD  ${CMAKE_SOURCE_DIR}/stdlib)
set(EXTI ${CMAKE_SOURCE_DIR}/include)   # httplib.h 등 외부 헤더

# ── 소스 파일 ─────────────────────────────────────────────────────
set(BLUSK_SOURCES
    ${SRC}/main.cpp
    ${SRC}/lexer.cpp
    ${SRC}/parser.cpp
    ${SRC}/ast.cpp
    ${SRC}/checker.cpp
    ${SRC}/compiler.cpp
    ${SRC}/vm.cpp
    ${SRC}/value.cpp
    ${SRC}/gc.cpp
    ${SRC}/scope.cpp
    ${SRC}/string_table.cpp
    ${SRC}/module.cpp
    ${SRC}/debug.cpp
    ${SRC}/repl.cpp
    ${SRC}/native.cpp
    ${SRC}/jit.cpp
    ${SRC}/meta.cpp
    # stdlib
    ${STD}/blusk_io.cpp
    ${STD}/blusk_string.cpp
    ${STD}/blusk_math.cpp
    ${STD}/blusk_time.cpp
    ${STD}/blusk_collections.cpp
)

add_executable(blusk ${BLUSK_SOURCES})

target_include_directories(blusk PRIVATE
    ${INC}
    ${STD}
    ${EXTI}
)

# ── 스레드 ────────────────────────────────────────────────────────
find_package(Threads REQUIRED)
target_link_libraries(blusk PRIVATE Threads::Threads)

# ── LLVM @native (선택, 1.0 목표) ────────────────────────────────
option(BLUSK_LLVM "Enable LLVM backend for @native" OFF)
if(BLUSK_LLVM)
    find_package(LLVM REQUIRED CONFIG)
    message(STATUS "LLVM version: ${LLVM_PACKAGE_VERSION}")
    target_compile_definitions(blusk PRIVATE LLVM_ENABLED)
    target_include_directories(blusk PRIVATE ${LLVM_INCLUDE_DIRS})
    target_link_libraries(blusk PRIVATE ${LLVM_LIBRARIES})
endif()

# ── httplib (Saturday AI API 모드, 선택) ─────────────────────────
option(BLUSK_HTTP "Enable HTTP API mode" OFF)
if(BLUSK_HTTP)
    find_package(OpenSSL REQUIRED)
    target_link_libraries(blusk PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

# ── 설치 ──────────────────────────────────────────────────────────
install(TARGETS blusk DESTINATION bin)

# ── 예제 파일도 설치 ──────────────────────────────────────────────
install(DIRECTORY ${CMAKE_SOURCE_DIR}/examples/
        DESTINATION share/blusk/examples)

message(STATUS "")
message(STATUS "BLUSK ${CMAKE_PROJECT_VERSION} build config:")
message(STATUS "  Build type : ${CMAKE_BUILD_TYPE}")
message(STATUS "  LLVM       : ${BLUSK_LLVM}")
message(STATUS "  HTTP       : ${BLUSK_HTTP}")
message(STATUS "  AI         : Reserved (custom engine)")
message(STATUS "")
