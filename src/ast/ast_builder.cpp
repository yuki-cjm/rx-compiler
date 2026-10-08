#include "ast_builder.hpp"
#include "ast.hpp"

#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace rx::ast {
namespace {

[[noreturn]] void unsupported(antlr4::ParserRuleContext *context) {
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

} // namespace

Program AstBuilder::build(Parser::CrateContext *crate) const {
    Program program;
    for (auto *item : crate->item()) {
        auto *function = item->functionDefinition();
        if (function == nullptr) {
            unsupported(item);
        }
        program.functions.push_back(buildFunction(function));
    }
    return program;
}

Function
AstBuilder::buildFunction(Parser::FunctionDefinitionContext *context) const {
    Function function;
    function.name = context->identifier()->getText();

    if (auto *returnType = context->typeRef()) {
        function.returnType = returnType->getText();
    }

    if (auto *parameters = context->functionParameters()) {
        if (parameters->selfParam() != nullptr) {
            unsupported(parameters->selfParam());
        }
        for (auto *parameter : parameters->functionParam()) {
            function.parameters.push_back(
                {parameter->identifierBinding()->getText(),
                 parameter->typeRef()->getText()});
        }
    }

    function.body = buildBlock(context->blockExpression());
    return function;
}

BlockExpr
AstBuilder::buildBlock(Parser::BlockExpressionContext *context) const {
    std::vector<StmtPtr> statements;
    for (auto *statement : context->statement()) {
        statements.push_back(buildStmt(statement));
    }

    ExprPtr tail;
    if (auto *tailContext = context->statementExpression()) {
        tail = buildExpression(tailContext);
    }
    return BlockExpr(std::move(statements), std::move(tail));
}

StmtPtr AstBuilder::buildStmt(Parser::StatementContext *context) const {
    if (auto *let = context->letStatement()) {
        auto statement = std::make_unique<LetStmt>();
        statement->name = let->identifierBinding()->getText();
        if (auto *type = let->typeRef()) {
            statement->type = type->getText();
        }
        statement->initializer = buildExpression(let->expression());
        return statement;
    }

    if (auto *expressionWithBlock = context->expressionWithBlock()) {
        ExprPtr expression;
        if (expressionWithBlock->blockExpression() != nullptr) {
            expression =
                buildExpression(expressionWithBlock->blockExpression());
        } else if (expressionWithBlock->ifExpression() != nullptr) {
            expression = buildExpression(expressionWithBlock->ifExpression());
        } else {
            unsupported(expressionWithBlock);
        }
        return std::make_unique<ExprStmt>(std::move(expression));
    }

    if (auto *expression = context->statementExpression()) {
        return std::make_unique<ExprStmt>(buildExpression(expression));
    }

    if (context->SEMI() != nullptr) {
        return std::make_unique<EmptyStmt>();
    }

    unsupported(context);
}

ExprPtr AstBuilder::buildExpression(antlr4::ParserRuleContext *context) const {
    const auto buildUnary = [this](antlr4::ParserRuleContext *operatorContext,
                                   antlr4::ParserRuleContext *operandContext) {
        if (operatorContext != nullptr) {
            return ExprPtr(std::make_unique<UnaryExpr>(
                unaryOpFromText(operatorContext->getText()),
                buildExpression(operandContext)));
        }
        return buildExpression(operandContext);
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
                builtArguments.push_back(buildExpression(argument));
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
                        builtArguments.push_back(buildExpression(argument));
                    }
                    result = std::make_unique<CallExpr>(
                        std::move(result), std::move(builtArguments));
                } else if (suffix->LBRACKET() != nullptr) {
                    if (suffix->expression() == nullptr) {
                        unsupported(suffix);
                    }
                    result = std::make_unique<IndexExpr>(
                        std::move(result),
                        buildExpression(suffix->expression()));
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
        return applyPostfixSuffixes(
            buildExpression(postfix->primaryExpression()),
            postfix->postfixSuffix());
    }

    if (context->getRuleIndex() == Parser::RuleConditionPostfixExpression) {
        auto *postfix =
            dynamic_cast<Parser::ConditionPostfixExpressionContext *>(context);
        return applyPostfixSuffixes(
            buildExpression(postfix->conditionPrimary()),
            postfix->postfixSuffix());
    }

