// =============================================================
//  BLUSK error.h  -  에러/경고 리포트
// =============================================================
#pragma once
#include <string>
#include <iostream>

class BluskError {
public:
    static void report(const std::string& msg, const std::string& file, int line) {
        std::cerr << "\n[BLUSK Error] " << msg << "\n";
        std::cerr << "  at " << file << ", line " << line << "\n\n";
    }

    static void warn(const std::string& msg, const std::string& file, int line) {
        std::cerr << "\n[BLUSK Warning] " << msg << "\n";
        std::cerr << "  at " << file << ", line " << line << "\n\n";
    }

    static void info(const std::string& msg, const std::string& file, int line) {
        std::cerr << "[BLUSK Info] " << msg
                  << "  (" << file << ":" << line << ")\n";
    }

    static void fatal(const std::string& msg) {
        std::cerr << "\n[BLUSK Fatal] " << msg << "\n";
        std::cerr << "  Program terminated.\n\n";
        exit(1);
    }
};
