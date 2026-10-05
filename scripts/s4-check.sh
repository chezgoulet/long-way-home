#!/usr/bin/env bash
# Gate S4, as far as it is built: a station's console opens from the command the ship's own panel
# sends, shows that station's systems, and operates the ship within that station's authority.
#
#   scripts/s4-check.sh
#
# One headless engine run on deck 4. The Tactical console is opened with `ui_tactical` -- the
# command the retail panels issue -- and operated by key: condition red, the second system switched
# off, and a priority key, which only Engineering may use and which must therefore change nothing.
#
# Not checked, because not built: the other stations' own controls beyond on/off, live status on
# the panel surfaces in the world, and the panels on the merged ship's bridge being walked up to.
# Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
OUT="$GAME_DIR/ship/tactical.txt"
SHOT="$GAME_DIR/screenshots/lwh_tactical.tga"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }
mkdir -p "$GAME_DIR/ship"
rm -f "${OUT:?}" "${SHOT:?}"

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> tactical"
SDL_AUDIODRIVER=dummy timeout 180 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipTest 6 +map tour/deck04 >"$HOME_DIR/s4-tactical.out" 2>&1 || true

fail() { echo "FAIL  $1"; exit 1; }
[ -f "$OUT" ] || fail "the run did not finish (see $HOME_DIR/s4-tactical.out)"
grep -q 'condition red' "$OUT" || fail "the Tactical console's condition-red key did not reach the ship"
grep -q 'phasers .* OFF' "$OUT" || fail "the Tactical console did not switch off its second system (phasers)"
grep -q 'shields .* power 200/200' "$OUT" || fail "the shields, untouched, are not at full power under red alert"
echo "PASS  opened by the retail panel's command, the Tactical console set the condition and switched its own system"
grep -q '^SHIP: phasers priority 7$' "$HOME_DIR/s4-tactical.out" \
  || fail "a priority key at Tactical changed the power order, which is Engineering's to set"
echo "PASS  the power order cannot be changed from Tactical"

# Clearance. The player becomes a security ensign and tries four things at two consoles.
find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> clearance"
SDL_AUDIODRIVER=dummy timeout 180 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipTest 7 +map tour/deck04 >"$HOME_DIR/s4-clearance.out" 2>&1 || true
grep -h '^SHIP: .* refused\|^SHIP: you are\|^SHIP: clearance test' "$HOME_DIR/s4-clearance.out" | sed 's/^SHIP: /    /' || true
grep -q '^SHIP: off refused at MAIN ENGINEERING: you are not cleared for this station' "$HOME_DIR/s4-clearance.out" \
  || fail "a security ensign was not refused at the Engineering console"
grep -q '^SHIP: alert refused at TACTICAL: calling the alert needs a lieutenant or above' "$HOME_DIR/s4-clearance.out" \
  || fail "an ensign was allowed to call the alert"
grep -q '^SHIP: off refused at TACTICAL: that system is not operated from this station' "$HOME_DIR/s4-clearance.out" \
  || fail "Tactical was allowed to switch a system that is not its own"
grep -q '^SHIP: clearance test: sensors on, phasers off, condition 0$' "$HOME_DIR/s4-clearance.out" \
  || fail "the ship's state after the clearance test is not what the refusals imply"
echo "PASS  a security ensign operates Tactical's own systems and nothing else, and does not call the alert"
[ -f "$SHOT" ] || fail "no screenshot of the console"
echo "      screenshot: $SHOT"

# The other panels: the transporter's opens the Operations console, and Sickbay shows the medical
# state. Both are the panel commands the maps carry, with the ship simulation running.
OPS_SHOT="$GAME_DIR/screenshots/lwh_ops.tga"
MED_SHOT="$GAME_DIR/screenshots/lwh_sickbay.tga"
rm -f "$OPS_SHOT" "$MED_SHOT"
find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> panels"
SDL_AUDIODRIVER=dummy timeout 180 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipTest 14 +map tour/deck04 >"$HOME_DIR/s4-panels.out" 2>&1 || true
grep -h 'LWH: the .* panel opens the\|SHIP: medical state:' "$HOME_DIR/s4-panels.out" | sed 's/^EFSP: /    /' || true
grep -q 'LWH: the transporter panel opens the OPERATIONS console' "$HOME_DIR/s4-panels.out" \
  || fail "the transporter panel did not open the Operations console"
grep -q 'SHIP: medical state: INJURED ' "$HOME_DIR/s4-panels.out" \
  || fail "the Sickbay console has no medical state"
[ -f "$OPS_SHOT" ] && [ -f "$MED_SHOT" ] || fail "the panels did not open (no screenshots)"
echo "PASS  the transporter panel opens Operations, and Sickbay shows the ship's medical state"
echo "      screenshots: $OPS_SHOT, $MED_SHOT"
