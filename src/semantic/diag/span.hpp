#pragma once

#include <cstddef>

namespace rx::semantic {

// 源码位置（行 / 列，从 1 开始）。
// 当前 AST 尚未携带位置信息；此类型为后续增强诊断预留，
// 也用于 Diagnostic 中带位置的报错重载。
struct Span {
    std::size_t line = 0;    // 0 表示未知
    std::size_t column = 0;  // 0 表示未知
};

} // namespace rx::semantic
