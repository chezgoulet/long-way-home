"""Tests for tools/console/verbs.py: the ship's API freshness check (docs/evidence/the-computer-api.md).

The check must fail in both directions -- a console verb missing from the enumeration, and an
enumeration row the console does not answer -- and pass on the real tree. No engine is needed.

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
        with open(self.console, "w") as f:
            f.write(MINI_CONSOLE)
        with open(self.register, "w") as f:
            f.write(MINI_REGISTER)

    def tearDown(self):
        self.tmp.cleanup()

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
        with open(self.console, "w") as f:
            f.write(text.replace('\tif ( !Q_stricmp( cmd, "alert" ) ) return;',
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

    def test_the_real_tree_passes(self):
        lines, out = self.out()
        self.assertEqual(self.v.check(CONSOLE, REGISTER, out), 0, "\n".join(lines))


if __name__ == "__main__":
    unittest.main()
