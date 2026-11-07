#include "calls.hpp"

#include "../builtin/builtin_box.hpp"
#include "../builtin/builtin_scalar.hpp"
#include "../builtin/builtin_traits.hpp"
#include "../builtin/builtin_vec.hpp"
#include "../resolve/path_resolve.hpp"
#include "../resolve/resolver.hpp"

namespace rx::semantic {

namespace {

ReceiverKind receiverKindOf(const ast::SelfParam &self) {
    if (!self.isBorrowed) {
        return ReceiverKind::Owned;
    }
    return self.isMutable ? ReceiverKind::Mutable : ReceiverKind::Shared;
}

std::optional<CallSignature>
signatureOfFunction(SymbolTable &table, Diagnostic &diag,
                    const ast::FunctionItem &function,
                    const TypePtr &selfType) {
    TypeResolver resolver(table, diag);
    CallSignature signature;
    if (function.returnType != nullptr) {
        signature.result = resolver.resolve(*function.returnType, selfType);
        if (signature.result == nullptr) {
            return std::nullopt;
        }
    } else {
        signature.result = Type::makeUnit();
    }
    for (const ast::FunctionParam &parameter : function.parameters) {
        TypePtr type = resolver.resolve(*parameter.type, selfType);
        if (type == nullptr) {
            return std::nullopt;
        }
        signature.params.push_back(std::move(type));
    }
    return signature;
}

std::optional<CallSignature> builtinFunctionSignature(const std::string &name) {
    CallSignature signature;
    if (name == "get_i32") {
        signature.result = Type::makeScalar(TypeKind::I32);
        return signature;
    }
    if (name == "print_i32" || name == "println_i32") {
        signature.params.push_back(Type::makeScalar(TypeKind::I32));
        signature.result = Type::makeUnit();
        return signature;
    }
    return std::nullopt;
}

bool isContainerPath(const ast::PathInExpr &path) {
    return path.segments.size() == 2 &&
           path.segments[0].ident.kind == ast::PathSegmentKind::Ident &&
           (path.segments[0].ident.name == "Box" ||
            path.segments[0].ident.name == "Vec");
}

std::optional<TypePtr> containerElement(SymbolTable &table, Diagnostic &diag,
                                        const ast::PathInExpr &path) {
    const ast::PathExprSegment &first = path.segments[0];
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

std::optional<CallSignature> resolveContainerCall(SymbolTable &table,
                                                  Diagnostic &diag,
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
    if (builtin->receiver == ReceiverKind::None) {
        signature.params = builtin->params;
    } else {
        TypePtr base = (container == "Box") ? Type::makeBox(*element)
                                            : Type::makeVec(*element);
        TypePtr receiverType =
            builtin->receiver == ReceiverKind::Mutable
                ? Type::makeReference(base, Mutability::Mutable)
                : Type::makeReference(base, Mutability::Shared);
        signature.params.push_back(std::move(receiverType));
        for (TypePtr parameter : builtin->params) {
            signature.params.push_back(std::move(parameter));
        }
    }
    return signature;
}

} // namespace

std::optional<CallSignature> resolveCallable(SymbolTable &table,
                                             Diagnostic &diag,
                                             const ast::CallExpr &call,
                                             const TypePtr &selfType) {
    auto *path = dynamic_cast<const ast::PathInExpr *>(call.callee.get());
    if (path == nullptr) {
        diag.error("callee is not a callable path");
        return std::nullopt;
    }

    if (isContainerPath(*path)) {
        return resolveContainerCall(table, diag, *path);
    }

    PathResolver resolver(table, diag);

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
            if (typeSymbol == nullptr) {
                diag.error("unresolved type name '" + first.ident.name + "'");
                return std::nullopt;
            }
            if (typeSymbol->kind != SymbolKind::Struct) {
                diag.error("'" + first.ident.name + "' is not a struct");
                return std::nullopt;
            }
            structId = typeSymbol->structId;
        } else {
            diag.error("invalid value path");
            return std::nullopt;
        }

        const Symbol *associated =
            table.lookupAssoc(structId, second.ident.name);
        if (associated != nullptr && associated->kind == SymbolKind::Function) {
            TypePtr targetSelf = Type::makeStruct(structId);
            auto signature = signatureOfFunction(
                table, diag, *associated->function, targetSelf);
            if (!signature) {
                return std::nullopt;
            }
            if (associated->hasReceiver) {
                signature->receiver =
                    receiverKindOf(*associated->function->selfParam);
                TypePtr base = Type::makeStruct(structId);
                TypePtr receiverType;
                if (signature->receiver == ReceiverKind::Owned) {
                    receiverType = base;
                } else if (signature->receiver == ReceiverKind::Mutable) {
                    receiverType =
                        Type::makeReference(base, Mutability::Mutable);
                } else {
                    receiverType =
                        Type::makeReference(base, Mutability::Shared);
                }
                signature->params.insert(signature->params.begin(),
                                         receiverType);
            }
            return signature;
        }

