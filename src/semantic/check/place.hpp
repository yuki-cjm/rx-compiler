#pragma once

#include "../../ast/ast.hpp"
#include "../symbol/symbol.hpp"
#include "../symbol/symbol_table.hpp"
#include "../ty/ty.hpp"

#include <optional>
#include <string>

namespace rx::semantic {

// place（可寻址存储位置）的描述。
struct PlaceInfo {
    TyPtr type;      // 该 place 的值的类型
    bool isMutable;  // 是否可写；穿过 shared reference 后为 false
};

// place 可变性传播的纯规则（规范 expressions.html 的「Places and values」）。
// 完整的 place 分类由 typeck 组合这些规则完成（需要递归查子表达式类型）。
class PlaceRules {
  public:
    // 解引用一层：由引用类型得到目标 place。可变性取决于引用可变性。
    static std::optional<PlaceInfo> deref(const TyPtr &referenceType);

    // 索引一层：数组 / Vec 得到元素类型（Vec 索引的可变性由容器访问方式决定）。
    static std::optional<TyPtr> indexElement(const TyPtr &containerType);

    // 字段访问：按 receiver 类型（可能为 &S / &mut S / Box<S>，需先按 deref 链
    // 到达结构体 S）查找字段；返回字段信息（含类型）。
    static const FieldInfo *field(const SymbolTable &table,
                                  const TyPtr &baseType, const std::string &name);

    // 沿内建 deref 链（&U→U、&mut U→U、Box<U>→U）查找结构体类型 S，
    // 返回 S 以及过程中是否穿越了 shared reference（穿越后不可写）。
    // 供字段访问 / 方法接收者调整复用。
    static bool stripToStruct(const TyPtr &baseType, StructId *outId,
                              bool *crossedShared);
};

} // namespace rx::semantic
