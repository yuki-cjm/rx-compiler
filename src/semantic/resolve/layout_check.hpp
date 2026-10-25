#pragma once

#include "../../ast/ast.hpp"
#include "../symbol/symbol_table.hpp"
#include "../diag/diagnostic.hpp"

namespace rx::semantic {

// 递归布局检查（规范 types.html 的「Recursive types」）。
// 按值 struct / 数组包含环没有有限大小，是编译错误；
// 经 Reference / Box / Vec 的间接递归是合法的。
class LayoutChecker {
  public:
    LayoutChecker(SymbolTable &table, Diagnostic &diag);

    // 检查程序中所有结构体的递归布局。
    void check(const ast::Program &program);

  private:
    SymbolTable &table_;
    Diagnostic &diag_;
};

} // namespace rx::semantic
