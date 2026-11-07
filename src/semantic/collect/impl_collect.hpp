#pragma once

#include "../../ast/ast.hpp"
#include "../symbol/symbol_table.hpp"
#include "../diag/diagnostic.hpp"

namespace rx::semantic {

class ImplCollector {
  public:
    ImplCollector(SymbolTable &table, Diagnostic &diag);
    void collect(const ast::ImplItem &item);

  private:
    SymbolTable &table_;
    Diagnostic &diag_;
};

} // namespace rx::semantic
