#include "builtin.hpp"

#include "builtin_scalar.hpp"

#include "../symbol/symbol.hpp"

namespace rx::semantic {

namespace {

Symbol makeContainerTypeSymbol(const std::string &name) {
    Symbol symbol;
    symbol.kind = SymbolKind::BuiltinType;
    symbol.name = name;
    return symbol;
}

} // namespace

void registerBuiltins(SymbolTable &table) {
    registerScalarBuiltins(table);
    table.declareType("Box", makeContainerTypeSymbol("Box"));
    table.declareType("Vec", makeContainerTypeSymbol("Vec"));
}

} // namespace rx::semantic
