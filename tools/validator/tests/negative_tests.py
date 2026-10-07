#!/usr/bin/env python3
"""Negative tests for the scenario validator.

A validator that has never been shown a fault cannot be trusted. This builds a clean fixture
around a real shipped map source, confirms the fixture passes, then seeds one deliberate fault
at a time and asserts the validator catches it -- by error code, and naming the offending object.

Usage:
  negative_tests.py --source-map /path/to/a/shipped.map
                    [--ibize PATH] [--ibi-dump PATH] [--entitydict PATH]

Fixtures are generated at run time, including a copy of the supplied map. Nothing third-party
is stored in the repository.
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
VALIDATOR = os.path.join(os.path.dirname(HERE), "validate.py")

SCRIPT_GOOD = 'rem ( "validator fixture" );\nwait ( 1.000 );\n'

USESCRIPT = re.compile(r'"usescript"\s+"([^"]+)"')


def corpus_index(corpus):
    """Case-insensitive map of script stem -> path, so a map's references can be resolved."""
    idx = {}
    for dirpath, _, names in os.walk(corpus):
        for n in names:
            if n.lower().endswith(".txt"):
                rel = os.path.relpath(os.path.join(dirpath, n), corpus)
                idx[os.path.splitext(rel)[0].replace("\\", "/").lower()] = os.path.join(dirpath, n)
    return idx


def build_clean(root, source_map, corpus):
    """A clean fixture must be realistic: a shipped map references shipped scripts, so those
    scripts are supplied too. Otherwise 'clean' just means 'nothing has been read'."""
    os.makedirs(os.path.join(root, "maps"), exist_ok=True)
    os.makedirs(os.path.join(root, "scripts"), exist_ok=True)
    deck = os.path.join(root, "maps", "deck.map")
    shutil.copy(source_map, deck)
    os.chmod(deck, 0o644)   # copytree/copy preserve the source's mode; fixtures must stay editable

    index = corpus_index(corpus) if corpus else {}
    text = open(deck, encoding="utf-8", errors="replace").read()
    for ref in sorted(set(USESCRIPT.findall(text))):
        src = index.get(ref.lower())
        if src:
            dest = os.path.join(root, "scripts", ref + ".txt")
            os.makedirs(os.path.dirname(dest), exist_ok=True)
            shutil.copy(src, dest)
            os.chmod(dest, 0o644)
    if corpus:
        with open(os.path.join(root, "scripts", "deck.txt"), "w") as fh:
            fh.write(SCRIPT_GOOD)
    json.dump({"name": "fixture", "maps": ["maps/*.map"], "scripts": ["scripts/**/*.txt"],
               "inhabited": ["maps/deck.map"]},
              open(os.path.join(root, "scenario.json"), "w"))


def run(root, args):
    cmd = [sys.executable, VALIDATOR, root, "--quiet"]
    for flag, value in (("--ibize", args.ibize), ("--ibi-dump", args.ibi_dump),
                        ("--entitydict", args.entitydict)):
        if value:
            cmd += [flag, value]
    r = subprocess.run(cmd, capture_output=True, text=True)
    return r.returncode, r.stdout + r.stderr


def seed_unknown_class(root):
    with open(os.path.join(root, "maps", "deck.map"), "a") as fh:
        fh.write('\n// seeded fault\n{\n"classname" "Foo_Unknown_Class"\n"origin" "0 0 0"\n}\n')


def seed_missing_script(root):
    with open(os.path.join(root, "maps", "deck.map"), "a") as fh:
        fh.write('\n// seeded fault\n{\n"classname" "target_scriptrunner"\n'
                 '"usescript" "no_such_script"\n"origin" "0 0 0"\n}\n')


def seed_no_nav(root):
    with open(os.path.join(root, "maps", "empty_room.map"), "w") as fh:
        fh.write('{\n"classname" "worldspawn"\n"message" "empty"\n}\n'
                 '{\n"classname" "light"\n"origin" "0 0 0"\n}\n')
    # declare it inhabited: a space the scenario claims crew will live in must be navigable
    man = json.load(open(os.path.join(root, "scenario.json")))
    man["inhabited"] = ["maps/deck.map", "maps/empty_room.map"]
    json.dump(man, open(os.path.join(root, "scenario.json"), "w"))


def seed_missing_asset(root):
    with open(os.path.join(root, "maps", "deck.map"), "a") as fh:
        fh.write('\n{\n"classname" "func_static"\n// brush 0\n{\n'
                 '( 0 0 0 ) ( 1 0 0 ) ( 0 1 0 ) assets/scenario/absent_shader 0 0 0 1 1\n}\n}\n')


