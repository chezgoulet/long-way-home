#!/usr/bin/env python3
"""The meeting place check (docs/staff-meetings.md, the owner's ruling 2026-10-09).

A meeting happens *somewhere*: the subject and the people choose the room, and the room is a real
place the master map names (``docs/ship-master-map.md``).  The mapping lives in one place a program
can read -- ``module/ship/meeting_rooms.def``, included (X-macro) by ``module/ship/ship_core.cpp``
and keyed by the meeting's **kind** and its **subject** (the department whose business it is).

This tool checks the table against the kind enumeration in ``module/ship/ship_core.h`` and fails when
a kind of meeting has no room at all -- a meeting that cannot say where it happens is the bug this
phase exists to fix.  The pattern is ``tools/console/verbs.py``: the enumeration is derived, the table
is curated, and a check fails when they drift.

  tools/meetings/rooms.py check        # what scripts/meeting-place-check.sh runs
  tools/meetings/rooms.py list         # kind<TAB>subject<TAB>room<TAB>deck, one row per line
  tools/meetings/rooms.py check --header X --table Y   # check modified copies without touching the tree

Demonstrated failing when a kind's row is removed, and passing on the tree, in
``docs/evidence/the-meeting-place.md``.
"""

import argparse
import re
import sys
from pathlib import Path

_ENUM_KIND = "enum MeetingKind"
_ENUM_DEPT = "enum Department"

_ROW = re.compile(
    r'^\s*MEETING_ROOM\(\s*(\w+)\s*,\s*(\w+)\s*,\s*"([^"]*)"\s*,\s*(\d+)\s*\)\s*$', re.M
)

_BLOCK_COMMENT = re.compile(r"/\*.*?\*/", re.S)
_LINE_COMMENT = re.compile(r"//[^\n]*")
_IDENT = re.compile(r"[A-Za-z_]\w*")


def _strip(text):
    return _LINE_COMMENT.sub("", _BLOCK_COMMENT.sub("", text))


def enum_names(header_text, marker):
    """The enumerator names of ``enum ... { ... }`` introduced by ``marker``, minus the ``_COUNT``."""
    text = _strip(header_text)
    start = text.index(marker)
    brace = text.index("{", start)
    end = text.index("}", brace)
    body = text[brace + 1 : end]
    names = [t for t in _IDENT.findall(body) if not t.endswith("_COUNT")]
    return names


def read_rows(table_text):
    """The table's rows as (kind, dept, room, deck); raises on a line that does not parse."""
    text = _strip(table_text)
    rows = _ROW.findall(text)
    declared = len(re.findall(r"^\s*MEETING_ROOM\(", table_text, re.M))
    if declared != len(rows):
        raise ValueError(
            "%d MEETING_ROOM line(s) but only %d parsed" % (declared, len(rows))
        )
    return [(k, d, r, int(deck)) for (k, d, r, deck) in rows]


def check(header, table, out):
    kinds = enum_names(Path(header).read_text(encoding="utf-8"), _ENUM_KIND)
    depts = enum_names(Path(header).read_text(encoding="utf-8"), _ENUM_DEPT)
    rows = read_rows(Path(table).read_text(encoding="utf-8"))

    out("rooms.py: %s names %d kind(s); %s maps %d room row(s)" % (header, len(kinds), table, len(rows)))

    failed = False
    known_kinds = set(kinds)
    known_depts = set(depts)
    seen = set()
    for kind, dept, room, deck in rows:
        if kind not in known_kinds:
            out("FAIL  a row names an unknown meeting kind %r" % kind)
            failed = True
        if dept not in known_depts:
            out("FAIL  a row names an unknown subject department %r" % dept)
            failed = True
        if not room:
            out("FAIL  a row names no room (%s/%s)" % (kind, dept))
            failed = True
        if not (1 <= deck <= 15):
            out("FAIL  a row names deck %d for %s/%s; there are fifteen decks" % (deck, kind, dept))
            failed = True
        key = (kind, dept)
        if key in seen:
            out("FAIL  two rows for the same kind and subject: %s/%s" % (kind, dept))
            failed = True
        seen.add(key)

    # The bug this phase closes: a kind of meeting with no room to happen in.
    homeless = [k for k in kinds if not any(r[0] == k for r in rows)]
    if homeless:
        for k in homeless:
            out("FAIL  the meeting kind %s has no room" % k)
        failed = True

    if failed:
        return 1
    out("PASS  every meeting kind has a room, and every room row names a known kind, subject and deck")
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("mode", choices=["check", "list"])
    ap.add_argument("--header", default="module/ship/ship_core.h")
    ap.add_argument("--table", default="module/ship/meeting_rooms.def")
    args = ap.parse_args()

    if args.mode == "list":
        for kind, dept, room, deck in read_rows(Path(args.table).read_text(encoding="utf-8")):
            print("%s\t%s\t%s\t%d" % (kind, dept, room, deck))
        return 0
    try:
        return check(args.header, args.table, lambda s: print(s))
    except ValueError as e:
        print("FAIL  %s" % e)
        return 1


if __name__ == "__main__":
    sys.exit(main())
