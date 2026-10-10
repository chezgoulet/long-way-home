#!/usr/bin/env bash
# Every meeting room's post, in front of the camera (the room-posts brief, Tasks A and B).
#
#   scripts/room-survey-check.sh
#
# The owner's ruling (docs/evidence/the-meeting-place.md, 2026-10-10): a meeting's room that renders
# pitch black is a defect, not the look. The dark-room pass (docs/evidence/the-dark-room.md) found the
# security office's fault -- the player stood *inside* the composition's own geometry -- and fixed it
# in tools/shipmap/dressdeck06.py. It found something else too: the room survey stood at each room's
# own point and turned to four facings, and ten of those frames measured dark while the same room read
# from another facing. That is a *post* problem, so this pass gives every room one post -- a position
# and a facing -- the meeting would use, and this check asserts it: the frame each post produces
# (g_shipTest 97, screenshots/lwh_roompost_<room>.tga) is measured with tools/meetings/legibility.py,
# and a post that measures dark **fails and names the room**. A survey that prints a dark frame and
# passes is the defect this whole line of work came out of.
#
# LWH_ROOM_POST_ANCHOR="x y z" re-anchors the security office's post (the meeting's own room) through
# g_shipTestPos, the candidate tool g_shipTest 96 uses, so a post can be planted inside a wall and the
# assertion shown to catch it -- without rebuilding the map. An anchor the harness cannot honour (not
# three numbers, or with no world geometry beneath it) is now rejected loudly by the harness, and the
# check fails on the rejection rather than quietly standing at the table's own position (Task B of the
# room-reads brief; the trap is named in docs/evidence/the-room-posts.md). Demonstrated in
# docs/evidence/the-room-reads.md.
#
# Needs the merged ship (scripts/build-ship.sh) and xvfb-run. Writes under build/g3-home only.

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

# Every room the meeting table (module/ship/meeting_rooms.def) names, as the survey's own slug.
POSTS="briefing-room ready-room bridge sickbay astrometrics engineering security-office"

ANCHOR_ARGS=()
if [ -n "${LWH_ROOM_POST_ANCHOR:-}" ]; then
    echo "==> planted: the security office's post anchored at ${LWH_ROOM_POST_ANCHOR} (g_shipTestPos)"
    ANCHOR_ARGS=(+set g_shipTestPos "$LWH_ROOM_POST_ANCHOR")
fi

mkdir -p "$GAME_DIR/screenshots"
cp "$PAK" "$GAME_DIR/"
find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
rm -f "$GAME_DIR"/screenshots/lwh_roompost_*.tga

echo "==> every meeting room, stood at its post (g_shipTest 97)"
SDL_AUDIODRIVER=dummy timeout 300 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipMode 1 +set g_shipDeckPitch 3072 +set g_shipTest 97 \
    ${ANCHOR_ARGS[@]+"${ANCHOR_ARGS[@]}"} +map voyager >"$HOME_DIR/room-survey.out" 2>&1 || true

grep -h '^SHIP: room post' "$HOME_DIR/room-survey.out" | sed 's/^SHIP: /    /' || true

# The anchor must not be silently ignored (the room-reads brief, Task B). If LWH_ROOM_POST_ANCHOR was
# given, the harness must either reject it loudly (a bad or out-of-bounds anchor) or print that it
# honoured it; an anchor that is neither is the silent no-op this check exists to close.
if [ -n "${LWH_ROOM_POST_ANCHOR:-}" ]; then
    if grep -qE 'BAD ANCHOR|ANCHOR OUT OF BOUNDS' "$HOME_DIR/room-survey.out"; then
        grep -hE 'BAD ANCHOR|ANCHOR OUT OF BOUNDS' "$HOME_DIR/room-survey.out" | sed 's/^SHIP: /    /'
        fail "the planted anchor was rejected by the harness (above); the survey did not run"
    fi
    grep -q 'room post: the security office anchored at .* by g_shipTestPos' "$HOME_DIR/room-survey.out" \
        || fail "an anchor was given (LWH_ROOM_POST_ANCHOR) but the harness did not honour it -- a silently ignored anchor is the trap this check closes"
fi

echo
echo "==> the verdicts (the middle band of each frame; legible = a visible room and a lit feature)"
for room in $POSTS; do
    f="$GAME_DIR/screenshots/lwh_roompost_${room}.tga"
    [ -f "$f" ] || fail "no frame of the $room post"
    python3 "$ROOT/tools/meetings/legibility.py" "$f"
done

echo
echo "==> every meeting room's post must read (the assertion this pass adds)"
for room in $POSTS; do
    python3 "$ROOT/tools/meetings/legibility.py" --require-legible \
        "$GAME_DIR/screenshots/lwh_roompost_${room}.tga" >/dev/null \
        || fail "the $room post renders dark -- the room a meeting would use has no legible post"
done
echo "PASS  every meeting room was stood at one post and photographed; every post measures legible;"
echo "      the frame and its numbers are above, one line per room"
