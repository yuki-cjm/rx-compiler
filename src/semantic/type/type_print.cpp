#include "type_print.hpp"

#include "../symbol/symbol_table.hpp"

#include <string>

namespace rx::semantic {

namespace {

std::string scalarName(TypeKind kind) {
    switch (kind) {
    case TypeKind::I32:
        return "i32";
    case TypeKind::U32:
        return "u32";
    case TypeKind::Isize:
        return "isize";
    case TypeKind::Usize:
        return "usize";
    case TypeKind::Bool:
        return "bool";
    default:
        return "?";
    }
}

std::string render(const TypePtr &type, const SymbolTable *table) {
    if (!type) {
        return "<unknown>";
    }
    switch (type->kind) {
    case TypeKind::I32:
    case TypeKind::U32:
    case TypeKind::Isize:
    case TypeKind::Usize:
    case TypeKind::Bool:
        return scalarName(type->kind);
    case TypeKind::Unit:
        return "()";
    case TypeKind::Never:
        return "!";
    case TypeKind::Struct: {
        if (table != nullptr) {
            if (const Symbol *symbol = table->lookupStruct(type->structId)) {
                return symbol->name;
            }
        }
        return "struct#" + std::to_string(type->structId);
    }
    case TypeKind::Reference: {
        std::string result = type->mutability == Mutability::Mutable ? "&mut "
                                                                   : "&";
        return result + render(type->inner, table);
    }
    case TypeKind::Array:
        return "[" + render(type->inner, table) + "; " +
               std::to_string(type->length) + "]";
    case TypeKind::Box:
        return "Box<" + render(type->inner, table) + ">";
    case TypeKind::Vec:
        return "Vec<" + render(type->inner, table) + ">";
    }
    return "?";
}

} // namespace

std::string typeToString(const TypePtr &type) { return render(type, nullptr); }

std::string typeToString(const TypePtr &type, const SymbolTable &table) {
    return render(type, &table);
}

} // namespace rx::semantic
