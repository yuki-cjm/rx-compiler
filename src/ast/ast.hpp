#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace rx::ast {

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
DeriveTrait deriveTraitFromText(const std::string &text);

class ASTNode {
  public:
    virtual ~ASTNode() = default;
    virtual void print(std::ostream &out, std::size_t indent = 0) const = 0;
};

class Item : public ASTNode {};
class Stmt : public ASTNode {};
class Expr : public ASTNode {};
class Type : public ASTNode {};

using ItemPtr = std::unique_ptr<Item>;
using StmtPtr = std::unique_ptr<Stmt>;
using ExprPtr = std::unique_ptr<Expr>;
using TypePtr = std::unique_ptr<Type>;

struct ConstValue {
    void print(std::ostream &out, std::size_t indent) const;

    enum class Kind {
        Integer,
        Boolean,
        Path,
        Negate,
    };

    Kind kind;
    std::string value;
    bool boolValue = false;
    std::unique_ptr<ConstValue> operand;
};

struct PathSegment {
    void print(std::ostream &out, std::size_t indent) const;

    PathSegmentKind kind = PathSegmentKind::Ident;
    std::string name;
};

struct GenericArg {
    void print(std::ostream &out, std::size_t indent) const;

    std::optional<std::string> lifetime;
    TypePtr type;
};

struct TypePathSegment {
    void print(std::ostream &out, std::size_t indent) const;

    PathSegment ident;
    std::optional<std::vector<GenericArg>> genericArgs;
};

struct PathExprSegment {
    void print(std::ostream &out, std::size_t indent) const;

    PathSegment ident;
    std::optional<std::vector<GenericArg>> genericArgs;
};

struct UnitType final : Type {
    void print(std::ostream &out, std::size_t indent) const override;
};

struct PathType final : Type {
    void print(std::ostream &out, std::size_t indent) const override;

    std::vector<TypePathSegment> segments;
};

struct ReferenceType final : Type {
    void print(std::ostream &out, std::size_t indent) const override;

    std::optional<std::string> lifetime;
    bool isMutable = false;
    TypePtr inner;
};

struct ArrayType final : Type {
    void print(std::ostream &out, std::size_t indent) const override;

    TypePtr element;
    ConstValue length;
};

namespace detail {

inline void printIndent(std::ostream &out, std::size_t indent) {
    for (std::size_t i = 0; i < indent; ++i) {
        out << "  ";
    }
}

} // namespace detail

class Crate final : public ASTNode {
  public:
    void addItem(ItemPtr item) { items_.push_back(std::move(item)); }

    void print(std::ostream &out, std::size_t indent = 0) const override;

  private:
    std::vector<ItemPtr> items_;
};

class EmptyStmt final : public Stmt {
  public:
    void print(std::ostream &out, std::size_t indent = 0) const override;
};

struct LetStmt : public Stmt {
    void print(std::ostream &out, std::size_t indent) const override;

    std::string name;
    bool isMutable = false;
    TypePtr type;
    ExprPtr initializer;
};

struct ExprStmt : public Stmt {
    ExprStmt(ExprPtr expression);
    void print(std::ostream &out, std::size_t indent) const override;

    ExprPtr expression;
};

struct IntegerLiteralExpr final : Expr {
    explicit IntegerLiteralExpr(std::string value);
    void print(std::ostream &out, std::size_t indent) const override;

    std::string value;
};

struct BooleanLiteralExpr final : Expr {
    explicit BooleanLiteralExpr(bool value);
    void print(std::ostream &out, std::size_t indent) const override;

    bool value;
};

struct UnitExpr final : Expr {
    void print(std::ostream &out, std::size_t indent) const override;
};

struct PathInExpr final : Expr {
    explicit PathInExpr(std::vector<PathExprSegment> segments);
    void print(std::ostream &out, std::size_t indent) const override;

    std::vector<PathExprSegment> segments;
};

struct CallExpr final : Expr {
    CallExpr(ExprPtr callee, std::vector<ExprPtr> arguments);
    void print(std::ostream &out, std::size_t indent) const override;

    ExprPtr callee;
    std::vector<ExprPtr> arguments;
};

struct IndexExpr final : Expr {
    IndexExpr(ExprPtr operand, ExprPtr index);
    void print(std::ostream &out, std::size_t indent) const override;

    ExprPtr operand;
    ExprPtr index;
};

struct FieldAccessExpr final : Expr {
    FieldAccessExpr(ExprPtr operand, std::string field);
    void print(std::ostream &out, std::size_t indent) const override;

    ExprPtr operand;
    std::string field;
};

struct UnaryExpr final : Expr {
    UnaryExpr(UnaryOp op, ExprPtr operand);
    void print(std::ostream &out, std::size_t indent) const override;

    UnaryOp op;
    ExprPtr operand;
};

struct CastExpr final : Expr {
    CastExpr(ExprPtr operand, TypePtr targetType);
    void print(std::ostream &out, std::size_t indent) const override;

    ExprPtr operand;
    TypePtr targetType;
};

struct BinaryExpr final : Expr {
    BinaryExpr(BinaryOp op, ExprPtr left, ExprPtr right);
    void print(std::ostream &out, std::size_t indent) const override;

