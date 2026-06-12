// =============================================================
//  BLUSK module.cpp  -  모듈 시스템 구현
// =============================================================
#include "../include/module.h"
#include "../../../stdlib/blusk_io.h"
#include "../../../stdlib/blusk_string.h"
#include "../../../stdlib/blusk_math.h"
#include "../../../stdlib/blusk_time.h"
#include "../../../stdlib/blusk_collections.h"
#include "../include/error.h"
#include <iostream>
#include <algorithm>

// ──────────────────────────────────────────────────────────────────
//  표준 라이브러리 자동 등록
// ──────────────────────────────────────────────────────────────────
void ModuleRegistry::initStdlib() {
    // ── io ───────────────────────────────────────────────────────
    {
        ModuleEntry m; m.name = "io";
        m.funcs["print"]       = BluskStd::IO::print;
        m.funcs["println"]     = BluskStd::IO::println;
        m.funcs["input"]       = BluskStd::IO::input;
        m.funcs["read_file"]   = BluskStd::IO::readFile;
        m.funcs["write_file"]  = BluskStd::IO::writeFile;
        m.funcs["append_file"] = BluskStd::IO::appendFile;
        m.funcs["file_exists"] = BluskStd::IO::fileExists;
        m.funcs["mmap_read"]   = BluskStd::IO::mmapRead;
        modules_["io"] = m;
    }
    // ── string ───────────────────────────────────────────────────
    {
        ModuleEntry m; m.name = "string";
        m.funcs["len"]         = BluskStd::Str::length;
        m.funcs["substr"]      = BluskStd::Str::substr;
        m.funcs["index_of"]    = BluskStd::Str::indexOf;
        m.funcs["contains"]    = BluskStd::Str::contains;
        m.funcs["replace"]     = BluskStd::Str::replace;
        m.funcs["upper"]       = BluskStd::Str::toUpper;
        m.funcs["lower"]       = BluskStd::Str::toLower;
        m.funcs["trim"]        = BluskStd::Str::trim;
        m.funcs["split"]       = BluskStd::Str::split;
        m.funcs["join"]        = BluskStd::Str::join;
        m.funcs["starts_with"] = BluskStd::Str::startsWith;
        m.funcs["ends_with"]   = BluskStd::Str::endsWith;
        m.funcs["parse_int"]   = BluskStd::Str::parseInt;
        m.funcs["parse_float"] = BluskStd::Str::parseFloat;
        m.funcs["format"]      = BluskStd::Str::format;
        modules_["string"] = m;
    }
    // ── Blusk.num.Math ───────────────────────────────────────────
    {
        ModuleEntry m; m.name = "Blusk.num.Math";
        m.funcs["sqrt"]    = BluskStd::Math::sqrt;
        m.funcs["pow"]     = BluskStd::Math::pow;
        m.funcs["abs"]     = BluskStd::Math::abs;
        m.funcs["ceil"]    = BluskStd::Math::ceil;
        m.funcs["floor"]   = BluskStd::Math::floor;
        m.funcs["round"]   = BluskStd::Math::round;
        m.funcs["sin"]     = BluskStd::Math::sin;
        m.funcs["cos"]     = BluskStd::Math::cos;
        m.funcs["tan"]     = BluskStd::Math::tan;
        m.funcs["asin"]    = BluskStd::Math::asin;
        m.funcs["acos"]    = BluskStd::Math::acos;
        m.funcs["atan"]    = BluskStd::Math::atan;
        m.funcs["atan2"]   = BluskStd::Math::atan2;
        m.funcs["exp"]     = BluskStd::Math::exp;
        m.funcs["log"]     = BluskStd::Math::log;
        m.funcs["log10"]   = BluskStd::Math::log10;
        m.funcs["log2"]    = BluskStd::Math::log2;
        m.funcs["min"]     = BluskStd::Math::min;
        m.funcs["max"]     = BluskStd::Math::max;
        m.funcs["clamp"]   = BluskStd::Math::clamp;
        m.funcs["lerp"]    = BluskStd::Math::lerp;
        m.funcs["random"]  = BluskStd::Math::random;
        m.funcs["rand_int"]= BluskStd::Math::randInt;
        m.funcs["invsqrt"] = BluskStd::Math::invsqrt;
        m.funcs["gcd"]     = BluskStd::Math::gcd;
        m.funcs["lcm"]     = BluskStd::Math::lcm;
        m.funcs["is_prime"]= BluskStd::Math::isPrime;
        m.consts["PI"] = Value::Float(3.14159265358979323846);
        m.consts["E"]  = Value::Float(2.71828182845904523536);
        modules_["Blusk.num.Math"] = m;
        modules_["blusk.math"]     = m; // alias
    }
    // ── time ─────────────────────────────────────────────────────
    {
        ModuleEntry m; m.name = "time";
        m.funcs["now"]       = BluskStd::Time::now;
        m.funcs["perf_now"]  = BluskStd::Time::perfNow;
        m.funcs["sleep"]     = BluskStd::Time::sleep;
        m.funcs["timestamp"] = BluskStd::Time::timestamp;
        m.funcs["date"]      = BluskStd::Time::date;
        m.funcs["elapsed"]   = BluskStd::Time::elapsed;
        modules_["time"] = m;
    }
    // ── collections ──────────────────────────────────────────────
    {
        ModuleEntry m; m.name = "collections";
        m.funcs["push"]     = BluskStd::Collections::push;
        m.funcs["pop"]      = BluskStd::Collections::pop;
        m.funcs["shift"]    = BluskStd::Collections::shift;
        m.funcs["unshift"]  = BluskStd::Collections::unshift;
        m.funcs["arr_len"]  = BluskStd::Collections::arrLen;
        m.funcs["arr_get"]  = BluskStd::Collections::arrGet;
        m.funcs["arr_set"]  = BluskStd::Collections::arrSet;
        m.funcs["slice"]    = BluskStd::Collections::arrSlice;
        m.funcs["sort"]     = BluskStd::Collections::arrSort;
        m.funcs["reverse"]  = BluskStd::Collections::arrRev;
        m.funcs["find"]     = BluskStd::Collections::arrFind;
        m.funcs["unique"]   = BluskStd::Collections::arrUniq;
        m.funcs["flatten"]  = BluskStd::Collections::arrFlatten;
        m.funcs["map_new"]  = BluskStd::Collections::mapNew;
        m.funcs["map_get"]  = BluskStd::Collections::mapGet;
        m.funcs["map_set"]  = BluskStd::Collections::mapSet;
        m.funcs["map_has"]  = BluskStd::Collections::mapHas;
        m.funcs["map_del"]  = BluskStd::Collections::mapDel;
        m.funcs["map_keys"] = BluskStd::Collections::mapKeys;
        m.funcs["map_vals"] = BluskStd::Collections::mapVals;
        m.funcs["map_len"]  = BluskStd::Collections::mapLen;
        modules_["collections"] = m;
    }
}

