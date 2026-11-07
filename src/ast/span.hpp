#pragma once

#include <cstddef>

namespace rx::ast {

struct Span {
    std::size_t line = 0;
    std::size_t column = 0;
};

} // namespace rx::ast
