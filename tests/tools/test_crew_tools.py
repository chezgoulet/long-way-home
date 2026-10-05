"""Tests for the crew authoring tools: crewgen (validate, render, suggest), g3report (the verdict)
and the validator's E008. No game data is needed: the "installation" is a pk3 built here around a
few entities, which is all these tools read from a real one.

    python3 -m unittest discover -s tests/tools
"""

import contextlib
import io
import json
import os
import struct
import subprocess
import sys
import tempfile
import unittest
import zipfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools", "crewgen"))
import crewgen  # noqa: E402
import g3report  # noqa: E402


def ent(**kv):
    return "{\n" + "".join(f'"{k}" "{v}"\n' for k, v in kv.items()) + "}\n"


def fake_installation(root, entities):
    """A BaseEF/pak0.pk3 holding maps/tour/deck99.bsp (header + entity lump only) and an NPC table."""
    text = "".join(entities).encode("latin-1") + b"\0"
    header = b"IBSP" + struct.pack("<i", 46) + struct.pack("<ii", 8 + 17 * 8, len(text)) + b"\0" * (16 * 8)
    os.makedirs(os.path.join(root, "BaseEF"))
    with zipfile.ZipFile(os.path.join(root, "BaseEF", "pak0.pk3"), "w") as z:
        z.writestr("maps/tour/deck99.bsp", header + text)
        z.writestr("ext_data/NPCs.cfg", "Renner\n{\n}\n\nShowers\n{\n}\nGreen\n{\n}\n")
    return root


def corridor(n=14, step=120):
    ents = [ent(classname="worldspawn")]
    ents += [ent(classname="waypoint", origin=f"{i * step} 0 0") for i in range(n)]
    ents.append(ent(classname="waypoint_navgoal_1", targetname="console", origin="0 40 0"))
    ents.append(ent(classname="NPC_starfleet", NPC_targetname="Laird", origin="600 30 0"))
    ents.append(ent(classname="info_player_start", origin="1200 30 0"))
    return ents


def section(**over):
    crew = {
        "map": "tour/deck99",
        "max": 6,
        "members": [{"name": f"w{i}", "type": "Renner", "at": [240 * i, 0, 0]} for i in range(5)],
        "posts": [{"name": f"p{i}", "at": [240 * i + 120, 0, 0], "holder": f"w{i}"} for i in range(5)],
    }
    crew.update(over)
    return crew


class Validate(unittest.TestCase):
    def errors(self, crew, data=None):
        return crewgen.validate(crew, data)[0]

    def assertFault(self, crew, needle, data=None):
        errors = self.errors(crew, data)
        self.assertTrue(any(needle in e for e in errors), f"expected an error containing {needle!r}, got {errors}")

    def test_clean_section_passes(self):
        errors, warnings = crewgen.validate(section())
        self.assertEqual(errors, [])
        self.assertEqual(warnings, [])

    def test_each_structural_fault_is_named(self):
        cases = [
            (section(map="maps/tour/deck99.bsp"), "crew.map"),
            (section(map=None), "crew.map"),
            (section(max=0), "crew.max"),
            (section(max=3), "crew.max is 3"),
            (section(extra=1), "unknown key 'extra'"),
            (section(bounds={"reach_ms": -1}), "crew.bounds.reach_ms"),
            (section(bounds={"speed": 1}), "unknown key 'speed'"),
            (section(members=[{"name": "a b"}]), "members[0].name"),
            (section(members=[{"name": "a", "type": "Renner"}]), "both 'type' and 'at'"),
            (section(members=[{"name": "a", "type": "Renner", "at": [1, 2]}]), "members[0].at"),
            (section(members=[{"name": "a", "type": "Renner", "at": [1, 2, float("nan")]}]), "members[0].at"),
            (section(members=[{"name": "a"}, {"name": "A"}]), "duplicate 'A'"),
            (section(posts=[{"name": "p", "at": [0, 0, 0], "navgoal": "console"}]), "exactly one of"),
            (section(posts=[{"name": "p"}]), "exactly one of"),
            (section(posts=[{"name": "p", "at": [0, 0, 0], "radius": 0}]), "posts[0].radius"),
            (section(posts=[{"name": "p", "at": [0, 0, 0], "yaw": "north"}]), "posts[0].yaw"),
            (section(posts=[{"name": "p", "at": [0, 0, 0], "holder": "nobody"}]), "'nobody' is not a declared member"),
            (section(posts=[{"name": "p", "at": [0, 0, 0]}, {"name": "p", "at": [9, 0, 0]}]), "duplicate 'p'"),
            ("not an object", "must be an object"),
        ]
        for crew, needle in cases:
            with self.subTest(needle=needle):
                self.assertFault(crew, needle)

    def test_g3_bar_is_a_warning_not_an_error(self):
        errors, warnings = crewgen.validate(section(members=[{"name": "solo"}], posts=[]))
        self.assertEqual(errors, [])
        self.assertTrue(any("G3 bar" in w for w in warnings))

    def test_checked_against_the_map(self):
        with tempfile.TemporaryDirectory() as tmp:
            data = crewgen.GameData(fake_installation(tmp, corridor()))
            self.assertEqual(self.errors(section(), data), [])
            self.assertFault(section(map="tour/nowhere"), "nor a map in the game data", data)
            self.assertFault(section(posts=[{"name": "p", "navgoal": "helm"}]), "no waypoint_navgoal named 'helm'", data)
            self.assertEqual(self.errors(section(posts=[{"name": "p", "navgoal": "CONSOLE"}]), data), [])
            self.assertFault(section(posts=[{"name": "p", "at": [0, 5000, 0]}]), "from any waypoint", data)
            self.assertFault(section(members=[{"name": "a", "type": "Gorn", "at": [0, 0, 0]}], posts=[]), "not in the game's NPC table", data)
            self.assertFault(section(members=[{"name": "a", "type": "Renner", "at": [0, 9000, 0]}], posts=[]), "from any waypoint", data)
            self.assertFault(section(members=[{"name": "Laird", "type": "Renner", "at": [0, 0, 0]}], posts=[]), "already places", data)
            self.assertFault(section(members=[{"name": "Ghost"}], posts=[]), "places no NPC named 'Ghost'", data)
            self.assertEqual(self.errors(section(members=[{"name": "laird"}], posts=[]), data), [])

    def test_installation_without_paks_is_reported(self):
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaises(FileNotFoundError):
                crewgen.GameData(tmp)


