#pragma once

#include "symbol.hpp"

#include <map>
#include <string>
#include <vector>

namespace rx::semantic {

class Scope {
  public:
    void push();
    void pop();
    void declare(const std::string &name, const Symbol &symbol);
    const Symbol *lookup(const std::string &name) const;
    bool containsLocally(const std::string &name) const;

  private:
    std::vector<std::map<std::string, Symbol>> layers_;
};

} // namespace rx::semantic
