#include "infer.hpp"

namespace rx::semantic {

namespace {

bool endsWith(const std::string &text, const std::string &suffix) {
    return text.size() >= suffix.size() &&
           text.compare(text.size() - suffix.size(), suffix.size(), suffix) ==
               0;
}

} // namespace

bool IntegerInfer::hasSuffix(const std::string &literalText) {
    return endsWith(literalText, "i32") || endsWith(literalText, "u32") ||
           endsWith(literalText, "isize") || endsWith(literalText, "usize");
}

TyPtr IntegerInfer::infer(const std::string &literalText,
                          const std::optional<TyPtr> &expected) {
    if (endsWith(literalText, "usize")) {
        return Ty::makeScalar(TyKind::Usize);
    }
    if (endsWith(literalText, "isize")) {
        return Ty::makeScalar(TyKind::Isize);
    }
    if (endsWith(literalText, "u32")) {
        return Ty::makeScalar(TyKind::U32);
    }
    if (endsWith(literalText, "i32")) {
        return Ty::makeScalar(TyKind::I32);
    }
    if (expected && isInteger(*expected)) {
        return *expected;
    }
    return Ty::makeScalar(TyKind::I32);
}

} // namespace rx::semantic
