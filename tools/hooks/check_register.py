#!/usr/bin/env python3
"""The hook register's freshness check (docs/hook-register.md, Task D).

Re-derives the public surface of module/ship/ship_core.h and compares it to the hook names in the
register's tables.  It catches the one thing a hand-written catalogue always misses: a public
function that exists in the code and not in the register.

It does not judge reachability: that is a person's call, and a script cannot see whether a screen
sends a command or a console draws a readout.  It checks *presence*, and only presence.

Exit 0 when the two sets are identical; 1 when the code has a function the register has not (the
hard failure the brief asks for), or when the register names a function the code no longer has (a
stale row, also drift).
"""

import argparse
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from surface import public_functions  # noqa: E402

# A register hook row: `| `Name` | ...`
_ROW = re.compile(r"^\|\s*`([A-Za-z_]\w*)`\s*\|", re.M)


def registered_hooks(path: Path):
    rows = _ROW.findall(path.read_text(encoding="utf-8"))
    seen, dupes = [], []
    for r in rows:
        if r in seen:
            dupes.append(r)
        else:
            seen.append(r)
    return seen, dupes


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--header", default="module/ship/ship_core.h")
    ap.add_argument("--register", default="docs/hook-register.md")
    args = ap.parse_args()

    surface = public_functions(Path(args.header).read_text(encoding="utf-8"))
    registered, dupes = registered_hooks(Path(args.register))

    missing = [n for n in surface if n not in registered]
    extra = [n for n in registered if n not in surface]

    print("hooks-check: %s declares %d public functions; %s registers %d hooks"
          % (args.header, len(surface), args.register, len(registered)))
    if dupes:
        print("  duplicate row(s) in the register: %s" % ", ".join(sorted(set(dupes))))

    failed = False
    if missing:
        failed = True
        print("FAIL  in the code and not in the register (%d): %s" % (len(missing), ", ".join(missing)))
    if extra:
        failed = True
        print("FAIL  in the register and not in the code (%d): %s" % (len(extra), ", ".join(extra)))
    if failed:
        return 1
    print("PASS  every public function is registered, and every registered hook exists")
    return 0


if __name__ == "__main__":
    sys.exit(main())
