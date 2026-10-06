# Staff meetings: scheduled, pre-written, and reacting only to the novel

Owner's design, 2026-10-06 (second revision; the first assumed generation on room entry).

A staff meeting is the most-used scene in Star Trek: the room, the table, the argument, the decision, and then
the thing happening. The engine already has every part of that scene except one — the room, the roster, posts,
the arbiter, the log and the clock all exist. What is missing is the dialogue. This document is how a local LLM
supplies it without ever becoming a dependency of the running game.

## The three phases

**1. Async generation, off the critical path.** Long before the meeting, the simulation enqueues a *meeting
brief* — who will be present, what is being decided, the enumerated options, what each one costs, what the ship
and the people look like right now. A background worker (not the render loop) hands that brief to a local model
and gets back a **meeting script**: a branch tree of in-character dialogue that terminates in the enumerated
outcomes. This can take thirty seconds or three minutes. Nobody is waiting, so the cost does not matter.

**2. The room, in real time.** The player walks in. The script drives. Barks, lines, the options as they are
offered. **No model is called and no latency exists**, because every branch was written before the meeting was
scheduled. The meeting is a content artifact being played back.

**3. The novel answer.** If the player supplies something the script does not cover, *then* the model is called,
live, with the brief, the script so far, and what the player just did — and it produces a response in character
and picks from the same enumerated outcome set. This is the rare path, and the only path with latency in it.

## Why this ordering is right (the strong arguments, not the obvious one)

**It makes the model auditable.** A pre-written script is a file. It can be reviewed before it is ever seen,
diffed when the persona brief changes, and checked for contradictions with the record. In-room generation cannot
be validated by anybody at any point — it exists for one moment and is gone.

**It makes saves portable and replayable.** A meeting's dialogue is either in the pre-written script or, if it
came from a novel answer, **written into the log as text at the moment it happens**. So replay reads the log,
not the model. Reloading a save never regenerates a conversation differently, and a save remains playable on a
machine with no model at all. Without async pre-writing, every reload would be a new roll.

**It allows a pre-flight validator, which is the single best argument.** Off the critical path there is time to
check the script before it is used: *does every branch terminate in one of the enumerated outcomes? Is any
branch a dead end? Does a character say something the record contradicts? Does Tuvok raise his voice?* A cheap
validator can reject and regenerate. This is impossible in real time.

**It gives the ship's clock a job.** Meetings fall out of the simulation rather than out of level design: a
scheduled briefing at the watch change, or a meeting that *becomes due* because a threshold was crossed — the
dilithium reserve, casualties exceeding beds, Borg pressure rising, a decision the player has been deferring.
The meeting is a consequence of the state, which is the rule the whole design rests on: **the simulation
decides, the model speaks.**

**It is bounded per meeting, not per line.** One expensive call produces a whole tree. The resource profile is a
burst of work at a quiet moment and then nothing — and "quiet" is schedulable: generate while docked, while
paused, while saving, between watches, never during combat, one meeting queued at a time.

## The model is never a dependency

**The meeting works with the model absent.** If generation has not finished, failed, or the machine cannot run
it, the meeting plays from an authored skeleton: the enumerated outcomes, their costs, and minimal authored
dialogue for each. Generated flesh is an enhancement over a structure that always exists. Nothing in the
critical path may wait on inference — the same rule as never stopping a watchdog without a replacement standing
by.

## The hard parts, named honestly

**Novelty detection is the actual engineering.** Deciding *is this input covered by a pre-written branch* is not
a model task; it is classification. Recommendation: each branch carries an **intent descriptor**, and the
player's input is matched to the nearest by embedding similarity against a small always-resident local
embedding model (a hundred megabytes, not a generator). Below threshold, it is novel. Above it, the branch
plays and the model is never touched. Cheap, local, and tunable by watching what gets misclassified.

**The script can go stale between generation and the meeting.** It was written around the ship as it was.
Anchor the brief's facts, and treat a material change — a fire, a death, the reserve running out — as a trigger
either to regenerate or to fire a small set of authored **interruption hooks** ("the state changed" variants of
a line). Same spirit as the rule already in force for the ship: nothing is written by an event that does not
last long enough to write it.

**The novel answer should be promoted, not discarded.** After the meeting, the generated exchange can be folded
back into the script as a new branch. The script is then a **cache that grows with play**: the same scenario in
a later run replays the player's own invention for free, and the model's output has become reviewable content
sitting in a file. This is how the system gets *cheaper* over a campaign instead of more expensive.

**The model cannot alter state.** It produces text and selects from the enumerated outcome set. Nothing else.
If it can write to the simulation, it becomes an unauditable input to a deterministic model, saves stop
reproducing, tests stop meaning anything, and a hallucination becomes a mechanic.

**What the player's "novel input" actually is.** The game is a shooter: there is no text box by default. Three
candidate input paths, cheapest first — (a) **the station console**, which we have already built, taking typed
input in a meeting; (b) **action as input**: the player leaves, stays silent, pulls a weapon, opens the door —
anything outside the enumerated options is itself a novel answer, and this needs no new UI at all; (c) **voice**,
later, on the strength of the Vox work. Recommendation: consoles and actions first, voice when the rest holds.

## The artifact

A meeting script is data, one file per meeting, small: identity and time, who is present, the decision, the
enumerated options with their costs, a branch tree of dialogue keyed by speaker and register with intent
descriptors, interruption hooks, and the authored skeleton as fallback. The log records the playthrough — what
was said as well as what was decided — so the meeting can be read afterwards.

## Acceptance

- A meeting is scheduled by the clock or by a threshold, and its script is generated **before** the player
  arrives, with the room in real time and no inference call on arrival.
- With the model disabled entirely, the meeting still runs from the skeleton and resolves correctly.
- A player input matching no branch triggers exactly one live call, and its response is written into the log and
  replayed from the log on reload rather than regenerated.
- Every branch resolves to one of the enumerated outcomes, and the validator rejects a script that does not.
- Same scenario, second run: the previously novel exchange is offered from the script without a model call.
- Nothing the model produces can change ship state.
