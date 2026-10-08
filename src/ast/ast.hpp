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

class ASTNode {
  public:
    virtual ~ASTNode() = default;
    virtual void print(std::ostream &out, std::size_t indent = 0) const = 0;
};

class Item : public ASTNode {};

class Stmt : public ASTNode {};

using ItemPtr = std::unique_ptr<Item>;
using StmtPtr = std::unique_ptr<Stmt>;

struct Expr;
using ExprPtr = std::unique_ptr<Expr>;

class Expr : public ASTNode {
  public:
    void print(std::ostream &out, std::size_t indent = 0) const override = 0;
};

namespace detail {

inline void printIndent(std::ostream &out, std::size_t indent) {
    for (std::size_t i = 0; i < indent; ++i) {
        out << "  ";
    }
}

} // namespace detail

class EmptyStmt final : public Stmt {
 public:
   void print(std::ostream &out, std::size_t indent = 0) const override;
};

class Crate final : public ASTNode {
 public:
   void addItem(ItemPtr item) { items_.push_back(std::move(item)); }

   void print(std::ostream &out, std::size_t indent = 0) const override;

 private:
   std::vector<ItemPtr> items_;
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

std::string_view unaryOpText(UnaryOp op);
UnaryOp unaryOpFromText(std::string_view text);
std::string_view binaryOpText(BinaryOp op);
BinaryOp binaryOpFromText(std::string_view text);

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

struct LetStatement {
    std::string name;
    std::optional<std::string> type;
    ExprPtr initializer;
};

struct Block {
    std::vector<LetStatement> statements;
    ExprPtr tail;
};

struct Function {
    std::string name;
    std::vector<Parameter> parameters;
    std::optional<std::string> returnType;
    Block body;
};

struct Program {
    std::vector<Function> functions;

    void print(std::ostream &out) const;
};

} // namespace rx::ast