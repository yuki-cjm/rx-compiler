#pragma once

#include "../../ast/ast.hpp"
#include "../symbol/symbol.hpp"
#include "../diag/diagnostic.hpp"

namespace rx::semantic {

// 各种「重名 / 保护名」检查，被 Collector 调用。
// 顶层 type / value 命名空间的重名由 SymbolTable::declare* 的返回值检测，
// 这里负责结构体内部（字段）与函数内部（参数）的重名，以及保护名冲突。

// 检查结构体字段是否重名（规范：字段在同一结构体命名空间内必须唯一）。
void checkDuplicateFields(const ast::StructItem &item, Diagnostic &diag);

// 检查函数参数是否重名。
void checkDuplicateParameters(const ast::FunctionItem &item, Diagnostic &diag);

// 检查符号是否与内建保护名冲突（依据其所属命名空间）。
// 顶层 type 名 = i32 之类是编译错误；顶层 value 名 = get_i32 之类是编译错误。
void checkProtectedName(const Symbol &symbol, Diagnostic &diag);

} // namespace rx::semantic
