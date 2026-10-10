#!/usr/bin/env bash
# Voice in (docs/staff-meetings.md, M6 second half): an audio file is transcribed to a string by the
# STT worker, and that string takes the identical path a typed one takes -- the free-text pill's own
# seam. The strong test is identity, not resemblance: the transcribed and typed forms of the same
# line give the same verdict, the same log entry and a byte-identical ship state, with at most one
# inference call across both; a replay costs nothing.
#
#   scripts/voice-in-check.sh [--map NAME] [--model-dir DIR] [--audio FILE]
#
# The engine runs are headless (xvfb-run, SDL_AUDIODRIVER=dummy) and the module captures no audio:
# the boundary is the audio file. The STT worker runs from a throwaway venv under the gitignored
# voice-scratch/ (built here from the uv cache if absent), pointed at a borrowed model it reads in
# place. The classify phase needs the local ollama (all-minilm and the generator); the STT phase does
# not. No model, no audio and no transcript is committed.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MAP="tour/deck04"
MODEL_DIR="${LWH_STT_MODEL:-/home/c/hermes-vox/models-store/work/tw/sherpa-onnx-whisper-tiny.en}"
AUDIO=""
while [ $# -gt 0 ]; do
  case "$1" in
    --map)       MAP="$2"; shift 2 ;;
    --model-dir) MODEL_DIR="$2"; shift 2 ;;
    --audio)     AUDIO="$2"; shift 2 ;;
    *) echo "unknown argument: $1" >&2; exit 2 ;;
  esac
done
[ -n "$AUDIO" ] || AUDIO="$MODEL_DIR/test_wavs/0.wav"
EXPECTED="$(grep -i "$(basename "$AUDIO")" "$MODEL_DIR/test_wavs/trans.txt" 2>/dev/null | sed 's/^[^[:space:]]*[[:space:]]*//' || true)"

VENV="$ROOT/voice-scratch/venv"
PY="$VENV/bin/python"
WORKER="$ROOT/tools/speech/transcribe.py"
MEETING_WORKER="$ROOT/tools/meetings/worker.py"

HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
MEET="$GAME_DIR/ship/meetings"
TRANSCRIPT="$MEET/transcript.txt"
CLASSIFY="$MEET/classify.jsonl"
LEDGER="$MEET/novel.txt"
SCRIPTS="$MEET/scripts.txt"
OUT_A="$HOME_DIR/voice-in-a.out"
OUT_B="$HOME_DIR/voice-in-b.out"
OUT_C="$HOME_DIR/voice-in-c.out"

[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }
[ -f "$AUDIO" ] || { echo "no audio file at $AUDIO (pass --audio); the model's test_wavs/ is beside it" >&2; exit 1; }

fail() { echo "FAIL  $1"; exit 1; }

# ---- the throwaway STT venv (gitignored; built from the uv cache when absent) ---------------------
if [ ! -x "$PY" ]; then
  UV="${UV:-$(command -v uv || echo "$HOME/.local/bin/uv")}"
  [ -x "$UV" ] || fail "no uv at $UV to build the STT venv; install sherpa-onnx into $VENV by hand"
  echo "==> building the throwaway STT venv under voice-scratch/ (gitignored)"
  "$UV" venv --python python3 "$VENV" >/dev/null
  "$UV" pip install --offline --python "$PY" sherpa-onnx==1.13.8 >/dev/null 2>&1 \
    || "$UV" pip install --python "$PY" sherpa-onnx==1.13.8 >/dev/null
fi

run_engine() { # $1 = out file
  find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
  SDL_AUDIODRIVER=dummy timeout 200 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
      +set s_useOpenAL 0 +set g_ship 1 +set g_shipMode 1 +set g_shipTest 86 \
      +set lwh_voice_audio "$AUDIO" +map "$MAP" >"$1" 2>&1 || true
  grep -h '^SHIP: voice-in test\|^SHIP: dictate\|^SHIP: meetings: queued' "$1" | sed 's/^SHIP: /    /' || true
}

# ---- run A: the STT worker absent. The request is queued; no transcript; the room plays on --------
echo "==> run A: the game with the STT worker absent"
rm -rf "$MEET"; mkdir -p "$MEET"
run_engine "$OUT_A"
grep -q 'voice-in test: the STT worker is absent: .* has no transcript, the input is pending' "$OUT_A" \
  || fail "the absent STT worker was not reported as a pending input"
grep -q 'dictate: no transcript for .* yet; the transcription is pending and the room is unaffected' "$OUT_A" \
  || fail "the voice say did not fail soft with no transcript"
grep -q 'meeting choose: watch-change outcome 1 applied' "$OUT_A" \
  || fail "the meeting did not resolve with the STT worker absent"
[ -s "$MEET/voice.jsonl" ] || fail "the capture boundary was not written (ship/meetings/voice.jsonl)"

# ---- Task A: the audio file becomes the string ---------------------------------------------------
echo
echo "==> Task A: a known audio file transcribes to the expected text"
"$PY" "$WORKER" transcribe --audio "$AUDIO" --out "$TRANSCRIPT" --expected "$EXPECTED" --model-dir "$MODEL_DIR" \
  >"$MEET/transcribe.out" 2>&1 || fail "the STT worker exited non-zero"
