#!/usr/bin/env python3
"""The accessibility measurement for the new screens (docs/evidence/every-screen.md).

Two measurements, both from real data rather than a claim:

  * contrast, WCAG 2.x, on the engine's own palette
    (upstream/efgame/src/game/q_math.cpp), each colour against the surface it
    actually sits on -- the black console background, the selected-row fill
    CT_DKPURPLE2, or the title band a black heading is drawn on;
  * rendered type height, from the screenshots the engine writes at 2x the
    640x480 UI space: the vertical run of ink for a known line of each font,
    measured on the pixels, not read from a constant.

    python3 scripts/screens-a11y.py --palette ../upstream/efgame/src/game/q_math.cpp \
        --shots build/g3-home/baseEF/screenshots
"""

import argparse
import os
import re
import struct
import sys

# The colours the new screens draw with, and the surface each actually sits on.
# (foreground, background, minimum ratio, what it is for)
PAIRS = [
    ("CT_LTGOLD1", "CT_BLACK", 4.5, "values, the chart and queue body"),
    ("CT_LTBLUE2", "CT_BLACK", 4.5, "secondary body, the read colour, the title band fill"),
    ("CT_LTPURPLE1", "CT_BLACK", 4.5, "footer and hint text"),
    ("CT_LTORANGE", "CT_BLACK", 4.5, "column labels"),
    ("CT_WHITE", "CT_BLACK", 4.5, "body and the selected row's text"),
    ("CT_RED", "CT_BLACK", 4.5, "refusals, alarms, the recovery window"),
    ("CT_WHITE", "CT_DKPURPLE2", 3.0, "the selected row's text on the fill"),
    ("CT_LTGOLD1", "CT_DKPURPLE2", 3.0, "a value on the selected fill"),
    ("CT_BLACK", "CT_LTGOLD1", 4.5, "the heading on a gold title band (command screens)"),
    ("CT_BLACK", "CT_LTBLUE2", 4.5, "the heading on a blue title band (the survey)"),
]


def parse_palette(path):
    """Return {name: (r, g, b)} from the engine's colorTable[]."""
    text = open(path, encoding="utf-8", errors="replace").read()
    out = {}
    pattern = re.compile(
        r"\{\s*([0-9]*\.?[0-9]+)f?\s*,\s*([0-9]*\.?[0-9]+)f?\s*,\s*"
        r"([0-9]*\.?[0-9]+)f?\s*,\s*[0-9]*\.?[0-9]+f?\s*\}\s*,?\s*//\s*(CT_\w+)"
    )
    for m in pattern.finditer(text):
        r, g, b, name = m.groups()
        out[name] = (float(r), float(g), float(b))
    return out


def _lin(c):
    return c / 12.92 if c <= 0.03928 else ((c + 0.055) / 1.055) ** 2.4


def luminance(rgb):
    r, g, b = (_lin(c) for c in rgb)
    return 0.2126 * r + 0.7152 * g + 0.0722 * b


def contrast(a, b):
    la, lb = luminance(a), luminance(b)
    hi, lo = max(la, lb), min(la, lb)
    return (hi + 0.05) / (lo + 0.05)


def read_tga(path):
    """Return (width, height, bpp, pixel_at(x,y)->(r,g,b)). Top-left origin."""
    data = open(path, "rb").read()
    (idlen, cmap_type, typ, _cmap_first, _cmap_len, _cmap_bits,
     _x0, _y0, w, h, bpp, desc) = struct.unpack("<BBBHHBHHHHBB", data[:18])
    if typ != 2 or bpp != 24:
        raise ValueError(f"{path}: not an uncompressed 24-bit TGA")
    start = 18 + idlen  # these screenshots carry no colour map
    top_down = bool(desc & 0x20)

    def pixel(x, y):
        row = y if top_down else h - 1 - y
        i = start + (row * w + x) * 3
        return data[i + 2], data[i + 1], data[i]  # BGR

    return w, h, bpp, pixel


def distinct(a, b, tol=40):
    return abs(a[0] - b[0]) + abs(a[1] - b[1]) + abs(a[2] - b[2]) > tol


