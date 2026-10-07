#!/usr/bin/env python3
"""Qualify ANTLR 4.13.2 C++ definitions emitted with -package rx."""

from pathlib import Path
import re


GENERATED = Path(__file__).resolve().parent.parent / "generated"


def update(name, replacements, symbol_pattern):
    path = GENERATED / name
    try:
        source = path.read_text(encoding="utf-8")
    except OSError as error:
        raise SystemExit(f"cannot read {path}: {error}") from error

    if "using namespace rx;" not in source:
        raise SystemExit(f"{path} was not generated with ANTLR's -package rx option")
    if re.search(symbol_pattern, source) is None:
        return

    for pattern, replacement, expected_count in replacements:
        source, changed = re.subn(pattern, replacement, source, count=expected_count)
        if changed == 0:
            raise SystemExit(f"expected a match for {pattern!r} in {path}")
        if expected_count and changed != expected_count:
            raise SystemExit(
                f"expected {expected_count} matches for {pattern!r} in {path}, got {changed}"
            )

    try:
        path.write_text(source, encoding="utf-8")
    except OSError as error:
        raise SystemExit(f"cannot write {path}: {error}") from error


def main():
    update(
        "Lexer.cpp",
        [
            (r"(?<![\w:])Lexer::", "rx::Lexer::", 11),
            (
                r"(?m)^rx::Lexer::Lexer\((.*?)\) : Lexer\((.*?)\) \{$",
                r"rx::Lexer::Lexer(\1) : antlr4::Lexer(\2) {",
                1,
            ),
        ],
        r"(?<![\w:])Lexer::",
    )
    update(
        "Parser.cpp",
        [
            (r"(?<![\w:])Parser::", "rx::Parser::", 0),
            (
                r" : Parser\(input, antlr4::atn::ParserATNSimulatorOptions\(\)\) \{\}",
                r" : rx::Parser(input, antlr4::atn::ParserATNSimulatorOptions()) {}",
                1,
            ),
            (r"\) : Parser\(input\) \{", r") : antlr4::Parser(input) {", 1),
        ],
        r"(?<![\w:])Parser::",
    )


if __name__ == "__main__":
    main()
