#pragma once

#include "builtin.hpp"

#include <string>

namespace rx::semantic {

// 注册标量类型（i32/u32/isize/usize/bool）到 type 命名空间，
// 以及 I/O 内建函数（get_i32/print_i32/println_i32）到 value 命名空间。
void registerScalarBuiltins(SymbolTable &table);

// ---- 保护名查询（规范 names.html）----

// type 命名空间的保护名：i32/u32/isize/usize/bool/Box/Vec/Copy/Clone/PartialEq/Eq。
// 其中「i32」的重声明是编译错误；其余大部分重声明是 undefined behavior，
// 是否诊断由 duplicate_check 决定。
bool isProtectedTypeName(const std::string &name);

// value 命名空间的保护名：get_i32/print_i32/println_i32。
// 顶层函数 / 常量重声明这些名字是编译错误。
bool isProtectedValueName(const std::string &name);

} // namespace rx::semantic
