#pragma once

#include "../ast/ast.hpp"
#include "symbol/symbol_table.hpp"
#include "diag/diagnostic.hpp"

namespace rx::semantic {

// 串联所有 pass：collect -> resolve→ typeck -> consteval→ main → layout
class SemanticAnalyzer {
  public:
    bool check(const ast::Program &program);

    const Diagnostic &diagnostics() const;

  private:
    Diagnostic diag_;
    SymbolTable table_;
};

} // namespace rx::semantic
