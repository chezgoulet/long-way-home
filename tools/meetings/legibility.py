#!/usr/bin/env python3
"""Measure a room's frame, and say whether it is legible or dark.

The owner's ruling (docs/evidence/the-meeting-place.md, 2026-10-10): *"if a room is pitch black it
needs its own light."*  So whether a room reads is a property of the **rendered frame**, not of the
`.map`: a map full of light entities can still render dark.  This tool reads the engine's own
screenshot (an uncompressed TGA) and measures it, so a check built on it can fail for the reason
the ruling cares about -- and cannot be satisfied by counting lights.

  tools/meetings/legibility.py FRAME.tga [FRAME.tga ...]     # a line per frame, with a verdict
  tools/meetings/legibility.py --require-legible FRAME ...   # exit 1 if any frame is dark

A frame is **legible** when it shows both a visible room and a light: its typical pixel is not near
black (`--min-median`) *and* it carries something clearly lit (`--min-p90`).  A frame that is all
shadow has a low median; a frame shot into a solid wall has neither a lit feature nor structure.  The
measure is a pixel's **brightest channel**, not its luma, so a red console (astrometrics) or a blue
lamp counts as light rather than as a dark pixel.  The area measured is the middle band of the frame
by default (`--top`..`--bottom`, 10%..55%): above it is the engine's console text, below it the weapon
and the meeting band -- neither is the room.

The thresholds are ours and overrulable.  They were set against the frames the dark-room pass
produced (docs/evidence/the-dark-room.md): a room read at median 40-120 with a lit feature at p90
120-255, while a camera inside the composition's own geometry read at median 55 with nothing above
p90 61, and a pitch-black frame at median 14 p90 15.
"""

import argparse
import struct
import sys


def read_tga(path):
    """An uncompressed true-colour TGA -> (width, height, rows of (r,g,b)), top row first."""
    with open(path, "rb") as f:
        data = f.read()
    id_len, cmap, img_type = data[0], data[1], data[2]
    if cmap:
        raise ValueError("%s: colour-mapped TGA is not supported" % path)
    if img_type not in (2, 3):
        raise ValueError("%s: not an uncompressed true-colour TGA (type %d)" % (path, img_type))
    w, h, bpp, desc = struct.unpack_from("<HHBB", data, 12)
    if bpp not in (24, 32):
        raise ValueError("%s: %d bpp is not supported" % (path, bpp))
    step = bpp // 8
    start = 18 + id_len
    top_down = bool(desc & 0x20)
    rows = []
    for y in range(h):
        row = data[start + y * w * step: start + (y + 1) * w * step]
        px = [(row[x * step + 2], row[x * step + 1], row[x * step]) for x in range(w)]  # BGR -> RGB
        rows.append(px)
    if not top_down:
        rows.reverse()
    return w, h, rows


def value(rgb):
    """The pixel's brightest channel: a red console or a blue lamp is light, not a low luma."""
    return max(rgb[0], rgb[1], rgb[2])


def measure(path, top=0.10, bottom=0.55):
    w, h, rows = read_tga(path)
    lo = max(0, int(h * top))
    hi = max(lo + 1, int(h * bottom))
    vals = [value(px) for row in rows[lo:hi] for px in row]
    n = len(vals)
    vals.sort()

    def q(p):
        return vals[min(n - 1, int(n * p))]
    return {"w": w, "h": h, "mean": sum(vals) / n, "median": q(0.5), "p90": q(0.90)}


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("frames", nargs="+")
    ap.add_argument("--min-median", type=float, default=35.0,
                    help="the typical pixel must be at least this bright (default 35)")
    ap.add_argument("--min-p90", type=float, default=80.0,
                    help="something in the frame must be at least this bright (default 80)")
    ap.add_argument("--top", type=float, default=0.10, help="the top of the measured band (default 0.10)")
    ap.add_argument("--bottom", type=float, default=0.55, help="the bottom of the measured band (default 0.55)")
    ap.add_argument("--require-legible", action="store_true", help="exit 1 if any frame is dark")
    a = ap.parse_args(argv)

    dark = []
    for path in a.frames:
        m = measure(path, a.top, a.bottom)
        ok = m["median"] >= a.min_median and m["p90"] >= a.min_p90
        print("%-46s mean %5.1f median %5.1f p90 %5.1f  %s"
              % (path.split("/")[-1], m["mean"], m["median"], m["p90"],
                 "LEGIBLE" if ok else "DARK"))
        if not ok:
            dark.append(path)
    if dark:
        print("%d frame(s) dark: %s" % (len(dark), ", ".join(p.split("/")[-1] for p in dark)))
        if a.require_legible:
            return 1
    else:
        print("all %d frame(s) legible" % len(a.frames))
    return 0


if __name__ == "__main__":
    sys.exit(main())
