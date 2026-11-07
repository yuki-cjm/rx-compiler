#pragma once

#include "../../ast/ast.hpp"
#include "../diag/diagnostic.hpp"

namespace rx::semantic {

void checkDuplicateFields(const ast::StructItem &item, Diagnostic &diag); // 结构体字段
void checkDuplicateParameters(const ast::FunctionItem &item, Diagnostic &diag); // 函数参数

} // namespace rx::semantic
