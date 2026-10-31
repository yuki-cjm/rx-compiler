#include "operators.hpp"

#include "../ty/ty_compare.hpp"

namespace rx::semantic {

namespace {

// 取标量操作数：标量本身，或一层 shared reference 指向标量。
std::optional<TyPtr> stripSharedScalar(const TyPtr &operand) {
    if (isInteger(operand) || isBool(operand)) {
        return operand;
    }
    if (isReference(operand) && operand->mutability == Mutability::Shared &&
        (isInteger(operand->inner) || isBool(operand->inner))) {
        return operand->inner;
    }
    return std::nullopt;
}

struct OrderingShape {
    TyPtr scalar;
    std::vector<Mutability> references;
    bool valid = false;
};

OrderingShape stripForOrdering(const TyPtr &operand) {
    OrderingShape shape;
    TyPtr current = operand;
    while (isReference(current)) {
        shape.references.push_back(current->mutability);
        current = current->inner;
    }
    if (isInteger(current) || isBool(current)) {
        shape.scalar = current;
        shape.valid = true;
    }
    return shape;
}

bool orderingCompatible(const TyPtr &left, const TyPtr &right) {
    OrderingShape lhs = stripForOrdering(left);
    OrderingShape rhs = stripForOrdering(right);
    if (!lhs.valid || !rhs.valid) {
        return false;
    }
    if (!sameTy(lhs.scalar, rhs.scalar)) {
        return false;
    }
    if (lhs.references.size() != rhs.references.size()) {
        return false;
    }
    for (std::size_t index = 0; index < lhs.references.size(); ++index) {
        if (lhs.references[index] == rhs.references[index]) {
            continue;
        }
        // 唯一的非对称组合：左侧 &T、右侧 &mut T（单层、同一 T）。
        bool asymmetric =
            lhs.references.size() == 1 && lhs.references[0] == Mutability::Shared &&
            rhs.references[0] == Mutability::Mutable;
        if (!asymmetric) {
            return false;
        }
    }
    return true;
}

} // namespace

std::optional<TyPtr> Operators::unaryResult(ast::UnaryOp op,
                                            const TyPtr &operand) {
    switch (op) {
    case ast::UnaryOp::Negate: {
        auto scalar = stripSharedScalar(operand);
        if (!scalar || !isSignedInteger(*scalar)) {
            return std::nullopt;
        }
        return scalar;
    }
    case ast::UnaryOp::LogicalNot: {
        auto scalar = stripSharedScalar(operand);
        if (!scalar) {
            return std::nullopt;
        }
        return scalar;
    }
    case ast::UnaryOp::Dereference: {
        if (isReference(operand) || isBox(operand)) {
            return operand->inner;
        }
        return std::nullopt;
    }
    case ast::UnaryOp::Borrow:
        return Ty::makeReference(operand, Mutability::Shared);
    case ast::UnaryOp::BorrowMut:
        return Ty::makeReference(operand, Mutability::Mutable);
    case ast::UnaryOp::DoubleBorrow:
        return Ty::makeReference(
            Ty::makeReference(operand, Mutability::Shared),
            Mutability::Shared);
    case ast::UnaryOp::DoubleBorrowMut:
        return Ty::makeReference(
            Ty::makeReference(operand, Mutability::Mutable),
            Mutability::Shared);
    }
    return std::nullopt;
}

std::optional<TyPtr> Operators::binaryResult(ast::BinaryOp op,
                                             const TyPtr &left,
                                             const TyPtr &right) {
    if (isAssignment(op) || isCompoundAssignment(op)) {
        return Ty::makeUnit();
    }

    switch (op) {
    case ast::BinaryOp::Add:
    case ast::BinaryOp::Subtract:
    case ast::BinaryOp::Multiply:
    case ast::BinaryOp::Divide:
    case ast::BinaryOp::Remainder: {
        auto lhs = stripSharedScalar(left);
        auto rhs = stripSharedScalar(right);
        if (!lhs || !rhs || !isInteger(*lhs) || !isInteger(*rhs) ||
            !sameTy(*lhs, *rhs)) {
            return std::nullopt;
        }
        return lhs;
    }
    case ast::BinaryOp::BitwiseAnd:
    case ast::BinaryOp::BitwiseXor:
    case ast::BinaryOp::BitwiseOr: {
        auto lhs = stripSharedScalar(left);
        auto rhs = stripSharedScalar(right);
        if (!lhs || !rhs || !sameTy(*lhs, *rhs)) {
            return std::nullopt;
        }
        return lhs;
    }
    case ast::BinaryOp::ShiftLeft:
    case ast::BinaryOp::ShiftRight: {
        auto lhs = stripSharedScalar(left);
        auto rhs = stripSharedScalar(right);
        if (!lhs || !rhs || !isInteger(*lhs) || !isInteger(*rhs)) {
            return std::nullopt;
        }
        return lhs;
    }
    case ast::BinaryOp::Less:
    case ast::BinaryOp::LessEqual:
    case ast::BinaryOp::Greater:
    case ast::BinaryOp::GreaterEqual: {
        if (!orderingCompatible(left, right)) {
            return std::nullopt;
        }
        return Ty::makeScalar(TyKind::Bool);
    }
    case ast::BinaryOp::Equal:
    case ast::BinaryOp::NotEqual: {
        if (!sameTy(left, right)) {
            return std::nullopt;
        }
        return Ty::makeScalar(TyKind::Bool);
    }
    case ast::BinaryOp::LogicalAnd:
    case ast::BinaryOp::LogicalOr: {
        if (!isBool(left) || !isBool(right)) {
            return std::nullopt;
        }
        return Ty::makeScalar(TyKind::Bool);
    }
    case ast::BinaryOp::Assign:
    case ast::BinaryOp::AddAssign:
    case ast::BinaryOp::SubtractAssign:
    case ast::BinaryOp::MultiplyAssign:
    case ast::BinaryOp::DivideAssign:
    case ast::BinaryOp::RemainderAssign:
    case ast::BinaryOp::BitwiseAndAssign:
    case ast::BinaryOp::BitwiseXorAssign:
    case ast::BinaryOp::BitwiseOrAssign:
    case ast::BinaryOp::ShiftLeftAssign:
    case ast::BinaryOp::ShiftRightAssign:
        return Ty::makeUnit();
    }
    return std::nullopt;
}

bool Operators::isAssignment(ast::BinaryOp op) {
    return op == ast::BinaryOp::Assign;
}

bool Operators::isCompoundAssignment(ast::BinaryOp op) {
    switch (op) {
    case ast::BinaryOp::AddAssign:
    case ast::BinaryOp::SubtractAssign:
    case ast::BinaryOp::MultiplyAssign:
    case ast::BinaryOp::DivideAssign:
    case ast::BinaryOp::RemainderAssign:
    case ast::BinaryOp::BitwiseAndAssign:
    case ast::BinaryOp::BitwiseXorAssign:
    case ast::BinaryOp::BitwiseOrAssign:
    case ast::BinaryOp::ShiftLeftAssign:
    case ast::BinaryOp::ShiftRightAssign:
        return true;
    default:
        return false;
    }
}

bool Operators::checkCompoundAssignment(ast::BinaryOp op, const TyPtr &left,
                                        const TyPtr &right) {
    auto rhs = stripSharedScalar(right);
    if (!rhs) {
        return false;
    }
    switch (op) {
    case ast::BinaryOp::AddAssign:
    case ast::BinaryOp::SubtractAssign:
    case ast::BinaryOp::MultiplyAssign:
    case ast::BinaryOp::DivideAssign:
    case ast::BinaryOp::RemainderAssign:
        return isInteger(left) && sameTy(left, *rhs);
    case ast::BinaryOp::BitwiseAndAssign:
    case ast::BinaryOp::BitwiseXorAssign:
    case ast::BinaryOp::BitwiseOrAssign:
        return (isInteger(left) || isBool(left)) && sameTy(left, *rhs);
    case ast::BinaryOp::ShiftLeftAssign:
    case ast::BinaryOp::ShiftRightAssign:
        return isInteger(left) && isInteger(*rhs);
    default:
        return false;
    }
}

} // namespace rx::semantic
