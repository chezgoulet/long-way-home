#!/usr/bin/env python3
"""Light deck 8's astrometrics room -- a lighting fix, not a post fix.

    lightdeck08.py --deck build/gdk/maps/eliteforce_virtualvoyager_maps/deck08.map \
                   --out build/ship/generated [--report FILE]

The room-post sweep (docs/evidence/the-room-posts.md) stood the meeting's camera at every open
vantage of every room a meeting can happen in and measured the frame. Astrometrics was the one room
where **no post saved it**: from five points and all six vantages of each, the frame's middle band
read `median 18..55, p90 49..72` -- a visible room with nothing lit enough to pass (`p90 >= 80`).
That is the other case the brief names, a **lighting** defect, and a lighting fix belongs here in
`tools/shipmap/` so it survives `scripts/build-ship.sh`, not in a hand edit to the generated map.

The room already carries lights -- its own fixtures read `light` 30 (and a few 100) in the published
source -- but they leave the console ring and the viewer bank dim in the bake. This adds a small set
of working lights over the room's own surfaces, in the published deck's own coordinate frame (the
stitcher moves the deck to its height: `world_z = local_z - (deck-1)*3072`, x and y unchanged). It
copies the source and writes deck08.map to --out, where the stitcher's later `--decks` takes it in
place of the published one.

No geometry, no texture, no new asset: light entities only.
"""

import argparse
import json
import os
import sys

# The astrometrics room in deck08's own frame: floor top z -3768, the room x -1876..-1476,
# y -3140..-2652, and the room's own point (-1672, -3040). The soffit over the point sits at
# z -3672..-3628, so the lights are hung at z -3696, the height the room's own fixtures already use,
# where they are below the soffit and in the open. Positions and intensity are the ones the frames
# chose: the added light is what lifts the measured band over `p90 >= 80`.
LIGHTS = [
    # x, y, z, intensity, colour, why
    (-1672, -3040, -3696, 200, "0.95 0.97 1.0", "over the room's own point"),
    (-1560, -3040, -3696, 175, "0.95 0.97 1.0", "the console ring's east bank"),
    (-1790, -3040, -3696, 175, "0.95 0.97 1.0", "the console ring's west bank"),
    (-1672, -2860, -3696, 175, "0.90 0.94 1.0", "the viewer end"),
    (-1672, -3120, -3696, 150, "0.90 0.94 1.0", "the door end"),
]

ROOM_POINT = (-1672, -3040)   # the deck's own `stop_andlook` nav goal: this is astrometrics


def lit(text):
    """The source with the working lights appended, and the count added."""
    entities = []
    for x, y, z, intensity, colour, why in LIGHTS:
        entities.append(
            '{\n"classname" "light"\n"origin" "%g %g %g"\n"light" "%d"\n"_color" "%s"\n'
            "// %s\n}\n" % (x, y, z, intensity, colour, why)
        )
    return text.rstrip("\n") + "\n" + "".join(entities), len(entities)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--deck", default="build/gdk/maps/eliteforce_virtualvoyager_maps/deck08.map",
                    help="the published deck-8 source")
    ap.add_argument("--out", required=True, help="where to write deck08.map (the stitcher's later --decks)")
    ap.add_argument("--report")
    a = ap.parse_args(argv)

    if not os.path.isfile(a.deck):
        print("lightdeck08.py: no deck source at %s -- run scripts/fetch-gdk.sh" % a.deck, file=sys.stderr)
        return 1
    with open(a.deck, encoding="latin-1") as f:
        source = f.read()
    # A guard, so a wrong source fails loudly instead of lighting the wrong room.
    if '"stop_andlook"' not in source:
        print("lightdeck08.py: %s is not the deck-8 source (no stop_andlook nav goal)" % a.deck, file=sys.stderr)
        return 1

    out_text, added = lit(source)
    os.makedirs(a.out, exist_ok=True)
    path = os.path.join(a.out, "deck08.map")
    with open(path, "w", encoding="latin-1") as f:
        f.write(out_text)

    report = {
        "room": "astrometrics",
        "point": list(ROOM_POINT),
        "lights_added": added,
        "reason": "no post saved the room: every vantage measured dark (docs/evidence/the-room-posts.md)",
    }
    if a.report:
        with open(a.report, "w") as f:
            json.dump(report, f, indent=1)
    print("deck08 <- %s (astrometrics lighting fix)" % a.deck)
    print("  astrometrics room, point %s: %d working light(s) added" % (ROOM_POINT, added))
    print("  wrote %s" % path)
    return 0


if __name__ == "__main__":
    sys.exit(main())
