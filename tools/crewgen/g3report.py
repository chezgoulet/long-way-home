#!/usr/bin/env python3
"""Judge a G3 measured run from the files the game module wrote.

    g3report.py CREW_DIR MAP_KEY [--saves DIR] [--frame-slack-us N] [--min-run-s N]

CREW_DIR holds, for MAP_KEY (the map name with '/' replaced by '_'):

    <key>.report.json           the crewed run            (required)
    <key>.baseline.json         the same run, layer off   (optional: script and frame comparison)
    <key>.saved.json            crew state as saved       (optional: with .restored.json,
    <key>.restored.json         crew state as reloaded     the save/load equality check)
    <key>.reloaded.report.json  the run after the reload  (optional: posts still held)

Prints one line per criterion of docs/g3-reactive-crew.md section 5 and exits 0 only if every
criterion passed. A criterion whose evidence is absent is reported as NOT MEASURED and fails the
run: an unmeasured gate is not a passed one.
"""

import argparse
import json
import os
import sys

PERSISTED = ("ent", "duty_post", "override_post", "goal_post", "action", "flags", "schedule_cursor",
             "first_arrival_ms")


def load(path):
    try:
        with open(path, encoding="utf-8") as f:
            return json.load(f)
    except (OSError, ValueError):
        return None


def judge(report, baseline=None, saved=None, restored=None, reloaded=None, save_sizes=None,
          frame_slack_us=1000, min_run_s=600):
    """Returns a list of (criterion, status, detail); status is PASS, FAIL or 'NOT MEASURED'."""
    rows = []

    def row(name, ok, detail):
        rows.append((name, "NOT MEASURED" if ok is None else "PASS" if ok else "FAIL", detail))

    members = report.get("members", [])
    n = len(members)
    row("5-10 crew present on one deck", 5 <= n <= 10, f"{n} on {report.get('map')}")

    samples, sample_s = report.get("samples", 0), report.get("sample_ms", 5000) / 1000.0
    run_s = max(samples - 1, 0) * sample_s
    cover, bound = report.get("min_coverage_percent", 0), report.get("coverage_bound_percent", 90)
    row("post coverage at every sample", samples > 0 and cover >= bound,
        f"minimum {cover}% over {samples} samples (bar {bound}%)")
    row("observation run long enough", run_s >= min_run_s, f"{run_s:.0f} s sampled (bar {min_run_s} s)")

    reach = report.get("reach_bound_ms", 0)
    arrivals = [m.get("first_arrival_ms", -1) for m in members]
    never = sum(a < 0 for a in arrivals)
    row("every NPC reaches its post within the bound", bool(arrivals) and all(0 <= a <= reach for a in arrivals),
        f"slowest {max(arrivals) if arrivals else -1} ms (bound {reach} ms)" + (f"; {never} never arrived" if never else ""))

    row("zero navigation failures", report.get("stuck_events", 1) == 0 and report.get("out_of_world", 1) == 0,
        f"{report.get('stuck_events')} stuck, {report.get('out_of_world')} out of world")

    addresses, acks, missed = report.get("addresses", 0), report.get("acks", 0), report.get("missed_acks", 0)
    answered = acks == addresses and missed == 0 and report.get("max_ack_ms", 0) <= report.get("ack_bound_ms", 0)
    row("address -> acknowledgement within the bound", None if addresses == 0 else answered,
        f"{acks} of {addresses} acknowledged, slowest {report.get('max_ack_ms')} ms (bound {report.get('ack_bound_ms')} ms)")

    violations = report.get("violations", 1)
    if baseline is None:
        row("no ICARUS script regressions", None if violations == 0 else False,
            f"{violations} precedence violations; no baseline run to compare the deck's scripts against")
    else:
        base = sorted(baseline.get("scripts", {}).get("running", []))
        crewed = sorted(report.get("scripts", {}).get("running", []))
        row("no ICARUS script regressions", violations == 0 and base == crewed,
            f"{violations} precedence violations; scripts in flight {len(crewed)} with the crew, {len(base)} without"
            + ("" if base == crewed else " -- the sets differ"))

    runs, yielded, resumed = (report.get(k, 0) for k in ("script_runs", "script_yields", "script_resumes"))
    row("a script takes a post-holder, and gives them back", None if runs == 0 else yielded == runs and resumed == runs,
        f"{runs} script run on a crew member at post: the layer yielded {yielded} time(s), and had them back on duty {resumed} time(s)")

    if saved is None or restored is None:
        row("save/load restores posts and schedule cursor", None, "no saved/restored pair")
    else:
        a = [{k: m.get(k) for k in PERSISTED} for m in saved.get("members", [])]
        b = [{k: m.get(k) for k in PERSISTED} for m in restored.get("members", [])]
        ok = bool(a) and a == b
        detail = f"{len(a)} crew compared field by field"
        if not ok:
            detail += " -- they differ"
        elif reloaded is None:
            ok, detail = None, detail + "; the run after the reload left no report"
        else:
            ok = reloaded.get("samples", 0) > 0 and reloaded.get("min_coverage_percent", 0) >= bound
            detail += f"; after the reload coverage is {reloaded.get('min_coverage_percent')}%"
        row("save/load restores posts and schedule cursor", ok, detail)

    per_npc = report.get("save_bytes", 0) / n if n else 0
    budget = report.get("save_budget_per_npc", 0)
    detail = f"{per_npc:.0f} bytes per NPC of layer state (budget {budget})"
    if save_sizes:
        detail += f"; the whole save grew {save_sizes[1] - save_sizes[0]} bytes with {n} crew added"
    row("save-size increase within budget", n > 0 and per_npc <= budget, detail)

    layer = f"layer {report.get('layer_avg_us')} us avg, {report.get('layer_max_us')} us max per frame"
    if baseline is None:
        row("frame time holds", None, layer + "; no baseline run to compare the game frame against")
    else:
        with_crew, without = report.get("game_frame_avg_us", 0), baseline.get("game_frame_avg_us", 0)
        row("frame time holds", with_crew <= without + frame_slack_us,
            f"game frame {with_crew} us with the crew, {without} us without (slack {frame_slack_us} us); " + layer)
    return rows


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("crew_dir")
    ap.add_argument("map_key")
    ap.add_argument("--saves", help="directory holding crewbase.sav and crewrun.sav, for the whole-save delta")
    ap.add_argument("--frame-slack-us", type=int, default=1000)
    ap.add_argument("--min-run-s", type=int, default=600)
    a = ap.parse_args(argv)

    def part(suffix):
        return load(os.path.join(a.crew_dir, f"{a.map_key}.{suffix}.json"))

    report = part("report")
    if report is None:
        print(f"no report for {a.map_key} in {a.crew_dir}: the run did not complete")
        return 1
    sizes = None
    if a.saves:
        paths = [os.path.join(a.saves, name) for name in ("crewbase.sav", "crewrun.sav")]
        if all(os.path.exists(p) for p in paths):
            sizes = [os.path.getsize(p) for p in paths]

    rows = judge(report, part("baseline"), part("saved"), part("restored"), part("reloaded.report"), sizes,
                 a.frame_slack_us, a.min_run_s)
    width = max(len(r[0]) for r in rows)
    for name, status, detail in rows:
        print(f"{status:<12}  {name:<{width}}  {detail}")
    bad = [r for r in rows if r[1] != "PASS"]
    print()
    print("G3 measured run: " + ("PASS" if not bad else f"NOT PASSED ({len(bad)} of {len(rows)} criteria)"))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
