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
import check_register  # noqa: E402


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

    def test_emphasis_around_a_citation_is_punctuation(self):
        # The real case (docs/hook-register.md): *docs/a.md, part three; docs/start-states.md.*
        with tempfile.TemporaryDirectory() as d:
            write(d, "docs/start-states.md", "x")
            write(d, "docs/ok.md", "*docs/start-states.md.* the entry point\n")
            self.assertEqual(check_paths.main(["--root", d, "--allow", "no-allow.txt", "--min-refs", "0"]), 0)

    def test_emphasis_does_not_rescue_a_dangling_path(self):
        with tempfile.TemporaryDirectory() as d:
            write(d, "docs/ok.md", "*docs/nowhere.md.* the missing one\n")
            self.assertEqual(check_paths.main(["--root", d, "--allow", "no-allow.txt", "--min-refs", "0"]), 1)

    def test_a_dangling_glob_still_fails(self):
        with tempfile.TemporaryDirectory() as d:
            write(d, "docs/ok.md", "see docs/nowhere/*.md\n")
            self.assertEqual(check_paths.main(["--root", d, "--allow", "no-allow.txt", "--min-refs", "0"]), 1)


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


CODED = {
    "id": "save-version",
    "owner": "code.h",
    "value": "7",
    "owner_pattern": "const int SAVE_VERSION = (\\d+);",
    "current_form": "SAVE_VERSION\\s*=\\s*(\\d+)",
    "exempt_files": [{"file": "docs/quotes-it.md", "reason": "it records another file's line"}],
}


class CodedClaimTests(unittest.TestCase):
    """The current-tense convention: the assertion form is read, prose is history, a quote is exempt."""

    def corpus(self, d, value="7"):
        write(d, "code.h", f"const int SAVE_VERSION = {value};\n")
        write(d, "claims.json", json.dumps({"claims": [CODED]}))

    def test_pass_on_a_matching_assertion(self):
        with tempfile.TemporaryDirectory() as d:
            self.corpus(d)
            write(d, "docs/a.md", "the constant is `SAVE_VERSION = 7` today\n")
            self.assertEqual(check_claims.main(["--root", d, "--claims", "claims.json"]), 0)

    def test_fail_on_a_current_tense_disagreement(self):
        with tempfile.TemporaryDirectory() as d:
            self.corpus(d)
            write(d, "docs/a.md", "the constant is `SAVE_VERSION = 6`\n")
            self.assertEqual(check_claims.main(["--root", d, "--claims", "claims.json"]), 1)

    def test_pass_on_a_historical_statement_of_another_number(self):
        # "save version 6" is prose history, not a current-tense assertion: not read.
        with tempfile.TemporaryDirectory() as d:
            self.corpus(d)
            write(d, "docs/ledger.md", "the pass landed at save version 6\n")
            self.assertEqual(check_claims.main(["--root", d, "--claims", "claims.json"]), 0)

    def test_evidence_is_a_dated_record_and_is_not_read(self):
        with tempfile.TemporaryDirectory() as d:
            self.corpus(d)
            write(d, "docs/evidence/old.md", "the constant was SAVE_VERSION = 3\n")
            self.assertEqual(check_claims.main(["--root", d, "--claims", "claims.json"]), 0)

    def test_a_named_exempt_file_may_quote_the_assertion(self):
        with tempfile.TemporaryDirectory() as d:
            self.corpus(d)
            write(d, "docs/quotes-it.md", "| the handoff | SAVE_VERSION = 6 |\n")
            self.assertEqual(check_claims.main(["--root", d, "--claims", "claims.json"]), 0)

    def test_fail_when_the_owner_stops_defining_the_constant(self):
        with tempfile.TemporaryDirectory() as d:
            self.corpus(d)
            write(d, "code.h", "// the constant moved\n")
            self.assertEqual(check_claims.main(["--root", d, "--claims", "claims.json"]), 1)


class RegisterCheckTests(unittest.TestCase):
    """A register row whose marker has become false must fail, in either direction."""

    def test_pass_when_a_closed_row_marker_resolves(self):
        with tempfile.TemporaryDirectory() as d:
            write(d, "scripts/prove.sh", "#!/bin/sh\n")
            write(d, "docs/reg.md",
                  "| C1 | ... <!-- register-row id=C1 state=closed "
                  "marker=check:scripts/prove.sh --> |\n")
            self.assertEqual(check_register.main(["--root", d, "--register", "docs/reg.md"]), 0)

    def test_fail_when_an_open_row_marker_resolves(self):
        with tempfile.TemporaryDirectory() as d:
            write(d, "scripts/prove.sh", "#!/bin/sh\n")
            write(d, "docs/reg.md",
                  "| C1 | still unbuilt <!-- register-row id=C1 state=open "
                  "marker=check:scripts/prove.sh --> |\n")
            self.assertEqual(check_register.main(["--root", d, "--register", "docs/reg.md"]), 1)

    def test_fail_when_a_closed_row_marker_is_absent(self):
        with tempfile.TemporaryDirectory() as d:
            write(d, "docs/reg.md",
                  "| C1 | closed <!-- register-row id=C1 state=closed "
                  "marker=check:scripts/gone.sh --> |\n")
            self.assertEqual(check_register.main(["--root", d, "--register", "docs/reg.md"]), 1)

    def test_pass_when_an_open_row_marker_is_absent(self):
        with tempfile.TemporaryDirectory() as d:
            write(d, "docs/reg.md",
                  "| O5 | unbuilt <!-- register-row id=O5 state=open "
                  "marker=check:scripts/not-yet.sh --> |\n")
            self.assertEqual(check_register.main(["--root", d, "--register", "docs/reg.md"]), 0)

    def test_fail_on_a_malformed_marker(self):
        with tempfile.TemporaryDirectory() as d:
            write(d, "docs/reg.md",
                  "| C1 | ... <!-- register-row id=C1 state=closed marker=nonsense:x --> |\n")
            self.assertEqual(check_register.main(["--root", d, "--register", "docs/reg.md"]), 1)

    def test_fail_on_a_register_with_no_markers(self):
        with tempfile.TemporaryDirectory() as d:
            write(d, "docs/reg.md", "| C1 | a row with no marker |\n")
            self.assertEqual(check_register.main(["--root", d, "--register", "docs/reg.md"]), 1)

    def test_symbol_and_text_markers_resolve_in_a_file(self):
        with tempfile.TemporaryDirectory() as d:
            write(d, "docs/reg.md",
                  "<!-- register-row id=A state=closed marker=symbol:module/x.cpp:Bond -->\n"
                  "<!-- register-row id=B state=closed marker=text:docs/g.md:RULED -->\n")
            write(d, "module/x.cpp", "float Bond(const Ship &s, int a, int b);\n")
            write(d, "docs/g.md", "C2 is RULED 2026-10-08.\n")
            self.assertEqual(check_register.main(["--root", d, "--register", "docs/reg.md"]), 0)


if __name__ == "__main__":
    unittest.main()
