cmake_minimum_required(VERSION 3.15)
project(rx_compiler LANGUAGES CXX)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(antlr4-runtime 4.13.2 EXACT CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(rx-parse
    src/main.cpp
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
