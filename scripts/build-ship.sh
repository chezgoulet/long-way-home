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

# The decks nobody ever published get a generated placeholder each (tools/shipmap/gendeck.py) until
# their own brief is built. Decks 12, 13 and 14 are different: each is re-dressed from a published
# source, because its brief names the room to copy -- tour/deck11, main engineering. Each is the
# pattern before it: dressdeck13.py is dressdeck12.py's copy with deck 13's program, and
# dressdeck14.py is deck 13's copy with deck 14's program (see the briefs under docs/locations/ and
# docs/authoring-a-location.md).
python3 "$ROOT/tools/shipmap/gendeck.py" --deck 6 --deck 7 --out "$ROOT/build/ship/generated"
python3 "$ROOT/tools/shipmap/dressdeck12.py" --deck11 "$DECKS/deck11.map" --out "$ROOT/build/ship/generated" \
  --report "$ROOT/build/ship/deck12-dress.json" --census "$ROOT/build/ship/deck12-census.json"
python3 "$ROOT/tools/shipmap/dressdeck13.py" --deck11 "$DECKS/deck11.map" --out "$ROOT/build/ship/generated" \
  --report "$ROOT/build/ship/deck13-dress.json" --census "$ROOT/build/ship/deck13-census.json"
python3 "$ROOT/tools/shipmap/dressdeck14.py" --deck11 "$DECKS/deck11.map" --out "$ROOT/build/ship/generated" \
  --report "$ROOT/build/ship/deck14-dress.json" --census "$ROOT/build/ship/deck14-census.json"
# Every material a re-dress uses must already exist in the game (the brief's section 4: no new art).
python3 "$ROOT/tools/shipmap/parts_census.py" --map "$ROOT/build/ship/generated/deck12.map" \
  --game "$ROOT/build/baseEF" --census "$ROOT/build/ship/deck12-census.json"
python3 "$ROOT/tools/shipmap/parts_census.py" --map "$ROOT/build/ship/generated/deck13.map" \
  --game "$ROOT/build/baseEF" --census "$ROOT/build/ship/deck13-census.json"
python3 "$ROOT/tools/shipmap/parts_census.py" --map "$ROOT/build/ship/generated/deck14.map" \
  --game "$ROOT/build/baseEF" --census "$ROOT/build/ship/deck14-census.json"

# What goes in the pak beside the map: the turbolift's deck list for fifteen decks. It is ours,
# authored under tools/shipmap/data (the game's own list covers only the ten published decks), and
# it must override the retail ext_data/sp_turbolift.dat in the pak search order.
mkdir -p "$ROOT/build/ship/files/ext_data"
cp "$ROOT/tools/shipmap/data/sp_turbolift.dat" "$ROOT/build/ship/files/ext_data/"
# The status panel's shader and its placeholder image: the ship's live state is drawn onto this
# surface (gi.UpdatePanelImage), so the shader and a real image at the same size must be present.
mkdir -p "$ROOT/build/ship/files/scripts" "$ROOT/build/ship/files/gfx/lwh"
cp "$ROOT/tools/shipmap/data/lwh_panel.shader" "$ROOT/build/ship/files/scripts/"
cp "$ROOT/tools/shipmap/data/lwh_borg.shader" "$ROOT/build/ship/files/scripts/"
python3 "$ROOT/tools/shipmap/panelart.py" "$ROOT/build/ship/files/gfx/lwh/panel.tga"

python3 "$ROOT/tools/shipmap/stitch.py" --decks "$DECKS" --decks "$ROOT/build/ship/generated" --out "$ROOT/build/ship/voyager.map" \
  --report "$ROOT/build/ship/stitch-report.json"
# Visibility and lighting take about three minutes for the whole ship; --fast skips them.
"$ROOT/scripts/build-map.sh" "$ROOT/build/ship/voyager.map" --name voyager --out "$OUT" --allow-missing-shaders \
  --entities "$ROOT/build/ship/voyager.ents" --files "$ROOT/build/ship/files" "${PASSES[@]}"
echo "load it with:  +set g_shipDeckPitch 3072 +map voyager   (the pitch tells scripts which deck they are on)"
