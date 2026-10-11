#include "ast.hpp"

#include "ast_visitor.hpp"

#include <stdexcept>
#include <utility>

namespace rx::ast {

namespace {

void invalidOperator() { throw std::logic_error("invalid AST operator"); }

} // namespace

std::string unaryOpText(UnaryOp op) {
    switch (op) {
    case UnaryOp::Negate:
        return "-";
    case UnaryOp::LogicalNot:
        return "!";
    case UnaryOp::Dereference:
        return "*";
    case UnaryOp::Borrow:
        return "&";
    case UnaryOp::BorrowMut:
        return "&mut";
    case UnaryOp::DoubleBorrow:
        return "&&";
    case UnaryOp::DoubleBorrowMut:
        return "&&mut";
    }
    invalidOperator();
}

UnaryOp unaryOpFromText(const std::string &text) {
    if (text == "-") {
        return UnaryOp::Negate;
    }
    if (text == "!") {
        return UnaryOp::LogicalNot;
    }
    if (text == "*") {
        return UnaryOp::Dereference;
    }
    if (text == "&") {
        return UnaryOp::Borrow;
    }
    if (text == "&mut") {
        return UnaryOp::BorrowMut;
    }
    if (text == "&&") {
        return UnaryOp::DoubleBorrow;
    }
    if (text == "&&mut") {
        return UnaryOp::DoubleBorrowMut;
    }
    throw std::invalid_argument("unknown unary operator: " + text);
}

std::string binaryOpText(BinaryOp op) {
    switch (op) {
    case BinaryOp::Add:
        return "+";
    case BinaryOp::Subtract:
        return "-";
    case BinaryOp::Multiply:
        return "*";
    case BinaryOp::Divide:
        return "/";
    case BinaryOp::Remainder:
        return "%";
    case BinaryOp::ShiftLeft:
        return "<<";
    case BinaryOp::ShiftRight:
        return ">>";
    case BinaryOp::BitwiseAnd:
        return "&";
    case BinaryOp::BitwiseXor:
        return "^";
    case BinaryOp::BitwiseOr:
        return "|";
    case BinaryOp::Less:
        return "<";
    case BinaryOp::LessEqual:
        return "<=";
    case BinaryOp::Greater:
        return ">";
    case BinaryOp::GreaterEqual:
        return ">=";
    case BinaryOp::Equal:
        return "==";
    case BinaryOp::NotEqual:
        return "!=";
    case BinaryOp::LogicalAnd:
        return "&&";
    case BinaryOp::LogicalOr:
        return "||";
    case BinaryOp::Assign:
        return "=";
    case BinaryOp::AddAssign:
        return "+=";
    case BinaryOp::SubtractAssign:
        return "-=";
    case BinaryOp::MultiplyAssign:
        return "*=";
    case BinaryOp::DivideAssign:
        return "/=";
    case BinaryOp::RemainderAssign:
        return "%=";
    case BinaryOp::BitwiseAndAssign:
        return "&=";
    case BinaryOp::BitwiseXorAssign:
        return "^=";
    case BinaryOp::BitwiseOrAssign:
        return "|=";
    case BinaryOp::ShiftLeftAssign:
        return "<<=";
    case BinaryOp::ShiftRightAssign:
        return ">>=";
    }
    invalidOperator();
}

BinaryOp binaryOpFromText(const std::string &text) {
    if (text == "+") {
        return BinaryOp::Add;
    }
    if (text == "-") {
        return BinaryOp::Subtract;
    }
    if (text == "*") {
        return BinaryOp::Multiply;
    }
    if (text == "/") {
        return BinaryOp::Divide;
    }
    if (text == "%") {
        return BinaryOp::Remainder;
    }
    if (text == "<<") {
        return BinaryOp::ShiftLeft;
    }
    if (text == ">>") {
        return BinaryOp::ShiftRight;
    }
    if (text == "&") {
        return BinaryOp::BitwiseAnd;
    }
    if (text == "^") {
        return BinaryOp::BitwiseXor;
    }
    if (text == "|") {
        return BinaryOp::BitwiseOr;
    }
    if (text == "<") {
        return BinaryOp::Less;
    }
    if (text == "<=") {
        return BinaryOp::LessEqual;
    }
    if (text == ">") {
        return BinaryOp::Greater;
    }
    if (text == ">=") {
        return BinaryOp::GreaterEqual;
    }
    if (text == "==") {
        return BinaryOp::Equal;
    }
    if (text == "!=") {
        return BinaryOp::NotEqual;
    }
    if (text == "&&") {
        return BinaryOp::LogicalAnd;
    }
    if (text == "||") {
        return BinaryOp::LogicalOr;
    }
    if (text == "=") {
        return BinaryOp::Assign;
    }
    if (text == "+=") {
        return BinaryOp::AddAssign;
    }
    if (text == "-=") {
        return BinaryOp::SubtractAssign;
    }
    if (text == "*=") {
        return BinaryOp::MultiplyAssign;
    }
    if (text == "/=") {
        return BinaryOp::DivideAssign;
    }
    if (text == "%=") {
        return BinaryOp::RemainderAssign;
    }
    if (text == "&=") {
        return BinaryOp::BitwiseAndAssign;
    }
    if (text == "^=") {
        return BinaryOp::BitwiseXorAssign;
    }
    if (text == "|=") {
        return BinaryOp::BitwiseOrAssign;
    }
    if (text == "<<=") {
        return BinaryOp::ShiftLeftAssign;
    }
    if (text == ">>=") {
        return BinaryOp::ShiftRightAssign;
    }
    throw std::invalid_argument("unknown binary operator: " + text);
}

std::string deriveTraitText(DeriveTrait trait) {
    switch (trait) {
    case rx::ast::DeriveTrait::Clone:
        return "Clone";
    case rx::ast::DeriveTrait::PartialEq:
        return "PartialEq";
    case rx::ast::DeriveTrait::Eq:
        return "Eq";
    case rx::ast::DeriveTrait::Copy:
        return "Copy";
    }
    invalidOperator();
}

std::any Crate::accept(ASTVisitor &visitor) { return visitor.visitCrate(this); }

std::any EmptyStmt::accept(ASTVisitor &visitor) {
    return visitor.visitEmptyStmt(this);
}

std::any LetStmt::accept(ASTVisitor &visitor) {
    return visitor.visitLetStmt(this);
}

ExprStmt::ExprStmt(ExprPtr expression) : expression(std::move(expression)) {}

std::any ExprStmt::accept(ASTVisitor &visitor) {
    return visitor.visitExprStmt(this);
}

IntegerLiteralExpr::IntegerLiteralExpr(std::string value)
    : value(std::move(value)) {}

std::any IntegerLiteralExpr::accept(ASTVisitor &visitor) {
    return visitor.visitIntegerLiteralExpr(this);
}

BooleanLiteralExpr::BooleanLiteralExpr(bool value) : value(value) {}

std::any BooleanLiteralExpr::accept(ASTVisitor &visitor) {
    return visitor.visitBooleanLiteralExpr(this);
}

std::any UnitExpr::accept(ASTVisitor &visitor) {
    return visitor.visitUnitExpr(this);
}

PathInExpr::PathInExpr(std::vector<PathExprSegment> segments)
    : segments(std::move(segments)) {}

std::any PathInExpr::accept(ASTVisitor &visitor) {
    return visitor.visitPathInExpr(this);
}

CallExpr::CallExpr(ExprPtr callee, std::vector<ExprPtr> arguments)
    : callee(std::move(callee)), arguments(std::move(arguments)) {}

std::any CallExpr::accept(ASTVisitor &visitor) {
    return visitor.visitCallExpr(this);
}

IndexExpr::IndexExpr(ExprPtr operand, ExprPtr index)
    : operand(std::move(operand)), index(std::move(index)) {}

std::any IndexExpr::accept(ASTVisitor &visitor) {
    return visitor.visitIndexExpr(this);
}

FieldAccessExpr::FieldAccessExpr(ExprPtr operand, PathExprSegment field)
    : operand(std::move(operand)), field(std::move(field)) {}

std::any FieldAccessExpr::accept(ASTVisitor &visitor) {
    return visitor.visitFieldAccessExpr(this);
}

UnaryExpr::UnaryExpr(UnaryOp op, ExprPtr operand)
    : op(op), operand(std::move(operand)) {}

std::any UnaryExpr::accept(ASTVisitor &visitor) {
    return visitor.visitUnaryExpr(this);
}

CastExpr::CastExpr(ExprPtr operand, TypePtr targetType)
    : operand(std::move(operand)), targetType(std::move(targetType)) {}

std::any CastExpr::accept(ASTVisitor &visitor) {
    return visitor.visitCastExpr(this);
}

BinaryExpr::BinaryExpr(BinaryOp op, ExprPtr left, ExprPtr right)
    : op(op), left(std::move(left)), right(std::move(right)) {}

std::any BinaryExpr::accept(ASTVisitor &visitor) {
    return visitor.visitBinaryExpr(this);
}

ArrayExpr::ArrayExpr(std::vector<ExprPtr> elements,
                     std::optional<ConstValue> repeatCount)
    : elements(std::move(elements)), repeatCount(std::move(repeatCount)) {}

std::any ArrayExpr::accept(ASTVisitor &visitor) {
    return visitor.visitArrayExpr(this);
}

BlockExpr::BlockExpr(std::vector<StmtPtr> statements, ExprPtr tail)
    : statements(std::move(statements)), tail(std::move(tail)) {}

std::any BlockExpr::accept(ASTVisitor &visitor) {
    return visitor.visitBlockExpr(this);
}

IfExpr::IfExpr(ExprPtr condition, BlockExpr thenBranch, ExprPtr elseBranch)
    : condition(std::move(condition)), thenBranch(std::move(thenBranch)),
      elseBranch(std::move(elseBranch)) {}

std::any IfExpr::accept(ASTVisitor &visitor) {
    return visitor.visitIfExpr(this);
}

LoopExpr::LoopExpr(BlockExpr body) : body(std::move(body)) {}

std::any LoopExpr::accept(ASTVisitor &visitor) {
    return visitor.visitLoopExpr(this);
}

WhileExpr::WhileExpr(ExprPtr condition, BlockExpr body)
    : condition(std::move(condition)), body(std::move(body)) {}

std::any WhileExpr::accept(ASTVisitor &visitor) {
    return visitor.visitWhileExpr(this);
}

BreakExpr::BreakExpr(ExprPtr value) : value(std::move(value)) {}

std::any BreakExpr::accept(ASTVisitor &visitor) {
    return visitor.visitBreakExpr(this);
}

ReturnExpr::ReturnExpr(ExprPtr value) : value(std::move(value)) {}

std::any ReturnExpr::accept(ASTVisitor &visitor) {
    return visitor.visitReturnExpr(this);
}

std::any ContinueExpr::accept(ASTVisitor &visitor) {
    return visitor.visitContinueExpr(this);
}

StructExprField::StructExprField(std::string name, ExprPtr value)
    : name(std::move(name)), value(std::move(value)) {}

StructExpr::StructExpr(PathInExpr path, std::vector<StructExprField> fields)
    : path(std::move(path)), fields(std::move(fields)) {}

std::any StructExpr::accept(ASTVisitor &visitor) {
    return visitor.visitStructExpr(this);
}

StructField::StructField(std::string name, TypePtr type)
    : name(std::move(name)), type(std::move(type)) {}

std::any StructItem::accept(ASTVisitor &visitor) {
    return visitor.visitStructItem(this);
}

std::any FunctionItem::accept(ASTVisitor &visitor) {
    return visitor.visitFunctionItem(this);
}

DeriveAttribute::DeriveAttribute(std::vector<DeriveTrait> traits)
    : traits(std::move(traits)) {}

std::any ConstantItem::accept(ASTVisitor &visitor) {
    return visitor.visitConstantItem(this);
}

std::any ImplItem::accept(ASTVisitor &visitor) {
    return visitor.visitImplItem(this);
}

std::any UseItem::accept(ASTVisitor &visitor) {
    return visitor.visitUseItem(this);
}

std::any UnitType::accept(ASTVisitor &visitor) {
    return visitor.visitUnitType(this);
}

std::any PathType::accept(ASTVisitor &visitor) {
    return visitor.visitPathType(this);
}

std::any ReferenceType::accept(ASTVisitor &visitor) {
    return visitor.visitReferenceType(this);
}

std::any ArrayType::accept(ASTVisitor &visitor) {
    return visitor.visitArrayType(this);
}

} // namespace rx::ast
