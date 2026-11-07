#pragma once

#include <cstddef>

namespace rx::ast {

// 源码位置（行 / 列，从 1 开始；0 表示未知）。
struct Span {
    std::size_t line = 0;
    std::size_t column = 0;
};

} // namespace rx::ast
