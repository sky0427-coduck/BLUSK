// =============================================================
//  BLUSK module.h  -  실제 import 시스템
//  기존: import 키워드가 flag만 세팅
//  이후: 실제 .sky 파일 로드 + stdlib 등록
// =============================================================
#pragma once
#include "value.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

// ------------------------------------------------------------------
//  NativeFunc : C++ 함수를 BLUSK에서 호출 가능하게 등록
// ------------------------------------------------------------------
using NativeFunc = std::function<Value(const std::vector<Value>&)>;

// ------------------------------------------------------------------
//  ModuleEntry : 모듈 하나의 내보내기 목록
// ------------------------------------------------------------------
struct ModuleEntry {
    std::string name;
    std::unordered_map<std::string, NativeFunc> funcs;
    std::unordered_map<std::string, Value>       consts;
};

// ------------------------------------------------------------------
//  ModuleRegistry : 전역 모듈 레지스트리
// ------------------------------------------------------------------
class ModuleRegistry {
public:
    static ModuleRegistry& instance() {
        static ModuleRegistry reg;
        return reg;
    }

    // 모듈 등록 (stdlib 초기화 시 호출)
    void registerModule(const std::string& name, ModuleEntry entry);

    // 함수 조회 (import 후 호출)
    NativeFunc* getFunc(const std::string& moduleName,
                        const std::string& funcName);

    // 상수 조회
    const Value* getConst(const std::string& moduleName,
                          const std::string& constName);

    // 모듈 존재 여부
    bool hasModule(const std::string& name) const;

    // 모듈 목록
    std::vector<std::string> listModules() const;

    // .sky 파일 로드 (사용자 정의 모듈)
    bool loadSkyFile(const std::string& path, const std::string& alias = "");

private:
    ModuleRegistry() { initStdlib(); }
    void initStdlib(); // 표준 라이브러리 자동 등록

    std::unordered_map<std::string, ModuleEntry> modules_;
};

// ------------------------------------------------------------------
//  import 파싱 결과
// ------------------------------------------------------------------
struct ImportSpec {
    std::string modulePath;   // "Blusk.num.Math" or "io"
    std::string alias;        // as 키워드 (향후 지원)
    bool        isSkyFile = false;
    bool        isWildcard= false;
};

// ------------------------------------------------------------------
//  ModuleLoader : import 문 처리
// ------------------------------------------------------------------
class ModuleLoader {
public:
    explicit ModuleLoader(const std::string& baseDir = ".");

    // import 문 처리 → 활성화된 모듈 목록 반환
    std::vector<std::string> processImport(const ImportSpec& spec);

    // 임포트된 함수 조회 (SVM에서 CALL 시)
    NativeFunc* resolve(const std::string& name);

    // 임포트된 상수 조회
    const Value* resolveConst(const std::string& name);

    // 활성 모듈 목록
    const std::vector<std::string>& activeModules() const { return active_; }

private:
    std::string baseDir_;
    std::vector<std::string> active_;

    // "func_name" → NativeFunc 매핑 (활성 임포트 기준)
    std::unordered_map<std::string, NativeFunc> resolved_;
    std::unordered_map<std::string, Value>       resolvedConsts_;

    std::string normalizeModuleName(const std::string& raw);
};
