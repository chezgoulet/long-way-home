# The async generator and the novelty classifier — the model call (M4)

`docs/staff-meetings.md` (the owner's design), `docs/programme-meetings-and-voice.md` (the order and
the eight Hermes Vox lessons), `docs/handoff-live-crew-and-meetings.md` (M4), and
`docs/evidence/meeting-overlay.md` (M3, the seam this fills), applied on `feat/the-model-call` (cut
from `testing`), 2026-10-09.

**Observed, and the command that produced it.** Everything below is the output of a command named
beside it, on this tree, after `cmake -S ../upstream/efgame -B ../upstream/efgame/build-linux
-DLWH_MODULE_DIR=$PWD/module && cmake --build ../upstream/efgame/build-linux --target efgame --target
efui` (both modules built cleanly). The engine was run headless; no engine process or agent session
was left running. The build host is `sasquatch`, CPU-only, `ollama` on `127.0.0.1:11434`, with
`all-minilm` (45 MB, the embedding model) and `qwen2.5:3b` (1.9 GB, the generator). Nothing was pulled
and no dependency was added to the repository. **No model, no cache and no generated content is
committed.**

## What was built, and where

| file | what it is |
|---|---|
| `tools/meetings/worker.py` | **new**: the off-loop worker — `generate` (brief → validated script), `classify` (novelty), `validate` (the pre-flight validator). Talks to ollama with Python's own `urllib`. |
| `module/ship/ship_core.{h,cpp}` | the novelty seam **filled in**: a content-addressed verdict index (`NoveltyKey`, `ClearNoveltyIndex`, `AddNoveltyVerdict`, `NoveltyVerdictCount`) that `ClassifyNovelInput` consults. |
| `module/ship/g_ship.cpp` | the host half: the generator manifest, the classify manifest, the script and verdict loaders, the `ship meeting script|reload|generate|verdicts` verbs, and the `g_shipTest 83` demonstration. |
| `module/ui/ui_lwh_meeting.cpp` | **Task A**: the pill row lifted clear of the engine's version stamp, and `meet.said` moved into the speaker rail. |
| `scripts/model-call-check.sh` | **new**: the whole path, headless: run A → validator → generator → classifier → run B → model-absent. |
| `tests/ship/test_ship_core.cpp` | `TestNoveltyIndex`: the index's semantics without a screen. |
| `tests/tools/test_meeting_worker.py` | the validator, the ledger and the field sanitising, with no network. |

The module performs **no I/O to a model**. It writes a JSON-lines manifest of queued briefs
(`ship/meetings/generate.jsonl`) and of typed inputs (`ship/meetings/classify.jsonl`); `worker.py`
drains them and writes back the script (`ship/meetings/scripts.txt`) and its verdicts and call ledger
(`ship/meetings/novel.txt`). This is the pattern `docs/evidence/audio-plumbing.md` set with
`tools/voice/render.py`: the module writes a manifest and reads a result, and applies nothing itself.

## Task A — the two overlay fixes, on screen

**What was wrong.** `UI_MenuFrame2` (`upstream/efgame/src/ui/ui_atoms.cpp:2429`) draws `Q3_VERSION`
*after* every menu's content, at **(371,445)** with the tiny font, so no fill of ours can hide it; M3
drew the free-text pill at y=438, right across it. And `meet.said` was drawn at **(158,446)** — the
free-text row's own band.

**What changed.** The pill block is packed above the stamp: it starts just below the measured answer
line and compresses its pitch if the outcomes and the free-text pill together would reach y=443
(`module/ui/ui_lwh_meeting.cpp`). With the four-outcome allocation skeleton the free-text pill moves
from y=438 to y≈423 — the row lifted ~16 units — and the stamp's strip is left empty. `meet.said` is
drawn in the **speaker rail's lower strip** (x < 150, from y=434, wrapped): the rail is entirely left
of the stamp's box (x ≥ 371), so the seam's answer can never be read as the stamp's contents whatever
its length. Named as a call.

**The measurement.** From the rendered frame (`scripts/meeting-overlay-check.sh --map tour/deck04`,
`lwh_meeting.tga`, 1280×1024 for the 640×480 frame):

```
$ python3 - build/g3-home/baseEF/screenshots/lwh_meeting.tga <<'PY'
import sys
from PIL import Image
im = Image.open(sys.argv[1]).convert("RGB"); W, H = im.size; px = im.load()
sx, sy = W / 640.0, H / 480.0
cols = {px[x, y] for y in range(int(444 * sy), int(456 * sy)) for x in range(int(371 * sx), int(500 * sx), 4)}
print("the version stamp box (virtual 371..500 x 444..456) holds %d colour(s): %s" % (len(cols), sorted(cols)[:2]))
PY
the version stamp box (virtual 371..500 x 444..456) holds 1 colour(s): [(0, 0, 0)]
```

