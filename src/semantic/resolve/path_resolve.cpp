#include "path_resolve.hpp"

#include "../type/type.hpp"

namespace rx::semantic {

PathResolver::PathResolver(SymbolTable &table, Diagnostic &diag)
    : table_(table), diag_(diag) {}

const Symbol *PathResolver::resolveValuePath(const ast::PathInExpr &path,
                                             const TypePtr &selfType) {
    if (path.segments.empty()) {
        diag_.error(path.span(), "empty value path");
        return nullptr;
    }

    if (path.segments.size() == 1) {
        const ast::PathExprSegment &segment = path.segments[0];
        if (segment.genericArgs.has_value()) {
            diag_.error(path.span(), "functions have no type parameters");
            return nullptr;
        }
        if (segment.ident.kind == ast::PathSegmentKind::SelfValue) {
            const Symbol *symbol = table_.resolveValue("self");
            if (symbol == nullptr) {
                diag_.error(path.span(), "'self' used outside a method");
                return nullptr;
            }
            return symbol;
        }
        if (segment.ident.kind == ast::PathSegmentKind::SelfType) {
            diag_.error(path.span(), "'Self' is a type, not a value");
            return nullptr;
        }
        const Symbol *symbol = table_.resolveValue(segment.ident.name);
        if (symbol == nullptr) {
            diag_.error(path.span(), "unresolved value name '" + segment.ident.name + "'");
            return nullptr;
        }
        return symbol;
    }

    if (path.segments.size() == 2) {
        const ast::PathExprSegment &first = path.segments[0];
        const ast::PathExprSegment &second = path.segments[1];

        StructId structId = 0;
        if (first.ident.kind == ast::PathSegmentKind::SelfType) {
            if (selfType == nullptr || !isStruct(selfType)) {
                diag_.error(path.span(), "'Self' used outside a struct context");
                return nullptr;
            }
            structId = selfType->structId;
        } else if (first.ident.kind == ast::PathSegmentKind::Ident) {
            if (first.ident.name == "Box" || first.ident.name == "Vec") {
                // 内建容器路径由 typeck 特殊处理
                return nullptr;
            }
            const Symbol *typeSymbol = table_.lookupType(first.ident.name);
            if (typeSymbol == nullptr) {
                diag_.error(path.span(),
                            "unresolved type name '" + first.ident.name + "'");
                return nullptr;
            }
            if (typeSymbol->kind != SymbolKind::Struct) {
                diag_.error(path.span(),
                            "'" + first.ident.name + "' is not a struct");
                return nullptr;
            }
            structId = typeSymbol->structId;
        } else {
            diag_.error(path.span(), "invalid value path");
            return nullptr;
        }

        const Symbol *symbol = table_.lookupAssoc(structId, second.ident.name);
        if (symbol == nullptr) {
            diag_.error(path.span(), "no associated item '" + second.ident.name + "'");
            return nullptr;
        }
        return symbol;
    }

    diag_.error(path.span(), "invalid value path");
    return nullptr;
}

} // namespace rx::semantic
