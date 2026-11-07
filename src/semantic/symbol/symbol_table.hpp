#pragma once

#include "symbol.hpp"
#include "namespace.hpp"
#include "scope.hpp"

#include <map>
#include <string>

namespace rx::semantic {
class SymbolTable {
  public:
    // type
    bool declareType(const std::string &name, Symbol symbol);
    const Symbol *lookupType(const std::string &name) const;

    // value
    bool declareGlobalValue(const std::string &name, Symbol symbol);
    const Symbol *lookupGlobalValue(const std::string &name) const;

    // assoc
    bool declareAssoc(StructId target, const std::string &name, Symbol symbol);
    const Symbol *lookupAssoc(StructId target, const std::string &name) const;

    // scope
    Scope &scope();
    const Scope &scope() const;

    const Symbol *resolveValue(const std::string &name) const; // value
    StructId allocateStructId(); // struct
    const FieldInfo *lookupField(StructId structId, const std::string &name) const; // struct
    const Symbol *lookupStruct(StructId structId) const; // struct
    Symbol *lookupStructMutable(StructId structId); // struct

  private:
    SymbolMap types_;
    SymbolMap values_;
    std::map<StructId, SymbolMap> assoc_;
    std::map<StructId, Symbol*> structs_;
    Scope scope_;
    StructId nextStructId_ = 0;
};

} // namespace rx::semantic
