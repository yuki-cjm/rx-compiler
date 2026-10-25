#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace rx::semantic {

// 结构体的唯一标识。由 SymbolTable 在收集阶段顺序分配，
// 用于在语义类型中实现「nominal」比较：两个 struct 类型仅当 id 相同才算同一类型。
using StructId = std::uint32_t;

// 引用可变性。
enum class Mutability {
    Shared,   // &T
    Mutable,  // &mut T
};

// 语义类型的种类。与 AST 中的语法 Type（ast::Type）不同，
// Ty 是规范化、可比较的类型表示，供类型检查、强制转换与常量求值使用。
enum class TyKind {
    I32,      // 有符号 32 位整数
    U32,      // 无符号 32 位整数
    Isize,    // 有符号 32 位整数（与 i32 是不同名义类型）
    Usize,    // 无符号 32 位整数（与 u32 是不同名义类型）
    Bool,     // 布尔
    Unit,     // ()，函数 / 控制流的单元结果
    Never,    // !，发散表达式（内部使用，可适配任意目标类型）
    Struct,   // 命名结构体，按 structId 比较（nominal）
    Reference,// &T / &mut T
    Array,    // [T; N]
    Box,      // Box<T>
    Vec,      // Vec<T>
};

struct Ty;
// 不可变的类型节点句柄。递归类型（Reference/Array/Box/Vec 的 inner）通过它引用子节点。
using TyPtr = std::shared_ptr<const Ty>;

// 不可变类型节点。字段按 kind 分别有意义，其余字段保持默认值。
struct Ty {
    TyKind kind;

    // 仅 Reference 使用。
    Mutability mutability = Mutability::Shared;

    // 仅 Reference / Array / Box / Vec 使用：指向内层 / 元素类型。
    TyPtr inner;

    // 仅 Array 使用：已求值的长度（usize 常量值）。
    std::uint64_t length = 0;

    // 仅 Struct 使用：指向收集阶段注册的结构体。
    StructId structId = 0;

    // 工厂方法。调用方需保证 kind 与工厂语义一致
    // （如 makeScalar 只应传四种整数之一或 Bool）。
    static TyPtr makeScalar(TyKind kind);
    static TyPtr makeUnit();
    static TyPtr makeNever();
    static TyPtr makeStruct(StructId id);
    static TyPtr makeReference(TyPtr inner, Mutability mutability);
    static TyPtr makeArray(TyPtr element, std::uint64_t length);
    static TyPtr makeBox(TyPtr element);
    static TyPtr makeVec(TyPtr element);
};

// ---- 便捷谓词（实现位于 ty.cpp）----

// 是否为四种整数之一（i32/u32/isize/usize）。
bool isScalar(const TyPtr &ty);
bool isInteger(const TyPtr &ty);
// 是否为有符号整数（i32/isize）。
bool isSignedInteger(const TyPtr &ty);
// 是否为无符号整数（u32/usize）。
bool isUnsignedInteger(const TyPtr &ty);
bool isBool(const TyPtr &ty);
bool isUnit(const TyPtr &ty);
bool isNever(const TyPtr &ty);
bool isStruct(const TyPtr &ty);
bool isReference(const TyPtr &ty);
bool isArray(const TyPtr &ty);
bool isBox(const TyPtr &ty);
bool isVec(const TyPtr &ty);

} // namespace rx::semantic
