#include "type.hpp"

#include <utility>

namespace rx::semantic {

TypePtr Type::makeScalar(TypeKind kind) {
    auto type = std::make_shared<Type>();
    type->kind = kind;
    return type;
}

TypePtr Type::makeUnit() {
    auto type = std::make_shared<Type>();
    type->kind = TypeKind::Unit;
    return type;
}

TypePtr Type::makeNever() {
    auto type = std::make_shared<Type>();
    type->kind = TypeKind::Never;
    return type;
}

TypePtr Type::makeStruct(StructId id) {
    auto type = std::make_shared<Type>();
    type->kind = TypeKind::Struct;
    type->structId = id;
    return type;
}

TypePtr Type::makeReference(TypePtr inner, Mutability mutability) {
    auto type = std::make_shared<Type>();
    type->kind = TypeKind::Reference;
    type->inner = std::move(inner);
    type->mutability = mutability;
    return type;
}

TypePtr Type::makeArray(TypePtr element, std::uint64_t length) {
    auto type = std::make_shared<Type>();
    type->kind = TypeKind::Array;
    type->inner = std::move(element);
    type->length = length;
    return type;
}

TypePtr Type::makeBox(TypePtr element) {
    auto type = std::make_shared<Type>();
    type->kind = TypeKind::Box;
    type->inner = std::move(element);
    return type;
}

TypePtr Type::makeVec(TypePtr element) {
    auto type = std::make_shared<Type>();
    type->kind = TypeKind::Vec;
    type->inner = std::move(element);
    return type;
}

bool isScalar(const TypePtr &type) {
    if (!type) {
        return false;
    }
    switch (type->kind) {
        case TypeKind::I32:
        case TypeKind::U32:
        case TypeKind::Isize:
        case TypeKind::Usize:
            return true;
        default:
            return false;
    }
}

bool isInteger(const TypePtr &type) { return isScalar(type); }

bool isSignedInteger(const TypePtr &type) {
    return type && (type->kind == TypeKind::I32 || type->kind == TypeKind::Isize);
}

bool isUnsignedInteger(const TypePtr &type) {
    return type && (type->kind == TypeKind::U32 || type->kind == TypeKind::Usize);
}

bool isBool(const TypePtr &type) { return type && type->kind == TypeKind::Bool; }

bool isUnit(const TypePtr &type) { return type && type->kind == TypeKind::Unit; }

bool isNever(const TypePtr &type) { return type && type->kind == TypeKind::Never; }

bool isStruct(const TypePtr &type) { return type && type->kind == TypeKind::Struct; }

bool isReference(const TypePtr &type) {
    return type && type->kind == TypeKind::Reference;
}

bool isArray(const TypePtr &type) { return type && type->kind == TypeKind::Array; }

bool isBox(const TypePtr &type) { return type && type->kind == TypeKind::Box; }

bool isVec(const TypePtr &type) { return type && type->kind == TypeKind::Vec; }

} // namespace rx::semantic