    if (context->getRuleIndex() ==
        Parser::RuleConditionBreakPostfixExpression) {
        auto *postfix =
            dynamic_cast<Parser::ConditionBreakPostfixExpressionContext *>(
                context);
        return applyPostfixSuffixes(
            buildExpression(postfix->conditionPrimaryWithoutBareBlock()),
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
            ExprPtr result;
            if (expressionWithBlock->blockExpression() != nullptr) {
                result =
                    buildExpression(expressionWithBlock->blockExpression());
            } else if (expressionWithBlock->ifExpression() != nullptr) {
                result = buildExpression(expressionWithBlock->ifExpression());
            } else {
                unsupported(expressionWithBlock);
            }
            result = applyDotSuffix(std::move(result), postfix->dotSuffix());
            return applyPostfixSuffixes(std::move(result),
                                        postfix->postfixSuffix());
        }
        return applyPostfixSuffixes(buildExpression(postfix->nonBlockPrimary()),
                                    postfix->postfixSuffix());
    }

    if (context->getRuleIndex() == Parser::RulePrimaryExpression) {
        auto *primary =
            dynamic_cast<Parser::PrimaryExpressionContext *>(context);
        if (primary->expressionWithBlock() != nullptr) {
            auto *expressionWithBlock = primary->expressionWithBlock();
            if (expressionWithBlock->blockExpression() != nullptr) {
                return buildExpression(expressionWithBlock->blockExpression());
            }
            if (expressionWithBlock->ifExpression() != nullptr) {
                return buildExpression(expressionWithBlock->ifExpression());
            }
            unsupported(expressionWithBlock);
        }
        return buildExpression(primary->nonBlockPrimary());
    }

