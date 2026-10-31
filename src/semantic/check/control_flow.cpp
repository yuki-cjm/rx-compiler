#include "control_flow.hpp"

#include "../ty/ty_compare.hpp"

namespace rx::semantic {

bool ControlFlow::checkReturn(Diagnostic &diag, const TyPtr &returnType,
                              const std::optional<TyPtr> &valueType) {
    TyPtr value = valueType.value_or(Ty::makeUnit());
    if (sameTy(value, returnType) || isNever(value)) {
        return true;
    }
    diag.error("return value does not match the function return type");
    return false;
}

bool ControlFlow::checkBreak(Diagnostic &diag, int loopDepth) {
    if (loopDepth <= 0) {
        diag.error("break used outside a loop");
        return false;
    }
    return true;
}

bool ControlFlow::checkContinue(Diagnostic &diag, int loopDepth) {
    if (loopDepth <= 0) {
        diag.error("continue used outside a loop");
        return false;
    }
    return true;
}

} // namespace rx::semantic
