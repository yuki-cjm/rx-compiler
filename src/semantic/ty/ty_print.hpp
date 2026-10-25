#pragma once

#include "ty.hpp"

#include <string>

namespace rx::semantic {

class SymbolTable;

// 将语义类型渲染为人类可读字符串，用于诊断信息。
// 例如：i32、&mut T、[i32; 4]、Box<Vec<i32>>、Foo（结构体名）。
//
// 不带符号表的版本无法解析结构体名，Struct 退化为 "struct#<id>" 占位形式。
std::string typeToString(const TyPtr &ty);

// 带符号表的版本：可把 StructId 解析为结构体名，输出更友好。
std::string typeToString(const TyPtr &ty, const SymbolTable &table);

} // namespace rx::semantic
