#include "builtin_traits.hpp"

#include "../symbol/symbol.hpp"

namespace rx::semantic {

bool hasDerive(const Symbol *symbol, ast::DeriveTrait trait) {
    if (symbol == nullptr) {
        return false;
    }
    for (ast::DeriveTrait candidate : symbol->derives) {
        if (candidate == trait) {
            return true;
        }
    }
    return false;
}

bool isCopy(const SymbolTable &table, const TypePtr &type) {
    if (!type) {
        return false;
    }
    switch (type->kind) {
        case TypeKind::I32:
        case TypeKind::U32:
        case TypeKind::Isize:
        case TypeKind::Usize:
        case TypeKind::Bool:
            return true;
        case TypeKind::Reference:
            // 共享引用是 Copy；可变引用不是。
            return type->mutability == Mutability::Shared;
        case TypeKind::Array:
            return isCopy(table, type->inner);
        case TypeKind::Struct:
            return hasDerive(table.lookupStruct(type->structId), ast::DeriveTrait::Copy);
        case TypeKind::Box:
        case TypeKind::Vec:
        case TypeKind::Unit:
        case TypeKind::Never:
            return false;
    }
    return false;
}

bool isClone(const SymbolTable &table, const TypePtr &type) {
    if (!type) {
        return false;
    }
    switch (type->kind) {
        case TypeKind::I32:
        case TypeKind::U32:
        case TypeKind::Isize:
        case TypeKind::Usize:
        case TypeKind::Bool:
            return true;
        case TypeKind::Reference:
            return type->mutability == Mutability::Shared;
        case TypeKind::Array:
            return isClone(table, type->inner);
        case TypeKind::Struct:
            return hasDerive(table.lookupStruct(type->structId), ast::DeriveTrait::Clone);
        case TypeKind::Box:
        case TypeKind::Vec:
            return isClone(table, type->inner);
        case TypeKind::Unit:
        case TypeKind::Never:
            return false;
    }
    return false;
}

bool supportsPartialEq(const SymbolTable &table, const TypePtr &type) {
    if (!type) {
        return false;
    }
    switch (type->kind) {
        case TypeKind::I32:
        case TypeKind::U32:
        case TypeKind::Isize:
        case TypeKind::Usize:
        case TypeKind::Bool:
            return true;
        case TypeKind::Reference:
        case TypeKind::Array:
        case TypeKind::Box:
        case TypeKind::Vec:
            return supportsPartialEq(table, type->inner);
        case TypeKind::Struct:
            return hasDerive(table.lookupStruct(type->structId), ast::DeriveTrait::PartialEq);
        case TypeKind::Unit:
        case TypeKind::Never:
            return false;
    }
    return false;
}

bool supportsEq(const SymbolTable &table, const TypePtr &type) {
    if (!type) {
        return false;
    }
    switch (type->kind) {
        case TypeKind::I32:
        case TypeKind::U32:
        case TypeKind::Isize:
        case TypeKind::Usize:
        case TypeKind::Bool:
            return true;
        case TypeKind::Reference:
        case TypeKind::Array:
        case TypeKind::Box:
        case TypeKind::Vec:
            return supportsEq(table, type->inner);
        case TypeKind::Struct:
            return hasDerive(table.lookupStruct(type->structId), ast::DeriveTrait::Eq);
        case TypeKind::Unit:
        case TypeKind::Never:
            return false;
    }
    return false;
}

} // namespace rx::semantic
