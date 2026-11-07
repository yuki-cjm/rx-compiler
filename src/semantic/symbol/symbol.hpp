#pragma once

#include "../type/type.hpp"
#include "../../ast/ast.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace rx::semantic {

enum class SymbolKind {
    Function,
    Struct,
    Constant,
    Local,
    BuiltinType,
    BuiltinFunction,
};

struct FieldInfo {
    std::string name;
    TypePtr type;
};

struct Symbol {
    SymbolKind kind;
    std::string name;

    // Function
    const ast::FunctionItem *function = nullptr;
    bool hasReceiver = false;
    StructId implTarget = 0;

    // Struct
    StructId structId = 0;
    std::vector<FieldInfo> fields;
    std::vector<ast::DeriveTrait> derives;

    // Constant / Local
    TypePtr type;
    bool isMutable = false;
    const ast::ConstantItem *constant = nullptr; // Constant
};

} // namespace rx::semantic
