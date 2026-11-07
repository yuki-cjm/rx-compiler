#pragma once

#include "symbol.hpp"

#include <map>
#include <string>

namespace rx::semantic {

class SymbolMap {
  public:
    bool insert(const std::string &name, const Symbol &symbol);
    const Symbol *find(const std::string &name) const;
    Symbol *findMutable(const std::string &name);
    bool contains(const std::string &name) const;
    const std::map<std::string, Symbol> &entries() const;

  private:
    std::map<std::string, Symbol> entries_;
};

} // namespace rx::semantic
