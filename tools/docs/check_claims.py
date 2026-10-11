#!/usr/bin/env python3
"""Class 2 of ``scripts/docs-check.sh``: an owned number may not be contradicted.

Some numbers have a single owner.  The main computer core's deck is recorded in
``docs/ship-master-map.md``; the code that builds it, the budget table, the lore ledger and the access
document must all agree, or one of them is wrong by construction.  A code constant -- the save format
version -- is owned by the file that defines it, and any document that states its current value must
agree with that file.  The claims, and the patterns that find them, are in
``tools/docs/owned_claims.json``.

Two shapes of claim, because a number lives in two kinds of place:

- **A stated figure** (``sources``): the owner is a document, every other place that states the number
  is listed, and each listed statement must match the owner.
- **A coded constant** (``current_form``): the owner is the code file, and *every* document that
  asserts the constant -- in the owner's own assertion form, e.g. ``SAVE_VERSION = 56`` -- must match
  it.  This is the shape that catches the save version drifting across the corpus.

**History is not drift.**  A ledger entry about a past pass legitimately says "save version 54"; a
dated handoff legitimately records the value of its date.  Those are prose statements and the check
does not read them.  It reads only the assertion form -- the shape a document uses to tell a reader
what the constant *is*.  A check that could not tell a current claim from a dated record would flag the
ledger's own history, which is a false positive, not a guard.  The convention is recorded in
``owned_claims.json`` (``_tense_convention``) and in ``docs/evidence/the-number-owners.md``.

Scope, stated honestly:

- It catches drift in the claims that are **registered**.  It cannot notice a contradiction nobody
  has registered; registering one is a JSON row.
- It does **not** police ``docs/evidence/``: those are dated records, allowed to hold superseded
  values, which are annotated rather than rewritten.  A stale value there is a person's to catch.
- For a coded constant it proves that a document *asserts the same number as the code*; it cannot
  prove the sentence around the number is still true.
- A file may be exempt from the assertion-form scan when it is **recording** another file's
  assertion rather than making one; each exemption carries its reason in the registry.

Exit 0 when every registered claim agrees with its owner; 1 otherwise (a mismatch, or a registered
owner/source pattern that no longer matches -- a stale registry row is drift too).
"""

import argparse
import json
import re
import sys
from pathlib import Path

# Directories that hold generated, vendored or upstream content, never our prose.
SKIP_DIRS = {"build", "upstream", ".git", ".venv", "venv", "node_modules", "third_party"}
# Dated records: allowed to hold superseded values, so never read as current-tense claims.
EVIDENCE_DIR = "docs/evidence"


def _read(path):
    return path.read_text(encoding="utf-8", errors="replace")


def check_stated_claim(root, claim, out):
    """A document owns the value; every other place that states it must agree."""
    cid = claim["id"]
    value = str(claim["value"])
    owner = claim["owner"]
    owner_pat = claim["owner_pattern"]
    ok = True

    text = _read(root / owner)
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
        stext = _read(root / f)
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


def current_form_hits(root, claim):
    """Yield (relative_file, line, captured_value) for every assertion-form statement of the claim.

    Every ``*.md`` under the root is scanned except the owner, ``docs/evidence/`` (a dated record)
    and the named ``exempt_files`` (files that quote another file's assertion rather than making
    one).  The form is the owner's own assertion, so a prose reference to the same number is not a
    hit -- that is the historical/current distinction the convention draws.
    """
    owner = claim["owner"]
    pat = claim["current_form"]
    exempt = {e["file"] for e in claim.get("exempt_files", [])}
    for f in sorted(root.rglob("*.md")):
        rel = f.relative_to(root)
        rels = str(rel)
        if set(rel.parts[:-1]) & SKIP_DIRS:
            continue
        if rels == owner or rels in exempt:
            continue
        if rels == EVIDENCE_DIR or rels.startswith(EVIDENCE_DIR + "/"):
            continue
        text = _read(f)
        for m in re.finditer(pat, text, re.MULTILINE):
            yield rels, text.count("\n", 0, m.start()) + 1, m.group(1)


def check_coded_claim(root, claim, out):
    """A code constant is the owner; every assertion-form statement must agree with it."""
    cid = claim["id"]
    value = str(claim["value"])
    owner = claim["owner"]
    owner_pat = claim["owner_pattern"]
    ok = True

    text = _read(root / owner)
    found = [m.group(1) for m in re.finditer(owner_pat, text, re.MULTILINE)]
    if not found:
        out.append(f"FAIL  {cid}: owner {owner} no longer defines the constant "
                   f"(pattern {owner_pat!r})")
        ok = False
    elif any(v != value for v in found):
        out.append(f"FAIL  {cid}: owner {owner} defines {found}, registry expects {value}")
        ok = False

    n = 0
    for rel, ln, v in current_form_hits(root, claim):
        n += 1
        if v != value:
            out.append(f"FAIL  {cid}: {rel}:{ln} states {v} as the current value "
                       f"(the assertion form); owner {owner} says {value}")
            ok = False

    if ok:
        out.append(f"    {cid}: {owner} = {value}; the owner defines it; "
                   f"{n} document assertion(s), all agree")
    return ok


def check_claim(root, claim, out):
    if "current_form" in claim:
        return check_coded_claim(root, claim, out)
    return check_stated_claim(root, claim, out)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--root", default=".")
    ap.add_argument("--claims", default="tools/docs/owned_claims.json")
    args = ap.parse_args(argv)

    root = Path(args.root).resolve()
    claims_path = Path(args.claims)
    if not claims_path.is_absolute():
        claims_path = root / claims_path
    data = json.loads(_read(claims_path))
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
        print(f"{len(claims)} claim(s) registered; at least one contradicts its owner",
              file=sys.stderr)
        return 1
    print(f"    {len(claims)} claim(s) registered, all agree with their owner")
    return 0


if __name__ == "__main__":
    sys.exit(main())
