#!/usr/bin/env bash
# The audio plumbing (docs/staff-meetings.md, phase two; docs/evidence/audio-plumbing.md): three
# sources with one owner each, a cue track that cannot stop a dialogue line, the render queue with a
# cache key that avoids a re-render, the warm in the async window, and the delivery direction carried
# to the synthesizer's --exaggeration. No model is loaded and no sound is played.
#
#   scripts/audio-check.sh [--map NAME]
#
# One headless engine run, then the Python renderer in dry-run. Needs the game data
# (scripts/playtest-host-setup.sh) and xvfb-run.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MAP="tour/deck04"
while [ $# -gt 0 ]; do
  case "$1" in
    --map) MAP="$2"; shift 2 ;;
    *) echo "unknown argument: $1" >&2; exit 2 ;;
  esac
done

HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
OUT="$GAME_DIR/ship"
VOICE="$OUT/voice"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }
mkdir -p "$VOICE" "$GAME_DIR/screenshots"
rm -f "$VOICE/render.jsonl"
# Be hermetic: the check plans from an empty queue, so a cache or a durations file left by an earlier
# run (or by the real renderer) does not make the plan a no-op. Reference clips under refs/ are kept.
rm -f "$VOICE"/*.wav "$VOICE/durations.txt"

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> the audio plumbing (g_shipTest 81)"
SDL_AUDIODRIVER=dummy timeout 240 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipMode 1 +set g_shipTest 81 +map "$MAP" \
    >"$HOME_DIR/audio.out" 2>&1 || true

grep -h '^SHIP: voice test' "$HOME_DIR/audio.out" | sed 's/^/    /' || true

fail() { echo "FAIL  $1"; exit 1; }
[ -f "$OUT/voice.txt" ] || fail "the audio run did not finish"

# The normal case: the watch change emits a brief, and the async window plans its audio.
grep -q 'brief(s) queued at the watch change' "$HOME_DIR/audio.out" \
  || fail "no brief was emitted at the watch change"
grep -q 'async window: warmed=1, planned [1-9][0-9]* line(s), queue unfinished=[1-9][0-9]*, second plan=0' "$HOME_DIR/audio.out" \
  || fail "the async window did not warm and plan, or planned the same lines twice"
grep -q 'the meeting plays with the queue unfinished' "$HOME_DIR/audio.out" \
  || fail "the meeting did not play with the queue unfinished"

# Three sources, one owner each, and a non-owner can neither write nor retire.
grep -q 'three sources, owner each: dialogue=the renderer, cue=the cue player, live=the live line' "$HOME_DIR/audio.out" \
  || fail "the three sources and their owners were not named"
grep -q 'non-owner write refused=1, non-owner retire refused=1, the cue did not stop the line=1' "$HOME_DIR/audio.out" \
  || fail "the ownership invariant did not hold: a non-owner wrote or retired, or a cue stopped a line"

# The cue emit site fires in the normal case (the emission, not a count).
grep -q 'cue emitted=[0-9][0-9]* in the normal case: breath (the pause is a person' "$HOME_DIR/audio.out" \
  || fail "the cue emit site did not fire in the normal case"

# The cache key: same text, same voice, same delivery, same file; a re-render is a no-op.
grep -q 'cache key [0-9a-f]\{16\}; fresh queue 1, queue again 0, cached requeue 0 (0 = a no-op)' "$HOME_DIR/audio.out" \
  || fail "the cache key did not make a second render a no-op"
grep -q 'pruned with the save: 1 entr(ies) dropped, cache now 0' "$HOME_DIR/audio.out" \
  || fail "the cache was not pruned with the save"

# The delivery direction reaches the synthesizer's --exaggeration: one line's value end to end.
grep -q 'delivery order -> exaggeration 0.80' "$HOME_DIR/audio.out" \
  || fail "the delivery direction did not reach the renderer's exaggeration"
[ -f "$VOICE/render.jsonl" ] || fail "the render manifest was not written"
python3 "$ROOT/tools/voice/render.py" --cache "$VOICE" --manifest "$VOICE/render.jsonl" --dry-run \
    >"$HOME_DIR/audio-render.out" 2>&1 || true
grep -q -- '--exaggeration 0.8' "$HOME_DIR/audio-render.out" \
  || fail "the order line did not reach synthesize.py --exaggeration 0.8"
grep -q -- '--exaggeration 0.5' "$HOME_DIR/audio-render.out" \
  || fail "the report line did not reach synthesize.py --exaggeration 0.5"

# The cache is never repository data, and the repository carries no audio.
if git -C "$ROOT" ls-files | grep -Ei '\.(wav|mp3|ogg|flac|ogg|aiff?|m4a)$' >/dev/null; then
  fail "the repository contains generated audio"
fi
if git -C "$ROOT" check-ignore -q "$VOICE"; then :; else
  fail "the audio cache is not gitignored: $VOICE"
fi

# ---- the casting map, voice out, the pause cue, and pacing (M5) ---------------------------------
# A second headless run: the cast is derived from the start state (a canon survivor keeps the retail
# voice; a canon character who died has none; a fictitious member is cast from the non-canon pool);
# the map is deterministic; every line is planned with its cast reference during generation, with no
# inference; a novel answer plays a cue immediately and resolves or defers; and a line's measured
# clip duration paces the pill. No model is loaded.
echo
echo "==> the casting map and voice out (g_shipTest 84)"
find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
SDL_AUDIODRIVER=dummy timeout 240 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipMode 1 +set g_shipTest 84 +map "$MAP" \
    >"$HOME_DIR/audio-cast.out" 2>&1 || true
grep -h '^SHIP: cast test' "$HOME_DIR/audio-cast.out" | sed 's/^/    /' || true

grep -q 'cast test: canon: 9 canon voice(s), 132 from the non-canon pool' "$HOME_DIR/audio-cast.out" \
  || fail "the canon default did not keep 9 retail voices and cast 132 from the pool"
grep -q 'cast test: canon survivor Tuvok -> tuvok (the retail voice)' "$HOME_DIR/audio-cast.out" \
  || fail "a canon survivor was not given their own retail voice"
grep -q 'cast test: the chair: Janeway is dead -> voice "" (no voice; no line is authored)' "$HOME_DIR/audio-cast.out" \
  || fail "a canon character who died was given a voice"
grep -q 'cast test: all-fictitious: 141 crew, 0 canon voice(s) used' "$HOME_DIR/audio-cast.out" \
  || fail "an all-fictitious crew used a canon voice"
grep -q 'cast test: the same start state re-cast gives the same map=1' "$HOME_DIR/audio-cast.out" \
  || fail "the casting map was not deterministic"
grep -q 'cast test: voice out during generation: planned 6 line(s), queue unfinished=6, inference calls=0' "$HOME_DIR/audio-cast.out" \
  || fail "the meeting's lines were not planned with cast references, or a model was called"
grep -q 'cast test: pacing: .* runs [0-9.]*s (.*); inference calls=0' "$HOME_DIR/audio-cast.out" \
  || fail "the line's duration did not pace the pill"
grep -q 'cast test: pause: fast cue=[0-9]*, resolved inside the pause=1' "$HOME_DIR/audio-cast.out" \
  || fail "a fast novel answer did not play a cue and resolve inside the pause"
grep -q 'cast test: pause: slow deferral=[0-9]* "I have to think about that." -> the outcome moves to a later beat' "$HOME_DIR/audio-cast.out" \
  || fail "a slow novel answer did not defer with the holding line"
grep -q 'cast test: the cache prunes with the save: [0-9]* of [0-9]* entr(ies) dropped, cache now 0' "$HOME_DIR/audio-cast.out" \
  || fail "the voice cache was not pruned with the save"

echo "PASS  three sources with one owner each; a cue cannot stop a line; the cache key makes a re-render a"
echo "      no-op and prunes with the save; the warm happens in the async window; the cue emit site fires;"
echo "      the delivery direction reaches synthesizer --exaggeration; the repository carries no audio;"
echo "      the casting map is derived and deterministic (canon, the chair, all-fictitious); the lines are"
echo "      planned with cast references and no inference; a novel answer cues immediately and resolves or"
echo "      defers; a line's measured duration paces the pill"
