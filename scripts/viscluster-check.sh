#!/usr/bin/env bash
# The vis-cluster ceiling is q3map2's, not the engine's (docs/evidence/the-bridge-traps.md). q3map2
# refuses a map past MAX_MAP_VISCLUSTERS (16384) with "MAX_MAP_VISIBILITY exceeded", so the lever for
# the next full-detail deck is a rebuild of a compiler we do not own. This reads the merged ship's own
# count from the packed artifact's BSP lump 16 (the visibility lump) and reports how much of the cap it
# spends. It reports; it does not fork the compiler.
#
#   scripts/viscluster-check.sh [PK3]
#
# Needs the built ship (scripts/build-ship.sh) and python3 + unzip. Reads the packed pk3, never the
# build directory. Writes nothing.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PK3="${1:-$ROOT/build/ship/out/longway_voyager.pk3}"
WORK="$(mktemp -d)"
trap 'rm -rf "${WORK:?}"' EXIT

fail() { echo "FAIL  $1"; exit 1; }

[ -f "$PK3" ] || { echo "no merged ship at $PK3 -- run scripts/build-ship.sh" >&2; exit 1; }
command -v unzip >/dev/null 2>&1 || fail "unzip not found"

echo "==> the merged ship's visibility lump (BSP lump 16), read from the packed artifact"
unzip -o -j "$PK3" '*.bsp' -d "$WORK" >/dev/null || fail "no .bsp inside $PK3"
python3 - "$WORK"/*.bsp <<'PY'
import struct, sys

CAP = 16384   # q3map2 MAX_MAP_VISCLUSTERS (q3map2.h, 0x4000) -- a compiler constant, not an engine one
for path in sys.argv[1:]:
    data = open(path, "rb").read()
    off, ln = struct.unpack_from("<II", data, 8 + 16 * 8)   # lump 16 = visibility
    nclusters, clusterbytes = struct.unpack_from("<ii", data, off)
    pct = 100.0 * nclusters / CAP
    print(f"    {path.rsplit('/', 1)[-1]}: portalclusters {nclusters} of {CAP} ({pct:.1f}%), "
          f"visdata bytes {ln}, clusterbytes {clusterbytes}")
    if nclusters <= 0:
        sys.exit("FAIL  the visibility lump has no clusters")
    if nclusters > CAP:
        sys.exit(f"FAIL  {nclusters} clusters is past MAX_MAP_VISCLUSTERS {CAP}")
PY

Q3MAP2="$(find "$ROOT/build/tools" -name q3map2 2>/dev/null | head -1)"
if [ -n "$Q3MAP2" ] && command -v strings >/dev/null 2>&1; then
  echo "==> the cap itself, in the compiler we run"
  strings "$Q3MAP2" > "$WORK/q3.strings" 2>/dev/null || true
  if grep -q 'MAX_MAP_VISIBILITY exceeded' "$WORK/q3.strings"; then
    grep -m1 'MAX_MAP_VISIBILITY exceeded' "$WORK/q3.strings" | sed 's/^/    /'
  else
    fail "the q3map2 binary does not carry the vis-cluster overflow message"
  fi
fi

echo "PASS  the ship is under q3map2's MAX_MAP_VISCLUSTERS; the next full-detail deck is priced against"
echo "      a compiler constant we do not own, not against an engine limit"