class Render(unittest.TestCase):
    def test_every_field_reaches_the_file(self):
        crew = section(
            bounds={"reach_ms": 60000, "save_bytes_per_npc": 128},
            members=[{"name": "w0", "type": "Renner", "at": [-10, 2.5, 0], "yaw": 90}, {"name": "Laird"}],
            posts=[{"name": "helm", "navgoal": "console", "priority": 2, "holder": "w0"},
                   {"name": "aft", "at": [1, 2, 3], "yaw": 180, "radius": 32}])
        self.assertEqual(crewgen.validate(crew)[0], [])
        lines = crewgen.render(crew, "scenario.json").splitlines()
        self.assertTrue(lines[0].startswith("#"))
        self.assertEqual(lines[1:], [
            "crew max 6",
            "bound reach_ms 60000",
            "budget save_bytes_per_npc 128",
            "member w0 type Renner at -10 2.5 0 yaw 90",
            "member Laird",
            "post helm navgoal console priority 2 holder w0",
            "post aft at 1 2 3 yaw 180 radius 32",
        ])


class Suggest(unittest.TestCase):
    def test_suggestion_is_valid_and_keeps_clear(self):
        with tempfile.TemporaryDirectory() as tmp:
            data = crewgen.GameData(fake_installation(tmp, corridor()))
            crew = crewgen.suggest(data, "tour/deck99", 5)
            errors, warnings = crewgen.validate(crew, data)
            self.assertEqual(errors, [])
            self.assertEqual(warnings, [])
            self.assertEqual(crew, crewgen.suggest(data, "tour/deck99", 5), "suggestions must be repeatable")
            taken = [tuple(m["at"]) for m in crew["members"]] + [tuple(p["at"]) for p in crew["posts"]]
            self.assertEqual(len(set(taken)), 10, "no two crew or posts share a waypoint")
            for pos in taken:
                for busy in [(600, 30, 0), (1200, 30, 0), (0, 40, 0)]:
                    self.assertGreater(crewgen.dist(pos, busy), 96)

    def test_too_small_a_map_is_refused(self):
        with tempfile.TemporaryDirectory() as tmp:
            data = crewgen.GameData(fake_installation(tmp, corridor(n=6)))
            with self.assertRaises(SystemExit):
                crewgen.suggest(data, "tour/deck99", 5)


