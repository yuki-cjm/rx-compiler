#include "resolver.hpp"

#include "../consteval/const_eval.hpp"

namespace rx::semantic {

TypeResolver::TypeResolver(SymbolTable &table, Diagnostic &diag)
    : table_(table), diag_(diag) {}

TypePtr TypeResolver::resolve(const ast::Type &type, const TypePtr &selfType) {
    if (auto *unit = dynamic_cast<const ast::UnitType *>(&type)) {
        return Type::makeUnit();
    }
    if (auto *reference = dynamic_cast<const ast::ReferenceType *>(&type)) {
        TypePtr inner = resolve(*reference->inner, selfType);
        if (inner == nullptr) {
            return nullptr;
        }
        Mutability mutability =
            reference->isMutable ? Mutability::Mutable : Mutability::Shared;
        return Type::makeReference(std::move(inner), mutability);
    }
    if (auto *array = dynamic_cast<const ast::ArrayType *>(&type)) {
        TypePtr element = resolve(*array->element, selfType);
        if (element == nullptr) {
            return nullptr;
        }
        auto length =
            evaluateArrayLength(table_, diag_, array->length, selfType);
        if (!length) {
            return nullptr;
        }
        return Type::makeArray(std::move(element), *length);
    }
    if (auto *path = dynamic_cast<const ast::PathType *>(&type)) {
        return resolvePath(*path, selfType);
    }
    diag_.error(type.span(), "unsupported type");
    return nullptr;
}

std::size_t countTypeArguments(
    const std::optional<std::vector<ast::GenericArg>> &arguments) {
    if (!arguments) {
        return 0;
    }
    std::size_t count = 0;
    for (const ast::GenericArg &argument : *arguments) {
        if (argument.type != nullptr) {
            ++count;
        }
    }
    return count;
}

const ast::Type *findTypeArgument(
    const std::optional<std::vector<ast::GenericArg>> &arguments) {
    if (!arguments) {
        return nullptr;
    }
    for (const ast::GenericArg &argument : *arguments) {
        if (argument.type != nullptr) {
            return argument.type.get();
        }
    }
    return nullptr;
}

TypePtr TypeResolver::resolvePath(const ast::PathType &path,
                                const TypePtr &selfType) {
    if (path.segments.size() != 1) {
        diag_.error(path.span(), "multi-segment type paths are unsupported");
        return nullptr;
    }

    const ast::TypePathSegment &segment = path.segments[0];
    if (segment.ident.kind == ast::PathSegmentKind::SelfType) {
        if (selfType == nullptr) {
            diag_.error(path.span(), "'Self' used outside a struct context");
            return nullptr;
        }
        return selfType;
    }
    if (segment.ident.kind != ast::PathSegmentKind::Ident) {
        diag_.error(path.span(), "unresolved type name");
        return nullptr;
    }

    const std::string &name = segment.ident.name;
    const Symbol *symbol = table_.lookupType(name);
    if (symbol == nullptr) {
        diag_.error(path.span(), "unresolved type name '" + name + "'");
        return nullptr;
    }

    std::size_t typeArgCount = countTypeArguments(segment.genericArgs);

    if (symbol->kind == SymbolKind::Struct) {
        if (typeArgCount != 0) {
            diag_.error(path.span(), "structs have no type parameters");
            return nullptr;
        }
        return Type::makeStruct(symbol->structId);
    }

    if (symbol->kind == SymbolKind::BuiltinType) {
        if (name == "Box" || name == "Vec") {
            if (typeArgCount != 1) {
                diag_.error(name + " requires exactly one concrete type "
                                   "argument");
                return nullptr;
            }
            TypePtr element = resolve(*findTypeArgument(segment.genericArgs),
                                    selfType);
            if (element == nullptr) {
                return nullptr;
            }
            return name == "Box" ? Type::makeBox(std::move(element))
                                 : Type::makeVec(std::move(element));
        }
        if (typeArgCount != 0) {
            diag_.error(path.span(), "primitives take no type arguments");
            return nullptr;
        }
        return symbol->type;
    }

    diag_.error(path.span(), "unresolved type name '" + name + "'");
    return nullptr;
}

} // namespace rx::semantic
