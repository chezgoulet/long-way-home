"""Tests for the map-side tools: mapgen (brush winding), check-bsp (empty-lump detection) and
entitydict (the .def parser). No game data and no compiler needed.

    python3 -m unittest discover -s tests/tools
"""

import json
import os
import re
import struct
import subprocess
import sys
import tempfile
import unittest

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
MAPGEN = os.path.join(ROOT, "tools", "mapgen", "mapgen.py")
CHECK_BSP = os.path.join(ROOT, "tools", "mapgen", "check-bsp.py")
ENTITYDICT = os.path.join(ROOT, "tools", "entitydict", "entitydict.py")

POINT = r"\(\s*(-?\d+) (-?\d+) (-?\d+)\s*\)"
FACE = re.compile(rf"^{POINT}\s*{POINT}\s*{POINT}", re.M)
BRUSH = re.compile(r"\{\n((?:\(.*\n)+)\}")


def run(*cmd):
    return subprocess.run([sys.executable, *cmd], capture_output=True, text=True, stdin=subprocess.DEVNULL)


def cross(u, v):
    return (u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0])


def sub(a, b):
    return tuple(p - q for p, q in zip(a, b))


def dot(a, b):
    return sum(p * q for p, q in zip(a, b))


class MapGen(unittest.TestCase):
    def generate(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = os.path.join(tmp, "room.map")
            p = run(MAPGEN, "--out", out, "--name", "room")
            self.assertEqual(p.returncode, 0, p.stderr)
            with open(out) as f:
                return f.read()

    def test_every_brush_face_winds_inward(self):
        """The fault that once produced a BSP with empty lumps and a zero exit: a face whose three
        points give a normal pointing out of its brush. Checked here, before any compiler runs."""
        brushes = BRUSH.findall(self.generate())
        self.assertGreaterEqual(len(brushes), 6, "a closed room needs a floor, a ceiling and four walls")
        for brush in brushes:
            faces = [tuple(int(v) for v in m.groups()) for m in FACE.finditer(brush)]
            self.assertEqual(len(faces), 6)
            points = [f[i:i + 3] for f in faces for i in (0, 3, 6)]
            centre = tuple(sum(p[i] for p in points) / len(points) for i in range(3))
            for f in faces:
                a, b, c = f[0:3], f[3:6], f[6:9]
                normal = cross(sub(b, a), sub(c, a))
                self.assertNotEqual(normal, (0, 0, 0), "degenerate face")
                self.assertGreater(dot(normal, sub(centre, a)), 0, f"face {f} winds outward")

    def test_the_room_is_inhabitable(self):
        text = self.generate()
        classes = re.findall(r'"classname" "([^"]+)"', text)
        self.assertEqual(classes.count("worldspawn"), 1)
        self.assertEqual(classes.count("info_player_start"), 1)
        self.assertGreater(classes.count("waypoint"), 0, "geometry without navigation holds no crew")
        self.assertNotIn("common/caulk", text, "a room built of caulk loads and renders as a void")
        heights = {int(z) for z in re.findall(r'"classname" "waypoint"\n"origin" "-?\d+ -?\d+ (-?\d+)"', text)}
        self.assertEqual(heights, {24}, "waypoints stand on the floor (z=0): origin 24 above it, not in it")


class CheckBsp(unittest.TestCase):
    # Lump order: entities, shaders, planes, nodes, leafs, leafsurfaces, leafbrushes, models,
    # brushes, brushsides, drawverts, drawindexes, fogs, surfaces, lightmaps, lightgrid, visibility
    def check(self, lengths, magic=b"IBSP"):
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "m.bsp")
            with open(path, "wb") as f:
                f.write(magic + struct.pack("<i", 46))
                for n in lengths:
                    f.write(struct.pack("<ii", 8 + 17 * 8, n))
            return run(CHECK_BSP, path)

    def test_full_map_passes(self):
        p = self.check([100] * 17)
        self.assertEqual(p.returncode, 0, p.stdout)
        self.assertIn("OK", p.stdout)

    def test_each_required_lump_is_required(self):
        for index, name in ((1, "shaders"), (8, "brushes"), (13, "surfaces")):
            with self.subTest(lump=name):
                lengths = [100] * 17
                lengths[index] = 0
                p = self.check(lengths)
                self.assertEqual(p.returncode, 1, p.stdout)
                self.assertIn(name, p.stdout)

    def test_unlit_map_is_still_a_map(self):
        lengths = [100] * 17
        for index in (12, 14, 15, 16):  # fogs, lightmaps, lightgrid, visibility
            lengths[index] = 0
        self.assertEqual(self.check(lengths).returncode, 0)

    def test_not_a_bsp(self):
        p = self.check([100] * 17, magic=b"RBSP")
        self.assertNotEqual(p.returncode, 0)
        self.assertIn("not a BSP", p.stdout + p.stderr)

    def test_no_arguments_is_a_usage_error_not_a_hang(self):
        self.assertNotEqual(run(CHECK_BSP).returncode, 0)


DEF = """/*QUAKED waypoint (0.7 0.7 0) (-12 -12 -24) (12 12 32) ONEWAY
a place to go.
"targetname" - name
radius - how far
*/
/*QUAKED NPC_* (1 0 0) (-12 -12 -24) (12 12 32) x RIFLEMAN
family template
*/
/*QUAKED info_null (0 0.5 0) ? */
/*QUAKED after_oneliner (0 0 1) (-8 -8 -8) (8 8 8)
"wait" - seconds
*/
"""


class EntityDict(unittest.TestCase):
    def test_parse(self):
        with tempfile.TemporaryDirectory() as tmp:
            src, out = os.path.join(tmp, "e.def"), os.path.join(tmp, "e.json")
            with open(src, "w") as f:
                f.write(DEF)
            p = run(ENTITYDICT, "parse", "--def", src, "--json", out)
            self.assertEqual(p.returncode, 0, p.stderr)
            with open(out) as f:
                by_name = json.load(f)
            # a bare output filename (no directory part) must work too
            bare = subprocess.run([sys.executable, ENTITYDICT, "parse", "--def", src, "--json", "bare.json"],
                                  capture_output=True, text=True, cwd=tmp)
            self.assertEqual(bare.returncode, 0, bare.stderr)
            self.assertTrue(os.path.exists(os.path.join(tmp, "bare.json")))
        self.assertEqual(set(by_name), {"waypoint", "NPC_*", "info_null", "after_oneliner"})
        self.assertIn("radius", by_name["waypoint"]["keys"], "keys come from the body, which starts on line two")
        self.assertEqual(by_name["info_null"]["flags"], [])
        self.assertEqual(by_name["info_null"]["keys"], [], "a one-line entry must not borrow the next entry's body")
        self.assertEqual(by_name["after_oneliner"]["keys"], ["wait"])
        self.assertEqual(by_name["waypoint"]["flags"], ["ONEWAY"])
        self.assertIn("targetname", by_name["waypoint"]["keys"])
        self.assertEqual(by_name["waypoint"]["mins"], ["-12", "-12", "-24"])
        self.assertTrue(by_name["NPC_*"]["wildcard"])
        self.assertEqual(by_name["NPC_*"]["flags"], ["RIFLEMAN"], "the 'x' placeholder is not a flag")


if __name__ == "__main__":
    unittest.main()
