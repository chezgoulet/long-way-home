#!/usr/bin/env bash
# Which of the two cg_public.h the SP bridge compiles against, pinned (docs/evidence/the-bridge-traps.md).
#
#   scripts/bridge-header-check.sh
#
# Two same-named headers carry different snapshot limits: the module's (efgame/src/cgame/cg_public.h)
# is MAX_ENTITIES_IN_SNAPSHOT 1024, and the engine's network client
# (EFAndroid-SP/app/jni/efcode/cgame/cg_public.h) is 256. The bridge includes "../cgame/cg_public.h",
# so which one it gets is decided by include-path order, not by the source -- and that is silent: the
# wrong one compiles, halves snapshot_t, and corrupts the in-process copy. Patch 0022 pins it with a
# preprocessor guard. This runs the preprocessor over the bridge's own source twice: with the bridge's
# include path (the module's header) the guard must pass, and with the engine's cgame added ahead of it
# the guard must fire. Compile-time only: no build, no game data.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
UP="${UPSTREAM_DIR:-$(dirname "$ROOT")/upstream}"
SP="$UP/EFAndroid-SP/app/jni/sp/sp_bridge.cpp"
EFG="$UP/efgame"
EF="$UP/EFAndroid-SP/app/jni/efcode"
MODULE_CG="$EFG/src/cgame/cg_public.h"
ENGINE_CG="$EF/cgame/cg_public.h"
ERR="$(mktemp)"
trap 'rm -f "${ERR:?}"' EXIT

[ -f "$SP" ] || { echo "no upstream checkout at $UP -- run scripts/bootstrap-upstream.sh" >&2; exit 1; }
[ -f "$MODULE_CG" ] || { echo "missing $MODULE_CG" >&2; exit 1; }
[ -f "$ENGINE_CG" ] || { echo "missing $ENGINE_CG" >&2; exit 1; }
command -v g++ >/dev/null 2>&1 || { echo "g++ not found" >&2; exit 1; }

fail() { echo "FAIL  $1"; exit 1; }

echo "==> the two headers"
mod="$(grep -m1 'MAX_ENTITIES_IN_SNAPSHOT' "$MODULE_CG" | tr -s ' \t' ' ')"
eng="$(grep -m1 'MAX_ENTITIES_IN_SNAPSHOT' "$ENGINE_CG" | tr -s ' \t' ' ')"
printf '    %s :%s\n' "${MODULE_CG#"$UP/"}" "$mod"
printf '    %s :%s\n' "${ENGINE_CG#"$UP/"}" "$eng"
echo "$mod" | grep -q 'MAX_ENTITIES_IN_SNAPSHOT 1024' || fail "the module's cg_public.h does not carry 1024"
echo "$eng" | grep -q 'MAX_ENTITIES_IN_SNAPSHOT 256' || fail "the engine's cg_public.h does not carry 256"
grep -q '#if MAX_ENTITIES_IN_SNAPSHOT != 1024' "$SP" \
  || fail "sp_bridge.cpp carries no guard on which cg_public.h it compiled against"

# The include path the sp_game_bridge target gives sp_bridge.cpp (engine/CMakeLists.txt).
INC=(-I "$ROOT/engine/platform" -I "$EFG/src/game" -I "$EFG/src/cgame" -I "$EFG/src/ui"
     -I "$EFG/src/client" -I "$EFG/src/renderer" -I "$EFG/src/qcommon"
     -I "$UP/EFAndroid-SP/app/jni/sp" -include "$EFG/android/ef_android_compat.h")

echo "==> the bridge as built: the module's header resolves first, and the guard passes"
if ! g++ -E "${INC[@]}" "$SP" >/dev/null 2>"$ERR"; then
  sed 's/^/    /' "$ERR" | head -5
  fail "the bridge did not preprocess with the bridge's own include path"
fi

echo "==> the wrong header: the engine's cgame ahead of the module's, and the guard fires"
if g++ -E -I "$EF/cgame" "${INC[@]}" "$SP" >/dev/null 2>"$ERR"; then
  fail "the guard did not fire when the engine's 256 cg_public.h was used: the collision is unguarded"
fi
grep -q '#error "sp_bridge.cpp must compile against the module' "$ERR" \
  || { sed 's/^/    /' "$ERR" | head -5; fail "the bridge failed for the wrong reason"; }
grep -m1 '#error "sp_bridge.cpp' "$ERR" | sed 's/^/    /'

echo "PASS  the module's cg_public.h (1024) resolves for the bridge and the guard holds; pointing the"
echo "      include path at the engine's network cg_public.h (256) fails the build instead of silently"
echo "      halving snapshot_t"
