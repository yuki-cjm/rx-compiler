#pragma once

#include "../symbol/symbol_table.hpp"
#include "../type/type.hpp"

#include <string>
#include <vector>

namespace rx::semantic {

enum class ReceiverKind {
    None,
    Owned,   // self / mut self
    Shared,  // &self
    Mutable, // &mut self
};

struct BuiltinFnSig {
    std::string name;
    ReceiverKind receiver = ReceiverKind::None;
    std::vector<TypePtr> params;
    TypePtr result;
};

void registerBuiltins(SymbolTable &table);

} // namespace rx::semantic
