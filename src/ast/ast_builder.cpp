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

Block AstBuilder::buildBlock(Parser::BlockExpressionContext *context) const {
    Block block;
    for (auto *statement : context->statement()) {
        auto *letStatement = statement->letStatement();
        if (letStatement == nullptr) {
            unsupported(statement);
        }
        block.statements.push_back(buildLet(letStatement));
    }

    if (auto *tail = context->statementExpression()) {
        block.tail = buildExpression(tail);
    }
    return block;
}

LetStatement AstBuilder::buildLet(Parser::LetStatementContext *context) const {
    LetStatement statement;
    statement.name = context->identifierBinding()->getText();
    if (auto *type = context->typeRef()) {
        statement.type = type->getText();
    }
    statement.initializer = buildExpression(context->expression());
    return statement;
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
                binaryOpFromText(
                    additive->additiveOperator(i - 1)->getText()),
                std::move(result),
                buildExpression(operands[i]));
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
                binaryOpFromText(
                    additive->additiveOperator(i - 1)->getText()),
                std::move(result),
                buildExpression(operands[i]));
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
        unsupported(context);
    }

    if (context->getRuleIndex() == Parser::RuleShiftExpression ||
        context->getRuleIndex() == Parser::RuleClosedShiftExpression ||
        context->getRuleIndex() == Parser::RuleStatementShiftExpression ||
        context->getRuleIndex() == Parser::RuleStatementClosedShiftExpression) {
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

            if (rule->getRuleIndex() == Parser::RuleAdditiveExpression ||
                rule->getRuleIndex() == Parser::RuleClosedAdditiveExpression ||
                rule->getRuleIndex() ==
                    Parser::RuleStatementAdditiveExpression ||
                rule->getRuleIndex() ==
                    Parser::RuleStatementClosedAdditiveExpression) {
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
            Parser::RuleStatementClosedBitAndExpression) {
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
                ruleIndex == Parser::RuleStatementClosedShiftExpression) {
                ExprPtr operand = buildExpression(rule);
                if (!result) {
                    result = std::move(operand);
                } else {
                    if (!pendingAnd) {
                        unsupported(context);
                    }
                    result = std::make_unique<BinaryExpr>(
                        BinaryOp::BitwiseAnd, std::move(result),
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
            Parser::RuleStatementClosedBitXorExpression) {
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
                ruleIndex == Parser::RuleStatementClosedBitAndExpression) {
                ExprPtr operand = buildExpression(rule);
                if (!result) {
                    result = std::move(operand);
                } else {
                    if (!pendingXor) {
                        unsupported(context);
                    }
                    result = std::make_unique<BinaryExpr>(
                        BinaryOp::BitwiseXor, std::move(result),
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
        context->getRuleIndex() == Parser::RuleStatementClosedBitOrExpression) {
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
                ruleIndex == Parser::RuleStatementClosedBitXorExpression) {
                ExprPtr operand = buildExpression(rule);
                if (!result) {
                    result = std::move(operand);
                } else {
                    if (!pendingOr) {
                        unsupported(context);
                    }
                    result = std::make_unique<BinaryExpr>(
                        BinaryOp::BitwiseOr, std::move(result),
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
            const auto op = binaryOpFromText(
                comparison->comparisonExceptLt()->getText());
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
            const auto op = binaryOpFromText(
                comparison->comparisonExceptLt()->getText());
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

    const auto ruleIndex = context->getRuleIndex();
    if (ruleIndex == Parser::RuleLogicalAndExpression ||
        ruleIndex == Parser::RuleStatementLogicalAndExpression) {
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
                childRuleIndex == Parser::RuleStatementComparisonExpression) {
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
            result = std::make_unique<BinaryExpr>(
                BinaryOp::LogicalAnd, std::move(result),
                buildExpression(operands[i]));
        }
        return result;
    }

    const auto children = ruleChildren(context);
    if (children.size() == 1) {
        return buildExpression(children.front());
    }

    unsupported(context);

    return std::make_unique<UnitExpr>();
}

} // namespace rx::ast