def modal_colour(pixel, x0, y0, x1, y1):
    """The most common colour in the box: the band or background a line sits on."""
    counts = {}
    for y in range(y0, y1):
        for x in range(x0, x1, 2):
            c = pixel(x, y)
            counts[c] = counts.get(c, 0) + 1
    return max(counts, key=counts.get)


def ink_height(pixel, x0, y0, x1, y1, background):
    """The vertical run of rows in the box that carry ink unlike the background."""
    rows = []
    for y in range(y0, y1):
        for x in range(x0, x1, 2):
            if distinct(pixel(x, y), background):
                rows.append(y)
                break
    return (min(rows), max(rows), max(rows) - min(rows) + 1) if rows else None


def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument("--palette", required=True)
    ap.add_argument("--shots", required=True)
    args = ap.parse_args(argv[1:])

    palette = parse_palette(args.palette)
    print(f"palette:   {args.palette} ({len(palette)} colours parsed)")

    fails = 0
    print("\ncontrast (WCAG 2.x), each colour against the surface it sits on:")
    for fg, bg, bar, why in PAIRS:
        if fg not in palette or bg not in palette:
            print(f"  MISSING {fg} or {bg}")
            fails += 1
            continue
        ratio = contrast(palette[fg], palette[bg])
        verdict = "PASS" if ratio >= bar else "FAIL"
        if verdict == "FAIL":
            fails += 1
        print(f"  {verdict} {fg:13s} on {bg:13s} {ratio:5.2f}:1  (>= {bar}:1)  {why}")

    print("\nrendered type height, measured on the pixels of a screenshot:")
    # The screenshots are 1280x1024 for a 640x480 UI: the horizontal scale is exactly 2, the
    # vertical scale is 1024/480 = 2.133 (the engine stretches to the window's aspect). Boxes below
    # are in rendered pixels at that scale.
    shots = {
        # Screenshot, box (x0,y0,x1,y1) rendered, background (None = the box's modal colour), what
        # it is, the font, and the ink-height band the font is accepted to fall in on the pixels.
        ("lwh_report.tga", (88, 42, 360, 80), None, "the SMALLFONT title on its band", "SMALLFONT", (28, 38)),
        ("lwh_report.tga", (88, 742, 620, 772), (0, 0, 0), "a TINYFONT label", "TINYFONT", (15, 24)),
        ("lwh_jobs.tga", (88, 42, 340, 80), None, "the SMALLFONT title on its band", "SMALLFONT", (28, 38)),
        ("lwh_jobs.tga", (88, 138, 180, 168), (0, 0, 0), "a TINYFONT column label", "TINYFONT", (15, 24)),
        ("lwh_survey.tga", (88, 42, 420, 80), None, "the SMALLFONT title on its band", "SMALLFONT", (28, 38)),
        ("lwh_survey.tga", (88, 206, 400, 236), (0, 0, 0), "a TINYFONT survey row", "TINYFONT", (15, 24)),
    }
    for name, box, bg, what, font, bounds in shots:
        path = os.path.join(args.shots, name)
        if not os.path.isfile(path):
            print(f"  FAIL {name}: no screenshot")
            fails += 1
            continue
        w, h, _bpp, pixel = read_tga(path)
        if (w, h) != (1280, 1024):
            print(f"  FAIL {name}: {w}x{h}, expected 1280x1024 (2x the 640x480 UI space)")
            fails += 1
            continue
        if bg is None:  # the title band: its colour is the box's most common pixel
            bg = modal_colour(pixel, *box)
        ink = ink_height(pixel, *box, bg)
        if not ink:
            print(f"  FAIL {name}: no ink found for {what}")
            fails += 1
            continue
        lo, hi, height = ink
        ok = bounds[0] <= height <= bounds[1]
        if not ok:
            fails += 1
        print(f"  {'PASS' if ok else 'FAIL'} {name}: {what} ink {height} px (rows {lo}-{hi}), "
              f"{font} expected {bounds[0]}-{bounds[1]}")

    print()
    if fails:
        print(f"FAIL  {fails} accessibility check(s)")
        return 1
    print("PASS  the new screens' colours and type heights measure to the palette they sit on")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
