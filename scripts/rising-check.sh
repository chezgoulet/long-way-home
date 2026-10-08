#!/usr/bin/env bash
# Rising to the occasion (docs/rising-to-the-occasion.md): the act, its cost, and the memory it
# leaves. One headless run on deck 4. A post must be held and the hand in front of it is beyond
# their skill; the drive decides whether they go, the log says why them, the cost is in the record,
# and the witnesses remember by name. The rules are tested without the game in tests/ship
# (TestRising*); `--rising` prints the transcript. Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
OUT="$HOME_DIR/rising.out"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> rising to the occasion: the act, its cost, and the memory it leaves"
SDL_AUDIODRIVER=dummy timeout 200 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipRole 1 +set g_shipDayScale 300 +set g_shipTest 60 +map tour/deck04 >"$OUT" 2>&1 || true

fail() { echo "FAIL  $1" >&2; exit 1; }

grep -hE '^SHIP: (RISING|day .*\[crew\])' "$OUT" | sed 's/^EFSP: /    /' || true
echo

# The instrument names the hand, their skill, and the post, before the act.
grep -qE '^SHIP: RISING instrument: .*engineering 0, effective 0\.[0-9]*, at the structural integrity' "$OUT" \
    || fail "the console did not name the hand, their skill and the post (see $OUT)"
# The act is written in the engine's own words: who, beyond their own skill, and it is offered.
grep -qE '^SHIP: RISING (held|failed|died): .*holds the structural integrity beyond their own engineering' "$OUT" \
    || fail "the act is not in the engine's own words"
# The reason they were the one is their drive, and the log says so.
grep -qE 'the reason they were the one: .*(useless|decompression|Borg|dying alone|promotion|prove|home)' "$OUT" \
    || fail "the record did not say why they were the one"
# The cost is in the record: a condition, named with its cause.
grep -qE '\[crew\] .*(comes down with exhausted|comes down with hypoxic)' "$OUT" \
    || fail "the act left no cost in the record"
# The witnesses remember the hero by name.
grep -qE '^SHIP: day .*\[crew\].*is remembered for it by name' "$OUT" \
    || fail "the witnesses do not remember the hero by name"
echo
echo "PASS  a post is held beyond the hand's skill; the record says why them and what it cost;"
echo "      the witnesses remember the name"
