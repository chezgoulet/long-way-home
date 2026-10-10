# Voice in — the transcribed string takes the identical path (M6, second half)

`docs/staff-meetings.md` (the owner's design: *"Speech-to-text produces a string that takes exactly the
same path through the intent descriptors"*, and the memory plan), `docs/handoff-live-crew-and-meetings.md`
(M6, the acceptance *"a spoken novel reply takes the identical path"*), `docs/evidence/model-call.md`
(the novelty seam, the verdict index and the promotion, M4), `docs/evidence/meeting-overlay.md` (the
free-text pill, M3), `docs/evidence/the-computer-api.md` (M6's first half, the refusal and the enumerated
set), and `docs/evidence/voice-spike.md` (the throwaway-venv procedure, and an ASR model used as an
instrument and not a dependency), applied on `feat/voice-in` (cut from `testing`), 2026-10-10.

**Observed, and the command that produced it.** Everything below is the output of a command named beside
it, on this tree, after `cmake -S ../upstream/efgame -B ../upstream/efgame/build-linux
-DLWH_MODULE_DIR=$PWD/module && cmake --build ../upstream/efgame/build-linux --target efgame --target
efui` (both modules built, only the pre-existing `-Wwrite-strings` warnings). The engine runs are headless
(`xvfb-run`, `SDL_AUDIODRIVER=dummy`) on `sasquatch`, CPU-only. **No microphone was involved at any
point**, and the engine has no capture path to involve: the boundary demonstrated here is **an audio
file**. No model, no cache, no audio and no transcript is committed.

## The premise, measured — and what this pass does not claim

`docs/handoff-live-crew-and-meetings.md` states the engine has no audio-capture path: there is no
`SDL_OpenAudioDevice` capture, no `iscapture`, no capture API anywhere in the upstream sources. That is
why **"spoken" is answered at the transcription boundary**: an audio file is transcribed to a string, and
that string takes the identical path a typed one takes. A live microphone needs an engine extension
(Task D, priced and not built) *and* a host with audio; this one is headless under `xvfb`. The
demonstration below therefore proves the **transcription seam and the identity of the path**, and says
plainly that it did **not** involve a microphone and cannot say whether speaking to the ship feels
different from typing to it.

## What was built, and where

| file | what it is |
|---|---|
| `tools/speech/transcribe.py` | **new**: the STT worker — `transcribe` (one audio file to one string), `drain` (the module's `voice.jsonl`), `residency` (the memory plan). Borrows the spike's sherpa-onnx whisper tiny.en, read in place. |
| `module/ship/g_ship.cpp` | the one free-text path `SubmitRoomInput`, reached by both `ship meeting say` and the new `ship dictate say`; the capture-boundary manifest (`ship/meetings/voice.jsonl`) and the transcript reader (`ship/meetings/transcript.txt`); the `ship dictate on\|off\|say` verb; `g_shipTest 86`. |
| `module/ship/console_api.def` | the `dictate` row, so the console verb and the ship's API cannot drift. |
| `scripts/voice-in-check.sh` | **new**: run A (worker absent), Task A (the transcript), run B (the identity), the classifier's call count, run C (the replay), the memory rule, the boundary. |
| `tests/ship/test_ship_core.cpp` | `TestVoiceInIsTheTypedPath`: a transcribed string is the typed string (same key, same verdict, same note). |
| `tests/tools/test_speech_worker.py` | **new**: the worker's pure half — the fields, the transcript file, the model discovery — with no model and no network. |

**The save format is unchanged.** `voiceInMode`, the pending count and the last transcript are host
state; the script and the verdict index were already player-local files beside the save (M4); and
nothing here writes ship state.

**The build is not the proof, but the symbols must be there.** No new `.cpp` was added under `module/`
(the files edited already existed and were in the glob), and `nm` confirms the path is in the built
library: `SubmitRoomInput`, `LookupTranscript` and `WriteVoiceRequest` are all present, and the
`SHIP: dictate:` strings are in `libefgame.so`:

```
$ nm -C ../upstream/efgame/build-linux/libefgame.so | grep -E 'SubmitRoomInput|LookupTranscript'
00000000001291c0 t SubmitRoomInput(std::__cxx11::basic_string<...> const&)
0000000000121600 t (anonymous namespace)::LookupTranscript(std::__cxx11::basic_string<...> const&)
```

## Task A — the transcription seam

The worker turns the capture file into the string. It reads a 16-bit mono wav, resamples to 16 kHz, and
decodes with the borrowed whisper tiny.en int8 pair on CPU:

```
$ voice-scratch/venv/bin/python tools/speech/transcribe.py transcribe \
      --audio .../sherpa-onnx-whisper-tiny.en/test_wavs/0.wav \
      --out  build/g3-home/baseEF/ship/meetings/transcript.txt \
      --expected "AFTER EARLY NIGHTFALL ... THE SQUALID QUARTER OF THE BROTHELS"
resident: sherpa-onnx whisper tiny.en (STT) from .../sherpa-onnx-whisper-tiny.en -- the one model
          this process loads; it is freed when this process exits
audio   : .../test_wavs/0.wav (6.62s)
load    : 0.41s
decode  : 0.44s (0.07x realtime)
text    : After early nightfall, the yellow lamps would light up here and there the squalid quarter of the brothels.
expected: AFTER EARLY NIGHTFALL THE YELLOW LAMPS WOULD LIGHT UP HERE AND THERE THE SQUALID QUARTER OF THE BROTHELS
normalised match: yes
peak RSS: 260 MB
wrote   : .../ship/meetings/transcript.txt (A|.../0.wav|...)
```

**What it needs, and what it costs.** `sherpa-onnx` 1.13.8 in a throwaway venv under the gitignored
`voice-scratch/` (`uv venv voice-scratch/venv` then `uv pip install sherpa-onnx==1.13.8`, built from the
uv cache offline; 43 MB), and the borrowed model read **in place** — the int8 encoder and decoder and the
token file, about 103 MB, under another tree. Nothing is copied and nothing is committed. Load is a
fraction of a second warm; decode is 0.07× realtime for this clip, so a spoken answer costs well under a
second on this CPU. **It is an instrument, not a dependency**: the module writes a request and reads a
file, and with the worker absent the game plays on (run A, below).

*One honest limit, recorded:* the model is `tiny.en`, and it is not perfect. On the third test clip it
reads "Hester Prynne" as "Hester-Prin"; the identity demonstration therefore uses clip `0.wav`, whose
transcript matches the model's own expected text under normalisation.

## Task B — the identical path, which is the acceptance

**The one function.** The body of the `ship meeting say` branch is now a function, `SubmitRoomInput`;
`ship meeting say <t>` calls it, and the new `ship dictate say <audio>` calls it with the transcribed
string. There is no second path for voice — the design says there is none, and the code obeys.

Run A is the STT worker absent: the request is queued and the string never exists, so the seam is never
reached and the meeting plays on.

```
$ scripts/voice-in-check.sh
==> run A: the game with the STT worker absent
    dictate: the capture boundary holds ".../test_wavs/0.wav"; queued one transcription for the worker
    dictate: no transcript for ".../test_wavs/0.wav" yet; the transcription is pending and the room is
             unaffected (the STT worker is absent)
    voice-in test: the STT worker is absent: "..." has no transcript, the input is pending, no string
             reaches the seam, and no classification is queued
    meeting choose: watch-change outcome 1 applied (decided by Reyes)
```

Run B is the same line, spoken and typed. **The strong test is identity, not resemblance.** From the same
pre-input snapshot, the transcribed string and the identical typed string give the same verdict, the same
log entry and a **byte-identical** ship state, and the two console paths hash to the **same**
content-addressed key:

```
==> run B: the transcribed and typed forms of the same line
    voice-in test: the STT worker transcribed ".../0.wav" -> "After early nightfall, the yellow lamps
             would light up here and there the squalid quarter of the brothels."
    dictate: "..." transcribed to "..."; the string now takes the path a typed one takes
    meetings: queued one classification for "..." (key d1818c2f)
    voice-in test: the spoken form (console): verdict novel; the log entry: a novel answer was given: ...
    meetings: queued one classification for "..." (key d1818c2f)
    voice-in test: the typed form (console):  verdict novel; the log entry: a novel answer was given: ...
    voice-in test: the verdict is the same: yes
    voice-in test: the log entry is the same: yes
    voice-in test: the ship's state is byte-identical across the two paths: yes (9912 bytes),
             the log entry identical: yes
```

**How the byte-identical state is measured, and why it is measured that way.** The two console commands
run on different frames, and between frames the ship's own clock advances (`Ship_Frame` ticks the
simulation), so a console-to-console pack comparison would measure the ticks, not the seam. The identity
is therefore measured **synchronously, at the one function both console commands call**: from a single
`Pack` snapshot, `SubmitRoomInput` is called with the transcribed string and then, from the same
snapshot, with the identical string, and the two packs are compared byte for byte. The console evidence
is the identical log entry and the identical key above. Named as a call, below.

**At most one inference call across both.** The voice and typed forms of the same text are the same
content-addressed key, so the classifier makes exactly one live call and caches every repeat:

```
==> the classifier: one live call for the repeated spoken input; a replay calls nothing
    novel   : "..." -> live call; answer 'Bring her to yellow.'; promoted to branch 3
    cached  : d1818c2f -> branch 3 (no call)   (x3)
    == 0 matched, 1 novel call(s), generation calls=0
    cached  : d1818c2f -> branch 3 (no call)   (x4)
    == 0 matched, 0 novel call(s), generation calls=0
```

**The replay costs nothing.** With the verdict loaded, the same spoken input resolves to its branch and
the call count is unchanged; the M4 replay-from-the-log is exactly the same for voice:

```
==> run C: the same spoken input, replayed from the log
    voice-in test: the spoken form (console): verdict matched branch 3; the log entry: an answer matched the branch: ...
    voice-in test: the typed form (console):  verdict matched branch 3; the log entry: an answer matched the branch: ...
    voice-in test: the verdict is the same: yes
    voice-in test: the log entry is the same: yes
    voice-in test: the ship's state is byte-identical across the two paths: yes (9947 bytes), the log entry identical: yes
    voice-in test: replay: verdicts loaded=1, novel-path calls=1; the same spoken input resolves as matched branch 3
```

The transcribed string also takes the computer's identical path: `ship dictate say` reaches
`SubmitRoomInput`, which routes the computer's room to `AddressComputerToConsole`, the same
`FindConsoleVerb` membership test the typed pill uses (Task B of `docs/evidence/the-computer-api.md`).
No new machinery was added for that; it is the same function.

## Task C — the memory rule, observed

The STT model is the third resident model, and **no two run at once**. The worker is a one-shot process:
it loads one model at start and frees it at exit. The module only ever *queues* an STT request in voice
mode, and the check sequences the workers, so the STT model is never running beside the embedding
matcher, the generator or the synthesizer:

```
==> the memory rule: which model is resident when
      STT (this worker, voice mode only), one model at a time:
        resident: sherpa-onnx whisper tiny.en (STT) from ... -- the one model this process loads;
                  it is freed when this process exits
        model present: yes
      the embedding matcher (the novelty classifier) is ollama's, resident only during classify
      the generator is ollama's, resident only during async generation
      the synthesizer is the render worker's, batched during generation
    this process loads exactly one of the four, and never at the same time as another.
    the STT model is loaded only by the transcribe worker above, for the life of that one process
    ollama ps, after classify (the meeting worker's models; no STT model here):
      qwen2.5:3b           357c53fb659c    2.1 GB    100% CPU     4096       4 minutes from now
      all-minilm:latest    1b226e2802db    73 MB     100% CPU     256        4 minutes from now
```

**A limit named rather than hidden.** `ollama ps` after classify shows *both* meeting models resident at
once: `all-minilm` (the matcher) and `qwen2.5:3b` (the generator). They are resident together because
ollama keeps a model loaded after a call; they do not **run** together, which is the design's rule, and
that is ollama's keep-alive, inherited from M4, not this pass's doing. What this pass guarantees is the
part it owns: the STT model is a separate one-shot process, run only in voice mode, and it is never
co-resident with the meeting worker's models. The game runs with the STT worker absent: **run A** above,
where the string is never produced and the room plays on.

## Task D — the capture seam, priced and not built. Build none of it.

A microphone is a priced option, not a wish, and **none of it is built in this pass.**

- **What it is.** An **engine extension**: SDL capture (`SDL_OpenAudioDevice(..., SDL_AUDIO_DEVICE_...,
  &want, ...)`, the `iscapture` direction) writing a ring buffer to a wav, exactly the file this pass
  uses as the boundary. It is engine work — the module cannot open a device and must not (`module/`
  holds game logic; a device is the engine's).
- **How it is gated.** Per `docs/engine-extension-policy.md`: a new cvar, **off by default**, so retail
  and cMod behaviour and saves are untouched. With the cvar off, nothing opens and the client is exactly
  what it is today; the `g_crew 0` / `g_ship 0` pattern is the one to copy.
- **What it touches.** One engine patch in the patch series: the capture open/close and the buffer flush
  (a `cmd.c`/`sp_integration.c`-style seam), a cvar registration, and a path under
  `ship/meetings/` beside the save. The module side already exists — it reads the same file today.
- **Rebasable and recorded.** It is one additive patch with one concern, and its rebase cost is recorded
  in `CONTRIBUTING.md`'s delta table, as the policy requires.
- **Why it cannot be demonstrated here.** This host is headless under `xvfb` and runs the engine with
  `SDL_AUDIODRIVER=dummy`; there is no capture device. A real microphone is an **owner's-session item**,
  on a host with audio, and it needs the extension above first.

The priced conclusion: **the seam on this side is finished and free; the missing half is an engine
extension plus a device, and it is deliberately not this pass.**

## The commands, and what they returned

```
scripts/voice-in-check.sh            -> PASS (run A, Task A, run B, the classifier, run C, the memory
                                        rule, the boundary)
scripts/test.sh                      -> all checks passed (crew, ship incl. TestVoiceInIsTheTypedPath,
                                        tools incl. test_speech_worker, hooks, api, harness, docs, syntax)
scripts/check.sh --source-map ... --script-corpus ... -> the entity dictionary and the script corpus as before
scripts/meeting-check.sh             -> PASS
scripts/meeting-overlay-check.sh     -> PASS
scripts/model-call-check.sh          -> PASS
scripts/audio-check.sh               -> PASS
scripts/computer-check.sh            -> PASS
scripts/api-check.sh                 -> PASS (the console answers 129 verb(s); the API enumerates 129),
                                        in both directions -- the `dictate` row was added to keep them equal
tests/ship/test_ship_core            -> ship_core: all checks passed (including TestVoiceInIsTheTypedPath)
tests/tools (unittest)               -> OK (including test_speech_worker)
```

The api-check caught this pass's own new verb: adding `ship dictate` to `Svcmd_Ship_f` made
`scripts/api-check.sh` fail with `FAIL ... (1): dictate` until the row was added to
`console_api.def` — the guardrail doing its job on the work that added it.

## What could not be verified

- **Whether speaking to the ship feels different from typing to it.** That needs the microphone this
  pass does not build, and a host with audio. It is the whole substance of the acceptance that remains
  open, and it is not replaced by an instrument reading.
- **The STT accuracy beyond one clip.** `tiny.en` is small; it mis-spelled a name on another test wav.
  The worker reports each transcript and a normalised match; a larger model behind the same `--model-dir`
  is a substitution, not a change to the seam.
- **Whether the meeting worker's models stay resident together is acceptable.** Observed above and named:
  they are resident together, they do not run together. That is the M4 posture, and this pass did not
  change it.

## Judgement calls, named as calls

1. **The boundary is an audio file, not a microphone.** The engine cannot capture, and the honest
   reading of "spoken" at this boundary is a file transcribed to a string. The microphone is priced (Task
   D) and not built; no evidence here implies one was used.
2. **The `ship meeting say` body became `SubmitRoomInput`, and both paths call it.** One free-text path,
   reached by a pill, a typed line and a transcribed line. A second path for voice would have been the
   defect; there is none.
3. **The seam's log wording is provenance-neutral**: "a novel answer was given: ..." / "an answer matched
   the branch: ...". The old "a typed answer matched the branch" would have made a spoken line write a
   different entry than a typed one, and *the same log entry* is the acceptance.
4. **Identity is measured at the function boundary.** Across frames the ship's clock advances, so a
   console-to-console pack comparison measures the ticks; the console evidence is the identical log entry
   and the identical key, and the byte-identical pack is measured synchronously from one snapshot.
5. **`ship dictate on|off|say` is a console verb, and its row is in the ship's API.** The free-text
   pill's `VOICE` affordance is where a player would reach this; with no microphone the deliberate reach
   is the console, and the verb is named `dictate` to keep it distinct from M5's voice-out `ship voice`.
6. **The transcript is player-local, under `ship/meetings/` beside the save, and gitignored.** It is a
   line of text, not an asset; it is not copied from anywhere and never committed.
7. **The STT model is read in place from another tree.** The worker's default model dir is the spike's
   borrowed whisper tiny.en; `--model-dir` points it anywhere. Nothing is copied into `voice-scratch/`.
8. **The worker reports its own residency and footprint**, so "which model is resident when" is a
   measurement the worker makes about itself, not a claim made about it.

## What voice in will need next (named, and not built)

The **microphone engine extension** (Task D) and the free-text pill's `VOICE` key wired to
`ship dictate say`. Everything between the string and the simulation exists and is proved above.

## The boundary, kept

No model, no cache, no audio and no transcript is committed — not to this file, not to the branch, not
to an issue, not to a pull request. The venv and the transcript live under the gitignored
`voice-scratch/` and `build/`; the model is borrowed from another tree and read in place. Nothing is
committed to `main`.
