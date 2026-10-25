#pragma once

#include "../ty/ty.hpp"
#include "../../ast/ast.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace rx::semantic {

// 符号种类。一个符号（Symbol）是「名字 → 实体」的绑定，
// 出现在 type / value / field 三个命名空间之一。
enum class SymbolKind {
    Function,        // 顶层函数 / 关联函数 / 方法（value 命名空间）
    Struct,          // 结构体类型（type 命名空间）
    Constant,        // 顶层 / 关联常量（value 命名空间）
    Local,           // let 绑定 / 函数参数（局部 value 作用域）
    BuiltinType,     // i32/u32/isize/usize/bool/Box/Vec 等内建类型
    BuiltinFunction, // get_i32/print_i32/println_i32 等内建函数
};

// 结构体字段：名字 + 已解析的类型（由 resolve 阶段填充 TyPtr）。
struct FieldInfo {
    std::string name;
    TyPtr type;
};

// 一个符号。字段按 kind 分别有意义，其余保持默认值。
// AST 指针仅作只读引用，不拥有所有权（Program 的生命周期覆盖整个语义分析）。
struct Symbol {
    SymbolKind kind;
    std::string name;

    // Function 专用。
    const ast::FunctionItem *function = nullptr; // 指向函数定义
    bool hasReceiver = false;                    // 是否为方法（带 self 接收者）
    StructId implTarget = 0;                     // 关联项所属结构体 id；0 表示顶层

    // Struct 专用。
    StructId structId = 0;                       // 结构体 id
    std::vector<FieldInfo> fields;               // 字段列表
    std::vector<ast::DeriveTrait> derives;       // #[derive(...)] 属性

    // Constant / Local 专用。
    TyPtr type;                                  // 已解析的类型
    bool isMutable = false;                      // Local 绑定是否 mut
    const ast::ConstantItem *constant = nullptr; // Constant 专用：指向常量定义
};

} // namespace rx::semantic
