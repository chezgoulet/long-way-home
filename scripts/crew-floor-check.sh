#!/usr/bin/env bash
# The crew layer's floor is per deck (docs/g3-reactive-crew.md).
#
#   scripts/crew-floor-check.sh
#
# One headless run on the merged ship. The harness (g_shipTest 88) stands the player on deck 12,
# where the crew stand far below the ship's lowest post but on their own deck's floor, and shows
# that no member is reported fallen; it then drops one member 3000 units below its deck, and shows
# that this one is caught -- against deck 12's own floor, named in the line. The two halves are one
# run, because the honest question is not "does the check fire" but "does it fire only when it
# should". Needs the built ship (build-ship.sh) and the built engine + modules. Writes under
# build/g3-home.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
PK3="$ROOT/build/ship/out/longway_voyager.pk3"
OUT="$HOME_DIR/crew-floor.out"
[ -f "$PK3" ] || { echo "no merged ship at $PK3 -- run scripts/build-ship.sh" >&2; exit 1; }
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }
mkdir -p "$GAME_DIR"
cp "$PK3" "$GAME_DIR/"
trap 'find "$GAME_DIR" -maxdepth 1 -name "longway_voyager.pk3" -delete' EXIT
rm -f "$OUT"

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> crew floor"
SDL_AUDIODRIVER=dummy timeout 200 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set com_hunkMegs 768 +set s_useOpenAL 0 +set g_ship 1 +set g_crew 1 +set g_crewFromShip 1 \
    +set g_shipDeckPitch 3072 +set g_shipTest 88 +map voyager >"$OUT" 2>&1 || true

fail() { echo "FAIL  $1"; exit 1; }
grep -h 'floor test\|fell out of the world\|has no posts' "$OUT" | sed 's/^EFSP: /    /' || true

grep -q 'SHIP: floor test: on deck 12' "$OUT" || fail "the harness did not reach deck 12 (see $OUT)"
grep -qE 'deck 12: the ship has [1-9][0-9]* here' "$OUT" || fail "no crew were embodied on deck 12 to test"

python3 - "$OUT" <<'PY'
import re, sys
text = open(sys.argv[1]).read()
before, sep, after = text.partition("SHIP: floor test: dropped")
if not sep:
    print("FAIL  no member was dropped (see %s)" % sys.argv[1]); sys.exit(1)

# Standing: deck 12's crew are hundreds of metres below the ship's lowest post, and none of them
# is reported fallen.
if "fell out of the world" in before:
    print("FAIL  a member was reported fallen before it was dropped:")
    for line in before.splitlines():
        if "fell out of the world" in line:
            print("    " + line.strip()); break
    sys.exit(1)

# Dropped 3000 below its deck: caught, against that deck's floor. 3000 > OUT_OF_WORLD_DROP (2048).
m = re.search(r"(\S+) to \((-?\d+) (-?\d+) (-?\d+)\), \d+ below", after)
f = re.search(r"(\S+) fell out of the world at \((-?\d+) (-?\d+) (-?\d+)\) \(deck (\d+), floor (-?\d+)\)", after)
if not m or not f:
    print("FAIL  the dropped member was not caught against its own deck's floor"); sys.exit(1)
if m.group(1) != f.group(1):
    print("FAIL  %s was dropped but %s was reported fallen" % (m.group(1), f.group(1))); sys.exit(1)
if int(f.group(5)) != 12:
    print("FAIL  the member was judged against deck %s, not deck 12" % f.group(5)); sys.exit(1)
if int(f.group(4)) >= int(f.group(6)) - 2048:
    print("FAIL  the drop (%s) was not more than 2048 below the floor (%s)" % (f.group(4), f.group(6)))
    sys.exit(1)
print("PASS  no member on deck 12 was fallen while it stood (floor %s); dropped to z %s, %s was caught"
      % (f.group(6), f.group(4), f.group(1)))
PY

echo "PASS  the crew layer's floor is the member's own deck's, and it fires only below that"
