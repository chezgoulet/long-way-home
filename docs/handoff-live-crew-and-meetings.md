# Handoff: the live crew, the meeting room, and the voice

Three design documents were approved on 2026-10-06 and merged to `testing`:

- `docs/affinities-and-allegiance.md` — taste and allegiance as the priors that make identical events land
  differently in different people.
- `docs/staff-meetings.md` — the scheduled meeting, the pre-written branch tree, the local model, the LCARS
  overlay, pre-generated audio, the filled pause, voice in, and the ship's computer.
- `docs/art/lcars-meeting-overlay.png` — the overlay as storyboarded, with its HTML source beside it.

This brief turns them into work. **The designs are the specification; this document is the order of operations,
the acceptance tests, and the standing rules.** Where they disagree, they win.

## What already exists, and must be built on rather than around

**Corrected 2026-10-06: five claims in this section were wrong when the brief was written.** Every fact below
was re-checked against the committed tree and the corrections are marked. The corrected facts are the ones to
build from.

**And read this part first: `module/`, `tests/`, `scenarios/` and `.github/` do not exist on `testing` at
all.** Measured: the builder branch carries 17 files under `module/`, `testing` carries none. The module code,
the test suite and the scenario manifests are all `feature/g3-reactive-crew` only, so this brief is a *target*,
not a description of the branch it is sitting on — a builder working from `testing` alone has nothing to
extend.

Verified in the tree, not assumed:

