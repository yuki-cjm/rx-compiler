#include "typeck.hpp"

#include "assign.hpp"
#include "calls.hpp"
#include "coercion.hpp"
#include "control_flow.hpp"
#include "infer.hpp"
#include "operators.hpp"
#include "place.hpp"
#include "../resolve/resolver.hpp"
#include "../resolve/path_resolve.hpp"
#include "../consteval/const_eval.hpp"
#include "../builtin/builtin_traits.hpp"
#include "../ty/ty_compare.hpp"

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
            TyPtr target = resolver.resolve(*implementation->typeName, nullptr);
            if (target != nullptr && isStruct(target)) {
                for (const ast::ItemPtr &associated : implementation->associatedItems) {
                    if (auto *function =
                            dynamic_cast<ast::FunctionItem *>(associated.get())) {
                        checkFunctionBody(*function, target);
                    }
                }
            }
        }
    }
    return !diag_.hasErrors();
}

void TypeChecker::checkFunctionBody(ast::FunctionItem &function,
                                    const TyPtr &selfType) {
    TypeResolver resolver(table_, diag_);
    TyPtr returnType = function.returnType != nullptr
                           ? resolver.resolve(*function.returnType, selfType)
                           : Ty::makeUnit();

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
            symbol.type = Ty::makeReference(selfType, mutability);
            symbol.isMutable = false;
        }
        table_.scope().declare("self", symbol);
    }

    for (const ast::FunctionParam &parameter : function.parameters) {
        TyPtr type = resolver.resolve(*parameter.type, selfType);
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

TyPtr TypeChecker::resolveConstantType(const Symbol &symbol) {
    if (symbol.constant == nullptr) {
        return nullptr;
    }
    TypeResolver resolver(table_, diag_);
    TyPtr selfType =
        symbol.implTarget != 0 ? Ty::makeStruct(symbol.implTarget) : nullptr;
    return resolver.resolve(*symbol.constant->type, selfType);
}

TyPtr TypeChecker::checkExpr(ast::Expr &expr,
                             const std::optional<TyPtr> &expected) {
    auto saved = expected_;
    expected_ = expected;
    TyPtr result = std::any_cast<TyPtr>(expr.accept(*this));
    expected_ = saved;

    // 共享访问的 place 若持有 &mut T，读出的值降级为 &T。
    if (result != nullptr && isReference(result) &&
        result->mutability == Mutability::Mutable) {
        auto place = classifyPlace(expr);
        if (place && place->access == PlaceAccess::Shared) {
            result = Ty::makeReference(result->inner, Mutability::Shared);
        }
    }

    if (result != nullptr && expected && !sameTy(result, *expected)) {
        if (Coercion::canCoerce(table_, result, *expected)) {
            result = *expected;
        } else {
            diag_.error("expression does not coerce to the expected type");
        }
    }
    recordType(expr, result);
    return result;
}

void TypeChecker::recordType(const ast::Expr &expr, const TyPtr &ty) {
    exprTypes_[&expr] = ty;
}

TyPtr TypeChecker::typeOf(const ast::Expr &expr) const {
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
        result.access = symbol->isMutable ? PlaceAccess::Mutable
                                          : PlaceAccess::Owned;
        return result;
    }

    if (auto *unary = dynamic_cast<ast::UnaryExpr *>(&expr)) {
        if (unary->op != ast::UnaryOp::Dereference) {
            return std::nullopt;
        }
        TyPtr type = typeOf(*unary->operand);
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
        TyPtr current = typeOf(*field->operand);
        PlaceAccess access = baseAccess(*field->operand);
        while (current != nullptr && !isStruct(current)) {
            if (isReference(current)) {
                if (current->mutability == Mutability::Shared) {
                    access = PlaceAccess::Shared;
                } else {
                    access = access == PlaceAccess::Shared ? PlaceAccess::Shared
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
        TyPtr current = typeOf(*index->operand);
        PlaceAccess access = baseAccess(*index->operand);
        while (current != nullptr && !isArray(current) && !isVec(current)) {
            if (isReference(current)) {
                if (current->mutability == Mutability::Shared) {
                    access = PlaceAccess::Shared;
                } else {
                    access = access == PlaceAccess::Shared ? PlaceAccess::Shared
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
            result.access = access == PlaceAccess::Mutable ? PlaceAccess::Mutable
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
    return std::any(Ty::makeUnit());
}

std::any TypeChecker::visitLetStmt(ast::LetStmt *node) {
    TyPtr expected = nullptr;
    if (node->type != nullptr) {
        TypeResolver resolver(table_, diag_);
        expected = resolver.resolve(*node->type, currentSelf_);
        if (expected == nullptr) {
            return std::any(Ty::makeUnit());
        }
    }
    TyPtr initializer =
        checkExpr(*node->initializer,
                  expected ? std::optional<TyPtr>(expected) : std::nullopt);
    TyPtr bindingType = expected != nullptr ? expected : initializer;

    Symbol symbol;
    symbol.kind = SymbolKind::Local;
    symbol.name = node->binding.name;
    symbol.type = bindingType;
    symbol.isMutable = node->binding.isMutable;
    table_.scope().declare(node->binding.name, symbol);
    return std::any(Ty::makeUnit());
}

std::any TypeChecker::visitExprStmt(ast::ExprStmt *node) {
    bool withBlock =
        dynamic_cast<ast::BlockExpr *>(node->expression.get()) != nullptr ||
        dynamic_cast<ast::IfExpr *>(node->expression.get()) != nullptr ||
        dynamic_cast<ast::LoopExpr *>(node->expression.get()) != nullptr ||
        dynamic_cast<ast::WhileExpr *>(node->expression.get()) != nullptr;
    TyPtr type;
    if (withBlock && !node->hasSemicolon) {
        type = checkExpr(*node->expression, Ty::makeUnit());
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
    return std::any(Ty::makeScalar(TyKind::Bool));
}

std::any TypeChecker::visitUnitExpr(ast::UnitExpr *node) {
    (void)node;
    return std::any(Ty::makeUnit());
}

std::any TypeChecker::visitPathInExpr(ast::PathInExpr *node) {
    PathResolver resolver(table_, diag_);
    const Symbol *symbol = resolver.resolveValuePath(*node, currentSelf_);
    if (symbol == nullptr) {
        return std::any(TyPtr(nullptr));
    }
    if (symbol->kind == SymbolKind::Local) {
        return std::any(symbol->type);
    }
    if (symbol->kind == SymbolKind::Constant) {
        return std::any(resolveConstantType(*symbol));
    }
    // 函数值（function item as value）是 undefined behavior，返回空类型。
    return std::any(TyPtr(nullptr));
}

std::any TypeChecker::visitCallExpr(ast::CallExpr *node) {
    if (auto *fieldAccess =
            dynamic_cast<ast::FieldAccessExpr *>(node->callee.get())) {
        // 方法调用：receiver.method(args)。
        if (fieldAccess->field.genericArgs.has_value()) {
            diag_.error("method segments have no type parameters");
        }
        TyPtr receiverType = checkExpr(*fieldAccess->operand, std::nullopt);
        std::string methodName = fieldAccess->field.ident.name;
        auto signature = resolveMethod(table_, diag_, receiverType, methodName,
                                       currentSelf_);
        if (!signature) {
            return std::any(TyPtr(nullptr));
        }
        if (signature->receiver == ReceiverKind::Mutable) {
            PlaceAccess access = baseAccess(*fieldAccess->operand);
            TyPtr current = receiverType;
            while (current != nullptr && signature->selfType != nullptr &&
                   !sameTy(current, signature->selfType)) {
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
                diag_.error("method requires a mutable receiver");
            }
        }
        if (node->arguments.size() != signature->params.size()) {
            diag_.error("argument count mismatch");
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
        return std::any(TyPtr(nullptr));
    }
    if (node->arguments.size() != signature->params.size()) {
        diag_.error("argument count mismatch");
    } else {
        for (std::size_t index = 0; index < node->arguments.size(); ++index) {
            checkExpr(*node->arguments[index], signature->params[index]);
        }
    }
    return std::any(signature->result);
}

std::any TypeChecker::visitIndexExpr(ast::IndexExpr *node) {
    TyPtr baseType = checkExpr(*node->operand, std::nullopt);
    TyPtr indexType =
        checkExpr(*node->index, Ty::makeScalar(TyKind::Usize));

    TyPtr current = baseType;
    while (current != nullptr && !isArray(current) && !isVec(current)) {
        if (isReference(current) || isBox(current)) {
            current = current->inner;
        } else {
            break;
        }
    }
    if (current == nullptr || (!isArray(current) && !isVec(current))) {
        diag_.error("value is not indexable");
        return std::any(TyPtr(nullptr));
    }
    return std::any(current->inner);
}

std::any TypeChecker::visitFieldAccessExpr(ast::FieldAccessExpr *node) {
    TyPtr baseType = checkExpr(*node->operand, std::nullopt);
    StructId structId = 0;
    bool crossedShared = false;
    if (!PlaceRules::stripToStruct(baseType, &structId, &crossedShared)) {
        diag_.error("field access requires a struct");
        return std::any(TyPtr(nullptr));
    }
    const FieldInfo *field =
        table_.lookupField(structId, node->field.ident.name);
    if (field == nullptr) {
        diag_.error("unknown field '" + node->field.ident.name + "'");
        return std::any(TyPtr(nullptr));
    }
    return std::any(field->type);
}

std::any TypeChecker::visitUnaryExpr(ast::UnaryExpr *node) {
    TyPtr operand = checkExpr(*node->operand, std::nullopt);

    if (node->op == ast::UnaryOp::BorrowMut ||
        node->op == ast::UnaryOp::DoubleBorrowMut) {
        auto place = classifyPlace(*node->operand);
        if (place && place->access != PlaceAccess::Mutable) {
            diag_.error("cannot mutably borrow an immutable place");
        }
    }

    auto result = Operators::unaryResult(node->op, operand);
    if (!result) {
        diag_.error("invalid operand for unary operator");
        return std::any(TyPtr(nullptr));
    }
    return std::any(*result);
}

std::any TypeChecker::visitCastExpr(ast::CastExpr *node) {
    TyPtr operand = checkExpr(*node->operand, std::nullopt);
    TypeResolver resolver(table_, diag_);
    TyPtr target = resolver.resolve(*node->targetType, currentSelf_);
    if (target == nullptr) {
        return std::any(TyPtr(nullptr));
    }
    bool valid = (isInteger(operand) && isInteger(target)) ||
                 (isBool(operand) && isInteger(target));
    if (!valid) {
        diag_.error("unsupported cast");
    }
    return std::any(target);
}

std::any TypeChecker::visitBinaryExpr(ast::BinaryExpr *node) {
    if (Operators::isAssignment(node->op)) {
        TyPtr leftType = checkExpr(*node->left, std::nullopt);
        auto place = classifyPlace(*node->left);
        if (!place) {
            diag_.error("assignment target is not a place");
            return std::any(Ty::makeUnit());
        }
        PlaceInfo target{place->type, place->access == PlaceAccess::Mutable};
        TyPtr rightType =
            checkExpr(*node->right, std::optional<TyPtr>(place->type));
        AssignChecker::check(table_, target, rightType, diag_);
        return std::any(Ty::makeUnit());
    }

    if (Operators::isCompoundAssignment(node->op)) {
        TyPtr leftType = checkExpr(*node->left, std::nullopt);
        auto place = classifyPlace(*node->left);
        if (!place) {
            diag_.error("compound assignment target is not a place");
            return std::any(Ty::makeUnit());
        }
        PlaceInfo target{place->type, place->access == PlaceAccess::Mutable};
        TyPtr rightType = checkExpr(*node->right, std::nullopt);
        AssignChecker::checkCompound(table_, target, node->op, rightType,
                                     diag_);
        return std::any(Ty::makeUnit());
    }

    TyPtr leftType = checkExpr(*node->left, std::nullopt);
    TyPtr rightType = checkExpr(*node->right, std::nullopt);
    auto result = Operators::binaryResult(node->op, leftType, rightType);
    if (!result) {
        diag_.error("invalid operands for binary operator");
        return std::any(Ty::makeNever());
    }
    if (node->op == ast::BinaryOp::Equal ||
        node->op == ast::BinaryOp::NotEqual) {
        if (!supportsPartialEq(table_, leftType)) {
            diag_.error("equality requires PartialEq");
        }
    }
    return std::any(*result);
}

std::any TypeChecker::visitArrayExpr(ast::ArrayExpr *node) {
    TyPtr expectedElement = nullptr;
    std::uint64_t expectedLength = 0;
    if (expected_ && isArray(*expected_)) {
        expectedElement = (*expected_)->inner;
        expectedLength = (*expected_)->length;
    }

    if (node->repeatCount.has_value()) {
        TyPtr element = checkExpr(
            *node->elements[0],
            expectedElement ? std::optional<TyPtr>(expectedElement)
                            : std::nullopt);
        auto length = evaluateArrayLength(table_, diag_, *node->repeatCount,
                                          currentSelf_);
        if (!length) {
            return std::any(TyPtr(nullptr));
        }
        if (expected_ && isArray(*expected_) && *length != expectedLength) {
            diag_.error("array length mismatch");
        }
        if (*length > 1 && !isCopy(table_, element)) {
            diag_.error("repeating an element more than once requires Copy");
        }
        return std::any(Ty::makeArray(element, *length));
    }

    std::vector<TyPtr> elementTypes;
    for (ast::ExprPtr &element : node->elements) {
        elementTypes.push_back(
            checkExpr(*element,
                      expectedElement ? std::optional<TyPtr>(expectedElement)
                                      : std::nullopt));
    }
    if (expected_ && isArray(*expected_) &&
        node->elements.size() != expectedLength) {
        diag_.error("array length mismatch");
    }
    TyPtr elementType;
    if (expectedElement != nullptr) {
        elementType = expectedElement;
    } else {
        auto lub = Coercion::leastUpperBound(table_, elementTypes);
        elementType = lub ? *lub : Ty::makeNever();
    }
    return std::any(Ty::makeArray(elementType, node->elements.size()));
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

    TyPtr result;
    if (node->tail != nullptr) {
        result = checkExpr(*node->tail, expected_);
    } else {
        result = blockDiverges ? Ty::makeNever() : Ty::makeUnit();
    }

    table_.scope().pop();
    return std::any(result);
}

std::any TypeChecker::visitIfExpr(ast::IfExpr *node) {
    TyPtr condition = checkExpr(*node->condition, Ty::makeScalar(TyKind::Bool));
    if (!isBool(condition)) {
        diag_.error("if condition must be bool");
    }

    if (node->elseBranch != nullptr) {
        TyPtr thenType = checkExpr(
            node->thenBranch,
            expected_ ? std::optional<TyPtr>(*expected_) : std::nullopt);
        TyPtr elseType = checkExpr(
            *node->elseBranch,
            expected_ ? std::optional<TyPtr>(*expected_) : std::nullopt);
        if (expected_) {
            return std::any(*expected_);
        }
        auto lub = Coercion::leastUpperBound(table_, {thenType, elseType});
        if (!lub) {
            diag_.error("incompatible if branches");
            return std::any(Ty::makeNever());
        }
        return std::any(*lub);
    }

    TyPtr thenType = checkExpr(node->thenBranch, Ty::makeUnit());
    (void)thenType;
    return std::any(Ty::makeUnit());
}

std::any TypeChecker::visitLoopExpr(ast::LoopExpr *node) {
    loopDepth_++;
    loopIsWhile_.push_back(false);
    loopBreakValues_.emplace_back();
    auto savedBreak = currentLoopBreak_;
    currentLoopBreak_ = expected_;

    checkExpr(node->body, Ty::makeUnit());

    auto breaks = std::move(loopBreakValues_.back());
    loopBreakValues_.pop_back();
    loopIsWhile_.pop_back();
    currentLoopBreak_ = savedBreak;
    loopDepth_--;

    if (expected_) {
        return std::any(*expected_);
    }
    if (breaks.empty()) {
        return std::any(Ty::makeNever());
    }
    auto lub = Coercion::leastUpperBound(table_, breaks);
    if (!lub) {
        diag_.error("incompatible break values");
        return std::any(Ty::makeNever());
    }
    return std::any(*lub);
}

std::any TypeChecker::visitWhileExpr(ast::WhileExpr *node) {
    conditionBarriers_.push_back(loopDepth_);
    TyPtr condition = checkExpr(*node->condition, Ty::makeScalar(TyKind::Bool));
    conditionBarriers_.pop_back();
    if (!isBool(condition)) {
        diag_.error("while condition must be bool");
    }

    loopDepth_++;
    loopIsWhile_.push_back(true);
    loopBreakValues_.emplace_back();
    checkExpr(node->body, Ty::makeUnit());
    loopBreakValues_.pop_back();
    loopIsWhile_.pop_back();
    loopDepth_--;

    return std::any(Ty::makeUnit());
}

std::any TypeChecker::visitBreakExpr(ast::BreakExpr *node) {
    if (loopDepth_ <= 0) {
        diag_.error("break used outside a loop");
        return std::any(Ty::makeNever());
    }
    if (!conditionBarriers_.empty() &&
        loopDepth_ <= conditionBarriers_.back()) {
        diag_.error("break in a while condition cannot target an outer loop");
        return std::any(Ty::makeNever());
    }

    if (node->value != nullptr) {
        if (loopIsWhile_.back()) {
            diag_.error("a while loop cannot accept a break value");
        } else {
            TyPtr value = checkExpr(
                *node->value,
                currentLoopBreak_ ? std::optional<TyPtr>(*currentLoopBreak_)
                                  : std::nullopt);
            loopBreakValues_.back().push_back(value);
        }
    } else if (!loopIsWhile_.back()) {
        loopBreakValues_.back().push_back(Ty::makeUnit());
    }
    return std::any(Ty::makeNever());
}

std::any TypeChecker::visitReturnExpr(ast::ReturnExpr *node) {
    TyPtr valueType;
    if (node->value != nullptr) {
        valueType = checkExpr(
            *node->value,
            currentReturn_ ? std::optional<TyPtr>(*currentReturn_)
                           : std::nullopt);
    } else {
        valueType = Ty::makeUnit();
    }
    if (currentReturn_ && !isNever(valueType) &&
        !Coercion::canCoerce(table_, valueType, *currentReturn_)) {
        diag_.error("return value does not match the function return type");
    }
    return std::any(Ty::makeNever());
}

std::any TypeChecker::visitContinueExpr(ast::ContinueExpr *node) {
    (void)node;
    if (loopDepth_ <= 0) {
        diag_.error("continue used outside a loop");
    } else if (!conditionBarriers_.empty() &&
               loopDepth_ <= conditionBarriers_.back()) {
        diag_.error("continue in a while condition cannot target an outer loop");
    }
    return std::any(Ty::makeNever());
}

std::any TypeChecker::visitStructExpr(ast::StructExpr *node) {
    const ast::PathInExpr &path = node->path;
    StructId structId = 0;
    if (path.segments.size() != 1) {
        diag_.error("struct expression path must be a single name");
        return std::any(TyPtr(nullptr));
    }
    const ast::PathExprSegment &segment = path.segments[0];
    if (segment.ident.kind == ast::PathSegmentKind::SelfType) {
        if (!isStruct(currentSelf_)) {
            diag_.error("'Self' used outside a struct context");
            return std::any(TyPtr(nullptr));
        }
        structId = currentSelf_->structId;
    } else if (segment.ident.kind == ast::PathSegmentKind::Ident) {
        const Symbol *symbol = table_.lookupType(segment.ident.name);
        if (symbol == nullptr || symbol->kind != SymbolKind::Struct) {
            diag_.error("'" + segment.ident.name + "' is not a struct");
            return std::any(TyPtr(nullptr));
        }
        structId = symbol->structId;
    } else {
        diag_.error("invalid struct expression path");
        return std::any(TyPtr(nullptr));
    }

    const Symbol *structure = table_.lookupStruct(structId);
    std::set<std::string> initialized;
    for (const ast::StructExprField &field : node->fields) {
        if (!initialized.insert(field.name).second) {
            diag_.error("duplicate struct field '" + field.name + "'");
        }
        const FieldInfo *info = table_.lookupField(structId, field.name);
        if (info == nullptr) {
            diag_.error("unknown struct field '" + field.name + "'");
            continue;
        }
        checkExpr(*field.value, info->type);
    }
    for (const FieldInfo &info : structure->fields) {
        if (initialized.count(info.name) == 0) {
            diag_.error("missing struct field '" + info.name + "'");
        }
    }
    return std::any(Ty::makeStruct(structId));
}

// ---- Types ----

std::any TypeChecker::visitUnitType(ast::UnitType *node) {
    (void)node;
    return std::any(Ty::makeUnit());
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
