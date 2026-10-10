#!/usr/bin/env bash
# The meeting's place (docs/staff-meetings.md, the owner's ruling 2026-10-09): a meeting happens
# *somewhere*, and the room belongs to the brief, derived from the meeting's kind and its subject.
#
#   scripts/meeting-place-check.sh
#
# Part one -- the table. tools/meetings/rooms.py checks module/ship/meeting_rooms.def against the kind
# enumeration in module/ship/ship_core.h and fails when a kind of meeting has no room at all. (Planted
# failure, and the fix, in docs/evidence/the-meeting-place.md.)
#
# Part two -- the screen. One headless engine run (g_shipTest 91) on the merged ship: a security matter
# calls its meeting before the player arrives; the player walks into the security office on deck 6; the
# room opens there with the security chief present; the frame is photographed. Needs the merged ship
# (scripts/build-ship.sh) and xvfb-run.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

echo "==> the meeting place: every kind of meeting has a room (the table)"
python3 tools/meetings/rooms.py check

HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
OUT="$GAME_DIR/ship"
SHOT="$GAME_DIR/screenshots/lwh_meeting_place.tga"
SHOT_A="$GAME_DIR/screenshots/lwh_meeting_place_a.tga"
SHOT_B="$GAME_DIR/screenshots/lwh_meeting_place_b.tga"
PAK="$ROOT/build/ship/out/longway_voyager.pk3"
[ -f "$PAK" ] || { echo "no merged ship at $PAK -- run scripts/build-ship.sh" >&2; exit 1; }
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

fail() { echo "FAIL  $1"; exit 1; }

mkdir -p "$OUT" "$GAME_DIR/screenshots"
cp "$PAK" "$GAME_DIR/"
rm -f "${OUT:?}/meeting-place.txt" "${SHOT:?}" "${SHOT_A:?}" "${SHOT_B:?}"

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo
echo "==> the meeting's place on the screen (g_shipTest 91)"
SDL_AUDIODRIVER=dummy timeout 260 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipMode 1 +set g_shipDeckPitch 3072 +set g_shipTest 91 \
    +map voyager >"$HOME_DIR/meeting-place.out" 2>&1 || true

grep -h '^SHIP: meeting place' "$HOME_DIR/meeting-place.out" | sed 's/^SHIP: /    /' || true

# Two kinds, two rooms: a command decision names the briefing room, a security matter the security
# office; an engineering matter names main engineering. The place is derived from the kind and subject.
grep -q '^SHIP: meeting place: watch-change (subject command) -> the briefing room, deck 1' "$HOME_DIR/meeting-place.out" \
  || fail "a command decision did not name the briefing room"
grep -q '^SHIP: meeting place: Borg (subject security) -> the security office, deck 6' "$HOME_DIR/meeting-place.out" \
  || fail "a security matter did not name the security office"
grep -q '^SHIP: meeting place: allocation (subject engineering) -> main engineering, deck 11' "$HOME_DIR/meeting-place.out" \
  || fail "an engineering matter did not name main engineering"

# The place is on the brief before the player arrives: the queue names the room at generation time.
grep -q '^SHIP: meeting place: 1 brief queued before the player arrives' "$HOME_DIR/meeting-place.out" \
  || fail "the security brief was not queued before the player arrived"
grep -q '^SHIP: meeting place:   Borg in the security office (deck 6), present ' "$HOME_DIR/meeting-place.out" \
  || fail "the queued brief did not carry its room"

# The player is in the named room, and the room opens there -- not wherever the player stood.
grep -q '^SHIP: meeting place: at the security office: standing at ' "$HOME_DIR/meeting-place.out" \
  || fail "the player did not stand in the security office"
grep -q '^SHIP: meeting place: the room is the security office (deck 6); kind Borg' "$HOME_DIR/meeting-place.out" \
  || fail "the opened room did not name the security office"
grep -q '^SHIP: meeting place: present .* (security)' "$HOME_DIR/meeting-place.out" \
  || fail "the security chief was not present in the security office"

[ -f "$OUT/meeting-place.txt" ] || fail "the meeting did not reach its end"
[ -f "$SHOT" ] || fail "no screenshot of the player at the meeting in the named room"
[ -f "$SHOT_A" ] || fail "no screenshot of the named room (angle a)"
[ -f "$SHOT_B" ] || fail "no screenshot of the named room (angle b)"
echo "PASS  every kind of meeting has a room; a command decision named the briefing room and a security"
echo "      matter the security office with the chief present; the room was on the brief before the"
echo "      player arrived; the player stood in the named room and the frame is photographed"
echo "      screenshot: $SHOT (the place named on the overlay), $SHOT_A and $SHOT_B (the room itself)"
