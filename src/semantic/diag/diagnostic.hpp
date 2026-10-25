#pragma once

#include "span.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace rx::semantic {

// 诊断收集器：累积语义错误，而非遇错即停。
// 语义测试只关心最终退出码（0 接受 / 1 拒绝），
// 因此这里只保存人类可读的消息；位置信息可选。
class Diagnostic {
  public:
    // 记录一条错误消息。
    void error(const std::string &message);

    // 记录一条带源码位置的错误消息。
    void error(const Span &span, const std::string &message);

    // 是否存在错误。
    bool hasErrors() const;

    // 错误条数。
    std::size_t errorCount() const;

    // 全部错误消息（供最终打印到 stderr）。
    const std::vector<std::string> &messages() const;

    // 清空（如需复用同一收集器）。
    void clear();

  private:
    std::vector<std::string> messages_;
};

} // namespace rx::semantic
