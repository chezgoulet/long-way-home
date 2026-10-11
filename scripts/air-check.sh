#!/usr/bin/env bash
# The independent air feed in the engine (O2, docs/budget-squaring.md Finding two): the three
# sources that survive the core as separate state -- one isolated, one failed, one carrying -- a deck
# holding its air on its own source with the grid down, the number before and after the crew act, and
# the shortfall when a source is over-committed. It also runs the holodeck reactor (O2's remainder):
# the fourth source, off the grid, refused while the grid is up, holding a deck with it down, a
# shortfall when over-committed, and the cost when the grid returns while a deck is still tied to it.
# The model half is proved without the game in tests/ship (TestTheAir).
#
#   scripts/air-check.sh
#
# One headless engine run. Needs the game data (scripts/playtest-host-setup.sh) and xvfb-run. Writes
# under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
OUT="$GAME_DIR/ship"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }
mkdir -p "$OUT" "$GAME_DIR/screenshots"
rm -f "${OUT:?}/air.txt"

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> the independent air feed (g_shipTest 98)"
SDL_AUDIODRIVER=dummy timeout 180 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipMode 1 +set g_shipTest 98 +map tour/deck04 \
    >"$HOME_DIR/air.out" 2>&1 || true

grep -h '^SHIP: air test' "$HOME_DIR/air.out" | sed 's/^/    /' || true

fail() { echo "FAIL  $1"; exit 1; }

[ -f "$OUT/air.txt" ] || fail "the run did not finish: no ship/air.txt"
grep -q '^SHIP: air test: the three independent sources: impulse 250, auxiliary 90, batteries 60 EPS' "$HOME_DIR/air.out" \
  || fail "the three sources' capacities were not the budget-squaring numbers"
grep -q '^SHIP: air test: deck 9 feed auxiliary fusion support 1.00; deck 8 feed the main grid support 0.00' "$HOME_DIR/air.out" \
  || fail "a deck on its own source did not hold air with the grid down, or a grid deck did not lose it"
grep -qE '^SHIP: air test: deck 9 breached: [0-9]+ min of air; field raised: -1 min' "$HOME_DIR/air.out" \
  || fail "the breach did not produce a number, or the crew act (the field) did not move it"
grep -q '^SHIP: air test: impulse isolated, batteries health 0%, auxiliary holds 1 deck(s)' "$HOME_DIR/air.out" \
  || fail "the three sources were not shown as separate -- one isolated, one failed, one carrying"
grep -q '^SHIP: air test: batteries asked for 2 decks, hold 1; committed 120 EPS, short 60 EPS' "$HOME_DIR/air.out" \
  || fail "the shortfall was not reported as a number when the source was over-committed"
grep -q 'air: 2 deck(s) on their own source' "$OUT/air.txt" \
  || fail "the console did not show the decks on their own source"
grep -q '^SHIP: air test: holodeck reactor: grid fresh 1800 EPS, tie with the grid up refused, tie with it down allowed' "$HOME_DIR/air.out" \
  || fail "the holodeck reactor joined the grid, or a tie with the grid up was not refused"
grep -q '^SHIP: air test: deck 9 feed holodeck reactor support 1.00; deck 8 support 0.00' "$HOME_DIR/air.out" \
  || fail "a deck did not hold its air on the holodeck reactor with the grid down"
grep -q '^SHIP: air test: holodeck asked for 2 decks, holds 1; short 60 EPS, nothing shed' "$HOME_DIR/air.out" \
  || fail "the holodeck reactor's over-commitment was not a shortfall number"
grep -qE '^SHIP: air test: the grid returned: reactor health 0%, deck 9 support 0.00, short [0-9]+ EPS' "$HOME_DIR/air.out" \
  || fail "the return of the grid while a deck was tied did not spend the reactor (the incompatibility's cost)"
echo "PASS  a deck holds its air on its own source; the three sources are separate; the shortfall is a number"
echo "      and the holodeck reactor is the fourth: off the grid, refused live, spent by the cross-tie"
echo "      report: $OUT/air.txt"
