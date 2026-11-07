#pragma once

#include "type.hpp"

#include <string>

namespace rx::semantic {

class SymbolTable;

std::string typeToString(const TypePtr &type);

std::string typeToString(const TypePtr &type, const SymbolTable &table);

} // namespace rx::semantic
