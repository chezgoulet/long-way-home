#!/usr/bin/env bash
# Runs the G3 reactive-crew measurement on a scenario and judges the result.
#
#   scripts/g3-measure.sh [--scenario DIR] [--seconds N] [--display]
#
# --scenario  a scenario with a `crew` section      (default: scenarios/deck04-watch)
# --seconds   length of the sampled observation run (default: 600, the charter's ten minutes)
# --display   use the current display instead of a virtual one (to watch the run)
#
# Three engine runs, each headless by default and each quitting on its own:
#   1. baseline  the deck with the layer off -- the game-frame clock and the script census
#   2. crewed    the measured run; ends by saving
#   3. reload    loads that save, checks the crew come back as they were and still hold their posts
# Then tools/crewgen/g3report.py reads what the module wrote and prints one line per criterion.
#
# Everything is written under build/g3-home, never the playtest home, so a measurement cannot
# overwrite a save the owner made by playing.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SCENARIO="$ROOT/scenarios/deck04-watch"
SECONDS_RUN=600
USE_DISPLAY=0
while [ $# -gt 0 ]; do
  case "$1" in
    --scenario) SCENARIO="$2"; shift 2 ;;
    --seconds)  SECONDS_RUN="$2"; shift 2 ;;
    --display)  USE_DISPLAY=1; shift ;;
    *) echo "unknown argument: $1" >&2; exit 2 ;;
  esac
done

BUILD="$ROOT/build"
G3_HOME="$BUILD/g3-home"
[ -d "$BUILD/baseEF" ] || { echo "no game data at $BUILD/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }

MAP="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["crew"]["map"])' "$SCENARIO/scenario.json")"
KEY="${MAP//\//_}"
GAME_DIR="$G3_HOME/baseEF"
OUT="$GAME_DIR/crew"
SAVES="$GAME_DIR/saves"
KEEP="$G3_HOME/crewed-run"

echo "==> validating $SCENARIO"
python3 "$ROOT/tools/validator/validate.py" "$SCENARIO" --data "$BUILD" --quiet
mkdir -p "$OUT" "$SAVES" "$KEEP"
# Results of an earlier measurement must not be read as this one's.
for stale in report baseline saved restored reloaded.report; do
  rm -f "${OUT:?}/${KEY:?}.$stale.json" "${KEEP:?}/${KEY:?}.$stale.json"
done
rm -f "${SAVES:?}/crewbase.sav" "${SAVES:?}/crewrun.sav" "${KEEP:?}/crewrun.sav"
python3 "$ROOT/tools/crewgen/crewgen.py" build "$SCENARIO" --data "$BUILD" --out "$GAME_DIR"
# A scenario that brings its own map has it compiled and packed where the engine will find it.
if [ -f "$SCENARIO/maps/$MAP.map" ]; then
  "$ROOT/scripts/build-map.sh" "$SCENARIO/maps/$MAP.map" --name "$MAP" --out "$GAME_DIR"
fi

# The harness's precedence test runs this script on a crew member; compiled by our own compiler.
IBIZE="$ROOT/../upstream/ibize-build/ibize"
[ -x "$IBIZE" ] || { echo "$IBIZE not built -- run scripts/bootstrap-upstream.sh" >&2; exit 1; }
mkdir -p "$GAME_DIR/real_scripts/lwh"
"$IBIZE" "$ROOT/tests/g3/interrupt.txt" "$GAME_DIR/real_scripts/lwh/interrupt.IBI" >/dev/null

# Sound stays on, into SDL's null device when headless: acknowledgement is timed against the
# length of the lines actually spoken, which a run with the sound system off would report as zero.
AUDIO="${SDL_AUDIODRIVER:-}"
RUNNER=()
if [ "$USE_DISPLAY" -eq 0 ]; then
  AUDIO=dummy
  command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found; install xvfb or pass --display" >&2; exit 1; }
  RUNNER=(xvfb-run -a)
fi

# One engine run. A stale PID file makes the engine stop at a "safe video settings" dialog that a
# headless run can never answer, so it is cleared first.
engine() {
  local label="$1" limit="$2"; shift 2
  find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
  echo "==> $label"
  if ! SDL_AUDIODRIVER="$AUDIO" timeout "$limit" "${RUNNER[@]}" "$ROOT/scripts/run-engine.sh" --home-dir "$G3_HOME" \
        +set s_useOpenAL 0 +set g_crewQuit 1 "$@" >"$G3_HOME/$label.out" 2>&1; then
    echo "    the engine did not finish within ${limit}s (or failed); see $G3_HOME/$label.out" >&2
  fi
  grep -h '^CREW: \(verdict\|coverage\|run complete\|baseline run complete\|restored\)' "$G3_HOME/$label.out" | sed 's/^/    /' || true
}

MARGIN=180   # map load, the walk to the posts, and the save
engine baseline $((SECONDS_RUN + MARGIN)) +set g_crew 0 +set g_crewRun "$SECONDS_RUN" +map "$MAP"
engine crewed   $((SECONDS_RUN + MARGIN)) +set g_crew 1 +set g_crewRun "$SECONDS_RUN" +map "$MAP"

# The reload run writes its own report and save over the crewed run's; keep those aside first.
if [ -f "$OUT/$KEY.report.json" ] && [ -f "$SAVES/crewrun.sav" ]; then
  cp "$OUT/$KEY.report.json" "$OUT/$KEY.saved.json" "$SAVES/crewrun.sav" "$KEEP/"
  engine reload $((30 + MARGIN)) +set g_crew 1 +set g_crewRun 30 +load crewrun
  if cmp -s "$OUT/$KEY.report.json" "$KEEP/$KEY.report.json"; then
    echo "    the reload run wrote no report of its own" >&2
  else
    mv "$OUT/$KEY.report.json" "$OUT/$KEY.reloaded.report.json"
  fi
  cp "$KEEP/$KEY.report.json" "$KEEP/$KEY.saved.json" "$OUT/"
  cp "$KEEP/crewrun.sav" "$SAVES/"
else
  echo "    the crewed run left no report or save; skipping the reload" >&2
fi

echo
python3 "$ROOT/tools/crewgen/g3report.py" "$OUT" "$KEY" --saves "$SAVES" --min-run-s "$SECONDS_RUN"