    if (context->getRuleIndex() == Parser::RuleConditionPrimary) {
        auto *primary =
            dynamic_cast<Parser::ConditionPrimaryContext *>(context);
        if (primary->blockExpression() != nullptr) {
            return buildExpression(primary->blockExpression());
        }
        return buildExpression(primary->conditionPrimaryWithoutBareBlock());
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
        context->getRuleIndex() == Parser::RuleConditionCastExpression) {
        ExprPtr result;
        std::vector<Parser::TypeRefContext *> targetTypes;

        if (context->getRuleIndex() == Parser::RuleCastExpression) {
            auto *cast = dynamic_cast<Parser::CastExpressionContext *>(context);
            result = buildExpression(cast->unaryExpression());
            targetTypes = cast->typeRef();
        } else {
            auto *cast =
                dynamic_cast<Parser::ConditionCastExpressionContext *>(context);
            result = buildExpression(cast->conditionUnaryExpression());
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

        ExprPtr result = buildExpression(operands.front());

        for (std::size_t i = 0; i < operators.size(); i++) {
            result = std::make_unique<BinaryExpr>(
                binaryOpFromText(operators[i]->getText()), std::move(result),
                buildExpression(operands[i + 1]));
        }

        return result;
    }

    if (context->getRuleIndex() ==
        Parser::RuleStatementMultiplicativeExpression) {
        auto *multiplicative =
            dynamic_cast<Parser::StatementMultiplicativeExpressionContext *>(
                context);
        const auto operators = multiplicative->multiplicativeOperator();

        ExprPtr result =
            buildExpression(multiplicative->statementCastExpression());
        const auto operands = multiplicative->castExpression();

        for (std::size_t i = 0; i < operators.size(); ++i) {
            result = std::make_unique<BinaryExpr>(
                binaryOpFromText(operators[i]->getText()), std::move(result),
                buildExpression(operands[i]));
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

        ExprPtr result = buildExpression(operands.front());
        for (std::size_t i = 0; i < operators.size(); ++i) {
            result = std::make_unique<BinaryExpr>(
                binaryOpFromText(operators[i]->getText()), std::move(result),
                buildExpression(operands[i + 1]));
        }
        return result;
    }

    if (context->getRuleIndex() == Parser::RuleNonBlockPrimary ||
        context->getRuleIndex() ==
            Parser::RuleConditionPrimaryWithoutBareBlock) {
        auto *primary = dynamic_cast<Parser::NonBlockPrimaryContext *>(context);
        if (primary != nullptr && primary->LPAREN() != nullptr) {
            if (primary->expression() == nullptr) {
                return std::make_unique<UnitExpr>();
            }
            return buildExpression(primary->expression());
        }
        if (primary != nullptr &&
            (primary->LBRACE() != nullptr || primary->BREAK() != nullptr ||
             primary->RETURN() != nullptr || primary->CONTINUE() != nullptr)) {
            unsupported(context);
        }

        auto *conditionPrimary =
            dynamic_cast<Parser::ConditionPrimaryWithoutBareBlockContext *>(
                context);
        if (conditionPrimary != nullptr &&
            conditionPrimary->LPAREN() != nullptr) {
            if (conditionPrimary->expression() == nullptr) {
                return std::make_unique<UnitExpr>();
            }
            return buildExpression(conditionPrimary->expression());
        }
        if (conditionPrimary != nullptr &&
            (conditionPrimary->ifExpression() != nullptr ||
             conditionPrimary->LOOP() != nullptr ||
             conditionPrimary->WHILE() != nullptr ||
             conditionPrimary->BREAK() != nullptr ||
             conditionPrimary->RETURN() != nullptr ||
             conditionPrimary->CONTINUE() != nullptr)) {
            if (conditionPrimary->ifExpression() != nullptr) {
                return buildExpression(conditionPrimary->ifExpression());
            }
            unsupported(context);
        }
    }

    if (context->getRuleIndex() == Parser::RulePathInExpression) {
        auto *path = dynamic_cast<Parser::PathInExpressionContext *>(context);

        std::vector<std::string> segments;
        for (auto *segment : path->pathExprSegment()) {
            segments.push_back(segment->getText());
        }
        return std::make_unique<PathExpr>(std::move(segments));
    }

    if (context->getRuleIndex() == Parser::RuleAdditiveExpression) {
        auto *additive =
            dynamic_cast<Parser::AdditiveExpressionContext *>(context);
        const auto operands = additive->multiplicativeExpression();
        ExprPtr result = buildExpression(operands.front());
        for (std::size_t i = 1; i < operands.size(); ++i) {
            result = std::make_unique<BinaryExpr>(
                binaryOpFromText(additive->additiveOperator(i - 1)->getText()),
                std::move(result), buildExpression(operands[i]));
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

        ExprPtr result = buildExpression(operands.front());
        for (std::size_t i = 1; i < operands.size(); ++i) {
            result = std::make_unique<BinaryExpr>(
                binaryOpFromText(additive->additiveOperator(i - 1)->getText()),
                std::move(result), buildExpression(operands[i]));
        }
        return result;
    }

    if (context->getRuleIndex() == Parser::RuleConditionAdditiveExpression) {
        auto *additive =
            dynamic_cast<Parser::ConditionAdditiveExpressionContext *>(context);
        const auto operands = additive->conditionMultiplicativeExpression();
        const auto operators = additive->additiveOperator();

        ExprPtr result = buildExpression(operands.front());
        for (std::size_t i = 0; i < operators.size(); ++i) {
            result = std::make_unique<BinaryExpr>(
                binaryOpFromText(operators[i]->getText()), std::move(result),
                buildExpression(operands[i + 1]));
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
                ExprPtr operand = buildExpression(rule);
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
                ExprPtr operand = buildExpression(rule);
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
                ExprPtr operand = buildExpression(rule);
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
                ExprPtr operand = buildExpression(rule);
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
                ExprPtr operand = buildExpression(rule);
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
                ExprPtr operand = buildExpression(rule);
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
            return buildExpression(closedCast->unaryExpression());
        }
        return std::make_unique<CastExpr>(
            buildExpression(closedCast->castExpression()),
            closedCast->closedCastType()->getText());
    }

    if (context->getRuleIndex() == Parser::RuleConditionClosedCastExpression) {
        auto *closedCast =
            dynamic_cast<Parser::ConditionClosedCastExpressionContext *>(
                context);
        if (closedCast->conditionUnaryExpression() != nullptr) {
            return buildExpression(closedCast->conditionUnaryExpression());
        }
        return std::make_unique<CastExpr>(
            buildExpression(closedCast->conditionCastExpression()),
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
                ExprPtr operand = buildExpression(rule);

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
                ExprPtr operand = buildExpression(rule);
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
                ExprPtr operand = buildExpression(rule);
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
                ExprPtr operand = buildExpression(rule);
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
                op, buildExpression(comparison->bitOrExpression(0)),
                buildExpression(comparison->bitOrExpression(1)));
        } else if (comparison->LT() != nullptr) {
            const auto op = binaryOpFromText(comparison->LT()->getText());
            return std::make_unique<BinaryExpr>(
                op, buildExpression(comparison->closedBitOrExpression()),
                buildExpression(comparison->bitOrExpression(0)));
        } else {
            return buildExpression(comparison->bitOrExpression(0));
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
                op, buildExpression(comparison->statementBitOrExpression()),
                buildExpression(comparison->bitOrExpression()));
        } else if (comparison->LT() != nullptr) {
            const auto op = binaryOpFromText(comparison->LT()->getText());
            return std::make_unique<BinaryExpr>(
                op,
                buildExpression(comparison->statementClosedBitOrExpression()),
                buildExpression(comparison->bitOrExpression()));
        } else {
            return buildExpression(comparison->statementBitOrExpression());
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
            return std::make_unique<BinaryExpr>(
                op, buildExpression(operands[0]), buildExpression(operands[1]));
        }
        if (comparison->LT() != nullptr) {
            if (comparison->conditionBitOrExpression().size() != 1 ||
                comparison->conditionClosedBitOrExpression() == nullptr) {
                unsupported(context);
            }
            return std::make_unique<BinaryExpr>(
                BinaryOp::Less,
                buildExpression(comparison->conditionClosedBitOrExpression()),
                buildExpression(comparison->conditionBitOrExpression(0)));
        }

        const auto operands = comparison->conditionBitOrExpression();
        if (operands.size() != 1) {
            unsupported(context);
        }
        return buildExpression(operands[0]);
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
            return buildExpression(operands.front());
        }

        ExprPtr result = buildExpression(operands.front());
        for (std::size_t i = 1; i < operands.size(); ++i) {
            result = std::make_unique<BinaryExpr>(BinaryOp::LogicalAnd,
                                                  std::move(result),
                                                  buildExpression(operands[i]));
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
            return buildExpression(operands.front());
        }

        ExprPtr result = buildExpression(operands.front());
        for (std::size_t i = 1; i < operands.size(); ++i) {
            result = std::make_unique<BinaryExpr>(BinaryOp::LogicalOr,
                                                  std::move(result),
                                                  buildExpression(operands[i]));
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
                op, buildExpression(assignment->logicalOrExpression()),
                buildExpression(assignment->expression()));
        } else {
            return buildExpression(assignment->logicalOrExpression());
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
                op, buildExpression(assignment->statementLogicalOrExpression()),
                buildExpression(assignment->expression()));
        } else {
            return buildExpression(assignment->statementLogicalOrExpression());
        }
    }

    if (context->getRuleIndex() == Parser::RuleConditionExpression) {
        auto *condition =
            dynamic_cast<Parser::ConditionExpressionContext *>(context);
        return buildExpression(condition->conditionAssignmentExpression());
    }

    if (context->getRuleIndex() == Parser::RuleConditionAssignmentExpression) {
        auto *assignment =
            dynamic_cast<Parser::ConditionAssignmentExpressionContext *>(
                context);
        ExprPtr left =
            buildExpression(assignment->conditionLogicalOrExpression());
        if (assignment->assignmentOperator() == nullptr) {
            return left;
        }

        const auto op =
            binaryOpFromText(assignment->assignmentOperator()->getText());
        return std::make_unique<BinaryExpr>(
            op, std::move(left),
            buildExpression(assignment->conditionExpression()));
    }

    if (context->getRuleIndex() == Parser::RuleArrayExpression) {
        auto *array = dynamic_cast<Parser::ArrayExpressionContext *>(context);
        std::vector<ExprPtr> elements;
        for (auto *element : array->expression()) {
            elements.push_back(buildExpression(element));
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

        ExprPtr condition = buildExpression(ifContext->conditionExpression());
        BlockExpr thenBranch = buildBlock(blocks[0]);
        ExprPtr elseBranch;
        if (ifContext->ifExpression() != nullptr) {
            if (blocks.size() != 1) {
                unsupported(context);
            }
            elseBranch = buildExpression(ifContext->ifExpression());
        } else if (blocks.size() == 2) {
            elseBranch = buildExpression(blocks[1]);
        } else if (ifContext->ELSE() != nullptr) {
            unsupported(context);
        }

        return std::make_unique<IfExpr>(
            std::move(condition), std::move(thenBranch), std::move(elseBranch));
    }

    const auto children = ruleChildren(context);
    if (children.size() == 1) {
        return buildExpression(children.front());
    }

    unsupported(context);

    return std::make_unique<UnitExpr>();
}

} // namespace rx::ast
