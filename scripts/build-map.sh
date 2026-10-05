#!/usr/bin/env bash
# Headless map build: .map -> q3map2 -> structural check -> a .pk3 the engine loads.
#
#   scripts/build-map.sh MAP_SOURCE [--name NAME] [--out DIR] [--q3map2 PATH] [--light] [--allow-missing-shaders]
#
# MAP_SOURCE  a .map file (from an editor, or tools/mapgen/mapgen.py)
# --name      the map's name in game (default: the file's name)
# --out       where the .pk3 goes (default: build/home/baseEF, where the engine finds it)
# --q3map2    the compiler (default: on PATH, else the copy scripts/fetch-map-tools.sh fetched)
# --light     also run the visibility and lighting passes (slower; a test room does not need them)
# --entities FILE
#             entities to add to the compiled map without compiling them (tools/shipmap/inject.py)
# --allow-missing-shaders
#             report shaders that did not resolve and carry on. For published sources, which name a
#             few textures the shipped game no longer has; never for a map of our own.
#
# Then, in game:  map NAME      (`map`, not `spmap`: see docs/evidence/g2-authored-space-loads.md)
# Navigation needs no step here: the engine bakes maps/NAME.nav from the map's waypoints on first load.
#
# Three faults this guards against, each of which once produced a useless BSP and a zero exit:
#   * the compiler finding no assets, because it looks for a directory spelled exactly `baseEF`
#     and the installation spells it otherwise -- build/baseEF is the correctly-cased link
#   * a shader that did not resolve, which q3map2 reports as "Couldn't find image for shader"
#     and then carries on
#   * a BSP with empty brush, shader or surface lumps, which only check-bsp.py catches

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC=""; NAME=""; OUT="$ROOT/build/home/baseEF"; Q3MAP2=""; LIGHT=0; ALLOW_MISSING=0; EXTRA_ENTS=""
while [ $# -gt 0 ]; do
  case "$1" in
    --name)   NAME="$2"; shift 2 ;;
    --out)    OUT="$2"; shift 2 ;;
    --q3map2) Q3MAP2="$2"; shift 2 ;;
    --light)  LIGHT=1; shift ;;
    --allow-missing-shaders) ALLOW_MISSING=1; shift ;;
    --entities) EXTRA_ENTS="$2"; shift 2 ;;
    -*)       echo "unknown argument: $1" >&2; exit 2 ;;
    *)        SRC="$1"; shift ;;
  esac
done
[ -n "$SRC" ] && [ -f "$SRC" ] || { echo "usage: build-map.sh MAP_SOURCE [--name NAME] [--out DIR] [--q3map2 PATH] [--light]" >&2; exit 2; }
[ -n "$NAME" ] || NAME="$(basename "$SRC" .map)"
case "$NAME" in *[!A-Za-z0-9_-]*|"") echo "map name '$NAME': letters, digits, '_' and '-' only" >&2; exit 2 ;; esac

if [ -z "$Q3MAP2" ]; then
  Q3MAP2="$(command -v q3map2 || true)"
  [ -n "$Q3MAP2" ] || Q3MAP2="$(find "$ROOT/build/tools" -path '*/usr/bin/q3map2' 2>/dev/null | sort | tail -1)"
fi
[ -n "$Q3MAP2" ] && [ -x "$Q3MAP2" ] || { echo "q3map2 not found: run scripts/fetch-map-tools.sh, or pass --q3map2" >&2; exit 1; }
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }

WORK="$(mktemp -d)"
trap 'rm -rf "${WORK:?}"' EXIT
mkdir -p "$WORK/maps"
cp "$SRC" "$WORK/maps/$NAME.map"
LOG="$WORK/q3map2.log"
COMMON=(-fs_basepath "$ROOT/build" -fs_game baseEF -game quake3)

compile() {
  echo "==> q3map2 $1"
  if ! "$Q3MAP2" "${COMMON[@]}" "$@" "$WORK/maps/$NAME.map" >>"$LOG" 2>&1; then
    tail -20 "$LOG" >&2
    echo "q3map2 $1 failed" >&2; exit 1
  fi
}
compile -meta
if [ "$LIGHT" -eq 1 ]; then
  compile -vis -fast
  compile -light -fast
fi

if grep -q "Couldn't find image for shader" "$LOG"; then
  missing="$(grep "Couldn't find image for shader" "$LOG" | sort -u)"
  if [ "$ALLOW_MISSING" -eq 1 ]; then
    echo "    $(echo "$missing" | wc -l) shader(s) did not resolve (allowed):"
    echo "$missing" | sed 's/.*shader /      /' | head -20
  else
    echo "$missing" | head -10 >&2
    echo "shaders did not resolve: the map would load untextured or not at all" >&2; exit 1
  fi
fi
[ -f "$WORK/maps/$NAME.bsp" ] || { echo "q3map2 exited 0 but wrote no BSP" >&2; tail -20 "$LOG" >&2; exit 1; }
python3 "$ROOT/tools/mapgen/check-bsp.py" "$WORK/maps/$NAME.bsp"
if [ -n "$EXTRA_ENTS" ]; then
  python3 "$ROOT/tools/shipmap/inject.py" "$WORK/maps/$NAME.bsp" "$EXTRA_ENTS"
fi

mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd)"   # absolute: the archive is written from inside the work directory
PK3="$OUT/longway_$NAME.pk3"
rm -f "${PK3:?}"
(cd "$WORK" && zip -q -X "$PK3" "maps/$NAME.bsp")
echo "wrote $PK3"
echo "in game: map $NAME"
