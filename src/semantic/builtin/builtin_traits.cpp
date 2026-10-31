#include "builtin_traits.hpp"

#include "../symbol/symbol.hpp"

namespace rx::semantic {

namespace {

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

} // namespace

bool isCopy(const SymbolTable &table, const TyPtr &ty) {
    if (!ty) {
        return false;
    }
    switch (ty->kind) {
    case TyKind::I32:
    case TyKind::U32:
    case TyKind::Isize:
    case TyKind::Usize:
    case TyKind::Bool:
        return true;
    case TyKind::Reference:
        // 共享引用是 Copy；可变引用不是。
        return ty->mutability == Mutability::Shared;
    case TyKind::Array:
        return isCopy(table, ty->inner);
    case TyKind::Struct:
        return hasDerive(table.lookupStruct(ty->structId), ast::DeriveTrait::Copy);
    case TyKind::Box:
    case TyKind::Vec:
    case TyKind::Unit:
    case TyKind::Never:
        return false;
    }
    return false;
}

bool isClone(const SymbolTable &table, const TyPtr &ty) {
    if (!ty) {
        return false;
    }
    switch (ty->kind) {
    case TyKind::I32:
    case TyKind::U32:
    case TyKind::Isize:
    case TyKind::Usize:
    case TyKind::Bool:
        return true;
    case TyKind::Reference:
        return ty->mutability == Mutability::Shared;
    case TyKind::Array:
        return isClone(table, ty->inner);
    case TyKind::Struct:
        return hasDerive(table.lookupStruct(ty->structId),
                         ast::DeriveTrait::Clone);
    case TyKind::Box:
    case TyKind::Vec:
        return isClone(table, ty->inner);
    case TyKind::Unit:
    case TyKind::Never:
        return false;
    }
    return false;
}

bool supportsPartialEq(const SymbolTable &table, const TyPtr &ty) {
    if (!ty) {
        return false;
    }
    switch (ty->kind) {
    case TyKind::I32:
    case TyKind::U32:
    case TyKind::Isize:
    case TyKind::Usize:
    case TyKind::Bool:
        return true;
    case TyKind::Reference:
    case TyKind::Array:
    case TyKind::Box:
    case TyKind::Vec:
        return supportsPartialEq(table, ty->inner);
    case TyKind::Struct:
        return hasDerive(table.lookupStruct(ty->structId),
                         ast::DeriveTrait::PartialEq);
    case TyKind::Unit:
    case TyKind::Never:
        return false;
    }
    return false;
}

bool supportsEq(const SymbolTable &table, const TyPtr &ty) {
    if (!ty) {
        return false;
    }
    switch (ty->kind) {
    case TyKind::I32:
    case TyKind::U32:
    case TyKind::Isize:
    case TyKind::Usize:
    case TyKind::Bool:
        return true;
    case TyKind::Reference:
    case TyKind::Array:
    case TyKind::Box:
    case TyKind::Vec:
        return supportsEq(table, ty->inner);
    case TyKind::Struct:
        return hasDerive(table.lookupStruct(ty->structId), ast::DeriveTrait::Eq);
    case TyKind::Unit:
    case TyKind::Never:
        return false;
    }
    return false;
}

} // namespace rx::semantic
