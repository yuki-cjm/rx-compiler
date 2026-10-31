#include "namespace.hpp"

namespace rx::semantic {

bool SymbolMap::insert(const std::string &name, const Symbol &symbol) {
    if (entries_.count(name) != 0) {
        return false;
    }
    entries_.emplace(name, symbol);
    return true;
}

const Symbol *SymbolMap::find(const std::string &name) const {
    auto iterator = entries_.find(name);
    return iterator == entries_.end() ? nullptr : &iterator->second;
}

Symbol *SymbolMap::findMutable(const std::string &name) {
    auto iterator = entries_.find(name);
    return iterator == entries_.end() ? nullptr : &iterator->second;
}

bool SymbolMap::contains(const std::string &name) const {
    return entries_.count(name) != 0;
}

const std::map<std::string, Symbol> &SymbolMap::entries() const {
    return entries_;
}

} // namespace rx::semantic
