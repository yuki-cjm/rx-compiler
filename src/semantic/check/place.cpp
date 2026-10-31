#include "place.hpp"

#include "../ty/ty.hpp"

namespace rx::semantic {

std::optional<PlaceInfo> PlaceRules::deref(const TyPtr &referenceType) {
    if (!isReference(referenceType)) {
        return std::nullopt;
    }
    PlaceInfo info;
    info.type = referenceType->inner;
    info.isMutable = referenceType->mutability == Mutability::Mutable;
    return info;
}

std::optional<TyPtr> PlaceRules::indexElement(const TyPtr &containerType) {
    if (isArray(containerType) || isVec(containerType)) {
        return containerType->inner;
    }
    return std::nullopt;
}

const FieldInfo *PlaceRules::field(const SymbolTable &table,
                                   const TyPtr &baseType,
                                   const std::string &name) {
    StructId structId = 0;
    bool crossedShared = false;
    if (!stripToStruct(baseType, &structId, &crossedShared)) {
        return nullptr;
    }
    return table.lookupField(structId, name);
}

bool PlaceRules::stripToStruct(const TyPtr &baseType, StructId *outId,
                               bool *crossedShared) {
    TyPtr current = baseType;
    *crossedShared = false;
    for (;;) {
        if (isStruct(current)) {
            *outId = current->structId;
            return true;
        }
        if (isReference(current)) {
            if (current->mutability == Mutability::Shared) {
                *crossedShared = true;
            }
            current = current->inner;
        } else if (isBox(current)) {
            current = current->inner;
        } else {
            return false;
        }
    }
}

} // namespace rx::semantic
