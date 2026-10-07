# Rx Compiler Template

[![Tests](https://github.com/ACMClassCourse-2025/rx-compiler/actions/workflows/test.yml/badge.svg)](https://github.com/ACMClassCourse-2025/rx-compiler/actions/workflows/test.yml)
[![Target: RV32IM](https://img.shields.io/badge/target-RV32IM-283272)](https://msyksphinz-self.github.io/riscv-isadoc/)
[![Simulator: REIMU](https://img.shields.io/badge/simulator-REIMU-d73a49)](https://github.com/wanoful/REIMU)
[English](README-EN.md) | [简体中文](README-ZH.md)

> Replace this with your own README when you start working on your compiler.

## Getting Started

Welcome to the Rx Compiler course! This repository provides a template from which you can build your own compiler for [the Rx programming language](https://acmclasscourse-2025.github.io/rx-compiler-specification/). It includes official testcases, a test scaffold, and the G4 representation of Rx to get you started.

We strongly recommend that you **fork this repository** instead of downloading zips, in case we need to update the official testcases. After forking your copy, clone it to your local machine.

Initialize the testcases and [REIMU](https://github.com/wanoful/REIMU) submodules:

```sh
git submodule update --init --recursive
```

Python 3, [xmake](https://xmake.io/) and any C++23 compiler is needed to build REIMU. Build REIMU separately from the project root before running tests:

```sh
xmake f -y -P vendor/REIMU -m release -o target/reimu
xmake -y -P vendor/REIMU
```

Rerun these commands after updating the REIMU submodule.

If you want to run default `rustc` test, install `rustup`, and run
```sh
rustup target add riscv32im-unknown-none-elf
```

IR tests also require **Clang 22**.

## Build and run the C++ parser locally

The grammar and generated class names remain `Lexer` and `Parser`; generate them with ANTLR's `-package rx` option to put them in the `rx` namespace and avoid confusion with the runtime's `antlr4::Lexer` and `antlr4::Parser`. Building requires CMake, a C++17 compiler, and the ANTLR 4.13.2 C++ runtime installed on the system.

To regenerate the lexer and parser, obtain the ANTLR 4.13.2 complete jar (the ANTLR tool and runtime are separate dependencies), then run from the repository root:

```sh
curl -fL https://www.antlr.org/download/antlr-4.13.2-complete.jar -o /tmp/antlr-4.13.2-complete.jar
java -jar /tmp/antlr-4.13.2-complete.jar -Dlanguage=Cpp -package rx -visitor -listener -Xexact-output-dir -o generated grammar/Lexer.g4
java -jar /tmp/antlr-4.13.2-complete.jar -Dlanguage=Cpp -package rx -visitor -listener -Xexact-output-dir -lib generated -o generated grammar/Parser.g4
```

Then build the parser and parse an Rx source file. Before compilation, CMake automatically qualifies the generated `.cpp` definitions with `rx::` (ANTLR 4.13.2's `-package` option namespaces the headers but leaves these member definitions unqualified):

```sh
cmake -S . -B build/parser
cmake --build build/parser --target rx-parse -j2
./build/parser/rx-parse tests/official/parser/accept/0005_fn_item-7c27348778.rx
```

`rx-parse` prints the parse tree to stdout; without a filename, it reads from stdin. Syntax errors return exit code `1`; command-line or file errors return `2`. Ubuntu's packaged runtime may be older than the generated code; to use matching version 4.13.2, build and install the C++ runtime from ANTLR 4.13.2 source. The project then finds the system-installed CMake package and does not need an ANTLR source checkout.

```sh
curl -fL https://github.com/antlr/antlr4/archive/refs/tags/4.13.2.tar.gz -o /tmp/antlr4-4.13.2.tar.gz
tar -xzf /tmp/antlr4-4.13.2.tar.gz -C /tmp
cmake -S /tmp/antlr4-4.13.2/runtime/Cpp -B /tmp/antlr4-4.13.2-build \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local \
  -DANTLR4_INSTALL=ON -DANTLR_BUILD_CPP_TESTS=OFF
cmake --build /tmp/antlr4-4.13.2-build -j2
sudo cmake --install /tmp/antlr4-4.13.2-build
sudo ldconfig
```

## Overview

In this course you can use **any language** to implement your compiler. Contact the TA if your language is not mainstream so that we can provide support for it on the Online Judge. For this reason, the template we provide here is **language-agnostic**. You will find:

- Testcases under `tests/`. Official testcases reside under `tests/official` and is a git submodule. You may add your own testcases under `tests/custom`.
- A test runner. The `Makefile` runs testcases using the compiler configured in `config.mk`. By default it uses your system rustc with the `riscv32im-unknown-none-elf` target and executes generated assembly in REIMU. **Replace the compiler commands in `config.mk` with your own compiler commands** to set up testing for your compiler. See [Running tests](#running-tests) for details.
    - Some auxiliary files (such as the reference compiler helper code in `crates/rx`) help the default rustc produce assembly suitable for REIMU. Its `src/entry.rs` is compiled separately for each testcase and supplies the bare-metal entry point and `Box`/`Vec` imports. The library implements integer I/O through REIMU's libc, plus allocation and panic handling for bare-metal RV32 targets in `src/runtime.rs`. The reference IR and codegen commands request LLVM IR or assembly together with a static library so rustc performs whole-program LTO and includes the runtime in the generated program. The extra `{output}.a` is a build artifact. Codegen strips debug metadata with `scripts/strip_asm_debug.py`; the IR runner lowers `.ll` with Clang and strips the same unsupported metadata. In both paths, `RUN` receives assembly as `{output}`. These helpers can be removed when you replace the Rust compiler commands.
- REIMU under `vendor/REIMU`, pinned as a git submodule. The `RUN` command in `config.mk` invokes it; the test runner does not depend on a particular simulator.
- G4 grammar for Rx under `grammar/`. You may use it to generate the lexer and parser for your compiler.

## Setting up the Makefile

The Makefile is our unified entrypoint in accessing your compiler. You are expected to edit [`config.mk`](config.mk) and hook in your compiler commands. In practice, specify in these fields:

| Command Name | Purpose |
| --- | --- |
| `BUILD` | Build your compiler once; may be empty. Must exit 0. May write shared runtime assembly to `{runtime}`. |
| `SEMANTIC` | Check a complete program through semantic analysis. Exit 0 to accept or 1 to reject. |
| `IR` | Write LLVM IR to `{output}` (`.ll`). The runner lowers it to RV32IM assembly with Clang before `RUN`. |
| `CODEGEN` | Compile codegen and optimization testcases and write RV32IM assembly to `{output}` for the default `RUN`. |
| `RUN` | Run assembly `{output}` with the captured `{runtime}`. Optional `{stdout}` and `{profile}` placeholders select per-execution output and profiling files. |

For example, if your compiler supports `--stage` and `-o` and emits RV32IM assembly:

```make
BUILD = cargo build --release && cp runtime.s {runtime}
SEMANTIC = ./target/release/compiler --stage semantic {source}
IR = ./target/release/compiler --stage ir {source} -o {output}
CODEGEN = ./target/release/compiler --stage codegen {source} -o {output}
RUN = xmake run -P vendor/REIMU reimu -f {output},{runtime} -o {stdout} -p {profile} 1>&2
```

REIMU starts at the assembly's global `main` symbol and provides its supported libc functions. `-o {stdout}` saves the program's output for comparison, `-p {profile}` saves its cycle profile, and `1>&2` sends simulator status messages to the stderr log. Do not add `--silent` when collecting cycles: REIMU suppresses profiles in silent mode.

Write any required runtime assembly to `{runtime}` during `BUILD`. Exported functions must follow the ILP32 calling convention. Leave the file empty if the runtime is embedded in the generated program.

IR and codegen use the same testcases. IR uses your `IR` command, then the runner invokes Clang with `-S -x ir --target=riscv32-unknown-none-elf -march=rv32im -mabi=ilp32 -O0 -mllvm -riscv-no-aliases`; codegen uses the assembly produced by `CODEGEN` directly.

## Testcases

Tests reside in `tests/`, and are organized into subdirectories. We recommend you follow the "namespace:test-suite:testcase" pattern. For instance, `official:semantic:arrays` is the `arrays` testcase in the `semantic` test suite of the `official` namespace.

Each testcase can have one or more source files, optional input and output files and a compulsory `manifest.json` file which defines the format of the testcase. See [the official schema](tests/official/manifest.schema.json) for details. The manifest's `stage` argument determines how the testcase runs: `semantic` uses `SEMANTIC`; each `codegen` entry runs both `IR` → Clang → `RUN` and `CODEGEN` → `RUN`; `optimization` uses `CODEGEN` followed by `RUN`.

You are encouraged to add your own testcases under `tests/custom`. The runner will find them automatically.

Requirements for each kind of testcase:

| Testcase Type | Requirements |
| --- | --- |
| Semantic | The compiler must exit 0 or 1 to match `compilation_success`. Other exit codes, signals, and timeouts fail the case. |
| IR | Compilation must create LLVM `.ll`; Clang must lower it successfully, and every runtime output must match. |
| Codegen | Compilation must exit 0 and create `{output}`. Each `io` pair runs the artifact and the output must match the expected file. |
| Optimization | Same as Codegen, with cycle reporting when `RUN` provides `{profile}`. |

Testcases with type `lex` and `parse` will be skipped since we already provide the G4 grammar. Extend the Makefile if you want to DIY these stages.

## Running tests

Run from the project root:

```sh
make test
make test FILTER=official:semantic
make test FILTER=official:codegen:arrays,official:optimization
make test STAGE=ir
make test STAGE=codegen FILTER=official:codegen:arrays
make test STAGE=ir CLANG=clang-22
make test FILTER=custom
make test FILTER=official:optimization COMPILE_TIMEOUT=60 RUN_TIMEOUT=30
make test VERBOSE=true
```

Supported environment variables include:

- `FILTER`, which selects directories under `tests`, using `:` between folder names and `,` between selections. Omit `FILTER` or leave it empty to run all supported tests.
- `STAGE`: optionally run only `semantic`, `ir`, `codegen`, or `optimization`. A directory `FILTER` for codegen fixtures includes both IR and codegen unless `STAGE` restricts it.
- `CLANG`: LLVM IR lowering tool, default `clang-22`.
- `VERBOSE=true`, which shows every test name and its duration instead of grouped progress. Defaults to `false`.
- `COMPILE_TIMEOUT` and `RUN_TIMEOUT`, which override the default timeouts for compilation and execution.

## CI/CD support

This repo comes with a GitHub Actions workflow that runs tests on every push and pull request. You can inspect or modify the workflow in `.github/workflows/`. We advise turning it off while you develop the compiler and turning it back on when the compiler is ready for test-based development.

To disable the test, rename `.github/workflows/test.yml` to `.github/workflows/test.yml.disabled`. GitHub Actions will only discover `*.yml` and `*.yaml` files.
