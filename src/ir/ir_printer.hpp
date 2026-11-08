#pragma once

#include "ir_type.hpp"
#include "ir_value.hpp"

#include <iosfwd>
#include <string>

namespace rx::ir {

// ===========================================================================
// IRPrinter：把 Module 打印为 LLVM IR 文本（.ll）
// ===========================================================================
// 打印顺序：
//   目标三元组 / 数据布局
//   -> 具名结构体类型声明（去重，嵌套 struct 先声明内层）
//   -> 全局变量
//   -> 函数（declare / define）
// 实现提示：打印具名 struct 前需先收集所有被引用的具名 struct 并拓扑排序，
// 以保证被嵌套的类型先于使用它的类型打印。
class IRPrinter {
  public:
    explicit IRPrinter(std::ostream &out);

    void printModule(const Module &module);

  private:
    void printType(const TypePtr &type);           // 打印类型（含递归的结构体）
    void printConstant(const Constant &constant);  // 打印常量
    void printGlobal(const GlobalVariable &global); // 打印全局变量
    void printFunction(const Function &function);   // 打印函数（define/declare）
    void printBasicBlock(const BasicBlock &block);  // 打印基本块
    void printInstruction(const Instruction &inst); // 打印单条指令
    // 打印一个值引用：常量 -> 字面量，基本块/局部值 -> %name，函数/全局 -> @name
    std::string printValue(const Value &value);

    std::ostream &out_;
};

} // namespace rx::ir
