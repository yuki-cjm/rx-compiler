#include "place.hpp"

namespace rx::semantic {

bool PlaceRules::stripToStruct(const TypePtr &baseType, StructId *outId, bool *crossedShared) {
    TypePtr current = baseType;
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
