#!/usr/bin/env bash
# The documentation freshness check, in the two classes that are mechanically verifiable.
#
#   class 1  a cited `docs/...` path that does not resolve          (tools/docs/check_paths.py)
#   class 2  a registered owned number contradicted by its owner    (tools/docs/check_claims.py)
#
# What it cannot catch is a *description* that has gone stale -- a section that says a thing is
# unbuilt when it was built last week.  That is a person's read; the classes here are the ones a
# script can decide.  See docs/evidence/docs-consistency-check.md for the demonstration (a planted
# error fails, removing it passes).
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
