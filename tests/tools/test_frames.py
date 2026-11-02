"""Tests for the frame instrument (tools/frames/frame.py).

The tool exists so a check can fail for the reason its claim cares about -- a dark room, a screen
that did not open -- without counting lights or grepping logs.  So the tests that carry the weight
are the ones where it *fails*, and the ones that prove a region is a region and a difference is a
difference:

  * floor: a pitch-black frame is DARK; a flat frame with nothing lit is DARK; a uniform bright
    fill is DARK once the spread floor is asked for; a lit, structured room is LEGIBLE;
  * region: the same frame passes as a whole and fails in a named black region, so a check can be
    made to ask about the part it means;
  * diff: two identical frames are SAME; a changed region DIFFERS; the *same* change outside the
    measured region is SAME -- which is what makes the region worth naming.

    python3 -m unittest discover -s tests/tools
"""

import os
import struct
import sys
import tempfile
import unittest

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools", "frames"))
import frame  # noqa: E402


def write_tga(path, w, h, pixel):
    """An uncompressed 24bpp top-left TGA; pixel(x, y) -> (r, g, b)."""
    header = struct.pack("<BBBHHBHHHHBB", 0, 0, 2, 0, 0, 0, 0, 0, w, h, 24, 0x20)
    with open(path, "wb") as f:
        f.write(header)
        for y in range(h):
            for x in range(w):
                r, g, b = pixel(x, y)
                f.write(bytes((b, g, r)))


def R(x0, y0, x1, y1):
    """A region tuple as the tool makes them: (label, x0, y0, x1, y1)."""
    return ("r", x0, y0, x1, y1)


def opts(mode="floor", **kw):
    """A parsed-namespace for the tool, with only the given options set."""
    argv = [mode]
    for k, v in kw.items():
        argv += ["--" + k.replace("_", "-"), str(v)]
    argv += ["/dev/null"]
    return frame.build_parser().parse_args(argv)


