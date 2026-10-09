#!/usr/bin/env python3
"""The ``g_shipTest`` harness's case numbers are one namespace shared by every lane.

Each lane that adds a headless demonstration picks a number and adds a block. Two blocks with the
same number are both valid C++ and nothing complains: the earlier block runs first, and because most
of them end by quitting the run, the later one never executes at all. The feature is present, its
check fails, and nothing points at the cause.

That is not hypothetical. The deck 14 re-dress claimed ``60`` on 2026-10-06 and rising claimed ``60``
again on 2026-10-08; the rising block ran and quit at 5.2 s, so deck 14's own check failed silently
until a sweep ran it on a host with game data.

This fails on a duplicate, naming the number and how many blocks claim it.

    python3 tools/harness/check_cases.py [--src module/ship/g_ship.cpp]
"""

import argparse
import collections
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_SRC = ROOT / "module" / "ship" / "g_ship.cpp"
CASE = re.compile(r"g_shipTest->integer\s*==\s*(\d+)")


def cases(text):
    """Every case number the harness claims, with its number of claims."""
    return collections.Counter(int(m) for m in CASE.findall(text))


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--src", default=str(DEFAULT_SRC))
    args = ap.parse_args(argv)

    path = Path(args.src)
    if not path.exists():
        print(f"FAIL  no harness source at {path}", file=sys.stderr)
        return 1
    counts = cases(path.read_text(encoding="utf-8", errors="replace"))
    if not counts:
        print(f"FAIL  no g_shipTest cases found in {path}; the scan is probably broken", file=sys.stderr)
        return 1

    dupes = {n: c for n, c in counts.items() if c > 1}
    if dupes:
        for n, c in sorted(dupes.items()):
            print(f"FAIL  g_shipTest case {n} is claimed {c} times in {path.name}: the first block runs "
                  f"and the later one never does", file=sys.stderr)
        print(f"{len(dupes)} duplicate case number(s) among {len(counts)}", file=sys.stderr)
        return 1

    print(f"    {len(counts)} harness case numbers, all distinct")
    return 0


if __name__ == "__main__":
    sys.exit(main())