- **The save format is versioned with a changelog.** `module/ship/ship_core.h`: `SAVE_VERSION = 12`, whose comment
  names what each increment added ("2: parts, exposure; 3: control, intruders; 4: the Borg; 5: the outside;
  6: modes, the player; 7: orders; 8: morale; 9: severity, supplies, triage; 10: force fields; 11: the log;
  12: the away kit"). Follow that style: bump, and say what the new number carries.
  *(Corrected: was cited as `= 40`, with a changelog running to "40: each system's named failure state".)*
- **Both halves persist.** `module/ship/g_ship.cpp:803` and `module/crew/g_crew.cpp:1288` write their blobs
  through `gi.AppendToSaveGame( SAVE_CHUNK, blob.data(), ... )`; the reader rejects a mismatched magic or
  version at `module/ship/ship_core.cpp:1210` — `if (r.U32() != SAVE_MAGIC || r.U16() != SAVE_VERSION ...)`.
  The chunk ids are `'SHIP'` and `'CREW'`.
  *(Corrected: was cited as `g_ship.cpp:1662`, `g_crew.cpp:1455` and `ship_core.cpp:3146`. `g_ship.cpp` is
  1,021 lines and `g_crew.cpp` is 1,327, so the first two cannot exist; `ship_core.cpp` is 1,356, so neither
  can the third.)*
- **The clock already has watches.** `CrewMember::watch` (0 alpha 0800–1600, 1 beta 1600–2400, 2 gamma
  0000–0800) and `ScheduledActivity(watch, secondOfDay)` — a meeting scheduled at a watch change needs no new
  calendar.
- **The record already has memory and marks — verified against the working tree this time.** `struct Memory`
  (`module/ship/ship_core.h:213`) is `event · person · source · time · valence · salience`, held on
  `CrewMember` as a bounded `std::vector<Memory>` with `MEMORY_MAX = 8`; `enum MemorySource` carries exactly the
  four provenances the design asks for (saw it myself, was told, rumour, read it in the log); `enum MemoryEvent`
  declares six events (`MEM_DEATH`, `MEM_ORDER`, `MEM_PROMISE`, `MEM_LIE`, `MEM_RESCUE`, `MEM_VIOLATION`);
  `Remember()` writes, `MemoryDecay()` fades salience, and `Bond(a, b)` folds a relationship out of the marks
  and is readable from the console as `ship bond <a> <b>`. `CrewMember` also carries `faction` (0 Starfleet,
  1 Maquis) and the ship carries `resentment`. It has been there since save version 18.
  **What is thin is the wiring, not the structure:** only five `Remember()` calls exist in the whole module, and
  of the six declared events just four are ever written — **`MEM_ORDER`, `MEM_PROMISE`, `MEM_LIE` and
  `MEM_RESCUE` are declared and never produced.** So M1 *extends* the record rather than creating it, and the
  first work is to give the promise and the lie their writers.
  *(Two earlier versions of this bullet were wrong in opposite directions. The original asserted that the record
  already had memory and marks, naming a `struct Memory` and bonds "already folded from marks". A correction then
  asserted there was no `struct Memory`, no valence, no salience, no bond and no allegiance anywhere in
  `module/`. That correction was measured by grepping `origin/feature/g3-reactive-crew` at `26b8fa4`, where this
  work was still uncommitted in the working tree, and it generalised "absent from that commit" into "absent from
  the project". **A grep against a ref proves what that ref holds, never what the project has.**)*
- **The arbiter already decides.** `crew_core.h`: `Arbitrate(Signals)` over `LEVEL_SCRIPT 1 > LEVEL_COMBAT 2 >
  LEVEL_DIRECTOR 3 > LEVEL_DUTY 4 > LEVEL_IDLE 5`. **Its order is the charter's rule; deviations are bugs.**
- **Sound is played from the module by path.** `G_Sound(&g_entities[0], G_SoundIndex("sound/ambience/voyager/
  redalert.mp3"))` (`g_ship.cpp:1635`, `:2378`) — this is how the game's own canonical cues are used.
  *(Corrected: no `G_Sound` call appears anywhere in `module/`. The mechanism may still be right — the API is
  the engine's — but the citation was not: treat this as unverified and confirm the pattern against the code
  before relying on it.)*
- **There is a UI and a console path.** `module/ui`, `module/cgame`, and the station-panel work: `scripts/
  s9-check.sh`, `scripts/s4-panel-check.sh`, `scripts/viewscreen-check.sh`. **Extend the existing panel path;
  do not write a new renderer** unless that path proves genuinely unable to draw the overlay.

## Gates

Do not begin a gate until the one below it works. Each gate ends with: tests in `tests/`, a check script that
drives **the real engine** (the pattern of the existing `scripts/*-check.sh`), and an evidence document under
`docs/evidence/` whose PASS lines state what was actually observed.

**M1 — Taste and allegiance in the record.** Add to `CrewMember`: a small number of named likes and dislikes with
strengths, pursuits, and an allegiance with a strength. Generated for the 141 from the existing roster seed;
authored from canon for the named few — `docs/affinities-and-allegiance.md` lists the six verified items and the
**two explicitly unverified** (Seven's music studies, Chakotay's boxing): check those before authoring them, and
do not author them from this brief. Wire taste into mark valence at the writer, and into choice at the schedule
and the job queue. Bump `SAVE_VERSION` and extend the changelog comment. *Acceptance:* the same event gives two
crew opposite valence, traceable to a declared affinity; a Maquis-weighted crew member's morale drops on a
Starfleet-protocol decision and the mark records why; a liked pursuit appears in that member's scheduled downtime;
a volunteer for a loved task appears in the job queue.

**M2 — Meetings as data, played from a script.** A meeting is scheduled by the clock (a watch change) or by a
threshold crossing, and enqueues a **brief**. The **script** is data: identity and time, who is present, the
decision, the enumerated options with their costs, a branch tree of dialogue keyed by speaker with **intent
descriptors**, interruption hooks, and an authored skeleton as the fallback. A pill selected in the room resolves
to one enumerated outcome, which **the simulation applies** — the script never applies it. The log records what
was said as well as what was decided. *Acceptance:* with the model disabled entirely the meeting still plays from
the skeleton and resolves; a chosen branch resolves to exactly one outcome and is logged; nothing in the script
can alter ship state.

**M3 — The overlay.** Draw the dialogue overlay on the existing panel path to match
`docs/art/lcars-meeting-overlay.png`: a speaker rail (name, post, watch, mood), the line being answered, four
option pills each with a short description, and a free-text pill carrying a `VOICE` affordance drawn from the
start even though voice does not exist yet. Roughly the lower quarter of the frame; the scene stays visible.
*Acceptance:* a pill routes to its branch; typed text routes to novelty matching and never to a branch by
accident; arriving in the room calls no model; the layout matches the storyboard.

**M4 — The async generator.** A local model, called off the render loop during a quiet moment (docked, paused,
saving, never in combat), turns a brief into a script. A **pre-flight validator** runs before the script is
usable: every branch must terminate in one of the enumerated outcomes, no branch may be a dead end, and no line
may contradict the record. Failure means regenerate, and the meeting still plays from the skeleton meanwhile.
A material change between generation and the meeting (a fire, a death, the reserve running out) either
regenerates or fires an authored interruption hook. Novelty detection is classification, not generation: branch
intent descriptors are matched by a small always-resident embedding model with a threshold; below it, exactly
**one** live call happens, its output is written to the log, and the exchange is **promoted back into the
script** so a later run replays it for free. *Acceptance:* the script exists before the player arrives; the
validator rejects a deliberately broken script; one novel input produces exactly one call; reloading replays from
the log rather than regenerating; a second run offers the promoted branch with no call.

**M5 — Voice out.** Cast a voice per character and hold it for the campaign. Render every scripted line to audio
during generation, using the local TTS pipeline; use the game's own cues for the computer. Give each character a
small set of **pause cues** (a breath, a chair shift, a PADD tap, a deferral line) and play one immediately when
a novel answer is submitted; if the model is slow, the deferral line lands and the outcome moves to a later beat
— a message, a corridor conversation, the log. Cache beside the save and prune with it. *Acceptance:* a meeting
plays with **zero inference** and true durations that pace the pills; a novel input plays a cue immediately and
resolves either inside the pause or as a deferral; the cache prunes with the save.

**M6 — Voice in, and the computer.** Speech-to-text feeds the same pill; because the pill emits text, nothing
else changes. Then the ship's computer reuses the same overlay: pills, free text, a pre-generated voice, the same
branch model — with the enumerated set as **the ship's API**, and an unrecognised command refused in character
("that function is not available"), never invented. *Acceptance:* a spoken novel reply takes the identical path;
a computer command outside the enumerated set is refused rather than fabricated; no second UI system was built.

## Standing rules — these are not negotiable, and a failure here is a bug

1. **The simulation decides; the model speaks.** The model produces text and selects from enumerated outcomes.
   It may never write ship state.
2. **The model is never a dependency.** Every meeting plays from the authored skeleton with the model absent,
   busy, or broken.
3. **Determinism survives.** A save replays identically: dialogue is in the script or in the log, never
   regenerated.
4. **No assets and no generated audio in the repository, ever.** No game assets, no cloned voice, no rendered
   dialogue — not in a commit, not in an issue, not in a pull request. Voice the repository *ships* is voice we
   created. The licence reasoning is in `docs/staff-meetings.md`; read it before touching anything audio.
5. **The arbitration order does not change.**
6. **The look is the storyboard's.** LCARS, minimal, lower quarter, scene visible.

## Non-goals for this work

An LLM writing ship state; a general dialogue system replacing authored content; online or hosted inference; a
new renderer where the panel path serves; any synthesised lore-character audio crossing the repo boundary.

## Order

M1 → M2 → M3 → M4 → M5 → M6. M1 precedes M4 (the model needs the persona record). M2 precedes M4 (the model
needs a scene format to fill). M3 is independent of M4 and may proceed alongside it once M2 defines the scene.
