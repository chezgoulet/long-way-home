#!/usr/bin/env python3
"""Measure a frame, and say whether it carries what a check claims.

A check that ends in ``[ -f "$SHOT" ]`` proves a file was written.  It cannot tell a lit room from
a black one, a room from a corridor, or a screen that opened from the world behind it.  This is the
one instrument for asking a screenshot the questions a content check actually has, so that a check
can fail for the reason its claim cares about.  It is the generalisation of the room-legibility
measure written for `docs/evidence/the-dark-room.md`; `scripts/room-survey-check.sh` and
`scripts/meeting-place-check.sh` use it, and every converted check uses this same file.

Three assertions, and a way to name where each is asked:

  floor  the region is legible: not black, and not uniform.  A lit, structured picture passes; a
         pitch-black frame, a frame shot into the inside of a solid, and a flat single-colour fill
         all fail.  This is what the two original measurers ask of a room.
  diff   the region *changed* between a reference frame and this one, so a claim of the form
         "when X happens, Y appears" can be asserted from the picture: sparks where the system is
         worked, smoke on a burning deck, a screen that is now open, a lamp that is now red.  It
         needs a *before* frame, captured at the same vantage.
  derelict  the region reads as a **derelict interior** -- dark by design, but **structured** (a
         brightness-edge fraction, for the read of wreckage) and **marked** (a lit pixel, for failing
         light rather than absent light), and **not** a lit room (a median ceiling).  For an
         archetype whose whole claim is failing light, where `floor` cannot be used and no per-frame
         dark exemption is allowed (`docs/evidence/derelict-interior.md`).
  region where the claim is about part of the screen (a panel, a footer, a console, a title bar)
         the region is named and measured, not the whole frame.  `--region NAME:x0,y0,x1,y1` or
         `--band TOP,BOTTOM` (fractions of the height, full width).  The whole frame is the default.

  tools/frames/frame.py floor [OPTIONS] FRAME.tga [FRAME.tga ...]
  tools/frames/frame.py diff  [OPTIONS] REFERENCE.tga FRAME.tga [FRAME.tga ...]
  tools/frames/frame.py derelict [OPTIONS] FRAME.tga [FRAME.tga ...]

  python3 tools/frames/frame.py floor --min-p90 150 --min-spread 40 FRAME.tga
  python3 tools/frames/frame.py diff --region band=0,0,1280,120 --min-changed 0.05 BEFORE.tga AFTER.tga

THE TOOL'S OWN LIMITS, which every caller must carry:

  * a **floor** proves a region is *legible* -- not black, not uniform.  It does not prove the
    region is the *right* room, the *right* screen, or a *good* one.  A lit blank wall is legible.
  * a **difference** proves a region *changed*.  It does not prove it changed into the thing the
    claim means: a HUD moving, a particle drifting, or the engine's own console text scrolling all
    change the frame without the feature working.  Choose a region the claim owns and a threshold
    above the camera's own noise, and remember it is still only "changed, not the change you meant".
  * a **region** proves only about the pixels inside it.  Naming the wrong region, or a region too
    small to hold the feature, makes the assertion decorative.

  **The owner judges "right".**  This tool decides legible and changed; it cannot decide good.

A pixel's value is its **brightest channel**, not its luma, so a red console or a blue lamp counts
as light rather than as a dark pixel.  Frames are the engine's own screenshots: uncompressed
true-colour TGA.
"""

import argparse
import struct
import sys


def read_tga(path):
    """An uncompressed true-colour TGA -> (width, height, rows of (r,g,b)), top row first."""
    with open(path, "rb") as f:
        data = f.read()
    if len(data) < 18:
        raise ValueError("%s: too short to be a TGA" % path)
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
    need = start + w * h * step
    if len(data) < need:
        raise ValueError("%s: truncated (need %d bytes, have %d)" % (path, need, len(data)))
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


def band_region(w, h, top, bottom):
    """The full-width band between two fractions of the height, as (x0, y0, x1, y1) pixels."""
    y0 = max(0, int(h * top))
    y1 = max(y0 + 1, int(h * bottom))
    return (0, y0, w, min(h, y1))


