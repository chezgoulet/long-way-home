"""Tests for the scenario validator's negative-test harness. No game data and no compiler needed.

    python3 -m unittest discover -s tests/tools
"""

import os
import subprocess
import sys
import tempfile
import unittest

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
NEGATIVE = os.path.join(ROOT, "tools", "validator", "tests", "negative_tests.py")

# Geometry but no navigation entities. The validator's clean fixture cannot be built around a space
# like this (it would fail E003 as "declared inhabited but has no navigation"), so the harness must
# name it as a fixture problem and stop rather than letting it surface from inside a seeded case.
NAVLESS_MAP = ('{\n"classname" "worldspawn"\n"message" "no nav here"\n}\n'
               '{\n"classname" "light"\n"origin" "0 0 0"\n}\n')


class CleanFixture(unittest.TestCase):
    def test_navless_source_map_is_reported_as_a_fixture_problem(self):
        with tempfile.TemporaryDirectory() as tmp:
            src = os.path.join(tmp, "hm_navless.map")
            with open(src, "w") as fh:
                fh.write(NAVLESS_MAP)
            r = subprocess.run([sys.executable, NEGATIVE, "--source-map", src],
                               capture_output=True, text=True)
            out = r.stdout + r.stderr
            self.assertEqual(r.returncode, 2, out)                     # a fixture failure, not a test failure
            self.assertIn("clean fixture is not clean", out)
            self.assertNotIn("PASS  E001", out)                        # the seeded cases did not run
            self.assertNotIn("ALL PASS", out)


if __name__ == "__main__":
    unittest.main()
