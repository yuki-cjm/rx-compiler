cmake_minimum_required(VERSION 3.15)
project(rx_compiler LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(antlr4-runtime 4.13.2 EXACT CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(rx-parse
    src/main.cpp
    src/ast/ast.cpp
    src/ast/ast_builder.cpp
    src/ast/ast_printer.cpp
    src/semantic/semantic.cpp
    src/semantic/builtin/builtin.cpp
    src/semantic/builtin/builtin_box.cpp
    src/semantic/builtin/builtin_scalar.cpp
    src/semantic/builtin/builtin_traits.cpp
    src/semantic/builtin/builtin_vec.cpp
    src/semantic/check/assign.cpp
    src/semantic/check/calls.cpp
    src/semantic/check/coercion.cpp
    src/semantic/check/control_flow.cpp
    src/semantic/check/infer.cpp
    src/semantic/check/main_check.cpp
    src/semantic/check/operators.cpp
    src/semantic/check/place.cpp
    src/semantic/check/typeck.cpp
    src/semantic/collect/collector.cpp
    src/semantic/collect/duplicate_check.cpp
    src/semantic/collect/impl_collect.cpp
    src/semantic/consteval/const_deps.cpp
    src/semantic/consteval/const_eval.cpp
    src/semantic/diag/diagnostic.cpp
    src/semantic/diag/span.cpp
    src/semantic/resolve/layout_check.cpp
    src/semantic/resolve/path_resolve.cpp
    src/semantic/resolve/resolver.cpp
    src/semantic/symbol/namespace.cpp
    src/semantic/symbol/scope.cpp
    src/semantic/symbol/symbol.cpp
    src/semantic/symbol/symbol_table.cpp
    src/semantic/ty/ty.cpp
    src/semantic/ty/ty_compare.cpp
    src/semantic/ty/ty_print.cpp
    generated/Lexer.cpp
    generated/Parser.cpp
    generated/ParserBaseListener.cpp
    generated/ParserBaseVisitor.cpp
    generated/ParserListener.cpp
    generated/ParserVisitor.cpp
)
target_include_directories(rx-parse PRIVATE generated)
target_link_libraries(rx-parse PRIVATE antlr4_static)

add_custom_target(qualify-antlr-generated
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/qualify_antlr_cpp.py"
    DEPENDS generated/Lexer.cpp generated/Parser.cpp
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    VERBATIM
)
add_dependencies(rx-parse qualify-antlr-generated)
