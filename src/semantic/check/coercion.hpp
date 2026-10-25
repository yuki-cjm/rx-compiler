#pragma once

#include "../symbol/symbol_table.hpp"
#include "../ty/ty.hpp"

#include <optional>
#include <vector>

namespace rx::semantic {

// 强制转换（规范 types.html 的「Coercion types」）。
// 支持一对一与 least upper bound（LUB）两类。
class Coercion {
  public:
    // 一对一：from 能否强制转换到 to。
    // 支持：T→T、&mut T→&T、&/&mut S→&T（沿内建 deref 链）、&mut S→&mut T、never→任意。
    static bool canCoerce(const SymbolTable &table, const TyPtr &from,
                          const TyPtr &to);

    // 求若干结果类型的 LUB（if 分支 / 数组元素 / 同一 loop 的 break 值）。
    // 忽略 never；按源码顺序确定目标。返回 nullopt 表示无法确定（UB）。
    static std::optional<TyPtr>
    leastUpperBound(const SymbolTable &table, const std::vector<TyPtr> &types);
};

} // namespace rx::semantic
