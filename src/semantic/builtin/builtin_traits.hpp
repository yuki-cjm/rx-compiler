#pragma once

#include "../symbol/symbol_table.hpp"
#include "../ty/ty.hpp"

namespace rx::semantic {

// Copy / Clone / PartialEq / Eq 能力查询（规范 builtin-traits.html）。
// 用于：赋值 / 移动语义、== 合法性、Box/Vec 的 clone 可用性、derive 派生。
//
// 这些查询需要符号表以取得结构体的 derive 属性，因此以自由函数提供。
// 未实现的里程碑可先让它们返回保守值（如全部 true 或全部 false），
// 不影响前面里程碑的通过。

// T 是否实现 Copy（标量默认是；结构体取决于 #[derive(Copy)]）。
bool isCopy(const SymbolTable &table, const TyPtr &ty);

// T 是否实现 Clone。
bool isClone(const SymbolTable &table, const TyPtr &ty);

// T 是否支持 == / !=（PartialEq）。
bool supportsPartialEq(const SymbolTable &table, const TyPtr &ty);

// T 是否实现 Eq（结构体取决于 #[derive(Eq)]）。
bool supportsEq(const SymbolTable &table, const TyPtr &ty);

} // namespace rx::semantic
