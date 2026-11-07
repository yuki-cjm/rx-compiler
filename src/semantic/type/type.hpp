#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace rx::semantic {
using StructId = std::uint32_t;

enum class Mutability {
    Shared,   // &T
    Mutable,  // &mut T
};

enum class TypeKind {
    I32,
    U32,
    Isize,
    Usize,
    Bool,
    Unit,
    Never,
    Struct,
    Reference,
    Array,
    Box,
    Vec,
};

struct Type;
using TypePtr = std::shared_ptr<const Type>;

struct Type {
    TypeKind kind;
    Mutability mutability = Mutability::Shared;
    TypePtr inner;
    std::uint64_t length = 0;
    StructId structId = 0;

    static TypePtr makeScalar(TypeKind kind);
    static TypePtr makeUnit();
    static TypePtr makeNever();
    static TypePtr makeStruct(StructId id);
    static TypePtr makeReference(TypePtr inner, Mutability mutability);
    static TypePtr makeArray(TypePtr element, std::uint64_t length);
    static TypePtr makeBox(TypePtr element);
    static TypePtr makeVec(TypePtr element);
};

bool isScalar(const TypePtr &type);
bool isInteger(const TypePtr &type);
bool isSignedInteger(const TypePtr &type);
bool isUnsignedInteger(const TypePtr &type);
bool isBool(const TypePtr &type);
bool isUnit(const TypePtr &type);
bool isNever(const TypePtr &type);
bool isStruct(const TypePtr &type);
bool isReference(const TypePtr &type);
bool isArray(const TypePtr &type);
bool isBox(const TypePtr &type);
bool isVec(const TypePtr &type);

} // namespace rx::semantic
