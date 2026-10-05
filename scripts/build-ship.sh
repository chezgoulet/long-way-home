#!/usr/bin/env bash
# Build the whole ship as one map (gate S3): stitch the published deck sources and compile.
#
#   scripts/build-ship.sh [--out DIR] [--fast]
#
# Needs scripts/fetch-gdk.sh (the deck sources) and scripts/fetch-map-tools.sh (q3map2) to have
# been run. Writes build/ship/voyager.map, its stitch report, and longway_voyager.pk3 into DIR
# (default build/ship/out -- copy it beside the game's paks, or pass --out build/home/baseEF,
# to load it with `map voyager`).

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$ROOT/build/ship/out"
PASSES=(--light)
while [ $# -gt 0 ]; do
  case "$1" in
    --out) OUT="$2"; shift 2 ;;
    --fast) PASSES=(); shift ;;
    *) echo "unknown argument: $1" >&2; exit 2 ;;
  esac
done

DECKS="$ROOT/build/gdk/maps/eliteforce_virtualvoyager_maps"
[ -d "$DECKS" ] || { echo "no deck sources at $DECKS -- run scripts/fetch-gdk.sh" >&2; exit 1; }
mkdir -p "$ROOT/build/ship" "$OUT"

# The five decks nobody ever published get a generated placeholder each (tools/shipmap/gendeck.py).
python3 "$ROOT/tools/shipmap/gendeck.py" --deck 6 --deck 7 --deck 12 --deck 13 --deck 14 --out "$ROOT/build/ship/generated"

# What goes in the pak beside the map: the turbolift's deck list for fifteen decks.
mkdir -p "$ROOT/build/ship/files/ext_data"
cp "$ROOT/tools/shipmap/ext_data/sp_turbolift.dat" "$ROOT/build/ship/files/ext_data/"

python3 "$ROOT/tools/shipmap/stitch.py" --decks "$DECKS" --decks "$ROOT/build/ship/generated" --out "$ROOT/build/ship/voyager.map" \
  --report "$ROOT/build/ship/stitch-report.json"
# Visibility and lighting take about three minutes for the whole ship; --fast skips them.
"$ROOT/scripts/build-map.sh" "$ROOT/build/ship/voyager.map" --name voyager --out "$OUT" --allow-missing-shaders \
  --entities "$ROOT/build/ship/voyager.ents" --files "$ROOT/build/ship/files" "${PASSES[@]}"
echo "load it with:  +set g_shipDeckPitch 3072 +map voyager   (the pitch tells scripts which deck they are on)"
