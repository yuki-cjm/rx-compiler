#pragma once

#include "ast.hpp"
#include "Parser.h"

namespace rx::ast {

class AstBuilder {
public:
    Program build(Parser::CrateContext *crate) const;

private:
    Function buildFunction(Parser::FunctionDefinitionContext *context) const;
    Block buildBlock(Parser::BlockExpressionContext *context) const;
    LetStatement buildLet(Parser::LetStatementContext *context) const;
    ExprPtr buildExpression(antlr4::ParserRuleContext *context) const;
};

} // namespace rx::ast
