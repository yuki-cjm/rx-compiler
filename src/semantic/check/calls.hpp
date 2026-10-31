#pragma once

#include "../../ast/ast.hpp"
#include "../symbol/symbol_table.hpp"
#include "../diag/diagnostic.hpp"
#include "../ty/ty.hpp"
#include "../builtin/builtin.hpp"

#include <optional>
#include <string>
#include <vector>

namespace rx::semantic {

// 解析后的可调用签名（供 typeck 在检查实参前取得形参类型）。
struct CallSignature {
    std::vector<TyPtr> params;                  // 形参类型（不含 receiver）
    TyPtr result;                               // 返回类型
    ReceiverKind receiver = ReceiverKind::None; // 方法 receiver 形态
    TyPtr element;                              // 内建容器 / 派生 clone 元素类型
    TyPtr selfType;                             // 方法所属类型（receiver 可变性检查用）
};

// 解析函数 / 关联函数 / 内建函数调用的签名（callee 为路径表达式）。
// 失败返回 nullopt 并写入 diag。
std::optional<CallSignature>
resolveCallable(SymbolTable &table, Diagnostic &diag, const ast::CallExpr &call,
                const TyPtr &selfType);

// 在 receiverType 上按方法查找规则解析方法签名。
// 失败返回 nullopt 并写入 diag。
std::optional<CallSignature>
resolveMethod(SymbolTable &table, Diagnostic &diag, const TyPtr &receiverType,
              const std::string &methodName, const TyPtr &selfType);

// 函数 / 关联函数 / 方法调用检查（规范 call-expr / method-call-expr）。
// 与 typeck 解耦：参数表达式的类型已由 typeck 检查完毕后传入，
// 本类只做「签名匹配 + 实参形参逐位检查 + receiver 调整」。
class CallChecker {
  public:
    CallChecker(SymbolTable &table, Diagnostic &diag);

    // 检查普通 / 关联函数调用（callee 是路径，如 f() 或 Type::assoc_fn()）。
    // argumentTypes 为已检查的实参类型列表（不含 receiver）。
    // selfType 为当前 Self 上下文（无则 nullptr）。
    // 返回调用结果类型；失败返回 nullopt 并写入 diag。
    std::optional<TyPtr> checkCall(const ast::CallExpr &call,
                                   const std::vector<TyPtr> &argumentTypes,
                                   const TyPtr &selfType);

    // 检查方法调用（receiver.method(args)）。
    // receiverType 为接收者表达式的类型；argumentTypes 为其余实参类型。
    // 返回调用结果类型；失败返回 nullopt 并写入 diag。
    std::optional<TyPtr>
    checkMethodCall(const std::string &methodName,
                    const std::vector<TyPtr> &argumentTypes,
                    const TyPtr &receiverType, const TyPtr &selfType);

  private:
    SymbolTable &table_;
    Diagnostic &diag_;
};

} // namespace rx::semantic
