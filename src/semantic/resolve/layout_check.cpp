#include "layout_check.hpp"

#include "../symbol/symbol.hpp"
#include "../type/type.hpp"

#include <functional>
#include <set>

namespace rx::semantic {

LayoutChecker::LayoutChecker(SymbolTable &table, Diagnostic &diag)
    : table_(table), diag_(diag) {}

namespace {

// 沿 Struct / Array 递归
// Reference / Box / Vec 提供间接，终止递归
void collectInlineStructs(const TypePtr &type, std::set<StructId> &out) {
    if (type == nullptr) {
        return;
    }
    if (type->kind == TypeKind::Struct) {
        out.insert(type->structId);
        return;
    }
    if (type->kind == TypeKind::Array) {
        collectInlineStructs(type->inner, out);
        return;
    }
}

} // namespace

void LayoutChecker::check(const ast::Program &program) {
    std::map<StructId, std::vector<StructId>> inlineEdges;
    std::vector<StructId> order;

    for (const ast::ItemPtr &item : program.items) {
        auto *structure = dynamic_cast<const ast::StructItem *>(item.get());
        if (structure == nullptr) {
            continue;
        }
        const Symbol *symbol = table_.lookupType(structure->name);
        if (symbol == nullptr) {
            continue;
        } // 一般不发生
        order.push_back(symbol->structId);
        std::set<StructId> targets;
        for (const FieldInfo &field : symbol->fields) {
            collectInlineStructs(field.type, targets);
        }
        inlineEdges[symbol->structId].assign(targets.begin(), targets.end());
    }

    std::map<StructId, int> color;
    std::function<bool(StructId)> visit = [&](StructId node) {
        color[node] = 1;
        auto found = inlineEdges.find(node);
        if (found != inlineEdges.end()) {
            for (StructId next : found->second) {
                if (color[next] == 0) {
                    if (visit(next)) {
                        return true;
                    }
                } else if (color[next] == 1) {
                    diag_.error("recursive type has infinite inline layout");
                    return true;
                }
            }
        }
        color[node] = 2;
        return false;
    };

    for (StructId node : order) {
        if (color[node] == 0) {
            visit(node);
        }
    }
}

} // namespace rx::semantic
