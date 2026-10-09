# Voice out, and the casting map — the meeting with a cast and a voice (M5)

`docs/staff-meetings.md` (the owner's design), `docs/the-entry-point.md` (*"And the voice follows the
cast"*), `docs/handoff-live-crew-and-meetings.md` (M5), `docs/evidence/audio-plumbing.md` (phase two:
the three tracks, the render queue and the cache) and `docs/evidence/model-call.md` (M4, the sibling),
applied on `feat/voice-and-casting` (cut from `testing`), 2026-10-09.

**Observed, and the command that produced it.** Everything below is the output of a command named
beside it, on this tree, after `cmake -S ../upstream/efgame -B ../upstream/efgame/build-linux
-DLWH_MODULE_DIR=$PWD/module && cmake --build ../upstream/efgame/build-linux --target efgame --target
efui` (both modules built cleanly, only the pre-existing `-Wwrite-strings` warnings). The build host
is `sasquatch`, CPU-only, no GPU. The Python environment is a throwaway venv built by `uv` from its
cache under the gitignored `voice-scratch/`, with `HF_HOME` pointed at `voice-scratch/hf-cache`;
nothing was vendored and no dependency was added to the repository. **No model, no cache and no
generated audio are committed.**

## What was built, and where

| file | what it is |
|---|---|
| `module/ship/ship_core.{h,cpp}` | **the casting map** (`VoicePoolCount`, `VoicePoolName`, `VoiceIsCanonType`, `CastVoice`, `CastIsCanon`), the pacing (`CachedDuration`, `MeetingLineKey`, `MeetingLineSeconds`), and the pause (`SubmitNovelAnswer`); `PlanMeetingAudio` now names each line's **cast reference**, and `CacheRendered` carries the clip's measured duration. |
| `module/ship/g_ship.cpp` | the host half: `LoadVoiceDurations` (the renderer's measurements, beside the save), the line's duration published as the 7th cvar field, the `ship cast` readout, `ship meeting say` emitting its pause cue, and the `g_shipTest 84` demonstration. |
| `module/ui/ui_lwh_meeting.cpp` | the room reads each line's duration and offers that pill for exactly that long. |
| `tools/voice/render.py` | measures every clip it writes (`ffprobe`) and records `key|seconds`; skips a clip already on disk. |
| `scripts/audio-check.sh` | one more headless run (`g_shipTest 84`) for the cast, the pause and the pacing; the check is hermetic (it clears the cache first). |
| `tests/ship/test_ship_core.cpp` | `TestCastMap`, `TestCastDeterminism`, `TestPauseCue`, `TestLineDurations`, and the `--voice` printout. |

## Task A — the casting map, for the three proving cases

The cast is **cast-state data derived from the start state**, not a fixed table, and it is derived
rather than stored: the static half of a record comes back from the seed, so the same start state
casts the same map on the first build and again on a load. A voice is a **reference clip** the
analyzer builds from the retail assets the player owns, so the map names a reference identity
(`tuvok`, `munro`), not a display name.

```
$ build/…/test_ship_core --voice
== the casting map: cast-state data derived from the start state
  the non-canon pool (14 voices): munro alexa chell telsia foster biessman jurot csatlos nelson jaworski odell oviedo kenn laird
  canon survivor: Kathryn Janeway -> voice janeway (the retail voice)
  canon survivor: Tuvok -> voice tuvok (the retail voice)
  the chair: Kathryn Janeway is dead -> voice "" (no voice; no line authored)
  all-fictitious: 141 crew, 0 canon voice(s), 141 from the pool
  the same start state re-cast gives the same map: yes
```

And the three cases, headless, in the engine:

```
$ scripts/audio-check.sh        (the g_shipTest 84 run)
    SHIP: cast test: canon: 9 canon voice(s), 132 from the non-canon pool
    SHIP: cast test: canon survivor Tuvok -> tuvok (the retail voice)
    SHIP: cast test: the chair: Janeway is dead -> voice "" (no voice; no line is authored)
    SHIP: cast test: all-fictitious: 141 crew, 0 canon voice(s) used
    SHIP: cast test: the same start state re-cast gives the same map=1
```

- **Canon** (the default): the nine show command-crew records keep their own retail voice -- Janeway,
  Chakotay, Tuvok, Paris, Kim, Torres, the Doctor, Seven, Neelix -- and the remaining **132** records
  (the hazard team, the minor crew and the generated crew) are cast from the non-canon pool.
- **The chair**: the captain died of the Caretaker's array, so `CastVoice` is `""`; no reference is
  made, nothing is rendered, and no line is authored for her.
- **All-fictitious**: none of the show characters appears, and **no canon voice is used at all**.
- **Determinism**: the same start state re-cast gives the same map, and a pack/unpack derives the same
  map again (`TestCastDeterminism`).

**The pool is a decision taken in this brief, named as one.** The design says *"a voice from our own
casting"*; the mechanism requires a reference clip built from assets the player owns. The
implementable and provenance-clean reading is to cast a fictitious crew member from the **non-canon
pool** -- the hazard team and minor crew whose retail voices exist (`munro`, `alexa`, `chell`,
`telsia`, `foster`, `biessman`, `jurot`, `csatlos`, `nelson`, `jaworski`, `odell`, `oviedo`, `kenn`,
`laird`), which is **not** the canon command crew and **not** the synthetic enemy generics
(`genimp*`, `genklg*`). It is a decision, not a reading, and can be overruled.

## Task B — voice out, during generation, keyed by the cast

`PlanMeetingAudio` renders **every scripted line during generation**, off the critical path, and the
render job now names the **cast reference** rather than the speaker's display name. The manifest the
Python renderer drains carries the reference, so the clip is built from the right voice:

```
$ cat build/g3-home/baseEF/ship/voice/render.jsonl
{"key":"10e6e126657f6e19","voice":"janeway","text":"You have the watch. Carry on.","delivery":"order","exaggeration":0.80,"reference":"refs/janeway.wav"}
{"key":"cfc80414ee16a46e","voice":"jaworski","text":"Engineering holds at the allocation we set.","delivery":"report","exaggeration":0.50,"reference":"refs/jaworski.wav"}
…
{"key":"0ddf741df1bfeba0","voice":"laird","text":"Condition yellow, ship-wide.","delivery":"report","exaggeration":0.50,"reference":"refs/laird.wav"}
```

The delivery direction reaches the synthesizer's own knob exactly as phase two defined it (order →
0.80, report → 0.50). **The engine makes no inference call at any point**: it writes the manifest and
reads files; the renderer runs in the async window.

