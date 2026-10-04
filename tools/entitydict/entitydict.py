#!/usr/bin/env python3
"""Entity dictionary for the Elite Force single-player entity set.

Turns Raven's `SP_entities.def` -- the authoritative list of entity classes an editor
offers a level designer -- into machine-readable form, and cross-checks it against the
entity classes that actually appear in the shipped map sources.

That cross-check is the point. A dictionary that does not cover what the game's own maps
use cannot be trusted as a gate for new content, and a class used but not documented is a
finding, not a curiosity.

Usage:
  entitydict.py parse   --def SP_entities.def --json out/entities.json
  entitydict.py scan    --maps <dir> [--json out/map-entities.json]
  entitydict.py check   --def SP_entities.def --maps <dir>     # exit 1 on undocumented use
"""

import argparse
import json
import os
import re
import sys
from collections import Counter

# Format varies across the file. The colour triple is always present; the mins/maxs pairs are
# optional (classes that inherit their bounds omit them), and the spawnflag field may be a bare
# '?' meaning "no flags defined". Parse the shape, do not assume it.
# The name is followed by the colour group with or without a space ("NPC_Munro (1 0 0)" vs
# "NPC_HunterSeeker(1 0 0)"), so the name must stop at an opening bracket.
QUAKED = re.compile(r"^/\*QUAKED\s+(?P<name>[^\s(]+)(?P<rest>[^*]*)$", re.M)
PARENS = re.compile(r"\(([^)]*)\)")

# A documented key is either quoted ("wait" ...) or an identifier followed by ' - '.
KEY_QUOTED = re.compile(r'^\s*"([A-Za-z_][A-Za-z0-9_]*)"', re.M)
KEY_DASHED = re.compile(r"^\s*([A-Za-z_][A-Za-z0-9_]{2,})\s+-\s+\S", re.M)
CLASSNAME_IN_MAP = re.compile(r'"classname"\s+"([^"]+)"')


def parse_def(path):
    raw = open(path, encoding="utf-8", errors="replace").read().replace("\r\n", "\n")
    entries = []
    for m in QUAKED.finditer(raw):
        body_start = raw.find("\n", m.end())
        body_end = raw.find("*/", body_start if body_start != -1 else m.end())
        body = raw[body_start:body_end] if body_start != -1 and body_end != -1 else ""
        name = m.group("name")
        groups = PARENS.findall(m.group("rest"))
        color = groups[0].split() if len(groups) > 0 else []
        mins  = groups[1].split() if len(groups) > 2 else []
        maxs  = groups[2].split() if len(groups) > 2 else []
        flags_text = PARENS.sub(" ", m.group("rest"))
        flags = [f for f in flags_text.replace("?", " ").split() if f not in ("x",)]
        keys = sorted(set(KEY_QUOTED.findall(body)) | set(KEY_DASHED.findall(body)))
        entries.append({
            "name": name,
            "color": color,
            "mins": mins,
            "maxs": maxs,
            "flags": flags,
            "keys": keys,
            "doc_lines": [l for l in body.splitlines() if l.strip()],
            "wildcard": "*" in name,
        })
    return entries


def scan_maps(root):
    counts = Counter()
    files = 0
    for dirpath, _, names in os.walk(root):
        for n in names:
            if n.lower().endswith(".map"):
                files += 1
                try:
                    text = open(os.path.join(dirpath, n), encoding="utf-8", errors="replace").read()
                except OSError:
                    continue
                counts.update(CLASSNAME_IN_MAP.findall(text))
    return counts, files


def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)

    p = sub.add_parser("parse"); p.add_argument("--def", dest="deffile", action="append", required=True); p.add_argument("--json")
    s = sub.add_parser("scan");  s.add_argument("--maps", required=True); s.add_argument("--json")
    c = sub.add_parser("check"); c.add_argument("--def", dest="deffile", action="append", required=True); c.add_argument("--maps", required=True)
    a = ap.parse_args()

    if a.cmd == "parse":
        entries = [e for f in a.deffile for e in parse_def(f)]
        doc = {e["name"]: e for e in entries}
        if a.json:
            os.makedirs(os.path.dirname(a.json), exist_ok=True)
            json.dump(doc, open(a.json, "w"), indent=1, sort_keys=True)
        print(f"parsed {len(entries)} entity classes from {', '.join(os.path.basename(f) for f in a.deffile)}")
        print(f"  with documented keys: {sum(1 for e in entries if e['keys'])}")
        print(f"  spawnflag sets:       {sum(1 for e in entries if e['flags'])}")
        print(f"  wildcard/template:    {[e['name'] for e in entries if e['wildcard']]}")
        return 0

    if a.cmd == "scan":
        counts, files = scan_maps(a.maps)
        if a.json:
            os.makedirs(os.path.dirname(a.json), exist_ok=True)
            json.dump(dict(counts), open(a.json, "w"), indent=1, sort_keys=True)
        print(f"{files} map files, {len(counts)} distinct entity classes")
        for name, n in counts.most_common(15):
            print(f"  {n:6d}  {name}")
        return 0

    entries = [e for f in a.deffile for e in parse_def(f)]
    documented = {e["name"] for e in entries}
    counts, files = scan_maps(a.maps)
    used = set(counts)

    # The dictionaries document family templates ("item_*****", "weapon_*****"), which cover any
    # concrete class sharing their prefix. Treat those as documented rather than as gaps.
    prefixes = [n.split("*")[0] for n in documented if n.endswith("*")]
    covered = {u for u in used if u in documented or any(u.startswith(px) for px in prefixes)}
    undocumented = sorted(used - covered)
    documented = {n for n in documented if not n.endswith("*")}
    unused = sorted(documented - used)

    print(f"dictionary: {len(documented)} classes (+{len(prefixes)} family templates)   "
          f"maps: {files} files, {len(used)} classes used")
    print(f"used AND documented : {len(covered)}")
    print(f"used, UNDOCUMENTED  : {len(undocumented)}")
    for name in undocumented:
        print(f"  !! {name}  ({counts[name]} instances)")
    print(f"documented, unused  : {len(unused)}")
    if unused:
        print("   " + ", ".join(unused[:12]) + (" ..." if len(unused) > 12 else ""))
    return 1 if undocumented else 0


if __name__ == "__main__":
    sys.exit(main())
