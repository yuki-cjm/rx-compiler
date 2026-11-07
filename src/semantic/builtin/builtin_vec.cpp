#include "builtin_vec.hpp"

#include "../type/type.hpp"

#include <utility>

namespace rx::semantic {

TypePtr VecBuiltin::makeType(TypePtr element) {
    return Type::makeVec(std::move(element));
}

std::optional<BuiltinFnSig> VecBuiltin::lookup(const std::string &name, const TypePtr &element) {
    BuiltinFnSig signature;
    signature.name = name;
    if (name == "new") {
        signature.receiver = ReceiverKind::None;
        signature.result = makeType(element);
        return signature;
    }
    if (name == "len") {
        signature.receiver = ReceiverKind::Shared;
        signature.result = Type::makeScalar(TypeKind::Usize);
        return signature;
    }
    if (name == "is_empty") {
        signature.receiver = ReceiverKind::Shared;
        signature.result = Type::makeScalar(TypeKind::Bool);
        return signature;
    }
    if (name == "push") {
        signature.receiver = ReceiverKind::Mutable;
        signature.params.push_back(element);
        signature.result = Type::makeUnit();
        return signature;
    }
    if (name == "remove") {
        signature.receiver = ReceiverKind::Mutable;
        signature.params.push_back(Type::makeScalar(TypeKind::Usize));
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

} // namespace rx::semantic
