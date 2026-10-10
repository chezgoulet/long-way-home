#!/usr/bin/env bash
# Orders -- the third use of the same machine (docs/orders.md).
#
# Part one -- the register. tools/orders/orders.py checks module/ship/order_set.def against the
# dispatch in module/ship/ship_core.cpp and fails when the two drift in either direction: an order
# class the register names that the dispatch does not handle, or a case the dispatch handles that the
# register does not name. (Planted failures, and the fix, in docs/evidence/orders.md and
# tests/tools/test_order_set.py.)
#
# Part two -- the run. One headless engine run (g_shipTest 92) proves the set is derived per
# recipient and checked both ways, an order is dispatched as a job, an unauthorised order is refused
# by name with the state unchanged, the bespoke path refuses what falls outside the set, the
# recipient has a voice and an async plan, and a save replays. A second run (g_shipTest 93) opens the
# order on the screen and photographs the order being given and the recipient's answer -- the owner's
# judgement is whether being ordered reads as a moment rather than a menu.
#
#   scripts/order-check.sh [--map NAME]
#
# One/two headless engine runs. Needs the game data (scripts/playtest-host-setup.sh) and xvfb-run.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

echo "==> orders: the register and the dispatch agree, both directions"
python3 tools/orders/orders.py check

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
SHOT_GIVE="$GAME_DIR/screenshots/lwh_order_give.tga"
SHOT_ANSWER="$GAME_DIR/screenshots/lwh_order_answer.tga"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

fail() { echo "FAIL  $1"; exit 1; }

mkdir -p "$OUT" "$GAME_DIR/screenshots"
rm -f "${OUT:?}/orders.txt" "${OUT:?}/orders-overlay.txt" "${SHOT_GIVE:?}" "${SHOT_ANSWER:?}"

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo
echo "==> orders: the derivation, the check, the dispatch, the refusal (g_shipTest 92)"
SDL_AUDIODRIVER=dummy timeout 200 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipMode 1 +set g_shipTest 92 +map "$MAP" \
    >"$HOME_DIR/orders.out" 2>&1 || true

grep -h '^SHIP: order test' "$HOME_DIR/orders.out" | sed 's/^SHIP: /    /' || true

[ -f "$OUT/orders.txt" ] || fail "the orders run did not finish"

# The set follows the person: an engineer and a security officer offer different orders, and the same
# officer's set changes when their post does.
grep -q '^SHIP: order test: engineer set: see to the warp drive;.*fabricate' "$HOME_DIR/orders.out" \
  || fail "the engineer was not offered the engineering orders"
grep -q '^SHIP: order test: security set: take security to the deck .*retake the deck' "$HOME_DIR/orders.out" \
  || fail "the security officer was not offered the security orders"
grep -q '^SHIP: order test: the same officer given the phaser post: see to the phasers;' "$HOME_DIR/orders.out" \
  || fail "the set did not follow the person when the post changed"

# The check fails both ways, planted and caught, and the tree passes.
grep -q '^SHIP: order test: tree consistent=1; planted foreign caught=1; planted missing caught=1' "$HOME_DIR/orders.out" \
  || fail "the derivation guard did not catch a foreign order and a missing one"

# Dispatch pushes a job; an unauthorised order is refused by name with the state unchanged.
grep -q '^SHIP: order test: dispatched given=1; job [0-9]* kind repair target warp drive' "$HOME_DIR/orders.out" \
  || fail "the dispatched order did not push a repair job with its kind and target"
grep -q '^SHIP: order test: ensign refused=1 rule "you have no authority over .*"; state unchanged=1' "$HOME_DIR/orders.out" \
  || fail "the unauthorised order was not refused by name with the state unchanged"

# The bespoke path, and the recipient's voice with no inference.
grep -q '^SHIP: order test: bespoke novel refused=1; matched given=1' "$HOME_DIR/orders.out" \
  || fail "the bespoke path did not refuse a novel order and dispatch a matched one"
grep -q '^SHIP: order test: voice key .*; planned [1-9][0-9]* line(s); model calls 0' "$HOME_DIR/orders.out" \
  || fail "the recipient had no voice or a model was called for an enumerated order"

# A save replays the order and its job.
grep -q '^SHIP: order test: save replay loaded=1, orderRepairFirst=.*, job back=1' "$HOME_DIR/orders.out" \
  || fail "the order and its job did not round-trip through a save"

echo
echo "==> orders: the order on the screen (g_shipTest 93)"
find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
SDL_AUDIODRIVER=dummy timeout 200 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipMode 1 +set g_shipTest 93 +map "$MAP" \
    >"$HOME_DIR/orders-overlay.out" 2>&1 || true

grep -h '^SHIP: order overlay' "$HOME_DIR/orders-overlay.out" | sed 's/^SHIP: /    /' || true

[ -f "$OUT/orders-overlay.txt" ] || fail "the order overlay run did not finish"
grep -q '^SHIP: order overlay: given=1, job [0-9]*' "$HOME_DIR/orders-overlay.out" \
  || fail "the order was not given from the screen"
[ -f "$SHOT_GIVE" ] || fail "no screenshot of the order being given"
[ -f "$SHOT_ANSWER" ] || fail "no screenshot of the recipient's answer"

echo "PASS  the register and the dispatch agree both ways; the set is derived per recipient and"
echo "      checked both ways with both failures planted and caught; an order pushes a job; an"
echo "      unauthorised order is refused by name; the bespoke path refuses what is outside the set;"
echo "      the recipient speaks in their own voice and a pill costs no inference; a save replays"
echo "      screenshot: $SHOT_GIVE (the order being given), $SHOT_ANSWER (the recipient's answer)"
