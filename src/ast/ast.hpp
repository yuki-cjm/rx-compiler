#pragma once

#include <any>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace rx::ast {

class ASTVisitor;

enum class UnaryOp {
    Negate,
    LogicalNot,
    Dereference,
    Borrow,
    BorrowMut,
    DoubleBorrow,
    DoubleBorrowMut,
};

enum class BinaryOp {
    Add,
    Subtract,
    Multiply,
    Divide,
    Remainder,
    ShiftLeft,
    ShiftRight,
    BitwiseAnd,
    BitwiseXor,
    BitwiseOr,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    Equal,
    NotEqual,
    LogicalAnd,
    LogicalOr,
    Assign,
    AddAssign,
    SubtractAssign,
    MultiplyAssign,
    DivideAssign,
    RemainderAssign,
    BitwiseAndAssign,
    BitwiseXorAssign,
    BitwiseOrAssign,
    ShiftLeftAssign,
    ShiftRightAssign,
};

enum class DeriveTrait {
    Copy,
    Clone,
    PartialEq,
    Eq,
};

enum class PathSegmentKind {
    Ident,
    SelfValue,
    SelfType,
    Super,
    Crate,
};

std::string unaryOpText(UnaryOp op);
UnaryOp unaryOpFromText(const std::string &text);
std::string binaryOpText(BinaryOp op);
BinaryOp binaryOpFromText(const std::string &text);
std::string deriveTraitText(DeriveTrait trait);

class ASTNode {
  public:
    virtual ~ASTNode() = default;
    virtual std::any accept(ASTVisitor &visitor) = 0;
};

class Item : public ASTNode {};
class Stmt : public ASTNode {};
class Expr : public ASTNode {};
class Type : public ASTNode {};

using ItemPtr = std::unique_ptr<Item>;
using StmtPtr = std::unique_ptr<Stmt>;
using ExprPtr = std::unique_ptr<Expr>;
using TypePtr = std::unique_ptr<Type>;

struct Magnitude {
    enum class Kind {
        Integer,
        Path,
    };

    Kind kind;
    std::string value;
};

struct ConstValue {
    enum class Kind {
        Integer,
        Boolean,
        Path,
        Negate,
    };

    Kind kind;
    std::string value;
    bool boolValue = false;
    Magnitude operand;
};

struct PathSegment {
    PathSegmentKind kind = PathSegmentKind::Ident;
    std::string name;
};

struct GenericArg {
    std::optional<std::string> lifetime;
    TypePtr type;
};

struct TypePathSegment {
    PathSegment ident;
    std::optional<std::vector<GenericArg>> genericArgs;
};

struct PathExprSegment {
    PathSegment ident;
    std::optional<std::vector<GenericArg>> genericArgs;
};

struct UnitType final : Type {
    std::any accept(ASTVisitor &visitor) override;
};

struct PathType final : Type {
    std::any accept(ASTVisitor &visitor) override;

    std::vector<TypePathSegment> segments;
};

struct ReferenceType final : Type {
    std::any accept(ASTVisitor &visitor) override;

    std::optional<std::string> lifetime;
    bool isMutable = false;
    TypePtr inner;
};

struct ArrayType final : Type {
    std::any accept(ASTVisitor &visitor) override;

    TypePtr element;
    ConstValue length;
};

class Crate final : public ASTNode {
  public:
    void addItem(ItemPtr item) { items_.push_back(std::move(item)); }

    std::any accept(ASTVisitor &visitor) override;

    std::vector<ItemPtr> &items() { return items_; }
    const std::vector<ItemPtr> &items() const { return items_; }

  private:
    std::vector<ItemPtr> items_;
};

class EmptyStmt final : public Stmt {
  public:
    std::any accept(ASTVisitor &visitor) override;
};

struct IdentifierBinding {
    std::string name;
    bool isMutable = false;
};

struct LetStmt : public Stmt {
    std::any accept(ASTVisitor &visitor) override;

    IdentifierBinding binding;
    TypePtr type;
    ExprPtr initializer;
};

struct ExprStmt : public Stmt {
    ExprStmt(ExprPtr expression);
    std::any accept(ASTVisitor &visitor) override;

    ExprPtr expression;
};

struct IntegerLiteralExpr final : Expr {
    explicit IntegerLiteralExpr(std::string value);
    std::any accept(ASTVisitor &visitor) override;

    std::string value;
};

struct BooleanLiteralExpr final : Expr {
    explicit BooleanLiteralExpr(bool value);
    std::any accept(ASTVisitor &visitor) override;

    bool value;
};

struct UnitExpr final : Expr {
    std::any accept(ASTVisitor &visitor) override;
};

struct PathInExpr final : Expr {
    explicit PathInExpr(std::vector<PathExprSegment> segments);
    std::any accept(ASTVisitor &visitor) override;

    std::vector<PathExprSegment> segments;
};

struct CallExpr final : Expr {
    CallExpr(ExprPtr callee, std::vector<ExprPtr> arguments);
    std::any accept(ASTVisitor &visitor) override;

    ExprPtr callee;
    std::vector<ExprPtr> arguments;
};

struct IndexExpr final : Expr {
    IndexExpr(ExprPtr operand, ExprPtr index);
    std::any accept(ASTVisitor &visitor) override;

    ExprPtr operand;
    ExprPtr index;
};

struct FieldAccessExpr final : Expr {
    FieldAccessExpr(ExprPtr operand, PathExprSegment field);
    std::any accept(ASTVisitor &visitor) override;

