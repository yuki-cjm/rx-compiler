#pragma once

#include "../symbol/symbol_table.hpp"
#include "../type/type.hpp"

namespace rx::semantic {

bool hasDerive(const Symbol *symbol, ast::DeriveTrait trait);

bool isCopy(const SymbolTable &table, const TypePtr &type);
bool isClone(const SymbolTable &table, const TypePtr &type);
bool supportsPartialEq(const SymbolTable &table, const TypePtr &type);
bool supportsEq(const SymbolTable &table, const TypePtr &type);

} // namespace rx::semantic
