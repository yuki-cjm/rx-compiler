#pragma once

#include "../../ast/ast.hpp"
#include "../diag/diagnostic.hpp"
#include "../symbol/symbol_table.hpp"

namespace rx::semantic {

// Pass 0: 把名字注册进符号表，并做声明相关检查
class Collector {
  public:
    void collect(const ast::Program &program, SymbolTable &table,
                 Diagnostic &diag);
};

} // namespace rx::semantic
