#!/usr/bin/env bash
# The async generator and the novelty classifier (docs/staff-meetings.md, M4): the brief becomes a
# manifest for the off-loop worker; the generated script is validated before it is usable; the room
# plays from the generated script when it is fresh and from the authored skeleton when the model is
# absent; a typed input matching no branch makes exactly one live call, is written to the log and is
# promoted so a later run replays it with no call. The model never writes ship state.
#
#   scripts/model-call-check.sh [--map NAME]
#
# Engine runs need the game data (scripts/playtest-host-setup.sh) and xvfb-run; the worker needs the
# local ollama with all-minilm and qwen2.5:3b. Nothing here pulls a model or writes the repository.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MAP="tour/deck04"
WORKER="$ROOT/tools/meetings/worker.py"
while [ $# -gt 0 ]; do
  case "$1" in
    --map) MAP="$2"; shift 2 ;;
    *) echo "unknown argument: $1" >&2; exit 2 ;;
  esac
done

HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
MEET="$GAME_DIR/ship/meetings"
GEN="$MEET/generate.jsonl"
SCRIPTS="$MEET/scripts.txt"
LEDGER="$MEET/novel.txt"
CLASSIFY="$MEET/classify.jsonl"
PROMOTED="$MEET/promoted.txt"
FIX="$ROOT/build/model-check"
OUT_A="$HOME_DIR/model-a.out"
OUT_B="$HOME_DIR/model-b.out"
OUT_C="$HOME_DIR/model-c.out"

[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }
rm -rf "$FIX"
mkdir -p "$MEET" "$FIX" "$GAME_DIR/screenshots"

fail() { echo "FAIL  $1"; exit 1; }

run_engine() { # $1 = out file
  find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
  SDL_AUDIODRIVER=dummy timeout 200 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
      +set s_useOpenAL 0 +set g_ship 1 +set g_shipMode 1 +set g_shipTest 83 +map "$MAP" >"$1" 2>&1 || true
  grep -h '^SHIP: model test' "$1" | sed 's/^SHIP: /    /' || true
  grep -h '^SHIP: meetings:' "$1" | sed 's/^SHIP: /    /' || true
}

# ---- clean, then run A: no worker output yet, so the room plays from the skeleton ----------------
rm -rf "$MEET"
mkdir -p "$MEET"
echo "==> run A: the brief becomes a manifest; the room plays from the authored skeleton"
run_engine "$OUT_A"

grep -q 'model test: [1-9][0-9]* brief(s) queued at the watch change' "$OUT_A" \
  || fail "no brief was emitted at the watch change"
grep -q 'meetings: wrote ship/meetings/generate.jsonl' "$OUT_A" \
  || fail "the generator manifest was not written"
[ -s "$GEN" ] || fail "the generator manifest is empty"
grep -q 'model test: the room plays from the authored skeleton; 0 script(s) waiting before the player arrived' "$OUT_A" \
  || fail "the room did not play from the authored skeleton with no script loaded"
grep -q 'model test: typed "Make it so": no branch matched (novel)' "$OUT_A" \
  || fail "a typed input with no verdict was not novel"
[ -s "$CLASSIFY" ] || fail "the novel input did not become a classification request"

# ---- the validator refuses a deliberately broken script, by name ---------------------------------
cat >"$FIX/good.json" <<'JSON'
{"optionCount":4,"branches":[
 {"id":"b1","intent":"carry on","outcome":1,"lines":[{"speaker":"command","text":"Carry on.","delivery":"order"}]},
 {"id":"b2","intent":"watch the reserve","outcome":2,"lines":[{"speaker":"engineering","text":"I will watch it.","delivery":"report"}]}]}
JSON
cat >"$FIX/dead-end.json" <<'JSON'
{"optionCount":4,"branches":[
 {"id":"b1","intent":"carry on","outcome":1,"lines":[{"speaker":"command","text":"Carry on.","delivery":"order"}]},
 {"id":"b2","intent":"something else","lines":[{"speaker":"command","text":"We will see.","delivery":"flat"}]}]}
JSON
cat >"$FIX/no-terminus.json" <<'JSON'
{"optionCount":4,"branches":[
 {"id":"b1","intent":"carry on","outcome":9,"lines":[{"speaker":"command","text":"Carry on.","delivery":"order"}]}]}
JSON
cat >"$FIX/contradicts.json" <<'JSON'
{"optionCount":4,"forbidden":["Tuvok"],"branches":[
 {"id":"b1","intent":"carry on","outcome":1,"lines":[{"speaker":"command","text":"Tuvok will take the watch.","delivery":"order"}]}]}
JSON

