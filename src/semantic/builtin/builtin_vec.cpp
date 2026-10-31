#include "builtin_vec.hpp"

#include "../ty/ty.hpp"

#include <utility>

namespace rx::semantic {

TyPtr VecBuiltin::makeType(TyPtr element) {
    return Ty::makeVec(std::move(element));
}

bool VecBuiltin::isVecType(const TyPtr &ty) { return isVec(ty); }

std::optional<BuiltinFnSig> VecBuiltin::lookup(const std::string &name,
                                               const TyPtr &element) {
    BuiltinFnSig signature;
    signature.name = name;
    if (name == "new") {
        signature.receiver = ReceiverKind::None;
        signature.result = makeType(element);
        return signature;
    }
    if (name == "len") {
        signature.receiver = ReceiverKind::Shared;
        signature.result = Ty::makeScalar(TyKind::Usize);
        return signature;
    }
    if (name == "is_empty") {
        signature.receiver = ReceiverKind::Shared;
        signature.result = Ty::makeScalar(TyKind::Bool);
        return signature;
    }
    if (name == "push") {
        signature.receiver = ReceiverKind::Mutable;
        signature.params.push_back(element);
        signature.result = Ty::makeUnit();
        return signature;
    }
    if (name == "remove") {
        signature.receiver = ReceiverKind::Mutable;
        signature.params.push_back(Ty::makeScalar(TyKind::Usize));
        signature.result = element;
        return signature;
    }
    if (name == "clone") {
        signature.receiver = ReceiverKind::Shared;
        signature.result = makeType(element);
        return signature;
    }
    return std::nullopt;
}

std::optional<TyPtr> VecBuiltin::indexElement(const TyPtr &ty) {
    if (!isVec(ty)) {
        return std::nullopt;
    }
    return ty->inner;
}

} // namespace rx::semantic
