// =============================================================
//  BLUSK stdlib/blusk_collections.h  -  컬렉션 라이브러리
// =============================================================
#pragma once
#include "../compiler/src/include/value.h"
#include <vector>

namespace BluskStd::Collections {
    // Array 조작
    Value push    (const std::vector<Value>& args); // push(arr, val)
    Value pop     (const std::vector<Value>& args); // pop(arr) -> val
    Value shift   (const std::vector<Value>& args); // shift(arr) -> val (앞에서 제거)
    Value unshift (const std::vector<Value>& args); // unshift(arr, val) (앞에 추가)
    Value arrLen  (const std::vector<Value>& args); // len(arr) -> int
    Value arrGet  (const std::vector<Value>& args); // get(arr, idx)
    Value arrSet  (const std::vector<Value>& args); // set(arr, idx, val)
    Value arrSlice(const std::vector<Value>& args); // slice(arr, start, end)
    Value arrSort (const std::vector<Value>& args); // sort(arr) -> sorted
    Value arrRev  (const std::vector<Value>& args); // reverse(arr)
    Value arrMap  (const std::vector<Value>& args); // map(arr, fn_name) -- 향후 람다 연동
    Value arrFind (const std::vector<Value>& args); // find(arr, val) -> idx
    Value arrUniq (const std::vector<Value>& args); // unique(arr)
    Value arrFlatten(const std::vector<Value>& args);// flatten(arr)

    // HashMap (Value::OBJECT 기반)
    Value mapNew  (const std::vector<Value>& args); // map_new() -> object
    Value mapGet  (const std::vector<Value>& args); // map_get(map, key)
    Value mapSet  (const std::vector<Value>& args); // map_set(map, key, val)
    Value mapHas  (const std::vector<Value>& args); // map_has(map, key)
    Value mapDel  (const std::vector<Value>& args); // map_del(map, key)
    Value mapKeys (const std::vector<Value>& args); // map_keys(map) -> array
    Value mapVals (const std::vector<Value>& args); // map_vals(map) -> array
    Value mapLen  (const std::vector<Value>& args); // map_len(map) -> int
}