## Task C — the pause cue, and the deferral

A novel answer is the one place a live call happens; the wait is filled, not hidden. Each character
reaches for one of the four non-verbal cues (chosen from their own record and the seed), and it plays
**immediately** at submission, on the cue track -- so it can never stop a dialogue line. When the
answer arrives inside the pause the cue retires and the answer resolves; when the model is slower the
**deferral line** lands and the outcome moves to a later beat.

```
    SHIP: cast test: pause: fast cue=0, resolved inside the pause=1
    SHIP: cast test: pause: slow deferral=2 "I have to think about that." -> the outcome moves to a later beat
$ build/…/test_ship_core   (TestPauseCue)
  … the cue emit site fired at submission; the slow path leaves CUE_HOLDING on the cue track and the
    dialogue track untouched
```

In play (`g_shipTest 82`), the typed-input path now routes through it:

```
    meeting say: "Make it so": no branch matched (novel); cue I have to think about that.
    meeting say: deferred: the answer lands later (a message, a corridor conversation, the log)
```

## Task D — the boundary, demonstrated

**A meeting plays with zero inference.** The engine's plan writes a manifest and reads files; the
meeting then plays from the script and the cache. The check's own line:

```
    SHIP: cast test: voice out during generation: planned 6 line(s), queue unfinished=6, inference calls=0
```

**True durations pace the pills.** `render.py` measures every clip it writes and records it; the host
loads the measurements and the room offers each pill for exactly that long (`MeetingLineSeconds`).
The measured lines below are the six lines of the watch-change skeleton, rendered on this host:

```
$ .venv/bin/python tools/voice/render.py --cache build/g3-home/baseEF/ship/voice \
      --manifest build/g3-home/baseEF/ship/voice/render.jsonl
render  : janeway delivery=order exaggeration=0.8
…
cached  : … (a re-render is a no-op, key …)
durations: 6 clip(s) measured -> build/g3-home/baseEF/ship/voice/durations.txt
```

