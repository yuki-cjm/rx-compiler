#pragma once

#include "../../ast/ast.hpp"
#include "../symbol/symbol_table.hpp"
#include "../diag/diagnostic.hpp"

namespace rx::semantic {

// Pass 0：顶层项收集。
// 遍历 Program.items，把结构体 / 函数 / 常量注册进符号表，
// 并把 impl 块的关联项注册进对应结构体的关联命名空间。
// 同时触发重名 / 保护名 / 重复字段 / 重复参数检查。
//
// 收集顺序（实现于 collector.cpp）：
//   1) 先声明所有 struct / function / constant，使类型与值命名空间就绪；
//   2) 再处理 impl 块（此时可解析目标结构体）。
// 这样保证「顶层查找与声明顺序无关」。
class Collector {
  public:
    // 收集整个程序；发现的错误写入 diag。
    void collect(const ast::Program &program, SymbolTable &table,
                 Diagnostic &diag);
};

} // namespace rx::semantic
