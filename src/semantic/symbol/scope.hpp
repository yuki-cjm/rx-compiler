#pragma once

#include "symbol.hpp"

#include <map>
#include <string>
#include <vector>

namespace rx::semantic {

// 局部作用域：管理函数体 / 块内的值绑定，支持按层 shadowing。
// layers_ 的下标 0 表示最内层；lookup 从内向外逐层查找。
class Scope {
  public:
    // 进入一个新的嵌套作用域（进入块 / 函数体时调用）。
    void push();

    // 退出当前最内层作用域。
    void pop();

    // 在当前最内层声明一个值绑定。允许覆盖同层同名绑定（用于 let shadowing）。
    void declare(const std::string &name, const Symbol &symbol);

    // 从内向外查找名字，返回最近的绑定；未找到返回 nullptr。
    const Symbol *lookup(const std::string &name) const;

    // 当前最内层是否已存在该名字（用于函数参数查重等场景）。
    bool containsLocally(const std::string &name) const;

  private:
    std::vector<std::map<std::string, Symbol>> layers_;
};

} // namespace rx::semantic
