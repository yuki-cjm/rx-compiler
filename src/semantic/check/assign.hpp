#pragma once

#include "../../ast/ast.hpp"
#include "../symbol/symbol_table.hpp"
#include "../diag/diagnostic.hpp"
#include "../ty/ty.hpp"
#include "place.hpp"

namespace rx::semantic {

// 赋值 / 复合赋值检查（规范 operator-expr.html 的「Borrow, dereference, and
// assignment」与「Assignment destinations」）。
class AssignChecker {
  public:
    // 检查 place = value：目标必须是可写 place，且值类型可强制到目标类型。
    static bool check(const SymbolTable &table, const PlaceInfo &target,
                      const TyPtr &valueType, Diagnostic &diag);

    // 检查复合赋值（如 place += value）：目标必须是可写 place，
    // 且右值类型满足对应运算符的规则（不推断右值整数类型）。
    static bool checkCompound(const SymbolTable &table, const PlaceInfo &target,
                              ast::BinaryOp op, const TyPtr &rightType,
                              Diagnostic &diag);
};

} // namespace rx::semantic
