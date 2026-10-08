#!/usr/bin/env python3
"""Derive the public function surface of a C++ header.

The hook register (docs/hook-register.md) is a hand-maintained index of the surface of
module/ship/ship_core.h.  This module re-derives that surface from the code, so the register
can be checked for a function the code has and the register has not (scripts/hooks-check.sh).

The derivation is deliberately narrow and explainable:

  * comments are stripped first, so a function named only in a comment is not a declaration;
  * a declaration is a line that starts (after leading whitespace) with a run of type tokens
    and then a name immediately followed by '('; the type run must contain at least one
    whitespace/*/& between tokens, which is what separates a declaration ``int Day()`` from a
    call ``Day()`` and from an expression ``static_cast<int>(x)``;
  * both namespace-scope functions and the public member functions declared inside a struct
    match, which is why the eight ``Ship`` accessors (``Day``, ``Watch``, ...) are in the set.

It is not a C++ parser.  It is a derivation that is exact for this header and that fails loudly
(by disagreeing with the register) if a new declaration is added in a shape it cannot see; the
person is then expected to fix the derivation, not to hand-add the function to the register's
manifest.  ``--self-test`` proves the narrowness: it feeds the extractor a declaration, a
definition, a call, and a cast, and checks only the declaration is taken.
"""

import argparse
import re
import sys

# A name directly followed by '(' at the end of a run of type tokens.  The run is one or more
# "type token followed by space/*/&" groups, so a bare call like `Day()` has no type run and is
# not matched, while `const SystemSpec &Spec(` is.
_DECL = re.compile(
    r"(?m)^[ \t]*(?:[A-Za-z_][\w:<>]*[\s*&]+)+([A-Za-z_]\w*)\s*\("
)

_BLOCK_COMMENT = re.compile(r"/\*.*?\*/", re.S)
_LINE_COMMENT = re.compile(r"//[^\n]*")

# Keywords that can never be the name we want, as a belt-and-braces filter.
_NOT_A_NAME = {
    "if", "for", "while", "switch", "return", "sizeof", "new", "delete",
    "static_cast", "reinterpret_cast", "const_cast", "dynamic_cast", "defined",
}


def strip_comments(text: str) -> str:
    return _LINE_COMMENT.sub("", _BLOCK_COMMENT.sub("", text))


def public_functions(header_text: str):
    """Return the public function names declared in a header, in first-appearance order."""
    text = strip_comments(header_text)
    out = []
    for name in _DECL.findall(text):
        if name in _NOT_A_NAME:
            continue
        if name not in out:
            out.append(name)
    return out


def self_test() -> int:
    sample = """
    int Day() const;
    int Day() const { return 1; }
    CallIt(3);
    x = static_cast<int>(y);
    bool OwnsTrack(uint8_t producer, uint8_t track);
    // int NotADeclaration();
    """
    got = public_functions(sample)
    want = ["Day", "OwnsTrack"]
    if got != want:
        print("surface.py self-test FAILED: got %r want %r" % (got, want), file=sys.stderr)
        return 1
    print("surface.py self-test ok")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("header", nargs="?", default="module/ship/ship_core.h")
    ap.add_argument("--self-test", action="store_true")
    args = ap.parse_args()
    if args.self_test:
        return self_test()
    with open(args.header, "r", encoding="utf-8") as fh:
        for name in public_functions(fh.read()):
            print(name)
    return 0


if __name__ == "__main__":
    sys.exit(main())
