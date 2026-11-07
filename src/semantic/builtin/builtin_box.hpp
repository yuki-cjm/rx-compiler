#pragma once

#include "builtin.hpp"

#include <optional>
#include <string>

namespace rx::semantic {

class BoxBuiltin {
  public:
    static TypePtr makeType(TypePtr element);
    static std::optional<BuiltinFnSig> lookup(const std::string &name,
                                              const TypePtr &element); // 内建
};

} // namespace rx::semantic
