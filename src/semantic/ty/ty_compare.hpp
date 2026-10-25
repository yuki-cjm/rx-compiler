#pragma once

#include "ty.hpp"

namespace rx::semantic {

// 判断两个类型是否相同（结构比较 + nominal 比较）。
//
// 规则（规范 types.html）：
//   - 标量按 kind 区分，四种整数互不相同。
//   - Struct 按 structId 比较（nominal）。
//   - Reference 比较 inner 类型与 mutability，忽略生命周期。
//   - Array 比较元素类型与长度（长度按值比较）。
//   - Box/Vec 比较元素类型（结构比较）。
bool sameTy(const TyPtr &a, const TyPtr &b);

// 判断 from 是否能作为 to 使用（等价于 sameTy，但保留语义化命名，
// 供 coercion 模块在「相同类型」这一特例上调用）。
bool typeEquals(const TyPtr &from, const TyPtr &to);

} // namespace rx::semantic
