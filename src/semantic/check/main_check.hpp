#pragma once

#include "../symbol/symbol_table.hpp"
#include "../diag/diagnostic.hpp"

namespace rx::semantic {

// 程序入口 main 函数检查（规范 items/functions.html 的「Program entry」）。
//   - 必须存在顶层 main；
//   - main 不能有值参数；
//   - main 不能有泛型参数；
//   - main 必须返回 unit（无返回标注或显式 ()）。
class MainChecker {
  public:
    MainChecker(SymbolTable &table, Diagnostic &diag);

    // 执行 main 检查；发现的错误写入 diag。
    void check();

  private:
    SymbolTable &table_;
    Diagnostic &diag_;
};

} // namespace rx::semantic
