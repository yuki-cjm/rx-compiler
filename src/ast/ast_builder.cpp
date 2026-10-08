#include "ast_builder.hpp"
#include "ast.hpp"

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
                operatorContext->getText(), buildExpression(operandContext)));
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
        auto *multiplicative = dynamic_cast<Parser::MultiplicativeExpressionContext *>(context);
        const auto operands = multiplicative->castExpression();
        const auto operators = multiplicative->multiplicativeOperator();

        ExprPtr result = buildExpression(operands.front());

        for (std::size_t i = 0; i < operators.size(); i++) {
            result = std::make_unique<BinaryExpr> (
                operators[i]->getText(),
                std::move(result),
                buildExpression(operands[i + 1])
            );
        }

        return result;
    }

    if (context->getRuleIndex() == Parser::RuleStatementMultiplicativeExpression) {
        auto *multiplicative = dynamic_cast<Parser::StatementMultiplicativeExpressionContext *>(context);
        const auto operators = multiplicative->multiplicativeOperator();

        ExprPtr result = buildExpression(multiplicative->statementCastExpression());
        const auto operands = multiplicative->castExpression();

        for (std::size_t i = 0; i < operators.size(); ++i) {
            result = std::make_unique<BinaryExpr>(
                operators[i]->getText(), std::move(result),
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
        auto *path = dynamic_cast<Parser::PathInExpressionContext*>(context);

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
                additive->additiveOperator(i - 1)->getText(), std::move(result),
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
                additive->additiveOperator(i - 1)->getText(), std::move(result),
                buildExpression(operands[i]));
        }
        return result;
    }

    const auto children = ruleChildren(context);
    if (children.size() == 1) {
        return buildExpression(children.front());
    }

    unsupported(context);
}

} // namespace rx::ast
