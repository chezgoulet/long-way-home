#!/usr/bin/env bash
# Condition sets the odds, and stress sets the severity (docs/failure-is-content.md). The general
# mechanism, worked end to end on the transporter.
#
#   scripts/condition-check.sh
#
# One headless run on deck 4. The instrument states the transporter's condition before the act; a
# nominal system is beamed fifty times with no anomaly; the same system degraded and at battle
# stations lets go, changes the crew records, and writes the chain to the log. The rules are tested
# without the game in tests/ship (`TestConditionOdds`, `TestTransporterAnomaly`); `--risk` prints the
# measured odds table. Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
OUT="$HOME_DIR/condition.out"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> condition sets the odds, and stress sets the severity"
SDL_AUDIODRIVER=dummy timeout 200 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipRole 1 +set g_shipDayScale 300 +set g_shipTest 50 +map tour/deck04 >"$OUT" 2>&1 || true

fail() { echo "FAIL  $1"; exit 1; }

grep -hE "^SHIP: (RISK|transmitter|away team|the away team|the transporter)" "$OUT" | sed 's/^EFSP: /    /' || true
echo
grep -hE 'day [0-9]+ [0-9:]+ +\[(engineering|crew|sickbay)\].*(condition under|materialised|merged|misaligned|mangled|console let go)' \
    "$OUT" | sed 's/^EFSP: /    /' || true

# The instrument, before the act.
grep -q '^SHIP: transmitter: the pattern buffer is at 100%, nominal' "$OUT" \
    || fail "the console did not state the transporter's condition before the beam (see $OUT)"
# The top tenth: fifty beams, no anomaly of any kind.
grep -q '^SHIP: RISK nominal: 50 beams, 0 anomalies' "$OUT" || fail "a nominal system produced an anomaly"
# The same system, degraded and under load, states so and then gets it wrong.
grep -q '^SHIP: RISK instrument degraded: the pattern buffer is at 40%, and below 40 I would not send anyone' "$OUT" \
    || fail "the console did not state the degraded condition"
grep -q '^SHIP: RISK degraded under red alert: [1-9]' "$OUT" || fail "a degraded system under load produced no anomaly"
# The chain is in the record: condition, load, and what happened.
grep -qE '\[engineering\].*: transporters: (degraded|acute|catastrophic) anomaly at [0-9]+% condition under [0-9]+% load' "$OUT" \
    || fail "the log does not carry the chain (condition, load, severity)"
grep -qE '\[(crew|sickbay)\].*(materialised|merged|was mangled|misaligned|console let go)' "$OUT" \
    || fail "the anomaly left no consequence in the crew records"
echo
echo "PASS  a nominal system is beamed fifty times with no anomaly; the console states the condition before the act;"
echo "      the same system degraded and at battle stations lets go, writes the chain to the log, and changes or hurts a record"
