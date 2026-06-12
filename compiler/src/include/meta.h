// =============================================================
//  BLUSK meta.h  -  bluskmeta.json 핸들러
//  버전, 메인클래스, 엔트리포인트 등 메타 정보 관리
// =============================================================
#pragma once
#include <string>
#include <vector>
#include <unordered_map>

struct BluskMeta {
    std::string              name        = "unnamed";
    std::string              version     = "0.1.0";
    std::string              author;
    std::string              description;
    std::vector<std::string> mainClasses;    // @entry 클래스 목록
    std::string              defaultEntry;   // 기본 실행 엔트리
    std::string              packageRoot;    // root package.xxx
    std::string              bluskVersion   = "0.7-alpha";
    std::vector<std::string> dependencies;  // 외부 .sky 의존성
    std::unordered_map<std::string, std::string> extra; // 커스텀 필드
};

class MetaHandler {
public:
    // bluskmeta.json 로드
    static BluskMeta load(const std::string& path = "bluskmeta.json");

    // bluskmeta.json 저장
    static bool save(const BluskMeta& meta,
                     const std::string& path = "bluskmeta.json");

    // 기본 메타 생성 (새 프로젝트)
    static BluskMeta defaultMeta(const std::string& name = "my_project");

    // 메타 정보 출력
    static void print(const BluskMeta& meta);

    // @entry 클래스 등록
    static void registerEntry(BluskMeta& meta, const std::string& className);

    // root package 이름 추출 ("root package.example.sky" → "example.sky")
    static std::string extractPackageName(const std::string& rootDecl);

private:
    static std::string toJson(const BluskMeta& meta);
    static BluskMeta   fromJson(const std::string& json);
};
