#include "builtin_box.hpp"

#include "../type/type.hpp"

#include <utility>

namespace rx::semantic {

TypePtr BoxBuiltin::makeType(TypePtr element) {
    return Type::makeBox(std::move(element));
}

std::optional<BuiltinFnSig> BoxBuiltin::lookup(const std::string &name,
                                               const TypePtr &element) {
    BuiltinFnSig signature;
    signature.name = name;
    // Box::<T>::new() -> Box<T>
    if (name == "new") {
        signature.receiver = ReceiverKind::None;
        signature.params.push_back(element);
        signature.result = makeType(element);
        return signature;
    }
    // clone(&self) -> Box<T>
    if (name == "clone") {
        signature.receiver = ReceiverKind::Shared;
        signature.result = makeType(element);
        return signature;
    }
    return std::nullopt;
}

} // namespace rx::semantic
