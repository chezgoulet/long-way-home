#!/usr/bin/env bash
# Gate S3, the part a machine can check: the whole ship is one map, and every deck can be reached.
#
#   scripts/s3-check.sh [--rebuild]
#
# Builds the ship if it has not been built (scripts/build-ship.sh; --rebuild forces it), then two
# headless engine runs on it:
#   tour      rides the turbolift from deck to deck, all fifteen in one run with no level load, and
#             on each checks the player arrived on that deck, standing on a floor, clear of solid
#   measure   twenty seconds with every deck's entities and scripts live, for the game-frame time
#
# What it does not check is the rest of S3's bar: that a person can walk each deck once there.
# Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REBUILD=0
[ "${1:-}" = "--rebuild" ] && REBUILD=1

HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
PK3="$ROOT/build/ship/out/longway_voyager.pk3"
PITCH=3072
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

if [ "$REBUILD" -eq 1 ] || [ ! -f "$PK3" ]; then
  "$ROOT/scripts/build-ship.sh" | grep -E 'decks ->|brush models|boxes|turbolift|wrote' | sed 's/^/    /'
fi
mkdir -p "$GAME_DIR/crew"
cp "$PK3" "$GAME_DIR/"
# The ship stays out of the measurement home afterwards: G3's and S2's runs must not find it.
trap 'find "$GAME_DIR" -maxdepth 1 -name "longway_voyager.pk3" -delete' EXIT
find "$GAME_DIR/maps" -maxdepth 1 -name 'voyager.nav' -delete 2>/dev/null || true
find "$GAME_DIR/crew" -maxdepth 1 -name 'voyager.baseline.json' -delete

engine() {
  local label="$1"; shift
  find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
  echo "==> $label"
  # com_hunkMegs: a whole ship's visibility data does not fit the default hunk
  SDL_AUDIODRIVER=dummy timeout 300 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
      +set com_hunkMegs 768 +set s_useOpenAL 0 +set g_ship 1 +set g_shipDeckPitch "$PITCH" "$@" \
      >"$HOME_DIR/s3-$label.out" 2>&1 || true
}

engine tour    +set g_shipTest 5 +map voyager
engine measure +set g_crew 0 +set g_crewRun 20 +set g_crewQuit 1 +map voyager

fail() { echo "FAIL  $1"; exit 1; }
grep -h '^EFSP: SP_SpawnServer: .* inline models' "$HOME_DIR/s3-tour.out" | sed 's/^EFSP: SP_SpawnServer: /    /' || true
grep -h '^SHIP: deck ' "$HOME_DIR/s3-tour.out" | sed 's/^SHIP: /    /' || true
grep -q '^SHIP: turbolift tour: 15 of 15 decks reached standing and clear' "$HOME_DIR/s3-tour.out" \
  || fail "the turbolift tour did not reach all fifteen decks (see $HOME_DIR/s3-tour.out)"
echo "PASS  one map, fifteen decks, each reached by turbolift from the one before with no level load"

BASE="$GAME_DIR/crew/voyager.baseline.json"
[ -f "$BASE" ] || fail "the measured run did not finish"
python3 - "$BASE" <<'PY'
import json, sys
b = json.load(open(sys.argv[1]))
avg, worst = b["game_frame_avg_us"] / 1000, b["game_frame_max_us"] / 1000
print(f"INFO  game frame {avg:.1f} ms average, {worst:.1f} ms worst over {b['game_frames']} frames; "
      f"{b['scripts']['with_sequencer']} scripted entities live")
if avg > 16.7:
    print("FAIL  the game alone averages more than a 60 fps frame")
    sys.exit(1)
print("PASS  the game's average frame fits a 60 fps budget (the worst frame is reported, not judged)")
PY