The stamp's box is a single, uniform colour in the rendered frame: nothing of ours is in it. The
free-text pill and the options above it, and the "IN THE ROOM" line below, are visible in the frame.
A third frame, `lwh_meeting_said.tga`, shows `meet.said` ("TYPED ANSWER SENT TO THE NOVELTY SEAM")
drawn under MOOD in the speaker rail, left of the stamp's box (`scripts/meeting-overlay-check.sh` now
requires that frame).

## Task B — the generator, off the render loop

The brief becomes a manifest before anyone arrives:

```
$ scripts/model-call-check.sh --map tour/deck04   # run A, the skeleton
    model test: 1 brief(s) queued at the watch change
    meetings: wrote ship/meetings/generate.jsonl (1 brief(s))
    model test: the room plays from the authored skeleton; 0 script(s) waiting before the player arrived
    model test: outcome 1 line: "You have the watch. Carry on."
    model test: typed "Make it so": no branch matched (novel)
    meetings: queued one classification for "Make it so" (key 7ce10140)
    model test: novel-path calls=0 (before the load 0); the reloaded ship classifies the same input as novel
    model test: verdicts loaded=0, scripts loaded=0, generation calls=0
```

The manifest is identity and time, who is present, the decision, the enumerated options with their
costs, the authored skeleton as the fallback, and the ship and people as they are now
(`ship/meetings/generate.jsonl`, quoted in the check). The worker turns it into a script
**before the player arrives**:

```
$ python3 tools/meetings/worker.py generate --manifest ship/meetings/generate.jsonl \
      --script ship/meetings/scripts.txt --ledger ship/meetings/novel.txt --max-attempts 2
valid   : watch-change: 4 branch(es) accepted
== 5 script line(s) written; 1 generation call(s)
```

The script is the format M2 defined — a `D|kind|digest|decision` header and `L|kind|outcome|speaker|
delivery|text` lines — and the room plays it when its digest matches the brief's (`run B`, below).
The pre-flight validator refuses, by name, a dead end, a branch that does not terminate in an
enumerated outcome, and a line that contradicts the record:

```
$ python3 tools/meetings/worker.py validate --script build/model-check/dead-end.json
REFUSED dead-end b2: it reaches no enumerated outcome
$ python3 tools/meetings/worker.py validate --script build/model-check/no-terminus.json
REFUSED branch b1 does not terminate in an enumerated outcome (outcome 9, options 4)
$ python3 tools/meetings/worker.py validate --script build/model-check/contradicts.json
REFUSED branch b1 contradicts the record: it says "Tuvok"
```

On failure the worker regenerates (up to `--max-attempts`), and — because a brief the model could not
serve writes **no** script — the meeting plays from the authored skeleton. **A material change between
generation and the meeting** leaves the brief's `stateDigest` different from the script's, so the room
falls back to the authored skeleton and the brief is written again for the async worker:

```
    model test: after a material change the script was loaded=1, is fresh=0; the room plays the authored skeleton
    meetings: the script is stale for watch-change; queued for regeneration
$ python3 tools/meetings/worker.py generate --manifest ship/meetings/regenerate.jsonl \
      --script ship/meetings/scripts-regen.txt --ledger build/model-check/regen-ledger.txt
valid   : watch-change: 4 branch(es) accepted
```

This is the "regenerate" branch of the design's either/or; the authored interruption-hook alternative
is named and not built (a judgement call, below).

## Task C — the classifier, which is classification and not generation

Typed input is matched against the option intent descriptors by `all-minilm`; above the tuned
threshold (0.55) the branch plays and no generator is touched; below it exactly one live call happens,
its answer is written down and promoted:

```
$ python3 tools/meetings/worker.py classify --manifest ship/meetings/classify.jsonl \
      --ledger ship/meetings/novel.txt --script ship/meetings/scripts.txt \
      --transcript ship/meetings/promoted.txt --max-calls 1
novel   : "Make it so" -> live call; answer 'Bringing her to yellow.'; promoted to branch 3
== 0 matched, 1 novel call(s), generation calls=1

$ python3 tools/meetings/worker.py classify --manifest match.jsonl --ledger match-ledger.txt --max-calls 0
matched : "watch the reserve closely" -> branch 2 (0.66 >= 0.55); no generator call
== 1 matched, 0 novel call(s), generation calls=0
```

The second line is the point: a genuinely matching input is **classification**, and a budget of zero
proves the generator was never touched.

`ClassifyNovelInput` consults a **content-addressed index**, keyed by `NoveltyKey(brief, text)` over
the kind, the text and every option's intent; the verdict is what the worker wrote. With no verdict
loaded the seam still reports **NOVEL** and applies nothing — the M3 safety property is intact. A
second pass over the same manifest calls nothing:

```
$ python3 tools/meetings/worker.py classify --manifest ship/meetings/classify.jsonl ...
cached  : 7ce10140 -> branch 2 (no call)
== 0 matched, 0 novel call(s), generation calls=0
```

**The check counts the calls, and it can fail.** Two fresh novel inputs against a budget of one:

```
$ python3 tools/meetings/worker.py classify --manifest two-novel.jsonl --ledger over.txt --max-calls 1
== 0 matched, 2 novel call(s), generation calls=0
FAIL  2 novel call(s) exceed the budget of 1
```

