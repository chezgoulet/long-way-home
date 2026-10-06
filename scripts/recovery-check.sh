#!/usr/bin/env bash
# De-assimilation (docs/borg-incursion.md): reversible only in a narrow window, and never whole.
#
#   scripts/recovery-check.sh
#
# One headless run on deck 4. A crew member in the window is recovered; the trace names them and the
# lasting residue. The rules (too late past the window, sickbay must be able to pay, the scar drags
# morale) are tested without the game in tests/ship (`TestDeassimilation`). Writes under build/g3-home.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
OUT="$HOME_DIR/recovery.out"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> de-assimilation"
SDL_AUDIODRIVER=dummy timeout 180 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipDayScale 1 +set g_shipTest 42 +map tour/deck04 >"$OUT" 2>&1 || true

fail() { echo "FAIL  $1"; exit 1; }
grep -h 'SHIP: recover test\|SHIP: crew .* is back' "$OUT" | sed 's/^EFSP: /    /' || true

grep -q 'SHIP: recover test: .* is 50% assimilated' "$OUT" || fail "the captive was not set up (see $OUT)"
grep -q 'SHIP: crew 10 is back, with lasting residue (scar 50%)' "$OUT" \
  || fail "the recovery did not return the crew member with residue"
echo "PASS  a crew member in the window is de-assimilated, and comes back changed"
