# The audio plumbing — three sources, one owner each, and the playback that never waits

`docs/staff-meetings.md` (the owner's design), `docs/programme-meetings-and-voice.md` (the order and
the eight Hermes Vox lessons), `docs/evidence/voice-review.md` and `voice-fixes.md` (the mechanism,
built and accepted), and `docs/evidence/meeting-brief.md` (phase one), applied on
`feat/the-audio-plumbing` (cut from `testing`), 2026-10-08.

**Observed, and the command that produced it.** Everything below is the output of a command named
beside it, on this tree, after `cmake --build /home/c/big/git/upstream/efgame/build-linux --target
efgame` (which built `libefgame.so` with only the pre-existing `-Wwrite-strings` warnings). This phase
is **the audio model and the render queue**: no audio device is opened, no sound is played, no model
is loaded, and no live call is made. The overlay, the in-game positional playback and the live model
call are phase three and are **deliberately not built** — the live track has a name and an owner and
nothing else.

## The mistake this phase exists to prevent

A meeting has **three audio producers where a conversation had one**: the **pre-rendered dialogue
lines**, the **pre-generated non-lexical cues**, and a **live line** on the novel-answer path. That is
exactly the configuration that cost Hermes Vox a month: two producers on one track destroyed the
reply's audio state, and a path that closed a track it did not own truncated long replies.

The invariant is therefore written down and made testable: **one player per source, a named owner per
track, and one retirement rule per reply. Never two producers on one track. Never a path that closes a
track it does not own.**

## Task A — the audio model, and the track discipline

Three tracks, each with exactly one owner, as a function rather than a convention (`TrackOwner` in
`module/ship/ship_core.cpp`). A write or a retirement by any other producer is **refused**:

```
$ /tmp/.../test_ship_core --voice
== three sources, one owner each
  track dialogue  owner: the renderer
  track cue       owner: the cue player
  track live      owner: the live line
  the cue player writes to the dialogue track: refused
  the cue player retires the dialogue line:   refused
  a cue plays and retires: the dialogue line is still playing
```

| track | owner | what it plays |
|---|---|---|
| `TRACK_DIALOGUE` | `PROD_RENDERER` — "the renderer" | the pre-rendered lines |
| `TRACK_CUE` | `PROD_CUE_PLAYER` — "the cue player" | the non-lexical cues |
| `TRACK_LIVE` | `PROD_LIVE` — "the live line" | the novel answer (phase three; named, not built) |

**One retirement rule per reply.** `VoiceRetire` refuses any producer that does not own the reply's
track, and it is the only way a reply leaves a track. `VoiceWrite` refuses any producer that does not
own the track, and it refuses a track that already has its reply — **one player per source**.

**The cue track is separate from the dialogue track.** `EmitCue` writes only to `TRACK_CUE`; it cannot
touch a line, and a cue player cannot retire one. The test and the engine run both assert that a cue
playing and retiring leaves the dialogue line **still playing** (the line above).

Unit tests: `TestTrackOwnership`, `TestCueCannotStopALine` (`tests/ship/test_ship_core.cpp`).

## Task B — the render queue, and the warm-up

Rendering happens **off the critical path**, keyed and deduplicated, and the meeting can start with
the queue unfinished because the skeleton always exists.

```
$ scripts/audio-check.sh
    SHIP: voice test: 1 brief(s) queued at the watch change
    SHIP: voice test: async window: warmed=1, planned 6 line(s), queue unfinished=6, second plan=0
    SHIP: voice test: the meeting plays with the queue unfinished: 4 outcome(s) in "the watch handover, and what the incoming watch carries"
```

- **Where the warm-up happens: the async phase, when the worker opens.** `WarmVoice` is called as the
  async window opens on the queued brief (the harness; in play, the worker's first act). It writes to
  the log: *"voice: the model is warmed in the async window, before the meeting plays"*. This is
  lesson 4 — first-use latency is not steady state — held as a requirement, not advice.
- **The queue unfinished is not an error state.** The plan is a read of the brief and the skeleton;
  the meeting plays from the authored skeleton regardless. The harness deliberately reports both at
  once: the queue has 6 lines to render, and the watch-change skeleton has 4 legible outcomes.
- **`PlanMeetingAudio` deduplicates**: a second plan of the same meeting queues **0** more lines. The
  unmarked line (the sciences line in the watch-change skeleton) is marked, not rendered.

## Task C — the cue set, which is clips and never text

Small, named, and each with one purpose. A sentence-prosody TTS reading "Mm?" is its worst case
(lesson 3), so these are clips on the cue track, and the cue track's owner retires them.

```
  breath                          cue/breath.wav    the pause is a person, not a machine
  chair                           cue/chair.wav     someone changes posture: attention, or discomfort
  PADD tap                        cue/padd_tap.wav  the room thinking, a beat filled by a hand
  Hmm.                            cue/hmm.wav       acknowledgement while the answer is late
  I have to think about that.     cue/holding.wav   the short holding line the design names, before a deferral
```

**THE CUE EMIT SITE.** A cue is played in exactly one place, `EmitCue`, and it writes the emission to
the log so the emit site can be grepped — **never a counter**. The dead-layer check applies to cues as
it did to the brief, and the engine run reads the emission in the normal case:

```
    SHIP: voice test: cue emitted=2 in the normal case: breath (the pause is a person, not a machine)
```

`CUE_HOLDING` is the one lexical cue (the short spoken line the design names); the other four are
non-verbal. Whether each clip is assembled from the retail assets or spoken by the synthesizer is a
rendering detail of phase three; the mechanism names them, places them on their own track, and the
track's owner retires them.

Unit tests: `TestCueSet`, `TestCueEmitSite`.

## Task D — the delivery direction reaches the synthesizer

Phase one emits a delivery direction per line; this phase carries it, unchanged, all the way to the
model's own knob. The mapping is `DeliveryExaggeration` (the review's fix-pass values):

