#pragma once

#include "../symbol/symbol_table.hpp"
#include "../ty/ty.hpp"

#include <string>
#include <vector>

namespace rx::semantic {

// 内建方法 / 关联函数的 receiver 形态。
enum class ReceiverKind {
    None,    // 关联函数（如 Box::<T>::new），无 receiver
    Owned,   // self / mut self
    Shared,  // &self
    Mutable, // &mut self
};

// 一条内建函数签名。用于描述 Box / Vec 的构造器与方法。
// params 不包含 receiver；result 为返回类型。
struct BuiltinFnSig {
    std::string name;
    ReceiverKind receiver = ReceiverKind::None;
    std::vector<TyPtr> params;
    TyPtr result;
};

// 把全部内建（标量 + I/O + Box + Vec + trait 能力）注册进符号表。
// 实现位于 builtin.cpp，分别调用 scalar / box / vec / traits 的注册入口。
void registerBuiltins(SymbolTable &table);

} // namespace rx::semantic
