#pragma once

#include "span.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace rx::semantic {

class Diagnostic {
  public:
    void error(const std::string &message);
    void error(const Span &span, const std::string &message);
    bool hasErrors() const;
    std::size_t errorCount() const;
    const std::vector<std::string> &messages() const;
    void clear();

  private:
    std::vector<std::string> messages_;
};

} // namespace rx::semantic