| delivery | `exaggeration` |
|---|---|
| `order` | **0.8** |
| `report` | 0.5 |
| `confession` | 0.35 |
| `condolence` | 0.3 |
| `flat` | 0.2 |
| `unmarked` | — (refused) |

The seam is a JSON-lines render manifest the module writes and `tools/voice/render.py` drains, one
synthesizer invocation per line. One line's value, end to end:

```
$ scripts/audio-check.sh        # the module's plan, then render.py --dry-run
    SHIP: voice test: delivery order -> exaggeration 0.80 for "You have the watch. Carry on."
$ head -3 build/g3-home/baseEF/ship/voice/render.jsonl
{"key":"3dca3e034c880776","voice":"Kathryn Janeway","text":"Do that.","delivery":"order","exaggeration":0.80}
$ python3 tools/voice/render.py --cache .../ship/voice --manifest .../render.jsonl --dry-run
render  : .../synthesize.py --reference .../refs/Kathryn Janeway.wav --text 'Do that.' --out .../3dca3e034c880776.wav --exaggeration 0.8
```

**`UNMARKED` refuses the line** — the brief could not know its temperature, so it is *marked* rather
than defaulted: `render.py` refuses it and reports it, and `PlanMeetingAudio` never queues it at all.
The player never notices, because the line is not rendered and is not played: the meeting simply does
not speak that line, exactly as the skeleton is authored.

## Task E — the cache, and the boundary

- **Beside the save, pruned with it, never in the repository.** The cache is `<home>/baseEF/ship/voice`
  — where the ship's own `carry.ship` lives, under the player's writable home, and **outside version
  control** (`build/` is gitignored; the check asserts the cache path is ignored).
- **Never commit generated audio, reference clips or extracted assets.** `scripts/audio-check.sh`
  fails if `git ls-files` matches any audio extension (`.wav`, `.mp3`, `.ogg`, `.flac`, `.aiff`,
  `.m4a`), and `tools/voice/render.py` refuses a cache directory that is repository data (tracked, or
  inside the tree and not gitignored).
- **Nothing calls a model when the player arrives.** Generation is ahead of time; the only live path
  is the novel answer, and that is phase three. This phase references no model.

**The cache key** is `RenderKey(voice, text, delivery)`: an FNV-1a 64-bit digest over **length-prefixed**
voice and text and the delivery byte, printed as 16 hex characters. Same text, same voice, same
delivery direction → same file. Length-prefixing is the point: `"ab","c"` and `"a","bc"` cannot forge
each other's boundary.

```
    SHIP: voice test: cache key 4cc54c41433bed98; fresh queue 1, queue again 0, cached requeue 0 (0 = a no-op)
    SHIP: voice test: pruned with the save: 1 entr(ies) dropped, cache now 0
```

A second `QueueRender` of the same key is a **no-op** (0), and a line already cached is a no-op (0).
Across sessions the same no-op is enforced by `render.py` (`os.path.exists(out)` → skipped), so an
empty in-memory index after a load does not cause a re-render. **Prune:** `PruneVoiceCache` empties
the index, and `ship voice prune` also removes the files and empties the manifest.

## The save

