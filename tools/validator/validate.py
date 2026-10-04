#!/usr/bin/env python3
"""Scenario validator for Long Way Home.

Checks that a scenario's declared content actually resolves before anything tries to load it:
entity classes against the official dictionaries, mission scripts through the compiler and the
game's own reader, navigation coverage per map, and asset references.

Error codes are stable so a fault can be referred to by name:

  E001  unknown entity class           (used in a map, absent from the merged dictionaries)
  E002  missing script                  (a `usescript` reference with no script on disk)
  E003  inhabited map lacks navigation  (declared in `inhabited` but has no waypoint family)
  E004  unresolved internal asset       (a models/ or textures/ path the scenario owns, missing)
  E005  malformed manifest              (missing, unparseable, or missing required keys)
  E006  declared script fails to compile
  E007  compiled script fails round-trip (the game's reader rejects it)
  W001  declared script never referenced by any map
  W004  map has no navigation coverage and is not declared inhabited (fine for space)
  W002  retail asset references seen (unverifiable without game data) - informational
  W003  script checks skipped (no compiler/reader supplied)

Usage:
  validate.py <scenario-dir> [--ibize PATH] [--ibi-dump PATH] [--entitydict PATH]
                              [--def PATH ...] [--quiet]
Exit status: 0 = no errors, 1 = at least one error, 2 = usage problem.
"""

import argparse
import fnmatch
import glob
import json
import os
import re
import subprocess
import sys
from collections import Counter

# Entity classes that provide navigation. A space without them can hold geometry but not crew.
NAV_CLASSES = ("waypoint", "waypoint_navgoal", "waypoint_squadpath", "waypoint_small",
               "point_combat", "path_corner")

CLASSNAME = re.compile(r'"classname"\s+"([^"]+)"')
USESCRIPT = re.compile(r'"usescript"\s+"([^"]+)"')
KV = re.compile(r'"([A-Za-z_][A-Za-z0-9_]*)"\s+"([^"]*)"')
# A brush face is three parenthesised point triples followed by the shader name.
FACE = re.compile(r"^\s*\(\s*[-\d. ]+\)\s*\(\s*[-\d. ]+\)\s*\(\s*[-\d. ]+\)\s+(\S+)", re.M)

# Scenario-owned asset namespace. Everything else a shipped map references (textures/, models/,
# sound/, gfx/ ...) comes from the retail game data, which we hold no copy of and cannot verify --
# those are reported as informational rather than as errors, and asserting otherwise would make the
# validator cry wolf on every real map.
INTERNAL_PREFIXES = ("assets/",)


class Report:
    def __init__(self):
        self.errors = []
        self.warnings = []
        self.info = []

    def error(self, code, where, detail):
        self.errors.append((code, where, detail))

    def warn(self, code, where, detail):
        self.warnings.append((code, where, detail))

    def note(self, detail):
        self.info.append(detail)

    def emit(self, quiet=False):
        for code, where, detail in self.errors:
            print(f"{code}  ERROR  {where}: {detail}")
        for code, where, detail in self.warnings:
            print(f"{code}  warn   {where}: {detail}")
        if not quiet:
            for line in self.info:
                print(f"          info   {line}")
        print(f"\n{len(self.errors)} error(s), {len(self.warnings)} warning(s)")


def load_dictionary(paths):
    """Merged entity dictionary plus its family templates (item_***** covers item_*)."""
    names = set()
    prefixes = []
    for p in paths:
        if not p or not os.path.exists(p):
            continue
        for name in json.load(open(p)):
            if name.endswith("*"):
                prefixes.append(name.split("*")[0])
            else:
                names.add(name)
    return names, prefixes


def check_manifest(root, rep):
    path = os.path.join(root, "scenario.json")
    if not os.path.exists(path):
        rep.error("E005", "scenario.json", "manifest not found")
        return None
    try:
        man = json.load(open(path))
    except (OSError, ValueError) as e:
        rep.error("E005", "scenario.json", f"unparseable: {e}")
        return None
    for key in ("name", "maps", "scripts"):
        if key not in man:
            rep.error("E005", "scenario.json", f"missing required key '{key}'")
    return man


def script_stem(declared_path, root):
    """A declared script's identity as a map would reference it: path without extension,
    relative to the scenario root, forward slashes, lower case."""
    rel = os.path.relpath(declared_path, root).replace("\\", "/")
    return os.path.splitext(rel)[0].lower()


