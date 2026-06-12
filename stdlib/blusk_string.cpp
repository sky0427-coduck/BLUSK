// =============================================================
//  BLUSK stdlib/blusk_string.cpp
// =============================================================
#include "blusk_string.h"
#include "../compiler/src/include/error.h"
#include <algorithm>
#include <sstream>
#include <cctype>

namespace BluskStd::Str {

Value length(const std::vector<Value>& args) {
    if (args.empty()) return Value::Int(0);
    if (args[0].isString()) return Value::Int((int64_t)args[0].str.size());
    return Value::Int((int64_t)args[0].toString().size());
}

Value substr(const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString()) return Value::Nil();
    const std::string& s = args[0].str;
    int64_t start = args[1].toInt();
    int64_t len   = (args.size() >= 3) ? args[2].toInt() : (int64_t)s.size();
    if (start < 0) start = 0;
    if (start >= (int64_t)s.size()) return Value::String("");
    return Value::String(s.substr((size_t)start, (size_t)len));
}

Value indexOf(const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString()) return Value::Int(-1);
    size_t pos = args[0].str.find(args[1].toString());
    return Value::Int(pos == std::string::npos ? -1 : (int64_t)pos);
}

Value contains(const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString()) return Value::Bool(false);
    return Value::Bool(args[0].str.find(args[1].toString()) != std::string::npos);
}

Value replace(const std::vector<Value>& args) {
    if (args.size() < 3 || !args[0].isString()) return Value::Nil();
    std::string s = args[0].str;
    const std::string& from = args[1].toString();
    const std::string& to   = args[2].toString();
    if (from.empty()) return Value::String(s);
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return Value::String(s);
}

Value toUpper(const std::vector<Value>& args) {
    if (args.empty()) return Value::String("");
    std::string s = args[0].toString();
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    return Value::String(s);
}

Value toLower(const std::vector<Value>& args) {
    if (args.empty()) return Value::String("");
    std::string s = args[0].toString();
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return Value::String(s);
}

Value trim(const std::vector<Value>& args) {
    if (args.empty()) return Value::String("");
    std::string s = args[0].toString();
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end   = s.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) return Value::String("");
    return Value::String(s.substr(start, end - start + 1));
}

Value split(const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString()) return Value::Array();
    const std::string& s = args[0].str;
    std::string delim = args[1].toString();
    Value arr = Value::Array();
    if (delim.empty()) {
        for (char c : s) arr.arr->push_back(Value::String(std::string(1, c)));
        return arr;
    }
    size_t pos = 0, prev = 0;
    while ((pos = s.find(delim, prev)) != std::string::npos) {
        arr.arr->push_back(Value::String(s.substr(prev, pos - prev)));
        prev = pos + delim.size();
    }
    arr.arr->push_back(Value::String(s.substr(prev)));
    return arr;
}

Value join(const std::vector<Value>& args) {
    if (args.empty() || !args[0].isArr()) return Value::String("");
    std::string delim = (args.size() >= 2) ? args[1].toString() : "";
    std::ostringstream oss;
    const auto& vec = *args[0].arr;
    for (size_t i = 0; i < vec.size(); i++) {
        if (i > 0) oss << delim;
        oss << vec[i].toString();
    }
    return Value::String(oss.str());
}

Value startsWith(const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString()) return Value::Bool(false);
    const std::string& s = args[0].str;
    const std::string  p = args[1].toString();
    return Value::Bool(s.size() >= p.size() && s.substr(0, p.size()) == p);
}

Value endsWith(const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString()) return Value::Bool(false);
    const std::string& s = args[0].str;
    const std::string  p = args[1].toString();
    return Value::Bool(s.size() >= p.size() && s.substr(s.size() - p.size()) == p);
}

Value parseInt(const std::vector<Value>& args) {
    if (args.empty()) return Value::Int(0);
    try { return Value::Int(std::stoll(args[0].toString())); }
    catch (...) { return Value::Nil(); }
}

Value parseFloat(const std::vector<Value>& args) {
    if (args.empty()) return Value::Float(0.0);
    try { return Value::Float(std::stod(args[0].toString())); }
    catch (...) { return Value::Nil(); }
}

Value format(const std::vector<Value>& args) {
    if (args.empty()) return Value::String("");
    std::string tmpl = args[0].toString();
    std::string result;
    size_t argIdx = 1;
    for (size_t i = 0; i < tmpl.size(); i++) {
        if (tmpl[i] == '{' && i+1 < tmpl.size() && tmpl[i+1] == '}') {
            if (argIdx < args.size()) result += args[argIdx++].toString();
            i++;
        } else result += tmpl[i];
    }
    return Value::String(result);
}

} // namespace BluskStd::Str
