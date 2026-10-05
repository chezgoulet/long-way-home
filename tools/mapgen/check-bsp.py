#!/usr/bin/env python3
"""Structural check on a compiled BSP.

A compile that exits 0 while dropping all the geometry is a real, observed failure mode: bad
brush winding, or an unresolvable texture/base path, produce an empty map with no error. The BSP
is the only place that shows it, so check the BSP rather than the compiler's exit code.

Usage: check-bsp.py <file.bsp> [...]   -- exits non-zero if a map carries no geometry.
"""
import struct
import sys

NAMES = ["entities", "shaders", "planes", "nodes", "leafs", "leafsurfs", "leafbrushes",
         "models", "brushes", "brushsides", "drawverts", "drawindexes", "fogs", "surfaces",
         "lightmaps", "lightgrid", "visibility"]

# A playable space needs shaders to draw with, brushes to be solid, and surfaces to render.
REQUIRED = ("shaders", "brushes", "surfaces")


def lumps(path):
    data = open(path, "rb").read()
    if data[:4] != b"IBSP":
        raise SystemExit("%s: not a BSP (magic %r)" % (path, data[:4]))
    out = {}
    for i, name in enumerate(NAMES):
        off, length = struct.unpack("<ii", data[8 + i * 8:16 + i * 8])
        out[name] = (off, length)
    return out


def main(argv):
    bad = 0
    for path in argv[1:]:
        found = lumps(path)
        missing = [n for n in REQUIRED if found[n][1] == 0]
        size = sum(length for _, length in found.values())
        empty = sum(1 for _, length in found.values() if length == 0)
        verdict = "OK" if not missing else "EMPTY: " + ", ".join(missing)
        print("%-44s %-34s (%d/17 lumps empty)" % (path, verdict, empty))
        if missing:
            bad = 1
    return bad


if __name__ == "__main__":
    if len(sys.argv) < 2:
        raise SystemExit(__doc__)
    sys.exit(main(sys.argv))
