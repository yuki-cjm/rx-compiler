#include "const_eval.hpp"

#include "../resolve/resolver.hpp"
#include "../ty/ty_compare.hpp"

#include <cstdint>
#include <set>

namespace rx::semantic {

namespace {

// 解析后的整数字面量：类型 + 32 位无符号位型（负值以补码表示）。
struct ParsedInt {
    TyPtr type;
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

// 判断文本是否以给定后缀结尾（用于整数字面量后缀检测）。
bool endsWith(const std::string &text, const std::string &suffix) {
    return text.size() >= suffix.size() &&
           text.compare(text.size() - suffix.size(), suffix.size(), suffix) ==
               0;
}

std::optional<ParsedInt> parseInteger(const std::string &text,
                                      const std::optional<TyPtr> &expected) {
    std::string body = text;
    TyPtr type;

    if (endsWith(body, "usize")) {
        type = Ty::makeScalar(TyKind::Usize);
        body.resize(body.size() - 5);
    } else if (endsWith(body, "isize")) {
        type = Ty::makeScalar(TyKind::Isize);
        body.resize(body.size() - 5);
    } else if (endsWith(body, "u32")) {
        type = Ty::makeScalar(TyKind::U32);
        body.resize(body.size() - 3);
    } else if (endsWith(body, "i32")) {
        type = Ty::makeScalar(TyKind::I32);
        body.resize(body.size() - 3);
    } else if (expected && isInteger(expected.value())) {
        type = expected.value();
    } else {
        type = Ty::makeScalar(TyKind::I32);
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

// 把常量路径文本按 "::" 拆成段。
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

// 求值器状态（memo + 环检测），线程局部以便各入口共享。
thread_local std::map<const Symbol *, std::optional<EvaluatedConst>> memo_;
thread_local std::set<const Symbol *> visiting_;

// 解析常量路径，返回常量符号；失败返回 nullptr 并写 diag。
const Symbol *resolveConstPath(SymbolTable &table, Diagnostic &diag,
                               const std::string &text,
                               const TyPtr &selfType) {
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
            if (typeSymbol == nullptr || typeSymbol->kind != SymbolKind::Struct) {
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

// 求值一个常量符号的初值（memo 化 + 环检测）。
std::optional<EvaluatedConst> evaluateSymbol(SymbolTable &table,
                                             Diagnostic &diag,
                                             const Symbol *symbol) {
    auto found = memo_.find(symbol);
    if (found != memo_.end()) {
        return found->second;
    }
    if (visiting_.count(symbol) != 0) {
        diag.error("constant dependency cycle involving '" + symbol->name + "'");
        return std::nullopt;
    }

    visiting_.insert(symbol);

    TypeResolver resolver(table, diag);
    TyPtr selfType = symbol->implTarget != 0
                         ? Ty::makeStruct(symbol->implTarget)
                         : nullptr;
    TyPtr declared = resolver.resolve(*symbol->constant->type, selfType);

    std::optional<EvaluatedConst> result;
    if (declared != nullptr) {
        result = evaluateConst(table, diag, symbol->constant->value, declared,
                               selfType);
    }

    visiting_.erase(symbol);
    memo_[symbol] = result;
    return result;
}

std::optional<EvaluatedConst> matchExpected(Diagnostic &diag,
                                            EvaluatedConst result,
                                            const TyPtr &expectedType) {
    if (expectedType != nullptr && !sameTy(result.type, expectedType)) {
        diag.error("constant initializer does not match its declared type");
        return std::nullopt;
    }
    return result;
}

} // namespace

std::optional<EvaluatedConst> evaluateConst(SymbolTable &table,
                                            Diagnostic &diag,
                                            const ast::ConstValue &value,
                                            const TyPtr &expectedType,
                                            const TyPtr &selfType) {
    switch (value.kind) {
    case ast::ConstValue::Kind::Integer: {
        auto parsed = parseInteger(value.value, expectedType);
        if (!parsed) {
            diag.error("invalid integer literal '" + value.value + "'");
            return std::nullopt;
        }
        EvaluatedConst result;
        result.type = parsed->type;
        result.intValue = parsed->value;
        return matchExpected(diag, std::move(result), expectedType);
    }
    case ast::ConstValue::Kind::Boolean: {
        if (expectedType != nullptr && !isBool(expectedType)) {
            diag.error("constant initializer does not match its declared type");
            return std::nullopt;
        }
        EvaluatedConst result;
        result.type = Ty::makeScalar(TyKind::Bool);
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
        return matchExpected(diag, std::move(*result), expectedType);
    }
    case ast::ConstValue::Kind::Negate: {
        const ast::Magnitude &magnitude = value.operand;

        std::optional<EvaluatedConst> operand;
        if (magnitude.kind == ast::Magnitude::Kind::Integer) {
            // 期望类型不穿透一元负号：字面量按后缀或默认 i32。
            auto parsed = parseInteger(magnitude.value, std::nullopt);
            if (!parsed) {
                diag.error("invalid integer literal '" + magnitude.value + "'");
                return std::nullopt;
            }
            if (!isSignedInteger(parsed->type)) {
                diag.error("unsigned constant negation is invalid");
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
            diag.error("unsigned constant negation is invalid");
            return std::nullopt;
        }

        std::uint32_t negated =
            static_cast<std::uint32_t>(0 - static_cast<std::uint32_t>(
                                              operand->intValue));
        EvaluatedConst result;
        result.type = operand->type;
        result.intValue = negated;
        return matchExpected(diag, std::move(result), expectedType);
    }
    }
    return std::nullopt;
}

std::optional<std::uint64_t> evaluateArrayLength(SymbolTable &table,
                                                 Diagnostic &diag,
                                                 const ast::ConstValue &value,
                                                 const TyPtr &selfType) {
    auto result = evaluateConst(table, diag, value, Ty::makeScalar(TyKind::Usize),
                                selfType);
    if (!result) {
        return std::nullopt;
    }
    return result->intValue;
}

} // namespace rx::semantic
