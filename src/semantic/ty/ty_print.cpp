#include "ty_print.hpp"

#include "../symbol/symbol_table.hpp"

#include <string>

namespace rx::semantic {

namespace {

std::string scalarName(TyKind kind) {
    switch (kind) {
    case TyKind::I32:
        return "i32";
    case TyKind::U32:
        return "u32";
    case TyKind::Isize:
        return "isize";
    case TyKind::Usize:
        return "usize";
    case TyKind::Bool:
        return "bool";
    default:
        return "?";
    }
}

std::string render(const TyPtr &ty, const SymbolTable *table) {
    if (!ty) {
        return "<unknown>";
    }
    switch (ty->kind) {
    case TyKind::I32:
    case TyKind::U32:
    case TyKind::Isize:
    case TyKind::Usize:
    case TyKind::Bool:
        return scalarName(ty->kind);
    case TyKind::Unit:
        return "()";
    case TyKind::Never:
        return "!";
    case TyKind::Struct: {
        if (table != nullptr) {
            if (const Symbol *symbol = table->lookupStruct(ty->structId)) {
                return symbol->name;
            }
        }
        return "struct#" + std::to_string(ty->structId);
    }
    case TyKind::Reference: {
        std::string result = ty->mutability == Mutability::Mutable ? "&mut "
                                                                   : "&";
        return result + render(ty->inner, table);
    }
    case TyKind::Array:
        return "[" + render(ty->inner, table) + "; " +
               std::to_string(ty->length) + "]";
    case TyKind::Box:
        return "Box<" + render(ty->inner, table) + ">";
    case TyKind::Vec:
        return "Vec<" + render(ty->inner, table) + ">";
    }
    return "?";
}

} // namespace

std::string typeToString(const TyPtr &ty) { return render(ty, nullptr); }

std::string typeToString(const TyPtr &ty, const SymbolTable &table) {
    return render(ty, &table);
}

} // namespace rx::semantic
