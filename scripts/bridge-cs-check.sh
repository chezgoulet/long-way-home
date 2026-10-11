#!/usr/bin/env bash
# The bridge's configstring table, and a configstring numbered past its old literal 1024
# (docs/evidence/the-bridge-traps.md). The table was a literal 1024 that dropped anything above it
# with no word; patch 0021 derives it from MAX_CONFIGSTRINGS and says so under `developer`.
#
#   scripts/bridge-cs-check.sh [--engine PATH]
#
# One headless run on borg1 with `developer` set and `g_shipTest 95`: the game sets configstring 1500
# (past the old literal) and reads it back. The engine's own output must say the index was accepted,
# and the read-back must carry the value. `--engine PATH` points at another build so this can be
# demonstrated failing: against an engine built before patch 0021 the same run prints "(bridge table
# CS_MAX 1024)" and an empty read-back. Needs the game data and the built engine + module; writes
# under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ENGINE=""
while [ $# -gt 0 ]; do
  case "$1" in
    --engine) ENGINE="$2"; shift 2 ;;
    *) echo "unknown argument: $1" >&2; exit 2 ;;
  esac
done

HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
OUT="$HOME_DIR/bridge-cs.out"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }
mkdir -p "$GAME_DIR"

fail() { echo "FAIL  $1"; exit 1; }

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> the bridge's configstring table"
ARGS=(--home-dir "$HOME_DIR")
if [ -n "$ENGINE" ]; then ARGS+=(--engine "$ENGINE"); fi
SDL_AUDIODRIVER=dummy timeout 120 xvfb-run -a "$ROOT/scripts/run-engine.sh" "${ARGS[@]}" \
    +set com_hunkMegs 768 +set developer 1 +set s_useOpenAL 0 +set s_initsound 0 \
    +set g_ship 1 +set g_shipTest 95 +map borg1 >"$OUT" 2>&1 || true

grep -h 'SP_SpawnServer: content:\|SP_SetConfigstring:\|SHIP: configstring probe' "$OUT" | sed 's/^EFSP: /    /' || true

grep -q 'SP_SpawnServer: content: .*(bridge table SP_CS_MAX 4096)' "$OUT" \
  || fail "the bridge's table is not derived from MAX_CONFIGSTRINGS (no 'bridge table SP_CS_MAX 4096')"
grep -q 'SP_SetConfigstring: index 1500 accepted (bridge table SP_CS_MAX 4096)' "$OUT" \
  || fail "the engine did not accept configstring 1500: the table still drops it"
if grep -q 'SP_SetConfigstring: index [0-9]* DROPPED' "$OUT"; then
  grep 'SP_SetConfigstring: index [0-9]* DROPPED' "$OUT" | sed 's/^EFSP: /    /'
  fail "a configstring was dropped by the bridge"
fi
grep -q 'SHIP: configstring probe: index 1500 -> "lwh-cs-beyond-1024"' "$OUT" \
  || fail "configstring 1500 did not survive the bridge: the read-back is not the value"

echo "PASS  a configstring numbered past the bridge's old literal 1024 survives the bridge, and the"
echo "      engine says so in its own output (SP_SetConfigstring: index 1500 accepted); before patch"
echo "      0021 the same run prints an empty read-back -- point --engine at that build to see it fail"