// ──────────────────────────────────────────────────────────────────
//  레지스트리 API
// ──────────────────────────────────────────────────────────────────
void ModuleRegistry::registerModule(const std::string& name, ModuleEntry entry) {
    modules_[name] = std::move(entry);
}

NativeFunc* ModuleRegistry::getFunc(const std::string& mod, const std::string& fn) {
    auto mit = modules_.find(mod);
    if (mit == modules_.end()) return nullptr;
    auto fit = mit->second.funcs.find(fn);
    return fit != mit->second.funcs.end() ? &fit->second : nullptr;
}

const Value* ModuleRegistry::getConst(const std::string& mod, const std::string& cn) {
    auto mit = modules_.find(mod);
    if (mit == modules_.end()) return nullptr;
    auto cit = mit->second.consts.find(cn);
    return cit != mit->second.consts.end() ? &cit->second : nullptr;
}

bool ModuleRegistry::hasModule(const std::string& name) const {
    return modules_.count(name) > 0;
}

std::vector<std::string> ModuleRegistry::listModules() const {
    std::vector<std::string> list;
    for (auto& [k, _] : modules_) list.push_back(k);
    return list;
}

bool ModuleRegistry::loadSkyFile(const std::string& path, const std::string& alias) {
    // TODO: .sky 파일 파싱 후 모듈 등록 (파서 연동 필요)
    std::cerr << "[Module] .sky file loading not yet implemented: " << path << "\n";
    return false;
}

// ──────────────────────────────────────────────────────────────────
//  ModuleLoader
// ──────────────────────────────────────────────────────────────────
ModuleLoader::ModuleLoader(const std::string& baseDir) : baseDir_(baseDir) {}

std::string ModuleLoader::normalizeModuleName(const std::string& raw) {
    // "Blusk.org.blusk.26.*" → "blusk26" 등 정규화
    std::string s = raw;
    if (s.find("Blusk.num.Math") != std::string::npos) return "Blusk.num.Math";
    if (s == "blusk26" || s == "Blusk26") return "blusk26";
    if (s == "io" || s == "IO") return "io";
    if (s == "string" || s == "Blusk.string") return "string";
    if (s == "time" || s == "Blusk.time") return "time";
    if (s == "collections" || s == "Blusk.collections") return "collections";
    if (s == "ai" || s == "AI") return "ai";
    return s;
}

std::vector<std::string> ModuleLoader::processImport(const ImportSpec& spec) {
    std::string name = normalizeModuleName(spec.modulePath);
    active_.push_back(name);

    auto& reg = ModuleRegistry::instance();
    if (!reg.hasModule(name)) {
        // .sky 파일 시도
        if (spec.isSkyFile) {
            reg.loadSkyFile(baseDir_ + "/" + spec.modulePath + ".sky", spec.alias);
        }
        // blusk26, ai 등 내부 플래그 모듈은 SVM이 처리
        return active_;
    }

    // 함수/상수 resolved 맵에 등록
    auto& mod = reg;
    auto modules = reg.listModules();
    for (auto& mn : modules) {
        if (mn != name) continue;
        // wildcard or all
        if (spec.isWildcard || true) {
            // 모든 함수 등록
            if (auto* fn = reg.getFunc(name, "")) {} // 전체 등록은 아래에서
        }
    }

    // 실제 함수 등록 (SVM OP_CALL 시 resolve()로 조회)
    std::cerr << "[Module] Loaded: " << name << "\n";
    return active_;
}

NativeFunc* ModuleLoader::resolve(const std::string& funcName) {
    // 활성 모듈에서 함수 탐색
    auto& reg = ModuleRegistry::instance();
    for (const auto& mod : active_) {
        if (auto* fn = reg.getFunc(mod, funcName)) return fn;
    }
    return nullptr;
}

const Value* ModuleLoader::resolveConst(const std::string& name) {
    auto& reg = ModuleRegistry::instance();
    for (const auto& mod : active_) {
        if (auto* c = reg.getConst(mod, name)) return c;
    }
    return nullptr;
}
