#include "ty.hpp"

#include <utility>

namespace rx::semantic {

TyPtr Ty::makeScalar(TyKind kind) {
    auto ty = std::make_shared<Ty>();
    ty->kind = kind;
    return ty;
}

TyPtr Ty::makeUnit() {
    auto ty = std::make_shared<Ty>();
    ty->kind = TyKind::Unit;
    return ty;
}

TyPtr Ty::makeNever() {
    auto ty = std::make_shared<Ty>();
    ty->kind = TyKind::Never;
    return ty;
}

TyPtr Ty::makeStruct(StructId id) {
    auto ty = std::make_shared<Ty>();
    ty->kind = TyKind::Struct;
    ty->structId = id;
    return ty;
}

TyPtr Ty::makeReference(TyPtr inner, Mutability mutability) {
    auto ty = std::make_shared<Ty>();
    ty->kind = TyKind::Reference;
    ty->inner = std::move(inner);
    ty->mutability = mutability;
    return ty;
}

TyPtr Ty::makeArray(TyPtr element, std::uint64_t length) {
    auto ty = std::make_shared<Ty>();
    ty->kind = TyKind::Array;
    ty->inner = std::move(element);
    ty->length = length;
    return ty;
}

TyPtr Ty::makeBox(TyPtr element) {
    auto ty = std::make_shared<Ty>();
    ty->kind = TyKind::Box;
    ty->inner = std::move(element);
    return ty;
}

TyPtr Ty::makeVec(TyPtr element) {
    auto ty = std::make_shared<Ty>();
    ty->kind = TyKind::Vec;
    ty->inner = std::move(element);
    return ty;
}

bool isScalar(const TyPtr &ty) {
    if (!ty) {
        return false;
    }
    switch (ty->kind) {
    case TyKind::I32:
    case TyKind::U32:
    case TyKind::Isize:
    case TyKind::Usize:
        return true;
    default:
        return false;
    }
}

bool isInteger(const TyPtr &ty) { return isScalar(ty); }

bool isSignedInteger(const TyPtr &ty) {
    return ty && (ty->kind == TyKind::I32 || ty->kind == TyKind::Isize);
}

bool isUnsignedInteger(const TyPtr &ty) {
    return ty && (ty->kind == TyKind::U32 || ty->kind == TyKind::Usize);
}

bool isBool(const TyPtr &ty) { return ty && ty->kind == TyKind::Bool; }

bool isUnit(const TyPtr &ty) { return ty && ty->kind == TyKind::Unit; }

bool isNever(const TyPtr &ty) { return ty && ty->kind == TyKind::Never; }

bool isStruct(const TyPtr &ty) { return ty && ty->kind == TyKind::Struct; }

bool isReference(const TyPtr &ty) {
    return ty && ty->kind == TyKind::Reference;
}

bool isArray(const TyPtr &ty) { return ty && ty->kind == TyKind::Array; }

bool isBox(const TyPtr &ty) { return ty && ty->kind == TyKind::Box; }

bool isVec(const TyPtr &ty) { return ty && ty->kind == TyKind::Vec; }

} // namespace rx::semantic
