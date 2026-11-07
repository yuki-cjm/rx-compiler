#include "coercion.hpp"

#include "../type/type_compare.hpp"

namespace rx::semantic {

namespace {

bool derefReaches(const TypePtr &source, const TypePtr &target, bool &crossedShared) {
    TypePtr current = source;
    for (;;) {
        if (sameType(current, target)) {
            return true;
        }
        if (isReference(current)) {
            if (current->mutability == Mutability::Shared) {
                crossedShared = true;
            }
            current = current->inner;
        } else if (isBox(current)) {
            current = current->inner;
        } else {
            return false;
        }
    }
}

} // namespace

bool Coercion::canCoerce(const SymbolTable &table, const TypePtr &from, const TypePtr &to) {
    (void)table;
    if (sameType(from, to)) {
        return true;
    }
    if (isNever(from)) {
        return true;
    }
    if (isReference(from) && isReference(to)) {
        bool crossedShared = false;
        if (to->mutability == Mutability::Shared) {
            // &S or &mut S → &T
            return derefReaches(from->inner, to->inner, crossedShared);
        }
        // &mut S → &mut T：途中无 shared reference。
        if (from->mutability != Mutability::Mutable) {
            return false;
        }
        return derefReaches(from->inner, to->inner, crossedShared) && !crossedShared;
    }
    return false;
}

std::optional<TypePtr> Coercion::leastUpperBound(const SymbolTable &table, const std::vector<TypePtr> &types) {
    std::vector<TypePtr> remaining;
    for (const TypePtr &type : types) {
        if (!isNever(type)) {
            remaining.push_back(type);
        }
    }
    if (remaining.empty()) {
        return Type::makeNever();
    }

    TypePtr target = remaining[0];
    for (std::size_t index = 1; index < remaining.size(); ++index) {
        const TypePtr &candidate = remaining[index];
        if (canCoerce(table, candidate, target)) {
            continue;
        }
        if (canCoerce(table, target, candidate)) {
            bool allAdjust = true;
            for (std::size_t earlier = 0; earlier < index; ++earlier) {
                if (!canCoerce(table, remaining[earlier], candidate)) {
                    allAdjust = false;
                    break;
                }
            }
            if (allAdjust) {
                target = candidate;
                continue;
            }
        }
        return std::nullopt;
    }
    return target;
}

} // namespace rx::semantic
