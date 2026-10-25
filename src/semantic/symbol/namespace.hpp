#pragma once

#include "symbol.hpp"

#include <map>
#include <string>

namespace rx::semantic {

// 单个命名空间的符号映射：名字 → 符号。
// 顶层 type / value 命名空间，以及每个结构体的关联项命名空间都复用它。
// insert 检测重名：名字已存在时返回 false 且不覆盖。
class SymbolMap {
  public:
    // 插入一个符号。若名字已存在返回 false（重复声明）。
    bool insert(const std::string &name, const Symbol &symbol);

    // 查找符号；未找到返回 nullptr。
    const Symbol *find(const std::string &name) const;

    // 是否存在该名字。
    bool contains(const std::string &name) const;

    // 遍历底层映射（供收集阶段枚举所有符号）。
    const std::map<std::string, Symbol> &entries() const;

  private:
    std::map<std::string, Symbol> entries_;
};

} // namespace rx::semantic
