// =============================================================
//  BLUSK stdlib/blusk_string.h  -  문자열 라이브러리
// =============================================================
#pragma once
#include "../compiler/src/include/value.h"
#include <vector>

namespace BluskStd::Str {
    Value length  (const std::vector<Value>& args); // len(s) -> int
    Value substr  (const std::vector<Value>& args); // substr(s, start, len) -> string
    Value indexOf (const std::vector<Value>& args); // index_of(s, sub) -> int
    Value contains(const std::vector<Value>& args); // contains(s, sub) -> bool
    Value replace (const std::vector<Value>& args); // replace(s, old, new) -> string
    Value toUpper (const std::vector<Value>& args); // upper(s) -> string
    Value toLower (const std::vector<Value>& args); // lower(s) -> string
    Value trim    (const std::vector<Value>& args); // trim(s) -> string
    Value split   (const std::vector<Value>& args); // split(s, delim) -> array
    Value join    (const std::vector<Value>& args); // join(arr, delim) -> string
    Value startsWith(const std::vector<Value>& args);
    Value endsWith  (const std::vector<Value>& args);
    Value parseInt  (const std::vector<Value>& args); // parse_int(s) -> int
    Value parseFloat(const std::vector<Value>& args); // parse_float(s) -> float
    Value format    (const std::vector<Value>& args); // format(template, args...) -> string
}