def parse_region(spec, w, h):
    """A region spec -> (label, x0, y0, x1, y1). `NAME:x0,y0,x1,y1` or `x0,y0,x1,y1`."""
    label = "region"
    body = spec
    if ":" in spec:
        label, body = spec.split(":", 1)
        label = label or "region"
    nums = body.replace("x", ",").replace(" ", "").split(",")
    if len(nums) != 4:
        raise ValueError("region %r: expected x0,y0,x1,y1 (or NAME:x0,y0,x1,y1)" % spec)
    try:
        x0, y0, x1, y1 = (int(n) for n in nums)
    except ValueError:
        raise ValueError("region %r: the four coordinates must be integers" % spec)
    if x0 < 0 or y0 < 0 or x1 > w or y1 > h or x1 <= x0 or y1 <= y0:
        raise ValueError("region %r is outside a %dx%d frame, or empty" % (spec, w, h))
    return (label, x0, y0, x1, y1)


def region_of(a, w, h):
    """The region this invocation asks about, resolved against a frame of this size."""
    if a.region and a.band:
        raise ValueError("give --region or --band, not both")
    if a.region:
        return parse_region(a.region, w, h)
    if a.band:
        parts = a.band.replace("x", ",").split(",")
        if len(parts) != 2:
            raise ValueError("--band expects TOP,BOTTOM")
        top, bottom = float(parts[0]), float(parts[1])
        return ("band %g-%g" % (top, bottom),) + band_region(w, h, top, bottom)
    return ("whole frame", 0, 0, w, h)


def region_pixels(rows, region):
    """The pixels of a rectangle, as a flat list of (r,g,b)."""
    _, x0, y0, x1, y1 = region
    return [px for row in rows[y0:y1] for px in row[x0:x1]]


def summarize(pixels):
    """mean/median/p*/spread of the brightest-channel value over a set of pixels."""
    vals = sorted(value(px) for px in pixels)
    n = len(vals)
    if n == 0:
        raise ValueError("no pixels in the region")

    def q(p):
        return vals[min(n - 1, int(n * p))]
    return {
        "n": n,
        "mean": sum(vals) / n,
        "median": q(0.50),
        "p05": q(0.05),
        "p90": q(0.90),
        "p95": q(0.95),
        "p99": q(0.99),
        "max": vals[-1],
        "spread": q(0.95) - q(0.05),
    }


def floor_reason(m, pixels, a):
    """Why the region passes or fails the floor.  Not black, not uniform, and something lit."""
    if m["median"] < a.min_median:
        return "median %g < %g (mostly black)" % (m["median"], a.min_median)
    if m["p90"] < a.min_p90:
        return "p90 %g < %g (nothing lit)" % (m["p90"], a.min_p90)
    if m["mean"] < a.min_mean:
        return "mean %g < %g (mostly black)" % (m["mean"], a.min_mean)
    if m["spread"] < a.min_spread:
        return "spread %g < %g (uniform)" % (m["spread"], a.min_spread)
    if a.min_bright_px > 0:
        bc = sum(1 for px in pixels if value(px) >= a.bright)
        if bc < a.min_bright_px:
            return "only %d px >= %d, need %d (nothing lit)" % (bc, a.bright, a.min_bright_px)
    return None


def diff_pixels(ref_pixels, cur_pixels, min_delta):
    """The fraction of pixels that differ by more than min_delta (max channel), and their mean."""
    n = len(ref_pixels)
    changed = 0
    total = 0
    for x, y in zip(ref_pixels, cur_pixels):
        d = max(abs(x[0] - y[0]), abs(x[1] - y[1]), abs(x[2] - y[2]))
        total += d
        if d > min_delta:
            changed += 1
    return changed / n, total / n


def name(path):
    return path.split("/")[-1] or path


