#include "ty_compare.hpp"

namespace rx::semantic {

bool sameTy(const TyPtr &a, const TyPtr &b) {
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
    case TyKind::I32:
    case TyKind::U32:
    case TyKind::Isize:
    case TyKind::Usize:
    case TyKind::Bool:
    case TyKind::Unit:
    case TyKind::Never:
        return true;
    case TyKind::Struct:
        return a->structId == b->structId;
    case TyKind::Reference:
        return a->mutability == b->mutability &&
               sameTy(a->inner, b->inner);
    case TyKind::Array:
        return a->length == b->length && sameTy(a->inner, b->inner);
    case TyKind::Box:
    case TyKind::Vec:
        return sameTy(a->inner, b->inner);
    }
    return false;
}

bool typeEquals(const TyPtr &from, const TyPtr &to) {
    return sameTy(from, to);
}

} // namespace rx::semantic
