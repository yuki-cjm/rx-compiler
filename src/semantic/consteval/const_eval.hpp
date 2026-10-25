#pragma once

#include "../../ast/ast.hpp"
#include "../symbol/symbol_table.hpp"
#include "../diag/diagnostic.hpp"
#include "../ty/ty.hpp"

#include <cstdint>
#include <optional>

namespace rx::semantic {

// 常量求值结果。
struct EvaluatedConst {
    TyPtr type;                 // i32/u32/isize/usize/bool
    std::uint64_t intValue = 0; // 整数类型时有效
    bool boolValue = false;     // bool 类型时有效
};

// 求值一个常量上下文中的常量表达式（ast::ConstValue）。
// 支持：整数/布尔字面量、常量路径（递归求值）、取负、括号。
// expectedType 为期望类型（常量项声明类型 / 数组长度的 usize）。
// 失败（类型不匹配、路径非常量、取负用于无符号等）返回 nullopt 并写入 diag。
std::optional<EvaluatedConst> evaluateConst(SymbolTable &table,
                                            Diagnostic &diag,
                                            const ast::ConstValue &value,
                                            const TyPtr &expectedType);

// 求数组长度：必须是 usize 常量。供 resolve 阶段调用。
// 失败返回 nullopt 并写入 diag。
std::optional<std::uint64_t>
evaluateArrayLength(SymbolTable &table, Diagnostic &diag,
                    const ast::ConstValue &value);

} // namespace rx::semantic