def check_map(path, root, dictionary, prefixes, rep, inhabited, declared_scripts,
              script_names_by_base):
    # `dictionary is None` means no dictionary was supplied. Checking classes against an empty
    # dictionary would report every class in every map as unknown -- noise that looks like a finding.
    # Skip the check and say so instead.
    rel = os.path.relpath(path, root)
    text = open(path, encoding="utf-8", errors="replace").read()

    classes = Counter(CLASSNAME.findall(text))
    if dictionary is not None:
        unknown = sorted(c for c in classes
                         if c not in dictionary and not any(c.startswith(p) for p in prefixes))
        for name in unknown:
            rep.error("E001", rel, f"unknown entity class '{name}' ({classes[name]} instance(s))")

    nav = sum(n for c, n in classes.items() if c.startswith(NAV_CLASSES))
    if nav == 0:
        # Not every space is inhabited: a firing range, a cargo bay or an exterior has no crew
        # walking it. Only a space the scenario declares inhabited must be navigable.
        if inhabited:
            rep.error("E003", rel, "declared inhabited but has no navigation entities")
        else:
            rep.warn("W004", rel, "no navigation coverage and not declared inhabited")
    else:
        rep.note(f"{rel}: {nav} navigation entities, {len(classes)} distinct classes")

    # Resolve references against the scenario's *declared* scripts, not against a filesystem walk:
    # a scenario says what it contains, and that declaration is what a reference must satisfy.
    for script in sorted(set(USESCRIPT.findall(text))):
        key = os.path.splitext(script.replace("\\", "/"))[0].lower()
        if key in declared_scripts or os.path.basename(key) in script_names_by_base:
            continue
        rep.error("E002", rel, f"usescript '{script}' is not among the scenario's declared scripts")

    internals = {shader for shader in FACE.findall(text)
                 if shader.lower().startswith(INTERNAL_PREFIXES) and not shader.startswith(("//", "(", "{"))}
    for shader in sorted(internals):
        if not glob.glob(os.path.join(root, "**", os.path.basename(shader) + "*"), recursive=True):
            rep.error("E004", rel, f"internal asset '{shader}' not found in the scenario")

    retail = sum(1 for shader in FACE.findall(text)
                 if not shader.lower().startswith(INTERNAL_PREFIXES))
    if retail:
        rep.warn("W002", rel, f"{retail} retail asset reference(s) - unverifiable without game data")
    return set(USESCRIPT.findall(text))


def check_scripts(scripts, referenced, ibize, ibi_dump, rep, workdir, root):
    if not ibize or not ibi_dump or not (os.path.exists(ibize) and os.path.exists(ibi_dump)):
        rep.warn("W003", "scripts", "no compiler/reader supplied: script checks skipped")
        return
    os.makedirs(workdir, exist_ok=True)
    for script in scripts:
        base = os.path.splitext(os.path.basename(script))[0]
        if base not in {os.path.splitext(os.path.basename(r))[0] for r in referenced}:
            rep.warn("W001", os.path.relpath(script, root), "declared but never referenced by any map")
        out = os.path.join(workdir, os.path.relpath(script, root).replace("/", "_") + ".IBI")
        try:
            r = subprocess.run([ibize, script, out], capture_output=True, timeout=30)
        except (OSError, subprocess.TimeoutExpired) as e:
            rep.error("E006", os.path.relpath(script), f"compiler did not run: {e}")
            continue
        if r.returncode != 0 or r.stderr.strip() or not os.path.exists(out):
            rep.error("E006", os.path.relpath(script),
                      (r.stderr.decode(errors="replace").strip() or f"exit {r.returncode}")[:100])
            continue
        d = subprocess.run([ibi_dump, out], capture_output=True, timeout=30)
        if not d.stdout.decode(errors="replace").startswith("OK\t"):
            rep.error("E007", os.path.relpath(script), "compiled stream failed to read back")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("scenario")
    ap.add_argument("--ibize")
    ap.add_argument("--ibi-dump", dest="ibi_dump")
    ap.add_argument("--entitydict", help="JSON produced by entitydict.py parse --json")
    ap.add_argument("--quiet", action="store_true")
    a = ap.parse_args()

    root = os.path.abspath(a.scenario)
    if not os.path.isdir(root):
        print(f"not a directory: {root}")
        return 2

    rep = Report()
    man = check_manifest(root, rep)
    if man is None:
        rep.emit(a.quiet)
        return 1

    def expand(patterns):
        """Expand the manifest's globs, de-duplicated: overlapping patterns (say *.map and
        **/*.map) otherwise make one file report its faults twice."""
        seen, out = set(), []
        for pat in patterns:
            for hit in sorted(glob.glob(os.path.join(root, pat), recursive=True)):
                real = os.path.realpath(hit)
                if real not in seen:
                    seen.add(real)
                    out.append(hit)
        return out

    maps = expand(man.get("maps", []))
    if not maps:
        rep.error("E005", "scenario.json", "no maps matched the declared patterns")
    scripts = expand(man.get("scripts", []))

    if a.entitydict:
        dictionary, prefixes = load_dictionary([a.entitydict])
        if not dictionary:
            rep.warn("W003", "entities", f"dictionary {a.entitydict} is empty: entity-class check skipped")
            dictionary, prefixes = None, []
    else:
        rep.warn("W003", "entities",
                 "no --entitydict supplied: entity-class check skipped (not an all-clear)")
        dictionary, prefixes = None, []

    inhabited_globs = [os.path.join(root, g) for g in man.get("inhabited", [])]
    def is_inhabited(path):
        return any(fnmatch.fnmatch(path, g) for g in inhabited_globs)

    declared = {script_stem(s, root) for s in scripts}
    by_base = {os.path.basename(k): k for k in declared}

    referenced = set()
    for m in maps:
        referenced |= check_map(m, root, dictionary, prefixes, rep, is_inhabited(m),
                                declared, by_base)

    check_scripts(scripts, referenced, a.ibize, a.ibi_dump, rep,
                  os.path.join(root, ".validate-build"), root)

    rep.note(f"{len(maps)} map(s), {len(scripts)} script(s) checked")
    rep.emit(a.quiet)
    return 1 if rep.errors else 0


if __name__ == "__main__":
    sys.exit(main())
