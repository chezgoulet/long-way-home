#!/usr/bin/env bash
# The ship's computer, as somewhere to dive in (docs/staff-meetings.md, the depth pass). One headless
# engine run demonstrates the four tasks: a session on the meeting's own overlay; answers scoped by the
# records' own rule (PersonalVisibleTo, LogScopeForCrew); the API reachable by asking and applied by the
# simulation; the three refusals distinct; and no model call. No second UI is built.
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
SHOTS="$GAME_DIR/screenshots/lwh_computer_session.tga"
SHOTR="$GAME_DIR/screenshots/lwh_computer_refused.tga"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }
mkdir -p "$OUT" "$GAME_DIR/screenshots"
rm -f "${OUT:?}/computer.txt" "${SHOT:?}" "${SHOTS:?}" "${SHOTR:?}"
rm -rf "$GAME_DIR/ship/meetings"

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> the ship's computer (g_shipTest 85)"
SDL_AUDIODRIVER=dummy timeout 300 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipMode 1 +set g_shipTest 85 +map "$MAP" \
    >"$HOME_DIR/computer.out" 2>&1 || true

grep -h '^SHIP: computer' "$HOME_DIR/computer.out" | sed 's/^SHIP: /    /' || true

fail() { echo "FAIL  $1"; exit 1; }

# The API is enumerable, the room is the meeting's own, and the pills are the API's verbs.
grep -q 'computer test: the ship.s API enumerates [1-9][0-9]* verb(s); the computer.s voice is computer' "$HOME_DIR/computer.out" \
  || fail "the ship's API was not enumerable, or the computer had no voice of its own"
grep -q 'computer test: the room is the meeting.s own; kind computer, options 4' "$HOME_DIR/computer.out" \
  || fail "the computer did not use the meeting's own room"
grep -q 'computer test: pill 1: Ship.s status -> status' "$HOME_DIR/computer.out" \
  || fail "the pills were not the API's verbs"
grep -q 'computer test: pill 4: The job queue -> jobs' "$HOME_DIR/computer.out" \
  || fail "the pills were not the pass's knowledge topics"
grep -q 'computer test: the computer is not crew: [0-9]* pool voices, and it is none of them' "$HOME_DIR/computer.out" \
  || fail "the computer was cast from the pool rather than given its own voice"
grep -q 'computer test: pre-generated during generation: 4 line(s), voice computer, key [0-9a-f]*, inference calls=0' "$HOME_DIR/computer.out" \
  || fail "the computer's lines were not pre-generated under its own voice with no inference"

# Task A -- a session, not a shot: three exchanges in one sitting, and the room stays open.
grep -q 'computer test: session: 3 exchange(s) in one sitting' "$HOME_DIR/computer.out" \
  || fail "the computer did not hold a session of three exchanges"
grep -q 'computer test:   ask "status" -> alert' "$HOME_DIR/computer.out" \
  || fail "the session's first exchange was not answered from the ship's state"
grep -q 'computer test:   ask "jobs" ->' "$HOME_DIR/computer.out" \
  || fail "the session's second exchange was not answered"
grep -q 'computer test:   ask "chart" ->' "$HOME_DIR/computer.out" \
  || fail "the session's third exchange was not answered"
grep -q 'computer test: the room is still open after 3 exchange(s): yes' "$HOME_DIR/computer.out" \
  || fail "the room ended when the first answer landed"

# Task B -- the scope by contrast: the same question, two ranks. The junior is refused in character;
# the captain, who owns the private log, reaches it. The official log's scope differs the same way.
grep -q 'computer test: the junior asks "personal Janeway" -> not yours: that is Kathryn Janeway.s private log' "$HOME_DIR/computer.out" \
  || fail "the junior was not refused the captain's private log in character"
grep -q 'computer test: the captain asks "personal Janeway" -> answered: your private log' "$HOME_DIR/computer.out" \
  || fail "the captain did not reach his own private log"
grep -q 'computer test: the captain reads the log -> the official log (all scopes)' "$HOME_DIR/computer.out" \
  || fail "command did not read all log scopes"
grep -q 'computer test: the junior reads the same log -> the official log (command)' "$HOME_DIR/computer.out" \
  || fail "a junior post did not read its own log scope"