Measured on this host, from the six watch-change lines (the cast voice, the line, its delivery, and
the clip's own length):

| key | cast voice | line | delivery | measured |
|---|---|---|---|---|
| `10e6e126657f6e19` | janeway | *"You have the watch. Carry on."* | order | **1.820 s** |
| `cfc80414ee16a46e` | jaworski | *"Engineering holds at the allocation we set."* | report | **2.440 s** |
| `de4adfafd586eba9` | jaworski | *"I will watch the dilithium reserve and report any fall."* | report | **3.200 s** |
| `2fe19c11d7f2f5f1` | janeway | *"Do that."* | order | **0.620 s** |
| `0ddf741df1bfeba0` | laird | *"Condition yellow, ship-wide."* | report | **2.100 s** |
| `50e406f25eaaa469` | janeway | *"Make it so."* | order | **1.140 s** |

The six lines of the skeleton run **11.320 s** in total, and that is what the room is paced by.

With the durations loaded, the same run reports the measured pace rather than the text fallback, and
a second plan is a no-op because every key is already cached:

```
$ scripts/run-engine.sh --home-dir build/g3-home +set g_ship 1 +set g_shipMode 1 +set g_shipTest 84 +map tour/deck04
    SHIP: cast test: voice out during generation: planned 0 line(s), queue unfinished=0, inference calls=0
    SHIP: cast test: pacing: "You have the watch. Carry on." runs 1.820s (the clip's own measured duration); inference calls=0
    SHIP: cast test: the cache prunes with the save: 6 of 6 entr(ies) dropped, cache now 0
```

The plan is a **no-op because every key is already cached** (`planned 0`) -- a reload replays rather
than re-rendering -- and the line's pill is offered for **1.820 s**, the clip's own measured length.
With the cache cleared (`scripts/audio-check.sh` does), the same run plans the six lines and reports
the text fallback for the pacing:

```
    SHIP: cast test: voice out during generation: planned 6 line(s), queue unfinished=6, inference calls=0
    SHIP: cast test: pacing: "You have the watch. Carry on." runs 1.740s (text fallback, no clip rendered); inference calls=0
```

**The cache prunes with the save**, and a reload replays rather than re-rendering:

```
    SHIP: cast test: the cache prunes with the save: 6 of 6 entr(ies) dropped, cache now 0
$ .venv/bin/python tools/voice/render.py …     # a second time: every clip is cached, none is re-rendered
== 0 to render, 6 cached, 0 refused
```

**Nothing audio-shaped enters the repository.** `scripts/audio-check.sh` fails if `git ls-files`
matches any audio extension, and the cache is gitignored:

```
$ scripts/audio-check.sh
PASS  … the repository carries no audio;
      the casting map is derived and deterministic (canon, the chair, all-fictitious); the lines are
      planned with cast references and no inference; a novel answer cues immediately and resolves or
      defers; a line's measured duration paces the pill
```

## The clips produced, for the owner's ear

The six watch-change lines, rendered on this host from the character references, live under the
gitignored scratch and are **not** in the repository:

```
voice-scratch/out/janeway-you-have-the-watch-carry-on.{wav,mp3}
voice-scratch/out/jaworski-engineering-holds-at-the-allocation-we-set.{wav,mp3}
voice-scratch/out/jaworski-i-will-watch-the-dilithium-reserve-and-report-any-fall.{wav,mp3}
voice-scratch/out/janeway-do-that.{wav,mp3}
voice-scratch/out/laird-condition-yellow-ship-wide.{wav,mp3}
voice-scratch/out/janeway-make-it-so.{wav,mp3}
```

**Whether these sound like their characters is the owner's judgement**; nothing here can answer it.
The cast for this meeting is Janeway (the chair), a pool voice for Engineering (`jaworski`) and one
for Security (`laird`), because the watch's own people are who the brief put in the room.

## What could not be verified

- **Whether the cast sounds right.** That is the owner's ear, and it needs the clips brought to him;
  their paths are above. The F0/rate proxy of `voice-review.md` can screen a clip, not judge it.
- **Whether the per-character pause cue reads as a person rather than a soundboard.** No ear.
- **The non-canon pool's distinctness.** `voice-review.md` finding five: a minor character's
  performance carries less vocal characterisation, so a pool casting is less distinct by construction.
  That is the reason the design gives our own crew voices we choose; the pool is the implementable
  half of it.
- **The physical deletion on `ship voice prune` when no home path is exposed.** The repository-clean
  check is the load-bearing half.

## Judgement calls, named as calls

1. **The pool, and the rule that a canon character who survived keeps their own retail voice.** The
   show command crew (nine records) are the canon voices; everyone else is cast from the pool. Named
   as a decision above; overrulable.
2. **The pause budget is a constant (`NOVEL_PAUSE_SECONDS = 1.4s`)**, the length of a cue clip, not a
   measured value. A fast answer resolves inside it; a slow one defers. The two halves are one module
   function for the demonstration; the real host emits the cue at submission and resolves when the
   worker answers.
3. **The text-length fallback paces a line with no clip** (`max(0.8s, 0.06·chars)`), so the room still
   advances with the model absent. It is a reading estimate, named as one, and the overlay's playback
   test was re-timed to follow it (the M3 fixed 650 ms tick is superseded).
4. **The cast is a `std::string` reference identity**, so two people cast to the same voice share a
   cache key and a clip. The cache key is unchanged from phase two.
5. **The durations file is `key|seconds` beside the cache**, written by the renderer, read by the
   host at init and on `ship meeting reload`; the clips themselves are never in the repository.
6. **The manifest carries `reference`**, so the renderer builds `refs/<voice>.wav` under the cache;
   the reference rule (one line, one speaker, one condition) is the review's, unchanged.

## The boundary, kept

No generated audio, no reference clips and no extracted game assets are committed -- not to this
file, not to the branch, not to an issue, not to a pull request. The retail install was read only;
the venv, the model weights and every rendered clip live under the gitignored `voice-scratch/` on
this host. Nothing is committed to `main`. The engine and its modules were rebuilt; no engine process
or agent session was left running.
