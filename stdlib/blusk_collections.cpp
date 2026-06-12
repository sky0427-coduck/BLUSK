// =============================================================
//  BLUSK stdlib/blusk_collections.cpp
// =============================================================
#include "blusk_collections.h"
#include "../compiler/src/include/error.h"
#include <algorithm>
#include <unordered_set>

namespace BluskStd::Collections {

// ── Array ─────────────────────────────────────────────────────────

Value push(const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArr()) return Value::Nil();
    args[0].arr->push_back(args[1]);
    return Value::Int((int64_t)args[0].arr->size());
}
Value pop(const std::vector<Value>& args) {
    if (args.empty() || !args[0].isArr() || args[0].arr->empty()) return Value::Nil();
    Value v = args[0].arr->back();
    args[0].arr->pop_back();
    return v;
}
Value shift(const std::vector<Value>& args) {
    if (args.empty() || !args[0].isArr() || args[0].arr->empty()) return Value::Nil();
    Value v = args[0].arr->front();
    args[0].arr->erase(args[0].arr->begin());
    return v;
}
Value unshift(const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArr()) return Value::Nil();
    args[0].arr->insert(args[0].arr->begin(), args[1]);
    return Value::Int((int64_t)args[0].arr->size());
}
Value arrLen(const std::vector<Value>& args) {
    if (args.empty() || !args[0].isArr()) return Value::Int(0);
    return Value::Int((int64_t)args[0].arr->size());
}
Value arrGet(const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArr()) return Value::Nil();
    int64_t idx = args[1].toInt();
    auto& v = *args[0].arr;
    if (idx < 0 || idx >= (int64_t)v.size()) return Value::Nil();
    return v[(size_t)idx];
}
Value arrSet(const std::vector<Value>& args) {
    if (args.size() < 3 || !args[0].isArr()) return Value::Nil();
    int64_t idx = args[1].toInt();
    auto& v = *args[0].arr;
    if (idx < 0 || idx >= (int64_t)v.size()) return Value::Nil();
    v[(size_t)idx] = args[2];
    return Value::Bool(true);
}
Value arrSlice(const std::vector<Value>& args) {
    if (args.empty() || !args[0].isArr()) return Value::Array();
    auto& v = *args[0].arr;
    int64_t start = args.size() >= 2 ? args[1].toInt() : 0;
    int64_t end   = args.size() >= 3 ? args[2].toInt() : (int64_t)v.size();
    if (start < 0) start = 0;
    if (end > (int64_t)v.size()) end = (int64_t)v.size();
    Value result = Value::Array();
    for (int64_t i = start; i < end; i++) result.arr->push_back(v[(size_t)i]);
    return result;
}
Value arrSort(const std::vector<Value>& args) {
    if (args.empty() || !args[0].isArr()) return Value::Nil();
    Value copy = args[0];
    std::sort(copy.arr->begin(), copy.arr->end(),
              [](const Value& a, const Value& b){ return a < b; });
    return copy;
}
Value arrRev(const std::vector<Value>& args) {
    if (args.empty() || !args[0].isArr()) return Value::Nil();
    Value copy = args[0];
    std::reverse(copy.arr->begin(), copy.arr->end());
    return copy;
}
Value arrFind(const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArr()) return Value::Int(-1);
    auto& v = *args[0].arr;
    for (size_t i = 0; i < v.size(); i++)
        if (v[i] == args[1]) return Value::Int((int64_t)i);
    return Value::Int(-1);
}
Value arrUniq(const std::vector<Value>& args) {
    if (args.empty() || !args[0].isArr()) return Value::Array();
    Value result = Value::Array();
    std::unordered_set<std::string> seen;
    for (auto& v : *args[0].arr) {
        std::string key = v.toString();
        if (!seen.count(key)) { seen.insert(key); result.arr->push_back(v); }
    }
    return result;
}
Value arrFlatten(const std::vector<Value>& args) {
    if (args.empty() || !args[0].isArr()) return Value::Array();
    Value result = Value::Array();
    for (auto& v : *args[0].arr) {
        if (v.isArr()) for (auto& inner : *v.arr) result.arr->push_back(inner);
        else result.arr->push_back(v);
    }
    return result;
}
Value arrMap(const std::vector<Value>& args) {
    // 향후 람다 연동 - 지금은 stub
    if (args.empty() || !args[0].isArr()) return Value::Array();
    return args[0]; // TODO: 람다 연동 후 구현
}

// ── HashMap (Value::OBJECT 기반) ──────────────────────────────────
Value mapNew(const std::vector<Value>&) {
    auto obj = std::make_shared<BluskObject>();
    obj->className = "__map__";
    return Value::Object(obj);
}
Value mapGet(const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isObj() || !args[0].obj) return Value::Nil();
    auto it = args[0].obj->fields.find(args[1].toString());
    return it != args[0].obj->fields.end() ? it->second : Value::Nil();
}
Value mapSet(const std::vector<Value>& args) {
    if (args.size() < 3 || !args[0].isObj() || !args[0].obj) return Value::Nil();
    args[0].obj->fields[args[1].toString()] = args[2];
    return Value::Bool(true);
}
Value mapHas(const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isObj() || !args[0].obj) return Value::Bool(false);
    return Value::Bool(args[0].obj->fields.count(args[1].toString()) > 0);
}
Value mapDel(const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isObj() || !args[0].obj) return Value::Bool(false);
    return Value::Bool(args[0].obj->fields.erase(args[1].toString()) > 0);
}
Value mapKeys(const std::vector<Value>& args) {
    Value result = Value::Array();
    if (args.empty() || !args[0].isObj() || !args[0].obj) return result;
    for (auto& [k, _] : args[0].obj->fields) result.arr->push_back(Value::String(k));
    return result;
}
Value mapVals(const std::vector<Value>& args) {
    Value result = Value::Array();
    if (args.empty() || !args[0].isObj() || !args[0].obj) return result;
    for (auto& [_, v] : args[0].obj->fields) result.arr->push_back(v);
    return result;
}
Value mapLen(const std::vector<Value>& args) {
    if (args.empty() || !args[0].isObj() || !args[0].obj) return Value::Int(0);
    return Value::Int((int64_t)args[0].obj->fields.size());
}

} // namespace BluskStd::Collections
