#pragma once

#include "Parser.h"
#include "ast.hpp"

namespace rx::ast {

class AstBuilder {
  public:
    Program build(Parser::CrateContext *crate) const;

  private:
    Function   buildFunction(Parser::FunctionDefinitionContext *context) const;
    StructItem buildStruct(Parser::StructDefinitionContext *context) const;
    GenericParam
    buildGenericParams(Parser::GenericParamsContext *context) const;
    WhereClause
    buildWhereClause(Parser::WhereClauseContext *context) const;
    ConstantItem
    buildConstant(Parser::ConstantItemContext *context) const;
    ConstValue
    buildConstValue(Parser::ConstValueContext *context) const;
    ConstValue
    buildMagnitude(Parser::MagnitudeContext *context) const;
    BlockExpr  buildBlock(Parser::BlockExpressionContext *context) const;
    StmtPtr    buildStmt(Parser::StatementContext *context) const;
    ExprPtr    buildExpression(antlr4::ParserRuleContext *context) const;
};

} // namespace rx::ast
