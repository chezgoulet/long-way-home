#!/usr/bin/env bash
# Gate S5, as far as it is built: the crew on the player's deck are the people the ship's routine
# has there -- they come and go with it, and they walk to a place.
#
#   scripts/s5-check.sh
#
# Two headless engine runs on deck 4, told it is deck 2 (the mess deck), where the ship's routine
# sends each watch for a meal either side of its duty:
#   routine   the ship's clock run fast (a day in three minutes). The number embodied must follow the
#             number the ship has aboard, up to the cap, through at least two meals and the hours
#             between them.
#   walk      the clock at its normal rate, so the first meal lasts a minute. Most of those
#             embodied must reach their place on the deck in that time.
#
# Not checked, because not built: real stations for places, the merged ship, more than ten at once.
# Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }
mkdir -p "$GAME_DIR"

engine() {
  local label="$1" seconds="$2"; shift 2
  find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
  echo "==> $label"
  # The run has no end of its own: it is watched for a fixed time and stopped.
  SDL_AUDIODRIVER=dummy timeout "$seconds" xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
      +set s_useOpenAL 0 +set g_ship 1 +set g_crew 1 +set g_crewDebug 1 +set g_crewFromShip 1 +set g_crewDeck 2 "$@" \
      +map tour/deck04 >"$HOME_DIR/s5-$label.out" 2>&1 || true
}

engine routine 200 +set g_shipDayScale 480
engine walk     75 +set g_shipDayScale 60

fail() { echo "FAIL  $1"; exit 1; }

grep -h '^CREW: ship time' "$HOME_DIR/s5-routine.out" | sed 's/^CREW: /    /' || true
python3 - "$HOME_DIR/s5-routine.out" "$HOME_DIR/s5-walk.out" <<'PY'
import re, sys
rows = [tuple(map(int, m.groups())) for m in re.finditer(
    r"^CREW: ship time \d+:\d+, deck \d+: the ship has (\d+) here \(showing up to (\d+)\), (\d+) embodied", open(sys.argv[1]).read(), re.M)]
if len(rows) < 4:
    print(f"FAIL  the routine run reported only {len(rows)} change(s) of who is aboard"); sys.exit(1)
# Someone due to leave stays until the player is not looking at them, so the count may lag a
# change. What must hold: it never exceeds the cap, and by the end of each spell -- a run of reports
# with the same number aboard -- it has settled on the ship's number.
over = [r for r in rows if r[2] > r[1]]
if over:
    print(f"FAIL  more crew embodied than the cap allows: {over[:3]}"); sys.exit(1)
spells = []
for r in rows:
    if spells and (spells[-1][0] > 0) == (r[0] > 0):
        spells[-1] = r
    else:
        spells.append(r)
unsettled = [r for r in spells[:-1] if r[2] != min(r[0], r[1])]   # the last spell may be cut short by the run ending
if unsettled:
    print(f"FAIL  embodied crew did not settle on the ship's count: {unsettled[:3]}"); sys.exit(1)
meals = sum(1 for r in spells if r[0] > 0)
empty = sum(1 for r in spells if r[0] == 0)
if meals < 2 or empty < 2:
    print(f"FAIL  expected at least two meals and two empty spells, saw {meals} and {empty}"); sys.exit(1)
print(f"PASS  through {meals} meals and {empty} spells between them, the crew embodied were the crew the ship had aboard")

walk = open(sys.argv[2]).read()
shown = [int(m.group(1)) for m in re.finditer(r"\), (\d+) embodied", walk)]
shown = shown[0] if shown else 0
arrived = len(set(re.findall(r"^CREW: (lwh_crew_\d+) at post", walk, re.M)))
failed = len(set(re.findall(r"CREW: (lwh_crew_\d+) failed to reach", walk)))
print(f"INFO  of {shown} embodied, {arrived} reached their place within the meal hour and {failed} gave up")
if shown == 0 or arrived * 2 < shown:
    print("FAIL  fewer than half of the embodied crew reached their place"); sys.exit(1)
print("PASS  most of the crew embodied walked to their place on the deck")
PY
