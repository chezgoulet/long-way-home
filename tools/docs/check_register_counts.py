#!/usr/bin/env python3
"""Class 4 of ``scripts/docs-check.sh``: the hook register's stated counts agree with its rows.

The defect this exists to catch is the one ``docs/hook-register.md`` commits repeatedly: the register's
**counts** prose ("the tables above are now 479 hooks", "398") drifts behind the rows the register
actually carries, and a reader sizes work on the header. ``scripts/hooks-check.sh`` already guards the
register's **rows** against the surface of ``module/ship/ship_core.h``; nothing guarded the counts the
register states about itself.

The register carries one machine-readable counts marker, written on its counts section::

    <!-- register-counts play=299 console=158 nothing=34 content=5 total=496 -->

This tool re-derives the counts from the register's own rows and fails when the marker disagrees, and
fails again when the marker's ``total`` disagrees with the surface ``tools/hooks/surface.py`` derives
from the header. It does **not** judge reachability -- that is a person's call, as ``hooks-check.sh``
says -- so it checks only that the register is **internally consistent and the right size**, which is
what a header count is for.

Exit 0 when the marker agrees with the rows and the surface; 1 otherwise.
"""

import argparse
import re
import sys
from collections import Counter
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "hooks"))
from surface import public_functions  # noqa: E402

MARKER = re.compile(r"<!--\s*register-counts\s+(?P<body>[^>]*?)\s*-->")
STATES = ("play", "console", "nothing", "content")
# A register row: the hook in backticks is the first cell. The counts the register states are the
# 4th cell (reach). The first occurrence of a name is the register row; later repeats (the
# unreachable list) are the same hook and are not counted twice.
ROW = re.compile(r"^\|\s*`([^`]+)`\s*\|")


def derive_rows(text):
    seen = {}
    for line in text.splitlines():
        m = ROW.match(line)
        if not m:
            continue
        cells = [c.strip() for c in line.strip().strip("|").split("|")]
        if len(cells) < 4:
            continue
        name = m.group(1)
        if name in seen:
            continue
        seen[name] = cells[3]
    return seen


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--root", default=".")
    ap.add_argument("--register", default="docs/hook-register.md")
    ap.add_argument("--surface", default="module/ship/ship_core.h")
    args = ap.parse_args(argv)
    root = Path(args.root).resolve()

    text = (root / args.register).read_text(encoding="utf-8", errors="replace")
    rows = derive_rows(text)
    counts = Counter(rows.values())

    marker = MARKER.search(text)
    if not marker:
        print(f"FAIL  {args.register}: no register-counts marker; the check would pass vacuously",
              file=sys.stderr)
        return 1

    stated = {}
    for tok in marker.group("body").split():
        k, _, v = tok.partition("=")
        stated[k] = v

    ok = True
    for state in STATES:
        want = counts.get(state, 0)
        got = stated.get(state)
        if got is None:
            print(f"FAIL  {args.register}: marker has no {state}= count", file=sys.stderr)
            ok = False
        elif int(got) != want:
            print(f"FAIL  {args.register}: marker says {state}={got}, the rows count {want}",
                  file=sys.stderr)
            ok = False

    total_rows = sum(counts.values())
    if int(stated.get("total", -1)) != total_rows:
        print(f"FAIL  {args.register}: marker says total={stated.get('total')}, "
              f"the rows number {total_rows}", file=sys.stderr)
        ok = False

    surface = len(public_functions((root / args.surface).read_text(encoding="utf-8", errors="replace")))
    if int(stated.get("total", -1)) != surface:
        print(f"FAIL  {args.register}: marker total={stated.get('total')} but {args.surface} "
              f"declares {surface} public functions", file=sys.stderr)
        ok = False

    if not ok:
        return 1
    print(f"    {args.register}: counts agree with the rows and the surface "
          f"({total_rows} hooks: " + ", ".join(f"{s} {counts.get(s, 0)}" for s in STATES) + ")")
    return 0


if __name__ == "__main__":
    sys.exit(main())
