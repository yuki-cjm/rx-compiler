#pragma once

#include "Parser.h"
#include "ast.hpp"

namespace rx::ast {

class AstBuilder {
  public:
    Program build(Parser::CrateContext *crate) const;

  private:
    Function  buildFunction(Parser::FunctionDefinitionContext *context) const;
    BlockExpr buildBlock(Parser::BlockExpressionContext *context) const;
    StmtPtr   buildStmt(Parser::StatementContext *context) const;
    ExprPtr   buildExpression(antlr4::ParserRuleContext *context) const;
};

} // namespace rx::ast
