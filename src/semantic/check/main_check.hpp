#pragma once

#include "../symbol/symbol_table.hpp"
#include "../diag/diagnostic.hpp"

namespace rx::semantic {

class MainChecker {
  public:
    MainChecker(SymbolTable &table, Diagnostic &diag);

    void check();

  private:
    SymbolTable &table_;
    Diagnostic &diag_;
};

} // namespace rx::semantic
