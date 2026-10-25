#pragma once

#include <map>
#include <string>
#include <vector>

namespace rx::semantic {

// 常量依赖图与环检测（规范 const_eval.html）。
// 在求值之前先建图检测环：自引用或间接环（A→B→A）是编译错误。
// 节点以常量名（顶层常量 / 关联常量用其路径字符串）标识。
class ConstDepGraph {
  public:
    // 添加一条依赖边：from 引用 to。
    void addDependency(const std::string &from, const std::string &to);

    // 是否存在环。
    bool hasCycle() const;

    // 若存在环，返回环上的一个节点序列（用于诊断）；无环返回空。
    std::vector<std::string> findCycle() const;

  private:
    // 邻接表：常量名 → 其依赖的常量名列表。
    std::map<std::string, std::vector<std::string>> edges_;
};

} // namespace rx::semantic
