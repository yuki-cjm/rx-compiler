#pragma once

#include "ir_type.hpp"
#include "ir_value.hpp"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace rx::ir {

// ===========================================================================
// IRBuilder：构建 IR 的入口类（本骨架的核心，由你实现）
// ===========================================================================
// 职责：
//   1. 持有 Module 引用，通过 createFunction / createGlobalVariable 注册对象；
//   2. 维护当前插入基本块，createXxx 生成的指令都追加到该块末尾；
//   3. 给 SSA 值分配名字（可读名 hint 或自动编号）；
//   4. 提供类型 / 常量工厂，屏蔽 ir_type.hpp 与 ir_value.hpp 的构造细节。
//
// codegen 阶段典型用法：
//   Module module;
//   IRBuilder builder(module);
//   Function *main = builder.createFunction(builder.i32Ty(), {}, "main");
//   BasicBlock *entry = builder.appendBlock(main, "entry");
//   builder.setInsertPoint(entry);
//   builder.createRet(builder.intConst(32, 0));
//   module.print(std::cout);
class IRBuilder {
  public:
    explicit IRBuilder(Module &module);

    // ---- 类型工厂（转发到 ir_type.hpp 的工厂函数）----
    TypePtr voidTy() const;
    TypePtr intTy(unsigned bitWidth) const;
    TypePtr ptrTy(TypePtr pointee = nullptr) const; // 不透明指针，pointee 仅供文档
    TypePtr arrayTy(TypePtr element, std::uint64_t length) const;
    TypePtr structTy(std::string name, std::vector<TypePtr> fields) const;
    TypePtr opaqueStructTy(std::string name) const;
    TypePtr functionTy(TypePtr result, std::vector<TypePtr> params) const;
    TypePtr i1Ty() const;  // i1
    TypePtr i8Ty() const;  // i8
    TypePtr i32Ty() const; // i32

    // ---- 常量 ----
    // iN 整数常量（bool 用位宽 1 或 8 表示）
    ConstantPtr intConst(unsigned bitWidth, std::uint64_t value) const;
    // null 指针常量（type 为 ptr）
    ConstantPtr nullPtr() const;
    // 字符串字面量：创建 [N x i8] 全局常量，返回其地址（ptr 常量）
    ConstantPtr getGlobalString(const std::string &text);

    // ---- 模块级对象 ----
    // 创建函数定义 / 声明；result 为返回类型，params 为形参类型
    Function *createFunction(TypePtr result, std::vector<TypePtr> params,
                             const std::string &name, bool isDeclaration = false,
                             bool isVarArg = false);
    // 创建全局变量（initializer 可为空，表示 extern）
    GlobalVariable *createGlobalVariable(TypePtr type, const std::string &name,
                                         bool isConstant,
                                         ConstantPtr initializer);

    // ---- 基本块与插入点 ----
    // 在 parent 中新建基本块（不改变当前插入点）
    BasicBlock *createBasicBlock(Function *parent, std::string name = "");
    // 设置当前插入点；之后 createXxx 生成的指令都追加到该块
    void setInsertPoint(BasicBlock *block);
    // 当前插入基本块
    BasicBlock *insertPoint() const;

    // ---- 内存 ----
    // 在栈上分配 type 大小的空间，返回 ptr
    Instruction *createAlloca(TypePtr type, std::string name = "");
    // 从 pointer 读出一个 type 值
    Instruction *createLoad(TypePtr type, Value *pointer, std::string name = "");
    // 把 value 写入 pointer 指向的内存
    Instruction *createStore(Value *value, Value *pointer);

    // ---- 算术（符号无关）----
    Instruction *createAdd(Value *lhs, Value *rhs, std::string name = "");
    Instruction *createSub(Value *lhs, Value *rhs, std::string name = "");
    Instruction *createMul(Value *lhs, Value *rhs, std::string name = "");

    // ---- 算术（符号相关）----
    Instruction *createSDiv(Value *lhs, Value *rhs, std::string name = "");
    Instruction *createUDiv(Value *lhs, Value *rhs, std::string name = "");
    Instruction *createSRem(Value *lhs, Value *rhs, std::string name = "");
    Instruction *createURem(Value *lhs, Value *rhs, std::string name = "");

    // ---- 移位 ----
    Instruction *createShl(Value *lhs, Value *rhs, std::string name = "");
    Instruction *createLShr(Value *lhs, Value *rhs, std::string name = "");
    Instruction *createAShr(Value *lhs, Value *rhs, std::string name = "");

    // ---- 位运算 ----
    Instruction *createAnd(Value *lhs, Value *rhs, std::string name = "");
    Instruction *createOr(Value *lhs, Value *rhs, std::string name = "");
    Instruction *createXor(Value *lhs, Value *rhs, std::string name = "");

    // ---- 整数比较 ----
    Instruction *createICmp(ICmpPred pred, Value *lhs, Value *rhs,
                            std::string name = "");

    // ---- 类型转换 ----
    Instruction *createZExt(Value *value, TypePtr dst, std::string name = "");
    Instruction *createSExt(Value *value, TypePtr dst, std::string name = "");
    Instruction *createTrunc(Value *value, TypePtr dst, std::string name = "");
    Instruction *createBitCast(Value *value, TypePtr dst, std::string name = "");
    Instruction *createPtrToInt(Value *value, TypePtr dst, std::string name = "");
    Instruction *createIntToPtr(Value *value, TypePtr dst, std::string name = "");

    // ---- 取元素地址 ----
    // sourceType 为被索引的源元素类型；base 为基址指针，indices 为下标
    Instruction *createGetElementPtr(TypePtr sourceType, Value *base,
                                     std::vector<Value *> indices,
                                     std::string name = "");

    // ---- 控制流 ----
    Instruction *createBr(BasicBlock *target); // 无条件跳转
    Instruction *createCondBr(Value *condition, BasicBlock *thenBlock,
                              BasicBlock *elseBlock); // 条件跳转
    // phi 节点：incoming 为 (值, 来源块) 列表
    Instruction *createPhi(TypePtr type,
                           std::vector<std::pair<Value *, BasicBlock *>> incoming,
                           std::string name = "");
    Instruction *createRet(Value *value = nullptr); // 返回；value 为空表示 void
    Instruction *createUnreachable();

    // ---- 调用 ----
    Instruction *createCall(Function *callee, std::vector<Value *> arguments,
                            std::string name = "");

    // ---- 名字分配 ----
    // 返回下一个 SSA 名字：hint 非空时优先用可读名，否则用自动编号
    std::string nextName(const std::string &hint = "");

    // 当前模块（只读访问）
    Module &module();

  private:
    Module &module_;
    BasicBlock *insertPoint_ = nullptr; // 当前插入基本块
    unsigned nameCounter_ = 0;          // 自动编号计数器
};

} // namespace rx::ir
