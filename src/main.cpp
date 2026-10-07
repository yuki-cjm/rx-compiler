#include "antlr4-runtime.h"
#include "Lexer.h"
#include "Parser.h"
#include "ast/ast_builder.hpp"

#include <fstream>
#include <iostream>
#include <memory>

namespace {

class ErrorCounter final : public antlr4::BaseErrorListener {
public:
  void syntaxError(antlr4::Recognizer *, antlr4::Token *, size_t, size_t,
                   const std::string &message, std::exception_ptr) override {
    ++count;
    std::cerr << "syntax error: " << message << '\n';
  }

  size_t count = 0;
};

} // namespace

int main(int argc, char **argv) {
  bool printAst = false;
  const char *sourcePath = nullptr;
  for (int i = 1; i < argc; ++i) {
    const std::string argument = argv[i];
    if (argument == "--ast") {
      printAst = true;
    } else if (argument == "--help") {
      std::cout << "usage: rx-parse [--ast] [source.rx]\n";
      return 0;
    } else if (!argument.empty() && argument.front() == '-') {
      std::cerr << "unknown option: " << argument << '\n'
                << "usage: rx-parse [--ast] [source.rx]\n";
      return 2;
    } else if (sourcePath == nullptr) {
      sourcePath = argv[i];
    } else {
      std::cerr << "only one source file may be specified\n"
                << "usage: rx-parse [--ast] [source.rx]\n";
      return 2;
    }
  }

  std::unique_ptr<std::istream> file;
  std::istream *source = &std::cin;
  if (sourcePath != nullptr) {
    auto input = std::make_unique<std::ifstream>(sourcePath, std::ios::binary);
    if (!*input) {
      std::cerr << "cannot open source file: " << sourcePath << '\n';
      return 2;
    }
    source = input.get();
    file = std::move(input);
  }

  antlr4::ANTLRInputStream input(*source);
  rx::Lexer lexer(&input);
  ErrorCounter errors;
  lexer.removeErrorListeners();
  lexer.addErrorListener(&errors);

  antlr4::CommonTokenStream tokens(&lexer);
  rx::Parser parser(&tokens);
  parser.removeErrorListeners();
  parser.addErrorListener(&errors);

  auto *tree = parser.crate();
  if (errors.count != 0) {
    return 1;
  }

  if (!printAst) {
    std::cout << tree->toStringTree(&parser) << '\n';
    return 0;
  }

  try {
    const rx::ast::Program program = rx::ast::AstBuilder().build(tree);
    program.print(std::cout);
  } catch (const std::runtime_error &error) {
    std::cerr << "AST lowering error: " << error.what() << '\n';
    return 1;
  }
  return 0;
}
