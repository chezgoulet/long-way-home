#!/usr/bin/env python3
"""Write the placeholder image for the status-panel shader (lwh/panel).

    panelart.py OUT.tga

A 128x64 uncompressed 32-bit TGA: a dark screen with a lighter frame. The engine's image uploader
overwrites its pixels at runtime with live ship state (gi.UpdatePanelImage); this file only has to
exist, at this size, so the shader is a real loadable shader and the map's surfaces bind to it.
"""

import argparse
import struct
import sys

W, H = 128, 128


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("out")
    a = ap.parse_args(argv)
    px = bytearray()
    for y in range(H):
        for x in range(W):
            edge = x < 2 or y < 2 or x >= W - 2 or y >= H - 2
            r, g, b = (48, 48, 104) if edge else (8, 10, 22)
            px += bytes((b, g, r, 255))  # TGA stores BGR(A)
    # 18-byte TGA header: uncompressed true-colour, 32 bpp, top-left origin, 8 alpha bits.
    header = struct.pack("<BBBHHBHHHHBB", 0, 0, 2, 0, 0, 0, 0, 0, W, H, 32, 0x28)
    with open(a.out, "wb") as f:
        f.write(header + px)
    return 0


if __name__ == "__main__":
    sys.exit(main())