        // 关联项存在但不是函数（如关联常量）→ 不可调用。
        if (associated != nullptr) {
            diag.error("'" + second.ident.name + "' is not callable");
            return std::nullopt;
        }

        if (second.ident.name == "clone") {
            const Symbol *structure = table.lookupStruct(structId);
            if (hasDerive(structure, ast::DeriveTrait::Clone)) {
                CallSignature signature;
                signature.receiver = ReceiverKind::Shared;
                TypePtr base = Type::makeStruct(structId);
                signature.params.push_back(
                    Type::makeReference(base, Mutability::Shared));
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

    TypePtr targetSelf = symbol->implTarget != 0
                             ? Type::makeStruct(symbol->implTarget)
                             : nullptr;
    auto signature =
        signatureOfFunction(table, diag, *symbol->function, targetSelf);
    if (!signature) {
        return std::nullopt;
    }
    return signature;
}

std::optional<CallSignature> resolveMethod(SymbolTable &table, Diagnostic &diag,
                                           const TypePtr &receiverType,
                                           const std::string &methodName) {
    std::vector<TypePtr> levels;
    TypePtr current = receiverType;
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

    for (const TypePtr &level : levels) {
        std::vector<TypePtr> candidates;
        candidates.push_back(level);
        candidates.push_back(Type::makeReference(level, Mutability::Shared));
        candidates.push_back(Type::makeReference(level, Mutability::Mutable));

        for (const TypePtr &candidate : candidates) {
            if (isReference(candidate) &&
                candidate->mutability == Mutability::Shared) {
                if (isBox(candidate->inner)) {
                    TypePtr element = candidate->inner->inner;
                    if (methodName == "clone" && isClone(table, element)) {
                        CallSignature signature;
                        signature.receiver = ReceiverKind::Shared;
                        signature.result = Type::makeBox(element);
                        return signature;
                    }
                }
                if (isVec(candidate->inner)) {
                    TypePtr element = candidate->inner->inner;
                    CallSignature signature;
                    signature.receiver = ReceiverKind::Shared;
                    if (methodName == "len") {
                        signature.result = Type::makeScalar(TypeKind::Usize);
                        return signature;
                    }
                    if (methodName == "is_empty") {
                        signature.result = Type::makeScalar(TypeKind::Bool);
                        return signature;
                    }
                    if (methodName == "clone" && isClone(table, element)) {
                        signature.result = Type::makeVec(element);
                        return signature;
                    }
                }
                if (isArray(candidate->inner) && methodName == "len") {
                    CallSignature signature;
                    signature.receiver = ReceiverKind::Shared;
                    signature.result = Type::makeScalar(TypeKind::Usize);
                    return signature;
                }
                if (isArray(candidate->inner) && methodName == "clone" &&
                    isClone(table, candidate->inner->inner)) {
                    CallSignature signature;
                    signature.receiver = ReceiverKind::Shared;
                    signature.result = candidate->inner;
                    return signature;
                }
                if (methodName == "clone" &&
                    (isScalar(candidate->inner) || isBool(candidate->inner))) {
                    CallSignature signature;
                    signature.receiver = ReceiverKind::Shared;
                    signature.result = candidate->inner;
                    return signature;
                }
                // 共享引用 clone：&T.clone() 复制引用本身，无需 T: Clone。
                if (methodName == "clone" && isReference(candidate->inner) &&
                    candidate->inner->mutability == Mutability::Shared) {
                    CallSignature signature;
                    signature.receiver = ReceiverKind::Shared;
                    signature.result = candidate->inner;
                    return signature;
                }
            }
            if (isReference(candidate) &&
                candidate->mutability == Mutability::Mutable &&
                isVec(candidate->inner)) {
                TypePtr element = candidate->inner->inner;
                if (methodName == "push") {
                    CallSignature signature;
                    signature.receiver = ReceiverKind::Mutable;
                    signature.selfType = Type::makeVec(element);
                    signature.params.push_back(element);
                    signature.result = Type::makeUnit();
                    return signature;
                }
                if (methodName == "remove") {
                    CallSignature signature;
                    signature.receiver = ReceiverKind::Mutable;
                    signature.selfType = Type::makeVec(element);
                    signature.params.push_back(
                        Type::makeScalar(TypeKind::Usize));
                    signature.result = element;
                    return signature;
                }
            }

            StructId structId = 0;
            bool isReferenceCandidate = isReference(candidate);
            Mutability candidateMutability = Mutability::Shared;
            const TypePtr *structType = &candidate;
            if (isReferenceCandidate) {
                candidateMutability = candidate->mutability;
                structType = &candidate->inner;
            }
            if (!isStruct(*structType)) {
                continue;
            }
            structId = (*structType)->structId;

            const Symbol *methodSymbol =
                table.lookupAssoc(structId, methodName);
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
                        Type::makeStruct(structId));
                    if (signature) {
                        signature->receiver = kind;
                        signature->selfType = Type::makeStruct(structId);
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
                    signature.result = Type::makeStruct(structId);
                    return signature;
                }
            }
        }
    }

    diag.error("no matching method '" + methodName + "'");
    return std::nullopt;
}

} // namespace rx::semantic
