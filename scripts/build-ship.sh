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

# The five decks nobody ever published each get a deck source from a brief. Decks 7, 12, 13 and 14
# are re-dressed from one published source -- tour/deck11, main engineering -- because each brief names
# the room to copy. Deck 6 is the fifth and last, and the only one that *composes*: its brief names
# three sources, so a quarters slice (tour/deck09), an armory room (_brig) and a holodeck arch (a
# _holodeck_* programme map) are composited into one deck
# (tools/shipmap/dressdeck06.py; docs/locations/deck06-holodecks.brief.md).
BRIG="$ROOT/build/gdk/maps/brig-map/_brig.map"
HOLODECK_SRC="$ROOT/build/gdk/maps/eliteforce_holodeck_maps/Tour/_holodeck_firingrange.map"
mkdir -p "$ROOT/build/ship/files/scripts" "$ROOT/build/lwhshaders/scripts"
python3 "$ROOT/tools/shipmap/dressdeck06.py" --quarters "$DECKS/deck09.map" --armory "$BRIG" \
  --holodeck "$HOLODECK_SRC" --out "$ROOT/build/ship/generated" \
  --report "$ROOT/build/ship/deck06-dress.json" --census "$ROOT/build/ship/deck06-census.json" \
  --detail-shader "$ROOT/build/ship/files/scripts/lwh_detail06.shader"
cp "$ROOT/build/ship/files/scripts/lwh_detail06.shader" "$ROOT/build/lwhshaders/scripts/"
python3 "$ROOT/tools/shipmap/dressdeck12.py" --deck11 "$DECKS/deck11.map" --out "$ROOT/build/ship/generated" \
  --report "$ROOT/build/ship/deck12-dress.json" --census "$ROOT/build/ship/deck12-census.json"
python3 "$ROOT/tools/shipmap/dressdeck13.py" --deck11 "$DECKS/deck11.map" --out "$ROOT/build/ship/generated" \
  --report "$ROOT/build/ship/deck13-dress.json" --census "$ROOT/build/ship/deck13-census.json"
python3 "$ROOT/tools/shipmap/dressdeck14.py" --deck11 "$DECKS/deck11.map" --out "$ROOT/build/ship/generated" \
  --report "$ROOT/build/ship/deck14-dress.json" --census "$ROOT/build/ship/deck14-census.json"
# Deck 7's and deck 6's copied furniture is detail geometry (it must not add vis clusters: the ship is
# at q3map2's MAX_MAP_VISCLUSTERS ceiling), which needs per-material shader aliases. They are written
# for the pak (the engine renders them) and mirrored into a compiler-only game dir (q3map2 must see
# them to set the detail flag; build-map's --q3game).
mkdir -p "$ROOT/build/ship/files/scripts" "$ROOT/build/lwhshaders/scripts"
python3 "$ROOT/tools/shipmap/dressdeck07.py" --deck11 "$DECKS/deck11.map" --out "$ROOT/build/ship/generated" \
  --report "$ROOT/build/ship/deck07-dress.json" --census "$ROOT/build/ship/deck07-census.json" \
  --detail-shader "$ROOT/build/ship/files/scripts/lwh_detail07.shader"
cp "$ROOT/build/ship/files/scripts/lwh_detail07.shader" "$ROOT/build/lwhshaders/scripts/"
# Every material a re-dress uses must already exist in the game (the brief's section 4: no new art).
python3 "$ROOT/tools/shipmap/parts_census.py" --map "$ROOT/build/ship/generated/deck06.map" \
  --game "$ROOT/build/baseEF" --census "$ROOT/build/ship/deck06-census.json" \
  --shader "$ROOT/build/ship/files/scripts/lwh_detail06.shader"
python3 "$ROOT/tools/shipmap/parts_census.py" --map "$ROOT/build/ship/generated/deck12.map" \
  --game "$ROOT/build/baseEF" --census "$ROOT/build/ship/deck12-census.json"
python3 "$ROOT/tools/shipmap/parts_census.py" --map "$ROOT/build/ship/generated/deck13.map" \
  --game "$ROOT/build/baseEF" --census "$ROOT/build/ship/deck13-census.json"
python3 "$ROOT/tools/shipmap/parts_census.py" --map "$ROOT/build/ship/generated/deck14.map" \
  --game "$ROOT/build/baseEF" --census "$ROOT/build/ship/deck14-census.json"
python3 "$ROOT/tools/shipmap/parts_census.py" --map "$ROOT/build/ship/generated/deck07.map" \
  --game "$ROOT/build/baseEF" --census "$ROOT/build/ship/deck07-census.json" \
  --shader "$ROOT/build/ship/files/scripts/lwh_detail07.shader"

# What goes in the pak beside the map: the turbolift's deck list for fifteen decks. It is ours,
# authored under tools/shipmap/data (the game's own list covers only the ten published decks), and
# it must override the retail ext_data/sp_turbolift.dat in the pak search order.
mkdir -p "$ROOT/build/ship/files/ext_data"
cp "$ROOT/tools/shipmap/data/sp_turbolift.dat" "$ROOT/build/ship/files/ext_data/"
# The one config a Long Way Home run needs (gate G1). It goes into the pak so that any installation
# carrying the ship also carries the way in: the main menu's "Long Way Home" line execs it, and so
# does scripts/run-lwh.sh, and its values therefore live in exactly one place.
mkdir -p "$ROOT/build/ship/files"
cp "$ROOT/configs/lwh-start.cfg" "$ROOT/build/ship/files/"

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
  --entities "$ROOT/build/ship/voyager.ents" --files "$ROOT/build/ship/files" --q3game lwhshaders "${PASSES[@]}"
echo "load it with:  +set g_shipDeckPitch 3072 +map voyager   (the pitch tells scripts which deck they are on)"
