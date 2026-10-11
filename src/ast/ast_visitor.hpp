#pragma once

#include "ast.hpp"

#include <any>

namespace rx::ast {

class ASTVisitor {
  public:
    virtual ~ASTVisitor() = default;

    // Crate.
    virtual std::any visitCrate(Crate *node) = 0;

    // Items.
    virtual std::any visitFunctionItem(FunctionItem *node) = 0;
    virtual std::any visitStructItem(StructItem *node) = 0;
    virtual std::any visitConstantItem(ConstantItem *node) = 0;
    virtual std::any visitImplItem(ImplItem *node) = 0;
    virtual std::any visitUseItem(UseItem *node) = 0;

    // Statements.
    virtual std::any visitEmptyStmt(EmptyStmt *node) = 0;
    virtual std::any visitLetStmt(LetStmt *node) = 0;
    virtual std::any visitExprStmt(ExprStmt *node) = 0;

    // Expressions.
    virtual std::any visitIntegerLiteralExpr(IntegerLiteralExpr *node) = 0;
    virtual std::any visitBooleanLiteralExpr(BooleanLiteralExpr *node) = 0;
    virtual std::any visitUnitExpr(UnitExpr *node) = 0;
    virtual std::any visitPathInExpr(PathInExpr *node) = 0;
    virtual std::any visitCallExpr(CallExpr *node) = 0;
    virtual std::any visitIndexExpr(IndexExpr *node) = 0;
    virtual std::any visitFieldAccessExpr(FieldAccessExpr *node) = 0;
    virtual std::any visitUnaryExpr(UnaryExpr *node) = 0;
    virtual std::any visitCastExpr(CastExpr *node) = 0;
    virtual std::any visitBinaryExpr(BinaryExpr *node) = 0;
    virtual std::any visitArrayExpr(ArrayExpr *node) = 0;
    virtual std::any visitBlockExpr(BlockExpr *node) = 0;
    virtual std::any visitIfExpr(IfExpr *node) = 0;
    virtual std::any visitLoopExpr(LoopExpr *node) = 0;
    virtual std::any visitWhileExpr(WhileExpr *node) = 0;
    virtual std::any visitBreakExpr(BreakExpr *node) = 0;
    virtual std::any visitReturnExpr(ReturnExpr *node) = 0;
    virtual std::any visitContinueExpr(ContinueExpr *node) = 0;
    virtual std::any visitStructExpr(StructExpr *node) = 0;

    // Types.
    virtual std::any visitUnitType(UnitType *node) = 0;
    virtual std::any visitPathType(PathType *node) = 0;
    virtual std::any visitReferenceType(ReferenceType *node) = 0;
    virtual std::any visitArrayType(ArrayType *node) = 0;
};

} // namespace rx::ast
