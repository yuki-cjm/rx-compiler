#pragma once

#include "Parser.h"
#include "ast.hpp"

namespace rx::ast {

class AstBuilder {
  public:
    Program build(Parser::CrateContext *crate) const;

  private:
    ItemPtr buildItem(Parser::ItemContext *context) const;
    StmtPtr buildStmt(Parser::StatementContext *context) const;
    ExprPtr buildExpr(antlr4::ParserRuleContext *context) const;
};

} // namespace rx::ast