**Nothing new is saved, so there is no version bump and the cost is zero.** The mixer and the render
queue are host-local, derived from the briefs the save already carries (phase one's queue), and the
cache is player-local data on disk. `TestAudioSaveRoundTrip` and `TestPlanMeetingAudio` (which packs
the ship before and after planning) prove the ship's blob is unchanged.

## Acceptance

| item | result | where |
|---|---|---|
| three sources, one owner each, tested | ✅ | `TestTrackOwnership`, `--voice`, `audio-check.sh` |
| a non-owner cannot write to or retire another source's track | ✅ | `VoiceWrite`/`VoiceRetire` refuse; engine prints `refused=1` both |
| the cue track cannot stop a dialogue line | ✅ | `TestCueCannotStopALine`; engine prints `the cue did not stop the line=1` |
| the meeting plays with the queue unfinished; the skeleton carries it | ✅ | `TestPlanMeetingAudio`; engine prints `queue unfinished=6` beside 4 outcomes |
| a rendered line is cached, key stated, second render a no-op | ✅ | `RenderKey`; `TestRenderKeyAndCache`; engine prints `fresh queue 1, queue again 0, cached requeue 0` |
| the cache is pruned with the save; the repository stays clean of audio | ✅ | `PruneVoiceCache`; `audio-check.sh` clean-audio + gitignore check |
| the warm-up happens in the async phase, and it is said where | ✅ | `WarmVoice` at the async window; log line read by `TestPlanMeetingAudio` |
| the cue emit site fires in the normal case (emit site, not a count) | ✅ | `EmitCue`; engine reads `cue emitted=... in the normal case: breath` |
| the delivery direction reaches `--exaggeration`; one line end to end | ✅ | `DeliveryExaggeration` → manifest → `render.py` → `synthesize.py --exaggeration 0.8` |
| saves round-trip anything new; bump if you must and say the cost | ✅ | nothing new saved; **no bump**, cost zero; `TestAudioSaveRoundTrip` |
| `test.sh`, `check.sh`, `meeting-check.sh` and every existing check exit 0 | ✅ | all run below |

```
$ scripts/test.sh
  -> crew_core: all checks passed; ship_core: all checks passed; tools: 54 tests OK;
     python: ok; shellcheck: ok; 19 patches; "all checks passed"
$ scripts/check.sh --source-map build/gdk/maps/eliteforce_voyager_maps/voy1.map --script-corpus build/gdk/scripts
  -> entity dictionary 318 classes; validator negative tests ALL PASS; 2,024 files: 2,016 compiled
     and read back, 8 rejected -- exactly the known, documented set
$ bash scripts/meeting-check.sh --map tour/deck04
  -> PASS  a brief is emitted at the watch change; a person set an allocation end to end; the ship's own set was refused
$ scripts/audio-check.sh --map tour/deck04
  -> PASS  (the audio plumbing, above)
```

## What could not be verified

- **Whether a meeting is any good to sit in** — whether the cues land as *a room thinking* rather than
  as a soundboard. That is the owner's, at the walkthrough, and no instrument here can answer it.
- **The clips themselves.** No cue clip has been assembled or heard; this phase names the set, places
  it on its own track, and proves the emit site fires. Producing and judging the clips is phase three.
- **The physical file deletion on `ship voice prune`** if the engine exposes no home path: the index
  is always cleared, the directory removal is best-effort through `fs_homepath`. The repository-clean
  check is the load-bearing half, and it is not best-effort.
- **The cast map from a crew display name to a reference.** The render job carries the resolved
  speaker's name (`"Kathryn Janeway"`, `"Crewman 078"`); turning that into `refs/<character>.wav` is
  casting, and casting is phase three. The manifest is correct as a plan; the references it names do
  not exist yet.

## Judgement calls, named as calls

1. **The audio model lives in `ship_core.{h,cpp}`**, not a new file: the module build discovers our
   sources with a configure-time glob, and a new `.cpp` would not be compiled without re-running cmake
   (`AGENTS.md`, the trap). Same file as the power assignment and the meeting brief.
2. **The cache lives at `<home>/baseEF/ship/voice`**, beside the ship's own `carry.ship` and under the
   gitignored build home. It is "beside the save" in the sense of player-local, next to the ship's
   files; it is not inside the engine's `saves/` directory, which the module does not control.
3. **The render queue is not saved.** It is derived from the briefs the save already carries, so a load
   replans for free; the cache on disk is what makes the replan a no-op. This is why the version is
   **not** bumped, and the cost of that decision is named: a save taken mid-render loses only work that
   is cheap to redo.
4. **The three tracks are `dialogue`, `cue`, `live`**, and the live track is named with an owner and
   left empty. Phase three fills it; nothing here pre-empts the live path's design.
5. **`CUE_HOLDING` is the one lexical cue.** The design lists it with the non-verbals; we treat it as
   the short spoken holding line (so it is the synthesizer's work) and keep the four non-verbal clips
   strictly off the prose TTS, which is the lesson.
6. **The manifest is JSON lines, one object per line**, because it is cheap to append, trivial to drain,
   and readable. The Python side validates the delivery table against its own copy, so a drift between
   the C++ values and the renderer is caught rather than silently rendered.

## The boundary, kept

No generated audio, no reference clips and no extracted game assets are committed — not to this file,
not to the branch, not to an issue, not to a pull request. The retail install was read only. Nothing was
published, shared or uploaded. Nothing is committed to `main`. The engine and its module were rebuilt;
no engine process or agent session was left running.
