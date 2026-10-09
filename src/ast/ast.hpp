#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
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

std::string_view unaryOpText(UnaryOp op);
UnaryOp unaryOpFromText(std::string_view text);
std::string_view binaryOpText(BinaryOp op);
BinaryOp binaryOpFromText(std::string_view text);
std::string_view deriveTraitText(DeriveTrait trait);
DeriveTrait deriveTraitFromText(std::string_view text);

class ASTNode {
  public:
    virtual ~ASTNode() = default;
    virtual void print(std::ostream &out, std::size_t indent = 0) const = 0;
};

class Item : public ASTNode {};
class Stmt : public ASTNode {};
class Expr : public ASTNode {
  public:
    void print(std::ostream &out, std::size_t indent = 0) const override = 0;
};

using ItemPtr = std::unique_ptr<Item>;
using StmtPtr = std::unique_ptr<Stmt>;
using ExprPtr = std::unique_ptr<Expr>;

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
    std::optional<std::string> type;
    ExprPtr initializer;
};

struct ExprStmt : public Stmt {
    ExprStmt(ExprPtr expression);
    void print(std::ostream &out, std::size_t indent) const override;

    ExprPtr expression;
};

struct LiteralExpr final : Expr {
    explicit LiteralExpr(std::string value);
    void print(std::ostream &out, std::size_t indent) const override;

    std::string value;
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

struct NameExpr final : Expr {
    explicit NameExpr(std::string name);
    void print(std::ostream &out, std::size_t indent) const override;

    std::string name;
};

struct PathExpr final : Expr {
    explicit PathExpr(std::vector<std::string> segments);
    void print(std::ostream &out, std::size_t indent) const override;

    std::vector<std::string> segments;
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
    CastExpr(ExprPtr operand, std::string targetType);
    void print(std::ostream &out, std::size_t indent) const override;

    ExprPtr operand;
    std::string targetType;
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
              std::optional<std::string> repeatCount = std::nullopt);
    void print(std::ostream &out, std::size_t indent) const override;

    std::vector<ExprPtr> elements;
    std::optional<std::string> repeatCount;
};

struct Parameter {
    std::string name;
    std::string type;
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
    StructExpr(PathExpr path,
               std::vector<StructExprField> fields);

    void print(std::ostream &out, std::size_t indent) const override;

    PathExpr path;
    std::vector<StructExprField> fields;
};

struct StructField {
    StructField(std::string name, std::string type);
    void print(std::ostream &out, std::size_t indent) const;

    std::string name;
    std::string type;
};

struct Function final : Item {
    void print(std::ostream &out, std::size_t indent = 0) const override;

    std::string name;
    std::optional<GenericParam> genericParams;
    std::vector<Parameter> parameters;
    std::optional<std::string> returnType;
    BlockExpr body;
};

struct StructItem final : Item {
    void print(std::ostream &out, std::size_t indent = 0) const override;

    std::string name;
    std::optional<GenericParam> genericParams;
    std::vector<StructField> fields;
};

struct DeriveAttribute {
    DeriveAttribute(std::vector<DeriveTrait> traits);
    void print(std::ostream &out, std::size_t indent) const;

    std::vector<DeriveTrait> traits;
};

struct Program {
    std::vector<ItemPtr> items;

    void print(std::ostream &out) const;
};

} // namespace rx::ast