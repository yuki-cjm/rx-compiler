#pragma once

#include "../type/type.hpp"

namespace rx::semantic {

struct PlaceInfo {
    TypePtr type;
    bool isMutable;  // 穿过 shared reference 后为 false
};

class PlaceRules {
  public:
    static bool stripToStruct(const TypePtr &baseType, StructId *outId, bool *crossedShared);
};

} // namespace rx::semantic