def do_floor(a):
    rc = 0
    for path in a.frames:
        try:
            w, h, rows = read_tga(path)
            region = region_of(a, w, h)
        except (ValueError, OSError) as e:
            print("%-46s %s" % (name(path), e))
            rc = 1
            continue
        pixels = region_pixels(rows, region)
        m = summarize(pixels)
        why = floor_reason(m, pixels, a)
        print("%-46s %-15s mean %5.1f median %5.1f p90 %5.1f spread %5.1f  %s"
              % (name(path), region[0], m["mean"], m["median"], m["p90"], m["spread"],
                 "LEGIBLE" if why is None else "DARK -- " + why))
        if why is not None:
            rc = 1
    if rc and a.require:
        print("a frame does not carry the floor's claim")
        return 1
    return 0


def do_diff(a):
    rc = 0
    try:
        rw, rh, rrows = read_tga(a.frames[0])
        region = region_of(a, rw, rh)
    except (ValueError, OSError) as e:
        print("reference: %s" % e)
        return 1
    ref_px = region_pixels(rrows, region)
    for path in a.frames[1:]:
        try:
            w, h, rows = read_tga(path)
            if (w, h) != (rw, rh):
                raise ValueError("%dx%d does not match the reference's %dx%d" % (w, h, rw, rh))
        except (ValueError, OSError) as e:
            print("%-46s %s" % (name(path), e))
            rc = 1
            continue
        cur = region_pixels(rows, region)
        frac, mean = diff_pixels(ref_px, cur, a.min_delta)
        ok = frac >= a.min_changed
        print("%-46s %-15s changed %5.1f%% mean|d| %5.1f of %d px  %s"
              % (name(path), region[0], frac * 100.0, mean, len(cur),
                 "DIFFERS" if ok else "SAME -- the claim is not carried by the picture"))
        if not ok:
            rc = 1
    if rc and a.require:
        print("a frame did not differ from the reference where the claim lives")
        return 1
    return 0


def edge_fraction(rows, region, thresh):
    """The fraction of region pixels that sit on a horizontal brightness edge (> thresh).

    A wrecked corridor is **structure**: bulkheads, broken ribs, a floor to walk on, all picked out
    against the dark. A flat field -- a black void, or a uniform grey fill -- has almost no edges. The
    gradient is the brightest-channel difference between a pixel and the one to its left, so the measure
    reads the *picture's* structure and not a colour.
    """
    _, x0, y0, x1, y1 = region
    edges = 0
    n = 0
    for row in rows[y0:y1]:
        prev = None
        for x in range(x0, x1):
            v = value(row[x])
            if prev is not None:
                n += 1
                if abs(v - prev) > thresh:
                    edges += 1
            prev = v
    return (edges / n) if n else 0.0


def derelict_reason(m, edge_frac, a):
    """Why a region does or does not read as a derelict. The archetype's own stance, written here.

    A derelict interior (`docs/outside-the-ship.md`) is *"wrecked corridors, failing light, no gravity
    in places"*. The room measure (`floor`) cannot carry that claim: a derelict's defining property is
    failing light, so the region is **dark by design** and a luminance floor would fail it while passing
    a lit wall. This measure asserts what the archetype *means* instead:

      * **failing light, not a lit room** -- the typical pixel is dark (median at or below `max_median`).
        A bright room fails: that is the outpost, not the derelict;
      * **failing light, not absent light** -- something is clearly lit (p99 at or above `min_p99`): a
        marker, a failing strip. A black frame fails: the light is gone, not failing;
      * **structure, the read of wreckage** -- enough pixels sit on a brightness edge (`edge_frac`):
        bulkheads, ribs, a floor. A flat field -- black or uniform grey -- fails.

    **No per-frame dark exemption is allowed for this archetype.** A derelict is dark *everywhere*; the
    exemption the outpost needed (one facing onto the black sky) is exactly the failure mode this measure
    exists to avoid, so any frame that does not carry the claim fails, named.
    """
    if m["median"] < a.min_median:
        return "median %g < %g (pitch black: the light is absent, not failing)" % (m["median"], a.min_median)
    if m["median"] > a.max_median:
        return "median %g > %g (too bright: the room is lit, not failing)" % (m["median"], a.max_median)
    if m["p99"] < a.min_p99:
        return "p99 %g < %g (no lit marker: the light is absent, not failing)" % (m["p99"], a.min_p99)
    if edge_frac < a.min_edge:
        return "edge %.3f < %g (no structure: a flat field, not wreckage)" % (edge_frac, a.min_edge)
    return None


