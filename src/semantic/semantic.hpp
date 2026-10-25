#pragma once

#include "../ast/ast.hpp"
#include "symbol/symbol_table.hpp"
#include "diag/diagnostic.hpp"

namespace rx::semantic {

// 语义分析顶层入口。
// 串联所有 pass：收集（collect）→ 类型解析（resolve）→ 类型检查（typeck）
// → 常量求值（consteval，含环检测）→ main 检查（main_check）→ 布局检查（layout）。
//
// 用法：
//   SemanticAnalyzer sema;
//   bool ok = sema.check(program);
//   // ok == false 时，sema.diagnostics() 含全部错误消息。
class SemanticAnalyzer {
  public:
    // 对程序执行完整语义分析。返回 true 表示接受（无错误）。
    bool check(const ast::Program &program);

    // 诊断信息（含全部错误消息）。
    const Diagnostic &diagnostics() const;

  private:
    Diagnostic diag_;
    SymbolTable table_;
};

} // namespace rx::semantic
