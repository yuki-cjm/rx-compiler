#include "main_check.hpp"

#include "../resolve/resolver.hpp"
#include "../type/type.hpp"

namespace rx::semantic {

MainChecker::MainChecker(SymbolTable &table, Diagnostic &diag)
    : table_(table), diag_(diag) {}

void MainChecker::check() {
    const Symbol *main = table_.lookupGlobalValue("main");
    if (main == nullptr || main->kind != SymbolKind::Function) {
        diag_.error("an executable needs a top-level main function");
        return;
    }

    const ast::FunctionItem &function = *main->function;
    if (function.genericParams.has_value()) {
        diag_.error(function.span(), "main cannot have generic parameters");
    }
    if (!function.parameters.empty()) {
        diag_.error(function.span(), "main cannot have value parameters");
    }

    TypeResolver resolver(table_, diag_);
    TypePtr returnType = function.returnType != nullptr
                           ? resolver.resolve(*function.returnType, nullptr)
                           : Type::makeUnit();
    if (returnType != nullptr && !isUnit(returnType)) {
        diag_.error(function.span(), "main must return unit");
    }
}

} // namespace rx::semantic
