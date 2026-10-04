#!/usr/bin/env bash
# Build the NetRadiant / q3map2 game pack for Elite Force.
#
#   make-gamepack.sh --gdk DIR --data DIR [--out DIR]
#
#     --gdk    extracted Game Development Kit (has Tools/SP_entities.def, BaseEf/scripts/)
#     --data   the Elite Force installation (BaseEF with the paks)
#     --out    where to write eliteforce.game (default: <repo>/build/gamepack)
#
# Neither input is vendored in this repository: both are supplied at build time, so the pack is
# assembled from the owner's own copy of the game and the freely-distributed official tools.
#
# NetRadiant finds a game by a directory named <game>.game containing:
#     main/<game>_entities.def     the editor's entity dictionary (ours: the 214 SP entities)
#     main/default_shaderlist.txt  which shader scripts the editor loads
#     default_build_menu.xml       the compile command presets
#     game.xlink                   documentation links

set -euo pipefail

GDK=""; DATA=""; OUT=""
while [ $# -gt 0 ]; do
  case "$1" in
    --gdk)  GDK="$2"; shift 2 ;;
    --data) DATA="$2"; shift 2 ;;
    --out)  OUT="$2"; shift 2 ;;
    *) echo "unknown argument: $1" >&2; exit 2 ;;
  esac
done
[ -n "$GDK" ] && [ -n "$DATA" ] || { echo "usage: make-gamepack.sh --gdk DIR --data DIR [--out DIR]" >&2; exit 2; }
[ -n "$OUT" ] || OUT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)/build/gamepack"

PACK="$OUT/eliteforce.game"
rm -rf "$PACK"; mkdir -p "$PACK/main"

# ---- entity dictionary: the authoritative SP entity set from the GDK ----
DEF=""
for candidate in "$GDK/Tools/SP_entities.def" "$GDK/SP_entities.def"; do
  [ -f "$candidate" ] && { DEF="$candidate"; break; }
done
[ -n "$DEF" ] || { echo "SP_entities.def not found under $GDK" >&2; exit 1; }
cp "$DEF" "$PACK/main/eliteforce_entities.def"
ENTITIES=$(grep -c '^/\*QUAKED' "$PACK/main/eliteforce_entities.def" || true)

# ---- shader list: what the editor shows, taken from the game's own scripts ----
BASE=""
for candidate in "$DATA/baseEF" "$DATA/BaseEF"; do
  [ -d "$candidate" ] && { BASE="$candidate"; break; }
done
if [ -n "$BASE" ]; then
  : > "$PACK/main/default_shaderlist.txt"
  for p in "$BASE"/pak*.pk3; do
    [ -f "$p" ] || continue
    unzip -l "$p" 2>/dev/null | awk '{print $4}' | grep -E '^scripts/.*\.shader$' \
      | sed 's|^scripts/||; s|\.shader$||' >> "$PACK/main/default_shaderlist.txt" || true
  done
  sort -u -o "$PACK/main/default_shaderlist.txt" "$PACK/main/default_shaderlist.txt"
fi
SHADERS=$(wc -l < "$PACK/main/default_shaderlist.txt" 2>/dev/null || echo 0)

# ---- compile presets ----
cat > "$PACK/default_build_menu.xml" <<'XML'
<?xml version="1.0" encoding="iso-8859-1" standalone="yes"?>
<project version="2.0">
  <var name="basepath"/>
  <build name="Elite Force: BSP (fast)">
    <command>[RadiantPath]q3map2.[ExecutableType] -v -fs_game baseEF -fs_basepath "[BasePath]" -meta "[MapFile]"</command>
    <command>[RadiantPath]q3map2.[ExecutableType] -v -fs_game baseEF -fs_basepath "[BasePath]" -vis -fast "[MapFile]"</command>
    <command>[RadiantPath]q3map2.[ExecutableType] -v -fs_game baseEF -fs_basepath "[BasePath]" -light -fast "[MapFile]"</command>
  </build>
  <build name="Elite Force: BSP (final)">
    <command>[RadiantPath]q3map2.[ExecutableType] -v -fs_game baseEF -fs_basepath "[BasePath]" -meta -samplesize 8 "[MapFile]"</command>
    <command>[RadiantPath]q3map2.[ExecutableType] -v -fs_game baseEF -fs_basepath "[BasePath]" -vis "[MapFile]"</command>
    <command>[RadiantPath]q3map2.[ExecutableType] -v -fs_game baseEF -fs_basepath "[BasePath]" -light -samples 3 "[MapFile]"</command>
    <command>echo "Navigation data (.nav) is NOT built by q3map2 -- a space without it cannot hold crew."</command>
  </build>
</project>
XML

cat > "$PACK/game.xlink" <<'XML'
<?xml version="1.0" encoding="iso-8859-1" standalone="yes"?>
<links>
  <item name="Elite Force entity dictionary (SP)" url="main/eliteforce_entities.def"/>
</links>
XML

echo "game pack: $PACK"
echo "  entity classes : $ENTITIES"
echo "  shader scripts : $SHADERS"
echo "  files:"
find "$PACK" -type f | sed 's|^|    |'