# Task C -- the API through the computer, and the three refusals, distinct and named.
grep -q 'computer test: out of set "warpnine" -> not a function: that function is not available' "$HOME_DIR/computer.out" \
  || fail "an out-of-set request was not refused as a missing function"
grep -q 'computer test: the ship.s state is byte-identical across the refusal: yes' "$HOME_DIR/computer.out" \
  || fail "the refusal was not shown to write nothing"
grep -q 'computer test: the junior asks "alert red" -> not cleared:' "$HOME_DIR/computer.out" \
  || fail "an in-set request beyond clearance was not refused as a permission"
grep -q 'computer test: the alert is unchanged at index 0 (the permission changed nothing)' "$HOME_DIR/computer.out" \
  || fail "the permission refusal changed ship state"
grep -q 'computer test: the captain asks "alert yellow" -> answered: the alert is set to yellow; the alert is now index 1' "$HOME_DIR/computer.out" \
  || fail "a recognised request was not applied by the simulation"
grep -q 'computer test: log \[command\] the computer: the computer refused (not yours): personal Janeway' "$HOME_DIR/computer.out" \
  || fail "the scope refusal was not in the record"
grep -q 'computer test: log \[command\] the computer: the computer applied: alert yellow' "$HOME_DIR/computer.out" \
  || fail "the applied request was not in the record"

# Task A / B -- the wrap. Every live string is measured, in the engine's own font, against the column
# it is drawn in -- the transcript, the rail's echo of it, the pills, the queue, the rail's facts. A
# line wider than its column is an OVERFLOW and this check fails on it. Nothing is clipped to fit.
grep -h '^WRAP:' "$HOME_DIR/computer.out" | sed 's/^/    /' || true
grep -qE '^WRAP: checked [1-9][0-9]* string\(s\), [1-9][0-9]* line\(s\), overflows 0$' "$HOME_DIR/computer.out" \
  || fail "the wrap check did not run, or a line overflowed its column (see $HOME_DIR/computer.out)"
if grep -q '^WRAP: OVERFLOW' "$HOME_DIR/computer.out"; then fail "a wrapped line overflowed its column"; fi
# The refusal reads whole: its own end is a wrapped line of both the transcript and the rail's echo.
[ "$(grep -ci '^WRAP:   | .*or by command' "$HOME_DIR/computer.out")" -ge 2 ] \
  || fail "the refusal's own end is not a wrapped line of both the transcript and the rail's echo"
grep -q '^WRAP: rail-echo ' "$HOME_DIR/computer.out" \
  || fail "the wrap check did not measure the rail's echo"

# The model calls nothing, and the classification path is the API's membership test.
grep -q 'computer test: the classification path is the API.s membership test (FindConsoleVerb), not the embedding seam; model calls=0' "$HOME_DIR/computer.out" \
  || fail "the classification path was not the membership test, or a model was called"
grep -q 'computer test: addressed [1-9][0-9]* time(s), inference calls=0; nothing invented: yes' "$HOME_DIR/computer.out" \
  || fail "the computer's call count or the nothing-invented property failed"

# No second UI: the computer is drawn by the one meeting file.
[ "$(find "$ROOT/module/ui" -maxdepth 1 -name 'ui_lwh_meeting*.cpp' | wc -l)" -eq 1 ] \
  || fail "the computer has a second UI file"

[ -f "$SHOT" ] || fail "no screenshot of the computer on the meeting overlay"
[ -f "$SHOTS" ] || fail "no screenshot of the session (three exchanges on the overlay)"
[ -f "$SHOTR" ] || fail "no screenshot of the refusal on the meeting overlay"
echo "PASS  the computer is a session on the meeting's own overlay: three exchanges in one sitting with the"
echo "      room still open, answered from the ship's state and scoped by PersonalVisibleTo and LogScopeForCrew;"
echo "      a recognised request applied by the simulation; three refusals distinct and named -- not a function,"
echo "      not cleared, not yours; the classification is the API's membership test and the model calls nothing."
echo "      No second UI: module/ui/ui_lwh_meeting.cpp."
echo "      screenshots: $SHOT, $SHOTS, $SHOTR"
