#include "calls.hpp"

#include "../resolve/path_resolve.hpp"
#include "../resolve/resolver.hpp"
#include "../builtin/builtin_box.hpp"
#include "../builtin/builtin_scalar.hpp"
#include "../builtin/builtin_traits.hpp"
#include "../builtin/builtin_vec.hpp"
#include "coercion.hpp"

namespace rx::semantic {

namespace {

ReceiverKind receiverKindOf(const ast::SelfParam &self) {
    if (!self.isBorrowed) {
        return ReceiverKind::Owned;
    }
    return self.isMutable ? ReceiverKind::Mutable : ReceiverKind::Shared;
}

// 从函数定义解析形参（不含 receiver）与返回类型。
std::optional<CallSignature>
signatureOfFunction(SymbolTable &table, Diagnostic &diag,
                    const ast::FunctionItem &function, const TyPtr &selfType) {
    TypeResolver resolver(table, diag);
    CallSignature signature;
    if (function.returnType != nullptr) {
        signature.result = resolver.resolve(*function.returnType, selfType);
        if (signature.result == nullptr) {
            return std::nullopt;
        }
    } else {
        signature.result = Ty::makeUnit();
    }
    for (const ast::FunctionParam &parameter : function.parameters) {
        TyPtr type = resolver.resolve(*parameter.type, selfType);
        if (type == nullptr) {
            return std::nullopt;
        }
        signature.params.push_back(std::move(type));
    }
    return signature;
}

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

// 内建标量函数（get_i32 / print_i32 / println_i32）的签名。
std::optional<CallSignature> builtinFunctionSignature(const std::string &name) {
    CallSignature signature;
    if (name == "get_i32") {
        signature.result = Ty::makeScalar(TyKind::I32);
        return signature;
    }
    if (name == "print_i32" || name == "println_i32") {
        signature.params.push_back(Ty::makeScalar(TyKind::I32));
        signature.result = Ty::makeUnit();
        return signature;
    }
    return std::nullopt;
}

// 容器路径的首段（Box / Vec，带类型实参）→ 元素类型。
std::optional<TyPtr> containerElement(SymbolTable &table, Diagnostic &diag,
                                      const ast::PathInExpr &path) {
    if (path.segments.size() != 2) {
        return std::nullopt;
    }
    const ast::PathExprSegment &first = path.segments[0];
    if (first.ident.kind != ast::PathSegmentKind::Ident ||
        (first.ident.name != "Box" && first.ident.name != "Vec")) {
        return std::nullopt;
    }
    if (!first.genericArgs) {
        diag.error(first.ident.name +
                   " requires exactly one concrete type argument");
        return std::nullopt;
    }
    std::size_t typeArgs = 0;
    const ast::Type *typeArgument = nullptr;
    for (const ast::GenericArg &argument : *first.genericArgs) {
        if (argument.type != nullptr) {
            ++typeArgs;
            typeArgument = argument.type.get();
        }
    }
    if (typeArgs != 1) {
        diag.error(first.ident.name +
                   " requires exactly one concrete type argument");
        return std::nullopt;
    }
    TypeResolver resolver(table, diag);
    return resolver.resolve(*typeArgument, nullptr);
}

// 解析容器路径调用（Box::<T>::method / Vec::<T>::method）。
std::optional<CallSignature>
resolveContainerCall(SymbolTable &table, Diagnostic &diag,
                     const ast::PathInExpr &path) {
    auto element = containerElement(table, diag, path);
    if (!element) {
        return std::nullopt;
    }
    const std::string &container = path.segments[0].ident.name;
    const std::string &method = path.segments[1].ident.name;

    std::optional<BuiltinFnSig> builtin;
    if (container == "Box") {
        builtin = BoxBuiltin::lookup(method, *element);
    } else {
        builtin = VecBuiltin::lookup(method, *element);
    }
    if (!builtin) {
        diag.error("no such builtin method '" + method + "'");
        return std::nullopt;
    }
    if (method == "clone" && !isClone(table, *element)) {
        diag.error("clone requires the element type to be Clone");
        return std::nullopt;
    }

    CallSignature signature;
    signature.result = builtin->result;
    signature.receiver = builtin->receiver;
    signature.element = *element;
    if (builtin->receiver == ReceiverKind::None) {
        signature.params = builtin->params;
    } else {
        // 显式关联调用：receiver 作为第一个实参。
        TyPtr base = (container == "Box") ? Ty::makeBox(*element)
                                          : Ty::makeVec(*element);
        TyPtr receiverType =
            builtin->receiver == ReceiverKind::Mutable
                ? Ty::makeReference(base, Mutability::Mutable)
                : Ty::makeReference(base, Mutability::Shared);
        signature.params.push_back(std::move(receiverType));
        for (TyPtr parameter : builtin->params) {
            signature.params.push_back(std::move(parameter));
        }
    }
    return signature;
}

} // namespace

std::optional<CallSignature>
resolveCallable(SymbolTable &table, Diagnostic &diag, const ast::CallExpr &call,
                const TyPtr &selfType) {
    auto *path = dynamic_cast<const ast::PathInExpr *>(call.callee.get());
    if (path == nullptr) {
        diag.error("callee is not a callable path");
        return std::nullopt;
    }

    // 容器路径（Box::<T>::method / Vec::<T>::method）。
    if (path->segments.size() == 2 &&
        path->segments[0].ident.kind == ast::PathSegmentKind::Ident &&
        (path->segments[0].ident.name == "Box" ||
         path->segments[0].ident.name == "Vec")) {
        return resolveContainerCall(table, diag, *path);
    }

    PathResolver resolver(table, diag);

    // 两段路径：Type::method / Self::method（含派生 clone 回退）。
    if (path->segments.size() == 2) {
        const ast::PathExprSegment &first = path->segments[0];
        const ast::PathExprSegment &second = path->segments[1];
        if (second.genericArgs.has_value()) {
            diag.error("method segments have no type parameters");
            return std::nullopt;
        }

        StructId structId = 0;
        if (first.ident.kind == ast::PathSegmentKind::SelfType) {
            if (!isStruct(selfType)) {
                diag.error("'Self' used outside a struct context");
                return std::nullopt;
            }
            structId = selfType->structId;
        } else if (first.ident.kind == ast::PathSegmentKind::Ident) {
            const Symbol *typeSymbol = table.lookupType(first.ident.name);
            if (typeSymbol == nullptr ||
                typeSymbol->kind != SymbolKind::Struct) {
                diag.error("'" + first.ident.name + "' is not a struct");
                return std::nullopt;
            }
            structId = typeSymbol->structId;
        } else {
            diag.error("invalid value path");
            return std::nullopt;
        }

        const Symbol *associated = table.lookupAssoc(structId, second.ident.name);
        if (associated != nullptr &&
            associated->kind == SymbolKind::Function) {
            TyPtr targetSelf = Ty::makeStruct(structId);
            auto signature = signatureOfFunction(table, diag,
                                                 *associated->function,
                                                 targetSelf);
            if (!signature) {
                return std::nullopt;
            }
            if (associated->hasReceiver) {
                signature->receiver =
                    receiverKindOf(*associated->function->selfParam);
                TyPtr base = Ty::makeStruct(structId);
                TyPtr receiverType;
                if (signature->receiver == ReceiverKind::Owned) {
                    receiverType = base;
                } else if (signature->receiver == ReceiverKind::Mutable) {
                    receiverType = Ty::makeReference(base, Mutability::Mutable);
                } else {
                    receiverType = Ty::makeReference(base, Mutability::Shared);
                }
                signature->params.insert(signature->params.begin(),
                                         receiverType);
            }
            return signature;
        }

        if (second.ident.name == "clone") {
            const Symbol *structure = table.lookupStruct(structId);
            if (hasDerive(structure, ast::DeriveTrait::Clone)) {
                CallSignature signature;
                signature.receiver = ReceiverKind::Shared;
                TyPtr base = Ty::makeStruct(structId);
                signature.params.push_back(
                    Ty::makeReference(base, Mutability::Shared));
                signature.element = base;
                signature.result = base;
                return signature;
            }
        }

        diag.error("no associated item '" + second.ident.name + "'");
        return std::nullopt;
    }

    const Symbol *symbol = resolver.resolveValuePath(*path, selfType);
    if (symbol == nullptr) {
        return std::nullopt;
    }

    if (symbol->kind == SymbolKind::BuiltinFunction) {
        return builtinFunctionSignature(symbol->name);
    }
    if (symbol->kind != SymbolKind::Function) {
        diag.error("'" + symbol->name + "' is not callable");
        return std::nullopt;
    }

    TyPtr targetSelf = symbol->implTarget != 0
                           ? Ty::makeStruct(symbol->implTarget)
                           : nullptr;
    auto signature =
        signatureOfFunction(table, diag, *symbol->function, targetSelf);
    if (!signature) {
        return std::nullopt;
    }
    if (symbol->hasReceiver) {
        // 显式关联调用：receiver 作为第一个实参。
        signature->receiver = receiverKindOf(*symbol->function->selfParam);
        TyPtr base = Ty::makeStruct(symbol->implTarget);
        TyPtr receiverType;
        if (signature->receiver == ReceiverKind::Owned) {
            receiverType = base;
        } else if (signature->receiver == ReceiverKind::Mutable) {
            receiverType = Ty::makeReference(base, Mutability::Mutable);
        } else {
            receiverType = Ty::makeReference(base, Mutability::Shared);
        }
        signature->params.insert(signature->params.begin(), receiverType);
    }
    return signature;
}

std::optional<CallSignature>
resolveMethod(SymbolTable &table, Diagnostic &diag, const TyPtr &receiverType,
              const std::string &methodName, const TyPtr &selfType) {
    // 候选接收者类型链：L0, &L0, &mut L0, L1, &L1, &mut L1, ...
    std::vector<TyPtr> levels;
    TyPtr current = receiverType;
    for (;;) {
        levels.push_back(current);
        if (isReference(current)) {
            current = current->inner;
        } else if (isBox(current)) {
            current = current->inner;
        } else {
            break;
        }
    }

    for (const TyPtr &level : levels) {
        std::vector<TyPtr> candidates;
        candidates.push_back(level);
        candidates.push_back(Ty::makeReference(level, Mutability::Shared));
        candidates.push_back(Ty::makeReference(level, Mutability::Mutable));

        for (const TyPtr &candidate : candidates) {
            // 内建方法（Box / Vec / 数组 / 派生 clone）。
            if (isReference(candidate) &&
                candidate->mutability == Mutability::Shared) {
                if (isBox(candidate->inner)) {
                    TyPtr element = candidate->inner->inner;
                    if (methodName == "clone" && isClone(table, element)) {
                        CallSignature signature;
                        signature.receiver = ReceiverKind::Shared;
                        signature.element = element;
                        signature.result = Ty::makeBox(element);
                        return signature;
                    }
                }
                if (isVec(candidate->inner)) {
                    TyPtr element = candidate->inner->inner;
                    CallSignature signature;
                    signature.receiver = ReceiverKind::Shared;
                    signature.element = element;
                    if (methodName == "len") {
                        signature.result = Ty::makeScalar(TyKind::Usize);
                        return signature;
                    }
                    if (methodName == "is_empty") {
                        signature.result = Ty::makeScalar(TyKind::Bool);
                        return signature;
                    }
                    if (methodName == "clone" && isClone(table, element)) {
                        signature.result = Ty::makeVec(element);
                        return signature;
                    }
                }
                if (isArray(candidate->inner) && methodName == "len") {
                    CallSignature signature;
                    signature.receiver = ReceiverKind::Shared;
                    signature.result = Ty::makeScalar(TyKind::Usize);
                    return signature;
                }
                if (isArray(candidate->inner) && methodName == "clone" &&
                    isClone(table, candidate->inner->inner)) {
                    CallSignature signature;
                    signature.receiver = ReceiverKind::Shared;
                    signature.element = candidate->inner->inner;
                    signature.result = candidate->inner;
                    return signature;
                }
                // 标量 clone：i32.clone() 等。
                if (methodName == "clone" &&
                    (isScalar(candidate->inner) || isBool(candidate->inner))) {
                    CallSignature signature;
                    signature.receiver = ReceiverKind::Shared;
                    signature.element = candidate->inner;
                    signature.result = candidate->inner;
                    return signature;
                }
                // 共享引用 clone：&T.clone() 复制引用本身，无需 T: Clone。
                if (methodName == "clone" && isReference(candidate->inner) &&
                    candidate->inner->mutability == Mutability::Shared) {
                    CallSignature signature;
                    signature.receiver = ReceiverKind::Shared;
                    signature.element = candidate->inner;
                    signature.result = candidate->inner;
                    return signature;
                }
            }
            if (isReference(candidate) && candidate->mutability == Mutability::Mutable &&
                isVec(candidate->inner)) {
                TyPtr element = candidate->inner->inner;
                if (methodName == "push") {
                    CallSignature signature;
                    signature.receiver = ReceiverKind::Mutable;
                    signature.element = element;
                    signature.selfType = Ty::makeVec(element);
                    signature.params.push_back(element);
                    signature.result = Ty::makeUnit();
                    return signature;
                }
                if (methodName == "remove") {
                    CallSignature signature;
                    signature.receiver = ReceiverKind::Mutable;
                    signature.element = element;
                    signature.selfType = Ty::makeVec(element);
                    signature.params.push_back(Ty::makeScalar(TyKind::Usize));
                    signature.result = element;
                    return signature;
                }
            }

            // 内建 / 派生 clone 与固有方法（struct）。
            StructId structId = 0;
            bool isReferenceCandidate = isReference(candidate);
            Mutability candidateMutability = Mutability::Shared;
            const TyPtr *structType = &candidate;
            if (isReferenceCandidate) {
                candidateMutability = candidate->mutability;
                structType = &candidate->inner;
            }
            if (!isStruct(*structType)) {
                continue;
            }
            structId = (*structType)->structId;

            const Symbol *methodSymbol = table.lookupAssoc(structId, methodName);
            if (methodSymbol != nullptr &&
                methodSymbol->kind == SymbolKind::Function &&
                methodSymbol->hasReceiver) {
                ReceiverKind kind =
                    receiverKindOf(*methodSymbol->function->selfParam);
                bool matches =
                    (kind == ReceiverKind::Owned && !isReferenceCandidate) ||
                    (kind == ReceiverKind::Shared && isReferenceCandidate &&
                     candidateMutability == Mutability::Shared) ||
                    (kind == ReceiverKind::Mutable && isReferenceCandidate &&
                     candidateMutability == Mutability::Mutable);
                if (matches) {
                    auto signature = signatureOfFunction(
                        table, diag, *methodSymbol->function,
                        Ty::makeStruct(structId));
                    if (signature) {
                        signature->receiver = kind;
                        signature->selfType = Ty::makeStruct(structId);
                        return signature;
                    }
                }
            }

            // 派生 clone：&S -> S。
            if (methodName == "clone" && isReferenceCandidate &&
                candidateMutability == Mutability::Shared) {
                const Symbol *structSymbol = table.lookupStruct(structId);
                if (hasDerive(structSymbol, ast::DeriveTrait::Clone)) {
                    CallSignature signature;
                    signature.receiver = ReceiverKind::Shared;
                    signature.element = Ty::makeStruct(structId);
                    signature.result = Ty::makeStruct(structId);
                    return signature;
                }
            }
        }
    }

    diag.error("no matching method '" + methodName + "'");
    return std::nullopt;
}

CallChecker::CallChecker(SymbolTable &table, Diagnostic &diag)
    : table_(table), diag_(diag) {}

std::optional<TyPtr>
CallChecker::checkCall(const ast::CallExpr &call,
                       const std::vector<TyPtr> &argumentTypes,
                       const TyPtr &selfType) {
    auto signature = resolveCallable(table_, diag_, call, selfType);
    if (!signature) {
        return std::nullopt;
    }
    if (argumentTypes.size() != signature->params.size()) {
        diag_.error("argument count mismatch");
        return std::nullopt;
    }
    for (std::size_t index = 0; index < argumentTypes.size(); ++index) {
        if (!Coercion::canCoerce(table_, argumentTypes[index],
                                 signature->params[index])) {
            diag_.error("argument type mismatch");
            return std::nullopt;
        }
    }
    return signature->result;
}

std::optional<TyPtr> CallChecker::checkMethodCall(
    const std::string &methodName, const std::vector<TyPtr> &argumentTypes,
    const TyPtr &receiverType, const TyPtr &selfType) {
    auto signature =
        resolveMethod(table_, diag_, receiverType, methodName, selfType);
    if (!signature) {
        return std::nullopt;
    }
    if (argumentTypes.size() != signature->params.size()) {
        diag_.error("argument count mismatch");
        return std::nullopt;
    }
    for (std::size_t index = 0; index < argumentTypes.size(); ++index) {
        if (!Coercion::canCoerce(table_, argumentTypes[index],
                                 signature->params[index])) {
            diag_.error("argument type mismatch");
            return std::nullopt;
        }
    }
    return signature->result;
}

} // namespace rx::semantic
