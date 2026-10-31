#include "coercion.hpp"

#include "../ty/ty_compare.hpp"

namespace rx::semantic {

namespace {

// 从 place 类型 S 出发，沿内建 deref 链（&U→U、&mut U→U、Box<U>→U）
// 能否到达 T；若途中穿过 shared reference 则置 crossedShared。
bool derefReaches(const TyPtr &source, const TyPtr &target,
                  bool &crossedShared) {
    TyPtr current = source;
    for (;;) {
        if (sameTy(current, target)) {
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

bool Coercion::canCoerce(const SymbolTable &table, const TyPtr &from,
                         const TyPtr &to) {
    (void)table;
    if (sameTy(from, to)) {
        return true;
    }
    if (isNever(from)) {
        return true;
    }
    if (isReference(from) && isReference(to)) {
        bool crossedShared = false;
        if (to->mutability == Mutability::Shared) {
            // &S 或 &mut S → &T
            return derefReaches(from->inner, to->inner, crossedShared);
        }
        // &mut S → &mut T：要求外层 &mut 且途中无 shared reference。
        if (from->mutability != Mutability::Mutable) {
            return false;
        }
        return derefReaches(from->inner, to->inner, crossedShared) &&
               !crossedShared;
    }
    return false;
}

std::optional<TyPtr> Coercion::leastUpperBound(
    const SymbolTable &table, const std::vector<TyPtr> &types) {
    std::vector<TyPtr> remaining;
    for (const TyPtr &type : types) {
        if (!isNever(type)) {
            remaining.push_back(type);
        }
    }
    if (remaining.empty()) {
        return Ty::makeNever();
    }

    TyPtr target = remaining[0];
    for (std::size_t index = 1; index < remaining.size(); ++index) {
        const TyPtr &candidate = remaining[index];
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
