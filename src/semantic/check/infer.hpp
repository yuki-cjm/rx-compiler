#pragma once

#include "../ty/ty.hpp"

#include <optional>
#include <string>

namespace rx::semantic {

// 整数字面量推断（规范 types.html 的「Integer literals」）。
class IntegerInfer {
  public:
    // 判断字面量文本是否带类型后缀（如 1u32 / 0xffi32）。
    static bool hasSuffix(const std::string &literalText);

    // 决定无后缀整数 / 有后缀整数的类型：
    //   1) 后缀固定类型；
    //   2) 无后缀时，若期望类型是四种整数之一则用之；
    //   3) 否则默认 i32。
    static TyPtr infer(const std::string &literalText,
                       const std::optional<TyPtr> &expected);
};

} // namespace rx::semantic
