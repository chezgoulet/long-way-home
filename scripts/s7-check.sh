#!/usr/bin/env bash
# Gates S7 and S8, the part that is in the game: boarders the ship simulation counts on the
# player's deck are hostile bodies there, and killing one is one fewer for the ship.
#
#   scripts/s7-check.sh
#
# Two headless engine runs on deck 4, told it is deck 11 (Main Engineering): three raiders board,
# then three Borg drones. Each run waits for the bodies, reports what they are and whom they are
# hostile to, kills one, and reports the ship's count afterwards. The ship's clock is slowed to
# real time so that her own security, fighting the same boarders in the simulation, does not
# settle the matter first.
#
# The rules themselves -- hijacking, counter-hacking, assimilation, stripping -- are tested without
# the game in tests/ship. Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }
mkdir -p "$GAME_DIR"

fail() { echo "FAIL  $1"; exit 1; }

run() {
  local kind="$1" team="$2" noun="$3"
  local out="$HOME_DIR/s7-$kind.out"
  find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
  echo "==> $kind"
  SDL_AUDIODRIVER=dummy timeout 180 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
      +set s_useOpenAL 0 +set g_ship 1 +set g_shipDayScale 1 +set g_crew 1 +set g_crewFromShip 1 +set g_crewDeck 11 \
      +set g_shipTest 8 +set g_shipTestPos "$kind" +map tour/deck04 >"$out" 2>&1 || true
  grep -h '^SHIP: boarder body\|^SHIP: boarding test\|^CREW: a boarder is down' "$out" | sed 's/^[A-Z]*: /    /' || true
  grep -q '^SHIP: boarding test: 3 bodies for 3 boarders the ship counts' "$out" \
    || fail "$kind: the bodies on the deck are not the boarders the ship counts"
  [ "$(grep -c "^SHIP: boarder body .* team $team, hostile to team 1" "$out")" -eq 3 ] \
    || fail "$kind: the bodies are not three $noun hostile to the crew"
  grep -q '^SHIP: boarding test: after one was killed the ship counts 2' "$out" \
    || fail "$kind: killing a body did not take one from the ship's count"
  echo "PASS  three $noun board, are embodied hostile to the crew, and one killed is one fewer aboard"
}

run raiders 4 "raiders"
run borg    2 "Borg drones"
