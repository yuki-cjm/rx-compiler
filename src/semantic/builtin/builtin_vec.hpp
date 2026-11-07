#pragma once

#include "builtin.hpp"

#include <optional>
#include <string>

namespace rx::semantic {

class VecBuiltin {
  public:
    static TypePtr makeType(TypePtr element);

    // new/len/is_empty/push/remove/clone
    static std::optional<BuiltinFnSig> lookup(const std::string &name, const TypePtr &element);
};

} // namespace rx::semantic
