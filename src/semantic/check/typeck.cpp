#include "typeck.hpp"

#include "../builtin/builtin_traits.hpp"
#include "../consteval/const_eval.hpp"
#include "../resolve/path_resolve.hpp"
#include "../resolve/resolver.hpp"
#include "../type/type_compare.hpp"
#include "assign.hpp"
#include "calls.hpp"
#include "coercion.hpp"
#include "integer_infer.hpp"
#include "operators.hpp"
#include "place.hpp"

#include <any>
#include <set>

namespace rx::semantic {

TypeChecker::TypeChecker(SymbolTable &table, Diagnostic &diag)
    : table_(table), diag_(diag) {}

bool TypeChecker::check(const ast::Program &program) {
    for (const ast::ItemPtr &item : program.items) {
        if (auto *function = dynamic_cast<ast::FunctionItem *>(item.get())) {
            checkFunctionBody(*function, nullptr);
        } else if (auto *implementation =
                       dynamic_cast<ast::ImplItem *>(item.get())) {
            TypeResolver resolver(table_, diag_);
            TypePtr target =
                resolver.resolve(*implementation->typeName, nullptr);
            if (target != nullptr && isStruct(target)) {
                for (const ast::ItemPtr &associated :
                     implementation->associatedItems) {
                    if (auto *function = dynamic_cast<ast::FunctionItem *>(
                            associated.get())) {
                        checkFunctionBody(*function, target);
                    }
                }
            }
        }
    }
    return !diag_.hasErrors();
}

void TypeChecker::checkFunctionBody(ast::FunctionItem &function,
                                    const TypePtr &selfType) {
    TypeResolver resolver(table_, diag_);
    TypePtr returnType = function.returnType != nullptr
                             ? resolver.resolve(*function.returnType, selfType)
                             : Type::makeUnit();

    auto savedReturn = currentReturn_;
    auto savedSelf = currentSelf_;
    currentReturn_ = returnType;
    currentSelf_ = selfType;

    table_.scope().push();

    if (function.selfParam.has_value()) {
        const ast::SelfParam &self = *function.selfParam;
        Symbol symbol;
        symbol.kind = SymbolKind::Local;
        symbol.name = "self";
        if (!self.isBorrowed) {
            symbol.type = selfType;
            symbol.isMutable = self.isMutable;
        } else {
            Mutability mutability =
                self.isMutable ? Mutability::Mutable : Mutability::Shared;
            symbol.type = Type::makeReference(selfType, mutability);
            symbol.isMutable = false;
        }
        table_.scope().declare("self", symbol);
    }

    for (const ast::FunctionParam &parameter : function.parameters) {
        TypePtr type = resolver.resolve(*parameter.type, selfType);
        Symbol symbol;
        symbol.kind = SymbolKind::Local;
        symbol.name = parameter.binding.name;
        symbol.type = type;
        symbol.isMutable = parameter.binding.isMutable;
        table_.scope().declare(parameter.binding.name, symbol);
    }

    checkExpr(function.body, returnType);

    table_.scope().pop();
    currentReturn_ = savedReturn;
    currentSelf_ = savedSelf;
}

TypePtr TypeChecker::resolveConstantType(const Symbol &symbol) {
    if (symbol.constant == nullptr) {
        return nullptr;
    }
    TypeResolver resolver(table_, diag_);
    TypePtr selfType =
        symbol.implTarget != 0 ? Type::makeStruct(symbol.implTarget) : nullptr;
    return resolver.resolve(*symbol.constant->type, selfType);
}

TypePtr TypeChecker::checkExpr(ast::Expr &expr,
                               const std::optional<TypePtr> &expected) {
    auto saved = expected_;
    expected_ = expected;
    TypePtr result = std::any_cast<TypePtr>(expr.accept(*this));
    expected_ = saved;

    // 共享访问的 place 若持有 &mut T，读出的值降级为 &T。
    if (result != nullptr && isReference(result) &&
        result->mutability == Mutability::Mutable) {
        auto place = classifyPlace(expr);
        if (place && place->access == PlaceAccess::Shared) {
            result = Type::makeReference(result->inner, Mutability::Shared);
        }
    }

    if (result != nullptr && expected && !sameType(result, *expected)) {
        if (Coercion::canCoerce(table_, result, *expected)) {
            result = *expected;
        } else {
            diag_.error(expr.span(),
                        "expression does not coerce to the expected type");
        }
    }
    recordType(expr, result);
    return result;
}

void TypeChecker::recordType(const ast::Expr &expr, const TypePtr &type) {
    exprTypes_[&expr] = type;
}

TypePtr TypeChecker::typeOf(const ast::Expr &expr) const {
    auto found = exprTypes_.find(&expr);
    return found == exprTypes_.end() ? nullptr : found->second;
}

std::optional<TypeChecker::PlaceResult>
TypeChecker::classifyPlace(ast::Expr &expr) {
    if (auto *path = dynamic_cast<ast::PathInExpr *>(&expr)) {
        PathResolver resolver(table_, diag_);
        const Symbol *symbol = resolver.resolveValuePath(*path, currentSelf_);
        if (symbol == nullptr || symbol->kind != SymbolKind::Local) {
            return std::nullopt;
        }
        PlaceResult result;
        result.type = symbol->type;
        result.access =
            symbol->isMutable ? PlaceAccess::Mutable : PlaceAccess::Owned;
        return result;
    }

    if (auto *unary = dynamic_cast<ast::UnaryExpr *>(&expr)) {
        if (unary->op != ast::UnaryOp::Dereference) {
            return std::nullopt;
        }
        TypePtr type = typeOf(*unary->operand);
        if (type == nullptr || (!isReference(type) && !isBox(type))) {
            return std::nullopt;
        }
        PlaceResult result;
        result.type = type->inner;
        if (isReference(type)) {
            if (type->mutability == Mutability::Shared) {
                result.access = PlaceAccess::Shared;
            } else {
                auto base = classifyPlace(*unary->operand);
                result.access = (base && base->access == PlaceAccess::Shared)
                                    ? PlaceAccess::Shared
                                    : PlaceAccess::Mutable;
            }
        } else {
            auto base = classifyPlace(*unary->operand);
            result.access = base ? base->access : PlaceAccess::Mutable;
        }
        return result;
    }

    if (auto *field = dynamic_cast<ast::FieldAccessExpr *>(&expr)) {
        TypePtr current = typeOf(*field->operand);
        PlaceAccess access = baseAccess(*field->operand);
        while (current != nullptr && !isStruct(current)) {
            if (isReference(current)) {
                if (current->mutability == Mutability::Shared) {
                    access = PlaceAccess::Shared;
                } else {
                    access = access == PlaceAccess::Shared
                                 ? PlaceAccess::Shared
                                 : PlaceAccess::Mutable;
                }
                current = current->inner;
            } else if (isBox(current)) {
                current = current->inner;
            } else {
                break;
            }
        }
        if (current == nullptr || !isStruct(current)) {
            return std::nullopt;
        }
        const FieldInfo *info =
            table_.lookupField(current->structId, field->field.ident.name);
        if (info == nullptr) {
            return std::nullopt;
        }
        PlaceResult result;
        result.type = info->type;
        result.access = access;
        return result;
    }

    if (auto *index = dynamic_cast<ast::IndexExpr *>(&expr)) {
        TypePtr current = typeOf(*index->operand);
        PlaceAccess access = baseAccess(*index->operand);
        while (current != nullptr && !isArray(current) && !isVec(current)) {
            if (isReference(current)) {
                if (current->mutability == Mutability::Shared) {
                    access = PlaceAccess::Shared;
                } else {
                    access = access == PlaceAccess::Shared
                                 ? PlaceAccess::Shared
                                 : PlaceAccess::Mutable;
                }
                current = current->inner;
            } else if (isBox(current)) {
                current = current->inner;
            } else {
                break;
            }
        }
        if (current == nullptr) {
            return std::nullopt;
        }
        PlaceResult result;
        if (isArray(current)) {
            result.type = current->inner;
            result.access = access;
            return result;
        }
        if (isVec(current)) {
            result.type = current->inner;
            result.access = access == PlaceAccess::Mutable
                                ? PlaceAccess::Mutable
                                : PlaceAccess::Shared;
            return result;
        }
        return std::nullopt;
    }

    return std::nullopt;
}

TypeChecker::PlaceAccess TypeChecker::baseAccess(ast::Expr &expr) {
    auto place = classifyPlace(expr);
    return place ? place->access : PlaceAccess::Mutable;
}

bool TypeChecker::stmtDiverges(ast::Stmt &stmt) {
    if (auto *exprStmt = dynamic_cast<ast::ExprStmt *>(&stmt)) {
        return exprDiverges(*exprStmt->expression);
    }
    return false;
}

bool TypeChecker::exprDiverges(ast::Expr &expr) {
    if (dynamic_cast<ast::ReturnExpr *>(&expr) != nullptr ||
        dynamic_cast<ast::BreakExpr *>(&expr) != nullptr ||
        dynamic_cast<ast::ContinueExpr *>(&expr) != nullptr) {
        return true;
    }
    if (auto *block = dynamic_cast<ast::BlockExpr *>(&expr)) {
        for (auto &statement : block->statements) {
            if (stmtDiverges(*statement)) {
                return true;
            }
        }
        if (block->tail != nullptr) {
            return exprDiverges(*block->tail);
        }
        return false;
    }
    if (auto *loop = dynamic_cast<ast::LoopExpr *>(&expr)) {
        return isNever(typeOf(expr));
    }
    if (auto *ifExpr = dynamic_cast<ast::IfExpr *>(&expr)) {
        if (ifExpr->elseBranch == nullptr) {
            return false;
        }
        return exprDiverges(ifExpr->thenBranch) &&
               exprDiverges(*ifExpr->elseBranch);
    }
    return false;
}

// ---- Items ----

std::any TypeChecker::visitCrate(ast::Crate *node) {
    for (ast::ItemPtr &item : node->items()) {
        item->accept(*this);
    }
    return {};
}

std::any TypeChecker::visitFunctionItem(ast::FunctionItem *node) {
    (void)node;
    return {};
}

std::any TypeChecker::visitStructItem(ast::StructItem *node) {
    (void)node;
    return {};
}

std::any TypeChecker::visitConstantItem(ast::ConstantItem *node) {
    (void)node;
    return {};
}

std::any TypeChecker::visitImplItem(ast::ImplItem *node) {
    (void)node;
    return {};
}

std::any TypeChecker::visitUseItem(ast::UseItem *node) {
    (void)node;
    return {};
}

// ---- Statements ----

std::any TypeChecker::visitEmptyStmt(ast::EmptyStmt *node) {
    (void)node;
    return std::any(Type::makeUnit());
}

std::any TypeChecker::visitLetStmt(ast::LetStmt *node) {
    TypePtr expected = nullptr;
    if (node->type != nullptr) {
        TypeResolver resolver(table_, diag_);
        expected = resolver.resolve(*node->type, currentSelf_);
        if (expected == nullptr) {
            return std::any(Type::makeUnit());
        }
    }
    TypePtr initializer =
        checkExpr(*node->initializer,
                  expected ? std::optional<TypePtr>(expected) : std::nullopt);
    TypePtr bindingType = expected != nullptr ? expected : initializer;

    Symbol symbol;
    symbol.kind = SymbolKind::Local;
    symbol.name = node->binding.name;
    symbol.type = bindingType;
    symbol.isMutable = node->binding.isMutable;
    table_.scope().declare(node->binding.name, symbol);
    return std::any(Type::makeUnit());
}

std::any TypeChecker::visitExprStmt(ast::ExprStmt *node) {
    bool withBlock =
        dynamic_cast<ast::BlockExpr *>(node->expression.get()) != nullptr ||
        dynamic_cast<ast::IfExpr *>(node->expression.get()) != nullptr ||
        dynamic_cast<ast::LoopExpr *>(node->expression.get()) != nullptr ||
        dynamic_cast<ast::WhileExpr *>(node->expression.get()) != nullptr;
    TypePtr type;
    if (withBlock && !node->hasSemicolon) {
        type = checkExpr(*node->expression, Type::makeUnit());
    } else {
        type = checkExpr(*node->expression, std::nullopt);
    }
    return std::any(type);
}

// ---- Expressions ----

std::any TypeChecker::visitIntegerLiteralExpr(ast::IntegerLiteralExpr *node) {
    return std::any(IntegerInfer::infer(node->value, expected_));
}

std::any TypeChecker::visitBooleanLiteralExpr(ast::BooleanLiteralExpr *node) {
    (void)node;
    return std::any(Type::makeScalar(TypeKind::Bool));
}

std::any TypeChecker::visitUnitExpr(ast::UnitExpr *node) {
    (void)node;
    return std::any(Type::makeUnit());
}

std::any TypeChecker::visitPathInExpr(ast::PathInExpr *node) {
    PathResolver resolver(table_, diag_);
    const Symbol *symbol = resolver.resolveValuePath(*node, currentSelf_);
    if (symbol == nullptr) {
        return std::any(TypePtr(nullptr));
    }
    if (symbol->kind == SymbolKind::Local) {
        return std::any(symbol->type);
    }
    if (symbol->kind == SymbolKind::Constant) {
        return std::any(resolveConstantType(*symbol));
    }
    // 函数值（function item as value）是 undefined behavior，返回空类型。
    return std::any(TypePtr(nullptr));
}

std::any TypeChecker::visitCallExpr(ast::CallExpr *node) {
    if (auto *fieldAccess =
            dynamic_cast<ast::FieldAccessExpr *>(node->callee.get())) {
        if (fieldAccess->field.genericArgs.has_value()) {
            diag_.error(node->span(),
                        "method segments have no type parameters");
        }
        TypePtr receiverType = checkExpr(*fieldAccess->operand, std::nullopt);
        std::string methodName = fieldAccess->field.ident.name;
        auto signature = resolveMethod(table_, diag_, receiverType, methodName);
        if (!signature) {
            return std::any(TypePtr(nullptr));
        }
        if (signature->receiver == ReceiverKind::Mutable) {
            PlaceAccess access = baseAccess(*fieldAccess->operand);
            TypePtr current = receiverType;
            while (current != nullptr && signature->selfType != nullptr &&
                   !sameType(current, signature->selfType)) {
                if (isReference(current)) {
                    if (current->mutability == Mutability::Shared) {
                        access = PlaceAccess::Shared;
                    } else {
                        access = access == PlaceAccess::Shared
                                     ? PlaceAccess::Shared
                                     : PlaceAccess::Mutable;
                    }
                    current = current->inner;
                } else if (isBox(current)) {
                    current = current->inner;
                } else {
                    break;
                }
            }
            if (access != PlaceAccess::Mutable) {
                diag_.error(node->span(), "method requires a mutable receiver");
            }
        }
        if (node->arguments.size() != signature->params.size()) {
            diag_.error(node->span(), "argument count mismatch");
        } else {
            for (std::size_t index = 0; index < node->arguments.size();
                 ++index) {
                checkExpr(*node->arguments[index], signature->params[index]);
            }
        }
        return std::any(signature->result);
    }

    // 普通 / 关联函数调用。
    auto signature = resolveCallable(table_, diag_, *node, currentSelf_);
    if (!signature) {
        return std::any(TypePtr(nullptr));
    }
    if (node->arguments.size() != signature->params.size()) {
        diag_.error(node->span(), "argument count mismatch");
    } else {
        for (std::size_t index = 0; index < node->arguments.size(); ++index) {
            checkExpr(*node->arguments[index], signature->params[index]);
        }
    }
    return std::any(signature->result);
}

std::any TypeChecker::visitIndexExpr(ast::IndexExpr *node) {
    TypePtr baseType = checkExpr(*node->operand, std::nullopt);
    TypePtr indexType =
        checkExpr(*node->index, Type::makeScalar(TypeKind::Usize));

    TypePtr current = baseType;
    while (current != nullptr && !isArray(current) && !isVec(current)) {
        if (isReference(current) || isBox(current)) {
            current = current->inner;
        } else {
            break;
        }
    }
    if (current == nullptr || (!isArray(current) && !isVec(current))) {
        diag_.error(node->span(), "value is not indexable");
        return std::any(TypePtr(nullptr));
    }
    return std::any(current->inner);
}

std::any TypeChecker::visitFieldAccessExpr(ast::FieldAccessExpr *node) {
    TypePtr baseType = checkExpr(*node->operand, std::nullopt);
    StructId structId = 0;
    bool crossedShared = false;
    if (!PlaceRules::stripToStruct(baseType, &structId, &crossedShared)) {
        diag_.error(node->span(), "field access requires a struct");
        return std::any(TypePtr(nullptr));
    }
    const FieldInfo *field =
        table_.lookupField(structId, node->field.ident.name);
    if (field == nullptr) {
        diag_.error(node->span(),
                    "unknown field '" + node->field.ident.name + "'");
        return std::any(TypePtr(nullptr));
    }
    return std::any(field->type);
}

std::any TypeChecker::visitUnaryExpr(ast::UnaryExpr *node) {
    TypePtr operand = checkExpr(*node->operand, std::nullopt);

    if (node->op == ast::UnaryOp::BorrowMut ||
        node->op == ast::UnaryOp::DoubleBorrowMut) {
        auto place = classifyPlace(*node->operand);
        if (place && place->access != PlaceAccess::Mutable) {
            diag_.error(node->span(),
                        "cannot mutably borrow an immutable place");
        }
    }

    auto result = Operators::unaryResult(node->op, operand);
    if (!result) {
        diag_.error(node->span(), "invalid operand for unary operator");
        return std::any(TypePtr(nullptr));
    }
    return std::any(*result);
}

std::any TypeChecker::visitCastExpr(ast::CastExpr *node) {
    TypePtr operand = checkExpr(*node->operand, std::nullopt);
    TypeResolver resolver(table_, diag_);
    TypePtr target = resolver.resolve(*node->targetType, currentSelf_);
    if (target == nullptr) {
        return std::any(TypePtr(nullptr));
    }
    bool valid = (isInteger(operand) && isInteger(target)) ||
                 (isBool(operand) && isInteger(target));
    if (!valid) {
        diag_.error(node->span(), "unsupported cast");
    }
    return std::any(target);
}

std::any TypeChecker::visitBinaryExpr(ast::BinaryExpr *node) {
    if (Operators::isAssignment(node->op)) {
        TypePtr leftType = checkExpr(*node->left, std::nullopt);
        auto place = classifyPlace(*node->left);
        if (!place) {
            diag_.error(node->span(), "assignment target is not a place");
            return std::any(Type::makeUnit());
        }
        PlaceInfo target{place->type, place->access == PlaceAccess::Mutable};
        TypePtr rightType =
            checkExpr(*node->right, std::optional<TypePtr>(place->type));
        AssignChecker::check(table_, target, rightType, diag_);
        return std::any(Type::makeUnit());
    }

    if (Operators::isCompoundAssignment(node->op)) {
        TypePtr leftType = checkExpr(*node->left, std::nullopt);
        auto place = classifyPlace(*node->left);
        if (!place) {
            diag_.error(node->span(),
                        "compound assignment target is not a place");
            return std::any(Type::makeUnit());
        }
        PlaceInfo target{place->type, place->access == PlaceAccess::Mutable};
        TypePtr rightType = checkExpr(*node->right, std::nullopt);
        AssignChecker::checkCompound(table_, target, node->op, rightType,
                                     diag_);
        return std::any(Type::makeUnit());
    }

    TypePtr leftType = checkExpr(*node->left, std::nullopt);
    TypePtr rightType = checkExpr(*node->right, std::nullopt);
    auto result = Operators::binaryResult(node->op, leftType, rightType);
    if (!result) {
        diag_.error(node->span(), "invalid operands for binary operator");
        return std::any(Type::makeNever());
    }
    if (node->op == ast::BinaryOp::Equal ||
        node->op == ast::BinaryOp::NotEqual) {
        if (!supportsPartialEq(table_, leftType)) {
            diag_.error(node->span(), "equality requires PartialEq");
        }
    }
    return std::any(*result);
}

std::any TypeChecker::visitArrayExpr(ast::ArrayExpr *node) {
    TypePtr expectedElement = nullptr;
    std::uint64_t expectedLength = 0;
    if (expected_ && isArray(*expected_)) {
        expectedElement = (*expected_)->inner;
        expectedLength = (*expected_)->length;
    }

    if (node->repeatCount.has_value()) {
        TypePtr element =
            checkExpr(*node->elements[0],
                      expectedElement ? std::optional<TypePtr>(expectedElement)
                                      : std::nullopt);
        auto length = evaluateArrayLength(table_, diag_, *node->repeatCount,
                                          currentSelf_);
        if (!length) {
            return std::any(TypePtr(nullptr));
        }
        if (expected_ && isArray(*expected_) && *length != expectedLength) {
            diag_.error(node->span(), "array length mismatch");
        }
        if (*length > 1 && !isCopy(table_, element)) {
            diag_.error(node->span(),
                        "repeating an element more than once requires Copy");
        }
        return std::any(Type::makeArray(element, *length));
    }

    std::vector<TypePtr> elementTypes;
    for (ast::ExprPtr &element : node->elements) {
        elementTypes.push_back(checkExpr(
            *element, expectedElement ? std::optional<TypePtr>(expectedElement)
                                      : std::nullopt));
    }
    if (expected_ && isArray(*expected_) &&
        node->elements.size() != expectedLength) {
        diag_.error(node->span(), "array length mismatch");
    }
    TypePtr elementType;
    if (expectedElement != nullptr) {
        elementType = expectedElement;
    } else {
        auto lub = Coercion::leastUpperBound(table_, elementTypes);
        elementType = lub ? *lub : Type::makeNever();
    }
    return std::any(Type::makeArray(elementType, node->elements.size()));
}

std::any TypeChecker::visitBlockExpr(ast::BlockExpr *node) {
    table_.scope().push();

    bool blockDiverges = false;
    for (ast::StmtPtr &statement : node->statements) {
        statement->accept(*this);
        if (stmtDiverges(*statement)) {
            blockDiverges = true;
        }
    }

    TypePtr result;
    if (node->tail != nullptr) {
        result = checkExpr(*node->tail, expected_);
    } else {
        result = blockDiverges ? Type::makeNever() : Type::makeUnit();
    }

    table_.scope().pop();
    return std::any(result);
}

std::any TypeChecker::visitIfExpr(ast::IfExpr *node) {
    TypePtr condition =
        checkExpr(*node->condition, Type::makeScalar(TypeKind::Bool));
    if (!isBool(condition)) {
        diag_.error(node->span(), "if condition must be bool");
    }

    if (node->elseBranch != nullptr) {
        TypePtr thenType = checkExpr(
            node->thenBranch,
            expected_ ? std::optional<TypePtr>(*expected_) : std::nullopt);
        TypePtr elseType = checkExpr(
            *node->elseBranch,
            expected_ ? std::optional<TypePtr>(*expected_) : std::nullopt);
        if (expected_) {
            return std::any(*expected_);
        }
        auto lub = Coercion::leastUpperBound(table_, {thenType, elseType});
        if (!lub) {
            diag_.error(node->span(), "incompatible if branches");
            return std::any(Type::makeNever());
        }
        return std::any(*lub);
    }

    TypePtr thenType = checkExpr(node->thenBranch, Type::makeUnit());
    (void)thenType;
    return std::any(Type::makeUnit());
}

std::any TypeChecker::visitLoopExpr(ast::LoopExpr *node) {
    loopDepth_++;
    loopIsWhile_.push_back(false);
    loopBreakValues_.push_back({});
    auto savedBreak = currentLoopBreak_;
    currentLoopBreak_ = expected_;

    checkExpr(node->body, Type::makeUnit());

    auto breaks = std::move(loopBreakValues_.back());
    loopBreakValues_.pop_back();
    loopIsWhile_.pop_back();
    currentLoopBreak_ = savedBreak;
    loopDepth_--;

    if (expected_) {
        return std::any(*expected_);
    }
    if (breaks.empty()) {
        return std::any(Type::makeNever());
    }
    auto lub = Coercion::leastUpperBound(table_, breaks);
    if (!lub) {
        diag_.error(node->span(), "incompatible break values");
        return std::any(Type::makeNever());
    }
    return std::any(*lub);
}

std::any TypeChecker::visitWhileExpr(ast::WhileExpr *node) {
    conditionBarriers_.push_back(loopDepth_);
    TypePtr condition =
        checkExpr(*node->condition, Type::makeScalar(TypeKind::Bool));
    conditionBarriers_.pop_back();
    if (!isBool(condition)) {
        diag_.error(node->span(), "while condition must be bool");
    }

    loopDepth_++;
    loopIsWhile_.push_back(true);
    loopBreakValues_.push_back({});
    checkExpr(node->body, Type::makeUnit());
    loopBreakValues_.pop_back();
    loopIsWhile_.pop_back();
    loopDepth_--;

    return std::any(Type::makeUnit());
}

std::any TypeChecker::visitBreakExpr(ast::BreakExpr *node) {
    if (loopDepth_ <= 0) {
        diag_.error(node->span(), "break used outside a loop");
        return std::any(Type::makeNever());
    }
    if (!conditionBarriers_.empty() &&
        loopDepth_ <= conditionBarriers_.back()) {
        diag_.error(node->span(),
                    "break in a while condition cannot target an outer loop");
        return std::any(Type::makeNever());
    }

    if (node->value != nullptr) {
        if (loopIsWhile_.back()) {
            diag_.error(node->span(),
                        "a while loop cannot accept a break value");
        } else {
            TypePtr value = checkExpr(
                *node->value, currentLoopBreak_
                                  ? std::optional<TypePtr>(*currentLoopBreak_)
                                  : std::nullopt);
            loopBreakValues_.back().push_back(value);
        }
    } else if (!loopIsWhile_.back()) {
        loopBreakValues_.back().push_back(Type::makeUnit());
    }
    return std::any(Type::makeNever());
}

std::any TypeChecker::visitReturnExpr(ast::ReturnExpr *node) {
    TypePtr valueType;
    if (node->value != nullptr) {
        valueType =
            checkExpr(*node->value,
                      currentReturn_ ? std::optional<TypePtr>(*currentReturn_)
                                     : std::nullopt);
    } else {
        valueType = Type::makeUnit();
    }
    if (currentReturn_ && !isNever(valueType) &&
        !Coercion::canCoerce(table_, valueType, *currentReturn_)) {
        diag_.error(node->span(),
                    "return value does not match the function return type");
    }
    return std::any(Type::makeNever());
}

std::any TypeChecker::visitContinueExpr(ast::ContinueExpr *node) {
    (void)node;
    if (loopDepth_ <= 0) {
        diag_.error(node->span(), "continue used outside a loop");
    } else if (!conditionBarriers_.empty() &&
               loopDepth_ <= conditionBarriers_.back()) {
        diag_.error(
            node->span(),
            "continue in a while condition cannot target an outer loop");
    }
    return std::any(Type::makeNever());
}

std::any TypeChecker::visitStructExpr(ast::StructExpr *node) {
    const ast::PathInExpr &path = node->path;
    StructId structId = 0;
    if (path.segments.size() != 1) {
        diag_.error(node->span(),
                    "struct expression path must be a single name");
        return std::any(TypePtr(nullptr));
    }
    const ast::PathExprSegment &segment = path.segments[0];
    if (segment.ident.kind == ast::PathSegmentKind::SelfType) {
        if (!isStruct(currentSelf_)) {
            diag_.error(node->span(), "'Self' used outside a struct context");
            return std::any(TypePtr(nullptr));
        }
        structId = currentSelf_->structId;
    } else if (segment.ident.kind == ast::PathSegmentKind::Ident) {
        const Symbol *symbol = table_.lookupType(segment.ident.name);
        if (symbol == nullptr) {
            diag_.error(node->span(),
                        "unresolved type name '" + segment.ident.name + "'");
            return std::any(TypePtr(nullptr));
        }
        if (symbol->kind != SymbolKind::Struct) {
            diag_.error(node->span(),
                        "'" + segment.ident.name + "' is not a struct");
            return std::any(TypePtr(nullptr));
        }
        structId = symbol->structId;
    } else {
        diag_.error(node->span(), "invalid struct expression path");
        return std::any(TypePtr(nullptr));
    }

    const Symbol *structure = table_.lookupStruct(structId);
    std::set<std::string> initialized;
    for (const ast::StructExprField &field : node->fields) {
        if (!initialized.insert(field.name).second) {
            diag_.error(node->span(),
                        "duplicate struct field '" + field.name + "'");
        }
        const FieldInfo *info = table_.lookupField(structId, field.name);
        if (info == nullptr) {
            diag_.error(node->span(),
                        "unknown struct field '" + field.name + "'");
            continue;
        }
        checkExpr(*field.value, info->type);
    }
    for (const FieldInfo &info : structure->fields) {
        if (initialized.count(info.name) == 0) {
            diag_.error(node->span(),
                        "missing struct field '" + info.name + "'");
        }
    }
    return std::any(Type::makeStruct(structId));
}

// ---- Types ----

std::any TypeChecker::visitUnitType(ast::UnitType *node) {
    (void)node;
    return std::any(Type::makeUnit());
}

std::any TypeChecker::visitPathType(ast::PathType *node) {
    TypeResolver resolver(table_, diag_);
    return std::any(resolver.resolve(*node, currentSelf_));
}

std::any TypeChecker::visitReferenceType(ast::ReferenceType *node) {
    TypeResolver resolver(table_, diag_);
    return std::any(resolver.resolve(*node, currentSelf_));
}

std::any TypeChecker::visitArrayType(ast::ArrayType *node) {
    TypeResolver resolver(table_, diag_);
    return std::any(resolver.resolve(*node, currentSelf_));
}

} // namespace rx::semantic
