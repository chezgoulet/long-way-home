"""Tests for tools/voice/render.py: the delivery direction reaches --exaggeration, an unmarked line
is refused, a cached line is skipped, and a cache inside the repository is refused.

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
RENDER = os.path.join(ROOT, "tools", "voice", "render.py")


def load_render():
    spec = importlib.util.spec_from_file_location("lwh_voice_render", RENDER)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


class DeliveryMapping(unittest.TestCase):
    def setUp(self):
        self.render = load_render()

    def test_matches_the_cpp_values(self):
        # These are DeliveryExaggeration in module/ship/ship_core.cpp (docs/evidence/voice-review.md).
        self.assertEqual(self.render.DELIVERY_EXAGGERATION,
                         {"order": 0.8, "report": 0.5, "confession": 0.35,
                          "condolence": 0.3, "flat": 0.2})

    def test_unmarked_is_refused(self):
        with self.assertRaises(self.render.JobError):
            self.render.exaggeration_for("unmarked")
        with self.assertRaises(self.render.JobError):
            self.render.exaggeration_for("nonsense")

    def test_command_carries_exaggeration(self):
        job = {"key": "abc", "voice": "tuvok", "text": "Make it so.",
               "delivery": "order", "exaggeration": 0.8}
        cmd = self.render.command_for("/tmp/cache", "/tmp/cache", job)
        self.assertIn("--exaggeration", cmd)
        self.assertEqual(cmd[cmd.index("--exaggeration") + 1], "0.8")
        self.assertEqual(cmd[cmd.index("--out") + 1], os.path.join("/tmp/cache", "abc.wav"))

    def test_a_disagreeing_number_is_refused(self):
        job = {"key": "abc", "voice": "tuvok", "text": "Make it so.",
               "delivery": "order", "exaggeration": 0.5}
        with self.assertRaises(self.render.JobError):
            self.render.command_for("/tmp/cache", "/tmp/cache", job)

    def test_inside_repo(self):
        self.assertTrue(self.render.inside_repo(ROOT, ROOT))
        self.assertTrue(self.render.inside_repo(os.path.join(ROOT, "tools"), ROOT))
        self.assertFalse(self.render.inside_repo("/tmp/cache", ROOT))
        # Inside the working tree but gitignored (the build home) is not repository data.
        self.assertFalse(self.render.inside_repo(os.path.join(ROOT, "build", "cache"), ROOT))


class DryRun(unittest.TestCase):
    def setUp(self):
        self.render = load_render()
        self.tmp = tempfile.TemporaryDirectory()
        self.cache = os.path.join(self.tmp.name, "cache")
        os.makedirs(self.cache)
        self.manifest = os.path.join(self.tmp.name, "render.jsonl")
        self.jobs = [
            {"key": "k-order", "voice": "tuvok", "text": "Make it so.",
             "delivery": "order", "exaggeration": 0.8},
            {"key": "k-report", "voice": "janeway", "text": "The plant holds.",
             "delivery": "report"},
            {"key": "k-unmarked", "voice": "kim", "text": "I cannot call it.",
             "delivery": "unmarked"},
        ]
        with open(self.manifest, "w", encoding="utf-8") as fh:
            for job in self.jobs:
                fh.write(json.dumps(job) + "\n")

    def tearDown(self):
        self.tmp.cleanup()

    def test_dry_run_skips_unmarked_and_carries_exaggeration(self):
        result = subprocess.run(
            [sys.executable, RENDER, "--cache", self.cache, "--manifest", self.manifest, "--dry-run"],
            capture_output=True, text=True)
        self.assertEqual(result.returncode, 1)  # one refused
        self.assertIn("--exaggeration 0.8", result.stdout)
        self.assertIn("--exaggeration 0.5", result.stdout)
        self.assertIn("refused", result.stdout)
        self.assertNotIn("k-unmarked.wav", result.stdout)

    def test_a_cached_line_is_a_no_op(self):
        open(os.path.join(self.cache, "k-order.wav"), "wb").close()
        result = subprocess.run(
            [sys.executable, RENDER, "--cache", self.cache, "--manifest", self.manifest, "--dry-run"],
            capture_output=True, text=True)
        self.assertIn("cached  :", result.stdout)                       # k-order: not rendered
        renders = [l for l in result.stdout.splitlines() if l.startswith("render  :")]
        self.assertEqual(len(renders), 1)                               # only k-report
        self.assertIn("k-report", renders[0])

    def test_cache_inside_repo_is_refused(self):
        result = subprocess.run(
            [sys.executable, RENDER, "--cache", os.path.join(ROOT, "tools"),
             "--manifest", self.manifest, "--dry-run"],
            capture_output=True, text=True)
        self.assertEqual(result.returncode, 2)
        self.assertIn("inside the repository", result.stderr)


if __name__ == "__main__":
    unittest.main()
