#pragma once

#include "../../ast/ast.hpp"
#include "../type/type.hpp"

#include <optional>

namespace rx::semantic {

// 运算符类型规则
class Operators {
  public:
    // 一元运算符号： - / ! / * / & / &mut / && / &&mut
    static std::optional<TypePtr> unaryResult(ast::UnaryOp op, const TypePtr &operand);

    // 二元算术 / 位 / 移位 / 比较 / 逻辑运算结果类型。
    static std::optional<TypePtr> binaryResult(ast::BinaryOp op, const TypePtr &left, const TypePtr &right);

    // 普通赋值：=
    static bool isAssignment(ast::BinaryOp op);

    // 复合赋值：+= -= *= /= %= &= |= ^= <<= >>=
    static bool isCompoundAssignment(ast::BinaryOp op);

    // 复合赋值：校验右值类型是否与左值类型兼容
    static bool checkCompoundAssignment(ast::BinaryOp op, const TypePtr &left, const TypePtr &right);
};

} // namespace rx::semantic
