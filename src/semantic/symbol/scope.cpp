#include "scope.hpp"

namespace rx::semantic {

void Scope::push() { layers_.emplace_back(); }

void Scope::pop() {
    if (!layers_.empty()) {
        layers_.pop_back();
    }
}

void Scope::declare(const std::string &name, const Symbol &symbol) {
    if (layers_.empty()) {
        layers_.emplace_back();
    }
    // 覆盖同层同名绑定，实现 let shadowing。
    layers_.back()[name] = symbol;
}

const Symbol *Scope::lookup(const std::string &name) const {
    for (auto iterator = layers_.rbegin(); iterator != layers_.rend();
         ++iterator) {
        auto found = iterator->find(name);
        if (found != iterator->end()) {
            return &found->second;
        }
    }
    return nullptr;
}

bool Scope::containsLocally(const std::string &name) const {
    if (layers_.empty()) {
        return false;
    }
    return layers_.back().count(name) != 0;
}

} // namespace rx::semantic
