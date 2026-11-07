#include "const_eval.hpp"

#include "../resolve/resolver.hpp"
#include "../type/type_compare.hpp"

#include <cstdint>
#include <set>

namespace rx::semantic {

namespace {

struct ParsedInt {
    TypePtr type;
    std::uint64_t value;
};

int digitValue(char character) {
    if (character >= '0' && character <= '9') {
        return character - '0';
    }
    if (character >= 'a' && character <= 'f') {
        return character - 'a' + 10;
    }
    if (character >= 'A' && character <= 'F') {
        return character - 'A' + 10;
    }
    return -1;
}

// 后缀
bool endsWith(const std::string &text, const std::string &suffix) {
    return text.size() >= suffix.size() &&
           text.compare(text.size() - suffix.size(), suffix.size(), suffix) ==
               0;
}

std::optional<ParsedInt> parseInteger(const std::string &text,
                                      const std::optional<TypePtr> &expected) {
    std::string body = text;
    TypePtr type;

    if (endsWith(body, "usize")) {
        type = Type::makeScalar(TypeKind::Usize);
        body.resize(body.size() - 5);
    } else if (endsWith(body, "isize")) {
        type = Type::makeScalar(TypeKind::Isize);
        body.resize(body.size() - 5);
    } else if (endsWith(body, "u32")) {
        type = Type::makeScalar(TypeKind::U32);
        body.resize(body.size() - 3);
    } else if (endsWith(body, "i32")) {
        type = Type::makeScalar(TypeKind::I32);
        body.resize(body.size() - 3);
    } else if (expected && isInteger(expected.value())) {
        type = expected.value();
    } else {
        type = Type::makeScalar(TypeKind::I32);
    }

    int base = 10;
    std::size_t offset = 0;
    if (body.size() >= 2 && body[0] == '0') {
        if (body[1] == 'x' || body[1] == 'X') {
            base = 16;
            offset = 2;
        } else if (body[1] == 'o' || body[1] == 'O') {
            base = 8;
            offset = 2;
        } else if (body[1] == 'b' || body[1] == 'B') {
            base = 2;
            offset = 2;
        }
    }

    std::uint64_t value = 0;
    bool hasDigit = false;
    for (std::size_t index = offset; index < body.size(); ++index) {
        char character = body[index];
        if (character == '_') {
            continue;
        }
        int digit = digitValue(character);
        if (digit < 0 || digit >= base) {
            return std::nullopt;
        }
        value = value * static_cast<std::uint64_t>(base) +
                static_cast<std::uint64_t>(digit);
        hasDigit = true;
    }
    if (!hasDigit) {
        return std::nullopt;
    }

    ParsedInt result;
    result.type = std::move(type);
    result.value = value;
    return result;
}

std::vector<std::string> splitPath(const std::string &text) {
    std::vector<std::string> segments;
    std::size_t start = 0;
    while (start <= text.size()) {
        std::size_t end = text.find("::", start);
        if (end == std::string::npos) {
            segments.push_back(text.substr(start));
            break;
        }
        segments.push_back(text.substr(start, end - start));
        start = end + 2;
    }
    return segments;
}

std::map<const Symbol *, std::optional<EvaluatedConst>> memo_; // 缓存
std::set<const Symbol *> visiting_;                            // 环检测器

const Symbol *resolveConstPath(SymbolTable &table, Diagnostic &diag,
                               const std::string &text,
                               const TypePtr &selfType) {
    std::vector<std::string> segments = splitPath(text);
    if (segments.size() == 1) {
        const Symbol *symbol = table.resolveValue(segments[0]);
        if (symbol == nullptr) {
            diag.error("unresolved constant name '" + segments[0] + "'");
            return nullptr;
        }
        if (symbol->kind != SymbolKind::Constant) {
            diag.error("'" + segments[0] + "' is not a constant");
            return nullptr;
        }
        return symbol;
    }
    if (segments.size() == 2) {
        StructId structId = 0;
        if (segments[0] == "Self") {
            if (!isStruct(selfType)) {
                diag.error("'Self' used outside a struct context");
                return nullptr;
            }
            structId = selfType->structId;
        } else {
            const Symbol *typeSymbol = table.lookupType(segments[0]);
            if (typeSymbol == nullptr) {
                diag.error("unresolved type name '" + segments[0] + "'");
                return nullptr;
            }
            if (typeSymbol->kind != SymbolKind::Struct) {
                diag.error("'" + segments[0] + "' is not a struct");
                return nullptr;
            }
            structId = typeSymbol->structId;
        }
        const Symbol *symbol = table.lookupAssoc(structId, segments[1]);
        if (symbol == nullptr || symbol->kind != SymbolKind::Constant) {
            diag.error("'" + text + "' is not a constant");
            return nullptr;
        }
        return symbol;
    }
    diag.error("invalid constant path '" + text + "'");
    return nullptr;
}

std::optional<EvaluatedConst>
evaluateSymbol(SymbolTable &table, Diagnostic &diag, const Symbol *symbol) {
    auto found = memo_.find(symbol);
    if (found != memo_.end()) {
        return found->second;
    }
    if (visiting_.count(symbol) != 0) {
        diag.error("constant dependency cycle involving '" + symbol->name +
                   "'");
        return std::nullopt;
    }

    visiting_.insert(symbol);

    TypeResolver resolver(table, diag);
    TypePtr selfType = symbol->implTarget != 0
                           ? Type::makeStruct(symbol->implTarget)
                           : nullptr;
    TypePtr declared = resolver.resolve(*symbol->constant->type, selfType);

    std::optional<EvaluatedConst> result;
    if (declared != nullptr) {
        result = evaluateConst(table, diag, symbol->constant->value, declared,
                               selfType);
    }

    visiting_.erase(symbol);
    memo_[symbol] = result;
    return result;
}

std::optional<EvaluatedConst> matchExpected(Diagnostic &diag, const Span &span,
                                            EvaluatedConst result,
                                            const TypePtr &expectedType) {
    if (expectedType != nullptr && !sameType(result.type, expectedType)) {
        diag.error(span,
                   "constant initializer does not match its declared type");
        return std::nullopt;
    }
    return result;
}

} // namespace

std::optional<EvaluatedConst> evaluateConst(SymbolTable &table,
                                            Diagnostic &diag,
                                            const ast::ConstValue &value,
                                            const TypePtr &expectedType,
                                            const TypePtr &selfType) {
    switch (value.kind) {
        case ast::ConstValue::Kind::Integer: {
            auto parsed = parseInteger(value.value, expectedType);
            if (!parsed) {
                diag.error(value.span,
                           "invalid integer literal '" + value.value + "'");
                return std::nullopt;
            }
            EvaluatedConst result;
            result.type = parsed->type;
            result.intValue = parsed->value;
            return matchExpected(diag, value.span, std::move(result), expectedType);
        }   
        case ast::ConstValue::Kind::Boolean: {
            if (expectedType != nullptr && !isBool(expectedType)) {
                diag.error(value.span,
                           "constant initializer does not match its declared type");
                return std::nullopt;
            }
            EvaluatedConst result;
            result.type = Type::makeScalar(TypeKind::Bool);
            result.boolValue = value.boolValue;
            return result;
        }
        case ast::ConstValue::Kind::Path: {
            const Symbol *symbol =
                resolveConstPath(table, diag, value.value, selfType);
            if (symbol == nullptr) {
                return std::nullopt;
            }
            auto result = evaluateSymbol(table, diag, symbol);
            if (!result) {
                return std::nullopt;
            }
            return matchExpected(diag, value.span, std::move(*result),
                                 expectedType);
        }
        case ast::ConstValue::Kind::Negate: {
            const ast::Magnitude &magnitude = value.operand;

            std::optional<EvaluatedConst> operand;
            if (magnitude.kind == ast::Magnitude::Kind::Integer) {
                auto parsed = parseInteger(magnitude.value, std::nullopt);
                if (!parsed) {
                    diag.error(value.span,
                               "invalid integer literal '" + magnitude.value + "'");
                    return std::nullopt;
                }
                if (!isSignedInteger(parsed->type)) {
                    diag.error(value.span, "unsigned constant negation is invalid");
                    return std::nullopt;
                }
                EvaluatedConst parsedValue;
                parsedValue.type = parsed->type;
                parsedValue.intValue = parsed->value;
                operand = std::move(parsedValue);
            } else {
                const Symbol *symbol =
                    resolveConstPath(table, diag, magnitude.value, selfType);
                if (symbol == nullptr) {
                    return std::nullopt;
                }
                operand = evaluateSymbol(table, diag, symbol);
            }

            if (!operand) {
                return std::nullopt;
            }
            if (!isSignedInteger(operand->type)) {
                diag.error(value.span, "unsigned constant negation is invalid");
                return std::nullopt;
            }

            std::uint32_t negated = static_cast<std::uint32_t>(
                0 - static_cast<std::uint32_t>(operand->intValue));
            EvaluatedConst result;
            result.type = operand->type;
            result.intValue = negated;
            return matchExpected(diag, value.span, std::move(result), expectedType);
        }
    }
    return std::nullopt;
}

std::optional<std::uint64_t> evaluateArrayLength(SymbolTable &table,
                                                 Diagnostic &diag,
                                                 const ast::ConstValue &value,
                                                 const TypePtr &selfType) {
    auto result = evaluateConst(table, diag, value,
                                Type::makeScalar(TypeKind::Usize), selfType);
    if (!result) {
        return std::nullopt;
    }
    return result->intValue;
}

} // namespace rx::semantic
