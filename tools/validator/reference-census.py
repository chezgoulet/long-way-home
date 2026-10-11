#!/usr/bin/env python3
"""Census of the script references the published map sources carry.

The residual this answers -- *"126 script references the GDK corpus lacks"* -- was produced by a
scenario declaring every published map source and the shipped script corpus. This rebuilds that
scenario against a corpus and a map root you name, runs the validator over it, and separates what
it finds into the classes that decide whether a reference matters (see
`docs/evidence/the-126-references.md`):

  exact          the reference's path is in the corpus
  basename       the path is not, but a script of that name is somewhere in the corpus -- a
                 different directory, so "resolved" is the validator's leniency, not proof
  case           the path is in the corpus, but the manifest's case-sensitive `*.txt` glob missed
                 it (the GDK ships `foo.txt` and `Foo.TXT`; glob does not know that)
  absent         no spelling of the corpus satisfies it -- the boundary this check asserts

The first three are measured with the validator, run twice (a `*.txt` declaration and a
case-insensitive `*.[tT][xX][tT]` one); the difference between the runs is the `case` class. The
`absent` set is asserted, not reported: the check fails when it grows (a new dangling reference) or
shrinks (the maps or the corpus changed under the record). The two entries it carries are malformed
`usescript` values in shipped maps, named with their reasons below.

Usage:
  reference-census.py --map-root DIR --corpus DIR [--validator PATH] [--json OUT]
Exit status: 0 = the unresolved set is exactly the known boundary; 1 = it changed; 2 = usage.
"""

import argparse
import json
import os
import re
import subprocess
import sys
import tempfile
from collections import defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT_VALIDATOR = os.path.join(HERE, "validate.py")

# The references no corpus spelling satisfies. Each is a malformed field in a published map, not a
# script the GDK lacks; the reason is the classification's conclusion, kept beside the name so the
# check reads as a record rather than as a threshold. Detail: docs/evidence/the-126-references.md.
KNOWN_BOUNDARY = {
    "bs_run_and_shoot":
        "a behaviour state (SET_BEHAVIOR_STATE 'BS_RUN_AND_SHOOT') used as a usescript in "
        "voy16/voy4/forge5/scav2/stasis2/dn5 -- not a script in any corpus",
    "newearthhome":
        "a ref_tag targetname used as a usescript in deck08 -- a tag, not a script",
}

USESCRIPT = re.compile(r'"usescript"\s+"([^"]+)"')
E002 = re.compile(r"^E002\s+ERROR\s+(.+?):\s+usescript\s+'([^']+)'\s+is in neither",
                  re.MULTILINE)

CASE_SENSITIVE = "scripts/**/*.txt"
CASE_INSENSITIVE = "scripts/**/*.[tT][xX][tT]"


def corpus_index(corpus):
    """Every script file, keyed as the validator keys a reference: stem, forward slashes, lower."""
    idx = {}
    for root, _, files in os.walk(corpus):
        for name in files:
            if name.lower().endswith(".txt"):
                rel = os.path.relpath(os.path.join(root, name), corpus).replace(os.sep, "/")
                idx[os.path.splitext(rel)[0].lower()] = rel
    return idx


def map_references(map_root):
    refs = defaultdict(set)
    for root, _, files in os.walk(map_root, followlinks=True):
        for name in files:
            if name.lower().endswith(".map"):
                path = os.path.join(root, name)
                with open(path, encoding="utf-8", errors="replace") as fh:
                    text = fh.read()
                for ref in USESCRIPT.findall(text):
                    refs[ref.replace("\\", "/").lower()].add(os.path.relpath(path, map_root))
    return refs


