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
PURPOSE = {6: "Holodeck 2, Armory, Crew Quarters", 7: "Aux Computer Core, Cargo, Labs", 12: "Environmental Control", 13: "Life Support Plant", 14: "Stasis and Holodeck"}
X0, Y0, FLOOR = -4608, -4096, -4000   # inside the footprint the published decks share
W, D, H, T = 1536, 1024, 192, 16
# Each generated deck names its own wall and floor shaders, so a runtime asset swap (S8's Borg
# takeover) can address one deck and not the whole ship. The aliases and the Borg tint are declared
# in tools/shipmap/data/lwh_borg.shader, shipped in the pak beside the map.
WALL, FLOOR_TEX = "hall/hallcomp2", "hall/hallfloor1"

# The system worked on this deck (ship_core's table): a crew member on duty at that system stands at
# the marker named "lwh_station_<system>" (module/crew/g_crew.cpp). Decks with no system here have none.
STATION_SYSTEM = {6: 17, 12: 0}   # holodecks on deck 6, life support on deck 12


def deck_map(n):
    x1, y1, z0, z1 = X0 + W, Y0 + D, FLOOR, FLOOR + H
    # The deck's surfaces are split into sections so the Borg can take a deck *part by part* (S8's
    # "parts of the ship"): west wall, east wall, the ends and ceiling, and the floor. BorgAssets
    # turns them in order as the deck's assimilation rises. Interior walls use the ends' section.
    wall = "lwh/deck%02dwall2" % n
    wall0, wall1 = "lwh/deck%02dwall0" % n, "lwh/deck%02dwall1" % n
    floor_tex = "lwh/deck%02dfloor" % n
    parts = ["{", '"classname" "worldspawn"', '"message" "Deck %d - %s (placeholder)"' % (n, PURPOSE.get(n, "unassigned"))]
    for bounds, tex in [
        (((X0, Y0, z0 - T), (x1, y1, z0)), floor_tex),          # floor
        (((X0, Y0, z1), (x1, y1, z1 + T)), wall),               # ceiling
        (((X0 - T, Y0, z0), (X0, y1, z1)), wall0),              # west wall
        (((x1, Y0, z0), (x1 + T, y1, z1)), wall1),              # east wall
        (((X0 - T, Y0 - T, z0 - T), (x1 + T, Y0, z1 + T)), wall), (((X0 - T, y1, z0 - T), (x1 + T, y1 + T, z1 + T)), wall),
    ]:
        parts.append(brush(bounds, tex))
    # A status screen a few paces in front of the arrival point: a surface the ship's live state is
    # drawn on (Long Way Home). It is ours, and the shader it names is shipped beside the map.
    sx, cy = X0 + 384, Y0 + D // 2
    parts.append(brush(((sx, cy - 64, FLOOR + 40), (sx + 8, cy + 64, FLOOR + 168)), "lwh/panel"))

    # Deck 12's blockout (docs/locations/deck12-environmental-control.brief.md): a working half-deck in
    # two chambers, the plant on a raised deck behind a rail. Coarse massing only -- the detail waits on
    # the owner's approval, per docs/authoring-a-location.md. blockout() marks where waypoints must not go.
    if n == 12:
        mx = X0 + W // 2
        # the dividing wall, with a doorway in the middle
        parts.append(brush(((mx, Y0, z0), (mx + T, cy - 96, z1)), wall))
        parts.append(brush(((mx, cy + 96, z0), (mx + T, y1, z1)), wall))
        # a raised plant deck in the far chamber's back half, a low walkway in front of it
        parts.append(brush(((mx + T, Y0, z0), (x1, Y0 + D // 2, z0 + 48)), floor_tex))
        # Section 42, next door and off-limits (docs/ship-master-map.md). It is closed with solid
        # bulkheads on its open sides -- the deck's own west and north walls close the other two --
        # so there is no way in and it is not a room we spend. The decision is recorded in
        # docs/lore-ledger.md; the waypoints skip its inside below.
        parts.append(brush(((X0, Y0 + 256 - T, z0), (X0 + 256, Y0 + 256, z1)), wall))
        parts.append(brush(((X0 + 256 - T, Y0, z0), (X0 + 256, Y0 + 256, z1)), wall))
    if n == 13:
        # Deck 13's blockout (docs/locations/deck13-life-support.brief.md): a plant hall with a central
        # machinery island and a raised catwalk down one wall. Coarse massing only.
        ix0, ix1 = X0 + W // 3, X0 + 2 * W // 3
        parts.append(brush(((ix0, Y0 + 200, z0), (ix1, y1 - 200, z0 + 96)), wall))
        parts.append(brush(((X0 + T, Y0, z0 + 64), (X0 + T + 96, y1, z0 + 72)), floor_tex))
    if n == 6:
        # Deck 6's blockout (docs/locations/deck06-holodecks.brief.md): a corridor with two holodeck
        # doorframes and an armory locker block. Coarse massing only.
        for dx in (X0 + 384, X0 + 896):
            parts.append(brush(((dx - 128, y1 - 24, z0), (dx - 104, y1 - 8, z0 + 160)), wall))
            parts.append(brush(((dx, y1 - 24, z0), (dx + 24, y1 - 8, z0 + 160)), wall))
            parts.append(brush(((dx - 128, y1 - 24, z0 + 128), (dx + 24, y1 - 8, z0 + 160)), wall))
        parts.append(brush(((X0 + 1152, Y0 + 300, z0), (X0 + 1280, Y0 + 428, z0 + 96)), wall))
    if n == 14:
        # Deck 14's blockout (docs/locations/deck14-stasis.brief.md): stasis pods along a wall and a
        # holodeck doorframe. Coarse massing only.
        for i in range(4):
            px = X0 + 256 + i * 256
            parts.append(brush(((px, y1 - 320, z0), (px + 128, y1 - 200, z0 + 160)), wall))
        dx = X0 + 1152
        parts.append(brush(((dx - 24, Y0, z0), (dx, Y0 + 24, z0 + 192)), wall))
        parts.append(brush(((dx + 128, Y0, z0), (dx + 152, Y0 + 24, z0 + 192)), wall))
        parts.append(brush(((dx - 24, Y0, z0 + 160), (dx + 152, Y0 + 24, z0 + 192)), wall))
    if n == 7:
        # Deck 7's blockout (docs/locations/deck07-auxcore.brief.md): a central auxiliary core column
        # and two cargo islands. Coarse massing only.
        cx = X0 + W // 2
        parts.append(brush(((cx - 96, cy - 96, z0), (cx + 96, cy + 96, z0 + 240)), wall))
        parts.append(brush(((X0 + 256, Y0 + 256, z0), (X0 + 512, Y0 + 512, z0 + 96)), wall))
        parts.append(brush(((x1 - 512, y1 - 512, z0), (x1 - 256, y1 - 256, z0 + 96)), wall))
    parts.append("}")
    out = ["\n".join(parts)]
    out.append(entity([("classname", "info_player_start"), ("origin", "%d %d %d" % (X0 + 96, Y0 + D // 2, z0 + 24)), ("angle", "0")]))
    # A runtime shader remap a designer can place (RPG-X target_shaderremap, adopted by patches/0015):
    # falsename is the shader in game; truename replaces it, and a second use swaps back.
    out.append(entity([("classname", "target_shaderremap"), ("falsename", "textures/lwh/panel"),
                       ("truename", "textures/lwh/borg"), ("origin", "%d %d %d" % (X0 + 192, Y0 + D // 2, z0 + 24))]))
    # The turbolift edge this deck itself declares (the brief: a new deck needs a target_level_change
    # carrying a mapname, or it is unreachable). Placed inside the deck, pointing at the deck 4 hub;
    # the stitcher rewrites it to a teleporter to d04_arrival, and adds the reverse so the new deck is
    # reachable from every deck and every deck from it.
    out.append(entity([("classname", "target_level_change"), ("targetname", "lwh_lift_d%02d" % n),
                       ("mapname", "tour/deck04"), ("origin", "%d %d %d" % (X0 + 160, cy, z0 + 24))]))
    for lx in range(X0 + 256, x1, 512):
        out.append(entity([("classname", "light"), ("light", "350"), ("origin", "%d %d %d" % (lx, Y0 + D // 2, z1 - 32))]))
    if n in STATION_SYSTEM:
        out.append(entity([("classname", "info_notnull"), ("targetname", "lwh_station_%d" % STATION_SYSTEM[n]),
                           ("origin", "%d %d %d" % (X0 + 256, cy - 256, z0 + 24))]))
    for wx in range(X0 + 128, x1, 128):
        for wy in range(Y0 + 128, y1, 128):
            # No waypoint inside the status screen, or the engine refuses the map ("waypoint in solid").
            if abs(wx - sx) < 96 and abs(wy - cy) < 96:
                continue
            if n == 12:
                mx = X0 + W // 2
                # inside a dividing-wall segment (not its doorway: that waypoint links the chambers)
                if abs(wx - mx) <= T + 16 and (wy <= cy - 96 or wy >= cy + 96):
                    continue
                if wx > mx and wy <= Y0 + D // 2 + 16:           # inside the raised plant deck
                    continue
                if wx < X0 + 256 + T and wy < Y0 + 256 + T:      # inside, or on, the sealed section 42
                    continue
            if n == 13:
                ix0, ix1 = X0 + W // 3, X0 + 2 * W // 3
                if ix0 <= wx <= ix1 and Y0 + 200 <= wy <= y1 - 200:   # inside the machinery island
                    continue
            if n == 6 and X0 + 1152 <= wx <= X0 + 1280 and Y0 + 300 <= wy <= Y0 + 428:  # armory locker
                continue
            if n == 14 and y1 - 320 <= wy <= y1 - 200 and any(px <= wx <= px + 128 for px in (X0 + 256 + i * 256 for i in range(4))):
                continue                                                                 # stasis pods
            if n == 7:
                cx = X0 + W // 2
                if abs(wx - cx) <= 96 and abs(wy - cy) <= 96:                            # core column
                    continue
                if X0 + 256 <= wx <= X0 + 512 and Y0 + 256 <= wy <= Y0 + 512:            # cargo island
                    continue
                if x1 - 512 <= wx <= x1 - 256 and y1 - 512 <= wy <= y1 - 256:            # cargo island
                    continue
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
