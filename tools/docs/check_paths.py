#!/usr/bin/env python3
"""Class 1 of ``scripts/docs-check.sh``: a cited ``docs/...`` path must resolve.

A document that cites a path which does not exist is a dangling reference -- exactly the **O19**
defect (``docs/gap-the-log.md`` cited where the file was ``docs/evidence/gap-the-log.md``).  This is
mechanical: extract every ``docs/...`` token from every markdown file in the repository and check that
it is a file, a directory, or a glob matching at least one file.

Some paths are deliberately cited although absent, because a document is *recording* a dangling
reference or rejecting one.  Those are listed, with their reasons, in
``tools/docs/allow-missing.txt``.

What it cannot do: judge whether the cited path still *describes* what the citing document says it
does.  It proves the target exists, not that the sentence about it is still true.

Exit 0 when every reference resolves; 1 when any does not (or the scan found implausibly little,
which usually means the extractor broke and the check would pass vacuously).
"""

import argparse
import glob
import re
import sys
from pathlib import Path

# Directories that hold generated, vendored or upstream content, never our prose.
SKIP_DIRS = {"build", "upstream", ".git", ".venv", "venv", "node_modules", "third_party"}
# A repo-relative `docs/...` reference: it may not be mid-word (so `tools/docs/x.py` is not a match
# for the `docs/x.py` inside it) nor preceded by a slash.
REF = re.compile(r"(?<![A-Za-z0-9_/])docs/[A-Za-z0-9_./*-]+")
STRIP = ".,);:\"'"
DEFAULT_MIN_REFS = 50


def blank_fences(text):
    """Replace the interior of fenced code blocks with blank lines, keeping line numbers.

    A path inside a ``` fence is part of a transcript or a command -- most often a demonstration of a
    path that *does not* resolve -- not a live citation.  Blanking the interior rather than deleting it
    keeps every position and line number aligned with the original file.
    """
    out, in_fence = [], False
    for line in text.splitlines(keepends=True):
        if line.lstrip().startswith("```"):
            in_fence = not in_fence
            out.append("\n")
        else:
            out.append("\n" if in_fence else line)
    return "".join(out)


def find_refs(text):
    """Yield (line_number, token) for every docs/... reference outside a fenced code block.

    Line numbers are counted on the fence-blanked text, which keeps one newline per original line, so
    they match the file a reader would open.
    """
    blanked = blank_fences(text)
    for m in REF.finditer(blanked):
        tok = m.group(0).rstrip(STRIP)
        if tok:
            yield blanked.count("\n", 0, m.start()) + 1, tok


def load_allow(path):
    allow = set()
    if path and Path(path).exists():
        for line in Path(path).read_text(encoding="utf-8").splitlines():
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            allow.add(line)
            allow.add(line.rstrip("/"))
    return allow


def resolve(root, tok):
    """True if ``tok`` names an existing file or directory, or a glob that matches one."""
    t = tok.rstrip("/")
    if (root / t).exists():
        return True
    if any(ch in tok for ch in "*?["):
        return bool(glob.glob(str(root / tok))) or bool(glob.glob(str(root / t)))
    return False


def scan(root, skip_dirs=SKIP_DIRS):
    files = []
    for p in sorted(root.rglob("*.md")):
        if set(p.relative_to(root).parts[:-1]) & skip_dirs:
            continue
        files.append(p)
    return files


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--root", default=".")
    ap.add_argument("--allow", default="tools/docs/allow-missing.txt")
    ap.add_argument("--min-refs", type=int, default=DEFAULT_MIN_REFS,
                    help="fail if fewer than this many references are found (guards a broken scan)")
    args = ap.parse_args(argv)

    root = Path(args.root).resolve()
    allow_path = Path(args.allow)
    if not allow_path.is_absolute():
        allow_path = root / allow_path
    allow = load_allow(allow_path)

    files = scan(root)
    failures = []
    total = 0
    for f in files:
        text = f.read_text(encoding="utf-8", errors="replace")
        rel = f.relative_to(root)
        for ln, tok in find_refs(text):
            if tok in allow or tok.rstrip("/") in allow:
                continue
            total += 1
            if not resolve(root, tok):
                failures.append((str(rel), ln, tok))

    if total < args.min_refs:
        print(f"FAIL  the scan found only {total} docs/ references (expected at least "
              f"{args.min_refs}); the extractor is probably broken", file=sys.stderr)
        return 1
    if failures:
        for rel, ln, tok in failures:
            print(f"FAIL  {rel}:{ln}: cites `{tok}`, which does not exist", file=sys.stderr)
        print(f"{len(failures)} dangling docs/ reference(s) of {total}", file=sys.stderr)
        return 1
    print(f"    {len(files)} markdown files, {total} docs/ references, all resolve")
    return 0


if __name__ == "__main__":
    sys.exit(main())
