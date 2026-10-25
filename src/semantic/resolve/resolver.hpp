#pragma once

#include "../../ast/ast.hpp"
#include "../symbol/symbol_table.hpp"
#include "../diag/diagnostic.hpp"
#include "../ty/ty.hpp"

namespace rx::semantic {

// Pass 1：把 AST 的语法类型（ast::Type）解析成语义类型（Ty）。
// 负责：类型名解析（查 type 命名空间）、Box/Vec 类型实参个数校验、
// Self/SelfType 上下文处理、数组长度求值（依赖 consteval）。
class TypeResolver {
  public:
    TypeResolver(SymbolTable &table, Diagnostic &diag);

    // 解析一个语法类型。selfType 为当前 impl / struct 上下文的 Self 类型；
    // 顶层（无 Self 上下文）传 nullptr。失败返回 nullptr 并写入 diag。
    TyPtr resolve(const ast::Type &type, const TyPtr &selfType);

    // 便捷入口：解析函数 / 字段 / 常量签名中的类型（无 Self 上下文）。
    TyPtr resolveSignature(const ast::Type &type);

  private:
    SymbolTable &table_;
    Diagnostic &diag_;
};

} // namespace rx::semantic
