#!/usr/bin/env python3
"""The speech-to-text worker: an audio file becomes the string the game reads (M6, voice in).

The engine has no audio-capture path (`docs/evidence/voice-in.md`): there is no
`SDL_OpenAudioDevice` capture anywhere in the upstream sources, so this client cannot read a
microphone. The boundary this worker sits behind is therefore **an audio file** -- the microphone,
priced and not built, would deliver exactly such a file. The worker transcribes the file to a
string and writes it where the module reads it; the module then submits that string through the
identical path a typed one takes (`SubmitRoomInput`, the free-text pill's own seam).

It is the meeting worker's shape, run the other way (`tools/meetings/worker.py`): there the module
writes a manifest and the worker drains it; here the worker **produces** the text the game reads.

It is an **instrument, not a dependency**. The game runs with it absent, exactly as it runs with the
generator absent: no transcript means the input is pending, and the room plays on. Two rules the
worker keeps:

  * one model at a time. The STT model is loaded for the life of this process and freed when the
    process exits -- and the module only queues a request in voice mode, so the STT model is never
    resident alongside the embedding matcher, the generator or the synthesizer.
  * no model, no audio and no game data live in the repository. The model is borrowed from another
    tree and read in place; every transcript is player-local, under `ship/meetings`, beside the save.

    transcribe.py transcribe --audio FILE [--out FILE] [--model-dir DIR] [--expected TEXT]
    transcribe.py drain      --manifest FILE --transcript FILE [--model-dir DIR]
    transcribe.py residency  [--model-dir DIR]

`sherpa-onnx` is installed into a throwaway venv under the gitignored `voice-scratch/` (the
procedure `docs/evidence/voice-spike.md` records): `uv venv voice-scratch/venv` then
`uv pip install sherpa-onnx==1.13.8`. The default model dir is the whisper tiny.en the spike
borrowed; pass `--model-dir` to point at any sherpa-onnx whisper export.
"""

import argparse
import array
import os
import resource
import sys
import time
import wave

# The model the spike borrowed, complete on this host with test_wavs/ beside it. Never copied: the
# worker reads it in place, and `--model-dir` overrides for any other host.
DEFAULT_MODEL_DIR = "/home/c/hermes-vox/models-store/work/tw/sherpa-onnx-whisper-tiny.en"

# The transcript file is line-oriented, `A|audio|text`, the same shape the module's SplitFields
# reads. Neither the audio field nor the text may carry a `|` or a newline.
TRANSCRIPT_KIND = "A"


def sanitize(text):
    """The module splits fields on `|` and lines on newline: neither may appear in a value."""
    return str(text).replace("|", "/").replace("\n", " ").replace("\r", " ").strip()


def normalize(text):
    """For comparing a transcript to an expected line: case, punctuation and spacing do not count."""
    out = []
    for ch in str(text).upper():
        out.append(ch if ch.isalnum() else " ")
    return " ".join("".join(out).split())


def model_files(model_dir):
    enc = os.path.join(model_dir, "tiny.en-encoder.int8.onnx")
    dec = os.path.join(model_dir, "tiny.en-decoder.int8.onnx")
    tok = os.path.join(model_dir, "tiny.en-tokens.txt")
    if not os.path.exists(enc):
        enc = os.path.join(model_dir, "tiny.en-encoder.onnx")
    if not os.path.exists(dec):
        dec = os.path.join(model_dir, "tiny.en-decoder.onnx")
    return enc, dec, tok


def require_model(model_dir):
    enc, dec, tok = model_files(model_dir)
    for path in (enc, dec, tok):
        if not os.path.exists(path):
            raise SystemExit("no STT model at %s (missing %s); pass --model-dir" % (model_dir, path))
    return enc, dec, tok


def read_wave(path):
    """A 16-bit PCM mono wav as (sample_rate, [floats]). Reads and clears the file; copies nothing."""
    with wave.open(path, "rb") as w:
        if w.getnchannels() != 1:
            raise SystemExit("%s: %d channels; a capture file is mono" % (path, w.getnchannels()))
        if w.getsampwidth() != 2:
            raise SystemExit("%s: %d-byte samples; a capture file is 16-bit PCM" % (path, w.getsampwidth()))
        rate = w.getframerate()
        raw = w.readframes(w.getnframes())
    samples = array.array("h")
    samples.frombytes(raw)
    return rate, [s / 32768.0 for s in samples]


def load_recognizer(model_dir, threads):
    """The one model this process loads. Imported here so the rest of the tool needs no sherpa."""
    import sherpa_onnx
    enc, dec, tok = require_model(model_dir)
    return sherpa_onnx.OfflineRecognizer.from_whisper(
        encoder=enc, decoder=dec, tokens=tok, num_threads=threads,
        decoding_method="greedy_search", language="en", task="transcribe")


def resident_line(model_dir):
    return ("resident: sherpa-onnx whisper tiny.en (STT) from %s -- the one model this process "
            "loads; it is freed when this process exits" % model_dir)


