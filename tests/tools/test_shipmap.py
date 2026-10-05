"""Tests for tools/shipmap/stitch.py on synthetic deck sources. No game data needed.

    python3 -m unittest discover -s tests/tools
"""

import os
import re
import sys
import tempfile
import unittest

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools", "shipmap"))
import stitch  # noqa: E402


def box(z0, z1, tex="t/wall"):
    faces = [((0, 0, z0), (0, 64, z0), (64, 64, z0)), ((0, 0, z1), (64, 0, z1), (64, 64, z1)),
             ((0, 0, z0), (64, 0, z0), (64, 0, z1)), ((0, 64, z0), (0, 64, z1), (64, 64, z1)),
             ((0, 0, z0), (0, 0, z1), (0, 64, z1)), ((64, 0, z0), (64, 64, z0), (64, 64, z1))]
    return "{\n" + "".join("( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s 16 32 0 0.5 0.5 0 0 0\n" % (*a, *b, *c, tex)
                           for a, b, c in faces) + "}\n"


PATCH = """{
patchDef2
{
t/curve
( 3 3 0 0 0 )
(
( ( 0 0 -4000 0 0 ) ( 0 32 -4000 0 1 ) ( 0 64 -4000 0 2 ) )
( ( 32 0 -3990 1 0 ) ( 32 32 -3990 1 1 ) ( 32 64 -3990 1 2 ) )
( ( 64 0 -4000 2 0 ) ( 64 32 -4000 2 1 ) ( 64 64 -4000 2 2 ) )
)
}
}
"""


def ent(brushes="", **kv):
    return "{\n" + "".join('"%s" "%s"\n' % (k, v) for k, v in kv.items()) + brushes + "}\n"


def deck(extra=""):
    return (ent(box(-4096, -4000) + box(-8, 8) + PATCH, classname="worldspawn", message="deck")
            + ent(classname="info_player_start", origin="100 100 -3976")
            + ent(box(-4000, -3900), classname="func_group")
            + ent(box(-4000, -3900), classname="func_static")
            + ent(box(-4000, -3900), classname="func_static", targetname="lamp")
            + ent(box(-4000, -3900), classname="func_door", targetname="door1", team="shared")
            + ent(box(-4000, -3900), classname="trigger_multiple", target="door1")
            + extra)


