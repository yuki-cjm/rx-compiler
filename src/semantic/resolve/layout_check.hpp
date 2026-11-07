#pragma once

#include "../../ast/ast.hpp"
#include "../symbol/symbol_table.hpp"
#include "../diag/diagnostic.hpp"

namespace rx::semantic {

// 递归布局检查
class LayoutChecker {
  public:
    LayoutChecker(SymbolTable &table, Diagnostic &diag);
    void check(const ast::Program &program);

  private:
    SymbolTable &table_;
    Diagnostic &diag_;
};

} // namespace rx::semantic
