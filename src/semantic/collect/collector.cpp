#include "collector.hpp"

#include "duplicate_check.hpp"
#include "impl_collect.hpp"

#include <set>

namespace rx::semantic {

namespace {

bool hasRepeatedDerive(const ast::StructItem &item) {
    std::set<ast::DeriveTrait> seen;
    for (const ast::DeriveAttribute &attribute : item.attributes) {
        for (ast::DeriveTrait trait : attribute.traits) {
            if (!seen.insert(trait).second) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

void Collector::collect(const ast::Program &program, SymbolTable &table,
                        Diagnostic &diag) {
    // 阶段一：先声明所有 struct / function / constant，使顶层查找与声明顺序无关。
    for (const ast::ItemPtr &item : program.items) {
        if (auto *structure = dynamic_cast<const ast::StructItem *>(item.get())) {
            Symbol symbol;
            symbol.kind = SymbolKind::Struct;
            symbol.name = structure->name;
            symbol.fields.reserve(structure->fields.size());
            for (const ast::StructField &field : structure->fields) {
                FieldInfo info;
                info.name = field.name;
                symbol.fields.push_back(std::move(info));
            }
            for (const ast::DeriveAttribute &attribute : structure->attributes) {
                for (ast::DeriveTrait trait : attribute.traits) {
                    symbol.derives.push_back(trait);
                }
            }

            checkDuplicateFields(*structure, diag);
            if (hasRepeatedDerive(*structure)) {
                diag.error("repeated derive entry for struct '" +
                           structure->name + "'");
            }
            if (!table.declareType(structure->name, symbol)) {
                diag.error("duplicate struct '" + structure->name + "'");
            }
        } else if (auto *function =
                       dynamic_cast<const ast::FunctionItem *>(item.get())) {
            if (function->selfParam.has_value()) {
                diag.error("a top-level function cannot have a receiver");
            }
            checkDuplicateParameters(*function, diag);

            Symbol symbol;
            symbol.kind = SymbolKind::Function;
            symbol.name = function->name;
            symbol.function = function;
            if (!table.declareGlobalValue(function->name, symbol)) {
                diag.error("duplicate top-level value '" + function->name + "'");
            }
        } else if (auto *constant =
                       dynamic_cast<const ast::ConstantItem *>(item.get())) {
            Symbol symbol;
            symbol.kind = SymbolKind::Constant;
            symbol.name = constant->name;
            symbol.constant = constant;
            if (!table.declareGlobalValue(constant->name, symbol)) {
                diag.error("duplicate top-level value '" + constant->name + "'");
            }
        }
        // UseItem 直接丢弃（Rx 不解析 use 声明）。
    }

    // 阶段二：处理 impl 块（此时可解析目标结构体）。
    ImplCollector implCollector(table, diag);
    for (const ast::ItemPtr &item : program.items) {
        if (auto *implementation =
                dynamic_cast<const ast::ImplItem *>(item.get())) {
            implCollector.collect(*implementation);
        }
    }
}

} // namespace rx::semantic
