#pragma once

#include "../../ast/ast.hpp"
#include "../symbol/symbol_table.hpp"
#include "../diag/diagnostic.hpp"
#include "../type/type.hpp"

#include <cstdint>
#include <optional>

namespace rx::semantic {

struct EvaluatedConst {
    TypePtr type;
    std::uint64_t intValue = 0; // Integer
    bool boolValue = false;     // bool
};

std::optional<EvaluatedConst> evaluateConst(SymbolTable &table, Diagnostic &diag, const ast::ConstValue &value, const TypePtr &expectedType, const TypePtr &selfType);

std::optional<std::uint64_t> evaluateArrayLength(SymbolTable &table, Diagnostic &diag, const ast::ConstValue &value, const TypePtr &selfType);

} // namespace rx::semantic
