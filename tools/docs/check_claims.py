#!/usr/bin/env python3
"""Class 2 of ``scripts/docs-check.sh``: an owned number may not be contradicted.

Some numbers have a single owner.  The main computer core's deck is recorded in
``docs/ship-master-map.md``; the code that builds it, the budget table, the lore ledger and the access
document must all agree, or one of them is wrong by construction.  The claims, and the patterns that
find them, are in ``tools/docs/owned_claims.json``.

Scope, stated honestly:

- It catches drift in the claims that are **registered**.  It cannot notice a contradiction nobody
  has registered; registering one is a JSON row.
- It does **not** police ``docs/evidence/``: those are dated records, allowed to hold superseded
  values, which are annotated rather than rewritten.  A stale value there is a person's to catch.

Exit 0 when every registered claim agrees with its owner; 1 otherwise (a mismatch, or an owner/source
pattern that no longer matches -- a stale registry row is drift too).
"""

import argparse
import json
import re
import sys
from pathlib import Path


def check_claim(root, claim, out):
    cid = claim["id"]
    value = str(claim["value"])
    owner = claim["owner"]
    owner_pat = claim["owner_pattern"]
    ok = True

    text = (root / owner).read_text(encoding="utf-8", errors="replace")
    found = [m.group(1) for m in re.finditer(owner_pat, text, re.MULTILINE)]
    if not found:
        out.append(f"FAIL  {cid}: owner {owner} no longer states the claim "
                   f"(pattern {owner_pat!r})")
        ok = False
    elif any(v != value for v in found):
        out.append(f"FAIL  {cid}: owner {owner} states {found}, registry expects {value}")
        ok = False

    n_source = 0
    for src in claim["sources"]:
        f = src["file"]
        pat = src["pattern"]
        stext = (root / f).read_text(encoding="utf-8", errors="replace")
        matches = [(m.group(1), stext.count("\n", 0, m.start()) + 1)
                   for m in re.finditer(pat, stext, re.MULTILINE)]
        if not matches:
            out.append(f"FAIL  {cid}: {f} no longer states the claim (pattern {pat!r})")
            ok = False
            continue
        for v, ln in matches:
            n_source += 1
            if str(v) != value:
                out.append(f"FAIL  {cid}: {f}:{ln} says {v}, owner {owner} says {value}")
                ok = False

    if ok:
        out.append(f"    {cid}: {owner} = {value}; {n_source} statement(s) agree")
    return ok


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--root", default=".")
    ap.add_argument("--claims", default="tools/docs/owned_claims.json")
    args = ap.parse_args(argv)

    root = Path(args.root).resolve()
    claims_path = Path(args.claims)
    if not claims_path.is_absolute():
        claims_path = root / claims_path
    data = json.loads(claims_path.read_text(encoding="utf-8"))
    claims = data.get("claims", [])
    if not claims:
        print("FAIL  no claims registered; the check would pass vacuously", file=sys.stderr)
        return 1

    out = []
    ok = True
    for c in claims:
        ok = check_claim(root, c, out) and ok

    for line in out:
        print(line, file=(sys.stdout if not line.startswith("FAIL") else sys.stderr))
    if not ok:
        print(f"{len(claims)} claim(s) registered; at least one contradicts its owner", file=sys.stderr)
        return 1
    print(f"    {len(claims)} claim(s) registered, all agree with their owner")
    return 0


if __name__ == "__main__":
    sys.exit(main())
