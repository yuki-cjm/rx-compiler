#pragma once

#include "builtin.hpp"

#include <optional>
#include <string>

namespace rx::semantic {

// Vec<T> 的内建能力。
// 方法与索引规则见规范 heap.html 的「Builtin signatures」与「Vec operations」。
class VecBuiltin {
  public:
    // 构造 Vec<Ty>。
    static TyPtr makeType(TyPtr element);

    // 判断是否为 Vec 类型。
    static bool isVecType(const TyPtr &ty);

    // 查找内建签名。
    //   - name="new"       ：关联函数 Vec::<T>::new() -> Vec<T>（receiver=None）
    //   - name="len"       ：方法 len(&self) -> usize
    //   - name="is_empty"  ：方法 is_empty(&self) -> bool
    //   - name="push"      ：方法 push(&mut self, T) -> ()
    //   - name="remove"    ：方法 remove(&mut self, usize) -> T
    //   - name="clone"     ：方法 clone(&self) -> Vec<T>（T: Clone）
    // element 为 Vec 的元素类型；未命中返回 nullopt。
    static std::optional<BuiltinFnSig> lookup(const std::string &name,
                                              const TyPtr &element);

    // 索引：v[index] 得到 T place；非 Vec 返回 nullopt。
    static std::optional<TyPtr> indexElement(const TyPtr &ty);
};

} // namespace rx::semantic
