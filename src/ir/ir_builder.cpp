#include "ir_builder.hpp"

namespace rx::ir {

//   1. 构造 std::make_unique<Instruction>()，设置 opcode / type / operands 等字段；
//   2. 调用 nextName(hint) 分配名字（void 类型指令可跳过）；
//   3. insertPoint_->append(std::move(inst)) 追加到当前基本块；
//   4. 返回裸指针。

} // namespace rx::ir
