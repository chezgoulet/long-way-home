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
echo "==> holdings: a fold over the marks, and no stored field (C2)"
"$ROOT/scripts/holdings-check.sh"

echo
echo "==> assimilation: the log and the access transfer to the Collective"
"$ROOT/scripts/assimilation-check.sh"

echo
echo "==> bills: the watch bill and the reconfiguration bills (O6)"
"$ROOT/scripts/bills-check.sh"

echo
echo "==> ood: the deck's authority binds, and its refusal names its ground (C-11)"
"$ROOT/scripts/ood-check.sh"

echo
echo "==> shields: canon's six arcs, and damage routes to one (C-14)"
"$ROOT/scripts/shields-check.sh"

echo
echo "==> departments: the nine, and the complement as hull data (3.3, 8.53)"
"$ROOT/scripts/departments-check.sh"

echo
echo "==> damage control: the three tiers, severity scaling, and the post the large job empties (C-12)"
"$ROOT/scripts/damage-control-check.sh"

echo
echo "==> scenario: the declaration contract -- fields, the two laws, and the loader (O15)"
"$ROOT/scripts/scenario-check.sh"

echo
echo "==> evaluator: the breeding conditions decide, both ways, and it recruits (G4)"
"$ROOT/scripts/evaluator-check.sh"

echo
echo "==> maintenance: the seventh job kind, its axis (wear), and the odds it moves (3.23)"
"$ROOT/scripts/maintenance-check.sh"

echo
echo "==> manner: the crew's state is audible and visible, and degrades without audio (G14)"
"$ROOT/scripts/manner-check.sh"

echo
echo "==> api: the console's verbs and the ship's API agree, both directions"
"$ROOT/scripts/api-check.sh"

echo
echo "==> orders: the register and the dispatch agree, both directions"
python3 "$ROOT/tools/orders/orders.py" check

echo
echo "==> harness: the g_shipTest case numbers are distinct"
"$ROOT/scripts/harness-check.sh"

echo
echo "==> docs: cited paths resolve, and owned numbers agree"
"$ROOT/scripts/docs-check.sh"

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
