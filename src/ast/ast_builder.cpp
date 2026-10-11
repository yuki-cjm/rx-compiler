#include "ast_builder.hpp"
#include "ast.hpp"

#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace rx::ast {
namespace {

void unsupported(antlr4::ParserRuleContext *context) {
    throw std::runtime_error("AST sample does not support this syntax: " +
                             context->getText());
}

std::vector<antlr4::ParserRuleContext *>
ruleChildren(antlr4::ParserRuleContext *context) {
    std::vector<antlr4::ParserRuleContext *> result;
    for (auto *child : context->children) {
        if (auto *rule = dynamic_cast<antlr4::ParserRuleContext *>(child)) {
            result.push_back(rule);
        }
    }
    return result;
}

GenericParam buildGenericParams(Parser::GenericParamsContext *context) {
    GenericParam genericParams;
    genericParams.params.reserve(context->lifetimeParam().size());

    for (auto *lifetimeParam : context->lifetimeParam()) {
        LifetimeParam parameter;
        parameter.lifetime = lifetimeParam->lifetime()->getText();
        if (auto *bounds = lifetimeParam->lifetimeBounds()) {
            std::vector<std::string> boundNames;
            boundNames.reserve(bounds->lifetime().size());
            for (auto *bound : bounds->lifetime()) {
                boundNames.push_back(bound->getText());
            }
            parameter.lifetimeBounds = std::move(boundNames);
        }
        genericParams.params.push_back(std::move(parameter));
    }

    return genericParams;
}

WhereClause buildWhereClause(Parser::WhereClauseContext *context) {
    WhereClause clause;
    clause.items.reserve(context->whereClauseItem().size());

    for (auto *itemContext : context->whereClauseItem()) {
        WhereClauseItem item;
        if (auto *lifetime = itemContext->lifetime()) {
            item.lifetime = lifetime->getText();
            if (auto *bounds = itemContext->lifetimeBounds()) {
                item.bounds.reserve(bounds->lifetime().size());
                for (auto *bound : bounds->lifetime()) {
                    item.bounds.push_back(bound->getText());
                }
            }
        } else if (auto *type = itemContext->typeRef()) {
            item.type = type->getText();
            if (auto *bounds = itemContext->typeParamBounds()) {
                item.bounds.reserve(bounds->lifetime().size());
                for (auto *bound : bounds->lifetime()) {
                    item.bounds.push_back(bound->getText());
                }
            }
        } else {
            unsupported(itemContext);
        }
        clause.items.push_back(std::move(item));
    }

    return clause;
}

ConstValue buildMagnitude(Parser::MagnitudeContext *context) {
    if (context->LPAREN() != nullptr) {
        return buildMagnitude(context->magnitude());
    }

    ConstValue value;
    if (context->INTEGER_LITERAL() != nullptr) {
        value.kind = ConstValue::Kind::Integer;
        value.value = context->INTEGER_LITERAL()->getText();
    } else if (context->pathInExpression() != nullptr) {
        value.kind = ConstValue::Kind::Path;
        value.value = context->pathInExpression()->getText();
    } else {
        unsupported(context);
    }
    return value;
}

ConstValue buildConstValue(Parser::ConstValueContext *context) {
    if (context->LPAREN() != nullptr) {
        return buildConstValue(context->constValue());
    }

    ConstValue value;
    if (context->INTEGER_LITERAL() != nullptr) {
        value.kind = ConstValue::Kind::Integer;
        value.value = context->INTEGER_LITERAL()->getText();
    } else if (context->TRUE() != nullptr) {
        value.kind = ConstValue::Kind::Boolean;
        value.value = "true";
    } else if (context->FALSE() != nullptr) {
        value.kind = ConstValue::Kind::Boolean;
        value.value = "false";
    } else if (context->pathInExpression() != nullptr) {
        value.kind = ConstValue::Kind::Path;
        value.value = context->pathInExpression()->getText();
    } else if (context->MINUS() != nullptr) {
        value.kind = ConstValue::Kind::Negate;
        value.operand = std::make_unique<ConstValue>();
        *value.operand = buildMagnitude(context->magnitude());
    } else {
        unsupported(context);
    }
    return value;
}

} // namespace

Program AstBuilder::build(Parser::CrateContext *crate) const {
    Program program;
    for (auto *item : crate->item()) {
        program.items.push_back(buildItem(item));
    }
    return program;
}

ItemPtr AstBuilder::buildItem(Parser::ItemContext *context) const {
    const auto buildFunctionItem =
        [this](Parser::FunctionDefinitionContext *function) {
            FunctionItem item;
            item.name = function->identifier()->getText();
            if (auto *genericParams = function->genericParams()) {
                item.genericParams = buildGenericParams(genericParams);
            }
            if (auto *returnType = function->typeRef()) {
                item.returnType = returnType->getText();
            }
            if (auto *parameters = function->functionParameters()) {
                if (auto *selfParam = parameters->selfParam()) {
                    SelfParam self;
                    self.isBorrowed = selfParam->AMP() != nullptr;
                    self.isMutable = selfParam->MUT() != nullptr;
                    if (auto *lifetime = selfParam->lifetime()) {
                        self.lifetime = lifetime->getText();
                    }
                    item.selfParam = std::move(self);
                }
                for (auto *parameter : parameters->functionParam()) {
                    item.parameters.push_back(
                        {parameter->identifierBinding()->getText(),
                         parameter->typeRef()->getText()});
                }
            }
            if (auto *whereClause = function->whereClause()) {
                item.whereClause = buildWhereClause(whereClause);
            }
            item.body = std::move(static_cast<BlockExpr &>(
                *buildExpr(function->blockExpression())));
            return item;
        };

    const auto buildConstantItem = [](Parser::ConstantItemContext *constant) {
        ConstantItem item;
        item.name = constant->identifier()->getText();
        item.type = constant->typeRef()->getText();
        item.value = buildConstValue(constant->constValue());
        return item;
    };

    if (auto *function = context->functionDefinition()) {
        return std::make_unique<FunctionItem>(buildFunctionItem(function));
    }

    if (auto *structure = context->structDefinition()) {
        StructItem item;
        item.name = structure->identifier()->getText();
        if (auto *genericParams = structure->genericParams()) {
            item.genericParams = buildGenericParams(genericParams);
        }
        for (auto *attribute : structure->outerAttribute()) {
            std::vector<DeriveTrait> traits;
            traits.reserve(attribute->deriveName().size());
            for (auto *deriveName : attribute->deriveName()) {
                traits.push_back(deriveTraitFromText(deriveName->getText()));
            }
            item.attributes.emplace_back(std::move(traits));
        }
        for (auto *field : structure->structField()) {
            item.fields.emplace_back(field->identifier()->getText(),
                                     field->typeRef()->getText());
        }
        if (auto *whereClause = structure->whereClause()) {
            item.whereClause = buildWhereClause(whereClause);
        }
        return std::make_unique<StructItem>(std::move(item));
    }

    if (auto *constant = context->constantItem()) {
        return std::make_unique<ConstantItem>(buildConstantItem(constant));
    }

    if (auto *implementation = context->inherentImpl()) {
        ImplItem item;
        item.typeName = implementation->typeRef()->getText();
        if (auto *genericParams = implementation->genericParams()) {
            item.genericParams = buildGenericParams(genericParams);
        }
        if (auto *whereClause = implementation->whereClause()) {
            item.whereClause = buildWhereClause(whereClause);
        }
        for (auto *associated : implementation->associatedItem()) {
            if (associated->constantItem() != nullptr) {
                item.associatedItems.push_back(std::make_unique<ConstantItem>(
                    buildConstantItem(associated->constantItem())));
            } else if (associated->functionDefinition() != nullptr) {
                item.associatedItems.push_back(std::make_unique<FunctionItem>(
                    buildFunctionItem(associated->functionDefinition())));
            } else {
                unsupported(associated);
            }
        }
        return std::make_unique<ImplItem>(std::move(item));
    }

    unsupported(context);
}