def run_validator(validator, map_root, corpus, pattern):
    """One scenario declaring the map sources and the corpus, validated; return key -> maps."""
    with tempfile.TemporaryDirectory(prefix="lwh-references-") as work:
        os.symlink(os.path.abspath(map_root), os.path.join(work, "maps"))
        os.symlink(os.path.abspath(corpus), os.path.join(work, "scripts"))
        with open(os.path.join(work, "scenario.json"), "w") as fh:
            json.dump({"name": "published-map-references",
                       "maps": ["maps/**/*.map"], "scripts": [pattern]}, fh)
        r = subprocess.run([sys.executable, validator, work, "--quiet"],
                           capture_output=True, text=True)
        # rc is 1 when there are errors; that is the normal case here. Only a usage failure (2) or
        # a crash is a problem for the census itself.
        if r.returncode not in (0, 1):
            raise RuntimeError(f"validator exited {r.returncode}: {r.stdout}{r.stderr}")
        found = defaultdict(set)
        for where, ref in E002.findall(r.stdout + r.stderr):
            found[ref.replace("\\", "/").lower()].add(where)
        return found


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--map-root", required=True, help="the published map sources")
    ap.add_argument("--corpus", required=True, help="the script corpus")
    ap.add_argument("--validator", default=DEFAULT_VALIDATOR)
    ap.add_argument("--json", help="write the census as JSON")
    a = ap.parse_args()

    if not os.path.isdir(a.map_root):
        print(f"map root not found: {a.map_root}")
        return 2
    if not os.path.isdir(a.corpus):
        print(f"corpus not found: {a.corpus}")
        return 2

    idx = corpus_index(a.corpus)
    basenames = {os.path.basename(k) for k in idx}
    refs = map_references(a.map_root)
    exact = sorted(k for k in refs if k in idx)
    basename = sorted(k for k in refs if k not in idx and os.path.basename(k) in basenames)
    unresolved = sorted(k for k in refs if k not in idx and os.path.basename(k) not in basenames)

    cs = run_validator(a.validator, a.map_root, a.corpus, CASE_SENSITIVE)
    ci = run_validator(a.validator, a.map_root, a.corpus, CASE_INSENSITIVE)
    case = sorted(set(cs) - set(ci))
    absent = sorted(ci)
    known = set(KNOWN_BOUNDARY)

    print(f"map root: {a.map_root}")
    print(f"corpus:   {a.corpus} ({len(idx)} script files)")
    print(f"references in the map sources: {len(refs)} distinct (usescript), "
          f"in {sum(len(v) for v in refs.values())} map-reference pairs")
    print(f"  exact path in the corpus:   {len(exact)}")
    print(f"  basename elsewhere only:    {len(basename)}")
    for r in basename:
        print(f"    {r}   (referenced by {', '.join(sorted(refs[r]))})")
    print(f"  missed only by the `.txt` glob: {len(case)}")
    for r in case:
        print(f"    {r}")
    print(f"  genuinely absent (the boundary): {len(absent)}")
    for r in absent:
        print(f"    {r}" + (f"   -- {KNOWN_BOUNDARY[r]}" if r in known else "   -- UNKNOWN"))

    # The validator's own account of the same two runs, so the numbers above are traceable to it.
    print(f"  validator: `{CASE_SENSITIVE}` -> {len(cs)} unresolved; "
          f"`{CASE_INSENSITIVE}` -> {len(ci)} unresolved")

    grew = sorted(set(absent) - known)
    shrank = sorted(known - set(absent))
    if set(unresolved) != set(absent):
        print(f"\nFAIL  the validator and the corpus index disagree on the unresolved set: "
              f"{sorted(set(unresolved) ^ set(absent))}")
    if grew:
        print("\nFAIL  reference(s) the corpus does not satisfy and the record does not name:")
        for r in grew:
            print(f"      {r}   (referenced by {', '.join(sorted(ci[r]))})")
    if shrank:
        print("\nFAIL  reference(s) the record names that now resolve -- the record is stale:")
        for r in shrank:
            print(f"      {r}")
    ok = not grew and not shrank and set(unresolved) == set(absent)
    if ok:
        print(f"\nPASS  the {len(absent)} unresolved references are exactly the recorded boundary")

    if a.json:
        with open(a.json, "w") as fh:
            json.dump({"map_root": a.map_root, "corpus": a.corpus, "corpus_files": len(idx),
                       "references": {k: sorted(v) for k, v in refs.items()},
                       "exact": exact, "basename_only": basename,
                       "case_glob_only": case, "genuinely_absent": absent,
                       "known_boundary": KNOWN_BOUNDARY}, fh, indent=2)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
