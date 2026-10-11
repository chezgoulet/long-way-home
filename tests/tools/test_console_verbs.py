"""Tests for tools/console/verbs.py: the ship's API freshness check (docs/evidence/the-computer-api.md)
and the console ladder's walk (docs/evidence/console-ladder-and-recall.md, Task B).

The check must fail in both directions -- a console verb missing from the enumeration, and an
enumeration row the console does not answer -- and pass on the real tree. The walk must report a
verb claimed more than once and tell the two cases apart: claims that differ by argument shape (both
reachable, a warning) and a claim that shadows another (the later can never run, a failure). No
engine is needed.

    python3 -m unittest discover -s tests/tools
"""

import importlib.util
import os
import tempfile
import unittest

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
VERBS = os.path.join(ROOT, "tools", "console", "verbs.py")
CONSOLE = os.path.join(ROOT, "module", "ship", "g_ship.cpp")
REGISTER = os.path.join(ROOT, "module", "ship", "console_api.def")

MINI_CONSOLE = """
void Svcmd_Ship_f( void )
{
\tconst char *cmd = "status";
\tif ( !Q_stricmp( cmd, "status" ) ) return;
\tif ( !Q_stricmp( cmd, "alert" ) ) return;
}
"""
MINI_REGISTER = 'CONSOLE_VERB("status", "-", "-")\nCONSOLE_VERB("alert", "red", "engineering|tactical")\n'

# A second claim of a verb already claimed, with no argument distinction: the later claim is dead.
SHADOWED_CONSOLE = """
void Svcmd_Ship_f( void )
{
\tconst char *cmd = "status";
\tif ( !Q_stricmp( cmd, "status" ) ) return;
\tif ( !Q_stricmp( cmd, "status" ) ) return;
}
"""

# The same verb claimed twice, distinguished by argument shape: both can be reached.
SHAPED_CONSOLE = """
void Svcmd_Ship_f( void )
{
\tconst char *cmd = "frob";
\tif ( !Q_stricmp( cmd, "frob" ) && a[0] && b[0] ) return;
\telse if ( !Q_stricmp( cmd, "frob" ) && sys >= 0 ) return;
}
"""
SHAPED_REGISTER = 'CONSOLE_VERB("frob", "<a> <b>|<system>", "-")\n'


def load():
    spec = importlib.util.spec_from_file_location("lwh_console_verbs", VERBS)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


class Verbs(unittest.TestCase):
    def setUp(self):
        self.v = load()
        self.tmp = tempfile.TemporaryDirectory()
        self.console = os.path.join(self.tmp.name, "g_ship.cpp")
        self.register = os.path.join(self.tmp.name, "console_api.def")
        self.write(self.console, MINI_CONSOLE)
        self.write(self.register, MINI_REGISTER)

    def tearDown(self):
        self.tmp.cleanup()

    def write(self, path, text):
        with open(path, "w") as f:
            f.write(text)

    def out(self):
        lines = []
        return lines, lambda s: lines.append(s)

    def test_derive_finds_every_verb(self):
        self.assertEqual(self.v.derive(self.console), ["alert", "status"])

    def test_enumerate_reads_the_rows(self):
        rows = self.v.enumerate_api(self.register)
        self.assertEqual([r[0] for r in rows], ["status", "alert"])

    def test_check_passes_when_they_agree(self):
        lines, out = self.out()
        self.assertEqual(self.v.check(self.console, self.register, out), 0)
        self.assertTrue(any("PASS" in l for l in lines))

    def test_fails_when_the_console_gains_a_verb(self):
        with open(self.console) as f:
            text = f.read()
        self.write(self.console, text.replace(
            '\tif ( !Q_stricmp( cmd, "alert" ) ) return;',
            '\tif ( !Q_stricmp( cmd, "frobnicate" ) ) return;\n'
            '\tif ( !Q_stricmp( cmd, "alert" ) ) return;'))
        lines, out = self.out()
        self.assertEqual(self.v.check(self.console, self.register, out), 1)
        self.assertTrue(any("frobnicate" in l and "FAIL" in l for l in lines))

    def test_fails_when_the_register_gains_a_row(self):
        with open(self.register, "a") as f:
            f.write('CONSOLE_VERB("warpnine", "-", "-")\n')
        lines, out = self.out()
        self.assertEqual(self.v.check(self.console, self.register, out), 1)
        self.assertTrue(any("warpnine" in l and "FAIL" in l for l in lines))

    def test_a_bad_station_is_reported(self):
        with open(self.register, "a") as f:
            f.write('CONSOLE_VERB("status", "-", "tenforward")\n')  # duplicate verb + bad station
        with self.assertRaises(ValueError):
            self.v.enumerate_api(self.register)

    # -- the ladder walk (Task B) -------------------------------------------------------------

    def test_the_walk_reports_a_shadowed_claim(self):
        self.write(self.console, SHADOWED_CONSOLE)
        findings = self.v.collisions(self.v.claims(self.console))
        self.assertEqual([(v, k) for v, k, _, _ in findings], [("status", "shadow")])
        _, _, earlier, later = findings[0]
        self.assertEqual(earlier.line + 1, later.line, "the later claim is the one shadowed")

    def test_the_walk_reports_argument_shape_collisions_as_reachable(self):
        self.write(self.console, SHAPED_CONSOLE)
        findings = self.v.collisions(self.v.claims(self.console))
        self.assertEqual([(v, k) for v, k, _, _ in findings], [("frob", "reachable")])

    def test_a_planted_shadow_fails_the_check_and_names_it(self):
        self.write(self.console, SHADOWED_CONSOLE)
        lines, out = self.out()
        self.assertEqual(self.v.check(self.console, self.register, out), 1)
        text = "\n".join(lines)
        self.assertIn("status", text)
        self.assertIn("can never run", text)

    def test_an_argument_shape_collision_warns_but_does_not_fail(self):
        self.write(self.console, SHAPED_CONSOLE)
        self.write(self.register, SHAPED_REGISTER)
        lines, out = self.out()
        self.assertEqual(self.v.check(self.console, self.register, out), 0)
        text = "\n".join(lines)
        self.assertIn("WARN", text)
        self.assertIn("frob", text)

    def test_the_real_tree_passes(self):
        lines, out = self.out()
        self.assertEqual(self.v.check(CONSOLE, REGISTER, out), 0, "\n".join(lines))

    def test_the_real_tree_has_no_shadowed_claim(self):
        findings = self.v.collisions(self.v.claims(CONSOLE))
        shadowed = [(v, l.line) for v, k, _, l in findings if k == "shadow"]
        self.assertEqual(shadowed, [], "a verb claim on the tree can never run")

    def test_the_real_tree_claims_recall_on_two_reachable_branches(self):
        findings = [f for f in self.v.collisions(self.v.claims(CONSOLE)) if f[0] == "recall"]
        self.assertEqual(len(findings), 1, "recall is claimed twice")
        _, kind, earlier, later = findings[0]
        self.assertEqual(kind, "reachable", "neither recall claim may shadow the other")
        self.assertEqual(earlier.shape, "takes no arguments")
        self.assertEqual(later.shape, "needs at least 2 argument(s)")


if __name__ == "__main__":
    unittest.main()
