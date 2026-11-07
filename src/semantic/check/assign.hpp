#pragma once

#include "../../ast/ast.hpp"
#include "../symbol/symbol_table.hpp"
#include "../diag/diagnostic.hpp"
#include "../type/type.hpp"
#include "place.hpp"

namespace rx::semantic {

class AssignChecker {
  public:
    // assign
    static bool check(const SymbolTable &table, const PlaceInfo &target,
                      const TypePtr &valueType, Diagnostic &diag);

    // compound assignment
    static bool checkCompound(const SymbolTable &table, const PlaceInfo &target,
                              ast::BinaryOp op, const TypePtr &rightType,
                              Diagnostic &diag);
};

} // namespace rx::semantic
