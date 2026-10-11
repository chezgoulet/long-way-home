#!/usr/bin/env bash
# The doors: which of them stop a crew member, asked of the engine (the doors pass).
#
#   scripts/crew-door-check.sh
#
# Four headless runs, each quitting on its own:
#   census  the merged ship. Every door on the map is classified for a crew member from the engine's
#           own state (its touch trigger, its security flag), and the engine is then asked directly
#           about one ordinary and one security door: walk the door's own use path with a crew member
#           as the activator and watch the mover. An ordinary door opens; a security door refuses,
#           because the engine's gate is the player's security code and not the member's record. A
#           third kind -- a door with no touch trigger, which SP_func_door hands to a hand or a
#           script -- is asked of the layer's own rule and must be declined.
#   frozen  the same census with the layer's door opening switched off (`g_crewDoorOpen 0`): the old
#           behaviour, planted, so the check fails where the mechanism is broken -- a member whose
#           record gives security's access opens nothing.
#   walk    deck 4, at the six starting positions `crewgen suggest` proposes -- a suggestion the tool
#           warns cannot see doors. Each crew member walks to its post; anyone stopped is named with
#           the door that stopped them.
#   frozen2 the same walk with the layer's door opening switched off.
#
# Writes under build/g3-home. Needs the merged ship (scripts/build-ship.sh) and the built engine and
# modules. Nothing is written to the installation.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="$ROOT/build"
HOME_DIR="$BUILD/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
PK3="$BUILD/ship/out/longway_voyager.pk3"
DOORS="$BUILD/doors"
[ -f "$PK3" ] || { echo "no merged ship at $PK3 -- run scripts/build-ship.sh" >&2; exit 1; }
[ -d "$BUILD/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }
mkdir -p "$GAME_DIR" "$DOORS"

fail() { echo "FAIL  $1"; exit 1; }

engine() {
  local label="$1" limit="$2"; shift 2
  find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
  echo "==> $label"
  SDL_AUDIODRIVER=dummy timeout "$limit" xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
      +set com_hunkMegs 768 +set s_useOpenAL 0 +set g_ship 1 +set g_crew 1 "$@" >"$HOME_DIR/doors-$label.out" 2>&1 || true
}

# ---- the census: the merged ship ----------------------------------------------------------------
cp "$PK3" "$GAME_DIR/"
trap 'find "$GAME_DIR" -maxdepth 1 -name "longway_voyager.pk3" -delete' EXIT
census() {
  local label="$1"; shift
  engine "$label" 220 +set g_crewFromShip 1 +set g_shipDeckPitch 3072 +set g_crewDoors 1 "$@" +map voyager
  grep -hE '^CREW: doors: (access|[0-9]+ total)' "$HOME_DIR/doors-$label.out" | sed "s/^CREW: /    [$label] /" || true
}
census census +set g_crewDoorOpen 1
census census-frozen +set g_crewDoorOpen 0
CENSUS="$HOME_DIR/doors-census.out"
FROZEN="$HOME_DIR/doors-census-frozen.out"

grep -qE "CREW: doors: [1-9][0-9]* total" "$CENSUS" || fail "the census found no doors (see $CENSUS)"
grep -qE "live: ordinary '[^']*' for [^ ]+ -> OPENED" "$CENSUS" || fail "an ordinary door did not open for a crew member on the engine's own path"
grep -qE "layer: ordinary '[^']*' for [^ ]+ -> OPENED" "$CENSUS" || fail "the layer did not open an ordinary door for a crew member"
# ... and the layer's sample must not be the door the engine's own path was asked about. A func_door
# team shares one mover state, so a sample on the same team would read as opened by that earlier call
# and would prove nothing about the layer. This caught a real case: the first two ordinary doors on
# the merged ship are the two leaves of one double door.
ENG_ENT=$(sed -nE "s/.*live: ordinary '[^']*' for [^ ]+ -> [A-Z ]+ \(ent ([0-9]+)\).*/\1/p" "$CENSUS" | head -1)
LAY_ENT=$(sed -nE "s/.*layer: ordinary '[^']*' for [^ ]+ -> [A-Z ]+ \(ent ([0-9]+)\).*/\1/p" "$CENSUS" | head -1)
[ -n "$ENG_ENT" ] && [ -n "$LAY_ENT" ] && [ "$ENG_ENT" != "$LAY_ENT" ] \
  || fail "the census's two ordinary samples are missing or are the same door (engine '$ENG_ENT', layer '$LAY_ENT')"
grep -qE "live: security '[^']*' for [^ ]+ .*" "$CENSUS" || fail "no security door was tested"
grep -q "STAYED SHUT" "$CENSUS" || fail "a security door opened for a crew member with no player security code"
grep -qE "access: [0-9]+ .* [1-9][0-9]* do not" "$CENSUS" || fail "no embodied crew member was refused security access"
grep -qE "access: [^ ]+ opened security '[^']*' -> OPENED" "$CENSUS" || fail "a member whose record gives access did not open a security door"
# Rule three: a door the engine gave no touch trigger is a hand's or a script's, and the layer
# declines it. Asked of the layer's own rule, on a real map door.
grep -qE "layer: '[^']*' is a hand's or a script's door: the layer does not open it" "$CENSUS" \
  || fail "the no-touch-trigger door was not declined by the layer's own rule"
# The falsification: plant the old behaviour and show the check catching it.
if grep -q "opened security .* -> OPENED" "$FROZEN"; then
  fail "with the mechanism frozen (g_crewDoorOpen 0) the layer still opened a security door"
fi
if grep -q "layer: ordinary .* -> OPENED" "$FROZEN"; then
  fail "with the mechanism frozen (g_crewDoorOpen 0) the layer still opened an ordinary door"
fi
grep -q "frozen (g_crewDoorOpen 0): ordinary" "$FROZEN" || fail "the frozen census did not report the layer opening nothing"
grep -q "frozen (g_crewDoorOpen 0): lwh" "$FROZEN" || fail "the frozen census did not report the access mechanism frozen"

# ---- the walk-through: deck 4, at crewgen's suggested starting positions -------------------------
# The suggestion is what the tool proposes before anyone has walked it; the layer has to get the
# crew through the doors the deck puts between them and their posts.
SCEN="$DOORS/deck04-suggested"
mkdir -p "$SCEN"
python3 "$ROOT/tools/crewgen/crewgen.py" suggest --map tour/deck04 --data "$BUILD" --crew 6 > "$SCEN/scenario.json"
python3 "$ROOT/tools/crewgen/crewgen.py" build "$SCEN" --data "$BUILD" --out "$GAME_DIR" >/dev/null

walk() {
  local label="$1"; shift
  engine "$label" 150 +set g_crewDebug 1 "$@" +map tour/deck04
  # The engine colours a failure line (^3), so drop the colour codes before matching or printing.
  sed 's/\^[0-9]//g' "$HOME_DIR/doors-$label.out" \
    | grep -hE '^CREW: .*(at post|failed to reach|stopped by)' \
    | sed "s/^CREW: /    [$label] /" || true
}

walk walk +set g_crewDoorOpen 1
walk frozen2 +set g_crewDoorOpen 0

python3 - "$HOME_DIR/doors-walk.out" "$HOME_DIR/doors-frozen2.out" <<'PY'
import re, sys

def read(path):
    text = open(path, encoding="utf-8", errors="replace").read()
    return re.sub(r"\^[0-9]", "", text)   # drop the engine's colour codes

opened, frozen = read(sys.argv[1]), read(sys.argv[2])

def failed(text):
    return set(re.findall(r"^CREW: (\S+) failed to reach post '([^']+)'", text, re.M))

def arrived(text):
    return set(re.findall(r"^CREW: (\S+) at post '([^']+)'", text, re.M))

fo, ff = failed(opened), failed(frozen)
recovered = frozenset(ff) - frozenset(fo)
if recovered:
    print(f"INFO  {len(recovered)} member(s) stopped by a door with the layer's doors frozen, through it after: "
          + ", ".join(f"{w}" for w, _ in sorted(recovered)))

# Anyone still stopped must be named with the door that stopped them, and it must be a door that is
# *meant* to be shut: a security door the member's record does not open. A door a script owns stopped a
# member: the layer does not take the script's door, so the suggestion is the error.
for who, post in sorted(fo):
    m = re.search(r"^CREW: %s failed to reach post '%s'\n^CREW:   stopped by (\w+) door '([^']+)' \(ent \d+\) ([\d.]+) units"
                  % (re.escape(who), re.escape(post)), opened, re.M)
    if not m:
        print(f"FAIL  {who} is still stopped but no door was named for it"); sys.exit(1)
    kind, door, dist = m.groups()
    if kind == "security":
        print(f"INFO  {who} is still stopped by security door '{door}' {dist} units out: no access, "
              f"and the door is meant to be shut; the suggestion is the error")
    elif kind == "script":
        print(f"FAIL  {who} is stopped by script door '{door}' {dist} units out: the layer does not "
              f"take a script's door, so the suggestion is the error and the position must move")
        sys.exit(1)
    else:
        print(f"FAIL  {who} was stopped by {kind} door '{door}' -- that door should have opened")
        sys.exit(1)
print("PASS  every crew member the layer can get through its doors reached its post")
PY

echo "PASS  the doors stop crew exactly where the design means them to"