def good_report():
    members = [{"ent": 10 + i, "duty_post": i, "override_post": -1, "goal_post": -1, "action": 2, "flags": 2,
                "schedule_cursor": 0, "first_arrival_ms": 3000 + i} for i in range(6)]
    return {"map": "tour/deck99", "members": members, "samples": 121, "sample_ms": 5000, "min_coverage_percent": 100,
            "coverage_bound_percent": 90, "reach_bound_ms": 90000, "stuck_events": 0, "out_of_world": 0, "addresses": 6,
            "acks": 6, "missed_acks": 0, "max_ack_ms": 1500, "ack_bound_ms": 5000, "violations": 0, "save_bytes": 152,
            "save_budget_per_npc": 256, "script_runs": 1, "script_yields": 1, "script_resumes": 1, "layer_avg_us": 3, "layer_max_us": 50, "game_frame_avg_us": 340,
            "scripts": {"running": ["Chell", "Tuvok"]}}


class Judge(unittest.TestCase):
    def status(self, criterion, **kw):
        report = kw.pop("report", good_report())
        full = dict(baseline={"game_frame_avg_us": 240, "scripts": {"running": ["Tuvok", "Chell"]}},
                    saved={"members": report["members"]}, restored={"members": report["members"]},
                    reloaded={"samples": 7, "min_coverage_percent": 100})
        full.update(kw)
        rows = g3report.judge(report, **full)
        return {name: status for name, status, _ in rows}[criterion]

    def test_a_complete_clean_run_passes_every_criterion(self):
        r = good_report()
        rows = g3report.judge(r, {"game_frame_avg_us": 240, "scripts": {"running": ["Tuvok", "Chell"]}},
                              {"members": r["members"]}, {"members": r["members"]},
                              {"samples": 7, "min_coverage_percent": 100}, [1000, 5000])
        self.assertEqual([row for row in rows if row[1] != "PASS"], [])
        self.assertEqual(len(rows), 11)

    def test_each_criterion_can_fail(self):
        def broken(**change):
            r = good_report()
            r.update(change)
            return r

        gone = good_report()
        gone["members"] = gone["members"][:4]
        late = good_report()
        late["members"][2]["first_arrival_ms"] = -1
        cases = [
            ("5-10 crew present on one deck", dict(report=gone)),
            ("post coverage at every sample", dict(report=broken(min_coverage_percent=83))),
            ("observation run long enough", dict(report=broken(samples=7))),
            ("every NPC reaches its post within the bound", dict(report=late)),
            ("zero navigation failures", dict(report=broken(stuck_events=1))),
            ("zero navigation failures", dict(report=broken(out_of_world=1))),
            ("address -> acknowledgement within the bound", dict(report=broken(acks=5, missed_acks=1))),
            ("address -> acknowledgement within the bound", dict(report=broken(max_ack_ms=9000))),
            ("no ICARUS script regressions", dict(report=broken(violations=1))),
            ("no ICARUS script regressions", dict(baseline={"game_frame_avg_us": 240, "scripts": {"running": ["Tuvok"]}})),
            ("a script takes a post-holder, and gives them back", dict(report=broken(script_yields=0))),
            ("a script takes a post-holder, and gives them back", dict(report=broken(script_resumes=0))),
            ("save/load restores posts and schedule cursor", dict(restored={"members": gone["members"]})),
            ("save/load restores posts and schedule cursor", dict(reloaded={"samples": 7, "min_coverage_percent": 50})),
            ("save-size increase within budget", dict(report=broken(save_bytes=6 * 256 + 1))),
            ("frame time holds", dict(report=broken(game_frame_avg_us=5000))),
        ]
        for criterion, kw in cases:
            with self.subTest(criterion=criterion, change=list(kw)):
                self.assertEqual(self.status(criterion, **kw), "FAIL")

    def test_missing_evidence_is_not_a_pass(self):
        self.assertEqual(self.status("no ICARUS script regressions", baseline=None), "NOT MEASURED")
        self.assertEqual(self.status("frame time holds", baseline=None), "NOT MEASURED")
        self.assertEqual(self.status("save/load restores posts and schedule cursor", restored=None), "NOT MEASURED")
        self.assertEqual(self.status("save/load restores posts and schedule cursor", reloaded=None), "NOT MEASURED")
        untested = good_report()
        untested.update(script_runs=0, script_yields=0, script_resumes=0)
        self.assertEqual(self.status("a script takes a post-holder, and gives them back", report=untested), "NOT MEASURED")
        silent = good_report()
        silent.update(addresses=0, acks=0)
        self.assertEqual(self.status("address -> acknowledgement within the bound", report=silent), "NOT MEASURED")

    def test_cli_exit_status_follows_the_verdict(self):
        with tempfile.TemporaryDirectory() as tmp:
            r = good_report()
            for suffix, body in [("report", r), ("baseline", {"game_frame_avg_us": 240, "scripts": {"running": ["Chell", "Tuvok"]}}),
                                 ("saved", {"members": r["members"]}), ("restored", {"members": r["members"]}),
                                 ("reloaded.report", {"samples": 7, "min_coverage_percent": 100})]:
                with open(os.path.join(tmp, f"k.{suffix}.json"), "w") as f:
                    json.dump(body, f)
            with contextlib.redirect_stdout(io.StringIO()) as out:
                self.assertEqual(g3report.main([tmp, "k"]), 0)
                os.remove(os.path.join(tmp, "k.baseline.json"))
                self.assertEqual(g3report.main([tmp, "k"]), 1)
                self.assertEqual(g3report.main([tmp, "absent"]), 1)
            self.assertIn("G3 measured run: PASS", out.getvalue())
            self.assertIn("NOT MEASURED", out.getvalue())


