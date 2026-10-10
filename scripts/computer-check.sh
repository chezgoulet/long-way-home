#!/usr/bin/env bash
# The ship's computer (docs/staff-meetings.md, M6): the enumerated set as the ship's API, the same
# overlay a meeting uses, the computer's own voice, and the refusal -- an input outside the API is
# refused in character with the ship's state byte-identical, and nothing is invented.
#
#   scripts/computer-check.sh [--map NAME]
#
# One headless engine run. Needs the game data (scripts/playtest-host-setup.sh) and xvfb-run. The
# enumeration's own freshness check is scripts/api-check.sh (engine-free; in scripts/test.sh).

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
SHOT="$GAME_DIR/screenshots/lwh_computer.tga"
SHOTR="$GAME_DIR/screenshots/lwh_computer_refused.tga"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }
mkdir -p "$OUT" "$GAME_DIR/screenshots"
rm -f "${OUT:?}/computer.txt" "${SHOT:?}" "${SHOTR:?}"

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> the ship's computer (g_shipTest 85)"
SDL_AUDIODRIVER=dummy timeout 200 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipMode 1 +set g_shipTest 85 +map "$MAP" \
    >"$HOME_DIR/computer.out" 2>&1 || true

grep -h '^SHIP: computer' "$HOME_DIR/computer.out" | sed 's/^SHIP: /    /' || true

fail() { echo "FAIL  $1"; exit 1; }

# Task A: the API is enumerable, and the computer's pills are its verbs.
grep -q 'computer test: the ship.s API enumerates [1-9][0-9]* verb(s); the computer.s voice is computer' "$HOME_DIR/computer.out" \
  || fail "the ship's API was not enumerable, or the computer had no voice of its own"
grep -q 'computer test: the room is the meeting.s own; kind computer, options 6' "$HOME_DIR/computer.out" \
  || fail "the computer did not use the meeting's own room"
grep -q 'computer test: pill 1: Ship.s status -> status' "$HOME_DIR/computer.out" \
  || fail "the pills were not the API's verbs"
grep -q 'computer test: the computer is not crew: 14 pool voices, and it is none of them' "$HOME_DIR/computer.out" \
  || fail "the computer was cast from the pool rather than given its own voice"

# Task B/D: the lines are pre-generated during generation, keyed under the computer's own voice, with
# no inference call.
grep -q 'computer test: pre-generated during generation: 6 line(s), voice computer, key [0-9a-f]*, inference calls=0' "$HOME_DIR/computer.out" \
  || fail "the computer's lines were not pre-generated under its own voice with no inference"

# Task C: a recognised command is answered; an input outside the set is refused in character; the state
# is byte-identical across the refusal; and the log carries what was said.
grep -q 'computer test: free text "status" (in the API) -> Acknowledged. status.' "$HOME_DIR/computer.out" \
  || fail "a recognised command was not answered from the API"
grep -q 'computer test: free text "warpnine" (not in the API) -> that function is not available' "$HOME_DIR/computer.out" \
  || fail "an out-of-set command was not refused in character"
grep -q 'computer test: the ship.s state is byte-identical across the refusal: yes' "$HOME_DIR/computer.out" \
  || fail "the refusal was not shown to write nothing"
grep -q 'computer test: log \[command\] the computer: the computer refused: warpnine' "$HOME_DIR/computer.out" \
  || fail "the log did not carry the refusal"

# The pill path, and a replay that costs nothing.
grep -q 'computer test: pill 1 (status) -> Acknowledged. status.' "$HOME_DIR/computer.out" \
  || fail "the pill did not route through the same membership test"
grep -q 'computer test: the same command again -> Acknowledged. status. (the authored line; inference calls=0)' "$HOME_DIR/computer.out" \
  || fail "the replay was not the authored line with no inference"
grep -q 'computer test: addressed [1-9][0-9]* time(s), inference calls=0; nothing invented: yes' "$HOME_DIR/computer.out" \
  || fail "the computer's call count or the nothing-invented property failed"

[ -f "$SHOT" ] || fail "no screenshot of the computer on the meeting overlay"
[ -f "$SHOTR" ] || fail "no screenshot of the refusal on the meeting overlay"
echo "PASS  the ship's API is enumerable and the computer's pills are its verbs; the computer is answered on"
echo "      the meeting's own overlay with its own pre-generated voice; a recognised command is answered and"
echo "      an out-of-set command is refused in character with the state byte-identical and the log carrying"
echo "      it; a replay costs nothing. No second UI was built."
echo "      screenshots: $SHOT, $SHOTR"
