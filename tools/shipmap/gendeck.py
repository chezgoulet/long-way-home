#!/usr/bin/env python3
"""Generate a placeholder deck for the decks that have no published source.

    gendeck.py --deck 6 --out DIR

Writes DIR/deckNN.map in the same convention as the published deck sources (geometry near
z = -4000, one info_player_start where the turbolift sets you down), so the stitcher treats it like
any other deck.

What it makes is a sealed, lit, navigable hall with a waypoint grid: a deck that exists, can be
reached and walked, and can hold crew. It is NOT a reconstruction: no interior plan of these decks
exists to reconstruct. Everything about it -- size, shape, purpose -- is invented and is listed as
such in docs/lore-ledger.md. It is the floor the real deck will be built on.
"""

import argparse
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "mapgen"))
from mapgen import brush, entity  # noqa: E402

# What each missing deck is taken to hold. Invented placements unless the ledger says otherwise.
PURPOSE = {6: "Holodecks", 7: "Computer Core", 12: "Environmental Control", 13: "Cargo and Stores", 14: "Engineering Support"}
X0, Y0, FLOOR = -4608, -4096, -4000   # inside the footprint the published decks share
W, D, H, T = 1536, 1024, 192, 16
WALL, FLOOR_TEX = "hall/hallcomp2", "hall/hallfloor1"


def deck_map(n):
    x1, y1, z0, z1 = X0 + W, Y0 + D, FLOOR, FLOOR + H
    parts = ["{", '"classname" "worldspawn"', '"message" "Deck %d - %s (placeholder)"' % (n, PURPOSE.get(n, "unassigned"))]
    for bounds, tex in [
        (((X0, Y0, z0 - T), (x1, y1, z0)), FLOOR_TEX),          # floor
        (((X0, Y0, z1), (x1, y1, z1 + T)), WALL),               # ceiling
        (((X0 - T, Y0, z0), (X0, y1, z1)), WALL), (((x1, Y0, z0), (x1 + T, y1, z1)), WALL),
        (((X0 - T, Y0 - T, z0 - T), (x1 + T, Y0, z1 + T)), WALL), (((X0 - T, y1, z0 - T), (x1 + T, y1 + T, z1 + T)), WALL),
    ]:
        parts.append(brush(bounds, tex))
    parts.append("}")
    out = ["\n".join(parts)]
    out.append(entity([("classname", "info_player_start"), ("origin", "%d %d %d" % (X0 + 96, Y0 + D // 2, z0 + 24)), ("angle", "0")]))
    for lx in range(X0 + 256, x1, 512):
        out.append(entity([("classname", "light"), ("light", "350"), ("origin", "%d %d %d" % (lx, Y0 + D // 2, z1 - 32))]))
    for wx in range(X0 + 128, x1, 128):
        for wy in range(Y0 + 128, y1, 128):
            out.append(entity([("classname", "waypoint"), ("origin", "%d %d %d" % (wx, wy, z0 + 24))]))
    return "\n".join(out) + "\n"


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--deck", type=int, action="append", required=True)
    ap.add_argument("--out", required=True)
    a = ap.parse_args(argv)
    os.makedirs(a.out, exist_ok=True)
    for n in a.deck:
        if not 1 <= n <= 15:
            print(f"deck {n}: Voyager has decks 1 to 15", file=sys.stderr)
            return 2
        path = os.path.join(a.out, "deck%02d.map" % n)
        with open(path, "w") as f:
            f.write(deck_map(n))
        print(f"deck {n:2} ({PURPOSE.get(n, 'unassigned')}) -> {path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