echo "==> the validator"
python3 "$WORKER" validate --script "$FIX/good.json" || fail "the validator refused a good script"
python3 "$WORKER" validate --script "$FIX/dead-end.json" >"$FIX/dead.out" 2>&1 && fail "the validator accepted a dead end"
grep -q 'REFUSED dead-end b2' "$FIX/dead.out" || fail "the dead end was not refused by name"
python3 "$WORKER" validate --script "$FIX/no-terminus.json" >"$FIX/term.out" 2>&1 && fail "the validator accepted a non-terminating branch"
grep -q 'REFUSED branch b1 does not terminate in an enumerated outcome (outcome 9, options 4)' "$FIX/term.out" \
  || fail "the non-terminating branch was not refused by name"
python3 "$WORKER" validate --script "$FIX/contradicts.json" >"$FIX/contra.out" 2>&1 && fail "the validator accepted a contradiction"
grep -q 'REFUSED branch b1 contradicts the record' "$FIX/contra.out" || fail "the contradiction was not refused by name"
cat "$FIX/dead.out" "$FIX/term.out" "$FIX/contra.out" | sed 's/^/    /'

# ---- generate: the brief becomes a validated script, before the player arrives --------------------
echo "==> the generator"
python3 "$WORKER" generate --manifest "$GEN" --script "$SCRIPTS" --ledger "$LEDGER" --max-attempts 2 >"$FIX/gen.out" 2>&1 \
  || fail "the generator exited non-zero"
sed 's/^/    /' "$FIX/gen.out"
grep -q 'generation call(s)' "$FIX/gen.out" || fail "the generator made no model call"
[ -s "$SCRIPTS" ] || fail "no script was written"
grep -q '^D|' "$SCRIPTS" || fail "the script has no header"
grep -q '^L|' "$SCRIPTS" || fail "the script has no dialogue lines"

# ---- classify: one novel input, exactly one call; the same input later, no call ------------------
echo "==> the classifier"
python3 "$WORKER" classify --manifest "$CLASSIFY" --ledger "$LEDGER" --script "$SCRIPTS" --transcript "$PROMOTED" --max-calls 1 \
  >"$FIX/classify.out" 2>&1 || fail "the classifier exited non-zero on one novel input"
sed 's/^/    /' "$FIX/classify.out"
grep -q '^== 0 matched, 1 novel call(s)' "$FIX/classify.out" || fail "one novel input did not produce exactly one call"
grep -q 'promoted to branch' "$FIX/classify.out" || fail "the novel exchange was not promoted"
[ -s "$PROMOTED" ] || fail "no transcript of the promoted exchange"
grep -q '^L|' "$SCRIPTS" || fail "no promoted branch was added to the script"
grep -c '^L|' "$SCRIPTS" >"$FIX/lines-after" || true

# The same manifest again: the verdicts are already decided, so nothing calls the model.
python3 "$WORKER" classify --manifest "$CLASSIFY" --ledger "$LEDGER" --script "$SCRIPTS" --max-calls 1 \
  >"$FIX/classify2.out" 2>&1 || fail "the classifier exited non-zero on a cached input"
grep -q '^== 0 matched, 0 novel call(s)' "$FIX/classify2.out" || fail "a second classify called the model again"
grep -q 'cached ' "$FIX/classify2.out" || fail "the verdict was not replayed from the ledger"

# Above the threshold it is classification, not generation: a genuinely matching input plays its
# branch and the generator is never touched (a budget of zero proves it).
printf '%s\n' '{"key":"cc01","kind":0,"text":"watch the reserve closely","intents":[{"index":1,"intent":"Carry on as briefed (nothing)"},{"index":2,"intent":"Watch the reserve (Engineering'"'"'s attention this watch)"},{"index":3,"intent":"Bring her to yellow (the watch runs hot and the plant draws harder)"},{"index":4,"intent":"Note what the department raised (nothing yet)"}]}' >"$FIX/match.jsonl"
python3 "$WORKER" classify --manifest "$FIX/match.jsonl" --ledger "$FIX/match-ledger.txt" --max-calls 0 \
  >"$FIX/match.out" 2>&1 || fail "a matching input needed a generator call"
sed 's/^/    /' "$FIX/match.out"
grep -q '^== 1 matched, 0 novel call(s)' "$FIX/match.out" || fail "a matching input was not classified without a call"

