#!/usr/bin/env bash
# The published decks share shader names, so a per-deck remap cannot address one of them
# (docs/evidence/the-bridge-traps.md). This is the measurement behind that boundary, and the guard on
# the two things it rests on: the renderer's RemapShader is by name (so a shared name moves every
# deck), and the generated decks LWH authored name their sections per deck (the supported way round).
#
#   scripts/deck-shader-check.sh
#
# Needs the GDK deck sources (scripts/fetch-gdk.sh) and the built ship's generated decks
# (scripts/build-ship.sh); the renderer check needs the upstream checkout (scripts/bootstrap-upstream.sh).
# Reads no game data and writes nothing.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
UP="${UPSTREAM_DIR:-$(dirname "$ROOT")/upstream}"
DECKS="$ROOT/build/gdk/maps/eliteforce_virtualvoyager_maps"
GENERATED="$ROOT/build/ship/generated"

fail() { echo "FAIL  $1"; exit 1; }

[ -d "$DECKS" ] || { echo "no deck sources at $DECKS -- run scripts/fetch-gdk.sh" >&2; exit 1; }
[ -d "$GENERATED" ] || { echo "no generated decks at $GENERATED -- run scripts/build-ship.sh" >&2; exit 1; }

echo "==> the published decks, by the shader names their surfaces use"
# The measured numbers: 190 shaders named by more than one deck, 15 by all ten.
python3 "$ROOT/tools/shipmap/shader_census.py" --decks "$DECKS" --min-shared 100 --min-all 10

echo
echo "==> the generated decks' own sections, which must be one deck each"
python3 "$ROOT/tools/shipmap/shader_census.py" --decks "$GENERATED" \
  --unique-pattern '^lwh/deck[0-9]+(wall[0-9]|floor)$'

echo
echo "==> the renderer's remap is by name (a shared name moves every deck)"
REMAPPY="$UP/EFAndroid-SP/app/jni/efcode/renderervk/tr_shader.c"
if [ -f "$REMAPPY" ]; then
  grep -q 'remap all the shaders with the given name' "$REMAPPY" \
    || fail "RE_RemapShader no longer documents a by-name, whole-table remap: re-read the boundary"
  grep -m1 'remap all the shaders with the given name' "$REMAPPY" | sed 's/^[[:space:]]*/    /'
else
  echo "    (no upstream checkout at $UP; the renderer's remap was not read)"
fi

echo "PASS  the published decks share shader names across decks (hall/hallfloor3 and voyager/wall2 on"
echo "      all ten), so RemapShader -- by name -- cannot address one of them alone; the generated decks"
echo "      carry deck-unique section shaders, which is the mechanism the engine does offer"
