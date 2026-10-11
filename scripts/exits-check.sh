#!/usr/bin/env bash
# The wall-clock exit and the idempotent console registration (docs/omissions.md O14).
#
#   scripts/exits-check.sh
#
# Five headless engine runs. The quit path is the engine's own (Com_Quit_f -- a real `quit` reaches
# it, and the module answers through patches/0023):
#
#   suspend    holodeck, `g_shipQuit 1`: quitting leaves the ship standing and the run marked, and
#              the entry Suspend wrote is named.
#   default    holodeck, `g_shipQuit` unset (0): the simulation is shut down, which is what a quit
#              did before the exit was wired. The two exits, and the cvar chooses.
#   ironman    `g_shipMode 0`, `g_shipQuit 1`: ironman refuses the world's suspension.
#   retail     `g_ship 0`, a campaign map: a quit the module does not answer for, unchanged.
#   transition two SP map loads, which dlclose and reload libefgame.so: no console registration the
#              engine already holds is announced again (patches/0024).
#
# Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }
mkdir -p "$GAME_DIR"

engine() {
  local label="$1" seconds="$2"; shift 2
  find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
  echo "==> $label"
  SDL_AUDIODRIVER=dummy timeout "$seconds" xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
      +set s_useOpenAL 0 "$@" >"$HOME_DIR/exits-$label.out" 2>&1 || true
}

fail() { echo "FAIL  $1"; exit 1; }

engine suspend    150 +set g_ship 1 +set g_shipMode 1 +set g_shipQuit 1 +efsp tour/deck04 +wait 400 +quit
engine default    150 +set g_ship 1 +set g_shipMode 1 +efsp tour/deck04 +wait 400 +quit
engine ironman    150 +set g_ship 1 +set g_shipMode 0 +set g_shipQuit 1 +efsp tour/deck04 +wait 400 +quit
engine retail     150 +set g_ship 0 +efsp borg1 +wait 400 +quit
engine transition 220 +set g_ship 0 +efsp borg1 +wait 300 +efsp borg2 +wait 300 +quit

grep -h '^SHIP: quit:' "$HOME_DIR"/exits-suspend.out "$HOME_DIR"/exits-default.out \
    "$HOME_DIR"/exits-ironman.out | sed 's/^SHIP: /    /' || true

# the quit, our mode: the ship is left standing, and what Suspend wrote is named
grep -q '^SHIP: quit: the ship is left standing; the run is marked (left standing 1)$' "$HOME_DIR/exits-suspend.out" \
  || fail "holodeck did not leave the ship standing on quit"
grep -q '^SHIP: quit: the log carries: .*\[command\] .*: the ship is left standing; she keeps her own time$' \
    "$HOME_DIR/exits-suspend.out" \
  || fail "Suspend's log entry is not in the record"

# the cvar's default, and the fourth case: off means the simulation is shut down
grep -q '^SHIP: quit: the simulation is shut down' "$HOME_DIR/exits-default.out" \
  || fail "with g_shipQuit unset the simulation should shut down"
grep -q 'left standing' "$HOME_DIR/exits-default.out" \
  && fail "g_shipQuit 0 marked the run left standing"

# the guard: ironman means the ship keeps her own time
grep -q '^SHIP: quit: ironman will not suspend the world' "$HOME_DIR/exits-ironman.out" \
  || fail "ironman permitted the world's suspension"

# the retail path: the engine quits, and the module says nothing
grep -q '^----- Client Shutdown (Client quit) -----' "$HOME_DIR/exits-retail.out" \
  || fail "the retail quit did not reach the engine's quit path"
grep -q '^SHIP: quit' "$HOME_DIR/exits-retail.out" \
  && fail "a retail quit was answered by the simulation"

# the registrations: the transition announces nothing
before="$(grep -c 'already defined' "$HOME_DIR/exits-transition.out" || true)"
echo "    registrations on the transition: $before message(s) (was 38)"
[ "$before" = "0" ] || { grep 'already defined' "$HOME_DIR/exits-transition.out" | sed 's/^/    /'; fail "a registration was announced again"; }
grep -q '^EFSP: SP_LoadGame: ge=' "$HOME_DIR/exits-transition.out" \
  || fail "the transition did not reload the game module, so nothing was proved"

echo "PASS  holodeck suspends on quit and names what Suspend wrote; the default shuts the simulation down;"
echo "      ironman refuses; the retail quit is unchanged; a level transition announces no registration"
