// =============================================================
//  BLUSK meta.cpp  -  bluskmeta.json 구현 (경량 JSON)
// =============================================================
#include "../include/meta.h"
#include "../include/error.h"
#include <fstream>
#include <sstream>
#include <iostream>

// ──────────────────────────────────────────────────────────────────
//  경량 JSON 직렬화 (외부 라이브러리 없이)
// ──────────────────────────────────────────────────────────────────
static std::string jsonStr(const std::string& s) {
    return "\"" + s + "\"";
}
static std::string jsonArr(const std::vector<std::string>& v) {
    std::string r = "[";
    for (size_t i = 0; i < v.size(); i++) {
        if (i > 0) r += ", ";
        r += jsonStr(v[i]);
    }
    return r + "]";
}

std::string MetaHandler::toJson(const BluskMeta& m) {
    std::ostringstream oss;
    oss << "{\n"
        << "  \"name\": "         << jsonStr(m.name)         << ",\n"
        << "  \"version\": "      << jsonStr(m.version)      << ",\n"
        << "  \"author\": "       << jsonStr(m.author)       << ",\n"
        << "  \"description\": "  << jsonStr(m.description)  << ",\n"
        << "  \"bluskVersion\": " << jsonStr(m.bluskVersion) << ",\n"
        << "  \"packageRoot\": "  << jsonStr(m.packageRoot)  << ",\n"
        << "  \"defaultEntry\": " << jsonStr(m.defaultEntry) << ",\n"
        << "  \"mainClasses\": "  << jsonArr(m.mainClasses)  << ",\n"
        << "  \"dependencies\": " << jsonArr(m.dependencies) << "\n"
        << "}\n";
    return oss.str();
}

// ──────────────────────────────────────────────────────────────────
//  경량 JSON 파싱 (key: "value" 추출)
// ──────────────────────────────────────────────────────────────────
static std::string extractStr(const std::string& json, const std::string& key) {
    std::string pat = "\"" + key + "\": \"";
    size_t p = json.find(pat);
    if (p == std::string::npos) return "";
    p += pat.size();
    size_t e = json.find('"', p);
    return e == std::string::npos ? "" : json.substr(p, e - p);
}

static std::vector<std::string> extractArr(const std::string& json,
                                            const std::string& key)
{
    std::vector<std::string> result;
    std::string pat = "\"" + key + "\": [";
    size_t p = json.find(pat);
    if (p == std::string::npos) return result;
    p += pat.size();
    size_t e = json.find(']', p);
    std::string arr = json.substr(p, e - p);
    size_t i = 0;
    while (i < arr.size()) {
        size_t s = arr.find('"', i);
        if (s == std::string::npos) break;
        size_t end = arr.find('"', s + 1);
        if (end == std::string::npos) break;
        result.push_back(arr.substr(s + 1, end - s - 1));
        i = end + 1;
    }
    return result;
}

BluskMeta MetaHandler::fromJson(const std::string& json) {
    BluskMeta m;
    m.name         = extractStr(json, "name");
    m.version      = extractStr(json, "version");
    m.author       = extractStr(json, "author");
    m.description  = extractStr(json, "description");
    m.bluskVersion = extractStr(json, "bluskVersion");
    m.packageRoot  = extractStr(json, "packageRoot");
    m.defaultEntry = extractStr(json, "defaultEntry");
    m.mainClasses  = extractArr(json, "mainClasses");
    m.dependencies = extractArr(json, "dependencies");
    return m;
}

// ──────────────────────────────────────────────────────────────────
//  공개 API
// ──────────────────────────────────────────────────────────────────
BluskMeta MetaHandler::load(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        // 파일 없으면 기본값 반환 (에러 아님)
        return defaultMeta();
    }
    std::ostringstream ss; ss << f.rdbuf();
    return fromJson(ss.str());
}

bool MetaHandler::save(const BluskMeta& meta, const std::string& path) {
    std::ofstream f(path);
    if (!f.is_open()) {
        BluskError::report("Cannot write bluskmeta.json", path, 0);
        return false;
    }
    f << toJson(meta);
    return true;
}

BluskMeta MetaHandler::defaultMeta(const std::string& name) {
    BluskMeta m;
    m.name = name;
    m.version = "0.1.0";
    m.bluskVersion = "0.7-alpha";
    return m;
}

void MetaHandler::print(const BluskMeta& m) {
    std::cerr << "[Meta] name="         << m.name
              << " version="            << m.version
              << " blusk="              << m.bluskVersion
              << " pkg="                << m.packageRoot << "\n"
              << "[Meta] entries: ";
    for (auto& e : m.mainClasses) std::cerr << e << " ";
    std::cerr << "\n";
}

void MetaHandler::registerEntry(BluskMeta& meta, const std::string& cls) {
    for (auto& e : meta.mainClasses)
        if (e == cls) return;
    meta.mainClasses.push_back(cls);
    if (meta.defaultEntry.empty()) meta.defaultEntry = cls;
}

std::string MetaHandler::extractPackageName(const std::string& rootDecl) {
    // "root package.example.sky" → "example.sky"
    size_t p = rootDecl.find("package.");
    if (p == std::string::npos) return "";
    return rootDecl.substr(p + 8);
}
