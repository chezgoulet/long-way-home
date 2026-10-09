#!/usr/bin/env bash
# The meeting overlay (docs/staff-meetings.md, phase three / M3): the surface a meeting is attended
# on. The queue is reachable in play, a screen key opens a brief, a pill resolves to exactly one
# enumerated outcome that the simulation applies with the player's provenance, and typed text goes to
# the novelty seam and never to a branch. No model is called and no audio is built.
#
#   scripts/meeting-overlay-check.sh [--map NAME]
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
SHOT="$GAME_DIR/screenshots/lwh_meeting.tga"
SHOTQ="$GAME_DIR/screenshots/lwh_meeting_queue.tga"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }
mkdir -p "$OUT" "$GAME_DIR/screenshots"
rm -f "${OUT:?}/meeting-overlay.txt" "${SHOT:?}" "${SHOTQ:?}"

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> the meeting overlay (g_shipTest 82)"
SDL_AUDIODRIVER=dummy timeout 200 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipMode 1 +set g_shipTest 82 +map "$MAP" \
    >"$HOME_DIR/meeting-overlay.out" 2>&1 || true

grep -h '^SHIP: overlay test' "$HOME_DIR/meeting-overlay.out" | sed 's/^SHIP: /    /' || true
grep -h '^SHIP: meeting ' "$HOME_DIR/meeting-overlay.out" | sed 's/^SHIP: /    /' || true

fail() { echo "FAIL  $1"; exit 1; }

# The queue is reachable in play and lists the pending meetings.
grep -q 'overlay test: the queue is reachable in play and lists 1 pending meeting(s)' "$HOME_DIR/meeting-overlay.out" \
  || fail "the meeting queue was not reachable in play"
grep -q 'overlay test:   allocation: where the ship.s power goes' "$HOME_DIR/meeting-overlay.out" \
  || fail "the queue did not list the allocation meeting"

# A screen key opened the brief: the room shows its speaker, its line and its delivery, and the pills.
grep -q 'overlay test: meeting open=1, kind allocation' "$HOME_DIR/meeting-overlay.out" \
  || fail "no brief was opened into the room"
grep -q 'delivery .* (carried, not rendered)' "$HOME_DIR/meeting-overlay.out" \
  || fail "the line being answered did not carry its delivery direction"
grep -q 'overlay test: pill 4: The holodecks full, the shields dark' "$HOME_DIR/meeting-overlay.out" \
  || fail "the option pills were not published with their labels and costs"

# A pill resolved to exactly one enumerated outcome, and the simulation applied it with a person's
# provenance -- not automatic mode.
grep -q 'meeting choose: allocation outcome 3 applied' "$HOME_DIR/meeting-overlay.out" \
  || fail "the pill did not route to exactly one enumerated outcome"
grep -q 'after the pill, holodecks 100% shields 0% provenance the player' "$HOME_DIR/meeting-overlay.out" \
  || fail "the ship did not apply the allocation with the player's provenance"

# Typed text: the same input twice, at the novelty seam, and the allocation cannot move.
[ "$(grep -c '"Make it so": no branch matched (novel)' "$HOME_DIR/meeting-overlay.out")" -eq 2 ] \
  || fail "the same typed input twice did not reach the novelty seam both times"
grep -q 'overlay test: allocation unchanged by the typed text: 100 (was 100)' "$HOME_DIR/meeting-overlay.out" \
  || fail "typed text changed ship state"

# The meeting reached its end (the decision and the record) and the screen was photographed.
[ -f "$OUT/meeting-overlay.txt" ] || fail "the meeting did not reach its end"
[ -f "$SHOT" ] || fail "no screenshot of the room"
[ -f "$SHOTQ" ] || fail "no screenshot of the queue"
echo "PASS  the queue is reachable in play; a screen key opened a brief; a pill applied one outcome with the player's provenance;"
echo "      typed text reached the novelty seam and changed nothing; the meeting reached its end"
echo "      screenshots: $SHOTQ, $SHOT"
