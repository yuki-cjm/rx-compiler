#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
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

struct UnaryExpr final : Expr {
    UnaryExpr(std::string op, ExprPtr operand);
    void print(std::ostream &out, std::size_t indent) const override;

    std::string op;
    ExprPtr operand;
};

struct BinaryExpr final : Expr {
    BinaryExpr(std::string op, ExprPtr left, ExprPtr right);
    void print(std::ostream &out, std::size_t indent) const override;

    std::string op;
    ExprPtr left;
    ExprPtr right;
};

struct ComparisonExpr final : Expr {
    ComparisonExpr(std::string op, ExprPtr left, ExprPtr right);
    void print(std::ostream &out, std::size_t indent) const override;

    std::string op;
    ExprPtr left;
    ExprPtr right;
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