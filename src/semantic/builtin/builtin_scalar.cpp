#include "builtin_scalar.hpp"

#include "../type/type.hpp"

#include <utility>

namespace rx::semantic {

namespace {

Symbol makeTypeSymbol(const std::string &name, TypePtr type) {
    Symbol symbol;
    symbol.kind = SymbolKind::BuiltinType;
    symbol.name = name;
    symbol.type = std::move(type);
    return symbol;
}

Symbol makeFunctionSymbol(const std::string &name) {
    Symbol symbol;
    symbol.kind = SymbolKind::BuiltinFunction;
    symbol.name = name;
    return symbol;
}

} // namespace

void registerScalarBuiltins(SymbolTable &table) {
    table.declareType("i32"  , makeTypeSymbol("i32"  , Type::makeScalar(TypeKind::I32)));
    table.declareType("u32"  , makeTypeSymbol("u32"  , Type::makeScalar(TypeKind::U32)));
    table.declareType("isize", makeTypeSymbol("isize", Type::makeScalar(TypeKind::Isize)));
    table.declareType("usize", makeTypeSymbol("usize", Type::makeScalar(TypeKind::Usize)));
    table.declareType("bool" , makeTypeSymbol("bool" , Type::makeScalar(TypeKind::Bool)));

    table.declareGlobalValue("get_i32", makeFunctionSymbol("get_i32"));
    table.declareGlobalValue("print_i32", makeFunctionSymbol("print_i32"));
    table.declareGlobalValue("println_i32", makeFunctionSymbol("println_i32"));
}

} // namespace rx::semantic
