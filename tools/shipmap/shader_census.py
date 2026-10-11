#!/usr/bin/env python3
"""Count the shader names each deck's map source uses, and how many decks share each one.

    shader_census.py --decks DIR [--decks DIR ...] [--min-shared N] [--min-all N] [--json OUT]

DIR holds deckNN.map sources. A shader that more than one deck uses cannot be addressed on one deck
alone by `gi.RemapShader`, which remaps by name (docs/evidence/the-bridge-traps.md). This is the
measurement behind that boundary claim: the published Virtual Voyager decks share visible shaders
(hall/hallfloor3 and voyager/wall2 are on all ten), while the generated decks LWH authored name
their sections per deck (lwh/deck06wall0 ...) and do not.

Only the map text is read -- brush faces (the token after the three plane points) and a patch's own
`texture` line. Nothing here reads game data or a compiled BSP.
"""

import argparse
import collections
import glob
import json
import os
import re
import sys

# The same face regex the stitcher and the part census use (tools/shipmap/stitch.py).
FACE_TEXTURE = re.compile(r"^(\((?:[^()]*\)\s*\(){2}[^()]*\)\s+)(\S+)")


def deck_shaders(path):
    """Every shader name a deckNN.map names, lower-cased."""
    names = set()
    with open(path, encoding="utf-8", errors="replace") as fh:
        for line in fh:
            m = FACE_TEXTURE.match(line)
            if m:
                names.add(m.group(2).lower())
                continue
            # A patch's texture is a key-value line of its own inside patchDef2/patchDef3.
            stripped = line.strip()
            if stripped.startswith("texture"):
                parts = stripped.split()
                if len(parts) >= 2:
                    names.add(parts[1].lower())
    return names


def census(directory):
    """{deck name: {shader, ...}} for every deckNN.map under directory."""
    out = collections.OrderedDict()
    for path in sorted(glob.glob(os.path.join(directory, "deck*.map"))):
        name = os.path.splitext(os.path.basename(path))[0]
        out[name] = deck_shaders(path)
    return out


def owners(per_deck):
    """{shader: {deck, ...}}."""
    own = collections.defaultdict(set)
    for deck, names in per_deck.items():
        for n in names:
            own[n].add(deck)
    return own


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--decks", action="append", required=True,
                    help="a directory of deckNN.map sources (repeatable)")
    ap.add_argument("--min-shared", type=int, default=0,
                    help="fail unless at least this many shaders are named by more than one deck")
    ap.add_argument("--min-all", type=int, default=0,
                    help="fail unless at least this many shaders are named by every deck")
    ap.add_argument("--unique-pattern", default=None,
                    help="fail unless every shader matching this regex is named by exactly one deck "
                         "(the deck-unique section names a per-deck remap needs)")
    ap.add_argument("--json", help="write the full owner table here")
    args = ap.parse_args(argv)

    status = 0
    report = {}
    for directory in args.decks:
        per_deck = census(directory)
        if not per_deck:
            print(f"FAIL  no deckNN.map under {directory}", file=sys.stderr)
            return 1
        own = owners(per_deck)
        shared = {s: d for s, d in own.items() if len(d) > 1}
        on_all = sorted(s for s, d in own.items() if len(d) == len(per_deck))
        print(f"  {directory}")
        print(f"    {len(per_deck)} deck(s), {len(own)} distinct shader(s), "
              f"{len(shared)} named by more than one deck, {len(on_all)} by all {len(per_deck)}")
        for s in on_all:
            print(f"      all {len(per_deck)}: {s}")
        report[directory] = {"decks": len(per_deck), "distinct": len(own),
                             "shared": sorted(shared), "on_all": on_all}
        if len(shared) < args.min_shared:
            print(f"FAIL  {len(shared)} shared shader(s) is below --min-shared {args.min_shared}",
                  file=sys.stderr)
            status = 1
        if len(on_all) < args.min_all:
            print(f"FAIL  {len(on_all)} shader(s) on every deck is below --min-all {args.min_all}",
                  file=sys.stderr)
            status = 1
        if args.unique_pattern:
            pat = re.compile(args.unique_pattern)
            matched = {s: d for s, d in own.items() if pat.search(s)}
            if not matched:
                print(f"FAIL  no shader matches --unique-pattern {args.unique_pattern!r}", file=sys.stderr)
                status = 1
            else:
                multi = {s: d for s, d in matched.items() if len(d) != 1}
                for s in sorted(multi):
                    print(f"FAIL  {s} is named by {len(multi[s])} decks, not one", file=sys.stderr)
                if multi:
                    status = 1
                else:
                    print(f"    {len(matched)} shader(s) match {args.unique_pattern!r}, "
                          f"each named by exactly one deck")

    if args.json:
        with open(args.json, "w", encoding="utf-8") as fh:
            json.dump(report, fh, indent=2, sort_keys=True)
    return status


if __name__ == "__main__":
    sys.exit(main())
