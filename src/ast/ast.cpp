#include "ast.hpp"

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

DeriveTrait deriveTraitFromText(const std::string &text) {
    if (text == "Copy") {
        return rx::ast::DeriveTrait::Copy;
    } else if (text == "Clone") {
        return rx::ast::DeriveTrait::Clone;
    } else if (text == "PartialEq") {
        return rx::ast::DeriveTrait::PartialEq;
    } else if (text == "Eq") {
        return rx::ast::DeriveTrait::Eq;
    }
    throw std::invalid_argument("unknown derive trait: " + text);
}

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

PathExpr::PathExpr(std::vector<PathSegment> segments)
    : segments(std::move(segments)) {}

void PathSegment::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    switch (kind) {
    case PathSegmentKind::Ident:
        out << "Ident " << name << '\n';
        break;
    case PathSegmentKind::SelfValue:
        out << "SelfValue\n";
        break;
    case PathSegmentKind::SelfType:
        out << "SelfType\n";
        break;
    case PathSegmentKind::Super:
        out << "Super\n";
        break;
    case PathSegmentKind::Crate:
        out << "Crate\n";
        break;
    }
}

void PathExpr::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "Path\n";
    for (const auto &segment : segments) {
        segment.print(out, indent + 1);
    }
}

CallExpr::CallExpr(ExprPtr callee, std::vector<ExprPtr> arguments)
    : callee(std::move(callee)), arguments(std::move(arguments)) {}

void CallExpr::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "Call\n";
    detail::printIndent(out, indent + 1);
    out << "Callee\n";
    callee->print(out, indent + 2);
    detail::printIndent(out, indent + 1);
    out << "Arguments\n";
    for (const auto &argument : arguments) {
        argument->print(out, indent + 2);
    }
}

IndexExpr::IndexExpr(ExprPtr operand, ExprPtr index)
    : operand(std::move(operand)), index(std::move(index)) {}

void IndexExpr::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "Index\n";
    operand->print(out, indent + 1);
    index->print(out, indent + 1);
}

FieldAccessExpr::FieldAccessExpr(ExprPtr operand, std::string field)
    : operand(std::move(operand)), field(std::move(field)) {}

void FieldAccessExpr::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "FieldAccess " << field << '\n';
    operand->print(out, indent + 1);
}

UnaryExpr::UnaryExpr(UnaryOp op, ExprPtr operand)
    : op(op), operand(std::move(operand)) {}

void UnaryExpr::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "Unary " << unaryOpText(op) << '\n';
    operand->print(out, indent + 1);
}

void GenericArg::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    if (lifetime) {
        out << "Lifetime " << *lifetime << '\n';
    } else {
        out << "Type\n";
        type->print(out, indent + 1);
    }
}

void TypePathSegment::print(std::ostream &out, std::size_t indent) const {
    ident.print(out, indent);
    if (genericArgs) {
        detail::printIndent(out, indent + 1);
        out << "GenericArgs\n";
        for (const auto &arg : *genericArgs) {
            arg.print(out, indent + 2);
        }
    }
}

void UnitType::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "UnitType\n";
}

void PathType::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "PathType\n";
    for (const auto &segment : segments) {
        segment.print(out, indent + 1);
    }
}

void ReferenceType::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "ReferenceType";
    out << (isDouble ? " &&" : " &");
    if (lifetime) {
        out << ' ' << *lifetime;
    }
    if (isMutable) {
        out << " mut";
    }
    out << '\n';
    inner->print(out, indent + 1);
}

void ArrayType::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "ArrayType\n";
    element->print(out, indent + 1);
    detail::printIndent(out, indent + 1);
    out << "Length\n";
    length.print(out, indent + 2);
}

CastExpr::CastExpr(ExprPtr operand, TypePtr targetType)
    : operand(std::move(operand)), targetType(std::move(targetType)) {}

void CastExpr::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "Cast\n";
    operand->print(out, indent + 1);
    detail::printIndent(out, indent + 1);
    out << "Type\n";
    targetType->print(out, indent + 2);
}

BinaryExpr::BinaryExpr(BinaryOp op, ExprPtr left, ExprPtr right)
    : op(op), left(std::move(left)), right(std::move(right)) {}

void BinaryExpr::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "Binary " << binaryOpText(op) << '\n';
    left->print(out, indent + 1);
    right->print(out, indent + 1);
}

ArrayExpr::ArrayExpr(std::vector<ExprPtr> elements,
                     std::optional<ConstValue> repeatCount)
    : elements(std::move(elements)), repeatCount(std::move(repeatCount)) {}

