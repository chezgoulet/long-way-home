#!/usr/bin/env bash
# The appearance derivation: the face comes from the same seed as the person.
#
#   scripts/appearance-check.sh
#
# Two halves, because they prove different things:
#   1. the unit transcript (test_ship_core --appearance): the pool, the species split, the canon set
#      in one place, the named crew left unset, and a lineup of generated crew -- no game data needed;
#   2. the rendered picture: one headless run re-dresses the Starfleet NPCs on a deck from the crew
#      records and photographs the rank, because whether the crew *read* as people is the owner's
#      judgement and only the picture answers it.
#
# Needs the built modules and (for half 2) the game data and xvfb-run. Writes under build/.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
MAP="${MAP:-tour/deck04}"

fail() { echo "FAIL  $1"; exit 1; }

# ---- 1. the transcript ------------------------------------------------------------------------
WORK="$(mktemp -d)"
trap 'rm -rf "${WORK:?}"' EXIT
echo "==> the appearance derivation: the pool, the canon set, the lineup"
cmake -S tests/ship -B "$WORK" -DCMAKE_BUILD_TYPE=Debug >/dev/null
cmake --build "$WORK" -j"$(nproc)" >/dev/null
"$WORK/test_ship_core" --appearance | tee "$WORK/appearance.txt" | sed 's/^/    /'

grep -q '^the shipped head pool: [0-9][0-9]* entries' "$WORK/appearance.txt" \
  || fail "the pool was not enumerated"
grep -q 'Vulcan' "$WORK/appearance.txt" || fail "the Vulcan pool was not shown"
grep -q 'Human' "$WORK/appearance.txt" || fail "the human pool was not shown"
grep -q 'canon faces, named in one place' "$WORK/appearance.txt" || fail "the canon set was not named"
grep -q 'planted failure: CanonFaceInPool({"Garren","janeway"}) -> janeway' "$WORK/appearance.txt" \
  || fail "the planted canon face was not caught by name"
grep -q 'a lineup of generated crew' "$WORK/appearance.txt" || fail "no lineup was printed"
grep -q 'NAMED  type' "$WORK/appearance.txt" || fail "named crew were not shown as untouched"

# ---- 2. the picture ---------------------------------------------------------------------------
if [ ! -d "$ROOT/build/baseEF" ]; then
  echo "PASS  (half 1) the derivation is enumerated, canon-free and deterministic;"
  echo "      half 2 skipped: no game data at build/baseEF"
  exit 0
fi
command -v xvfb-run >/dev/null 2>&1 || fail "xvfb-run not found"

OUT="$ROOT/build/appearance-lineup.out"
SHOT="$GAME_DIR/screenshots/lwh_appearance_lineup.tga"
mkdir -p "$GAME_DIR/screenshots"
rm -f "$SHOT"
find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete

echo
echo "==> the rendered lineup: the Starfleet crew on $MAP, re-dressed from the records"
SDL_AUDIODRIVER=dummy timeout 180 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipDayScale 1 +set g_shipTest 90 +map "$MAP" >"$OUT" 2>&1 || true
grep -h 'appearance lineup: Crewman' "$OUT" | sed 's/^SHIP: /    /' | sed 's/ walks as head/ ->/' || true

grep -q 'appearance lineup: lining up [1-9][0-9]* of' "$OUT" || fail "no crew were lined up (see $OUT)"
grep -q 'appearance lineup: Crewman .* walks as head' "$OUT" || fail "no derived face reached a body"
# The residual is gone: a generated crew member is never dressed as the game's default head.
if grep -q 'appearance lineup: Crewman .* walks as head munro/default' "$OUT"; then
  fail "a generated crew member still wears munro/default"
fi
# The canon faces never appear among the derived heads.
if grep -qE 'appearance lineup: Crewman .* walks as head (janeway|chakotay|tuvok|tuvok_h|paris|kim|torres|doctor|seven|neelix)/' "$OUT"; then
  grep -E 'appearance lineup: Crewman .* walks as head' "$OUT"
  fail "a canon face appeared on a generated crew member"
fi
[ -f "$SHOT" ] || fail "no screenshot of the lineup"
echo "PASS  the face is derived from the record (never stored), no canon face is generated, and"
echo "      the rank was photographed: $SHOT"