def seed_bad_manifest(root):
    json.dump({"name": "fixture"}, open(os.path.join(root, "scenario.json"), "w"))


def seed_broken_script(root):
    with open(os.path.join(root, "scripts", "deck.txt"), "w") as fh:
        fh.write('this is not a script ( "unbalanced );\n')


def seed_content_overflow(root):
    """A level whose model registration approaches MAX_MODELS. Warnings do not fail the run, so
    this case asserts the warning is *emitted and named* rather than that the exit code changes."""
    with open(os.path.join(root, "maps", "dense.map"), "w") as fh:
        fh.write('{\n"classname" "worldspawn"\n}\n')
        for i in range(200):
            fh.write('{\n"classname" "misc_model"\n"model" "models/deck/prop_%03d.md3"\n'
                     '"origin" "0 0 0"\n}\n' % i)


WARN_CASES = [
    ("W005 content registration near the model limit", seed_content_overflow, "W005", "distinct models"),
]

CASES = [
    ("E001 unknown entity class", seed_unknown_class, "E001", "Foo_Unknown_Class"),
    ("E002 missing script", seed_missing_script, "E002", "no_such_script"),
    ("E003 map without navigation", seed_no_nav, "E003", "empty_room.map"),
    ("E004 unresolved internal asset", seed_missing_asset, "E004", "absent_shader"),
    ("E005 malformed manifest", seed_bad_manifest, "E005", "scenario.json"),
    ("E006 script fails to compile", seed_broken_script, "E006", "deck.txt"),
]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--source-map", required=True)
    ap.add_argument("--ibize")
    ap.add_argument("--ibi-dump", dest="ibi_dump")
    ap.add_argument("--entitydict")
    ap.add_argument("--script-corpus", dest="script_corpus",
                    help="directory of shipped scripts, used to make the clean fixture realistic")
    args = ap.parse_args()

    if not os.path.exists(args.source_map):
        print(f"source map not found: {args.source_map}")
        return 2

    failures = 0
    with tempfile.TemporaryDirectory() as tmp:
        clean = os.path.join(tmp, "clean")
        build_clean(clean, args.source_map, args.script_corpus)
        rc, out = run(clean, args)
        if rc == 0 and "ERROR" not in out:
            print("PASS  clean fixture validates with no errors")
        else:
            # The baseline must be clean, or every seeded case below is measured against a broken
            # fixture and a real regression reads like fixture noise. Say so as a fixture problem,
            # naming the source map, rather than letting it surface as an E003 from inside a case.
            print(f"FAIL  the clean fixture is not clean: the supplied source map is not a usable "
                  f"clean fixture ({os.path.basename(args.source_map)})")
            print("      A clean fixture must be built around a space whose declared-inhabited map "
                  "has navigation entities (a holomatch map has none).")
            for line in out.splitlines()[:8]:
                print("      " + line)
            return 2

        for label, seed, code, needle in CASES:
            case = os.path.join(tmp, "case_" + code)
            shutil.copytree(clean, case)
            for dirpath, _, names in os.walk(case):
                for n in names:
                    os.chmod(os.path.join(dirpath, n), 0o644)
            seed(case)
            rc, out = run(case, args)
            caught = (rc == 1) and any(line.startswith(code) and needle in line
                                       for line in out.splitlines())
            print(f"{'PASS' if caught else 'FAIL'}  {label}: expected {code} naming '{needle}' (rc={rc})")
            if not caught:
                failures += 1
                print("      output was:\n" + "\n".join("      " + l for l in out.splitlines()[:8]))

        for label, seed, code, needle in WARN_CASES:
            case = os.path.join(tmp, "warn_" + code)
            shutil.copytree(clean, case)
            for dirpath, _, names in os.walk(case):
                for n in names:
                    os.chmod(os.path.join(dirpath, n), 0o644)
            seed(case)
            rc, out = run(case, args)
            caught = any(line.startswith(code) and needle in line for line in out.splitlines())
            print(f"{'PASS' if caught else 'FAIL'}  {label}: expected {code} naming '{needle}'")
            if not caught:
                failures += 1
                print("      output was:\n" + "\n".join("      " + l for l in out.splitlines()[:8]))

    print(f"\n{'ALL PASS' if not failures else str(failures) + ' FAILURE(S)'}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
