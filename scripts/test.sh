#!/usr/bin/env bash
# Everything that can be checked without the game, the GDK or the upstream checkout.
#
#   scripts/test.sh
#
# Runs, in order: the crew direction layer's and the ship simulation's unit tests (compiled here),
# the Python tool tests,
# a syntax pass over every Python file, shellcheck over every script (when installed), and a
# check that the patch series is well-formed. This is what CI runs. The checks that need real
# content are separate: scripts/check.sh (G0, needs the GDK) and scripts/g3-measure.sh (G3,
# needs the game).

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
WORK="$(mktemp -d)"
trap 'rm -rf "${WORK:?}"' EXIT

echo "==> crew direction layer: unit tests"
cmake -S tests/crew -B "$WORK/crew" -DCMAKE_BUILD_TYPE=Debug >/dev/null
cmake --build "$WORK/crew" -j"$(nproc)" >/dev/null
"$WORK/crew/test_crew_core"

echo
echo "==> ship simulation: unit tests"
cmake -S tests/ship -B "$WORK/ship" -DCMAKE_BUILD_TYPE=Debug >/dev/null
cmake --build "$WORK/ship" -j"$(nproc)" >/dev/null
"$WORK/ship/test_ship_core"

echo
echo "==> tools: unit tests"
python3 -m unittest discover -s tests/tools -q

echo
echo "==> hooks: the register matches the public surface"
"$ROOT/scripts/hooks-check.sh"

echo
echo "==> python: syntax"
find tools tests -name '*.py' -print0 | PYTHONPYCACHEPREFIX="$WORK/pycache" xargs -0 python3 -m py_compile
echo "    ok"

echo
echo "==> shell: shellcheck"
if command -v shellcheck >/dev/null 2>&1; then
  find scripts tools -name '*.sh' -print0 | xargs -0 shellcheck --severity=warning
  echo "    ok"
else
  echo "    shellcheck not installed; skipped"
fi

echo
echo "==> patches: numbered without gaps, and each one parses"
n=0
for p in patches/*.patch; do
  n=$((n + 1))
  case "$(basename "$p")" in
    "$(printf '%04d' "$n")"-*) ;;
    *) echo "    $p is out of sequence (expected $(printf '%04d' "$n")-...)" >&2; exit 1 ;;
  esac
  git apply --stat "$p" >/dev/null || { echo "    $p does not parse as a patch" >&2; exit 1; }
done
echo "    $n patches"

echo
echo "all checks passed"
