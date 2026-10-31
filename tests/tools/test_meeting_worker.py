"""Tests for tools/meetings/worker.py: the pre-flight validator, the ledger the module reads, and the
field sanitising. No model is called here -- the network paths are the engine check's business.

    python3 -m unittest discover -s tests/tools
"""

import importlib.util
import json
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


class Prompt(unittest.TestCase):
    """The generator's input is the per-participant brief (Stage D), not a roster summary."""

    def setUp(self):
        self.w = load_worker()
        self.brief = {
            "kind": 2, "kindName": "allocation", "decision": "where the power goes",
            "trigger": "the plant is short",
            "agenda": [{"source": "the call", "crew": "", "text": "the plant is short"},
                       {"source": "a mark", "crew": "B'Elanna Torres", "text": "a mark she carries"}],
            "options": [{"index": 1, "label": "The chief's plan", "cost": "adopted",
                         "intent": "take the chief's recommendation"}],
            "skeleton": [{"index": 1, "lines": [{"speaker": "engineering", "delivery": "report",
                                                  "text": "The plan holds."}]}],
            "present": [
                {"name": "B'Elanna Torres", "post": "engineering",
                 "allegiance": {"to": "the Maquis", "strength": 0.85, "authored": 1},
                 "likes": [{"thing": "repair", "strength": 0.85}],
                 "dislikes": [{"thing": "by-the-book officers", "strength": 0.6}],
                 "wants": "wants \"repair\" kept", "willNotSay": "will not say what she thinks",
                 "stances": [{"option": 1, "stance": -1,
                              "reason": "the Maquis (+0.85) reads it as Starfleet procedure"}]},
                {"name": "Tuvok", "post": "security",
                 "allegiance": {"to": "Starfleet", "strength": 0.90, "authored": 1},
                 "likes": [{"thing": "kal-toh", "strength": 0.9}],
                 "wants": "wants \"kal-toh\" kept", "willNotSay": "will not say what he thinks",
                 "stances": [{"option": 1, "stance": 1,
                              "reason": "Starfleet (+0.90) reads it as Starfleet procedure"}]},
            ],
        }

    def test_the_prompt_carries_the_agenda_and_each_persons_brief(self):
        text = self.w.build_prompt(self.brief)
        self.assertIn("AGENDA", text)
        self.assertIn("a mark she carries", text)
        self.assertIn("B'Elanna Torres", text)
        self.assertIn('wants: wants "repair" kept', text)
        self.assertIn("will not say", text)
        self.assertIn("option 1: opposes", text)
        self.assertIn("the Maquis (+0.85) reads it as Starfleet procedure", text)
        self.assertIn("option 1: supports", text)


class Generator(unittest.TestCase):
    """The async generator is deterministic for a given brief, and soft when the model is absent."""

    def setUp(self):
        self.w = load_worker()
        self.tmp = tempfile.TemporaryDirectory()
        self.brief = {
            "kind": 2, "kindName": "allocation", "stateDigest": 12345,
            "decision": "where the power goes", "trigger": "the plant is short",
            "agenda": [{"source": "the call", "crew": "", "text": "the plant is short"}],
            "options": [
                {"index": 1, "label": "The chief's plan", "cost": "adopted", "intent": "take the chief's plan"},
                {"index": 2, "label": "Refuse the chief", "cost": "the argument stands", "intent": "refuse the chief"},
            ],
            "skeleton": [
                {"index": 1, "lines": [{"speaker": "engineering", "delivery": "report", "text": "The plan holds."}]},
                {"index": 2, "lines": [{"speaker": "engineering", "delivery": "report", "text": "Then it is on the record."}]},
            ],
            "present": [{"name": "B'Elanna Torres", "post": "engineering"}],
        }
        self.manifest = self.write("generate.jsonl", json.dumps(self.brief) + "\n")

    def tearDown(self):
        self.tmp.cleanup()

    def write(self, name, body):
        path = os.path.join(self.tmp.name, name)
        with open(path, "w", encoding="utf-8") as fh:
            fh.write(body)
        return path

    def args(self, script, ledger):
        return type("Args", (), {
            "manifest": self.manifest, "script": script, "ledger": ledger,
            "host": "http://127.0.0.1:1", "model": "test", "max_attempts": 1,
        })()

    def test_the_same_brief_generates_the_same_script(self):
        answer = json.dumps({"branches": [
            {"id": "b1", "outcome": 1, "lines": [{"speaker": "engineering", "text": "Adopted.", "delivery": "order"}]},
            {"id": "b2", "outcome": 2, "lines": [{"speaker": "engineering", "text": "Refused.", "delivery": "report"}]},
        ]})
        self.w.chat = lambda *a, **k: answer
        p1 = os.path.join(self.tmp.name, "a.txt")
        p2 = os.path.join(self.tmp.name, "b.txt")
        self.assertEqual(self.w.cmd_generate(self.args(p1, os.path.join(self.tmp.name, "a.ledger"))), 0)
        self.assertEqual(self.w.cmd_generate(self.args(p2, os.path.join(self.tmp.name, "b.ledger"))), 0)
        with open(p1, "rb") as fh1, open(p2, "rb") as fh2:
            self.assertEqual(fh1.read(), fh2.read())
        with open(p1, encoding="utf-8") as fh:
            text = fh.read()
        self.assertIn("D|2|12345|where the power goes", text)
        self.assertIn("L|2|1|engineering|order|Adopted.", text)

    def test_with_no_model_the_script_is_empty_and_the_skeleton_is_the_floor(self):
        def absent(*a, **k):
            raise self.w.urllib.error.URLError("no model here")
        self.w.chat = absent
        path = os.path.join(self.tmp.name, "none.txt")
        self.assertEqual(self.w.cmd_generate(self.args(path, os.path.join(self.tmp.name, "n.ledger"))), 0)
        with open(path, encoding="utf-8") as fh:
            self.assertEqual(fh.read(), "")
        self.assertFalse(self.w.validate_script({}, 2) == [])


if __name__ == "__main__":
    unittest.main()
