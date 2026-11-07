#pragma once

#include "../../ast/ast.hpp"
#include "../builtin/builtin.hpp"
#include "../diag/diagnostic.hpp"
#include "../symbol/symbol_table.hpp"
#include "../type/type.hpp"

#include <optional>
#include <string>
#include <vector>

namespace rx::semantic {

struct CallSignature {
    std::vector<TypePtr> params;
    TypePtr result;
    ReceiverKind receiver = ReceiverKind::None;
    TypePtr selfType;
};

std::optional<CallSignature> resolveCallable(SymbolTable &table, Diagnostic &diag, const ast::CallExpr &call, const TypePtr &selfType);
std::optional<CallSignature> resolveMethod(SymbolTable &table, Diagnostic &diag, const TypePtr &receiverType, const std::string &methodName);

} // namespace rx::semantic
