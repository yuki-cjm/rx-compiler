#pragma once

#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace rx::ast {

struct Expr;
using ExprPtr = std::unique_ptr<Expr>;

struct Expr {
  virtual ~Expr() = default;
  virtual void print(std::ostream &out, std::size_t indent) const = 0;
};

struct LiteralExpr final : Expr {
  explicit LiteralExpr(std::string value);
  void print(std::ostream &out, std::size_t indent) const override;

  std::string value;
};

struct NameExpr final : Expr {
  explicit NameExpr(std::string name);
  void print(std::ostream &out, std::size_t indent) const override;

  std::string name;
};

struct BinaryExpr final : Expr {
  BinaryExpr(std::string op, ExprPtr left, ExprPtr right);
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