def do_derelict(a):
    rc = 0
    for path in a.frames:
        try:
            w, h, rows = read_tga(path)
            region = region_of(a, w, h)
        except (ValueError, OSError) as e:
            print("%-46s %s" % (name(path), e))
            rc = 1
            continue
        pixels = region_pixels(rows, region)
        m = summarize(pixels)
        ef = edge_fraction(rows, region, a.edge)
        why = derelict_reason(m, ef, a)
        print("%-46s %-15s median %5.1f p99 %5.1f edge %5.3f  %s"
              % (name(path), region[0], m["median"], m["p99"], ef,
                 "DERELICT" if why is None else "NOT-WRECKED -- " + why))
        if why is not None:
            rc = 1
    if rc and a.require:
        print("a frame does not carry the derelict's claim")
        return 1
    return 0


def build_parser():
    ap = argparse.ArgumentParser(
        prog="frame.py", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="mode", required=True)

    def common(p):
        p.add_argument("--region", default=None,
                       help="NAME:x0,y0,x1,y1 or x0,y0,x1,y1 (default: the whole frame)")
        p.add_argument("--band", default=None,
                       help="TOP,BOTTOM fractions of the height, full width (e.g. 0.10,0.55)")
        p.add_argument("--require", action="store_true",
                       help="exit 1 if any frame fails the assertion")
        p.add_argument("frames", nargs="+")

    fl = sub.add_parser("floor", help="the region is legible: not black, not uniform")
    common(fl)
    fl.add_argument("--min-median", type=float, default=35.0,
                    help="the typical pixel must be at least this bright (default 35)")
    fl.add_argument("--min-p90", type=float, default=80.0,
                    help="something in the region must be at least this bright (default 80)")
    fl.add_argument("--min-mean", type=float, default=0.0,
                    help="the region's mean must be at least this (default 0, off)")
    fl.add_argument("--min-spread", type=float, default=0.0,
                    help="p95-p05 must be at least this: a uniform fill fails (default 0, off)")
    fl.add_argument("--min-bright-px", type=int, default=0,
                    help="this many pixels must be >= --bright (default 0, off)")
    fl.add_argument("--bright", type=int, default=180,
                    help="the 'clearly lit' level for --min-bright-px (default 180)")

    df = sub.add_parser("diff", help="the region changed from the reference frame")
    common(df)
    df.add_argument("--min-changed", type=float, default=0.02,
                    help="the fraction of region pixels that must change (default 0.02 = 2%%)")
    df.add_argument("--min-delta", type=int, default=16,
                    help="a pixel counts as changed above this max-channel difference (default 16)")

    dr = sub.add_parser("derelict",
                        help="the region reads as a derelict: failing light, structured, not a lit room")
    common(dr)
    dr.add_argument("--min-median", type=float, default=8.0,
                    help="the typical pixel must be at least this: pitch black is the light absent, "
                         "not failing (default 8)")
    dr.add_argument("--max-median", type=float, default=90.0,
                    help="the typical pixel must be at most this (default 90: failing, not a lit room)")
    dr.add_argument("--min-p99", type=float, default=80.0,
                    help="something must be at least this bright -- a lit marker (default 80)")
    dr.add_argument("--min-edge", type=float, default=0.004,
                    help="the fraction of pixels on a brightness edge, for structure (default 0.004)")
    dr.add_argument("--edge", type=int, default=16,
                    help="the gradient above which a pixel counts as an edge (default 16)")

    return ap


def main(argv=None):
    a = build_parser().parse_args(argv)
    return {"floor": do_floor, "diff": do_diff, "derelict": do_derelict}[a.mode](a)


if __name__ == "__main__":
    try:
        sys.exit(main())
    except ValueError as e:
        print("frame.py: %s" % e, file=sys.stderr)
        sys.exit(2)
