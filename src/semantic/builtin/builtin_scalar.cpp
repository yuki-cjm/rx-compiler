#include "builtin_scalar.hpp"

#include "../ty/ty.hpp"

#include <utility>

namespace rx::semantic {

namespace {

Symbol makeTypeSymbol(const std::string &name, TyPtr type) {
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
    table.declareType("i32", makeTypeSymbol("i32", Ty::makeScalar(TyKind::I32)));
    table.declareType("u32", makeTypeSymbol("u32", Ty::makeScalar(TyKind::U32)));
    table.declareType("isize",
                      makeTypeSymbol("isize", Ty::makeScalar(TyKind::Isize)));
    table.declareType("usize",
                      makeTypeSymbol("usize", Ty::makeScalar(TyKind::Usize)));
    table.declareType("bool",
                      makeTypeSymbol("bool", Ty::makeScalar(TyKind::Bool)));

    table.declareGlobalValue("get_i32", makeFunctionSymbol("get_i32"));
    table.declareGlobalValue("print_i32", makeFunctionSymbol("print_i32"));
    table.declareGlobalValue("println_i32", makeFunctionSymbol("println_i32"));
}

bool isProtectedTypeName(const std::string &name) {
    return name == "i32" || name == "u32" || name == "isize" ||
           name == "usize" || name == "bool" || name == "Box" ||
           name == "Vec" || name == "Copy" || name == "Clone" ||
           name == "PartialEq" || name == "Eq";
}

bool isProtectedValueName(const std::string &name) {
    return name == "get_i32" || name == "print_i32" || name == "println_i32";
}

} // namespace rx::semantic
