#!/usr/bin/env python3
"""Write the placeholder image for a live-panel shader (lwh/panel, lwh/env).

    panelart.py OUT.tga [--width 128] [--height 128]

An uncompressed 32-bit TGA: a dark screen with a lighter frame. The engine's image uploader
overwrites its pixels at runtime with live ship state (gi.UpdatePanelImage); this file only has to
exist, at the image's own size, so the shader is a real loadable shader and the map's surfaces bind
to it. The size matters: the renderer takes the image's own size as authority and resamples the
game's RGBA to it, so a face whose dimensions equal the image's is one whole tile.
"""

import argparse
import struct
import sys

W, H = 128, 128


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("out")
    ap.add_argument("--width", type=int, default=W)
    ap.add_argument("--height", type=int, default=H)
    a = ap.parse_args(argv)
    w, h = a.width, a.height
    px = bytearray()
    for y in range(h):
        for x in range(w):
            edge = x < 2 or y < 2 or x >= w - 2 or y >= h - 2
            r, g, b = (48, 48, 104) if edge else (8, 10, 22)
            px += bytes((b, g, r, 255))  # TGA stores BGR(A)
    # 18-byte TGA header: uncompressed true-colour, 32 bpp, bottom-left origin (descriptor 0x08:
    # eight alpha bits, no top-down flag -- the renderer warns and ignores a top-down declaration,
    # so the placeholder is written the way the renderer reads it).
    header = struct.pack("<BBBHHBHHHHBB", 0, 0, 2, 0, 0, 0, 0, 0, w, h, 32, 0x08)
    with open(a.out, "wb") as f:
        f.write(header + px)
    return 0


if __name__ == "__main__":
    sys.exit(main())
