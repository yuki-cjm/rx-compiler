#include "ast.hpp"

#include <utility>

namespace rx::ast {

LiteralExpr::LiteralExpr(std::string value) : value(std::move(value)) {}

void LiteralExpr::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "Literal " << value << '\n';
}

IntegerLiteralExpr::IntegerLiteralExpr(std::string value)
    : value(std::move(value)) {}

void IntegerLiteralExpr::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "IntegerLiteral " << value << '\n';
}

BooleanLiteralExpr::BooleanLiteralExpr(bool value) : value(value) {}

void BooleanLiteralExpr::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "BooleanLiteral " << (value ? "true" : "false") << '\n';
}

void UnitExpr::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "Unit\n";
}

NameExpr::NameExpr(std::string name) : name(std::move(name)) {}

void NameExpr::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "Name " << name << '\n';
}

PathExpr::PathExpr(std::vector<std::string> segments)
    : segments(std::move(segments)) {}

void PathExpr::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "Path\n";
    for (const auto &segment : segments) {
        detail::printIndent(out, indent + 1);
        out << "Segment " << segment << '\n';
    }
}

UnaryExpr::UnaryExpr(std::string op, ExprPtr operand)
    : op(std::move(op)), operand(std::move(operand)) {}

void UnaryExpr::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "Unary " << op << '\n';
    operand->print(out, indent + 1);
}

BinaryExpr::BinaryExpr(std::string op, ExprPtr left, ExprPtr right)
    : op(std::move(op)), left(std::move(left)), right(std::move(right)) {}

void BinaryExpr::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "Binary " << op << '\n';
    left->print(out, indent + 1);
    right->print(out, indent + 1);
}

void EmptyStmt::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "EmptyStmt\n";
}

void Crate::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "Crate\n";
    for (const auto &item : items_) {
        item->print(out, indent + 1);
    }
}

void Program::print(std::ostream &out) const {
    out << "Program\n";
    for (const auto &function : functions) {
        out << "  Function " << function.name;
        if (function.returnType) {
            out << " -> " << *function.returnType;
        }
        out << '\n';

        for (const auto &parameter : function.parameters) {
            out << "    Parameter " << parameter.name << ": " << parameter.type
                << '\n';
        }

        out << "    Block\n";
        for (const auto &statement : function.body.statements) {
            out << "      Let " << statement.name;
            if (statement.type) {
                out << ": " << *statement.type;
            }
            out << '\n';
            statement.initializer->print(out, 4);
        }
        if (function.body.tail) {
            out << "      Tail\n";
            function.body.tail->print(out, 4);
        }
    }
}

} // namespace rx::ast