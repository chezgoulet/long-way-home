"""Tests for the harness case-number check (tools/harness/check_cases.py).

No repository content is needed: each test builds a tiny source file and asserts the check passes when
it should and fails when it should.  The point of the tool is that it *can* fail for the reason we care
about, so the duplicate case carries as much weight as the clean one.

    python3 -m unittest discover -s tests/tools
"""

import os
import sys
import tempfile
import unittest

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools", "harness"))
import check_cases  # noqa: E402


class HarnessCaseTests(unittest.TestCase):
    def corpus(self, text):
        f = tempfile.NamedTemporaryFile("w", suffix=".cpp", delete=False)
        f.write(text)
        f.close()
        self.addCleanup(os.unlink, f.name)
        return f.name

    def test_distinct_numbers_pass(self):
        src = self.corpus("if ( g_shipTest->integer == 1 ) {}\nif ( g_shipTest->integer == 2 ) {}\n")
        self.assertEqual(check_cases.main(["--src", src]), 0)

    def test_a_duplicate_fails(self):
        src = self.corpus("if ( g_shipTest->integer == 60 ) {}\nif ( g_shipTest->integer == 60 ) {}\n")
        self.assertEqual(check_cases.main(["--src", src]), 1)

    def test_an_empty_scan_fails_rather_than_passing_vacuously(self):
        src = self.corpus("int main() { return 0; }\n")
        self.assertEqual(check_cases.main(["--src", src]), 1)

    def test_the_real_harness_has_no_duplicate(self):
        self.assertEqual(check_cases.main([]), 0)


if __name__ == "__main__":
    unittest.main()