## Task D — determinism and the boundary

- **The model never writes ship state.** The worker produces text and selects from the enumerated
  outcomes; the simulation applies them, exactly as a pill does. Structurally: the module's script
  loader copies the *authored* options (with their effects) and takes only the model's dialogue lines,
  so a model script cannot name a new effect. The worker's script is validated before it is usable.
- **A save replays identically.** Dialogue lives in the script (beside the save) or in the log (in the
  save), never regenerated. The verdict index is content-addressed, so the same input always resolves
  the same way. Run B shows the reloaded ship classifying the same input as matched, with the call
  count unchanged:

```
    model test: the room plays from the generated script; 1 script(s) waiting before the player arrived
    model test: typed "Make it so": matched branch 3
    model test: novel-path calls=1
    model test: novel-path calls=1 (before the load 1); the reloaded ship classifies the same input as matched branch 3
    model test: verdicts loaded=1, scripts loaded=1, generation calls=1
```

- **No inference on arrival.** Run A opened the room and reached the decision with no script and no
  verdict; no model was called by the engine at any point. The engine only wrote manifests and read
  files.
- **The register** moves four hooks in and re-derives the counts from its own rows: `NoveltyKey`,
  `ClearNoveltyIndex`, `AddNoveltyVerdict` (reached in play) and `NoveltyVerdictCount` (console).
  `73 + 61 + 54 + 166 + 18 = 372`, and `214 + 123 + 35 = 372`. `scripts/hooks-check.sh` passes.

## The commands, and what they returned

```
scripts/model-call-check.sh --map tour/deck04
  -> PASS (the whole path: run A, the validator, the generator, the classifier, run B, the model absent)
scripts/meeting-overlay-check.sh --map tour/deck04
  -> PASS (the overlay, with the Task A layout and three frames)
scripts/test.sh               -> all checks passed
scripts/check.sh              -> the entity dictionary and the script corpus as before
tests/ship/test_ship_core     -> ship_core: all checks passed (including TestNoveltyIndex)
tests/tools (unittest)        -> OK (including test_meeting_worker)
```

## What could not be verified

- **Whether a meeting *reads* as a room, and whether the counter-play of a novel answer feels like
  one.** That is the owner's, and it needs a picture. This lane can show the script, the call counts
  and the frames; it cannot judge the scene.
- **The generated prose.** It is a 3B model's, at a script of the skeleton's own scale; whether a line
  is *good* is the owner's ear. The validator guarantees only that it terminates and does not
  contradict the record, not that it is well written. The model is sampled, so the live answer varies
  between runs: the exchange quoted below is the run on the final tree, and the promoted transcript is
  that player's cache, never a fixture. (An early run echoed the player's own words back; the live
  prompt now forbids it, and the answer below is in character.)
- **The stale-script interruption hook.** The design's either/or is satisfied by *regenerate*; the
  authored "the state changed" line variants are named and not built.
- **The verdict index is host-local, beside the save, not inside the save blob.** It makes a save
  replay identically on the machine that holds it (the design's posture, as with the voice cache);
  a save carried to a machine with neither model nor cache plays from the skeleton. Named as a call.

## Judgement calls, named as calls

1. **Task A first, the two overlay fixes the owner approved.** Done first; the layout is measured,
   not asserted.
2. **The intent descriptor is the option's label with its cost.** The brief carries `intent` as a
   numeric id; the text the worker embeds — and the pill shows — is `label (cost)`. One place,
   `IntentDescriptor`.
3. **The script carries dialogue; the options and their effects stay the authored brief's.** The
   loader copies the authored options and takes only the model's lines, so a script cannot introduce
   an effect or a branch that the simulation did not already enumerate. This is what keeps "the model
   selects from the enumerated outcomes" true by construction rather than by trust.
4. **A matched typed input offers its branch; the pill still resolves it.** `ship meeting say` does
   not apply state even on a match, so M3's property — a say never writes ship state — holds; the
   promotion is offered (the verdict and the appended line) and a pill is the resolution.
5. **The verdict index is a file beside the save, content-addressed, and the module reads it at init
   and on command.** It is not in the save blob (no version bump), following the voice cache's
   posture: player-local, pruned with the save, never in the repository.
6. **`meet.said` lives in the speaker rail.** Anywhere in the right panel risks the stamp's box; the
   rail cannot reach it.
7. **The call ledger is the worker's, and the budget can fail.** `--max-calls` returns non-zero when a
   second call happens; the check proves it does.
8. **The model-absent demonstration stops nothing shared.** `sasquatch` is shared and other sessions
   use `ollama`; the worker is pointed at a dead port instead, writes no script, and the engine plays
   from the skeleton. The behaviour — no model, no dependency — is what is shown.

## The boundary, kept

No model file, no cache, no generated content and no audio is committed — not to this file, not to the
branch, not to an issue, not to a pull request. The only model artefacts are the two the owner pulled
(`all-minilm`, `qwen2.5:3b`), which stay in ollama's store. Nothing was pulled and no dependency was
added. Nothing is committed to `main`.
