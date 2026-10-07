#!/usr/bin/env bash
# Play a crewed scenario: build its crew file, then start the engine on its map with the
# direction layer on.
#
#   scripts/run-scenario.sh [SCENARIO_DIR] [-- <extra engine args>]
#
# SCENARIO_DIR defaults to scenarios/deck04-watch. This is the run for judging whether a deck
# feels inhabited -- the one criterion scripts/g3-measure.sh cannot decide. It needs a logged-in
# desktop session, like any playtest (see docs/playtest.md).
#
# The only thing written into the playtest home is maps/<map>.crew, which the game ignores unless
# g_crew is 1. In game, `crew report` in the console prints the run so far.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SCENARIO="$ROOT/scenarios/deck04-watch"
if [ $# -gt 0 ] && [ "$1" != "--" ]; then SCENARIO="$1"; shift; fi
[ "${1:-}" = "--" ] && shift

BUILD="$ROOT/build"
[ -f "$SCENARIO/scenario.json" ] || { echo "no scenario.json in $SCENARIO" >&2; exit 1; }
[ -d "$BUILD/baseEF" ] || { echo "no game data at $BUILD/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }

python3 "$ROOT/tools/validator/validate.py" "$SCENARIO" --data "$BUILD" --quiet
MAP="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["crew"]["map"])' "$SCENARIO/scenario.json")"
python3 "$ROOT/tools/crewgen/crewgen.py" build "$SCENARIO" --data "$BUILD" --out "$BUILD/home/baseEF"
# A scenario that brings its own map has it compiled and packed where the engine will find it.
if [ -f "$SCENARIO/maps/$MAP.map" ]; then
  "$ROOT/scripts/build-map.sh" "$SCENARIO/maps/$MAP.map" --name "$MAP" --out "$BUILD/home/baseEF"
fi

exec "$ROOT/scripts/run-engine.sh" +set g_crew 1 +map "$MAP" "$@"
