#pragma once

#include "../../ast/ast.hpp"
#include "../symbol/symbol_table.hpp"
#include "../diag/diagnostic.hpp"
#include "../type/type.hpp"

namespace rx::semantic {

// Pass 1：类型名解析、Box/Vec 类型实参个数校验、
// Self/SelfType 上下文处理、数组长度求值。
class TypeResolver {
  public:
    TypeResolver(SymbolTable &table, Diagnostic &diag);

    TypePtr resolve(const ast::Type &type, const TypePtr &selfType);

  private:
    TypePtr resolvePath(const ast::PathType &path, const TypePtr &selfType);

    SymbolTable &table_;
    Diagnostic &diag_;
};

} // namespace rx::semantic
