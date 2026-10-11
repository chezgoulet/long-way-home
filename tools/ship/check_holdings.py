#!/usr/bin/env python3
"""The guard for the C2 ruling: no stored ``holdings`` field may reappear.

``docs/morale.md`` (the owner's ruling, "Derived") makes ``holdings`` a fold over the marks -- a
read, ``Holdings()`` -- not a float the simulation nudges.  The C++ compiler already refuses a
``c.holdings`` it cannot find, but that is a build error at the point of use, not a statement that
the storage is gone.  This scan is that statement, and ``scripts/holdings-check.sh`` exercises it in
both directions: it passes on the folded tree and fails on a planted stored field.

What it looks for, in code only (comments and string literals are removed first, so prose that names
the old field, like the save changelog, is not a hit):

- a field declaration, ``float holdings``;
- a member read or write, ``.holdings`` or ``->holdings``;
- an assignment, ``holdings = ...``.

The folded read and its constants are ``Holdings`` and ``HOLDINGS_*`` (capital), so they are not
matched.  Exit 0 when nothing is found; 1 with one line per hit otherwise.

    python3 tools/ship/check_holdings.py [dir ...]        # default: module/ship
"""

import re
import sys
from pathlib import Path

# The stored field, in its three shapes. Capitalised `Holdings`/`HOLDINGS_*` are the fold and are
# deliberately not matched.
FIELD = re.compile(r"(?:\.\s*holdings\b|->\s*holdings\b|\bfloat\s+holdings\b|\bholdings\s*=)")

BLOCK_COMMENT = re.compile(r"/\*.*?\*/", re.DOTALL)
LINE_COMMENT = re.compile(r"//[^\n]*")
# A "..." or '...' literal, so a string that happens to name the field is not read as code.
STRING_LITERAL = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')

SOURCES = (".cpp", ".h", ".hpp", ".cc", ".inl")


def strip_noncode(text):
    """Remove comments and string literals, keeping the newline count so line numbers survive."""
    def keep_lines(m):
        return "\n" * m.group(0).count("\n")

    text = BLOCK_COMMENT.sub(keep_lines, text)
    text = LINE_COMMENT.sub("", text)
    text = STRING_LITERAL.sub("", text)
    return text


def scan(root):
    hits = []
    for path in sorted(Path(root).rglob("*")):
        if path.suffix not in SOURCES:
            continue
        code = strip_noncode(path.read_text(encoding="utf-8", errors="replace"))
        for n, line in enumerate(code.splitlines(), 1):
            if FIELD.search(line):
                hits.append((str(path), n, line.strip()))
    return hits


def main(argv):
    roots = argv[1:] or ["module/ship"]
    hits = []
    for root in roots:
        hits += scan(root)
    if not hits:
        print(f"    no stored holdings field in {', '.join(roots)}")
        return 0
    for path, n, line in hits:
        print(f"FAIL  {path}:{n}: {line}", file=sys.stderr)
    print(f"{len(hits)} stored-field reference(s) found; holdings must be a fold", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
