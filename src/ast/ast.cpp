#include "ast.hpp"

#include <utility>

namespace rx::ast {
namespace {

void printIndent(std::ostream &out, std::size_t indent) {
    for (std::size_t i = 0; i < indent; ++i) {
        out << "  ";
    }
}

} // namespace

LiteralExpr::LiteralExpr(std::string value) : value(std::move(value)) {}

void LiteralExpr::print(std::ostream &out, std::size_t indent) const {
    printIndent(out, indent);
    out << "Literal " << value << '\n';
}

NameExpr::NameExpr(std::string name) : name(std::move(name)) {}

void NameExpr::print(std::ostream &out, std::size_t indent) const {
    printIndent(out, indent);
    out << "Name " << name << '\n';
}

BinaryExpr::BinaryExpr(std::string op, ExprPtr left, ExprPtr right)
    : op(std::move(op)), left(std::move(left)), right(std::move(right)) {}

void BinaryExpr::print(std::ostream &out, std::size_t indent) const {
    printIndent(out, indent);
    out << "Binary " << op << '\n';
    left->print(out, indent + 1);
    right->print(out, indent + 1);
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
            out << "    Parameter " << parameter.name << ": " << parameter.type << '\n';
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