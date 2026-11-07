#include "symbol_table.hpp"

namespace rx::semantic {

bool SymbolTable::declareType(const std::string &name, Symbol symbol) {
    if (types_.contains(name)) {
        return false;
    }
    if (symbol.kind == SymbolKind::Struct) {
        symbol.structId = allocateStructId();
    }
    if (!types_.insert(name, symbol)) {
        return false;
    }
    if (symbol.kind == SymbolKind::Struct) {
        structs_[symbol.structId] = types_.findMutable(name);
    }
    return true;
}

const Symbol *SymbolTable::lookupType(const std::string &name) const {
    return types_.find(name);
}

bool SymbolTable::declareGlobalValue(const std::string &name, Symbol symbol) {
    return values_.insert(name, symbol);
}

const Symbol *SymbolTable::lookupGlobalValue(const std::string &name) const {
    return values_.find(name);
}

bool SymbolTable::declareAssoc(StructId target, const std::string &name, Symbol symbol) {
    return assoc_[target].insert(name, symbol);
}

const Symbol *SymbolTable::lookupAssoc(StructId target, const std::string &name) const {
    auto found = assoc_.find(target);
    if (found == assoc_.end()) {
        return nullptr;
    }
    return found->second.find(name);
}

Scope &SymbolTable::scope() { return scope_; }

const Scope &SymbolTable::scope() const { return scope_; }

const Symbol *SymbolTable::resolveValue(const std::string &name) const {
    if (const Symbol *local = scope_.lookup(name)) {
        return local;
    }
    return values_.find(name);
}

StructId SymbolTable::allocateStructId() { return ++nextStructId_; }

const FieldInfo *SymbolTable::lookupField(StructId structId, const std::string &name) const {
    const Symbol *symbol = lookupStruct(structId);
    if (symbol == nullptr) {
        return nullptr;
    }
    for (const FieldInfo &field : symbol->fields) {
        if (field.name == name) {
            return &field;
        }
    }
    return nullptr;
}

const Symbol *SymbolTable::lookupStruct(StructId structId) const {
    auto found = structs_.find(structId);
    return found == structs_.end() ? nullptr : found->second;
}

Symbol *SymbolTable::lookupStructMutable(StructId structId) {
    auto found = structs_.find(structId);
    return found == structs_.end() ? nullptr : found->second;
}

} // namespace rx::semantic