class FrameTests(unittest.TestCase):
    def tmp(self, pixel, w=200, h=200):
        f = tempfile.NamedTemporaryFile(suffix=".tga", delete=False)
        f.close()
        self.addCleanup(os.unlink, f.name)
        write_tga(f.name, w, h, pixel)
        return f.name

    # ---- floor ---------------------------------------------------------------------------------
    def test_a_pitch_black_frame_is_dark(self):
        path = self.tmp(lambda x, y: (4, 4, 4))
        w, h, rows = frame.read_tga(path)
        m = frame.summarize(frame.region_pixels(rows, R(0, 0, w, h)))
        self.assertIsNotNone(frame.floor_reason(m, frame.region_pixels(rows, R(0, 0, w, h)), opts()))

    def test_a_flat_wall_with_nothing_lit_is_dark(self):
        path = self.tmp(lambda x, y: (55, 55, 55))
        w, h, rows = frame.read_tga(path)
        px = frame.region_pixels(rows, R(0, 0, w, h))
        why = frame.floor_reason(frame.summarize(px), px, opts())
        self.assertIn("nothing lit", why)

    def test_a_uniform_bright_fill_is_dark_when_the_spread_floor_is_asked_for(self):
        # a grey fill is "not black" but it is "uniform": the brief's floor is both.
        path = self.tmp(lambda x, y: (200, 200, 200))
        w, h, rows = frame.read_tga(path)
        px = frame.region_pixels(rows, R(0, 0, w, h))
        self.assertIsNone(frame.floor_reason(frame.summarize(px), px, opts()))
        why = frame.floor_reason(frame.summarize(px), px, opts(min_spread=20))
        self.assertIn("uniform", why)

    def test_a_room_with_a_lit_feature_is_legible(self):
        def pixel(x, y):
            lit = 60 <= y < 160 and 60 <= x < 160   # a quarter of the frame is a lit panel
            return (230, 230, 210) if lit else (40, 40, 40)
        path = self.tmp(pixel)
        w, h, rows = frame.read_tga(path)
        px = frame.region_pixels(rows, R(0, 0, w, h))
        self.assertIsNone(frame.floor_reason(frame.summarize(px), px, opts(min_spread=20)))

    def test_a_red_console_counts_as_light(self):
        self.assertEqual(frame.value((220, 10, 10)), 220)
        self.assertGreater(frame.value((220, 10, 10)), 0.299 * 220 + 0.587 * 10 + 0.114 * 10)

    def test_thin_bright_text_can_be_asked_for_with_min_bright_px(self):
        # a screen is mostly black with thin bright text: p90 can be 0, so "something lit" must be
        # askable as a count of clearly-bright pixels.
        def pixel(x, y):
            return (240, 240, 240) if (y % 20 == 0 and x % 3 == 0) else (0, 0, 0)
        path = self.tmp(pixel)
        w, h, rows = frame.read_tga(path)
        px = frame.region_pixels(rows, R(0, 0, w, h))
        self.assertIsNotNone(frame.floor_reason(frame.summarize(px), px, opts()))
        # a screen is mostly black with thin bright text: relax the room floors and ask for lit pixels
        screen = opts(min_median=0, min_p90=0, min_bright_px=100)
        self.assertIsNone(frame.floor_reason(frame.summarize(px), px, screen))

    # ---- region --------------------------------------------------------------------------------
    def test_a_region_is_the_part_it_names(self):
        def pixel(x, y):
            return (220, 220, 220) if x < 100 else (0, 0, 0)   # lit left half, black right half
        path = self.tmp(pixel)
        w, h, rows = frame.read_tga(path)
        whole = frame.region_pixels(rows, R(0, 0, w, h))
        left = frame.region_pixels(rows, R(0, 0, 100, h))
        right = frame.region_pixels(rows, R(100, 0, w, h))
        self.assertIsNone(frame.floor_reason(frame.summarize(left), left, opts()))
        self.assertIsNotNone(frame.floor_reason(frame.summarize(right), right, opts()))
        # the whole frame passes (it has a lit half) even though the right half is black
        self.assertIsNone(frame.floor_reason(frame.summarize(whole), whole, opts()))

    def test_a_region_outside_the_frame_is_rejected(self):
        with self.assertRaises(ValueError):
            frame.parse_region("0,0,500,500", 200, 200)

    # ---- diff ----------------------------------------------------------------------------------
    def test_identical_frames_are_same(self):
        path = self.tmp(lambda x, y: (60, 60, 60))
        a = frame.build_parser().parse_args(["diff", path, path])
        self.assertEqual(frame.do_diff(a), 0)
        rc = frame.build_parser().parse_args(["diff", "--require", path, path])
        self.assertEqual(frame.do_diff(rc), 1)

    def test_a_changed_region_differs(self):
        before = self.tmp(lambda x, y: (60, 60, 60))
        after = self.tmp(lambda x, y: (220, 220, 220) if 60 <= y < 160 else (60, 60, 60))
        a = frame.build_parser().parse_args(["diff", "--require", "--min-changed", "0.1", before, after])
        self.assertEqual(frame.do_diff(a), 0)

    def test_a_change_outside_the_measured_region_is_same(self):
        before = self.tmp(lambda x, y: (60, 60, 60))
        after = self.tmp(lambda x, y: (220, 220, 220) if x < 100 else (60, 60, 60))
        # the change is in the left half; measure the right half and it is SAME
        a = frame.build_parser().parse_args(
            ["diff", "--require", "--min-changed", "0.05", "--region", "right:100,0,200,200", before, after])
        self.assertEqual(frame.do_diff(a), 1)

    # ---- alien ---------------------------------------------------------------------------------
    def test_an_alien_deck_is_lit_built_and_coloured(self):
        # the three clauses at once: a lit room (median/p90), structure (an edge fraction) and a
        # palette (a channel spread in enough pixels).
        def pixel(x, y):
            if (x // 20 + y // 20) % 2:
                return (150, 90, 40)          # a coloured panel
            return (20, 60, 30)               # a dark green wall
        path = self.tmp(pixel)
        w, h, rows = frame.read_tga(path)
        region = R(0, 0, w, h)
        px = frame.region_pixels(rows, region)
        why = frame.alien_reason(frame.summarize(px), frame.edge_fraction(rows, region, 16),
                                 frame.chroma_fraction(px, 24), opts("alien"))
        self.assertIsNone(why)

    def test_an_alien_deck_that_is_dark_fails_the_maintained_clause(self):
        # the derelict's failing light is a different archetype: this one must fail it.
        path = self.tmp(lambda x, y: (4, 4, 4))
        w, h, rows = frame.read_tga(path)
        region = R(0, 0, w, h)
        px = frame.region_pixels(rows, region)
        why = frame.alien_reason(frame.summarize(px), 0.01, 0.5, opts("alien"))
        self.assertIn("too dark", why)

    def test_an_alien_deck_that_is_a_flat_fill_fails_the_built_clause(self):
        path = self.tmp(lambda x, y: (90, 80, 70))
        w, h, rows = frame.read_tga(path)
        region = R(0, 0, w, h)
        px = frame.region_pixels(rows, region)
        why = frame.alien_reason(frame.summarize(px), 0.0001, 0.5, opts("alien"))
        self.assertIn("no structure", why)

    def test_an_alien_deck_with_no_palette_fails_the_design_language_clause(self):
        # the drab capture: the same geometry and light, every surface and lamp neutral grey. It is
        # lit and structured, so only the palette clause can reject it.
        def pixel(x, y):
            return (120, 120, 120) if (x // 20 + y // 20) % 2 else (40, 40, 40)
        path = self.tmp(pixel)
        w, h, rows = frame.read_tga(path)
        region = R(0, 0, w, h)
        px = frame.region_pixels(rows, region)
        cf = frame.chroma_fraction(px, 24)
        self.assertEqual(cf, 0.0)
        why = frame.alien_reason(frame.summarize(px), frame.edge_fraction(rows, region, 16), cf,
                                 opts("alien"))
        self.assertIn("neutral grey", why)

    def test_chroma_counts_only_pixels_with_a_channel_spread(self):
        self.assertEqual(frame.chroma_fraction([(10, 10, 10), (100, 70, 70)], 24), 0.5)
        self.assertEqual(frame.chroma_fraction([(10, 10, 10)], 24), 0.0)
        self.assertEqual(frame.chroma_fraction([], 24), 0.0)

    # ---- the reader ----------------------------------------------------------------------------
    def test_a_truncated_frame_is_rejected(self):
        f = tempfile.NamedTemporaryFile(suffix=".tga", delete=False)
        f.write(struct.pack("<BBBHHBHHHHBB", 0, 0, 2, 0, 0, 0, 0, 0, 200, 200, 24, 0x20) + b"\0" * 10)
        f.close()
        self.addCleanup(os.unlink, f.name)
        with self.assertRaises(ValueError):
            frame.read_tga(f.name)


if __name__ == "__main__":
    unittest.main()
