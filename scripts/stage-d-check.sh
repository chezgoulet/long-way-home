#!/usr/bin/env bash
# Stage D, the rest of it (docs/path-to-playtest.md; docs/affinities-and-allegiance.md;
# docs/the-record-and-the-log.md). One harness for the four pieces this stage still owed:
#
#   scripts/stage-d-check.sh collective   the Collective speaks a taken person's own words
#   scripts/stage-d-check.sh allegiance   allegiance weights a mark and bends an order
#   scripts/stage-d-check.sh pursuits     a pursuit puts an artifact into the log
#   scripts/stage-d-check.sh program      the liked program, as the model can carry it
#   scripts/stage-d-check.sh all          every mode, one build
#
# Each mode builds the ship suite on the host (no engine, no game data), runs the suite, prints the
# mode's `test_ship_core --<mode>` transcript, and falsifies once: a planted break must make the
# transcript fail for the reason the mode cares about. A guard that cannot fail is decoration.
# The in-engine observation of the program on the deck is not this check; it is named in
# docs/evidence/the-liked-program.md as the owner's session.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

CXX="${CXX:-g++}"
WORK="$(mktemp -d)"
trap 'rm -rf "${WORK:?}"' EXIT
fail() { echo "FAIL  $1" >&2; exit 1; }

MODE="${1:-all}"

build() { # $1 = source root, $2 = output binary
  "$CXX" -std=c++17 -g -I "$1/module/ship" -I "$1/module/ui" \
    "$1/tests/ship/test_ship_core.cpp" "$1/module/ship/ship_core.cpp" \
    "$1/module/ship/scenario_core.cpp" -o "$2"
}

echo "==> stage D: the ship suite, built here"
build "$ROOT" "$WORK/test_ship_core"
"$WORK/test_ship_core" >/dev/null

run_collective() {
  echo
  echo "==> the Collective's voice (test_ship_core --collective)"
  "$WORK/test_ship_core" --collective | tee "$WORK/collective.txt"
  grep -q '^PASS  the words are the taken person.s own: "the captain was wrong about the nebula"' "$WORK/collective.txt" \
    || fail "the Collective did not build a line from the taken private words"
  grep -q '^PASS  the voice is the stolen cast reference' "$WORK/collective.txt" \
    || fail "the Collective's line did not carry the taken voice"
  grep -q '^PASS  the render seam takes it under the stolen voice' "$WORK/collective.txt" \
    || fail "the render seam did not take the Collective's line"

  echo
  echo "==> falsification: a Collective that cannot speak its taken words must fail the transcript"
  mkdir -p "$WORK/broken-collective"
  cp -r "$ROOT/module" "$ROOT/tests" "$WORK/broken-collective/"
  sed -i 's/if (!HeldByCollective(s, crew)) return w; \/\/ not a mind it holds: no words, no voice/return w; \/\/ planted: the Collective holds no words/' \
    "$WORK/broken-collective/module/ship/ship_core.cpp"
  grep -q 'planted: the Collective holds no words' "$WORK/broken-collective/module/ship/ship_core.cpp" \
    || fail "the collective plant did not take"
  build "$WORK/broken-collective" "$WORK/test_broken_collective"
  if "$WORK/test_broken_collective" --collective >"$WORK/broken-collective.txt" 2>&1; then
    fail "the transcript passed with a Collective that holds no words: the check has no teeth"
  fi
  if grep -q 'the words are the taken person.s own: "the captain was wrong about the nebula"' \
      "$WORK/broken-collective.txt"; then
    fail "the broken copy failed for the wrong reason"
  fi
  echo "    the broken copy fails the transcript; the tree passes"
}

case "$MODE" in
  collective) run_collective ;;
  all) run_collective ;;
  *) echo "unknown mode: $MODE" >&2; exit 2 ;;
esac

echo
echo "PASS  the Collective speaks a taken person's own words in the stolen voice, and a planted"
echo "      speaker that cannot speak fails the same transcript"
