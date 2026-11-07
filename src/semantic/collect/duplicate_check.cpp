#include "duplicate_check.hpp"

#include <set>

namespace rx::semantic {

void checkDuplicateFields(const ast::StructItem &item, Diagnostic &diag) {
    std::set<std::string> seen;
    for (const ast::StructField &field : item.fields) {
        if (!seen.insert(field.name).second) {
            diag.error(item.span(), "duplicate field '" + field.name + "'");
        }
    }
}

void checkDuplicateParameters(const ast::FunctionItem &item, Diagnostic &diag) {
    std::set<std::string> seen;
    for (const ast::FunctionParam &parameter : item.parameters) {
        if (!seen.insert(parameter.binding.name).second) {
            diag.error(item.span(),
                       "duplicate parameter '" + parameter.binding.name + "'");
        }
    }
}

} // namespace rx::semantic
