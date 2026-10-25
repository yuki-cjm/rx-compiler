#pragma once

#include "../../ast/ast.hpp"
#include "../ty/ty.hpp"

#include <optional>

namespace rx::semantic {

// 运算符类型规则（规范 operator-expr.html）。
// 纯函数：给定操作数类型，返回结果类型；不合法返回 nullopt。
class Operators {
  public:
    // 一元运算（- / ! / * / & / &mut / && / &&mut）结果类型。
    static std::optional<TyPtr> unaryResult(ast::UnaryOp op,
                                            const TyPtr &operand);

    // 二元算术 / 位 / 移位 / 比较 / 逻辑运算结果类型。
    // 注意：赋值类（= 与复合赋值）返回 Unit，左值合法性由 assign 模块检查。
    static std::optional<TyPtr> binaryResult(ast::BinaryOp op,
                                             const TyPtr &left,
                                             const TyPtr &right);

    // 是否为普通赋值（=）。
    static bool isAssignment(ast::BinaryOp op);

    // 是否为复合赋值（+= -= *= /= %= &= |= ^= <<= >>=）。
    static bool isCompoundAssignment(ast::BinaryOp op);

    // 复合赋值：校验右值类型是否与左值类型兼容（对应运算符的右操作数规则）。
    static bool checkCompoundAssignment(ast::BinaryOp op, const TyPtr &left,
                                        const TyPtr &right);
};

} // namespace rx::semantic
