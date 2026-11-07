#include "diagnostic.hpp"

namespace rx::semantic {

void Diagnostic::error(const std::string &message) {
    messages_.push_back(message);
}

void Diagnostic::error(const Span &span, const std::string &message) {
    if (span.line != 0) {
        messages_.push_back("line " + std::to_string(span.line) + ":" +
                            std::to_string(span.column) + ": " + message);
    } else {
        messages_.push_back(message);
    }
}

bool Diagnostic::hasErrors() const { return !messages_.empty(); }

std::size_t Diagnostic::errorCount() const { return messages_.size(); }

const std::vector<std::string> &Diagnostic::messages() const {
    return messages_;
}

void Diagnostic::clear() { messages_.clear(); }

} // namespace rx::semantic
