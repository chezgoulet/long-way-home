#!/usr/bin/env bash
# The meeting (docs/staff-meetings.md, phase one: the brief, the skeleton, and the seams): the
# simulation emits a brief at the normal-case trigger -- the watch change -- and a person in the room
# sets an allocation that the simulation applies, while the meeting is refused when it tries to set
# the same allocation as the ship (automatic mode is the ship's own answer). Every line carries its
# delivery to the synthesis seam. No model is called and no audio is built.
#
#   scripts/meeting-check.sh [--map NAME]
#
# One headless engine run. Needs the game data (scripts/playtest-host-setup.sh) and xvfb-run.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MAP="tour/deck04"
while [ $# -gt 0 ]; do
  case "$1" in
    --map) MAP="$2"; shift 2 ;;
    *) echo "unknown argument: $1" >&2; exit 2 ;;
  esac
done

HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
OUT="$GAME_DIR/ship"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }
mkdir -p "$OUT" "$GAME_DIR/screenshots"
rm -f "${OUT:?}/meeting.txt"

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> the meeting (g_shipTest 80)"
SDL_AUDIODRIVER=dummy timeout 180 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipMode 1 +set g_shipTest 80 +map "$MAP" \
    >"$HOME_DIR/meeting.out" 2>&1 || true

grep -h '^SHIP: meeting test' "$HOME_DIR/meeting.out" | sed 's/^/    /' || true

fail() { echo "FAIL  $1"; exit 1; }
[ -f "$OUT/meeting.txt" ] || fail "the meeting run did not finish: the game stopped before the decision"

# Every meeting generates a brief, in the normal case (the emit site, not a counter).
grep -q 'brief(s) queued at the watch change' "$HOME_DIR/meeting.out" \
  || fail "no brief was emitted at the watch change"
grep -q 'SHIP: meeting test: watch-change:' "$HOME_DIR/meeting.out" \
  || fail "the emitted brief was not the ordinary watch-change brief"

# Every line carries its delivery, and the seam carries it with the text.
grep -q 'line -> synthesis: delivery .*, known 1, exaggeration' "$HOME_DIR/meeting.out" \
  || fail "the synthesis seam did not carry a delivery direction with the line"

# A person in the room sets the allocation, end to end, and it is theirs.
grep -q 'allocation outcome applied=1; holodecks 100%, shields 0% (provenance the player)' "$HOME_DIR/meeting.out" \
  || fail "a person's allocation outcome was not applied end to end"
grep -q 'holodecks .* alloc 100%' "$OUT/meeting.txt" || fail "holodecks were not set to 100% in the ship's state"
grep -q 'shields .* alloc   0%' "$OUT/meeting.txt" || fail "shields were not set to 0% in the ship's state"

# The meeting does not get to override a person: set as the ship, it is refused.
grep -q 'set as the ship (automatic) applied=0' "$HOME_DIR/meeting.out" \
  || fail "the meeting was allowed to set an allocation as the ship"
echo "PASS  a brief is emitted at the watch change; a person set an allocation end to end; the ship's own set was refused"
