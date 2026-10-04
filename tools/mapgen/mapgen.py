#!/usr/bin/env python3
"""Emit a Quake III / Elite Force .map file for a simple authored space.

A .map is plain text: brushes are lists of brush planes, entities are key/value blocks. That
makes a space something we can generate and compile rather than only hand-draw -- and it means a
new location can be produced, built and validated without a GUI in the loop.

  mapgen.py --out room.map [--width 512] [--depth 768] [--height 192] [--navgrid 128]
            [--texture textures/foo/bar] [--name testroom]

Emitted: six structural brushes forming a shell, a couple of lights, a player start, and a grid
of `waypoint` entities -- the navigation furniture the engine bakes maps/<map>.nav from.
"""

import argparse
import os


def brush(bounds, texture):
    """A box brush: six planes, written as the three points each plane needs."""
    (x1, y1, z1), (x2, y2, z2) = bounds
    planes = [
        ((x1, y1, z1), (x1, y2, z1), (x2, y2, z1)),   # bottom
        ((x1, y1, z2), (x2, y1, z2), (x2, y2, z2)),   # top
        ((x1, y1, z1), (x2, y1, z1), (x2, y1, z2)),   # south
        ((x1, y2, z1), (x1, y2, z2), (x2, y2, z2)),   # north
        ((x1, y1, z1), (x1, y1, z2), (x1, y2, z2)),   # west
        ((x2, y1, z1), (x2, y2, z1), (x2, y2, z2)),   # east
    ]
    out = ["{"]
    for a, b, c in planes:
        out.append("( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s 0 0 0 1 1"
                   % (a[0], a[1], a[2], b[0], b[1], b[2], c[0], c[1], c[2], texture))
    out.append("}")
    return "\n".join(out)


def entity(pairs):
    out = ["{"]
    for k, v in pairs:
        out.append('"%s" "%s"' % (k, v))
    out.append("}")
    return "\n".join(out)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", required=True)
    ap.add_argument("--name", default="testroom")
    ap.add_argument("--width", type=int, default=512)
    ap.add_argument("--depth", type=int, default=768)
    ap.add_argument("--height", type=int, default=192)
    ap.add_argument("--navgrid", type=int, default=128)
    ap.add_argument("--texture", default="common/caulk")
    a = ap.parse_args()

    W_, D_, H_, T = a.width, a.depth, a.height, 16

    # Structural brushes live INSIDE the worldspawn block -- they are the entity's brushes, not
    # siblings of it. Closing worldspawn and then emitting brushes makes the parser read a stray
    # brace ("Line N is incomplete"), which is the first thing to check when a generated map
    # refuses to compile.
    ws = ["{", '"classname" "worldspawn"', '"message" "%s"' % a.name]
    ws.append(brush(((0, 0, -T), (W_, D_, 0)), a.texture))               # floor
    ws.append(brush(((0, 0, H_), (W_, D_, H_ + T)), a.texture))          # ceiling
    ws.append(brush(((0, 0, 0), (T, D_, H_)), a.texture))                # west
    ws.append(brush(((W_ - T, 0, 0), (W_, D_, H_)), a.texture))          # east
    ws.append(brush(((0, 0, 0), (W_, T, H_)), a.texture))                # south
    ws.append(brush(((0, D_ - T, 0), (W_, D_, H_)), a.texture))          # north
    ws.append("}")
    parts = ["\n".join(ws)]

    # lighting
    parts.append(entity([("classname", "light"), ("light", "300"),
                         ("origin", "%d %d %d" % (W_ // 2, D_ // 2, H_ - 32))]))
    # where the player arrives
    parts.append(entity([("classname", "info_player_start"),
                         ("origin", "%d %d %d" % (W_ // 2, T + 48, 24)),
                         ("angle", "0")]))

    # navigation: a grid of waypoints. The engine bakes maps/<map>.nav from these on first load,
    # so a space with no waypoints is scenery and a space with them can hold crew.
    n = 0
    for x in range(T + a.navgrid, W_ - T, a.navgrid):
        for y in range(T + a.navgrid, D_ - T, a.navgrid):
            parts.append(entity([("classname", "waypoint"),
                                 ("origin", "%d %d %d" % (x, y, 8))]))
            n += 1

    with open(a.out, "w") as fh:
        fh.write("\n".join(parts) + "\n")
    print("%s: %d brushes/entities, %d waypoints, %dx%dx%d"
          % (a.out, len(parts), n, W_, D_, H_))


if __name__ == "__main__":
    main()
