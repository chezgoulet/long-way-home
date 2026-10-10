#!/usr/bin/env bash
# Every room a meeting can happen in, in front of the camera (the dark-room brief, Task C).
#
#   scripts/room-survey-check.sh
#
# The owner's ruling (docs/evidence/the-meeting-place.md, 2026-10-10): a meeting's room that renders
# pitch black is a defect, not the look. The dark-room pass (docs/evidence/the-dark-room.md) found
# the security office's fault -- the player stood *inside* the composition's own geometry -- and
# fixed it in tools/shipmap/dressdeck06.py. The ruling is not about that room only: this stands the
# player in every room the meeting table names (module/ship/meeting_rooms.def), photographs each
# from four facings (g_shipTest 97), and measures the frames -- not the lights -- with
# tools/meetings/legibility.py, so a dark room is visible rather than asserted.
#
# Needs the merged ship (scripts/build-ship.sh) and xvfb-run. Writes under build/g3-home only.
#
# The script reports the verdict for every room; it *fails* only for the security office, which is
# the meeting's own room and the one the ruling is about. A dark room elsewhere is a finding for the
# owner (and would be fixed the same way), not a gate on this pass.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
PAK="$ROOT/build/ship/out/longway_voyager.pk3"
[ -f "$PAK" ] || { echo "no merged ship at $PAK -- run scripts/build-ship.sh" >&2; exit 1; }
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

fail() { echo "FAIL  $1"; exit 1; }

mkdir -p "$GAME_DIR/screenshots"
cp "$PAK" "$GAME_DIR/"
find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
rm -f "$GAME_DIR"/screenshots/lwh_room_*.tga

echo "==> every meeting room, in front of the camera (g_shipTest 97)"
SDL_AUDIODRIVER=dummy timeout 300 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipMode 1 +set g_shipDeckPitch 3072 +set g_shipTest 97 \
    +map voyager >"$HOME_DIR/room-survey.out" 2>&1 || true

grep -h '^SHIP: room survey: [a-z].* at ' "$HOME_DIR/room-survey.out" | sed 's/^SHIP: /    /' || true

for room in briefing-room ready-room bridge sickbay astrometrics engineering security-office; do
    for n in 0 1 2 3; do
        [ -f "$GAME_DIR/screenshots/lwh_room_${room}_${n}.tga" ] || fail "no frame of $room (facing $n)"
    done
done

echo
echo "==> the verdicts (the middle band of each frame; legible = a visible room and a lit feature)"
python3 "$ROOT/tools/meetings/legibility.py" "$GAME_DIR"/screenshots/lwh_room_*.tga

# The meeting's own room must read: the security office is what the ruling is about, and the meeting
# faces north (yaw 90), which is frame 1 of the four. A wall or a corner is allowed elsewhere; the
# frame the meeting itself is taken from must not be dark.
python3 "$ROOT/tools/meetings/legibility.py" --require-legible \
    "$GAME_DIR/screenshots/lwh_room_security-office_1.tga" \
    || fail "the security office renders dark where the meeting stands -- the ruling's defect"
echo "PASS  every meeting room was photographed; the security office reads; the verdicts are above,"
echo "      one line per frame, and a room is dark rather than claimed legible"
