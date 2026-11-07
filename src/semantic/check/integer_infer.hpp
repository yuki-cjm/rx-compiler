#pragma once

#include "../type/type.hpp"

#include <optional>
#include <string>

namespace rx::semantic {

// 整数字面量推断
class IntegerInfer {
  public:
    static TypePtr infer(const std::string &literalText, const std::optional<TypePtr> &expected);
};

} // namespace rx::semantic