StmtPtr AstBuilder::buildStmt(Parser::StatementContext *context) const {
    if (auto *let = context->letStatement()) {
        auto statement = std::make_unique<LetStmt>();
        statement->name = let->identifierBinding()->getText();
        if (auto *type = let->typeRef()) {
            statement->type = type->getText();
        }
        statement->initializer = buildExpr(let->expression());
        return statement;
    }

    if (auto *expressionWithBlock = context->expressionWithBlock()) {
        return std::make_unique<ExprStmt>(buildExpr(expressionWithBlock));
    }

    if (auto *expression = context->statementExpression()) {
        return std::make_unique<ExprStmt>(buildExpr(expression));
    }

    if (context->SEMI() != nullptr) {
        return std::make_unique<EmptyStmt>();
    }

    unsupported(context);
}

ExprPtr AstBuilder::buildExpr(antlr4::ParserRuleContext *context) const {
    const auto buildBlock = [this](Parser::BlockExpressionContext *block) {
        std::vector<StmtPtr> statements;
        for (auto *statement : block->statement()) {
            statements.push_back(buildStmt(statement));
        }
        ExprPtr tail;
        if (auto *tailContext = block->statementExpression()) {
            tail = buildExpr(tailContext);
        }
        return BlockExpr(std::move(statements), std::move(tail));
    };

    const auto buildPath = [](Parser::PathInExpressionContext *path) {
        std::vector<std::string> segments;
        for (auto *segment : path->pathExprSegment()) {
            segments.push_back(segment->getText());
        }
        return PathExpr(std::move(segments));
    };

    const auto buildExpressionWithBlock =
        [this, &buildBlock](
            Parser::ExpressionWithBlockContext *expression) -> ExprPtr {
        if (expression->ifExpression() != nullptr) {
            return buildExpr(expression->ifExpression());
        }
        if (expression->LOOP() != nullptr) {
            if (expression->blockExpression() == nullptr) {
                unsupported(expression);
            }
            return std::make_unique<LoopExpr>(
                buildBlock(expression->blockExpression()));
        }
        if (expression->WHILE() != nullptr) {
            if (expression->conditionExpression() == nullptr ||
                expression->blockExpression() == nullptr) {
                unsupported(expression);
            }
            return std::make_unique<WhileExpr>(
                buildExpr(expression->conditionExpression()),
                buildBlock(expression->blockExpression()));
        }
        if (expression->blockExpression() != nullptr) {
            return buildExpr(expression->blockExpression());
        }
        unsupported(expression);
    };

    const auto buildUnary = [this](antlr4::ParserRuleContext *operatorContext,
                                   antlr4::ParserRuleContext *operandContext) {
        if (operatorContext != nullptr) {
            return ExprPtr(std::make_unique<UnaryExpr>(
                unaryOpFromText(operatorContext->getText()),
                buildExpr(operandContext)));
        }
        return buildExpr(operandContext);
    };

    const auto applyDotSuffix = [this](ExprPtr result,
                                       Parser::DotSuffixContext *dotSuffix) {
        std::string field;
        if (dotSuffix->pathExprSegment() != nullptr) {
            field = dotSuffix->pathExprSegment()->getText();
        } else if (dotSuffix->identifier() != nullptr) {
            field = dotSuffix->identifier()->getText();
        } else {
            unsupported(dotSuffix);
        }

        result = std::make_unique<FieldAccessExpr>(std::move(result),
                                                   std::move(field));
        if (auto *arguments = dotSuffix->callArguments()) {
            std::vector<ExprPtr> builtArguments;
            for (auto *argument : arguments->expression()) {
                builtArguments.push_back(buildExpr(argument));
            }
            result = std::make_unique<CallExpr>(std::move(result),
                                                std::move(builtArguments));
        }
        return result;
    };

    const auto applyPostfixSuffixes =
        [this, &applyDotSuffix](
            ExprPtr result,
            const std::vector<Parser::PostfixSuffixContext *> &suffixes) {
            for (auto *suffix : suffixes) {
                if (auto *arguments = suffix->callArguments()) {
                    std::vector<ExprPtr> builtArguments;
                    for (auto *argument : arguments->expression()) {
                        builtArguments.push_back(buildExpr(argument));
                    }
                    result = std::make_unique<CallExpr>(
                        std::move(result), std::move(builtArguments));
                } else if (suffix->LBRACKET() != nullptr) {
                    if (suffix->expression() == nullptr) {
                        unsupported(suffix);
                    }
                    result = std::make_unique<IndexExpr>(
                        std::move(result), buildExpr(suffix->expression()));
                } else if (suffix->dotSuffix() != nullptr) {
                    result =
                        applyDotSuffix(std::move(result), suffix->dotSuffix());
                } else {
                    unsupported(suffix);
                }
            }
            return result;
        };

    if (context->getRuleIndex() == Parser::RulePostfixExpression) {
        auto *postfix =
            dynamic_cast<Parser::PostfixExpressionContext *>(context);
        return applyPostfixSuffixes(buildExpr(postfix->primaryExpression()),
                                    postfix->postfixSuffix());
    }

    if (context->getRuleIndex() == Parser::RuleConditionPostfixExpression) {
        auto *postfix =
            dynamic_cast<Parser::ConditionPostfixExpressionContext *>(context);
        return applyPostfixSuffixes(buildExpr(postfix->conditionPrimary()),
                                    postfix->postfixSuffix());
    }

    if (context->getRuleIndex() ==
        Parser::RuleConditionBreakPostfixExpression) {
        auto *postfix =
            dynamic_cast<Parser::ConditionBreakPostfixExpressionContext *>(
                context);
        return applyPostfixSuffixes(
            buildExpr(postfix->conditionPrimaryWithoutBareBlock()),
            postfix->postfixSuffix());
    }

    if (context->getRuleIndex() == Parser::RuleStatementPostfixExpression) {
        auto *postfix =
            dynamic_cast<Parser::StatementPostfixExpressionContext *>(context);
        if (postfix->expressionWithBlock() != nullptr) {
            auto *expressionWithBlock = postfix->expressionWithBlock();
            if (postfix->dotSuffix() == nullptr) {
                unsupported(expressionWithBlock);
            }
            ExprPtr result = buildExpr(expressionWithBlock);
            result = applyDotSuffix(std::move(result), postfix->dotSuffix());
            return applyPostfixSuffixes(std::move(result),
                                        postfix->postfixSuffix());
        }
        return applyPostfixSuffixes(buildExpr(postfix->nonBlockPrimary()),
                                    postfix->postfixSuffix());
    }

    if (context->getRuleIndex() == Parser::RulePrimaryExpression) {
        auto *primary =
            dynamic_cast<Parser::PrimaryExpressionContext *>(context);
        if (primary->expressionWithBlock() != nullptr) {
            return buildExpr(primary->expressionWithBlock());
        }
        return buildExpr(primary->nonBlockPrimary());
    }

    if (context->getRuleIndex() == Parser::RuleExpressionWithBlock) {
        return buildExpressionWithBlock(
            dynamic_cast<Parser::ExpressionWithBlockContext *>(context));
    }

    if (context->getRuleIndex() == Parser::RuleConditionPrimary) {
        auto *primary =
            dynamic_cast<Parser::ConditionPrimaryContext *>(context);
        if (primary->blockExpression() != nullptr) {
            return buildExpr(primary->blockExpression());
        }
        return buildExpr(primary->conditionPrimaryWithoutBareBlock());
    }

    if (context->getRuleIndex() == Parser::RuleBlockExpression) {
        auto *block = dynamic_cast<Parser::BlockExpressionContext *>(context);
        return std::make_unique<BlockExpr>(buildBlock(block));
    }

    if (context->getRuleIndex() == Parser::RuleUnaryExpression) {
        auto *unary = dynamic_cast<Parser::UnaryExpressionContext *>(context);
        if (unary->unaryOperator() != nullptr) {
            return buildUnary(unary->unaryOperator(), unary->unaryExpression());
        }
        return buildUnary(nullptr, unary->postfixExpression());
    }

    if (context->getRuleIndex() == Parser::RuleConditionUnaryExpression) {
        auto *unary =
            dynamic_cast<Parser::ConditionUnaryExpressionContext *>(context);
        if (unary->unaryOperator() != nullptr) {
            return buildUnary(unary->unaryOperator(),
                              unary->conditionUnaryExpression());
        }
        return buildUnary(nullptr, unary->conditionPostfixExpression());
    }

    if (context->getRuleIndex() == Parser::RuleConditionBreakUnaryExpression) {
        auto *unary =
            dynamic_cast<Parser::ConditionBreakUnaryExpressionContext *>(
                context);
        if (unary->unaryOperator() != nullptr) {
            return buildUnary(unary->unaryOperator(),
                              unary->conditionUnaryExpression());
        }
        return buildUnary(nullptr, unary->conditionBreakPostfixExpression());
    }

    if (context->getRuleIndex() == Parser::RuleStatementUnaryExpression) {
        auto *unary =
            dynamic_cast<Parser::StatementUnaryExpressionContext *>(context);
        if (unary->unaryOperator() != nullptr) {
            return buildUnary(unary->unaryOperator(), unary->unaryExpression());
        }
        return buildUnary(nullptr, unary->statementPostfixExpression());
    }

    if (context->getRuleIndex() == Parser::RuleCastExpression ||
        context->getRuleIndex() == Parser::RuleConditionCastExpression ||
        context->getRuleIndex() == Parser::RuleStatementCastExpression) {
        ExprPtr result;
        std::vector<Parser::TypeRefContext *> targetTypes;

        if (context->getRuleIndex() == Parser::RuleCastExpression) {
            auto *cast = dynamic_cast<Parser::CastExpressionContext *>(context);
            result = buildExpr(cast->unaryExpression());
            targetTypes = cast->typeRef();
        } else if (context->getRuleIndex() ==
                   Parser::RuleConditionCastExpression) {
            auto *cast =
                dynamic_cast<Parser::ConditionCastExpressionContext *>(context);
            result = buildExpr(cast->conditionUnaryExpression());
            targetTypes = cast->typeRef();
        } else {
            auto *cast =
                dynamic_cast<Parser::StatementCastExpressionContext *>(context);
            result = buildExpr(cast->statementUnaryExpression());
            targetTypes = cast->typeRef();
        }

        for (auto *targetType : targetTypes) {
            result = std::make_unique<CastExpr>(std::move(result),
                                                targetType->getText());
        }
        return result;
    }

    if (context->getRuleIndex() == Parser::RuleLiteralExpression) {
        auto *literal =
            dynamic_cast<Parser::LiteralExpressionContext *>(context);
        if (literal->INTEGER_LITERAL() != nullptr) {
            return std::make_unique<IntegerLiteralExpr>(
                literal->INTEGER_LITERAL()->getText());
        }
        return std::make_unique<BooleanLiteralExpr>(literal->TRUE() != nullptr);
    }

    if (context->getRuleIndex() == Parser::RuleMultiplicativeExpression) {
        auto *multiplicative =
            dynamic_cast<Parser::MultiplicativeExpressionContext *>(context);
        const auto operands = multiplicative->castExpression();
        const auto operators = multiplicative->multiplicativeOperator();

        ExprPtr result = buildExpr(operands.front());

        for (std::size_t i = 0; i < operators.size(); i++) {
            result = std::make_unique<BinaryExpr>(
                binaryOpFromText(operators[i]->getText()), std::move(result),
                buildExpr(operands[i + 1]));
        }

        return result;
    }

    if (context->getRuleIndex() ==
        Parser::RuleStatementMultiplicativeExpression) {
        auto *multiplicative =
            dynamic_cast<Parser::StatementMultiplicativeExpressionContext *>(
                context);
        const auto operators = multiplicative->multiplicativeOperator();

        ExprPtr result = buildExpr(multiplicative->statementCastExpression());
        const auto operands = multiplicative->castExpression();

        for (std::size_t i = 0; i < operators.size(); ++i) {
            result = std::make_unique<BinaryExpr>(
                binaryOpFromText(operators[i]->getText()), std::move(result),
                buildExpr(operands[i]));
        }

        return result;
    }

    if (context->getRuleIndex() ==
        Parser::RuleConditionMultiplicativeExpression) {
        auto *multiplicative =
            dynamic_cast<Parser::ConditionMultiplicativeExpressionContext *>(
                context);
        const auto operands = multiplicative->conditionCastExpression();
        const auto operators = multiplicative->multiplicativeOperator();

        ExprPtr result = buildExpr(operands.front());
        for (std::size_t i = 0; i < operators.size(); ++i) {
            result = std::make_unique<BinaryExpr>(
                binaryOpFromText(operators[i]->getText()), std::move(result),
                buildExpr(operands[i + 1]));
        }
        return result;
    }

    if (context->getRuleIndex() == Parser::RuleNonBlockPrimary ||
        context->getRuleIndex() ==
            Parser::RuleConditionPrimaryWithoutBareBlock) {
        auto *primary = dynamic_cast<Parser::NonBlockPrimaryContext *>(context);
        if (primary != nullptr && primary->pathInExpression() != nullptr) {
            PathExpr path = buildPath(primary->pathInExpression());
            if (primary->LBRACE() == nullptr) {
                return std::make_unique<PathExpr>(std::move(path));
            }

            std::vector<StructExprField> fields;
            if (auto *structFields = primary->structExprFields()) {
                fields.reserve(structFields->structExprField().size());
                for (auto *field : structFields->structExprField()) {
                    fields.emplace_back(field->identifier()->getText(),
                                        buildExpr(field->expression()));
                }
            }
            return std::make_unique<StructExpr>(std::move(path),
                                                std::move(fields));
        }
        if (primary != nullptr && primary->LPAREN() != nullptr) {
            if (primary->expression() == nullptr) {
                return std::make_unique<UnitExpr>();
            }
            return buildExpr(primary->expression());
        }
        if (primary != nullptr) {
            if (primary->BREAK() != nullptr) {
                ExprPtr value;
                if (primary->expression() != nullptr) {
                    value = buildExpr(primary->expression());
                }
                return std::make_unique<BreakExpr>(std::move(value));
            }
            if (primary->RETURN() != nullptr) {
                ExprPtr value;
                if (primary->expression() != nullptr) {
                    value = buildExpr(primary->expression());
                }
                return std::make_unique<ReturnExpr>(std::move(value));
            }
            if (primary->CONTINUE() != nullptr) {
                return std::make_unique<ContinueExpr>();
            }
            if (primary->LBRACE() != nullptr) {
                unsupported(context);
            }
        }

        auto *conditionPrimary =
            dynamic_cast<Parser::ConditionPrimaryWithoutBareBlockContext *>(
                context);
        if (conditionPrimary != nullptr &&
            conditionPrimary->LPAREN() != nullptr) {
            if (conditionPrimary->expression() == nullptr) {
                return std::make_unique<UnitExpr>();
            }
            return buildExpr(conditionPrimary->expression());
        }
        if (conditionPrimary != nullptr &&
            (conditionPrimary->ifExpression() != nullptr ||
             conditionPrimary->LOOP() != nullptr ||
             conditionPrimary->WHILE() != nullptr ||
             conditionPrimary->BREAK() != nullptr ||
             conditionPrimary->RETURN() != nullptr ||
             conditionPrimary->CONTINUE() != nullptr)) {
            if (conditionPrimary->ifExpression() != nullptr) {
                return buildExpr(conditionPrimary->ifExpression());
            }
            if (conditionPrimary->LOOP() != nullptr) {
                if (conditionPrimary->blockExpression() == nullptr) {
                    unsupported(context);
                }
                return std::make_unique<LoopExpr>(
                    buildBlock(conditionPrimary->blockExpression()));
            }
            if (conditionPrimary->WHILE() != nullptr) {
                if (conditionPrimary->conditionExpression() == nullptr ||
                    conditionPrimary->blockExpression() == nullptr) {
                    unsupported(context);
                }
                return std::make_unique<WhileExpr>(
                    buildExpr(conditionPrimary->conditionExpression()),
                    buildBlock(conditionPrimary->blockExpression()));
            }
            if (conditionPrimary->BREAK() != nullptr) {
                ExprPtr value;
                if (conditionPrimary->conditionBreakExpression() != nullptr) {
                    value =
                        buildExpr(conditionPrimary->conditionBreakExpression());
                }
                return std::make_unique<BreakExpr>(std::move(value));
            }
            if (conditionPrimary->RETURN() != nullptr) {
                ExprPtr value;
                if (conditionPrimary->conditionExpression() != nullptr) {
                    value = buildExpr(conditionPrimary->conditionExpression());
                }
                return std::make_unique<ReturnExpr>(std::move(value));
            }
            if (conditionPrimary->CONTINUE() != nullptr) {
                return std::make_unique<ContinueExpr>();
            }
        }
    }

    if (context->getRuleIndex() == Parser::RulePathInExpression) {
        auto *path = dynamic_cast<Parser::PathInExpressionContext *>(context);
        return std::make_unique<PathExpr>(buildPath(path));
    }

    if (context->getRuleIndex() == Parser::RuleAdditiveExpression) {
        auto *additive =
            dynamic_cast<Parser::AdditiveExpressionContext *>(context);
        const auto operands = additive->multiplicativeExpression();
        ExprPtr result = buildExpr(operands.front());
        for (std::size_t i = 1; i < operands.size(); ++i) {
            result = std::make_unique<BinaryExpr>(
                binaryOpFromText(additive->additiveOperator(i - 1)->getText()),
                std::move(result), buildExpr(operands[i]));
        }
        return result;
    }

    if (context->getRuleIndex() == Parser::RuleStatementAdditiveExpression) {
        auto *additive =
            dynamic_cast<Parser::StatementAdditiveExpressionContext *>(context);
        std::vector<antlr4::ParserRuleContext *> operands;
        operands.push_back(additive->statementMultiplicativeExpression());
        for (auto *operand : additive->multiplicativeExpression()) {
            operands.push_back(operand);
        }

        ExprPtr result = buildExpr(operands.front());
        for (std::size_t i = 1; i < operands.size(); ++i) {
            result = std::make_unique<BinaryExpr>(
                binaryOpFromText(additive->additiveOperator(i - 1)->getText()),
                std::move(result), buildExpr(operands[i]));
        }
        return result;
    }

    if (context->getRuleIndex() == Parser::RuleConditionAdditiveExpression) {
        auto *additive =
            dynamic_cast<Parser::ConditionAdditiveExpressionContext *>(context);
        const auto operands = additive->conditionMultiplicativeExpression();
        const auto operators = additive->additiveOperator();

        ExprPtr result = buildExpr(operands.front());
        for (std::size_t i = 0; i < operators.size(); ++i) {
            result = std::make_unique<BinaryExpr>(
                binaryOpFromText(operators[i]->getText()), std::move(result),
                buildExpr(operands[i + 1]));
        }
        return result;
    }

    if (context->getRuleIndex() == Parser::RuleClosedAdditiveExpression) {
        ExprPtr result;
        std::string pendingOperator;

        for (auto *child : context->children) {
            auto *rule = dynamic_cast<antlr4::ParserRuleContext *>(child);
            if (rule == nullptr) {
                continue;
            }

            if (rule->getRuleIndex() == Parser::RuleAdditiveOperator) {
                pendingOperator = rule->getText();
                continue;
            }

            const auto ruleIndex = rule->getRuleIndex();
            if (ruleIndex == Parser::RuleMultiplicativeExpression ||
                ruleIndex == Parser::RuleClosedMultiplicativeExpression) {
                ExprPtr operand = buildExpr(rule);
                if (!result) {
                    result = std::move(operand);
                } else {
                    if (pendingOperator.empty()) {
                        unsupported(context);
                    }
                    result = std::make_unique<BinaryExpr>(
                        binaryOpFromText(pendingOperator), std::move(result),
                        std::move(operand));
                    pendingOperator.clear();
                }
            }
        }

        if (!result || !pendingOperator.empty()) {
            unsupported(context);
        }
        return result;
    }

    if (context->getRuleIndex() ==
        Parser::RuleConditionClosedAdditiveExpression) {
        ExprPtr result;
        std::string pendingOperator;

        for (auto *child : context->children) {
            auto *rule = dynamic_cast<antlr4::ParserRuleContext *>(child);
            if (rule == nullptr) {
                continue;
            }

            if (rule->getRuleIndex() == Parser::RuleAdditiveOperator) {
                pendingOperator = rule->getText();
                continue;
            }

            const auto ruleIndex = rule->getRuleIndex();
            if (ruleIndex == Parser::RuleConditionMultiplicativeExpression ||
                ruleIndex ==
                    Parser::RuleConditionClosedMultiplicativeExpression) {
                ExprPtr operand = buildExpr(rule);
                if (!result) {
                    result = std::move(operand);
                } else {
                    if (pendingOperator.empty()) {
                        unsupported(context);
                    }
                    result = std::make_unique<BinaryExpr>(
                        binaryOpFromText(pendingOperator), std::move(result),
                        std::move(operand));
                    pendingOperator.clear();
                }
            }
        }

        if (!result || !pendingOperator.empty()) {
            unsupported(context);
        }
        return result;
    }

    if (context->getRuleIndex() ==
        Parser::RuleStatementClosedAdditiveExpression) {
        ExprPtr result;
        std::string pendingOperator;

        for (auto *child : context->children) {
            auto *rule = dynamic_cast<antlr4::ParserRuleContext *>(child);
            if (rule == nullptr) {
                continue;
            }

            if (rule->getRuleIndex() == Parser::RuleAdditiveOperator) {
                pendingOperator = rule->getText();
                continue;
            }

            const auto ruleIndex = rule->getRuleIndex();
            if (ruleIndex ==
                    Parser::RuleStatementClosedMultiplicativeExpression ||
                ruleIndex == Parser::RuleStatementMultiplicativeExpression ||
                ruleIndex == Parser::RuleMultiplicativeExpression ||
                ruleIndex == Parser::RuleClosedMultiplicativeExpression) {
                ExprPtr operand = buildExpr(rule);
                if (!result) {
                    result = std::move(operand);
                } else {
                    if (pendingOperator.empty()) {
                        unsupported(context);
                    }
                    result = std::make_unique<BinaryExpr>(
                        binaryOpFromText(pendingOperator), std::move(result),
                        std::move(operand));
                    pendingOperator.clear();
                }
            }
        }

        if (!result || !pendingOperator.empty()) {
            unsupported(context);
        }
        return result;
    }

    if (context->getRuleIndex() == Parser::RuleClosedMultiplicativeExpression) {
        ExprPtr result;
        std::string pendingOperator;

        for (auto *child : context->children) {
            auto *rule = dynamic_cast<antlr4::ParserRuleContext *>(child);
            if (rule == nullptr) {
                continue;
            }

            if (rule->getRuleIndex() == Parser::RuleMultiplicativeOperator) {
                pendingOperator = rule->getText();
                continue;
            }

            const auto ruleIndex = rule->getRuleIndex();
            if (ruleIndex == Parser::RuleCastExpression ||
                ruleIndex == Parser::RuleClosedCastExpression) {
                ExprPtr operand = buildExpr(rule);
                if (!result) {
                    result = std::move(operand);
                } else {
                    if (pendingOperator.empty()) {
                        unsupported(context);
                    }
                    result = std::make_unique<BinaryExpr>(
                        binaryOpFromText(pendingOperator), std::move(result),
                        std::move(operand));
                    pendingOperator.clear();
                }
            }
        }

        if (!result || !pendingOperator.empty()) {
            unsupported(context);
        }
        return result;
    }

    if (context->getRuleIndex() ==
        Parser::RuleConditionClosedMultiplicativeExpression) {
        ExprPtr result;
        std::string pendingOperator;

        for (auto *child : context->children) {
            auto *rule = dynamic_cast<antlr4::ParserRuleContext *>(child);
            if (rule == nullptr) {
                continue;
            }

            if (rule->getRuleIndex() == Parser::RuleMultiplicativeOperator) {
                pendingOperator = rule->getText();
                continue;
            }

            const auto ruleIndex = rule->getRuleIndex();
            if (ruleIndex == Parser::RuleConditionCastExpression ||
                ruleIndex == Parser::RuleConditionClosedCastExpression) {
                ExprPtr operand = buildExpr(rule);
                if (!result) {
                    result = std::move(operand);
                } else {
                    if (pendingOperator.empty()) {
                        unsupported(context);
                    }
                    result = std::make_unique<BinaryExpr>(
                        binaryOpFromText(pendingOperator), std::move(result),
                        std::move(operand));
                    pendingOperator.clear();
                }
            }
        }

        if (!result || !pendingOperator.empty()) {
            unsupported(context);
        }
        return result;
    }

    if (context->getRuleIndex() ==
        Parser::RuleStatementClosedMultiplicativeExpression) {
        ExprPtr result;
        std::string pendingOperator;

        for (auto *child : context->children) {
            auto *rule = dynamic_cast<antlr4::ParserRuleContext *>(child);
            if (rule == nullptr) {
                continue;
            }

            if (rule->getRuleIndex() == Parser::RuleMultiplicativeOperator) {
                pendingOperator = rule->getText();
                continue;
            }

            const auto ruleIndex = rule->getRuleIndex();
            if (ruleIndex == Parser::RuleStatementClosedCastExpression ||
                ruleIndex == Parser::RuleStatementCastExpression ||
                ruleIndex == Parser::RuleCastExpression ||
                ruleIndex == Parser::RuleClosedCastExpression) {
                ExprPtr operand = buildExpr(rule);
                if (!result) {
                    result = std::move(operand);
                } else {
                    if (pendingOperator.empty()) {
                        unsupported(context);
                    }
                    result = std::make_unique<BinaryExpr>(
                        binaryOpFromText(pendingOperator), std::move(result),
                        std::move(operand));
                    pendingOperator.clear();
                }
            }
        }

        if (!result || !pendingOperator.empty()) {
            unsupported(context);
        }
        return result;
    }

    if (context->getRuleIndex() == Parser::RuleClosedCastExpression) {
        auto *closedCast =
            dynamic_cast<Parser::ClosedCastExpressionContext *>(context);
        if (closedCast->unaryExpression() != nullptr) {
            return buildExpr(closedCast->unaryExpression());
        }
        return std::make_unique<CastExpr>(
            buildExpr(closedCast->castExpression()),
            closedCast->closedCastType()->getText());
    }

    if (context->getRuleIndex() == Parser::RuleConditionClosedCastExpression) {
        auto *closedCast =
            dynamic_cast<Parser::ConditionClosedCastExpressionContext *>(
                context);
        if (closedCast->conditionUnaryExpression() != nullptr) {
            return buildExpr(closedCast->conditionUnaryExpression());
        }
        return std::make_unique<CastExpr>(
            buildExpr(closedCast->conditionCastExpression()),
            closedCast->closedCastType()->getText());
    }

    if (context->getRuleIndex() == Parser::RuleStatementClosedCastExpression) {
        auto *closedCast =
            dynamic_cast<Parser::StatementClosedCastExpressionContext *>(
                context);
        if (closedCast->statementUnaryExpression() != nullptr) {
            return buildExpr(closedCast->statementUnaryExpression());
        }
        if (closedCast->statementCastExpression() == nullptr ||
            closedCast->closedCastType() == nullptr) {
            unsupported(context);
        }
        return std::make_unique<CastExpr>(
            buildExpr(closedCast->statementCastExpression()),
            closedCast->closedCastType()->getText());
    }

    if (context->getRuleIndex() == Parser::RuleShiftExpression ||
        context->getRuleIndex() == Parser::RuleClosedShiftExpression ||
        context->getRuleIndex() == Parser::RuleStatementShiftExpression ||
        context->getRuleIndex() == Parser::RuleStatementClosedShiftExpression ||
        context->getRuleIndex() == Parser::RuleConditionShiftExpression ||
        context->getRuleIndex() == Parser::RuleConditionClosedShiftExpression) {
        ExprPtr result;
        std::string pendingOperator;

        for (auto *child : context->children) {
            if (auto *terminal =
                    dynamic_cast<antlr4::tree::TerminalNode *>(child)) {
                if (terminal->getSymbol()->getType() == Parser::SHL) {
                    pendingOperator = terminal->getText(); // <<
                }
                continue;
            }

            auto *rule = dynamic_cast<antlr4::ParserRuleContext *>(child);
            if (rule == nullptr) {
                continue;
            }

            if (rule->getRuleIndex() == Parser::RuleShiftRight) {
                pendingOperator = rule->getText(); // >>
                continue;
            }

            const auto ruleIndex = rule->getRuleIndex();
            if (ruleIndex == Parser::RuleAdditiveExpression ||
                ruleIndex == Parser::RuleClosedAdditiveExpression ||
                ruleIndex == Parser::RuleStatementAdditiveExpression ||
                ruleIndex == Parser::RuleStatementClosedAdditiveExpression ||
                ruleIndex == Parser::RuleConditionAdditiveExpression ||
                ruleIndex == Parser::RuleConditionClosedAdditiveExpression) {
                ExprPtr operand = buildExpr(rule);

                if (!result) {
                    result = std::move(operand);
                } else {
                    if (pendingOperator.empty()) {
                        unsupported(context);
                    }

                    result = std::make_unique<BinaryExpr>(
                        binaryOpFromText(pendingOperator), std::move(result),
                        std::move(operand));
                    pendingOperator.clear();
                }
            }
        }

        if (!result || !pendingOperator.empty()) {
            unsupported(context);
        }

        return result;
    }

    if (context->getRuleIndex() == Parser::RuleBitAndExpression ||
        context->getRuleIndex() == Parser::RuleClosedBitAndExpression ||
        context->getRuleIndex() == Parser::RuleStatementBitAndExpression ||
        context->getRuleIndex() ==
            Parser::RuleStatementClosedBitAndExpression ||
        context->getRuleIndex() == Parser::RuleConditionBitAndExpression ||
        context->getRuleIndex() ==
            Parser::RuleConditionClosedBitAndExpression) {
        ExprPtr result;
        bool pendingAnd = false;

        for (auto *child : context->children) {
            if (auto *terminal =
                    dynamic_cast<antlr4::tree::TerminalNode *>(child)) {
                if (terminal->getSymbol()->getType() != Parser::AMP ||
                    !result || pendingAnd) {
                    unsupported(context);
                }
                pendingAnd = true;
                continue;
            }

            auto *rule = dynamic_cast<antlr4::ParserRuleContext *>(child);
            if (rule == nullptr) {
                continue;
            }

            const auto ruleIndex = rule->getRuleIndex();
            if (ruleIndex == Parser::RuleShiftExpression ||
                ruleIndex == Parser::RuleClosedShiftExpression ||
                ruleIndex == Parser::RuleStatementShiftExpression ||
                ruleIndex == Parser::RuleStatementClosedShiftExpression ||
                ruleIndex == Parser::RuleConditionShiftExpression ||
                ruleIndex == Parser::RuleConditionClosedShiftExpression) {
                ExprPtr operand = buildExpr(rule);
                if (!result) {
                    result = std::move(operand);
                } else {
                    if (!pendingAnd) {
                        unsupported(context);
                    }
                    result = std::make_unique<BinaryExpr>(BinaryOp::BitwiseAnd,
                                                          std::move(result),
                                                          std::move(operand));
                    pendingAnd = false;
                }
            }
        }

        if (!result || pendingAnd) {
            unsupported(context);
        }
        return result;
    }

    if (context->getRuleIndex() == Parser::RuleBitXorExpression ||
        context->getRuleIndex() == Parser::RuleClosedBitXorExpression ||
        context->getRuleIndex() == Parser::RuleStatementBitXorExpression ||
        context->getRuleIndex() ==
            Parser::RuleStatementClosedBitXorExpression ||
        context->getRuleIndex() == Parser::RuleConditionBitXorExpression ||
        context->getRuleIndex() ==
            Parser::RuleConditionClosedBitXorExpression) {
        ExprPtr result;
        bool pendingXor = false;

        for (auto *child : context->children) {
            if (auto *terminal =
                    dynamic_cast<antlr4::tree::TerminalNode *>(child)) {
                if (terminal->getSymbol()->getType() != Parser::CARET ||
                    !result || pendingXor) {
                    unsupported(context);
                }
                pendingXor = true;
                continue;
            }

            auto *rule = dynamic_cast<antlr4::ParserRuleContext *>(child);
            if (rule == nullptr) {
                continue;
            }

            const auto ruleIndex = rule->getRuleIndex();
            if (ruleIndex == Parser::RuleBitAndExpression ||
                ruleIndex == Parser::RuleClosedBitAndExpression ||
                ruleIndex == Parser::RuleStatementBitAndExpression ||
                ruleIndex == Parser::RuleStatementClosedBitAndExpression ||
                ruleIndex == Parser::RuleConditionBitAndExpression ||
                ruleIndex == Parser::RuleConditionClosedBitAndExpression) {
                ExprPtr operand = buildExpr(rule);
                if (!result) {
                    result = std::move(operand);
                } else {
                    if (!pendingXor) {
                        unsupported(context);
                    }
                    result = std::make_unique<BinaryExpr>(BinaryOp::BitwiseXor,
                                                          std::move(result),
                                                          std::move(operand));
                    pendingXor = false;
                }
            }
        }

        if (!result || pendingXor) {
            unsupported(context);
        }
        return result;
    }

    if (context->getRuleIndex() == Parser::RuleBitOrExpression ||
        context->getRuleIndex() == Parser::RuleClosedBitOrExpression ||
        context->getRuleIndex() == Parser::RuleStatementBitOrExpression ||
        context->getRuleIndex() == Parser::RuleStatementClosedBitOrExpression ||
        context->getRuleIndex() == Parser::RuleConditionBitOrExpression ||
        context->getRuleIndex() == Parser::RuleConditionClosedBitOrExpression) {
        ExprPtr result;
        bool pendingOr = false;

        for (auto *child : context->children) {
            if (auto *terminal =
                    dynamic_cast<antlr4::tree::TerminalNode *>(child)) {
                if (terminal->getSymbol()->getType() != Parser::PIPE ||
                    !result || pendingOr) {
                    unsupported(context);
                }
                pendingOr = true;
                continue;
            }

            auto *rule = dynamic_cast<antlr4::ParserRuleContext *>(child);
            if (rule == nullptr) {
                continue;
            }

            const auto ruleIndex = rule->getRuleIndex();
            if (ruleIndex == Parser::RuleBitXorExpression ||
                ruleIndex == Parser::RuleClosedBitXorExpression ||
                ruleIndex == Parser::RuleStatementBitXorExpression ||
                ruleIndex == Parser::RuleStatementClosedBitXorExpression ||
                ruleIndex == Parser::RuleConditionBitXorExpression ||
                ruleIndex == Parser::RuleConditionClosedBitXorExpression) {
                ExprPtr operand = buildExpr(rule);
                if (!result) {
                    result = std::move(operand);
                } else {
                    if (!pendingOr) {
                        unsupported(context);
                    }
                    result = std::make_unique<BinaryExpr>(BinaryOp::BitwiseOr,
                                                          std::move(result),
                                                          std::move(operand));
                    pendingOr = false;
                }
            }
        }

        if (!result || pendingOr) {
            unsupported(context);
        }
        return result;
    }

    if (context->getRuleIndex() == Parser::RuleComparisonExpression) {
        auto *comparison =
            dynamic_cast<Parser::ComparisonExpressionContext *>(context);
        if (comparison->comparisonExceptLt() != nullptr) {
            const auto op =
                binaryOpFromText(comparison->comparisonExceptLt()->getText());
            return std::make_unique<BinaryExpr>(
                op, buildExpr(comparison->bitOrExpression(0)),
                buildExpr(comparison->bitOrExpression(1)));
        } else if (comparison->LT() != nullptr) {
            const auto op = binaryOpFromText(comparison->LT()->getText());
            return std::make_unique<BinaryExpr>(
                op, buildExpr(comparison->closedBitOrExpression()),
                buildExpr(comparison->bitOrExpression(0)));
        } else {
            return buildExpr(comparison->bitOrExpression(0));
        }
    }

    if (context->getRuleIndex() == Parser::RuleStatementComparisonExpression) {
        auto *comparison =
            dynamic_cast<Parser::StatementComparisonExpressionContext *>(
                context);
        if (comparison->comparisonExceptLt() != nullptr) {
            const auto op =
                binaryOpFromText(comparison->comparisonExceptLt()->getText());
            return std::make_unique<BinaryExpr>(
                op, buildExpr(comparison->statementBitOrExpression()),
                buildExpr(comparison->bitOrExpression()));
        } else if (comparison->LT() != nullptr) {
            const auto op = binaryOpFromText(comparison->LT()->getText());
            return std::make_unique<BinaryExpr>(
                op, buildExpr(comparison->statementClosedBitOrExpression()),
                buildExpr(comparison->bitOrExpression()));
        } else {
            return buildExpr(comparison->statementBitOrExpression());
        }
    }

    if (context->getRuleIndex() == Parser::RuleConditionComparisonExpression) {
        auto *comparison =
            dynamic_cast<Parser::ConditionComparisonExpressionContext *>(
                context);
        if (comparison->comparisonExceptLt() != nullptr) {
            const auto op =
                binaryOpFromText(comparison->comparisonExceptLt()->getText());
            const auto operands = comparison->conditionBitOrExpression();
            if (operands.size() != 2) {
                unsupported(context);
            }
            return std::make_unique<BinaryExpr>(op, buildExpr(operands[0]),
                                                buildExpr(operands[1]));
        }
        if (comparison->LT() != nullptr) {
            if (comparison->conditionBitOrExpression().size() != 1 ||
                comparison->conditionClosedBitOrExpression() == nullptr) {
                unsupported(context);
            }
            return std::make_unique<BinaryExpr>(
                BinaryOp::Less,
                buildExpr(comparison->conditionClosedBitOrExpression()),
                buildExpr(comparison->conditionBitOrExpression(0)));
        }

        const auto operands = comparison->conditionBitOrExpression();
        if (operands.size() != 1) {
            unsupported(context);
        }
        return buildExpr(operands[0]);
    }

    const auto ruleIndex = context->getRuleIndex();
    if (ruleIndex == Parser::RuleLogicalAndExpression ||
        ruleIndex == Parser::RuleStatementLogicalAndExpression ||
        ruleIndex == Parser::RuleConditionLogicalAndExpression) {
        std::vector<antlr4::ParserRuleContext *> operands;
        bool pendingAnd = false;

        for (auto *child : context->children) {
            if (auto *terminal =
                    dynamic_cast<antlr4::tree::TerminalNode *>(child)) {
                if (terminal->getSymbol()->getType() != Parser::ANDAND ||
                    operands.empty() || pendingAnd) {
                    unsupported(context);
                }
                pendingAnd = true;
                continue;
            }
            auto *rule = dynamic_cast<antlr4::ParserRuleContext *>(child);
            if (rule == nullptr) {
                continue;
            }

            const auto childRuleIndex = rule->getRuleIndex();
            if (childRuleIndex == Parser::RuleComparisonExpression ||
                childRuleIndex == Parser::RuleStatementComparisonExpression ||
                childRuleIndex == Parser::RuleConditionComparisonExpression) {
                if (!operands.empty() && !pendingAnd) {
                    unsupported(context);
                }
                operands.push_back(rule);
                pendingAnd = false;
            }
        }

        if (operands.empty() || pendingAnd) {
            unsupported(context);
        }

        if (operands.size() == 1) {
            return buildExpr(operands.front());
        }

        ExprPtr result = buildExpr(operands.front());
        for (std::size_t i = 1; i < operands.size(); ++i) {
            result = std::make_unique<BinaryExpr>(BinaryOp::LogicalAnd,
                                                  std::move(result),
                                                  buildExpr(operands[i]));
        }
        return result;
    }

    if (ruleIndex == Parser::RuleLogicalOrExpression ||
        ruleIndex == Parser::RuleStatementLogicalOrExpression ||
        ruleIndex == Parser::RuleConditionLogicalOrExpression) {
        std::vector<antlr4::ParserRuleContext *> operands;
        bool pendingOr = false;

        for (auto *child : context->children) {
            if (auto *terminal =
                    dynamic_cast<antlr4::tree::TerminalNode *>(child)) {
                if (terminal->getSymbol()->getType() != Parser::OROR ||
                    operands.empty() || pendingOr) {
                    unsupported(context);
                }
                pendingOr = true;
                continue;
            }

            auto *rule = dynamic_cast<antlr4::ParserRuleContext *>(child);
            if (rule == nullptr) {
                continue;
            }

            const auto childRuleIndex = rule->getRuleIndex();
            if (childRuleIndex == Parser::RuleLogicalAndExpression ||
                childRuleIndex == Parser::RuleStatementLogicalAndExpression ||
                childRuleIndex == Parser::RuleConditionLogicalAndExpression) {
                if (!operands.empty() && !pendingOr) {
                    unsupported(context);
                }
                operands.push_back(rule);
                pendingOr = false;
            }
        }

        if (operands.empty() || pendingOr) {
            unsupported(context);
        }

        if (operands.size() == 1) {
            return buildExpr(operands.front());
        }

        ExprPtr result = buildExpr(operands.front());
        for (std::size_t i = 1; i < operands.size(); ++i) {
            result = std::make_unique<BinaryExpr>(
                BinaryOp::LogicalOr, std::move(result), buildExpr(operands[i]));
        }
        return result;
    }

    if (context->getRuleIndex() == Parser::RuleAssignmentExpression) {
        auto *assignment =
            dynamic_cast<Parser::AssignmentExpressionContext *>(context);
        if (assignment->assignmentOperator() != nullptr) {
            const auto op =
                binaryOpFromText(assignment->assignmentOperator()->getText());
            return std::make_unique<BinaryExpr>(
                op, buildExpr(assignment->logicalOrExpression()),
                buildExpr(assignment->expression()));
        } else {
            return buildExpr(assignment->logicalOrExpression());
        }
    }

    if (context->getRuleIndex() == Parser::RuleStatementAssignmentExpression) {
        auto *assignment =
            dynamic_cast<Parser::StatementAssignmentExpressionContext *>(
                context);
        if (assignment->assignmentOperator() != nullptr) {
            const auto op =
                binaryOpFromText(assignment->assignmentOperator()->getText());
            return std::make_unique<BinaryExpr>(
                op, buildExpr(assignment->statementLogicalOrExpression()),
                buildExpr(assignment->expression()));
        } else {
            return buildExpr(assignment->statementLogicalOrExpression());
        }
    }

    if (context->getRuleIndex() == Parser::RuleConditionExpression) {
        auto *condition =
            dynamic_cast<Parser::ConditionExpressionContext *>(context);
        return buildExpr(condition->conditionAssignmentExpression());
    }

    if (context->getRuleIndex() == Parser::RuleConditionAssignmentExpression) {
        auto *assignment =
            dynamic_cast<Parser::ConditionAssignmentExpressionContext *>(
                context);
        ExprPtr left = buildExpr(assignment->conditionLogicalOrExpression());
        if (assignment->assignmentOperator() == nullptr) {
            return left;
        }

        const auto op =
            binaryOpFromText(assignment->assignmentOperator()->getText());
        return std::make_unique<BinaryExpr>(
            op, std::move(left), buildExpr(assignment->conditionExpression()));
    }

    if (context->getRuleIndex() == Parser::RuleArrayExpression) {
        auto *array = dynamic_cast<Parser::ArrayExpressionContext *>(context);
        std::vector<ExprPtr> elements;
        for (auto *element : array->expression()) {
            elements.push_back(buildExpr(element));
        }

        std::optional<std::string> repeatCount;
        if (array->SEMI() != nullptr) {
            if (array->constValue() == nullptr || elements.size() != 1) {
                unsupported(context);
            }
            repeatCount = array->constValue()->getText();
        }
        return std::make_unique<ArrayExpr>(std::move(elements),
                                           std::move(repeatCount));
    }

    if (context->getRuleIndex() == Parser::RuleIfExpression) {
        auto *ifContext = dynamic_cast<Parser::IfExpressionContext *>(context);
        const auto blocks = ifContext->blockExpression();
        if (ifContext->conditionExpression() == nullptr || blocks.empty() ||
            blocks.size() > 2) {
            unsupported(context);
        }

        ExprPtr condition = buildExpr(ifContext->conditionExpression());
        BlockExpr thenBranch = buildBlock(blocks[0]);
        ExprPtr elseBranch;
        if (ifContext->ifExpression() != nullptr) {
            if (blocks.size() != 1) {
                unsupported(context);
            }
            elseBranch = buildExpr(ifContext->ifExpression());
        } else if (blocks.size() == 2) {
            elseBranch = buildExpr(blocks[1]);
        } else if (ifContext->ELSE() != nullptr) {
            unsupported(context);
        }

        return std::make_unique<IfExpr>(
            std::move(condition), std::move(thenBranch), std::move(elseBranch));
    }

    const auto children = ruleChildren(context);
    if (children.size() == 1) {
        return buildExpr(children.front());
    }

    unsupported(context);

    return std::make_unique<UnitExpr>();
}

} // namespace rx::ast
