#pragma once

#include "ir_type.hpp"

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace rx::ir {

class Value;
class Constant;
class Instruction;
class Argument;
class BasicBlock;
class Function;
class GlobalVariable;
class Module;

using ConstantPtr = std::shared_ptr<Constant>;

// ===========================================================================
// Value：所有 IR 实体的基类
// ===========================================================================
// 对应 LLVM 的 Value。一个 Value 拥有一个类型与一个可选的 SSA 名字。
// 指令、常量、参数、基本块、函数、全局变量都是 Value 的派生类型。
class Value {
  public:
    virtual ~Value() = default;

    TypePtr type; // 该值的 IR 类型

    // SSA 名字（不含前缀）。打印时按类别加前缀：
    //   局部值/基本块 -> %name   函数/全局变量 -> @name
    std::string name;

    bool hasName = false; // 是否已分配名字（未命名时由打印器自动编号）
};

// ===========================================================================
// Constant：编译期常量（派生自 Value）
// ===========================================================================
enum class ConstantKind {
    Int,    // iN 整数常量（含 bool，用 i1/i8 存储）
    Null,   // null 指针常量
    Undef,  // undef 占位常量
    Array,  // 数组常量：[i32 1, i32 2]
    Struct, // 结构体常量：{ i32 1, i32 2 }
    String, // 字符串字面量：本身是 ptr 类型，指向 stringGlobal 全局常量
};

struct Constant final : Value {
    ConstantKind constantKind = ConstantKind::Int;

    // ---- Int ----
    unsigned bitWidth = 0;      // 位宽
    std::uint64_t intValue = 0; // 数值（按无符号存，打印时按位宽截断）

    // ---- Array / Struct ----
    std::vector<ConstantPtr> elements; // 元素 / 字段常量

    // ---- String ----
    std::string stringValue; // 字符串原始内容
    // 字符串对应的全局常量（IRBuilder::getGlobalString 负责创建与命名）
    std::shared_ptr<GlobalVariable> stringGlobal;
};

// ===========================================================================
// Instruction：一条 IR 指令（派生自 Value）
// ===========================================================================
// 操作码。算术/逻辑指令按"有无符号"细分，把符号性判断留在 codegen 层，
// 使 IRBuilder 本身保持类型无关。
enum class Opcode {
    // 内存
    Alloca, Load, Store,
    // 算术（符号无关）
    Add, Sub, Mul,
    // 算术（符号相关）
    SDiv, UDiv, SRem, URem, // 有/无符号 除、取余
    // 移位
    Shl, LShr, AShr, // 左移、逻辑右移、算术右移
    // 位运算
    And, Or, Xor,
    // 整数比较
    ICmp,
    // 类型转换
    ZExt, SExt, Trunc, BitCast, PtrToInt, IntToPtr,
    // 控制流
    Br, CondBr, Phi, Ret, Unreachable,
    // 取元素地址
    GetElementPtr,
    // 调用
    Call,
};

// 整数比较谓词（与 LLVM icmp 一一对应）
enum class ICmpPred {
    EQ,  // ==
    NE,  // !=
    ULT, // 无符号 <
    ULE, // 无符号 <=
    UGT, // 无符号 >
    UGE, // 无符号 >=
    SLT, // 有符号 <
    SLE, // 有符号 <=
    SGT, // 有符号 >
    SGE, // 有符号 >=
};

struct Instruction final : Value {
    Opcode opcode = Opcode::Add;

    // 通用操作数（原始指针，由所属基本块持有所有权）。各指令解读方式：
    //   Store         : operands[0] = 被存的值，operands[1] = 目标指针
    //   算术/位运算/icmp : operands[0]/[1] = 左右操作数
    //   转换/取址     : operands[0] = 被转换/取址的值
    //   Load/Alloca   : 不使用（类型信息见 type / sourceType）
    //   Call          : 实参列表
    std::vector<Value *> operands;

    // ---- ICmp ----
    ICmpPred icmpPred = ICmpPred::EQ;

    // ---- Alloca / GetElementPtr ----
    // Alloca：分配的元素类型（打印为 alloca <type>，结果为 ptr）
    // GetElementPtr：被索引的源元素类型（打印为 getelementptr <type>, ...）
    TypePtr sourceType;

    // ---- Phi ----
    // incoming[i] = (值, 来自哪个基本块)
    std::vector<std::pair<Value *, BasicBlock *>> incoming;

    // ---- Call ----
    Function *callee = nullptr; // 被调函数；返回类型 = callee->resultType

    // ---- Br / CondBr ----
    BasicBlock *trueTarget = nullptr;  // CondBr 的 then 目标 / Br 的目标
    BasicBlock *falseTarget = nullptr; // CondBr 的 else 目标
};

// ===========================================================================
// Argument：函数形参（派生自 Value）
// ===========================================================================
struct Argument final : Value {
    Function *parent = nullptr; // 所属函数
    unsigned index = 0;         // 参数序号（从 0 开始，对应 Function::paramTypes）
};

// ===========================================================================
// BasicBlock：基本块（派生自 Value，打印为 label）
// ===========================================================================
class BasicBlock final : public Value {
  public:
    Function *parent = nullptr; // 所属函数

    // 指令序列（含终止指令）
    std::vector<std::unique_ptr<Instruction>> instructions;

    // 是否已包含终止指令（br / ret / unreachable）
    bool hasTerminator() const;

    // 追加一条指令（转移所有权）；返回裸指针便于继续使用
    Instruction *append(std::unique_ptr<Instruction> inst);
};

// ===========================================================================
// Function：函数（派生自 Value，打印为 define/declare）
// ===========================================================================
class Function final : public Value {
  public:
    TypePtr resultType;             // 返回类型
    std::vector<TypePtr> paramTypes; // 形参类型列表

    // 形参对象（与 paramTypes 一一对应）
    std::vector<std::unique_ptr<Argument>> arguments;

    bool isDeclaration = false; // 是否仅为声明（declare，无函数体）
    bool isVarArg = false;      // 是否可变参数（如 printf 的 ...）

    // 基本块列表
    std::vector<std::unique_ptr<BasicBlock>> blocks;

    // 入口基本块（第一个块）；无块时返回 nullptr
    BasicBlock *entryBlock() const;

    // 新建基本块并追加到末尾；name 为空则由打印器编号
    BasicBlock *appendBlock(std::string name = "");
};

// ===========================================================================
// GlobalVariable：全局变量 / 常量（派生自 Value，打印为 @name）
// ===========================================================================
class GlobalVariable final : public Value {
  public:
    bool isConstant = false; // 是否 const（字符串字面量、常量数组等）

    ConstantPtr initializer; // 初始值；为空表示未初始化（extern）
};

// ===========================================================================
// Module：整个编译单元
// ===========================================================================
class Module {
  public:
    // 目标三元组与数据布局（打印在文件头部）
    std::string targetTriple = "riscv32-unknown-none-elf";
    std::string dataLayout;

    // 全局变量列表
    std::vector<std::unique_ptr<GlobalVariable>> globals;

    // 函数列表（含 declare 的外部函数）
    std::vector<std::unique_ptr<Function>> functions;

    // 按名字查找函数（找不到返回 nullptr）
    Function *getFunction(const std::string &name) const;

    // 新建并注册全局变量 / 函数（转移所有权）；返回裸指针便于继续使用
    GlobalVariable *addGlobal(std::unique_ptr<GlobalVariable> global);
    Function *addFunction(std::unique_ptr<Function> function);

    // 打印整个模块为 .ll 文本（委托给 IRPrinter，见 ir_printer.hpp）
    void print(std::ostream &out) const;
};

} // namespace rx::ir
