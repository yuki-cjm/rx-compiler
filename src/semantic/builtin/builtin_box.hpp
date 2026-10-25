#pragma once

#include "builtin.hpp"

#include <optional>
#include <string>

namespace rx::semantic {

// Box<T> 的内建能力。
// 与类型检查逻辑解耦：calls / operators 通过 lookup / derefTarget 查询，
// 不内嵌容器实现细节。
class BoxBuiltin {
  public:
    // 构造 Box<Ty>。
    static TyPtr makeType(TyPtr element);

    // 判断是否为 Box 类型。
    static bool isBoxType(const TyPtr &ty);

    // 查找内建签名。
    //   - name="new"    ：关联函数 Box::<T>::new(T) -> Box<T>（receiver=None）
    //   - name="clone"  ：方法 clone(&self) -> Box<T>（T: Clone，receiver=Shared）
    // element 为 Box 的元素类型；未命中返回 nullopt。
    static std::optional<BuiltinFnSig> lookup(const std::string &name,
                                              const TyPtr &element);

    // deref：*Box<T> 得到 T place；非 Box 返回 nullopt。
    static std::optional<TyPtr> derefTarget(const TyPtr &ty);
};

} // namespace rx::semantic