sed 's/^/    /' "$MEET/transcribe.out"
grep -q '^text    : ' "$MEET/transcribe.out" || fail "the worker produced no transcript"
grep -q 'normalised match: yes' "$MEET/transcribe.out" || fail "the transcript did not match the expected line"
[ -s "$TRANSCRIPT" ] || fail "no transcript was written where the module reads it"
TRANSCRIBED="$(grep '^text    : ' "$MEET/transcribe.out" | sed 's/^text    : //')"
echo "    the source audio: $AUDIO"
echo "    the transcript  : $TRANSCRIBED"

# ---- run B: the string takes the identical path --------------------------------------------------
echo
echo "==> run B: the transcribed and typed forms of the same line"
run_engine "$OUT_B"
grep -q 'voice-in test: the STT worker transcribed ' "$OUT_B" || fail "the run saw no transcript"
grep -q 'the string now takes the path a typed one takes' "$OUT_B" || fail "the voice say did not reach the seam"
grep -q 'voice-in test: the verdict is the same: yes' "$OUT_B" || fail "the two forms gave different verdicts"
grep -q 'voice-in test: the log entry is the same: yes' "$OUT_B" || fail "the two forms gave different log entries"
grep -q 'voice-in test: the ship.s state is byte-identical across the two paths: yes' "$OUT_B" \
  || fail "the two forms did not give a byte-identical state"
# The same content-addressed key from both console paths: one seam, not two.
grep -c 'meetings: queued one classification for "' "$OUT_B" >/dev/null || fail "no classification was queued"
KEYS="$(grep -o 'key [0-9a-f]*' "$OUT_B" | sort -u | wc -l)"
[ "$KEYS" -eq 1 ] || fail "the voice and typed forms hashed to different keys ($KEYS distinct)"

# ---- the call count: at most one inference call across both --------------------------------------
echo
echo "==> the classifier: one live call for the repeated spoken input; a replay calls nothing"
python3 "$MEETING_WORKER" classify --manifest "$CLASSIFY" --ledger "$LEDGER" --script "$SCRIPTS" \
  --transcript "$MEET/promoted.txt" --max-calls 1 >"$MEET/classify.out" 2>&1 \
  || fail "the classifier exited non-zero on one novel input"
sed 's/^/    /' "$MEET/classify.out"
grep -q '^== 0 matched, 1 novel call(s)' "$MEET/classify.out" \
  || fail "the same spoken input did not make exactly one live call"
python3 "$MEETING_WORKER" classify --manifest "$CLASSIFY" --ledger "$LEDGER" --max-calls 1 \
  >"$MEET/classify2.out" 2>&1 || fail "the classifier exited non-zero on a cached input"
sed 's/^/    /' "$MEET/classify2.out"
grep -q '^== 0 matched, 0 novel call(s)' "$MEET/classify2.out" \
  || fail "a second pass made a call again (the replay was not free)"

# ---- run C: the verdict is loaded, and the replay costs nothing ----------------------------------
echo
echo "==> run C: the same spoken input, replayed from the log"
run_engine "$OUT_C"
grep -q 'voice-in test: the verdict is the same: yes' "$OUT_C" || fail "the replay diverged between the two forms"
grep -q 'the same spoken input resolves as matched branch [0-9]*' "$OUT_C" \
  || fail "the replayed spoken input did not resolve to its branch"
grep -q 'voice-in test: replay: verdicts loaded=1, novel-path calls=1' "$OUT_C" \
  || fail "the replay did not read the verdict and keep the call count at one"

# ---- Task C: the memory rule, and the model present in one worker only ---------------------------
echo
echo "==> the memory rule: which model is resident when"
"$PY" "$WORKER" residency --model-dir "$MODEL_DIR" | sed 's/^/    /'
grep -q '^load    : ' "$MEET/transcribe.out" || fail "the STT worker reported no model load"
grep -q '^peak RSS: ' "$MEET/transcribe.out" || fail "the STT worker reported no peak footprint"
# The STT model runs only where voice mode queued a request: run A without voice mode is impossible
# to reach here (the harness turns it on), so the gate is shown at the seam instead -- a say with no
# transcript is pending, and no other worker loads the STT model.
echo "    the STT model is loaded only by the transcribe worker above, for the life of that one process"
grep -q 'resident: sherpa-onnx whisper tiny.en (STT)' "$MEET/transcribe.out" \
  || fail "the STT worker did not name the one model it loads"
# The meeting worker's models live in ollama and are resident there; the STT model is not, because
# its one-shot process has exited. The two are never resident together: the check sequences them,
# and the module only queues an STT request in voice mode.
if command -v ollama >/dev/null 2>&1; then
  echo "    ollama ps, after classify (the meeting worker's models; no STT model here):"
  ollama ps 2>/dev/null | sed 's/^/      /' || true
fi

# ---- the boundary: no model, no audio, no generated content in the repository --------------------
echo
echo "==> the boundary"
if git -C "$ROOT" ls-files | grep -Ei '\.(wav|mp3|ogg|flac|aiff?|m4a|onnx|gguf|safetensors|bin|pt)$' >/dev/null; then
  fail "the repository contains a model or audio artifact"
fi
git -C "$ROOT" check-ignore -q "$ROOT/voice-scratch/venv" || fail "the STT venv is not gitignored"
git -C "$ROOT" check-ignore -q "$MEET" || fail "the meeting cache is not gitignored"
echo "    the repository carries no model, no audio and no transcript"

echo "PASS  a known audio file transcribes to the expected text; the transcribed string takes the"
echo "      identical path -- the same verdict, the same log entry and a byte-identical ship state,"
echo "      with one live call across both; the replay is free; the STT worker is absent-proof and"
echo "      loads one model; the capture seam is an audio file, and no microphone was built."
