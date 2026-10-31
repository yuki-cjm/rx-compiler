#include "semantic.hpp"

#include "builtin/builtin.hpp"
#include "builtin/builtin_traits.hpp"
#include "check/main_check.hpp"
#include "check/typeck.hpp"
#include "collect/collector.hpp"
#include "consteval/const_eval.hpp"
#include "resolve/layout_check.hpp"
#include "resolve/resolver.hpp"
#include "ty/ty.hpp"

namespace rx::semantic {

namespace {

bool hasDerive(const Symbol *symbol, ast::DeriveTrait trait) {
    if (symbol == nullptr) {
        return false;
    }
    for (ast::DeriveTrait candidate : symbol->derives) {
        if (candidate == trait) {
            return true;
        }
    }
    return false;
}

// 解析结构体字段类型，写回符号表。
void resolveStructFields(const ast::Program &program, SymbolTable &table,
                         Diagnostic &diag) {
    TypeResolver resolver(table, diag);
    for (const ast::ItemPtr &item : program.items) {
        auto *structure = dynamic_cast<const ast::StructItem *>(item.get());
        if (structure == nullptr) {
            continue;
        }
        const Symbol *symbol = table.lookupType(structure->name);
        if (symbol == nullptr) {
            continue;
        }
        Symbol *mutableSymbol = table.lookupStructMutable(symbol->structId);
        if (mutableSymbol == nullptr) {
            continue;
        }
        TyPtr selfType = Ty::makeStruct(symbol->structId);
        for (std::size_t index = 0; index < structure->fields.size(); ++index) {
            mutableSymbol->fields[index].type =
                resolver.resolve(*structure->fields[index].type, selfType);
        }
    }
}

// 校验 struct 的 derive 需求。
void checkDerives(const ast::Program &program, SymbolTable &table,
                  Diagnostic &diag) {
    for (const ast::ItemPtr &item : program.items) {
        auto *structure = dynamic_cast<const ast::StructItem *>(item.get());
        if (structure == nullptr) {
            continue;
        }
        const Symbol *symbol = table.lookupType(structure->name);
        if (symbol == nullptr) {
            continue;
        }
        bool deriveCopy = hasDerive(symbol, ast::DeriveTrait::Copy);
        bool deriveClone = hasDerive(symbol, ast::DeriveTrait::Clone);
        bool derivePartialEq = hasDerive(symbol, ast::DeriveTrait::PartialEq);
        bool deriveEq = hasDerive(symbol, ast::DeriveTrait::Eq);

        if (deriveCopy && !deriveClone) {
            diag.error("deriving Copy requires Clone");
        }
        if (deriveEq && !derivePartialEq) {
            diag.error("deriving Eq requires PartialEq");
        }
        for (const FieldInfo &field : symbol->fields) {
            if (deriveCopy && !isCopy(table, field.type)) {
                diag.error("Copy requires every field to be Copy");
            }
            if (deriveClone && !isClone(table, field.type)) {
                diag.error("Clone requires every field to be Clone");
            }
            if (derivePartialEq && !supportsPartialEq(table, field.type)) {
                diag.error("PartialEq requires every field to be PartialEq");
            }
            if (deriveEq && !supportsEq(table, field.type)) {
                diag.error("Eq requires every field to be Eq");
            }
        }
    }
}

// 求值全部常量（含环检测），并顺带检查未使用的常量。
void checkConstants(const ast::Program &program, SymbolTable &table,
                    Diagnostic &diag) {
    TypeResolver resolver(table, diag);
    for (const ast::ItemPtr &item : program.items) {
        if (auto *constant = dynamic_cast<const ast::ConstantItem *>(item.get())) {
            const Symbol *symbol = table.lookupGlobalValue(constant->name);
            if (symbol == nullptr) {
                continue;
            }
            TyPtr type = resolver.resolve(*constant->type, nullptr);
            if (type != nullptr) {
                evaluateConst(table, diag, constant->value, type, nullptr);
            }
        } else if (auto *implementation =
                       dynamic_cast<const ast::ImplItem *>(item.get())) {
            TyPtr target = resolver.resolve(*implementation->typeName, nullptr);
            if (target == nullptr || !isStruct(target)) {
                continue;
            }
            for (const ast::ItemPtr &associated : implementation->associatedItems) {
                auto *constant =
                    dynamic_cast<const ast::ConstantItem *>(associated.get());
                if (constant == nullptr) {
                    continue;
                }
                const Symbol *symbol =
                    table.lookupAssoc(target->structId, constant->name);
                if (symbol == nullptr) {
                    continue;
                }
                TyPtr type = resolver.resolve(*constant->type, target);
                if (type != nullptr) {
                    evaluateConst(table, diag, constant->value, type, target);
                }
            }
        }
    }
}

} // namespace

bool SemanticAnalyzer::check(const ast::Program &program) {
    registerBuiltins(table_);
    Collector().collect(program, table_, diag_);

    resolveStructFields(program, table_, diag_);
    checkDerives(program, table_, diag_);
    checkConstants(program, table_, diag_);

    TypeChecker typechecker(table_, diag_);
    typechecker.check(program);

    MainChecker(table_, diag_).check();
    LayoutChecker(table_, diag_).check(program);

    return !diag_.hasErrors();
}

const Diagnostic &SemanticAnalyzer::diagnostics() const { return diag_; }

} // namespace rx::semantic
