#pragma once

#include "../../ast/ast.hpp"
#include "../diag/diagnostic.hpp"
#include "../symbol/symbol.hpp"
#include "../symbol/symbol_table.hpp"
#include "../type/type.hpp"

namespace rx::semantic {

// 解析Path
class PathResolver {
  public:
    PathResolver(SymbolTable &table, Diagnostic &diag);

    const Symbol *resolveValuePath(const ast::PathInExpr &path, const TypePtr &selfType);

  private:
    SymbolTable &table_;
    Diagnostic &diag_;
};

} // namespace rx::semantic