void ArrayExpr::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << (repeatCount ? "ArrayRepeat\n" : "Array\n");
    for (const auto &element : elements) {
        element->print(out, indent + 1);
    }
    if (repeatCount) {
        detail::printIndent(out, indent + 1);
        out << "RepeatCount\n";
        repeatCount->print(out, indent + 2);
    }
}

BlockExpr::BlockExpr(std::vector<StmtPtr> statements, ExprPtr tail)
    : statements(std::move(statements)), tail(std::move(tail)) {}

void LetStmt::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "Let " << name << '\n';
    if (type) {
        detail::printIndent(out, indent + 1);
        out << "Type\n";
        type->print(out, indent + 2);
    }
    initializer->print(out, indent + 1);
}

void BlockExpr::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "Block\n";
    for (const auto &statement : statements) {
        statement->print(out, indent + 1);
    }
    if (tail) {
        detail::printIndent(out, indent + 1);
        out << "Tail\n";
        tail->print(out, indent + 2);
    }
}

IfExpr::IfExpr(ExprPtr condition, BlockExpr thenBranch, ExprPtr elseBranch)
    : condition(std::move(condition)), thenBranch(std::move(thenBranch)),
      elseBranch(std::move(elseBranch)) {}

void IfExpr::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "If\n";
    condition->print(out, indent + 1);
    thenBranch.print(out, indent + 2);
    if (elseBranch) {
        detail::printIndent(out, indent + 1);
        out << "Else\n";
        elseBranch->print(out, indent + 2);
    }
}

LoopExpr::LoopExpr(BlockExpr body) : body(std::move(body)) {}

void LoopExpr::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "Loop\n";
    body.print(out, indent + 1);
}

WhileExpr::WhileExpr(ExprPtr condition, BlockExpr body)
    : condition(std::move(condition)), body(std::move(body)) {}

void WhileExpr::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "While\n";
    condition->print(out, indent + 1);
    body.print(out, indent + 1);
}

BreakExpr::BreakExpr(ExprPtr value) : value(std::move(value)) {}

void BreakExpr::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "Break\n";
    if (value) {
        value->print(out, indent + 1);
    }
}

ReturnExpr::ReturnExpr(ExprPtr value) : value(std::move(value)) {}

void ReturnExpr::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "Return\n";
    if (value) {
        value->print(out, indent + 1);
    }
}

void ContinueExpr::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "Continue\n";
}

void EmptyStmt::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "EmptyStmt\n";
}

ExprStmt::ExprStmt(ExprPtr expression) : expression(std::move(expression)) {}

void ExprStmt::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "ExprStmt\n";
    expression->print(out, indent + 1);
}

StructExprField::StructExprField(std::string name, ExprPtr value)
    : name(std::move(name)), value(std::move(value)) {}

void StructExprField::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "StructExprField " << name << '\n';
    value->print(out, indent + 1);
}

StructExpr::StructExpr(PathExpr path, std::vector<StructExprField> fields)
    : path(std::move(path)), fields(std::move(fields)) {}

void StructExpr::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "StructExpr\n";
    path.print(out, indent + 1);
    detail::printIndent(out, indent + 1);
    out << "Fields {\n";
    for (const auto &field : fields) {
        field.print(out, indent + 2);
    }
    detail::printIndent(out, indent + 1);
    out << "}\n";
}

StructField::StructField(std::string name, TypePtr type)
    : name(std::move(name)), type(std::move(type)) {}

void StructField::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << name << '\n';
    type->print(out, indent + 1);
}

void SelfParam::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "SelfParam ";
    if (isBorrowed) {
        out << '&';
    }
    if (lifetime) {
        out << *lifetime << ' ';
    }
    if (isMutable) {
        out << "mut ";
    }
    out << "self\n";
}

void WhereClauseItem::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "WhereClauseItem\n";
    if (lifetime) {
        detail::printIndent(out, indent + 1);
        out << "Lifetime " << *lifetime << '\n';
    } else if (type) {
        detail::printIndent(out, indent + 1);
        out << "Type\n";
        type->print(out, indent + 2);
    }
    if (!bounds.empty()) {
        detail::printIndent(out, indent + 1);
        out << "Bounds";
        for (const auto &bound : bounds) {
            out << ' ' << bound;
        }
        out << '\n';
    }
}

void WhereClause::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "WhereClause\n";
    for (const auto &item : items) {
        item.print(out, indent + 1);
    }
}

