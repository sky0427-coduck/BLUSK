// =============================================================
//  BLUSK stdlib/blusk_io.h  -  I/O 표준 라이브러리
// =============================================================
#pragma once
#include "../compiler/src/include/value.h"
#include <vector>

namespace BluskStd::IO {
    Value print     (const std::vector<Value>& args); // print(val...)
    Value println   (const std::vector<Value>& args); // println(val...)
    Value input     (const std::vector<Value>& args); // input(prompt?) -> string
    Value readFile  (const std::vector<Value>& args); // read_file(path) -> string
    Value writeFile (const std::vector<Value>& args); // write_file(path, content)
    Value appendFile(const std::vector<Value>& args); // append_file(path, content)
    Value fileExists(const std::vector<Value>& args); // file_exists(path) -> bool
    Value mmapRead  (const std::vector<Value>& args); // mmap_read(path) -> string
}