    ExprPtr operand;
    PathExprSegment field;
};

struct UnaryExpr final : Expr {
    UnaryExpr(UnaryOp op, ExprPtr operand);
    std::any accept(ASTVisitor &visitor) override;

    UnaryOp op;
    ExprPtr operand;
};

struct CastExpr final : Expr {
    CastExpr(ExprPtr operand, TypePtr targetType);
    std::any accept(ASTVisitor &visitor) override;

    ExprPtr operand;
    TypePtr targetType;
};

struct BinaryExpr final : Expr {
    BinaryExpr(BinaryOp op, ExprPtr left, ExprPtr right);
    std::any accept(ASTVisitor &visitor) override;

    BinaryOp op;
    ExprPtr left;
    ExprPtr right;
};

struct ArrayExpr final : Expr {
    ArrayExpr(std::vector<ExprPtr> elements,
              std::optional<ConstValue> repeatCount = std::nullopt);
    std::any accept(ASTVisitor &visitor) override;

    std::vector<ExprPtr> elements;
    std::optional<ConstValue> repeatCount;
};

struct FunctionParam {
    IdentifierBinding binding;
    TypePtr type;
};

struct SelfParam {
    bool isBorrowed = false;
    bool isMutable = false;
    std::optional<std::string> lifetime;
};

struct WhereClauseItem {
    std::optional<std::string> lifetime;
    TypePtr type;
    std::vector<std::string> bounds;
};

struct WhereClause {
    std::vector<WhereClauseItem> items;
};

struct LifetimeParam {
    std::string lifetime;
    std::optional<std::vector<std::string>> lifetimeBounds;
};

struct GenericParams {
    std::vector<LifetimeParam> params;
};

struct BlockExpr final : Expr {
    explicit BlockExpr(std::vector<StmtPtr> statements = {}, ExprPtr tail = {});
    std::any accept(ASTVisitor &visitor) override;

    std::vector<StmtPtr> statements;
    ExprPtr tail;
};

struct IfExpr final : Expr {
    IfExpr(ExprPtr condition, BlockExpr thenBranch, ExprPtr elseBranch);
    std::any accept(ASTVisitor &visitor) override;

    ExprPtr condition;
    BlockExpr thenBranch;
    ExprPtr elseBranch;
};

struct LoopExpr final : Expr {
    LoopExpr(BlockExpr body);
    std::any accept(ASTVisitor &visitor) override;

    BlockExpr body;
};

struct WhileExpr final : Expr {
    WhileExpr(ExprPtr condition, BlockExpr body);
    std::any accept(ASTVisitor &visitor) override;

    ExprPtr condition;
    BlockExpr body;
};

struct BreakExpr final : Expr {
    BreakExpr(ExprPtr value);
    std::any accept(ASTVisitor &visitor) override;

    ExprPtr value;
};

struct ReturnExpr final : Expr {
    ReturnExpr(ExprPtr value);
    std::any accept(ASTVisitor &visitor) override;

    ExprPtr value;
};

struct ContinueExpr final : Expr {
    std::any accept(ASTVisitor &visitor) override;
};

struct StructExprField {
    StructExprField(std::string name, ExprPtr value);

    std::string name;
    ExprPtr value;
};

struct StructExpr final : Expr {
    StructExpr(PathInExpr path, std::vector<StructExprField> fields);

    std::any accept(ASTVisitor &visitor) override;

    PathInExpr path;
    std::vector<StructExprField> fields;
};

struct StructField {
    StructField(std::string name, TypePtr type);

    std::string name;
    TypePtr type;
};

struct FunctionItem final : Item {
    std::any accept(ASTVisitor &visitor) override;

    std::string name;
    std::optional<GenericParams> genericParams;
    std::optional<SelfParam> selfParam;
    std::vector<FunctionParam> parameters;
    TypePtr returnType;
    std::optional<WhereClause> whereClause;
    BlockExpr body;
};

struct DeriveAttribute {
    DeriveAttribute(std::vector<DeriveTrait> traits);

    std::vector<DeriveTrait> traits;
};

struct StructItem final : Item {
    std::any accept(ASTVisitor &visitor) override;

    std::string name;
    std::optional<GenericParams> genericParams;
    std::vector<DeriveAttribute> attributes;
    std::vector<StructField> fields;
    std::optional<WhereClause> whereClause;
};

struct ConstantItem final : Item {
    std::any accept(ASTVisitor &visitor) override;

    std::string name;
    TypePtr type;
    ConstValue value;
};

struct ImplItem final : Item {
    std::any accept(ASTVisitor &visitor) override;

    TypePtr typeName;
    std::optional<GenericParams> genericParams;
    std::optional<WhereClause> whereClause;
    std::vector<ItemPtr> associatedItems;
};

struct UsePath {
    bool leadingPathSep = false;
    std::vector<PathSegment> segments;
};

struct UseTree {
    enum class Kind {
        Glob,
        List,
        Path,
    };

    Kind kind;

    std::optional<UsePath> prefix;
    bool trailingPathSep = false;

    std::vector<UseTree> items;

    UsePath path;
    std::optional<std::string> alias;
    bool aliasIsUnderscore = false;
};

struct UseItem final : Item {
    std::any accept(ASTVisitor &visitor) override;

    UseTree tree;
};

struct Program {
    std::vector<ItemPtr> items;
};

} // namespace rx::ast
