#pragma once

#include "../symbol/symbol_table.hpp"
#include "../type/type.hpp"

#include <optional>
#include <vector>

namespace rx::semantic {

// 强制转换
// &mut T -> &T
// &S -> &T
// &mut S -> &T
// &mut S -> &mut T(不穿shared)
// never -> T
// 其中S->T(deref链)
class Coercion {
  public:
    // from -> to
    static bool canCoerce(const SymbolTable &table, const TypePtr &from, const TypePtr &to);

    static std::optional<TypePtr> leastUpperBound(const SymbolTable &table, const std::vector<TypePtr> &types);
};

} // namespace rx::semantic
