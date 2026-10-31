#include "builtin_box.hpp"

#include "../ty/ty.hpp"

#include <utility>

namespace rx::semantic {

TyPtr BoxBuiltin::makeType(TyPtr element) {
    return Ty::makeBox(std::move(element));
}

bool BoxBuiltin::isBoxType(const TyPtr &ty) { return isBox(ty); }

std::optional<BuiltinFnSig> BoxBuiltin::lookup(const std::string &name,
                                               const TyPtr &element) {
    BuiltinFnSig signature;
    signature.name = name;
    if (name == "new") {
        signature.receiver = ReceiverKind::None;
        signature.params.push_back(element);
        signature.result = makeType(element);
        return signature;
    }
    if (name == "clone") {
        signature.receiver = ReceiverKind::Shared;
        signature.result = makeType(element);
        return signature;
    }
    return std::nullopt;
}

std::optional<TyPtr> BoxBuiltin::derefTarget(const TyPtr &ty) {
    if (!isBox(ty)) {
        return std::nullopt;
    }
    return ty->inner;
}

} // namespace rx::semantic