void LifetimeParam::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "LifetimeParam " << lifetime << '\n';
    if (lifetimeBounds) {
        detail::printIndent(out, indent + 1);
        out << "Bounds";
        for (const auto &bound : *lifetimeBounds) {
            out << ' ' << bound;
        }
        out << '\n';
    }
}

void GenericParam::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "GenericParams\n";
    for (const auto &parameter : params) {
        parameter.print(out, indent + 1);
    }
}

void StructItem::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "Struct " << name;
    out << '\n';
    for (const auto &attribute : attributes) {
        attribute.print(out, indent + 1);
    }
    if (genericParams) {
        genericParams->print(out, indent + 1);
    }
    if (whereClause) {
        whereClause->print(out, indent + 1);
    }
    for (const auto &field : fields) {
        field.print(out, indent + 1);
    }
}

void Crate::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "Crate\n";
    for (const auto &item : items_) {
        item->print(out, indent + 1);
    }
}

void FunctionItem::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "Function " << name << '\n';
    if (returnType) {
        detail::printIndent(out, indent + 1);
        out << "ReturnType\n";
        returnType->print(out, indent + 2);
    }

    if (genericParams) {
        genericParams->print(out, indent + 1);
    }
    if (selfParam) {
        selfParam->print(out, indent + 1);
    }
    for (const auto &parameter : parameters) {
        detail::printIndent(out, indent + 1);
        out << "Parameter " << parameter.name << '\n';
        parameter.type->print(out, indent + 2);
    }
    if (whereClause) {
        whereClause->print(out, indent + 1);
    }

    body.print(out, indent + 1);
}

DeriveAttribute::DeriveAttribute(std::vector<DeriveTrait> traits)
    : traits(std::move(traits)) {}

void DeriveAttribute::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "DeriveAttribute\n";
    for (const auto &trait : traits) {
        detail::printIndent(out, indent + 1);
        out << "Trait " << deriveTraitText(trait) << '\n';
    }
}

void ConstValue::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    switch (kind) {
    case Kind::Integer:
        out << "Integer " << value << '\n';
        break;
    case Kind::Boolean:
        out << "Boolean " << (boolValue ? "true" : "false") << '\n';
        break;
    case Kind::Path:
        out << "Path " << value << '\n';
        break;
    case Kind::Negate:
        out << "Negate\n";
        operand->print(out, indent + 1);
        break;
    }
}

void ConstantItem::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "Constant " << name << '\n';
    type->print(out, indent + 1);
    value.print(out, indent + 1);
}

void ImplItem::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "Impl\n";
    typeName->print(out, indent + 1);
    if (genericParams) {
        genericParams->print(out, indent + 1);
    }
    if (whereClause) {
        whereClause->print(out, indent + 1);
    }
    for (const auto &item : associatedItems) {
        item->print(out, indent + 1);
    }
}

void UsePath::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "UsePath\n";
    if (leadingPathSep) {
        detail::printIndent(out, indent + 1);
        out << "LeadingPathSep\n";
    }
    for (const auto &segment : segments) {
        segment.print(out, indent + 1);
    }
}

void UseTree::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    switch (kind) {
    case Kind::Glob:
        out << "UseTree Glob\n";
        break;
    case Kind::List:
        out << "UseTree List\n";
        break;
    case Kind::Path:
        out << "UseTree Path\n";
        break;
    }
    if (prefix) {
        detail::printIndent(out, indent + 1);
        out << "Prefix\n";
        prefix->print(out, indent + 2);
        if (trailingPathSep) {
            detail::printIndent(out, indent + 2);
            out << "TrailingPathSep\n";
        }
    }
    if (kind == Kind::List) {
        detail::printIndent(out, indent + 1);
        out << "Items {\n";
        for (const auto &item : items) {
            item.print(out, indent + 2);
        }
        detail::printIndent(out, indent + 1);
        out << "}\n";
    }
    if (kind == Kind::Path) {
        detail::printIndent(out, indent + 1);
        out << "Path\n";
        path.print(out, indent + 2);
        if (alias || aliasIsUnderscore) {
            detail::printIndent(out, indent + 1);
            out << "Alias " << (aliasIsUnderscore ? "_" : *alias) << '\n';
        }
    }
}

void UseItem::print(std::ostream &out, std::size_t indent) const {
    detail::printIndent(out, indent);
    out << "Use\n";
    tree.print(out, indent + 1);
}

void Program::print(std::ostream &out) const {
    out << "Program\n";
    for (const auto &item : items) {
        item->print(out, 1);
    }
}

} // namespace rx::ast