class ValidatorE008(unittest.TestCase):
    def run_validator(self, crew, data=None):
        with tempfile.TemporaryDirectory() as tmp:
            with open(os.path.join(tmp, "scenario.json"), "w") as f:
                json.dump({"name": "t", "maps": [], "scripts": [], "crew": crew}, f)
            cmd = [sys.executable, os.path.join(ROOT, "tools", "validator", "validate.py"), tmp]
            if data:
                cmd += ["--data", data]
            p = subprocess.run(cmd, capture_output=True, text=True)
            return p.returncode, p.stdout

    def test_bad_crew_section_is_e008(self):
        code, out = self.run_validator(section(posts=[{"name": "p"}]))
        self.assertEqual(code, 1)
        self.assertIn("E008", out)
        self.assertIn("exactly one of", out)

    def test_crew_only_scenario_needs_no_map_of_its_own(self):
        code, out = self.run_validator(section())
        self.assertEqual(code, 0, out)
        self.assertNotIn("E005", out)
        self.assertIn("W003", out)  # and says the section was not checked against the map

    def test_checked_against_the_installation_when_given(self):
        with tempfile.TemporaryDirectory() as tmp:
            fake_installation(tmp, corridor())
            code, out = self.run_validator(section(), tmp)
            self.assertEqual(code, 0, out)
            code, out = self.run_validator(section(posts=[{"name": "p", "navgoal": "helm"}]), tmp)
            self.assertEqual(code, 1)
            self.assertIn("no waypoint_navgoal named 'helm'", out)

    def test_scenario_owned_map_is_checked_from_its_source(self):
        """A scenario that brings its own .map needs no installation to be checked against it --
        and brush blocks in the source must not be mistaken for entities."""
        brush = "{\n( 0 0 0 ) ( 1 0 0 ) ( 0 1 0 ) tex 0 0 0 1 1\n}\n"
        source = '{\n"classname" "worldspawn"\n' + brush + "}\n" + "".join(corridor()[1:])
        self.assertEqual(len(crewgen.parse_entities(source)), len(corridor()))
        with tempfile.TemporaryDirectory() as tmp:
            os.makedirs(os.path.join(tmp, "maps", "tour"))
            with open(os.path.join(tmp, "maps", "tour", "deck99.map"), "w") as f:
                f.write(source)
            validator = os.path.join(ROOT, "tools", "validator", "validate.py")
            for crew, code, needle in [(section(), 0, ""),
                                       (section(posts=[{"name": "p", "at": [0, 7000, 0]}]), 1, "from any waypoint")]:
                with open(os.path.join(tmp, "scenario.json"), "w") as f:
                    json.dump({"name": "t", "maps": ["maps/**/*.map"], "scripts": [], "crew": crew}, f)
                p = subprocess.run([sys.executable, validator, tmp], capture_output=True, text=True)
                self.assertEqual(p.returncode, code, p.stdout)
                self.assertIn(needle, p.stdout)
                self.assertNotIn("not checked against the map", p.stdout)

    def test_the_shipped_scenarios_are_valid(self):
        for name in sorted(os.listdir(os.path.join(ROOT, "scenarios"))):
            path = os.path.join(ROOT, "scenarios", name)
            if not os.path.isfile(os.path.join(path, "scenario.json")):
                continue
            with self.subTest(scenario=name):
                p = subprocess.run([sys.executable, os.path.join(ROOT, "tools", "validator", "validate.py"), path],
                                   capture_output=True, text=True)
                self.assertEqual(p.returncode, 0, p.stdout)


if __name__ == "__main__":
    unittest.main()
