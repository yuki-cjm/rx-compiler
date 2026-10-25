#pragma once

#include "../diag/diagnostic.hpp"
#include "../ty/ty.hpp"

#include <optional>

namespace rx::semantic {

// 控制流检查：return / break / continue 与循环 / never 类型（规范 loop-expr /
// return-expr 等）。
class ControlFlow {
  public:
    // return：值类型（若有）需可强制到声明返回类型。
    static bool checkReturn(Diagnostic &diag, const TyPtr &returnType,
                            const std::optional<TyPtr> &valueType);

    // break：必须在循环内（loopDepth > 0）。
    static bool checkBreak(Diagnostic &diag, int loopDepth);

    // continue：必须在循环内（loopDepth > 0）。
    static bool checkContinue(Diagnostic &diag, int loopDepth);
};

} // namespace rx::semantic
