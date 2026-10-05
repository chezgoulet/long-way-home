#!/usr/bin/env bash
# Build the whole ship as one map (gate S3): stitch the published deck sources and compile.
#
#   scripts/build-ship.sh [--out DIR]
#
# Needs scripts/fetch-gdk.sh (the deck sources) and scripts/fetch-map-tools.sh (q3map2) to have
# been run. Writes build/ship/voyager.map, its stitch report, and longway_voyager.pk3 into DIR
# (default build/ship/out -- copy it beside the game's paks, or pass --out build/home/baseEF,
# to load it with `map voyager`).

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$ROOT/build/ship/out"
while [ $# -gt 0 ]; do
  case "$1" in
    --out) OUT="$2"; shift 2 ;;
    *) echo "unknown argument: $1" >&2; exit 2 ;;
  esac
done

DECKS="$ROOT/build/gdk/maps/eliteforce_virtualvoyager_maps"
[ -d "$DECKS" ] || { echo "no deck sources at $DECKS -- run scripts/fetch-gdk.sh" >&2; exit 1; }
mkdir -p "$ROOT/build/ship" "$OUT"

python3 "$ROOT/tools/shipmap/stitch.py" --decks "$DECKS" --out "$ROOT/build/ship/voyager.map" \
  --report "$ROOT/build/ship/stitch-report.json"
"$ROOT/scripts/build-map.sh" "$ROOT/build/ship/voyager.map" --name voyager --out "$OUT" --allow-missing-shaders
