# Programme: the meeting, the voice, and the plumbing

**Owner's instruction, 2026-10-07:** *"I want to fully implement the meeting system, the voice spike, and the
plumbing necessary for that whole system. Use insights and lessons learned from Hermes Vox."*

This document is the plan. `docs/staff-meetings.md` is the design and remains the owner's; this is the order the
work happens in, the seams between the pieces, and **the lessons Hermes Vox already paid for**, written as
requirements rather than as advice.

## The four pieces, and their dependency order

**The voice spike** — **DONE AND ACCEPTED, 2026-10-08.** Does the mechanism work at all? It does: four characters
proven, two artefacts found by the owner's ear, three knobs named, and both artefacts fixed and heard.
`docs/evidence/voice-review.md` is the whole arc. **The mechanism is not a risk any more** — what remains is
the plumbing that calls it.

**The power assignment** — **DONE, 2026-10-08** (`docs/power-assignment.md`, PR #59). The meeting *decides
allocations*, so the allocation model had to exist and be settable by a person before a meeting could set it.
That is why it went first. Its seam for the meeting is `SetAllocation`, `RecommendAllocation` and the
delegation.

**The plumbing** — the brief generator, the async worker, the cache, the cue track, the overlay, and the log
integration. It depends on the two above only at the edges.

**The audio plumbing (phase two) is built: `docs/evidence/audio-plumbing.md`, 2026-10-08.** The three
sources with one named owner each and one retirement rule per reply; the render queue, the cache key and
its prune; the cue set and its emit site; the warm in the async window; and the delivery direction
carried to `tools/voice/synthesize.py --exaggeration`. The async generator's novelty detection, the
overlay, and the player that plays the rendered audio remain.

**The meeting** — the scene, played back from a script generated ahead of time. It depends on all three.

## What Hermes Vox already paid for, as requirements

These are the lessons from the House's own voice client, converted from advice into things the meeting must do.

**1. One producer per audio track, and one party that retires a reply.** Vox's hardest bugs were never
synthesis quality — they were track and fence ownership. A one-shot TTS and a streaming TTS sharing a track
destroyed the reply's audio state; later the synth worker had to become *the only party that retires a reply*
because the text path was closing the track out from under it, truncating long replies whenever the playback
head caught the write pointer during a synthesis gap.

**A meeting has three audio sources — the pre-rendered lines, the pre-generated non-lexical cues, and a live
line on the novel-answer path — which is exactly the configuration that cost Vox a month.** So: **one player per
source, a named owner per track, and one retirement rule per reply**, written down before any of it is built.

**2. The meeting is a playback timeline, not a synthesis loop.** The design already says the rendered audio
carries the durations and pacing is data. Hold that line: **a cutscene does not fight itself.** Everything that
can be rendered ahead of time must be.

**3. Non-lexicals are clips, never synthesized.** Vox's ladder ended at *shipped non-verbal PCM clips on a
private track*, because a sentence-prosody TTS reading "Mm?" is its worst case. The breath, the chair, the PADD
tap, *"Hmm."* — **pre-generated clips on their own track**, and only real dialogue goes to the synthesizer.

**4. Warm during the async phase; never let the first meeting be the cold one.** Vox learned first-use latency
is not steady state and that warming is a fix as much as a measurement. **The async generation window is the
warm-up window** — load and warm the model there, so the player never waits on a cold start.

**5. Two latency regimes, two synthesis paths.** The meeting is batch, so quality wins. **The novel answer is
live, so latency wins** — and Vox already measured the tiers: Piper is the low-latency one, Kokoro is the richer
one. So the live path is the fast voice, and the pause-as-characterisation covers its latency.

**6. The four dead-layer checks, because the brief generator is their exact prey.** A layer can be built, wired,
unit-tested and never fire. So: **build the trigger inventory** (if a trigger only hangs off rare events, the
layer is silent by construction, and the fix is a trigger in the normal case — **every meeting generates a
brief, not just the dramatic ones**); **grep the emit sites, not the counters**; **check for two equal
constants making a branch unreachable**; and **a classification site is not a consumer** — grep the render
call, not the assignment.

**7. Judge it on the device, and believe the ear over the counter.** A green instrument measures the mechanism,
not the experience. **No meeting is signed off from a log line or a similarity score** — it is signed off by
listening.

**8. Rule out your own side first.** When a line sounds wrong, the order is **reference quality, then
segmentation, then separation, then the model** — never "the model is bad."

## The plumbing, named

- **The brief generator**, off the simulation: who is present, what is being decided, the enumerated options,
  what each costs, and the ship and people as they are now. **It reads; it does not write.**
- **The async worker**, outside the render loop, with the model absent being a valid outcome.
- **The authored skeleton**, which must exist for every meeting — the outcomes, their costs, and minimal
  dialogue. **The meeting works with the model absent.** That is not a fallback; it is the floor.
- **The cache**, beside the save, pruned with it, never in the repository.
- **The cue track and its owner.**
- **The record**: what was decided, by whom, and who was overruled — into the log, and into the officers' marks.
- **The seam the allocation lane is leaving**: what a meeting calls to set an allocation.

## The order, after these

**The meeting lane** once the spike and the allocation land. Then **the gap sweep**, in the order the register
already justifies — with two exceptions the owner has now made explicit:

- **The scenario content is not last.** It is the largest absence and it is *content*, but the meeting system is
  the frame it will be authored against: a scenario's drama is a meeting's argument. So the meeting's plumbing
  comes first and the scenarios are authored into it.
- **O2 — canon's three independent power sources — is promoted.** It is the mechanism for deck-by-deck survival,
  it is what makes the allocation screen matter when the grid fails, and it is small next to what it unlocks.

And the rest of the register's order stands as written, because the register gives its reasoning per item and
that reasoning has not been overturned.

## What is deliberately not decided here

- **Which synthesizer** — the spike decides it.
- **Whether the meeting runs on the player's machine only, or can use a server in multiplayer** — a ruling, not
  a preference, and it changes the plumbing's shape.
- **How many meetings a session holds** — a design question the first playthrough answers better than a
  document.
