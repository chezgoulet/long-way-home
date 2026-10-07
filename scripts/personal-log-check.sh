#!/usr/bin/env bash
# The personal log screen (docs/the-record-and-the-log.md, Task B). The store exists and the rule is
# tested; this drives the screen: the player writes a private entry, the screen reads it, the official
# read of every scope does not, and another person's read is empty. The privacy is the model's
# (PersonalLog returns one owner's entries), so what the screen can show is its owner's alone.
#
#   scripts/personal-log-check.sh
#
# One headless engine run. Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
OUT="$HOME_DIR/personallog.out"
SHOT="$GAME_DIR/screenshots/lwh_personal.tga"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

fail() { echo "FAIL  $1"; exit 1; }
pass() { echo "PASS  $1"; }

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> the personal log screen"
SDL_AUDIODRIVER=dummy timeout 180 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipTest 65 +map tour/deck04 >"$OUT" 2>&1 || true

grep -h '^SHIP: personal test' "$OUT" | sed 's/^SHIP: /    /' || true

grep -q 'personal test: Reyes wrote a private entry; the owner.s read has 1' "$OUT" \
  || fail "the player's own read did not carry the private entry (see $OUT)"
pass "the owner's own read carries the private entry"

grep -q 'personal test: the private screen reads ".*I did not put this in the report.*" for Reyes' "$OUT" \
  || fail "the personal log screen did not read the owner's entry"
pass "the personal log screen reads the owner's own words"

grep -q 'personal test: official read (all scopes) contains it: no' "$OUT" \
  || fail "the private entry leaked into the official log"
pass "the official log, every scope, does not carry it -- the two stores are separate"

grep -q "personal test: another person's read has 0" "$OUT" \
  || fail "another person's read carried the private entry"
pass "no other person's read returns it -- the visibility rule is the model's, not the screen's"

[ -f "$SHOT" ] || fail "no screenshot of the personal log screen"
echo "      screenshot: $SHOT"
echo
echo "PASS  the personal log screen exists, is private, and reads only its owner's store"
