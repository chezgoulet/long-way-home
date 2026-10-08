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

    render.py --cache DIR --manifest FILE [--synthesize PATH] [--dry-run]

--dry-run prints the synthesizer command it would run and touches no model. It is the
end-to-end proof: the delivery direction that started in the meeting brief arrives here
as an --exaggeration argument.
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
        return job["reference"]
    return os.path.join(cache, "refs", job["voice"] + ".wav")


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


def render(cache, manifest, synth, dry_run):
    jobs = load_manifest(manifest)
    done = skipped = refused = 0
    for job in jobs:
        try:
            exag = exaggeration_for(job["delivery"])
        except JobError as exc:
            print(f"refused : {exc}")
            refused += 1
            continue
        out = output_path(cache, job)
        if os.path.exists(out):
            print(f"cached  : {out} (a re-render is a no-op, key {job['key']})")
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
        done += 1
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
    ap.add_argument("--dry-run", action="store_true",
                    help="print the synthesizer commands; touch no model")
    args = ap.parse_args()

    if inside_repo(args.cache, repo_root):
        print(f"render: refusing a cache inside the repository: {args.cache}", file=sys.stderr)
        return 2
    if not os.path.exists(args.manifest):
        print(f"render: no manifest at {args.manifest}", file=sys.stderr)
        return 2
    return 1 if render(args.cache, args.manifest, args.synthesize, args.dry_run) else 0


if __name__ == "__main__":
    raise SystemExit(main())
