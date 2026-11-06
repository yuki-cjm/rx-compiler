#pragma once

#include "../../ast/ast_visitor.hpp"
#include "../symbol/symbol_table.hpp"
#include "../diag/diagnostic.hpp"
#include "../ty/ty.hpp"

#include <any>
#include <optional>
#include <unordered_map>

namespace rx::semantic {

// Pass 2：类型检查主 visitor。
// 实现 ast::ASTVisitor，递归遍历 AST，为每个表达式计算类型（TyPtr），
// 同时做名字解析、类型推断、运算符检查、强制转换、控制流检查等。
// 具体规则分散在 check/ 下的子模块（infer / coercion / operators / calls /
// place / assign / control_flow），本类只负责调度与上下文维护。
class TypeChecker : public ast::ASTVisitor {
  public:
    TypeChecker(SymbolTable &table, Diagnostic &diag);

    // 对整个程序做类型检查。返回 true 表示无新增错误。
    bool check(const ast::Program &program);

    // ---- ast::ASTVisitor 实现 ----
    std::any visitCrate(ast::Crate *node) override;
    std::any visitFunctionItem(ast::FunctionItem *node) override;
    std::any visitStructItem(ast::StructItem *node) override;
    std::any visitConstantItem(ast::ConstantItem *node) override;
    std::any visitImplItem(ast::ImplItem *node) override;
    std::any visitUseItem(ast::UseItem *node) override;

    std::any visitEmptyStmt(ast::EmptyStmt *node) override;
    std::any visitLetStmt(ast::LetStmt *node) override;
    std::any visitExprStmt(ast::ExprStmt *node) override;

    std::any visitIntegerLiteralExpr(ast::IntegerLiteralExpr *node) override;
    std::any visitBooleanLiteralExpr(ast::BooleanLiteralExpr *node) override;
    std::any visitUnitExpr(ast::UnitExpr *node) override;
    std::any visitPathInExpr(ast::PathInExpr *node) override;
    std::any visitCallExpr(ast::CallExpr *node) override;
    std::any visitIndexExpr(ast::IndexExpr *node) override;
    std::any visitFieldAccessExpr(ast::FieldAccessExpr *node) override;
    std::any visitUnaryExpr(ast::UnaryExpr *node) override;
    std::any visitCastExpr(ast::CastExpr *node) override;
    std::any visitBinaryExpr(ast::BinaryExpr *node) override;
    std::any visitArrayExpr(ast::ArrayExpr *node) override;
    std::any visitBlockExpr(ast::BlockExpr *node) override;
    std::any visitIfExpr(ast::IfExpr *node) override;
    std::any visitLoopExpr(ast::LoopExpr *node) override;
    std::any visitWhileExpr(ast::WhileExpr *node) override;
    std::any visitBreakExpr(ast::BreakExpr *node) override;
    std::any visitReturnExpr(ast::ReturnExpr *node) override;
    std::any visitContinueExpr(ast::ContinueExpr *node) override;
    std::any visitStructExpr(ast::StructExpr *node) override;

    std::any visitUnitType(ast::UnitType *node) override;
    std::any visitPathType(ast::PathType *node) override;
    std::any visitReferenceType(ast::ReferenceType *node) override;
    std::any visitArrayType(ast::ArrayType *node) override;

  private:
    // 在给定期望类型下检查一个表达式，返回其类型。
    // 会临时设置 expected_ 再调用 expr.accept(*this)。
    TyPtr checkExpr(ast::Expr &expr, const std::optional<TyPtr> &expected);

    // 记录表达式类型到 side table（供 place / calls 等后续查询）。
    void recordType(const ast::Expr &expr, const TyPtr &ty);

    // 查询已记录的表达式类型。
    TyPtr typeOf(const ast::Expr &expr) const;

    // ---- place 分类（赋值左值 / 借用 / 方法 receiver 的可变性检查）----
    enum class PlaceAccess {
        Mutable,  // 可写
        Owned,    // 不可写但拥有其值（&mut 可继续解引用）
        Shared,   // 共享借用（&mut 不可再解引用）
    };
    struct PlaceResult {
        TyPtr type;
        PlaceAccess access;
    };
    std::optional<PlaceResult> classifyPlace(ast::Expr &expr);
    PlaceAccess baseAccess(ast::Expr &expr);

    // 检查单个函数体（顶层或 impl 关联方法）。
    void checkFunctionBody(ast::FunctionItem &function, const TyPtr &selfType);

    // 解析常量符号的声明类型。
    TyPtr resolveConstantType(const Symbol &symbol);

    // 表达式是否发散（不完整正常执行）。
    bool exprDiverges(ast::Expr &expr);
    bool stmtDiverges(ast::Stmt &stmt);

    SymbolTable &table_;
    Diagnostic &diag_;

    // ---- 检查上下文 ----
    std::optional<TyPtr> currentReturn_;    // 当前函数返回类型（无标注 = Unit）
    TyPtr currentSelf_;                     // 当前方法 / impl 的 Self 类型（nullptr = 无）
    int loopDepth_ = 0;                     // 循环嵌套深度
    std::optional<TyPtr> currentLoopBreak_; // 当前 loop 的 break 目标类型
    std::vector<std::vector<TyPtr>> loopBreakValues_; // 各层 loop 收集的 break 值类型
    std::vector<bool> loopIsWhile_;         // 各层循环是否为 while
    std::vector<int> conditionBarriers_;    // while 条件的 break 屏障深度

    // ---- 期望类型（临时上下文）----
    std::optional<TyPtr> expected_;

    // 表达式 → 类型 side table（不改 AST，供后续 IR 复用）。
    std::unordered_map<const ast::Expr *, TyPtr> exprTypes_;
};

} // namespace rx::semantic
