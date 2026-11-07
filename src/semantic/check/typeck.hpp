#pragma once

#include "../../ast/ast_visitor.hpp"
#include "../symbol/symbol_table.hpp"
#include "../diag/diagnostic.hpp"
#include "../type/type.hpp"

#include <any>
#include <optional>
#include <unordered_map>

namespace rx::semantic {

// Pass 2：类型检查主 visitor。
// 具体规则分散在 check/ 下的子模块（infer / coercion / operators / calls /
// place / assign / control_flow），本类只负责调度与上下文维护。
class TypeChecker : public ast::ASTVisitor {
  public:
    TypeChecker(SymbolTable &table, Diagnostic &diag);

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
    TypePtr checkExpr(ast::Expr &expr, const std::optional<TypePtr> &expected);

    // 记录表达式类型到 side table
    void recordType(const ast::Expr &expr, const TypePtr &type);

    // 查询已记录的表达式类型
    TypePtr typeOf(const ast::Expr &expr) const;

    enum class PlaceAccess {
        Mutable,
        Owned,
        Shared,
    };

    struct PlaceResult {
        TypePtr type;
        PlaceAccess access;
    };
    std::optional<PlaceResult> classifyPlace(ast::Expr &expr);
    PlaceAccess baseAccess(ast::Expr &expr);

    // 检查单个函数体（顶层或 impl 关联方法）。
    void checkFunctionBody(ast::FunctionItem &function, const TypePtr &selfType);

    // 解析常量符号的声明类型。
    TypePtr resolveConstantType(const Symbol &symbol);

    // 表达式是否发散（不完整正常执行）。
    bool exprDiverges(ast::Expr &expr);
    bool stmtDiverges(ast::Stmt &stmt);

    SymbolTable &table_;
    Diagnostic &diag_;

    // ---- 检查上下文 ----
    std::optional<TypePtr> currentReturn_;
    TypePtr currentSelf_;
    int loopDepth_ = 0;
    std::optional<TypePtr> currentLoopBreak_;
    std::vector<std::vector<TypePtr>> loopBreakValues_;
    std::vector<bool> loopIsWhile_;
    std::vector<int> conditionBarriers_;

    std::optional<TypePtr> expected_;

    std::unordered_map<const ast::Expr *, TypePtr> exprTypes_;
};

} // namespace rx::semantic
