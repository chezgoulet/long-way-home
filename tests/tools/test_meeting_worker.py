"""Tests for tools/meetings/worker.py: the pre-flight validator, the ledger the module reads, and the
field sanitising. No model is called here -- the network paths are the engine check's business.

    python3 -m unittest discover -s tests/tools
"""

import importlib.util
import os
import subprocess
import sys
import tempfile
import unittest

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
WORKER = os.path.join(ROOT, "tools", "meetings", "worker.py")


def load_worker():
    spec = importlib.util.spec_from_file_location("lwh_meeting_worker", WORKER)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


class Validator(unittest.TestCase):
    def setUp(self):
        self.w = load_worker()

    def branch(self, bid, **kw):
        b = {"id": bid, "intent": "a way to ask", "lines": [{"speaker": "command", "text": "Aye.", "delivery": "order"}]}
        b.update(kw)
        return b

    def test_a_good_script_passes(self):
        script = {"branches": [self.branch("b1", outcome=1), self.branch("b2", outcome=4)]}
        self.assertEqual(self.w.validate_script(script, 4), [])

    def test_a_dead_end_is_named(self):
        script = {"branches": [self.branch("b2")]}  # lines, but no outcome and no continuation
        faults = self.w.validate_script(script, 4)
        self.assertTrue(any(f.startswith("dead-end b2") for f in faults), faults)

    def test_a_branch_that_does_not_terminate_in_an_outcome_is_named(self):
        script = {"branches": [self.branch("b1", outcome=9)]}
        faults = self.w.validate_script(script, 4)
        self.assertTrue(any("b1 does not terminate in an enumerated outcome (outcome 9, options 4)" in f for f in faults), faults)

    def test_a_goto_chain_that_terminates_passes(self):
        script = {"branches": [self.branch("b1", goto="b2"), self.branch("b2", outcome=2)]}
        self.assertEqual(self.w.validate_script(script, 4), [])

    def test_a_goto_to_a_missing_branch_is_a_dead_end(self):
        script = {"branches": [self.branch("b1", goto="nope")]}
        faults = self.w.validate_script(script, 4)
        self.assertTrue(any(f.startswith("dead-end b1") for f in faults), faults)

    def test_a_contradiction_is_named(self):
        script = {"branches": [self.branch("b1", outcome=1, lines=[
            {"speaker": "command", "text": "Tuvok will take the watch.", "delivery": "order"}])]}
        faults = self.w.validate_script(script, 4, forbidden=["Tuvok"])
        self.assertTrue(any("contradicts the record" in f for f in faults), faults)


class Ledger(unittest.TestCase):
    def setUp(self):
        self.w = load_worker()
        self.tmp = tempfile.TemporaryDirectory()
        self.path = os.path.join(self.tmp.name, "novel.txt")

    def tearDown(self):
        self.tmp.cleanup()

    def test_round_trip(self):
        calls = {"gen": 3, "novel": 1}
        verdicts = [["aabbccdd", 1, 2, "matched The chief's plan at 0.62 (threshold 0.55)"]]
        self.w.write_ledger(self.path, calls, verdicts)
        back_calls, back_verdicts = self.w.read_ledger(self.path)
        self.assertEqual(back_calls, calls)
        # The fields come back as strings, which is what the module parses (atoi / strtoul).
        self.assertEqual(back_verdicts,
                         [["aabbccdd", "1", "2", "matched The chief's plan at 0.62 (threshold 0.55)"]])

    def test_a_missing_ledger_is_zero(self):
        calls, verdicts = self.w.read_ledger(os.path.join(self.tmp.name, "absent.txt"))
        self.assertEqual(calls, {"gen": 0, "novel": 0})
        self.assertEqual(verdicts, [])


class Sanitize(unittest.TestCase):
    def setUp(self):
        self.w = load_worker()

    def test_fields_cannot_break_the_module_format(self):
        # The module splits lines on '|'; neither it nor a newline may appear in a value.
        self.assertEqual(self.w.sanitize("a|b"), "a/b")
        self.assertEqual(self.w.sanitize("a\nb"), "a b")
        self.assertEqual(self.w.sanitize("  x  "), "x")


class Cli(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()

    def tearDown(self):
        self.tmp.cleanup()

    def write(self, name, body):
        path = os.path.join(self.tmp.name, name)
        with open(path, "w", encoding="utf-8") as fh:
            fh.write(body)
        return path

    def run_validate(self, path):
        return subprocess.run([sys.executable, WORKER, "validate", "--script", path],
                              capture_output=True, text=True)

    def test_good_passes(self):
        p = self.write("good.json",
                       '{"optionCount":2,"branches":[{"id":"b1","outcome":1,"lines":[{"speaker":"command","text":"Aye.","delivery":"order"}]}]}')
        r = self.run_validate(p)
        self.assertEqual(r.returncode, 0)

    def test_dead_end_is_refused_by_name(self):
        p = self.write("dead.json",
                       '{"optionCount":2,"branches":[{"id":"b2","lines":[{"speaker":"command","text":"Hmm.","delivery":"flat"}]}]}')
        r = self.run_validate(p)
        self.assertEqual(r.returncode, 1)
        self.assertIn("REFUSED dead-end b2", r.stdout)


if __name__ == "__main__":
    unittest.main()
