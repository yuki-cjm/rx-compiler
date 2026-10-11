#pragma once

#include "ast.hpp"
#include "ast_visitor.hpp"

#include <any>
#include <cstddef>
#include <ostream>

namespace rx::ast {

class ASTPrinter final : public ASTVisitor {
  public:
    explicit ASTPrinter(std::ostream &out) : out_(out) {}

    void printProgram(Program &program);

    // Crate.
    std::any visitCrate(Crate *node) override;

    // Items.
    std::any visitFunctionItem(FunctionItem *node) override;
    std::any visitStructItem(StructItem *node) override;
    std::any visitConstantItem(ConstantItem *node) override;
    std::any visitImplItem(ImplItem *node) override;
    std::any visitUseItem(UseItem *node) override;

    // Statements.
    std::any visitEmptyStmt(EmptyStmt *node) override;
    std::any visitLetStmt(LetStmt *node) override;
    std::any visitExprStmt(ExprStmt *node) override;

    // Expressions.
    std::any visitIntegerLiteralExpr(IntegerLiteralExpr *node) override;
    std::any visitBooleanLiteralExpr(BooleanLiteralExpr *node) override;
    std::any visitUnitExpr(UnitExpr *node) override;
    std::any visitPathInExpr(PathInExpr *node) override;
    std::any visitCallExpr(CallExpr *node) override;
    std::any visitIndexExpr(IndexExpr *node) override;
    std::any visitFieldAccessExpr(FieldAccessExpr *node) override;
    std::any visitUnaryExpr(UnaryExpr *node) override;
    std::any visitCastExpr(CastExpr *node) override;
    std::any visitBinaryExpr(BinaryExpr *node) override;
    std::any visitArrayExpr(ArrayExpr *node) override;
    std::any visitBlockExpr(BlockExpr *node) override;
    std::any visitIfExpr(IfExpr *node) override;
    std::any visitLoopExpr(LoopExpr *node) override;
    std::any visitWhileExpr(WhileExpr *node) override;
    std::any visitBreakExpr(BreakExpr *node) override;
    std::any visitReturnExpr(ReturnExpr *node) override;
    std::any visitContinueExpr(ContinueExpr *node) override;
    std::any visitStructExpr(StructExpr *node) override;

    // Types.
    std::any visitUnitType(UnitType *node) override;
    std::any visitPathType(PathType *node) override;
    std::any visitReferenceType(ReferenceType *node) override;
    std::any visitArrayType(ArrayType *node) override;

  private:
    void dispatch(ASTNode *node);

    void printPathSegment(const PathSegment &segment, std::size_t indent);
    void printPathExprSegment(const PathExprSegment &segment, std::size_t indent);
    void printTypePathSegment(const TypePathSegment &segment, std::size_t indent);
    void printGenericArg(const GenericArg &arg, std::size_t indent);
    void printIdentifierBinding(const IdentifierBinding &binding,
                                std::size_t indent);
    void printFunctionParam(const FunctionParam &param, std::size_t indent);
    void printSelfParam(const SelfParam &param, std::size_t indent);
    void printWhereClauseItem(const WhereClauseItem &item, std::size_t indent);
    void printWhereClause(const WhereClause &clause, std::size_t indent);
    void printLifetimeParam(const LifetimeParam &param, std::size_t indent);
    void printGenericParams(const GenericParams &params, std::size_t indent);
    void printStructExprField(const StructExprField &field, std::size_t indent);
    void printStructField(const StructField &field, std::size_t indent);
    void printDeriveAttribute(const DeriveAttribute &attribute,
                              std::size_t indent);
    void printMagnitude(const Magnitude &magnitude, std::size_t indent);
    void printConstValue(const ConstValue &value, std::size_t indent);
    void printUsePath(const UsePath &path, std::size_t indent);
    void printUseTree(const UseTree &tree, std::size_t indent);

    void printIndent(std::size_t indent) const;

    std::ostream &out_;
    std::size_t indent_ = 0;
};

} // namespace rx::ast
