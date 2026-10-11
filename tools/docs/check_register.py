#!/usr/bin/env python3
"""Class 3 of ``scripts/docs-check.sh``: a register row may not stay stale against the tree.

The defect this exists to catch is the one ``docs/omissions.md`` names in its own header: **"A row
that still reads as unbuilt after the work has shipped is the defect this section exists to
prevent."**  Nothing checked a register row against the tree; the row is prose, and prose is a
person's read.  This is the coarsest mechanical thing that works.

A row that claims a capability is unbuilt (or a question unruled) carries a **marker** naming what
would close it -- a file, a symbol, a check, or the recorded ruling.  The marker is written in the
register itself, on the row, so the claim and its marker live in one place::

    | **C1** | ... <!-- register-row id=C1 state=closed marker=check:scripts/power-check.sh --> |

The marker is an HTML comment, so it is invisible to a reader and machine-readable to this check.

What it fails on:

- **``state=open`` and the marker resolves** -- the row still says not built / not modelled / no
  ruling, but what the row is waiting for now exists.  This is the stated defect.
- **``state=closed`` and the marker does not resolve** -- the row has been corrected to built or
  ruled, but the proof it names is gone (renamed, deleted, withdrawn).  Cheap to express, so it is
  checked too: a closed row pointing at nothing is a new kind of stale.
- **a malformed marker** (unknown kind, unreadable file), and **a register with no markers at all**,
  so a broken scan cannot pass vacuously.

**Where the marker approach is coarse, said plainly, because this is the part that would be tempting
to skip:** a marker proves a **symbol exists** -- a file, a name in a file, a line of a ruling.  It
does **not** prove the symbol does what the row describes, that the row's prose is true, or that a
capability is complete.  ``state=`` is written by the person correcting the row, and the check cannot
tell whether that person was right; it can only catch a row whose own machine-readable marker has
become false.  It also reads only rows that carry a marker; an unannotated stale row is invisible
here, which is why the pass records which rows it left unannotated and why.  This is a smoke alarm,
not a verdict.

Exit 0 when every annotated row agrees with the tree; 1 otherwise.
"""

import argparse
import re
import sys
from pathlib import Path

# <!-- register-row id=O2 state=closed marker=check:scripts/air-check.sh -->
MARKER = re.compile(
    r"<!--\s*register-row\s+"
    r"id=(?P<id>\S+)\s+"
    r"state=(?P<state>open|closed)\s+"
    r"marker=(?P<marker>\S+)\s*-->"
)

KINDS = {
    "file": "a file exists",
    "check": "a check script exists",
    "symbol": "a name occurs in a file",
    "text": "a string occurs in a file",
}


def _read(path):
    return path.read_text(encoding="utf-8", errors="replace")


def parse_markers(text):
    """Yield (line_number, id, state, marker_spec) for every marker comment in the text."""
    for m in MARKER.finditer(text):
        yield (text.count("\n", 0, m.start()) + 1, m.group("id"),
               m.group("state"), m.group("marker"))


def resolve(root, spec):
    """Return (exists, human_description, malformed) for a marker spec."""
    kind, _, rest = spec.partition(":")
    if kind in ("file", "check"):
        return (root / rest).is_file(), f"{rest} exists", False
    if kind == "symbol":
        path, _, name = rest.partition(":")
        f = root / path
        return (f.is_file() and name in _read(f),
                f"{name!r} occurs in {path}", False)
    if kind == "text":
        path, _, sub = rest.partition(":")
        f = root / path
        return (f.is_file() and sub in _read(f),
                f"{sub!r} occurs in {path}", False)
    return False, f"unknown marker kind {kind!r}", True


def check_register(root, register, out):
    ok = True
    markers = list(parse_markers(_read(root / register)))
    if not markers:
        out.append(f"FAIL  {register}: no register-row markers; the check would pass vacuously")
        return False

    for ln, rid, state, spec in markers:
        exists, desc, malformed = resolve(root, spec)
        if malformed:
            out.append(f"FAIL  {register}:{ln}: row {rid} has a malformed marker {spec!r}")
            ok = False
            continue
        if state == "open" and exists:
            out.append(f"FAIL  {register}:{ln}: row {rid} still reads as unbuilt/unruled, "
                       f"but its marker resolves ({desc})")
            ok = False
        elif state == "closed" and not exists:
            out.append(f"FAIL  {register}:{ln}: row {rid} is marked closed, "
                       f"but its marker is absent ({spec})")
            ok = False
        elif state == "closed":
            out.append(f"    {register}:{ln}: row {rid} closed; marker resolves ({desc})")
    return ok


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--root", default=".")
    ap.add_argument("--register", action="append", default=None,
                    help="a register file to read (repeatable); defaults to docs/omissions.md")
    args = ap.parse_args(argv)

    root = Path(args.root).resolve()
    registers = args.register or ["docs/omissions.md"]

    out = []
    ok = True
    total = 0
    for reg in registers:
        text = _read(root / reg)
        total += len(list(parse_markers(text)))
        ok = check_register(root, reg, out) and ok

    for line in out:
        print(line, file=(sys.stdout if not line.startswith("FAIL") else sys.stderr))
    if not ok:
        print(f"{total} register row(s) annotated; at least one disagrees with the tree",
              file=sys.stderr)
        return 1
    print(f"    {total} register row(s) annotated, all agree with the tree")
    return 0


if __name__ == "__main__":
    sys.exit(main())
