#!/usr/bin/env bash
# The screens thickened by Task C, and the two axes (docs/evidence/the-missing-screens.md). One run
# each: the Tactical console's phaser setting (the decision that exists with no contact); the triage
# board acting (it reported and could not act); and the two owner rulings -- the station shapes the
# console, the person filters it -- with a single-type console demonstrated and the remote call-up
# reaching a system stationed elsewhere from it.
#
#   scripts/screens-check.sh
#
# Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

fail() { echo "FAIL  $1"; exit 1; }
pass() { echo "PASS  $1"; }

run() {
  local test="$1"
  find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
  SDL_AUDIODRIVER=dummy timeout 180 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
      +set s_useOpenAL 0 +set g_ship 1 +set g_shipTest "$test" +map tour/deck04 >"$HOME_DIR/screens-$test.out" 2>&1 || true
}

echo "==> Tactical: the phaser setting (the decision with no contact)"
run 66
grep -h '^SHIP: tactical test' "$HOME_DIR/screens-66.out" | sed 's/^SHIP: /    /' || true
grep -qE 'tactical test: phaser bank (STUN|HEAVY STUN|KILL|VAPORIZE), demand [0-9]+ of [0-9]+ \([0-9]+%\)' "$HOME_DIR/screens-66.out" \
  || fail "the Tactical console did not cycle and report the phaser setting (see $HOME_DIR/screens-66.out)"
grep -qE 'tactical test: stun asks [0-9]+, vaporize asks [0-9]+; vaporize asks [0-9]+ more' "$HOME_DIR/screens-66.out" \
  || fail "the setting did not change what the bank asks of the power budget"
[ -f "$GAME_DIR/screenshots/lwh_tactical_after.tga" ] || fail "no screenshot of the Tactical console after the setting"
pass "Tactical offers a real decision with no contact -- the phaser setting -- and it changes the power budget"

echo
echo "==> the triage board acts"
run 67
grep -h '^SHIP: triage test' "$HOME_DIR/screens-67.out" | sed 's/^SHIP: /    /' || true
grep -q 'triage test: surgical field 1, EMH 1, triage order 1, ward 4, recovery window 0' "$HOME_DIR/screens-67.out" \
  || fail "the triage board did not act (see $HOME_DIR/screens-67.out)"
[ -f "$GAME_DIR/screenshots/lwh_triage_before.tga" ] && [ -f "$GAME_DIR/screenshots/lwh_triage_after.tga" ] \
  || fail "no before/after screenshot of the triage board"
pass "the triage board now acts: the surgical field, the Doctor, a captive recovered, and the order (command's)"

echo
echo "==> two axes: the station shapes the console, the person filters it"
run 68
grep -h '^SHIP: axes test\|^SHIP: remote call-up' "$HOME_DIR/screens-68.out" | sed 's/^SHIP: /    /' || true
grep -q 'axes test: reads for Tactical ".\+", Ops "", Sickbay ".\+"' "$HOME_DIR/screens-68.out" \
  || fail "Tactical and Sickbay did not carry a read layer, or Operations did (see $HOME_DIR/screens-68.out)"
grep -q 'axes test: reads for Tactical ".\+", Ops "",' "$HOME_DIR/screens-68.out" \
  || fail "Operations carried a read layer it does not need: it must stay single-type"
grep -q 'axes test: Conn reaches sensors: yes, operated' "$HOME_DIR/screens-68.out" \
  || fail "a commander at the single-type Conn console did not reach a system stationed elsewhere"
grep -q 'axes test: an uncleared ensign at Conn: "you are not cleared for CONN' "$HOME_DIR/screens-68.out" \
  || fail "an uncleared ensign at the Conn was not refused"
[ -f "$GAME_DIR/screenshots/lwh_conn_single.tga" ] || fail "no screenshot of the single-type Conn console"
pass "the station decides the console's content types; the person decides what they reach, from any console"

echo
echo "PASS  the two screens offer a real decision, and the two axes hold"
