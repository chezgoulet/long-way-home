#!/usr/bin/env bash
# Access and authority at the console (docs/access-and-authority.md, owner decision 2026-10-07).
#
#   scripts/clearance-check.sh [--map NAME]
#
# One headless engine run. The player takes the post of a security ensign and tries to work
# Engineering's console, where they are not cleared. The run then does what the documented model
# promises, by the same console commands a console sends:
#
#   1. the refusal is named -- it says who can open the control;
#   2. a department head delegates the station for a shift, and the ensign may then work it;
#   3. the grant is revoked, and the ensign is refused again;
#   4. a senior officer locks the ensign out, and the console that stopped answering names both;
#   5. the emergency override is begun and agreed.
#
# Run in holodeck mode (g_shipMode 1), the same as the other console checks. Writes under
# build/g3-home only.

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
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }
mkdir -p "$GAME_DIR"

OUT="$HOME_DIR/clearance.out"
find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> access and authority"
SDL_AUDIODRIVER=dummy timeout 180 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipMode 1 +set g_shipTest 64 +map "$MAP" >"$OUT" 2>&1 || true

grep -h 'clearance [0-9]' "$OUT" | sed 's/^/    /' || true

fail() { echo "FAIL  $1"; exit 1; }
pass() { echo "PASS  $1"; }

[ -f "$OUT" ] || fail "the run wrote no output"

grep -q 'clearance 1 (not cleared): "you are not cleared for MAIN ENGINEERING' "$OUT" \
  || fail "the not-cleared refusal did not name the station and who can open it"
pass "a locked control names who can open it (MAIN ENGINEERING, the Chief Engineer)"

grep -q 'clearance 2 (delegated for the shift): sensors off' "$OUT" \
  || fail "the delegated station did not accept the ensign's command"
pass "a department head's delegation for the shift opened the station"

grep -q 'clearance 3 (revoked): "you are not cleared for MAIN ENGINEERING' "$OUT" \
  || fail "the revoked ensign was not refused again"
pass "the revocation took the access back, and the refusal is legible"

grep -q 'clearance 4 (locked out): "LOCKED OUT: .* has locked .* out of MAIN ENGINEERING"' "$OUT" \
  || fail "the locked-out console did not name both hands"
pass "the lock-out is logged and the console that stopped answering names both hands"

grep -q 'clearance 5 (override): "OVERRIDE PENDING AT MAIN ENGINEERING' "$OUT" \
  || fail "the emergency override was not begun and agreed"
pass "the emergency override is begun, slow and logged, and a second officer agreed"

echo
sed 's/^/    /' "$OUT" | grep '^    SHIP: clearance' || true
