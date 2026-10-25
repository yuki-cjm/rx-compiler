#pragma once

#include "../../ast/ast.hpp"
#include "../symbol/symbol.hpp"
#include "../symbol/symbol_table.hpp"
#include "../diag/diagnostic.hpp"
#include "../ty/ty.hpp"

namespace rx::semantic {

// 类型路径与值路径的解析辅助，供 resolver 与 typeck 共用。
class PathResolver {
  public:
    PathResolver(SymbolTable &table, Diagnostic &diag);

    // 解析类型路径（如 `Foo`、`Foo::Bar`、`Self::T`）的首段 / 整体，
    // 返回最终解析到的类型符号；失败返回 nullptr 并写入 diag。
    // selfType 为当前 Self 类型（无则 nullptr）。
    const Symbol *resolveTypePath(const ast::PathType &path,
                                  const TyPtr &selfType);

    // 解析表达式里的值路径（如 `x`、`Foo::bar`、`self`、`Self::CONST`），
    // 返回值符号；失败返回 nullptr 并写入 diag。
    // 用于 typeck 阶段的名字解析（局部 / 常量 / 函数 / 关联常量）。
    const Symbol *resolveValuePath(const ast::PathInExpr &path,
                                   const TyPtr &selfType);

  private:
    SymbolTable &table_;
    Diagnostic &diag_;
};

} // namespace rx::semantic