def peak_rss_mb():
    return resource.getrusage(resource.RUSAGE_SELF).ru_maxrss / 1024.0


def read_transcripts(path):
    """The audio files already transcribed in `path`: the out file is a cache, and a re-drain is a no-op."""
    done = set()
    if os.path.exists(path):
        with open(path, "r", encoding="utf-8") as fh:
            for line in fh:
                parts = line.rstrip("\n").split("|", 2)
                if len(parts) == 3 and parts[0] == TRANSCRIPT_KIND:
                    done.add(parts[1])
    return done


def append_transcript(path, audio, text):
    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
    with open(path, "a", encoding="utf-8") as fh:
        fh.write("%s|%s|%s\n" % (TRANSCRIPT_KIND, sanitize(audio), sanitize(text)))


def transcribe_one(rec, audio):
    rate, samples = read_wave(audio)
    stream = rec.create_stream()
    stream.accept_waveform(rate, samples)
    t0 = time.time()
    rec.decode_stream(stream)
    return stream.result.text.strip(), len(samples) / float(rate), time.time() - t0


def cmd_transcribe(args):
    print(resident_line(args.model_dir))
    t0 = time.time()
    rec = load_recognizer(args.model_dir, args.threads)
    load = time.time() - t0
    text, seconds, decode = transcribe_one(rec, args.audio)
    print("audio   : %s (%.2fs)" % (args.audio, seconds))
    print("load    : %.2fs" % load)
    print("decode  : %.2fs (%.2fx realtime)" % (decode, decode / seconds if seconds else 0.0))
    print("text    : %s" % text)
    if args.expected:
        ok = normalize(text) == normalize(args.expected)
        print("expected: %s" % args.expected)
        print("normalised match: %s" % ("yes" if ok else "no"))
    print("peak RSS: %.0f MB" % peak_rss_mb())
    if args.out:
        append_transcript(args.out, args.audio, text)
        print("wrote   : %s (%s|%s|...)" % (args.out, TRANSCRIPT_KIND, sanitize(args.audio)))
    return 0


def cmd_drain(args):
    print(resident_line(args.model_dir))
    requests = []
    with open(args.manifest, "r", encoding="utf-8") as fh:
        for line in fh:
            line = line.strip()
            if not line:
                continue
            import json
            requests.append(json.loads(line))
    already = read_transcripts(args.transcript)
    todo = [r for r in requests if r.get("audio") and r["audio"] not in already]
    print("read    : %d request(s), %d already transcribed" % (len(requests), len(requests) - len(todo)))
    if not todo:
        print("== 0 transcribed, %d already present" % len(already))
        return 0
    rec = load_recognizer(args.model_dir, args.threads)
    n = 0
    for r in todo:
        text, seconds, decode = transcribe_one(rec, r["audio"])
        append_transcript(args.transcript, r["audio"], text)
        print("%s|%s|%s" % (TRANSCRIPT_KIND, sanitize(r["audio"]), sanitize(text)))
        print("          %.2fs audio, %.2fs decode" % (seconds, decode))
        n += 1
    print("== %d transcribed, %d already present" % (n, len(already)))
    print("peak RSS: %.0f MB" % peak_rss_mb())
    return 0


def cmd_residency(args):
    enc, dec, tok = model_files(args.model_dir)
    print("the memory plan (docs/staff-meetings.md), and which model this worker is:")
    print("  STT (this worker, voice mode only), one model at a time:")
    print("    %s" % resident_line(args.model_dir))
    print("    model present: %s" % ("yes" if os.path.exists(enc) and os.path.exists(dec) else "no"))
    print("  the embedding matcher (the novelty classifier) is ollama's, resident only during classify")
    print("  the generator is ollama's, resident only during async generation")
    print("  the synthesizer is the render worker's, batched during generation")
    print("this process loads exactly one of the four, and never at the same time as another.")
    return 0


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)

    t = sub.add_parser("transcribe")
    t.add_argument("--audio", required=True, help="the capture file the microphone would have written")
    t.add_argument("--out", default="", help="append `A|audio|text` here, where the module reads it")
    t.add_argument("--model-dir", default=DEFAULT_MODEL_DIR)
    t.add_argument("--threads", type=int, default=2)
    t.add_argument("--expected", default="", help="the line the audio is known to say, to check against")
    t.set_defaults(func=cmd_transcribe)

    d = sub.add_parser("drain")
    d.add_argument("--manifest", required=True, help="the module's ship/meetings/voice.jsonl")
    d.add_argument("--transcript", required=True, help="append the transcripts here")
    d.add_argument("--model-dir", default=DEFAULT_MODEL_DIR)
    d.add_argument("--threads", type=int, default=2)
    d.set_defaults(func=cmd_drain)

    r = sub.add_parser("residency")
    r.add_argument("--model-dir", default=DEFAULT_MODEL_DIR)
    r.set_defaults(func=cmd_residency)

    args = ap.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
