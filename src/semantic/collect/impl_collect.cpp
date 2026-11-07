#include "impl_collect.hpp"

#include "../resolve/resolver.hpp"
#include "../type/type.hpp"
#include "duplicate_check.hpp"

namespace rx::semantic {

ImplCollector::ImplCollector(SymbolTable &table, Diagnostic &diag)
    : table_(table), diag_(diag) {}

void ImplCollector::collect(const ast::ImplItem &item) {
    TypeResolver resolver(table_, diag_);
    TypePtr target = resolver.resolve(*item.typeName, nullptr);
    if (target == nullptr) {
        return;
    }
    if (!isStruct(target)) {
        diag_.error(item.span(), "impl target must be a user-defined struct");
        return;
    }

    StructId structId = target->structId;
    for (const ast::ItemPtr &associated : item.associatedItems) {
        if (auto *function =
                dynamic_cast<const ast::FunctionItem *>(associated.get())) {
            checkDuplicateParameters(*function, diag_);

            Symbol symbol;
            symbol.kind = SymbolKind::Function;
            symbol.name = function->name;
            symbol.function = function;
            symbol.hasReceiver = function->selfParam.has_value();
            symbol.implTarget = structId;
            if (!table_.declareAssoc(structId, function->name, symbol)) {
                diag_.error(function->span(),
                            "duplicate associated item '" + function->name +
                                "'");
            }
        } else if (auto *constant =
                       dynamic_cast<const ast::ConstantItem *>(
                           associated.get())) {
            Symbol symbol;
            symbol.kind = SymbolKind::Constant;
            symbol.name = constant->name;
            symbol.constant = constant;
            symbol.implTarget = structId;
            if (!table_.declareAssoc(structId, constant->name, symbol)) {
                diag_.error(constant->span(),
                            "duplicate associated item '" + constant->name +
                                "'");
            }
        }
    }
}

} // namespace rx::semantic
