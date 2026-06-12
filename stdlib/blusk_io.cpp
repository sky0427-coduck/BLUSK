// =============================================================
//  BLUSK stdlib/blusk_io.cpp
// =============================================================
#include "blusk_io.h"
#include "../compiler/src/include/error.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace BluskStd::IO {

// ── print / println ──────────────────────────────────────────────
Value print(const std::vector<Value>& args) {
    for (size_t i = 0; i < args.size(); i++) {
        if (i > 0) std::cout << " ";
        std::cout << args[i].toString();
    }
    std::cout << "\n";
    return Value::Nil();
}

Value println(const std::vector<Value>& args) { return print(args); }

// ── input ────────────────────────────────────────────────────────
Value input(const std::vector<Value>& args) {
    if (!args.empty()) std::cout << args[0].toString();
    std::string line;
    std::getline(std::cin, line);
    return Value::String(line);
}

// ── readFile ─────────────────────────────────────────────────────
Value readFile(const std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) {
        BluskError::report("read_file requires a string path", "stdlib.io", 0);
        return Value::Nil();
    }
    std::ifstream f(args[0].str);
    if (!f.is_open()) {
        BluskError::report("Cannot open file: " + args[0].str, "stdlib.io", 0);
        return Value::Nil();
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return Value::String(ss.str());
}

// ── writeFile ────────────────────────────────────────────────────
Value writeFile(const std::vector<Value>& args) {
    if (args.size() < 2) { BluskError::report("write_file(path, content)", "stdlib.io", 0); return Value::Nil(); }
    std::ofstream f(args[0].str);
    if (!f.is_open()) { BluskError::report("Cannot write: " + args[0].str, "stdlib.io", 0); return Value::Nil(); }
    f << args[1].toString();
    return Value::Bool(true);
}

// ── appendFile ───────────────────────────────────────────────────
Value appendFile(const std::vector<Value>& args) {
    if (args.size() < 2) { BluskError::report("append_file(path, content)", "stdlib.io", 0); return Value::Nil(); }
    std::ofstream f(args[0].str, std::ios::app);
    if (!f.is_open()) { BluskError::report("Cannot append: " + args[0].str, "stdlib.io", 0); return Value::Nil(); }
    f << args[1].toString();
    return Value::Bool(true);
}

// ── fileExists ───────────────────────────────────────────────────
Value fileExists(const std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Bool(false);
    struct stat buf;
    return Value::Bool(stat(args[0].str.c_str(), &buf) == 0);
}

// ── mmapRead : 메모리맵 고속 파일 읽기 (복사 없음) ───────────────
Value mmapRead(const std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) {
        BluskError::report("mmap_read requires a string path", "stdlib.io", 0);
        return Value::Nil();
    }
    const std::string& path = args[0].str;

#ifdef _WIN32
    HANDLE hFile = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
                               nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return Value::Nil();
    LARGE_INTEGER size; GetFileSizeEx(hFile, &size);
    HANDLE hMap = CreateFileMapping(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!hMap) { CloseHandle(hFile); return Value::Nil(); }
    const char* data = (const char*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    std::string result(data, (size_t)size.QuadPart);
    UnmapViewOfFile(data); CloseHandle(hMap); CloseHandle(hFile);
    return Value::String(result);
#else
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) return Value::Nil();
    struct stat st; fstat(fd, &st);
    const char* data = (const char*)mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    std::string result(data, st.st_size);
    munmap((void*)data, st.st_size); close(fd);
    return Value::String(result);
#endif
}

} // namespace BluskStd::IO
