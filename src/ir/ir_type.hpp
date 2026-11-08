#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace rx::ir {

// ===========================================================================
// IR 层类型系统
// ===========================================================================
// 注意：这里的 Type 与 semantic::Type 相互独立。codegen 阶段负责把
// semantic::Type（i32/u32/isize/usize/bool/unit/never/struct/reference/array/
// box/vec）映射成本文件描述的 LLVM IR 类型，例如：
//   i32 / u32 / isize / usize -> Int(32)
//   bool                      -> Int(1)（作为真值）或 Int(8)（作为存储单元）
//   ()                        -> Void
//   &T / &mut T / Box<T>      -> Ptr（不透明指针）
//   [T; N]                    -> Array(element, N)
//   struct S { ... }          -> Struct(name, fields)

enum class TypeKind {
    Void,     // void：无返回值、unit 类型、无参函数
    Int,      // iN：整数/布尔，位宽由 bitWidth 给出（i1 / i8 / i32）
    Ptr,      // ptr：不透明指针（LLVM 15+ 写法，打印时不带 pointee 类型）
    Array,    // [length x element] 定长数组
    Struct,   // %struct.<name> = type { fields... }；fields 为空表示 opaque 前置声明
    Function, // result (params...) 函数类型，用于函数指针与声明
};

struct Type;
using TypePtr = std::shared_ptr<const Type>;

struct Type {
    TypeKind kind = TypeKind::Void;

    // ---- Int ----
    unsigned bitWidth = 0; // 位宽：1（bool 真值）/ 8 / 32

    // ---- Ptr / Array ----
    TypePtr element;          // Ptr 指向的元素类型 / Array 的元素类型
    std::uint64_t length = 0; // Array 的元素个数

    // ---- Struct ----
    std::string name;            // 具名结构体名（打印为 %struct.<name>）
    std::vector<TypePtr> fields; // 字段类型列表；空表示 opaque 前置声明

    // ---- Function ----
    TypePtr result;              // 返回类型
    std::vector<TypePtr> params; // 形参类型列表

    static TypePtr makeVoid();
    static TypePtr makeInt(unsigned bitWidth);
    static TypePtr makePtr(TypePtr pointee); // pointee 仅供调试/文档，打印时为 opaque ptr
    static TypePtr makeArray(TypePtr element, std::uint64_t length);
    static TypePtr makeStruct(std::string name, std::vector<TypePtr> fields);
    static TypePtr makeOpaqueStruct(std::string name); // 前置声明（fields 为空）
    static TypePtr makeFunction(TypePtr result, std::vector<TypePtr> params);

    // 类型判定
    bool isVoid() const;
    bool isInt() const;
    bool isPtr() const;
    bool isArray() const;
    bool isStruct() const;
    bool isFunction() const;
};

TypePtr makeI1();  // i1
TypePtr makeI8();  // i8
TypePtr makeI32(); // i32

} // namespace rx::ir
