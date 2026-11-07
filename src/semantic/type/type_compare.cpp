#include "type_compare.hpp"

namespace rx::semantic {

bool sameType(const TypePtr &a, const TypePtr &b) {
    if (a == b) {
        return true;
    }
    if (!a || !b) {
        return false;
    }
    if (a->kind != b->kind) {
        return false;
    }

    switch (a->kind) {
        case TypeKind::I32:
        case TypeKind::U32:
        case TypeKind::Isize:
        case TypeKind::Usize:
        case TypeKind::Bool:
        case TypeKind::Unit:
        case TypeKind::Never:
            return true;
        case TypeKind::Struct:
            return a->structId == b->structId;
        case TypeKind::Reference:
            return a->mutability == b->mutability && sameType(a->inner, b->inner);
        case TypeKind::Array:
            return a->length == b->length && sameType(a->inner, b->inner);
        case TypeKind::Box:
        case TypeKind::Vec:
            return sameType(a->inner, b->inner);
    }
    return false;
}

} // namespace rx::semantic