    BinaryOp op;
    ExprPtr left;
    ExprPtr right;
};

struct ArrayExpr final : Expr {
    ArrayExpr(std::vector<ExprPtr> elements,
              std::optional<ConstValue> repeatCount = std::nullopt);
    void print(std::ostream &out, std::size_t indent) const override;

    std::vector<ExprPtr> elements;
    std::optional<ConstValue> repeatCount;
};

struct Parameter {
    std::string name;
    bool isMutable = false;
    TypePtr type;
};

struct SelfParam {
    void print(std::ostream &out, std::size_t indent) const;

    bool isBorrowed = false;
    bool isMutable = false;
    std::optional<std::string> lifetime;
};

struct WhereClauseItem {
    void print(std::ostream &out, std::size_t indent) const;

    std::optional<std::string> lifetime;
    TypePtr type;
    std::vector<std::string> bounds;
};

struct WhereClause {
    void print(std::ostream &out, std::size_t indent) const;

    std::vector<WhereClauseItem> items;
};

struct LifetimeParam {
    void print(std::ostream &out, std::size_t indent) const;

    std::string lifetime;
    std::optional<std::vector<std::string>> lifetimeBounds;
};

struct GenericParam {
    void print(std::ostream &out, std::size_t indent) const;

    std::vector<LifetimeParam> params;
};

struct BlockExpr final : Expr {
    explicit BlockExpr(std::vector<StmtPtr> statements = {}, ExprPtr tail = {});
    void print(std::ostream &out, std::size_t indent) const override;

    std::vector<StmtPtr> statements;
    ExprPtr tail;
};

struct IfExpr final : Expr {
    IfExpr(ExprPtr condition, BlockExpr thenBranch, ExprPtr elseBranch);
    void print(std::ostream &out, std::size_t indent) const override;

    ExprPtr condition;
    BlockExpr thenBranch;
    ExprPtr elseBranch;
};

struct LoopExpr final : Expr {
    LoopExpr(BlockExpr body);
    void print(std::ostream &out, std::size_t indent) const override;

    BlockExpr body;
};

struct WhileExpr final : Expr {
    WhileExpr(ExprPtr condition, BlockExpr body);
    void print(std::ostream &out, std::size_t indent) const override;

    ExprPtr condition;
    BlockExpr body;
};

struct BreakExpr final : Expr {
    BreakExpr(ExprPtr value);
    void print(std::ostream &out, std::size_t indent) const override;

    ExprPtr value;
};

struct ReturnExpr final : Expr {
    ReturnExpr(ExprPtr value);
    void print(std::ostream &out, std::size_t indent) const override;

    ExprPtr value;
};

struct ContinueExpr final : Expr {
    void print(std::ostream &out, std::size_t indent) const override;
};

struct StructExprField {
    StructExprField(std::string name, ExprPtr value);
    void print(std::ostream &out, std::size_t indent) const;

    std::string name;
    ExprPtr value;
};

struct StructExpr final : Expr {
    StructExpr(PathInExpr path, std::vector<StructExprField> fields);

    void print(std::ostream &out, std::size_t indent) const override;

    PathInExpr path;
    std::vector<StructExprField> fields;
};

struct StructField {
    StructField(std::string name, TypePtr type);
    void print(std::ostream &out, std::size_t indent) const;

    std::string name;
    TypePtr type;
};

struct FunctionItem final : Item {
    void print(std::ostream &out, std::size_t indent = 0) const override;

    std::string name;
    std::optional<GenericParam> genericParams;
    std::optional<SelfParam> selfParam;
    std::vector<Parameter> parameters;
    TypePtr returnType;
    std::optional<WhereClause> whereClause;
    BlockExpr body;
};

struct DeriveAttribute {
    DeriveAttribute(std::vector<DeriveTrait> traits);
    void print(std::ostream &out, std::size_t indent) const;

    std::vector<DeriveTrait> traits;
};

struct StructItem final : Item {
    void print(std::ostream &out, std::size_t indent = 0) const override;

    std::string name;
    std::optional<GenericParam> genericParams;
    std::vector<DeriveAttribute> attributes;
    std::vector<StructField> fields;
    std::optional<WhereClause> whereClause;
};

struct ConstantItem final : Item {
    void print(std::ostream &out, std::size_t indent = 0) const override;

    std::string name;
    TypePtr type;
    ConstValue value;
};

struct ImplItem final : Item {
    void print(std::ostream &out, std::size_t indent = 0) const override;

    TypePtr typeName;
    std::optional<GenericParam> genericParams;
    std::optional<WhereClause> whereClause;
    std::vector<ItemPtr> associatedItems;
};

struct UsePath {
    void print(std::ostream &out, std::size_t indent) const;

    bool leadingPathSep = false;
    std::vector<PathSegment> segments;
};

struct UseTree {
    void print(std::ostream &out, std::size_t indent) const;

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
    void print(std::ostream &out, std::size_t indent = 0) const override;

    UseTree tree;
};

struct Program {
    std::vector<ItemPtr> items;

    void print(std::ostream &out) const;
};

} // namespace rx::ast