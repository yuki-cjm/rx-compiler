#include "integer_infer.hpp"

namespace rx::semantic {

namespace {

bool endsWith(const std::string &text, const std::string &suffix) {
    return text.size() >= suffix.size() &&
           text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

} // namespace

TypePtr IntegerInfer::infer(const std::string &literalText,
                            const std::optional<TypePtr> &expected) {
    if (endsWith(literalText, "usize")) {
        return Type::makeScalar(TypeKind::Usize);
    }
    if (endsWith(literalText, "isize")) {
        return Type::makeScalar(TypeKind::Isize);
    }
    if (endsWith(literalText, "u32")) {
        return Type::makeScalar(TypeKind::U32);
    }
    if (endsWith(literalText, "i32")) {
        return Type::makeScalar(TypeKind::I32);
    }
    if (expected && isInteger(*expected)) {
        return *expected;
    }
    return Type::makeScalar(TypeKind::I32);
}

} // namespace rx::semantic
