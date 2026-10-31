#include "duplicate_check.hpp"

#include "../builtin/builtin_scalar.hpp"

#include <set>

namespace rx::semantic {

void checkDuplicateFields(const ast::StructItem &item, Diagnostic &diag) {
    std::set<std::string> seen;
    for (const ast::StructField &field : item.fields) {
        if (!seen.insert(field.name).second) {
            diag.error("duplicate field '" + field.name + "'");
        }
    }
}

void checkDuplicateParameters(const ast::FunctionItem &item, Diagnostic &diag) {
    std::set<std::string> seen;
    for (const ast::FunctionParam &parameter : item.parameters) {
        if (!seen.insert(parameter.binding.name).second) {
            diag.error("duplicate parameter '" + parameter.binding.name + "'");
        }
    }
}

void checkProtectedName(const Symbol &symbol, Diagnostic &diag) {
    if (symbol.kind == SymbolKind::Struct) {
        // 内建类型名中只有 i32 的重声明是必需报错的；其余是 undefined behavior。
        if (symbol.name == "i32") {
            diag.error("protected type name 'i32' cannot be redeclared");
        }
    } else if (symbol.kind == SymbolKind::Function ||
               symbol.kind == SymbolKind::Constant) {
        if (isProtectedValueName(symbol.name)) {
            diag.error("protected value name '" + symbol.name +
                       "' cannot be redeclared");
        }
    }
}

} // namespace rx::semantic
