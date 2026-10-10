"""The speech-to-text worker's pure half, with no model and no audio (docs/evidence/voice-in.md).

`tools/speech/transcribe.py` imports sherpa-onnx only when it is about to load a model, so the
fields, the transcript file and the command surface are testable here without the model, without a
venv and without the network. What is *not* tested here -- that a known file transcribes to the
expected text -- needs the borrowed model and is the check script's job.
"""

import os
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "tools", "speech"))

import transcribe  # noqa: E402


class TestFields(unittest.TestCase):
    def test_sanitize_removes_separators(self):
        # The module splits fields on `|` and lines on newline: neither may survive a value.
        self.assertEqual(transcribe.sanitize("a|b\nc\r\nd"), "a/b c  d")
        self.assertEqual(transcribe.sanitize("  spaced  "), "spaced")

    def test_normalize_ignores_case_and_punctuation(self):
        self.assertEqual(transcribe.normalize("After early nightfall, the lamps."),
                         transcribe.normalize("AFTER EARLY NIGHTFALL THE LAMPS"))
        self.assertNotEqual(transcribe.normalize("one"), transcribe.normalize("two"))


class TestTranscriptFile(unittest.TestCase):
    def test_round_trip(self):
        with tempfile.TemporaryDirectory() as d:
            path = os.path.join(d, "transcript.txt")
            transcribe.append_transcript(path, "/audio/one.wav", "Hello there.")
            transcribe.append_transcript(path, "/audio/two.wav", "A|second\nline")
            done = transcribe.read_transcripts(path)
            self.assertEqual(done, {"/audio/one.wav", "/audio/two.wav"})
            with open(path, "r", encoding="utf-8") as fh:
                lines = [ln.rstrip("\n") for ln in fh]
            self.assertEqual(lines[0], "A|/audio/one.wav|Hello there.")
            # The separator and the newline the text carried did not survive into the record.
            self.assertEqual(lines[1], "A|/audio/two.wav|A/second line")

    def test_read_missing_file_is_empty(self):
        self.assertEqual(transcribe.read_transcripts("/no/such/transcript.txt"), set())


class TestModelDiscovery(unittest.TestCase):
    def test_prefers_int8_then_falls_back(self):
        with tempfile.TemporaryDirectory() as d:
            for name in ("tiny.en-encoder.onnx", "tiny.en-decoder.onnx"):
                open(os.path.join(d, name), "w").close()
            enc, dec, _ = transcribe.model_files(d)
            self.assertTrue(enc.endswith("tiny.en-encoder.onnx"))
            self.assertTrue(dec.endswith("tiny.en-decoder.onnx"))

    def test_missing_model_is_a_named_error(self):
        with tempfile.TemporaryDirectory() as d:
            with self.assertRaises(SystemExit) as ctx:
                transcribe.require_model(d)
            self.assertIn("no STT model", str(ctx.exception))


class TestCommands(unittest.TestCase):
    def test_drain_with_no_manifest_rows_transcribes_nothing(self):
        with tempfile.TemporaryDirectory() as d:
            manifest = os.path.join(d, "voice.jsonl")
            open(manifest, "w").close()
            out = os.path.join(d, "transcript.txt")
            rc = transcribe.main(["drain", "--manifest", manifest, "--transcript", out])
            self.assertEqual(rc, 0)
            self.assertEqual(transcribe.read_transcripts(out), set())

    def test_drain_skips_a_file_already_transcribed(self):
        with tempfile.TemporaryDirectory() as d:
            manifest = os.path.join(d, "voice.jsonl")
            with open(manifest, "w", encoding="utf-8") as fh:
                fh.write('{"audio": "/audio/one.wav"}\n')
            out = os.path.join(d, "transcript.txt")
            transcribe.append_transcript(out, "/audio/one.wav", "Already here.")
            rc = transcribe.main(["drain", "--manifest", manifest, "--transcript", out])
            self.assertEqual(rc, 0)
            with open(out, "r", encoding="utf-8") as fh:
                self.assertEqual(len(fh.readlines()), 1)


if __name__ == "__main__":
    unittest.main()