# ---- the call count can fail: two fresh novel inputs against a budget of one ---------------------
printf '%s\n%s\n' \
 '{"key":"bb01","kind":0,"text":"Tea, Earl Grey, hot","intents":[{"index":1,"intent":"Carry on as briefed (nothing)"},{"index":2,"intent":"Watch the reserve (attention this watch)"},{"index":3,"intent":"Bring her to yellow (the watch runs hot)"},{"index":4,"intent":"Note what the department raised (nothing yet)"}]}' \
 '{"key":"bb02","kind":0,"text":"Sing me a song","intents":[{"index":1,"intent":"Carry on as briefed (nothing)"},{"index":2,"intent":"Watch the reserve (attention this watch)"},{"index":3,"intent":"Bring her to yellow (the watch runs hot)"},{"index":4,"intent":"Note what the department raised (nothing yet)"}]}' \
 >"$FIX/two-novel.jsonl"
if python3 "$WORKER" classify --manifest "$FIX/two-novel.jsonl" --ledger "$FIX/over.json" --max-calls 1 >"$FIX/over.out" 2>&1; then
  fail "the call budget accepted two novel calls against a budget of one"
fi
grep -q '^FAIL  [0-9]* novel call(s) exceed the budget of 1' "$FIX/over.out" \
  || fail "the call-count check failed for the wrong reason"
echo "    the call budget fails when a second call happens (proved)"

# ---- run B: the validated script is waiting, and the promoted branch replays with no call ---------
echo "==> run B: the script exists before the player arrives, and the familiar input replays"
run_engine "$OUT_B"
grep -q 'model test: the room plays from the generated script; 1 script(s) waiting before the player arrived' "$OUT_B" \
  || fail "the room did not play from the generated script"
grep -q 'model test: typed "Make it so": matched branch' "$OUT_B" \
  || fail "the promoted input did not match a branch on the second run"
grep -q 'model test: novel-path calls=1' "$OUT_B" \
  || fail "the novel-path call count was not 1"
grep -q 'the reloaded ship classifies the same input as matched branch' "$OUT_B" \
  || fail "the verdict did not survive the save/load"
grep -q 'model test: verdicts loaded=1, scripts loaded=1' "$OUT_B" \
  || fail "the verdict and the script were not both loaded"

# A material change after generation: the script is stale, the room plays the skeleton, the brief is
# re-queued -- and the worker regenerates it.
grep -q 'model test: after a material change the script was loaded=1, is fresh=0; the room plays the authored skeleton' "$OUT_B" \
  || fail "a material change did not make the script stale"
[ -s "$MEET/regenerate.jsonl" ] || fail "the stale brief was not queued for regeneration"
python3 "$WORKER" generate --manifest "$MEET/regenerate.jsonl" --script "$MEET/scripts-regen.txt" --ledger "$FIX/regen-ledger.txt" \
  --max-attempts 2 >"$FIX/regen.out" 2>&1 || fail "regeneration exited non-zero"
grep -q 'valid   :' "$FIX/regen.out" || fail "the worker did not regenerate the stale brief"
[ -s "$MEET/scripts-regen.txt" ] || fail "no regenerated script was written"
echo "    the stale brief was regenerated"

# ---- the model absent or broken: the worker fails soft, and the meeting plays from the skeleton ----
echo "==> the model absent"
python3 "$WORKER" generate --manifest "$GEN" --script "$MEET/scripts-offline.txt" --ledger "$FIX/offline-ledger.txt" \
  --host "http://127.0.0.1:9" --max-attempts 1 >"$FIX/offline.out" 2>&1 || fail "the worker did not fail soft without the model"
sed 's/^/    /' "$FIX/offline.out"
grep -q 'model unavailable' "$FIX/offline.out" || fail "the worker did not report the model absent"
[ ! -s "$MEET/scripts-offline.txt" ] || fail "the worker wrote a script with no model"

mv "$SCRIPTS" "$MEET/scripts.good"
run_engine "$OUT_C"
grep -q 'model test: the room plays from the authored skeleton' "$OUT_C" \
  || fail "the meeting did not play from the skeleton with the model absent"
mv "$MEET/scripts.good" "$SCRIPTS"

# ---- the repository carries no model and no generated content -------------------------------------
if git -C "$ROOT" ls-files | grep -Ei '\.(wav|mp3|ogg|flac|aiff?|m4a|gguf|safetensors|bin|pt|onnx)$' >/dev/null; then
  fail "the repository contains a model or audio artifact"
fi
git -C "$ROOT" check-ignore -q "$MEET" || fail "the meeting cache is not gitignored: $MEET"

echo "PASS  the brief becomes a manifest; the validator refuses a dead end, a non-terminating branch and a"
echo "      contradiction by name; the script is waiting before the player arrives; one novel input makes"
echo "      exactly one call and is promoted; the call budget can fail; a reload replays the promoted"
echo "      branch with no call; with the model absent the meeting plays from the skeleton; the repository"
echo "      carries no model and no generated content"