class Stitch(unittest.TestCase):
    def run_stitch(self, decks, pitch=2048):
        with tempfile.TemporaryDirectory() as tmp:
            files = {}
            for n, text in decks.items():
                files[n] = os.path.join(tmp, "deck%02d.map" % n)
                with open(files[n], "w") as f:
                    f.write(text)
            world_keys, world, ents, report = stitch.stitch(files, pitch)
            out = os.path.join(tmp, "ship.map")
            stitch.write(out, world_keys, world, ents)
            return world, ents, report, stitch.parse(out)

    def test_decks_are_placed_at_their_own_heights(self):
        world, ents, report, _ = self.run_stitch({1: deck(), 3: deck()})
        self.assertEqual(report["decks"][1]["z"], [-4096, -3900])
        self.assertEqual(report["decks"][3]["z"], [-4096 - 2 * 2048, -3900 - 2 * 2048])
        self.assertEqual(report["overlaps"], [])
        origins = [k["origin"] for k, _ in ents if "origin" in k]
        # deck 1: the arrival point and the player start, in the same place; deck 3: its arrival point
        self.assertEqual(origins, ["100 100 -3976", "100 100 -3976", "100 100 %d" % (-3976 - 4096)])

    def test_only_plane_points_move_and_textures_stay(self):
        world, _, _, _ = self.run_stitch({2: deck()})
        face = world[0][0]
        self.assertTrue(face.startswith("( 0 0 %d ) ( 0 64 %d ) ( 64 64 %d ) t/wall 16 32 0 0.5 0.5 0 0 0" % ((-4096 - 2048,) * 3)), face)

    def test_patch_rows_move_and_the_patch_header_does_not(self):
        world, _, _, _ = self.run_stitch({2: deck()})
        patch = [b for b in world if b[0].startswith("patchDef2")][0]
        self.assertIn("( 3 3 0 0 0 )", patch)  # width, height: the same shape as a row, and not one
        rows = [l for l in patch if l.startswith("( (")]
        self.assertEqual(rows[1], "( ( 32 0 %d 1 0 ) ( 32 32 %d 1 1 ) ( 32 64 %d 1 2 ) )" % ((-3990 - 2048,) * 3))

    def test_stray_brush_is_dropped(self):
        world, _, report, _ = self.run_stitch({1: deck()})
        self.assertEqual(report["dropped_stray_brushes"], 1)
        self.assertTrue(all("( 0 0 -8 )" not in l for b in world for l in b))

    def test_groups_and_unnamed_statics_fold_into_the_world(self):
        world, ents, report, _ = self.run_stitch({1: deck()})
        self.assertEqual(report["folded"], {"func_group": 1, "func_static": 1})
        self.assertEqual(len(world), 2 + 2)  # floor, patch, and the two folded boxes
        classes = [k["classname"] for k, _ in ents]
        self.assertNotIn("func_group", classes)
        self.assertEqual(classes.count("func_static"), 1)  # the named one can be addressed: it stays
        self.assertEqual(report["models"], {"func_static": 1, "func_door": 1, "trigger_multiple": 1})

    def test_only_names_shared_between_decks_are_renamed(self):
        d1 = deck(ent(classname="target_relay", targetname="only_here", target="door1"))
        _, ents, report, _ = self.run_stitch({1: d1, 4: deck()})
        self.assertEqual(report["renamed"], ["door1", "lamp", "shared"])
        names = {k.get("targetname") for k, _ in ents}
        self.assertIn("d01_door1", names)
        self.assertIn("d04_door1", names)
        self.assertIn("only_here", names)  # one deck only: the scripts that name it still find it
        relay = [k for k, _ in ents if k["classname"] == "target_relay"][0]
        self.assertEqual(relay["target"], "d01_door1")  # references follow the rename
        triggers = sorted(k["target"] for k, _ in ents if k["classname"] == "trigger_multiple")
        self.assertEqual(triggers, ["d01_door1", "d04_door1"])
        self.assertEqual(sorted(k["team"] for k, _ in ents if "team" in k), ["d01_shared", "d04_shared"])

    def test_names_the_maps_only_refer_to_are_not_renamed(self):
        """Both decks target "munro", and neither defines it: it is the player, made by the game."""
        extra = ent(classname="target_relay", target="munro")
        _, ents, report, _ = self.run_stitch({1: deck(extra), 2: deck(extra)})
        self.assertNotIn("munro", report["renamed"])
        self.assertEqual({k["target"] for k, _ in ents if k["classname"] == "target_relay"}, {"munro"})

    def test_one_player_start(self):
        _, ents, _, _ = self.run_stitch({1: deck(), 2: deck(), 5: deck()})
        classes = [k["classname"] for k, _ in ents]
        self.assertEqual(classes.count("info_player_start"), 1)
        arrivals = sorted(k["targetname"] for k, _ in ents if k["classname"] == "info_notnull")
        self.assertEqual(arrivals, ["d01_arrival", "d02_arrival", "d05_arrival"])

    def test_turbolift_travels_within_the_ship(self):
        lift = (ent(classname="target_level_change", targetname="tour_turbo_03", mapname="tour/deck03", target="door")
                + ent(classname="target_level_change", targetname="tour_turbo_07", mapname="tour/deck07")
                + ent(classname="target_level_change", targetname="gotobrig", mapname="_brig"))
        _, ents, report, _ = self.run_stitch({1: deck(lift), 3: deck(lift)})
        by_name = {k.get("targetname"): k for k, _ in ents}
        to3 = by_name["d01_tour_turbo_03"]
        self.assertEqual((to3["classname"], to3["target"]), ("target_teleporter", "d03_arrival"))
        self.assertNotIn("mapname", to3)
        self.assertIn("d03_arrival", by_name)
        # deck 7 is not in this ship, and the brig is not a deck: both still change level
        self.assertEqual(by_name["d01_tour_turbo_07"]["classname"], "target_level_change")
        self.assertEqual(by_name["d01_gotobrig"]["mapname"], "_brig")
        self.assertEqual(report["turbolift_links"], 2)
        self.assertEqual(report["level_changes_left"], ["_brig", "tour/deck07"])

    def test_every_entity_knows_its_deck_and_the_output_parses(self):
        _, ents, report, reparsed = self.run_stitch({1: deck(), 9: deck()})
        self.assertEqual({k["lwh_deck"] for k, _ in ents}, {"1", "9"})
        self.assertEqual(len(reparsed), report["entities"])
        self.assertEqual(reparsed[0][0]["classname"], "worldspawn")
        self.assertEqual(sum(len(b) for _, b in reparsed), report["world_brushes"] + report["brush_models"])

    def test_missing_textures_are_substituted_on_faces_and_patches(self):
        patch = PATCH.replace("t/curve", "hall/hallcomp")
        src = ent(box(-4096, -4000, tex="hall/hallfloor2") + box(-4000, -3990, tex="t/wall") + patch, classname="worldspawn")
        world, _, report, _ = self.run_stitch({1: src})
        text = "\n".join(l for b in world for l in b)
        self.assertNotIn("hall/hallfloor2", text)
        self.assertIn(") hall/hallfloor1 16 32 0 0.5 0.5 0 0 0", text)  # the alignment after the name is untouched
        self.assertIn("\nhall/hallcomp2\n", text)
        self.assertIn("t/wall", text)
        self.assertEqual(report["textures_substituted"], 6 + 1)

    def test_overlap_and_bad_pitch_are_reported(self):
        tall = ent(box(-4096, -1500) + box(-8, 8), classname="worldspawn")
        _, _, report, _ = self.run_stitch({1: tall, 2: tall}, pitch=2048)
        self.assertEqual(report["overlaps"], [[2, 1]])
        with self.assertRaises(ValueError):
            self.run_stitch({1: deck()}, pitch=1000)

    def test_malformed_sources_are_refused(self):
        with self.assertRaises(ValueError):
            self.run_stitch({1: ent(classname="info_null")})
        with self.assertRaises(ValueError):
            self.run_stitch({1: '{\n"classname" "worldspawn"\n'})


if __name__ == "__main__":
    unittest.main()
