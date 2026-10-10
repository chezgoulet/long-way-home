"""Tests for the room-legibility measure (tools/meetings/legibility.py).

The tool exists so a check can fail for the owner's reason -- a room that renders dark -- without
counting the lights.  So the tests that carry the weight are the ones where it *fails*: a pitch-black
frame is DARK, and a frame shot into a flat wall (a real room's worth of pixels, but nothing lit) is
DARK.  The clean case is a frame with a visible wall and a lit feature, which must be LEGIBLE.

    python3 -m unittest discover -s tests/tools
"""

import os
import struct
import sys
import tempfile
import unittest

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools", "meetings"))
import legibility  # noqa: E402


def write_tga(path, w, h, pixel):
    """An uncompressed 24bpp top-left TGA; pixel(x, y) -> (r, g, b)."""
    header = struct.pack("<BBBHHBHHHHBB", 0, 0, 2, 0, 0, 0, 0, 0, w, h, 24, 0x20)
    with open(path, "wb") as f:
        f.write(header)
        for y in range(h):
            for x in range(w):
                r, g, b = pixel(x, y)
                f.write(bytes((b, g, r)))


class LegibilityTests(unittest.TestCase):
    def tmp(self, pixel):
        f = tempfile.NamedTemporaryFile(suffix=".tga", delete=False)
        f.close()
        self.addCleanup(os.unlink, f.name)
        write_tga(f.name, 200, 200, pixel)
        return f.name

    def test_a_pitch_black_frame_is_dark(self):
        m = legibility.measure(self.tmp(lambda x, y: (4, 4, 4)))
        self.assertTrue(m["median"] < 35 or m["p90"] < 80)

    def test_a_flat_wall_with_nothing_lit_is_dark(self):
        # the composition's own geometry: a lit dark texture, but no feature above p90 61
        m = legibility.measure(self.tmp(lambda x, y: (55, 55, 55)))
        self.assertLess(m["p90"], 80)

    def test_a_room_with_a_lit_feature_is_legible(self):
        def pixel(x, y):
            lit = 60 <= y < 160 and 60 <= x < 160   # a quarter of the frame is a lit panel
            return (230, 230, 210) if lit else (40, 40, 40)
        m = legibility.measure(self.tmp(pixel))
        self.assertGreaterEqual(m["median"], 35)
        self.assertGreaterEqual(m["p90"], 80)

    def test_a_red_console_counts_as_light(self):
        # astrometrics reads by its red consoles; a luma measure called that dark, the value measure
        # must not.
        self.assertEqual(legibility.value((220, 10, 10)), 220)
        self.assertGreater(legibility.value((220, 10, 10)), 0.299 * 220 + 0.587 * 10 + 0.114 * 10)


if __name__ == "__main__":
    unittest.main()
