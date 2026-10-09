"""Tests for the documentation freshness check (tools/docs/check_paths.py, check_claims.py).

No repository content is needed: each test builds a tiny corpus in a temporary directory and asserts
that the check passes when it should and fails when it should.  The point of the tool is that it *can*
fail for the reason we care about, so the negative cases carry as much weight as the positive ones.

    python3 -m unittest discover -s tests/tools
"""

import json
import os
import pathlib
import sys
import tempfile
import unittest

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools", "docs"))
import check_claims  # noqa: E402
import check_paths  # noqa: E402


def write(root, rel, text):
    p = pathlib.Path(root) / rel
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(text, encoding="utf-8")
    return p


class PathCheckTests(unittest.TestCase):
    def test_find_refs_extracts_docs_tokens(self):
        toks = [t for _, t in check_paths.find_refs("see docs/a.md, docs/evidence/deck*-d.md and `docs/x`.")]
        self.assertEqual(toks, ["docs/a.md", "docs/evidence/deck*-d.md", "docs/x"])

    def test_pass_when_every_reference_resolves(self):
        with tempfile.TemporaryDirectory() as d:
            write(d, "docs/evidence/real.md", "x")
            write(d, "docs/evidence/deck01-redress.md", "x")
            write(d, "docs/ok.md", "see docs/evidence/real.md and docs/evidence/deck*-redress.md")
            self.assertEqual(check_paths.main(["--root", d, "--allow", "no-allow.txt", "--min-refs", "0"]), 0)

    def test_fail_on_a_dangling_reference(self):
        with tempfile.TemporaryDirectory() as d:
            write(d, "docs/evidence/real.md", "x")
            write(d, "docs/bad.md", "see docs/evidence/missing.md")
            self.assertEqual(check_paths.main(["--root", d, "--allow", "no-allow.txt", "--min-refs", "0"]), 1)

    def test_allowlist_exempts_a_recorded_dangling_path(self):
        with tempfile.TemporaryDirectory() as d:
            write(d, "docs/bad.md", "the wrong path docs/gap-the-log.md was corrected")
            write(d, "allow.txt", "# comment\ndocs/gap-the-log.md\n")
            self.assertEqual(check_paths.main(["--root", d, "--allow", "allow.txt", "--min-refs", "0"]), 0)

    def test_min_refs_guard_fails_a_vacuous_scan(self):
        with tempfile.TemporaryDirectory() as d:
            write(d, "docs/empty.md", "no references here")
            self.assertEqual(check_paths.main(["--root", d, "--allow", "no-allow.txt", "--min-refs", "5"]), 1)

    def test_find_refs_ignores_fenced_blocks(self):
        text = "prose docs/a.md\n\n```\nfenced docs/b.md\n```\n"
        self.assertEqual([t for _, t in check_paths.find_refs(text)], ["docs/a.md"])

    def test_a_missing_path_inside_a_fence_is_not_a_citation(self):
        with tempfile.TemporaryDirectory() as d:
            write(d, "docs/demo.md", "transcript:\n\n```\nsee docs/evidence/planted.md\n```\n")
            self.assertEqual(check_paths.main(["--root", d, "--allow", "no-allow.txt", "--min-refs", "0"]), 0)

    def test_line_numbers_survive_a_fence(self):
        text = "line one\n\n```\nignored\n```\nprose docs/a.md\n"
        self.assertEqual([ln for ln, _ in check_paths.find_refs(text)], [6])


CLAIM = {
    "id": "core-deck",
    "owner": "docs/owner.md",
    "value": "10",
    "owner_pattern": "core is on deck (\\d+)",
    "sources": [{"file": "docs/other.md", "pattern": "core \\| (\\d+)"}],
}


class ClaimCheckTests(unittest.TestCase):
    def corpus(self, d, owner_deck="10", other_deck="10"):
        write(d, "docs/owner.md", f"the core is on deck {owner_deck}\n")
        write(d, "docs/other.md", f"the core | {other_deck}\n")
        write(d, "claims.json", json.dumps({"claims": [CLAIM]}))

    def test_pass_when_sources_agree_with_owner(self):
        with tempfile.TemporaryDirectory() as d:
            self.corpus(d)
            self.assertEqual(check_claims.main(["--root", d, "--claims", "claims.json"]), 0)

    def test_fail_when_a_source_contradicts_the_owner(self):
        with tempfile.TemporaryDirectory() as d:
            self.corpus(d, other_deck="9")
            self.assertEqual(check_claims.main(["--root", d, "--claims", "claims.json"]), 1)

    def test_fail_when_the_owner_itself_is_wrong(self):
        with tempfile.TemporaryDirectory() as d:
            self.corpus(d, owner_deck="9")
            self.assertEqual(check_claims.main(["--root", d, "--claims", "claims.json"]), 1)

    def test_fail_when_a_pattern_no_longer_matches(self):
        with tempfile.TemporaryDirectory() as d:
            self.corpus(d)
            write(d, "docs/other.md", "the core moved\n")
            self.assertEqual(check_claims.main(["--root", d, "--claims", "claims.json"]), 1)

    def test_fail_when_no_claims_are_registered(self):
        with tempfile.TemporaryDirectory() as d:
            write(d, "claims.json", json.dumps({"claims": []}))
            self.assertEqual(check_claims.main(["--root", d, "--claims", "claims.json"]), 1)


if __name__ == "__main__":
    unittest.main()
