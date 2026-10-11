#include "ast_printer.hpp"

namespace rx::ast {

void ASTPrinter::printProgram(Program &program) {
    out_ << "Program\n";
    for (auto &item : program.items) {
        indent_ = 1;
        dispatch(item.get());
    }
}

void ASTPrinter::dispatch(ASTNode *node) { node->accept(*this); }

std::any ASTPrinter::visitCrate(Crate *node) {
    const std::size_t indent = indent_;
    printIndent(indent);
    out_ << "Crate\n";
    for (auto &item : node->items()) {
        indent_ = indent + 1;
        dispatch(item.get());
    }
    return {};
}

std::any ASTPrinter::visitFunctionItem(FunctionItem *node) {
    const std::size_t indent = indent_;
    printIndent(indent);
    out_ << "Function " << node->name << '\n';
    if (node->returnType) {
        printIndent(indent + 1);
        out_ << "ReturnType\n";
        indent_ = indent + 2;
        dispatch(node->returnType.get());
    }
    if (node->genericParams) {
        printGenericParams(*node->genericParams, indent + 1);
    }
    if (node->selfParam) {
        printSelfParam(*node->selfParam, indent + 1);
    }
    for (const auto &parameter : node->parameters) {
        printFunctionParam(parameter, indent + 1);
    }
    if (node->whereClause) {
        printWhereClause(*node->whereClause, indent + 1);
    }
    indent_ = indent + 1;
    dispatch(&node->body);
    return {};
}

std::any ASTPrinter::visitStructItem(StructItem *node) {
    const std::size_t indent = indent_;
    printIndent(indent);
    out_ << "Struct " << node->name << '\n';
    for (const auto &attribute : node->attributes) {
        printDeriveAttribute(attribute, indent + 1);
    }
    if (node->genericParams) {
        printGenericParams(*node->genericParams, indent + 1);
    }
    if (node->whereClause) {
        printWhereClause(*node->whereClause, indent + 1);
    }
    for (const auto &field : node->fields) {
        printStructField(field, indent + 1);
    }
    return {};
}

std::any ASTPrinter::visitConstantItem(ConstantItem *node) {
    const std::size_t indent = indent_;
    printIndent(indent);
    out_ << "Constant " << node->name << '\n';
    indent_ = indent + 1;
    dispatch(node->type.get());
    printConstValue(node->value, indent + 1);
    return {};
}

std::any ASTPrinter::visitImplItem(ImplItem *node) {
    const std::size_t indent = indent_;
    printIndent(indent);
    out_ << "Impl\n";
    indent_ = indent + 1;
    dispatch(node->typeName.get());
    if (node->genericParams) {
        printGenericParams(*node->genericParams, indent + 1);
    }
    if (node->whereClause) {
        printWhereClause(*node->whereClause, indent + 1);
    }
    for (auto &item : node->associatedItems) {
        indent_ = indent + 1;
        dispatch(item.get());
    }
    return {};
}

std::any ASTPrinter::visitUseItem(UseItem *node) {
    const std::size_t indent = indent_;
    printIndent(indent);
    out_ << "Use\n";
    printUseTree(node->tree, indent + 1);
    return {};
}

std::any ASTPrinter::visitEmptyStmt(EmptyStmt *node) {
    printIndent(indent_);
    out_ << "EmptyStmt\n";
    return {};
}

std::any ASTPrinter::visitLetStmt(LetStmt *node) {
    const std::size_t indent = indent_;
    printIndent(indent);
    out_ << "Let\n";
    printIdentifierBinding(node->binding, indent + 1);
    if (node->type) {
        printIndent(indent + 1);
        out_ << "Type\n";
        indent_ = indent + 2;
        dispatch(node->type.get());
    }
    indent_ = indent + 1;
    dispatch(node->initializer.get());
    return {};
}

std::any ASTPrinter::visitExprStmt(ExprStmt *node) {
    const std::size_t indent = indent_;
    printIndent(indent);
    out_ << "ExprStmt\n";
    indent_ = indent + 1;
    dispatch(node->expression.get());
    return {};
}

std::any ASTPrinter::visitIntegerLiteralExpr(IntegerLiteralExpr *node) {
    printIndent(indent_);
    out_ << "IntegerLiteral " << node->value << '\n';
    return {};
}

std::any ASTPrinter::visitBooleanLiteralExpr(BooleanLiteralExpr *node) {
    printIndent(indent_);
    out_ << "BooleanLiteral " << (node->value ? "true" : "false") << '\n';
    return {};
}

std::any ASTPrinter::visitUnitExpr(UnitExpr *node) {
    printIndent(indent_);
    out_ << "Unit\n";
    return {};
}

std::any ASTPrinter::visitPathInExpr(PathInExpr *node) {
    const std::size_t indent = indent_;
    printIndent(indent);
    out_ << "Path\n";
    for (const auto &segment : node->segments) {
        printPathExprSegment(segment, indent + 1);
    }
    return {};
}

std::any ASTPrinter::visitCallExpr(CallExpr *node) {
    const std::size_t indent = indent_;
    printIndent(indent);
    out_ << "Call\n";
    printIndent(indent + 1);
    out_ << "Callee\n";
    indent_ = indent + 2;
    dispatch(node->callee.get());
    printIndent(indent + 1);
    out_ << "Arguments\n";
    for (auto &argument : node->arguments) {
        indent_ = indent + 2;
        dispatch(argument.get());
    }
    return {};
}

std::any ASTPrinter::visitIndexExpr(IndexExpr *node) {
    const std::size_t indent = indent_;
    printIndent(indent);
    out_ << "Index\n";
    indent_ = indent + 1;
    dispatch(node->operand.get());
    indent_ = indent + 1;
    dispatch(node->index.get());
    return {};
}

std::any ASTPrinter::visitFieldAccessExpr(FieldAccessExpr *node) {
    const std::size_t indent = indent_;
    printIndent(indent);
    out_ << "FieldAccess\n";
    printPathExprSegment(node->field, indent + 1);
    indent_ = indent + 1;
    dispatch(node->operand.get());
    return {};
}

std::any ASTPrinter::visitUnaryExpr(UnaryExpr *node) {
    const std::size_t indent = indent_;
    printIndent(indent);
    out_ << "Unary " << unaryOpText(node->op) << '\n';
    indent_ = indent + 1;
    dispatch(node->operand.get());
    return {};
}

std::any ASTPrinter::visitCastExpr(CastExpr *node) {
    const std::size_t indent = indent_;
    printIndent(indent);
    out_ << "Cast\n";
    indent_ = indent + 1;
    dispatch(node->operand.get());
    printIndent(indent + 1);
    out_ << "Type\n";
    indent_ = indent + 2;
    dispatch(node->targetType.get());
    return {};
}

std::any ASTPrinter::visitBinaryExpr(BinaryExpr *node) {
    const std::size_t indent = indent_;
    printIndent(indent);
    out_ << "Binary " << binaryOpText(node->op) << '\n';
    indent_ = indent + 1;
    dispatch(node->left.get());
    indent_ = indent + 1;
    dispatch(node->right.get());
    return {};
}

std::any ASTPrinter::visitArrayExpr(ArrayExpr *node) {
    const std::size_t indent = indent_;
    printIndent(indent);
    out_ << (node->repeatCount ? "ArrayRepeat\n" : "Array\n");
    for (auto &element : node->elements) {
        indent_ = indent + 1;
        dispatch(element.get());
    }
    if (node->repeatCount) {
        printIndent(indent + 1);
        out_ << "RepeatCount\n";
        printConstValue(*node->repeatCount, indent + 2);
    }
    return {};
}

std::any ASTPrinter::visitBlockExpr(BlockExpr *node) {
    const std::size_t indent = indent_;
    printIndent(indent);
    out_ << "Block\n";
    for (auto &statement : node->statements) {
        indent_ = indent + 1;
        dispatch(statement.get());
    }
    if (node->tail) {
        printIndent(indent + 1);
        out_ << "Tail\n";
        indent_ = indent + 2;
        dispatch(node->tail.get());
    }
    return {};
}

std::any ASTPrinter::visitIfExpr(IfExpr *node) {
    const std::size_t indent = indent_;
    printIndent(indent);
    out_ << "If\n";
    indent_ = indent + 1;
    dispatch(node->condition.get());
    indent_ = indent + 2;
    dispatch(&node->thenBranch);
    if (node->elseBranch) {
        printIndent(indent + 1);
        out_ << "Else\n";
        indent_ = indent + 2;
        dispatch(node->elseBranch.get());
    }
    return {};
}

std::any ASTPrinter::visitLoopExpr(LoopExpr *node) {
    const std::size_t indent = indent_;
    printIndent(indent);
    out_ << "Loop\n";
    indent_ = indent + 1;
    dispatch(&node->body);
    return {};
}

std::any ASTPrinter::visitWhileExpr(WhileExpr *node) {
    const std::size_t indent = indent_;
    printIndent(indent);
    out_ << "While\n";
    indent_ = indent + 1;
    dispatch(node->condition.get());
    indent_ = indent + 1;
    dispatch(&node->body);
    return {};
}

std::any ASTPrinter::visitBreakExpr(BreakExpr *node) {
    const std::size_t indent = indent_;
    printIndent(indent);
    out_ << "Break\n";
    if (node->value) {
        indent_ = indent + 1;
        dispatch(node->value.get());
    }
    return {};
}

std::any ASTPrinter::visitReturnExpr(ReturnExpr *node) {
    const std::size_t indent = indent_;
    printIndent(indent);
    out_ << "Return\n";
    if (node->value) {
        indent_ = indent + 1;
        dispatch(node->value.get());
    }
    return {};
}

std::any ASTPrinter::visitContinueExpr(ContinueExpr *node) {
    printIndent(indent_);
    out_ << "Continue\n";
    return {};
}

std::any ASTPrinter::visitStructExpr(StructExpr *node) {
    const std::size_t indent = indent_;
    printIndent(indent);
    out_ << "StructExpr\n";
    indent_ = indent + 1;
    dispatch(&node->path);
    printIndent(indent + 1);
    out_ << "Fields {\n";
    for (const auto &field : node->fields) {
        printStructExprField(field, indent + 2);
    }
    printIndent(indent + 1);
    out_ << "}\n";
    return {};
}

std::any ASTPrinter::visitUnitType(UnitType *node) {
    printIndent(indent_);
    out_ << "UnitType\n";
    return {};
}

std::any ASTPrinter::visitPathType(PathType *node) {
    const std::size_t indent = indent_;
    printIndent(indent);
    out_ << "PathType\n";
    for (const auto &segment : node->segments) {
        printTypePathSegment(segment, indent + 1);
    }
    return {};
}

std::any ASTPrinter::visitReferenceType(ReferenceType *node) {
    const std::size_t indent = indent_;
    printIndent(indent);
    out_ << "ReferenceType &";
    if (node->lifetime) {
        out_ << ' ' << *node->lifetime;
    }
    if (node->isMutable) {
        out_ << " mut";
    }
    out_ << '\n';
    indent_ = indent + 1;
    dispatch(node->inner.get());
    return {};
}

std::any ASTPrinter::visitArrayType(ArrayType *node) {
    const std::size_t indent = indent_;
    printIndent(indent);
    out_ << "ArrayType\n";
    indent_ = indent + 1;
    dispatch(node->element.get());
    printIndent(indent + 1);
    out_ << "Length\n";
    printConstValue(node->length, indent + 2);
    return {};
}

void ASTPrinter::printPathSegment(const PathSegment &segment,
                                  std::size_t indent) {
    printIndent(indent);
    switch (segment.kind) {
    case PathSegmentKind::Ident:
        out_ << "Ident " << segment.name << '\n';
        break;
    case PathSegmentKind::SelfValue:
        out_ << "SelfValue\n";
        break;
    case PathSegmentKind::SelfType:
        out_ << "SelfType\n";
        break;
    case PathSegmentKind::Super:
        out_ << "Super\n";
        break;
    case PathSegmentKind::Crate:
        out_ << "Crate\n";
        break;
    }
}

void ASTPrinter::printPathExprSegment(const PathExprSegment &segment,
                                      std::size_t indent) {
    printPathSegment(segment.ident, indent);
    if (segment.genericArgs) {
        printIndent(indent + 1);
        out_ << "GenericArgs\n";
        for (const auto &arg : *segment.genericArgs) {
            printGenericArg(arg, indent + 2);
        }
    }
}

void ASTPrinter::printTypePathSegment(const TypePathSegment &segment,
                                      std::size_t indent) {
    printPathSegment(segment.ident, indent);
    if (segment.genericArgs) {
        printIndent(indent + 1);
        out_ << "GenericArgs\n";
        for (const auto &arg : *segment.genericArgs) {
            printGenericArg(arg, indent + 2);
        }
    }
}

void ASTPrinter::printGenericArg(const GenericArg &arg, std::size_t indent) {
    printIndent(indent);
    if (arg.lifetime) {
        out_ << "Lifetime " << *arg.lifetime << '\n';
    } else {
        out_ << "Type\n";
        indent_ = indent + 1;
        dispatch(arg.type.get());
    }
}

void ASTPrinter::printIdentifierBinding(const IdentifierBinding &binding,
                                        std::size_t indent) {
    printIndent(indent);
    if (binding.isMutable) {
        out_ << "mut ";
    }
    out_ << binding.name << '\n';
}

void ASTPrinter::printFunctionParam(const FunctionParam &param,
                                    std::size_t indent) {
    printIndent(indent);
    out_ << "FunctionParameter\n";
    printIdentifierBinding(param.binding, indent + 1);
    indent_ = indent + 1;
    dispatch(param.type.get());
}

void ASTPrinter::printSelfParam(const SelfParam &param, std::size_t indent) {
    printIndent(indent);
    out_ << "SelfParam ";
    if (param.isBorrowed) {
        out_ << '&';
    }
    if (param.lifetime) {
        out_ << *param.lifetime << ' ';
    }
    if (param.isMutable) {
        out_ << "mut ";
    }
    out_ << "self\n";
}

void ASTPrinter::printWhereClauseItem(const WhereClauseItem &item,
                                      std::size_t indent) {
    printIndent(indent);
    out_ << "WhereClauseItem\n";
    if (item.lifetime) {
        printIndent(indent + 1);
        out_ << "Lifetime " << *item.lifetime << '\n';
    } else if (item.type) {
        printIndent(indent + 1);
        out_ << "Type\n";
        indent_ = indent + 2;
        dispatch(item.type.get());
    }
    if (!item.bounds.empty()) {
        printIndent(indent + 1);
        out_ << "Bounds";
        for (const auto &bound : item.bounds) {
            out_ << ' ' << bound;
        }
        out_ << '\n';
    }
}

void ASTPrinter::printWhereClause(const WhereClause &clause,
                                  std::size_t indent) {
    printIndent(indent);
    out_ << "WhereClause\n";
    for (const auto &item : clause.items) {
        printWhereClauseItem(item, indent + 1);
    }
}

void ASTPrinter::printLifetimeParam(const LifetimeParam &param,
                                    std::size_t indent) {
    printIndent(indent);
    out_ << "LifetimeParam " << param.lifetime << '\n';
    if (param.lifetimeBounds) {
        printIndent(indent + 1);
        out_ << "Bounds";
        for (const auto &bound : *param.lifetimeBounds) {
            out_ << ' ' << bound;
        }
        out_ << '\n';
    }
}

void ASTPrinter::printGenericParams(const GenericParams &params,
                                    std::size_t indent) {
    printIndent(indent);
    out_ << "GenericParams\n";
    for (const auto &parameter : params.params) {
        printLifetimeParam(parameter, indent + 1);
    }
}

void ASTPrinter::printStructExprField(const StructExprField &field,
                                      std::size_t indent) {
    printIndent(indent);
    out_ << "StructExprField " << field.name << '\n';
    indent_ = indent + 1;
    dispatch(field.value.get());
}

void ASTPrinter::printStructField(const StructField &field,
                                  std::size_t indent) {
    printIndent(indent);
    out_ << field.name << '\n';
    indent_ = indent + 1;
    dispatch(field.type.get());
}

void ASTPrinter::printDeriveAttribute(const DeriveAttribute &attribute,
                                      std::size_t indent) {
    printIndent(indent);
    out_ << "DeriveAttribute\n";
    for (const auto &trait : attribute.traits) {
        printIndent(indent + 1);
        out_ << "Trait " << deriveTraitText(trait) << '\n';
    }
}

void ASTPrinter::printMagnitude(const Magnitude &magnitude,
                                std::size_t indent) {
    printIndent(indent);
    switch (magnitude.kind) {
    case Magnitude::Kind::Integer:
        out_ << "Integer " << magnitude.value << '\n';
        break;
    case Magnitude::Kind::Path:
        out_ << "Path " << magnitude.value << '\n';
        break;
    }
}

void ASTPrinter::printConstValue(const ConstValue &value, std::size_t indent) {
    printIndent(indent);
    switch (value.kind) {
    case ConstValue::Kind::Integer:
        out_ << "Integer " << value.value << '\n';
        break;
    case ConstValue::Kind::Boolean:
        out_ << "Boolean " << (value.boolValue ? "true" : "false") << '\n';
        break;
    case ConstValue::Kind::Path:
        out_ << "Path " << value.value << '\n';
        break;
    case ConstValue::Kind::Negate:
        out_ << "Negate\n";
        printMagnitude(value.operand, indent + 1);
        break;
    }
}

void ASTPrinter::printUsePath(const UsePath &path, std::size_t indent) {
    printIndent(indent);
    out_ << "UsePath\n";
    if (path.leadingPathSep) {
        printIndent(indent + 1);
        out_ << "LeadingPathSep\n";
    }
    for (const auto &segment : path.segments) {
        printPathSegment(segment, indent + 1);
    }
}

void ASTPrinter::printUseTree(const UseTree &tree, std::size_t indent) {
    printIndent(indent);
    switch (tree.kind) {
    case UseTree::Kind::Glob:
        out_ << "UseTree Glob\n";
        break;
    case UseTree::Kind::List:
        out_ << "UseTree List\n";
        break;
    case UseTree::Kind::Path:
        out_ << "UseTree Path\n";
        break;
    }
    if (tree.prefix) {
        printIndent(indent + 1);
        out_ << "Prefix\n";
        printUsePath(*tree.prefix, indent + 2);
        if (tree.trailingPathSep) {
            printIndent(indent + 2);
            out_ << "TrailingPathSep\n";
        }
    }
    if (tree.kind == UseTree::Kind::List) {
        printIndent(indent + 1);
        out_ << "Items {\n";
        for (const auto &item : tree.items) {
            printUseTree(item, indent + 2);
        }
        printIndent(indent + 1);
        out_ << "}\n";
    }
    if (tree.kind == UseTree::Kind::Path) {
        printIndent(indent + 1);
        out_ << "Path\n";
        printUsePath(tree.path, indent + 2);
        if (tree.alias || tree.aliasIsUnderscore) {
            printIndent(indent + 1);
            out_ << "Alias " << (tree.aliasIsUnderscore ? "_" : *tree.alias)
                 << '\n';
        }
    }
}

void ASTPrinter::printIndent(std::size_t indent) const {
    for (std::size_t i = 0; i < indent; ++i) {
        out_ << "  ";
    }
}

} // namespace rx::ast
