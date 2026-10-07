#include "antlr4-runtime.h"
#include "Lexer.h"
#include "Parser.h"

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
  if (argc > 2) {
    std::cerr << "usage: rx-parse [source.rx]\n";
    return 2;
  }

  std::unique_ptr<std::istream> file;
  std::istream *source = &std::cin;
  if (argc == 2) {
    auto input = std::make_unique<std::ifstream>(argv[1], std::ios::binary);
    if (!*input) {
      std::cerr << "cannot open source file: " << argv[1] << '\n';
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
  std::cout << tree->toStringTree(&parser) << '\n';
  return errors.count == 0 ? 0 : 1;
}
