#!/usr/bin/env python3
"""The render worker: a meeting's planned lines, rendered ahead of the meeting.

The meeting plans its audio in the async window (module/ship/ship_core.cpp,
PlanMeetingAudio) and writes a manifest of render jobs, one JSON object per line:

    {"key":"29f23f90cbae50f9","voice":"tuvok","text":"Make it so.",
     "delivery":"order","exaggeration":0.8,"reference":"refs/tuvok.wav"}

This script drains that manifest through tools/voice/synthesize.py, carrying each
line's delivery direction to the model's own emotion control:

    --exaggeration  0.8   for an order, 0.5 a report, ... (docs/evidence/voice-review.md)

Three rules hold the line, and they are the point of the plumbing:

  * an UNMARKED delivery is refused -- the brief could not know the line's temperature,
    so the line is not rendered at all rather than defaulted to flat;
  * the output is keyed (text, voice, delivery) and a job whose clip already exists is
    skipped, so a line is never rendered twice;
  * the cache is player-local, beside the save, and is never inside this repository.
    There is no audio in the repository, ever (docs/asset-doctrine.md).

    render.py --cache DIR --manifest FILE [--synthesize PATH] [--durations FILE] [--dry-run]

--dry-run prints the synthesizer command it would run and touches no model. It is the
end-to-end proof: the delivery direction that started in the meeting brief arrives here
as an --exaggeration argument.

The durations of the clips it writes are measured (`ffprobe`) and recorded, one `key|seconds`
line each, in the durations file beside the cache (default `<cache>/durations.txt`). The host
loads them and paces the meeting's pills by them, so the options are offered for as long as
the line actually runs -- and a clip already on disk is measured rather than re-rendered.
"""

import argparse
import json
import os
import shlex
import subprocess
import sys

# The delivery vocabulary, in the review's own knob (exaggeration / emotion_adv). It must agree
# with DeliveryExaggeration in module/ship/ship_core.cpp; the test asserts they match.
DELIVERY_EXAGGERATION = {
    "order": 0.8,
    "report": 0.5,
    "confession": 0.35,
    "condolence": 0.3,
    "flat": 0.2,
}
UNKNOWN = "unmarked"


class JobError(Exception):
    pass


def exaggeration_for(delivery):
    """The delivery's exaggeration, or raise for UNMARKED (refused, never defaulted)."""
    if delivery == UNKNOWN or delivery not in DELIVERY_EXAGGERATION:
        raise JobError(f"delivery {delivery!r} is unmarked: the line is refused, not defaulted")
    return DELIVERY_EXAGGERATION[delivery]


def load_manifest(path):
    jobs = []
    with open(path, "r", encoding="utf-8") as fh:
        for n, line in enumerate(fh, 1):
            line = line.strip()
            if not line:
                continue
            try:
                jobs.append(json.loads(line))
            except json.JSONDecodeError as exc:
                raise JobError(f"{path}:{n}: not JSON: {exc}") from exc
    return jobs


def inside_repo(path, repo_root):
    """True if `path` is tracked repository data (the cache must never be that).

    A path inside the working tree but under a gitignored directory -- build/, voice-scratch/,
    the player-local build home -- is not repository data, so it is allowed.
    """
    real = os.path.realpath(path)
    root = os.path.realpath(repo_root)
    if not (real == root or real.startswith(root + os.sep)):
        return False
    if real == root:
        return True
    ignored = subprocess.run(["git", "-C", root, "check-ignore", "-q", real],
                             stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return ignored.returncode != 0  # not ignored: it is repository data


def output_path(cache, job):
    return os.path.join(cache, job["key"] + ".wav")


def reference_path(cache, job):
    if job.get("reference"):
        ref = job["reference"]
        return ref if os.path.isabs(ref) else os.path.join(cache, ref)
    return os.path.join(cache, "refs", job["voice"] + ".wav")


def clip_duration(path):
    """The clip's own length in seconds, measured, or 0.0 when it cannot be read."""
    try:
        out = subprocess.run(
            ["ffprobe", "-v", "error", "-show_entries", "format=duration",
             "-of", "default=noprint_wrappers=1:nokey=1", path],
            capture_output=True, text=True, check=True).stdout.strip()
        return float(out)
    except (OSError, ValueError, subprocess.CalledProcessError):
        return 0.0


def command_for(synth, cache, job):
    """The synthesizer invocation for one job: the delivery has become --exaggeration."""
    exag = exaggeration_for(job["delivery"])
    if "exaggeration" in job and abs(float(job["exaggeration"]) - exag) > 1e-6:
        raise JobError(
            f"job {job['key']}: exaggeration {job['exaggeration']} disagrees with delivery "
            f"{job['delivery']!r} ({exag})")
    return [
        sys.executable, synth,
        "--reference", reference_path(cache, job),
        "--text", job["text"],
        "--out", output_path(cache, job),
        "--exaggeration", f"{exag}",
    ]


def render(cache, manifest, synth, dry_run, durations_path):
    jobs = load_manifest(manifest)
    done = skipped = refused = 0
    durations = []  # (key, seconds) measured from the clips on disk
    for job in jobs:
        try:
            exag = exaggeration_for(job["delivery"])
        except JobError as exc:
            print(f"refused : {exc}")
            refused += 1
            continue
        out = output_path(cache, job)
        if os.path.exists(out):
            secs = clip_duration(out)
            print(f"cached  : {out} ({secs:.3f}s; a re-render is a no-op, key {job['key']})")
            if secs > 0.0:
                durations.append((job["key"], secs))
            skipped += 1
            continue
        cmd = command_for(synth, cache, job)
        if dry_run:
            print("render  : " + shlex.join(cmd))
            done += 1
            continue
        os.makedirs(os.path.dirname(out), exist_ok=True)
        print(f"render  : {job['voice']} delivery={job['delivery']} exaggeration={exag}")
        subprocess.run(cmd, check=True)
        secs = clip_duration(out)
        if secs > 0.0:
            durations.append((job["key"], secs))
        done += 1
    if not dry_run and durations_path:
        os.makedirs(os.path.dirname(os.path.abspath(durations_path)), exist_ok=True)
        with open(durations_path, "w", encoding="utf-8") as fh:
            for key, secs in durations:
                fh.write("%s|%.3f\n" % (key, secs))
        print(f"durations: {len(durations)} clip(s) measured -> {durations_path}")
    print(f"== {done} to render, {skipped} cached, {refused} refused")
    return refused


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    repo_root = os.path.dirname(os.path.dirname(here))
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--cache", required=True, help="the player-local cache beside the save")
    ap.add_argument("--manifest", required=True, help="the JSON-lines manifest to drain")
    ap.add_argument("--synthesize", default=os.path.join(here, "synthesize.py"),
                    help="the synthesizer to call (default: beside this script)")
    ap.add_argument("--durations", default="",
                    help="where to record each clip's measured duration (default: <cache>/durations.txt)")
    ap.add_argument("--dry-run", action="store_true",
                    help="print the synthesizer commands; touch no model")
    args = ap.parse_args()

    if inside_repo(args.cache, repo_root):
        print(f"render: refusing a cache inside the repository: {args.cache}", file=sys.stderr)
        return 2
    if not os.path.exists(args.manifest):
        print(f"render: no manifest at {args.manifest}", file=sys.stderr)
        return 2
    durations = args.durations or os.path.join(args.cache, "durations.txt")
    return 1 if render(args.cache, args.manifest, args.synthesize, args.dry_run, durations) else 0


if __name__ == "__main__":
    raise SystemExit(main())
