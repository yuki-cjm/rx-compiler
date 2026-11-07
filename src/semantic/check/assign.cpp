#include "assign.hpp"

#include "coercion.hpp"
#include "operators.hpp"

namespace rx::semantic {

bool AssignChecker::check(const SymbolTable &table, const PlaceInfo &target,
                          const TypePtr &valueType, Diagnostic &diag) {
    if (!target.isMutable) {
        diag.error("assignment to an immutable place");
        return false;
    }
    if (!Coercion::canCoerce(table, valueType, target.type)) {
        diag.error("assigned value type is incompatible with the destination");
        return false;
    }
    return true;
}

bool AssignChecker::checkCompound(const SymbolTable &table,
                                  const PlaceInfo &target, ast::BinaryOp op,
                                  const TypePtr &rightType, Diagnostic &diag) {
    if (!target.isMutable) {
        diag.error("compound assignment to an immutable place");
        return false;
    }
    if (!Operators::checkCompoundAssignment(op, target.type, rightType)) {
        diag.error("compound assignment operands are incompatible");
        return false;
    }
    return true;
}

} // namespace rx::semantic
