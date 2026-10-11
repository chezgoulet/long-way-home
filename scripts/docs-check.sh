#!/usr/bin/env bash
# The documentation freshness check, in the three classes that are mechanically verifiable.
#
#   class 1  a cited `docs/...` path that does not resolve          (tools/docs/check_paths.py)
#   class 2  a registered owned number contradicted by its owner    (tools/docs/check_claims.py)
#   class 3  a register row still claiming unbuilt, with proof      (tools/docs/check_register.py)
#
# What it cannot catch is a *description* that has gone stale -- a section that says a thing is
# unbuilt when it was built last week.  That is a person's read; the classes here are the ones a
# script can decide.  See docs/evidence/docs-consistency-check.md and
# docs/evidence/the-number-owners.md for the demonstrations (a planted error fails, removing it
# passes).
#
#   scripts/docs-check.sh

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

echo "==> docs: cited paths resolve"
python3 tools/docs/check_paths.py

echo
echo "==> docs: owned numbers agree with their owner"
python3 tools/docs/check_claims.py

echo
echo "==> docs: register rows agree with the tree"
python3 tools/docs/check_register.py \
    --register docs/omissions.md \
    --register docs/walkthrough.md \
    --register docs/gates.md \
    --register docs/hook-triage.md \
    --register docs/designs-against-code.md
