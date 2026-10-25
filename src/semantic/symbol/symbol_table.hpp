#pragma once

#include "symbol.hpp"
#include "namespace.hpp"
#include "scope.hpp"

#include <map>
#include <string>

namespace rx::semantic {

// 符号表：语义分析的全局状态。
// 组合三个命名空间 + 局部作用域，并提供跨命名空间的便捷查找。
//
//   - types_   ：顶层 type 命名空间（结构体 + 内建类型）。
//   - values_  ：顶层 value 命名空间（顶层函数 + 常量 + 内建函数）。
//   - assoc_   ：按结构体划分的关联项命名空间（方法 / 关联函数 / 关联常量）。
//   - scope_   ：当前局部作用域（let / 参数）。
class SymbolTable {
  public:
    // ---- 顶层 type 命名空间 ----
    // 声明类型符号；若为 Struct 会自动为其分配并记录 structId。
    // 返回 false 表示重名。
    bool declareType(const std::string &name, Symbol symbol);
    const Symbol *lookupType(const std::string &name) const;

    // ---- 顶层 value 命名空间 ----
    bool declareGlobalValue(const std::string &name, Symbol symbol);
    const Symbol *lookupGlobalValue(const std::string &name) const;

    // ---- 关联项命名空间 ----
    bool declareAssoc(StructId target, const std::string &name, Symbol symbol);
    const Symbol *lookupAssoc(StructId target, const std::string &name) const;

    // ---- 局部作用域 ----
    Scope &scope();
    const Scope &scope() const;

    // 完整值名解析：先查局部作用域，再查顶层 value 命名空间。
    const Symbol *resolveValue(const std::string &name) const;

    // ---- 结构体 id 分配 ----
    StructId allocateStructId();

    // ---- 字段查找 ----
    // 通过 structId 找到结构体符号后在其字段列表中查找。
    const FieldInfo *lookupField(StructId structId, const std::string &name) const;

    // 通过 structId 取得结构体符号（供 resolve / trait 能力查询使用）。
    const Symbol *lookupStruct(StructId structId) const;

  private:
    SymbolMap types_;
    SymbolMap values_;
    std::map<StructId, SymbolMap> assoc_;
    std::map<StructId, const Symbol *> structs_; // structId → 结构体符号（含字段）
    Scope scope_;
    StructId nextStructId_ = 0;
};

} // namespace rx::semantic
