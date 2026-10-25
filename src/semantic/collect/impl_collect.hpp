#pragma once

#include "../../ast/ast.hpp"
#include "../symbol/symbol_table.hpp"
#include "../diag/diagnostic.hpp"

namespace rx::semantic {

// inherent impl 关联项收集。
// 把 impl 块里的方法 / 关联函数 / 关联常量注册进目标结构体的关联命名空间。
// 所有 impl 块共享同一关联值命名空间：跨 impl 块定义同名方法 / 关联常量是编译错误。
class ImplCollector {
  public:
    ImplCollector(SymbolTable &table, Diagnostic &diag);

    // 收集单个 impl 块。目标类型必须是用户定义结构体（简单路径类型），
    // 否则报错（inherent impl 只能用于用户定义 struct）。
    void collect(const ast::ImplItem &item);

  private:
    SymbolTable &table_;
    Diagnostic &diag_;
};

} // namespace rx::semantic
