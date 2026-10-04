# Program Charter v3 — Elite Force: Native Linux Client, Authoring Pipeline, Living NPCs

Date: 2026-10-04. Supersedes `program-proposal-v2.md`.
Status: **approved with conditions**; this revision implements them.
Prepared for: Christopher Goulet.

Time is explicitly de-emphasised. Sequencing below is by **gate and dependency**, not calendar.
Effort figures are relative magnitude, for ordering only. The one place wall-clock still matters is
human playtest acceptance, which is the only test that can judge inhabitation.

---

## 1. Decisions now fixed

| # | Decision | Answer |
|---|---|---|
| 1 | Program shape | Three tracks + capstone, **gated** |
| 2 | Virtual Voyager bar | Adopted, plus **no regression in the base campaign** |
| 3 | Capstone setting | **Voyager** — inhabitation, not asset volume |
| 4 | Track C fidelity | **Reactive crew first, named as such**, then living ship |
| 5 | Upstream strategy | Track A as a branch **offered** upstream; upstream acceptance is **not** a dependency |
| 6 | Editor (new, see §4) | **NetRadiant-custom + q3map2, native Linux, with an Elite Force game pack** |

---

## 2. Something is already true that was not at v2

I resolved the hardest open question in v2 — the map-authoring gap — and in doing so proved a
prerequisite by doing it rather than assuming it.

**The official Game Development Kit opens on Linux.** Its Windows installer (an MSI, inside a zip from
the Internet Archive) extracts cleanly under `msitools` — a small dependency ladder, no Windows, no
Wine, no VM. The payload is the whole official toolchain and its data, and it gives us, in hand:

- **`SP_entities.def`** — 5,730 lines, **214 QUAKED entity definitions**. This is the authoritative
  singleplayer entity dictionary: the exact contract any editor, validator or generator must speak.
- **`real_scripts.zip`** — **2,477 files, of which 2,175 are plain-text ICARUS mission scripts**,
  organised per map. This is the complete scripting corpus of the shipped game: simultaneously our
  best authoring reference, our template library, and our compiler's test suite.
- **`ICARUS Manual.doc`** and the **EF GDK FAQ** — both readable on Linux as text without Word. The
  manual documents the command set directly: `flush, if, loop, affect, run, wait, action, sound, move,
  rotate, use, kill, remove, print, rem, declare, free, get, random, set, camera, task, do`.
- The compilers (`q3map.exe`, `q3data.exe`, `bspc.exe`), the editors (`efRadiant.exe`, `BehavEd.exe`,
  `ShaderEd.exe`), `IBIze.exe`, `MD3View.exe`, the ROFF plugins, model skin references, and
  `SourceForBehavEd/` headers.
- `BaseEf/maps/` sample maps, and the two entity files (SP and Holomatch).

Also confirmed from the shipped Voyager map sources: **89 distinct entity classes** are actually used
in the campaign maps, including **689 `waypoint` entities** and their `waypoint_navgoal*` family,
`waypoint_squadpath`, `waypoint_small`, `point_combat`, `path_corner`, and — importantly for Track C —
**named crew**: `NPC_Janeway`, `NPC_Chakotay`, `NPC_Tuvok`, `NPC_Paris`, `NPC_Kim`, `NPC_Torres`,
`NPC_Doctor`, `NPC_Seven`, `NPC_Neelix`, `NPC_Vorik`, `NPC_Munro`, `NPC_Biessman`, plus
`NPC_starfleet` and `NPC_starfleet_random`.

**That last fact is the single most important one for the capstone.** Inhabitation does not require
new characters. The named crew are already placed in the maps. What is missing is that they *do*
something. Track C is what makes them do something.

---

## 3. Gates

Gates are hard. Nothing downstream starts before its gate passes, and each gate's evidence is an
artifact that either exists or does not.

| Gate | Contents | Exit evidence |
|---|---|---|
| **G0** | M0 (module builds) + native script compiler + asset validator | two libraries with four intended exports and no unexpected unresolved symbols; native compiler reproduces the shipped scripts byte-for-byte; validator catches every seeded fault |
| **G1** | M3 — client playable | first base-campaign mission completed from clean start; save written and reloaded |
| **G2** | Virtual Voyager acceptance + no base-campaign regression | the §5 bar passes; base campaign re-verified unchanged |
| **G3** | **Reactive crew** (§6) | the §6 measurable criteria pass on one deck |
| **G4** | Living ship | the §6 living criteria pass; **does not start until G3 works** |
| **G5** | Capstone | the §7 scenario is playable and inhabitation is confirmed by you, not by a log |

G0's three items are mutually independent and can be worked in parallel by separate agents. G3 and G4
are strictly sequential.

---

## 4. Editor strategy (the v2 gap, now closed)

The objection was correct: a compiler pipeline for existing maps is not an authoring path. Here is the
commitment, with the fallback ladder made explicit so we never stall.

**Primary: NetRadiant-custom (`Garux/netradiant-custom`) + `q3map2`, running natively on Linux.**

Why this one: it is the maintained descendant of the exact lineage efRadiant was hacked from
(efRadiant is a hack of the old Q3Radiant 147 line; NetRadiant is Q3Radiant → GtkRadiant 1.4 → 1.5 →
NetRadiant → this fork), it focuses on Quake and Quake 3 as its primary targets, it ships ready-to-use
packages and distro builds, and it carries `q3map2` in the same repository — editor and compiler from
one maintained source. Same map semantics as efRadiant, native on the target platform, and someone
else is maintaining it.

**The Elite Force game pack we author and own.** NetRadiant is game-agnostic; a game pack supplies the
entity dictionary, shader list and textures. That pack is built from artifacts we now hold:
`SP_entities.def` (214 entities) is the dictionary; the GDK supplies `shaderlist.txt` and the shaders;
the shipped map sources supply the 89 classes actually in use, as a cross-check that the dictionary
covers reality.

**Validation of the pack is a gate item, not an assumption:** open a shipped map source in NetRadiant,
rebuild it with `q3map2`, load the result in the client, and compare against the original. If a
rebuilt shipped map behaves identically, the pack is trustworthy. This is also the first honest test
of whether "same lineage" is enough.

**Fallback ladder, in order of preference:**
1. **NetRadiant-custom** — primary, as above.
2. **TrenchBroom** — modern, cross-platform, better ergonomics, and there is an active idTech3-oriented
   fork. Worth a timeboxed spike, but the general advice for Quake 3-family mapping is that
   TrenchBroom is not the strongest tool for this engine, so it is a *spike*, not a commitment.
3. **efRadiant under Wine** — for parity checks and for seeing exactly what the original mappers saw,
   not as the working editor.
4. **Programmatic `.map` generation** — `.map` files are plain text. Any pipeline-generated geometry
   (standard corridors, decks, module layouts) should be emitted directly, editor-independent. This is
   the path that scales with agents rather than with hand-editing, and it means the editor is required
   only for genuinely bespoke spaces.
5. **The floor:** if all editors fail, new locations are still possible via hand-authored `.map` text
   plus `q3map2`. Ugly, slow, real. That is why this is a ladder and not a single point of failure.

**Nav is part of authoring, not an afterthought.** The SP navigator works from `waypoint` and
`waypoint_navgoal*` entities, and NPCs can only live where those exist and are mutually reachable.
So the validator (§5) checks nav coverage per space, and "authored space" means "geometry plus
navigation plus entity dictionary", or it does not count.

---

## 5. Track B — tooling, with the artifacts now in hand

1. **Native script compiler** (replacing `IBIze.exe`), built from `Interpreter.cpp` + `Tokenizer.cpp`,
   which are already in the game source precisely because they are the offline compiler.
   *Acceptance: it compiles all 2,175 shipped scripts, and the output is byte-identical to the
   originals where the originals exist.* This is a real, large, cheap test — the corpus is free.
2. **Headless map + navigation build**: `.map` → `q3map2` → `bspc` → packaged `.pk3`, scriptable.
   *Acceptance: rebuild a shipped map source and load it.*
3. **Asset validator**: one command that checks a scenario's maps, models, textures, sounds, scripts,
   nav coverage and data files all resolve, and names what is missing.
   *Acceptance: negative tests — every seeded, deliberate fault is caught and correctly named.*
4. **Entity dictionary tooling**: convert `SP_entities.def` into machine-readable form, cross-check it
   against the 89 classes used in shipped maps, and emit the editor pack from it.
   *Acceptance: no class used in the shipped campaign is missing from the dictionary; unknown classes
   in a scenario are reported.*
5. **Character and ship assembly**: model source → `.tik` → `NPCs.cfg` entry → sound set → spawnable
   entity, documented and scripted.
   *Acceptance: one new character and one new ship appear in a test level.*
6. **Scenario scaffold**: a repository layout plus build file so a scenario is a versioned artifact,
   not a folder of loose files. *Acceptance: the capstone is built this way from day one.*

---

## 6. Track C — reactive crew first, living ship second

### G3 — Reactive crew (the first deliverable, and the only Track C promise until it works)

**Definition:** 5–10 NPCs, on **one deck**, with **no new animations** and **reused existing barks**,
performing **simple posts and acknowledgement**.

Concretely: named crew entities already placed in a Voyager deck are given posts and a loose routine;
they move between a small number of points using existing navigation; they turn to face the player;
they acknowledge being addressed or collided with using dialogue lines that already exist in the game;
they do not bump into each other or wander off the deck; they persist correctly across save and load.
Nothing here requires new art, new voice, or new animation. It requires the direction layer to exist
and the existing machinery to be driven properly.

**Measurable acceptance criteria (these are the gate, not a vibe):**
- 5–10 named crew present, on one named deck, all reachable and holding their assigned posts.
- Shift/post coverage: at least 90% of assigned posts occupied at any sampled moment over a
  10-minute observation run.
- Each NPC reaches its post within a bounded time from level start, or reports failure.
- Zero navigation failures over the run (no NPC permanently stuck, no falls out of world).
- Player address → acknowledgement within a bounded time, using existing voice lines.
- No ICARUS script conflict: every scripted sequence in the deck still fires and completes.
- Save/load restores post, current goal and schedule cursor for every NPC.
- Save-size increase attributable to the crew stays within an explicit per-NPC budget (tune at
  implementation; the point is that the budget exists and is tested).
- Frame time budget holds with the crew active on the deck.

### G4 — Living ship (starts only after G3 passes)

Adds the ship clock, shift schedules, duties as authored data, new life-shaped behaviour states,
NPC-to-NPC conversation, and a director that keeps the ship coherent. All of G3's criteria still apply,
at scale, on more decks, with 20–30 NPCs at 60 fps on the target desktop.

### Arbitration — written before any Track C code

Precedence, highest first. This is the rule; deviations are bugs:

1. **Scripted sequence (ICARUS)** — always wins. An authored moment is intent.
2. **Direct combat or reaction** — being shot at, or a hostile in perception, overrides routine.
3. **Director override** — the ship-level scheduler may pull an NPC off routine for coherence
   (restaffing a post, clearing a corridor).
4. **Duty / routine** — the schedule. What the NPC should be doing.
5. **Idle / social** — what the NPC does when nothing above applies.

Two corollaries worth stating: an NPC interrupted above level 4 returns to its routine when the
interrupt clears, and nothing in levels 3–5 may ever interrupt a level-1 script mid-sequence.

---

## 7. Capstone — Voyager, inhabitation first

One deck or a small set of connected Voyager spaces, using **existing** decks, **existing** characters,
**existing** voice, and **existing** navigation wherever possible. Authored objectives; reactive crew
inhabiting the space; playable on the native client.

The rule from the objection, adopted as policy: **the capstone proves inhabitation with existing
assets first.** New art, new animation and new voice are earned by demonstrating that inhabitation
works without them — and then used to deepen it, not to enable it.

Success is your judgement: you walk into the space and it is inhabited. Everything in §6 exists to make
that judgement supported rather than magical.

---

## 8. Interface contracts between tracks

Agents run in parallel; contracts are what keep parallel work from diverging.

- **Engine ↔ game module:** the four exports (`GetGameAPI`, `vmMain`, `dllEntry`, `GetUIAPI`) and the
  function-pointer tables. This boundary is fixed by the existing architecture; neither side may
  quietly extend it. Any new engine capability the autonomy layer needs is added as an explicit
  syscall, documented, versioned.
- **Track A ↔ Track B:** the mod/pak load paths and the folder layout a scenario must present. The
  validator encodes this contract and is the arbiter.
- **Track B ↔ Track C:** the scenario manifest — where schedules, posts, NPC parameters, nav coverage
  and script bindings live — is one documented data format. Track C reads it; Track B writes it;
  neither invents fields unilaterally.
- **Entity dictionary:** `SP_entities.def` (as converted) is the single source of truth for classnames
  and keyvalues. Editors, generators and validators all consume the same converted artifact.
- **Gate artifacts:** every workstream's output must be checkable by someone who did not write it.

---

## 9. Test plan — automated wherever cheap

Cheap and therefore mandatory:
- **Script compiler:** compile all 2,175 shipped scripts; compare output to originals. A corpus test
  with a large sample size for free.
- **Validator:** negative tests, one per fault class (missing texture, missing model, broken script
  reference, absent nav coverage, unknown entity class).
- **Module build (G0):** exported-symbol check and unresolved-symbol check, scripted, on every build.
- **Editor pack:** rebuild a shipped map and compare in-game behaviour to the original.
- **Save/load smoke tests in occupied spaces:** after G3, every build runs an automated save, load and
  state assertion.
- **Headless smoke test:** the client can be started under a virtual display with software rendering on
  the build host, producing a frame and a log. Not a substitute for playing — a substitute for not
  knowing whether the binary runs at all.
- **Performance harness:** a repeatable run that measures frame time with the crew active, so the
  performance criterion is measured, not remembered.

Not automated, and named as such: mouse feel, frame pacing, audio mix, and whether a space feels
inhabited. Those are yours.

---

## 10. Content budget (explicit, per the objection)

Content is not free, and the plan says so out loud:

- **Map work — expensive.** Existing decks and the published map sources are free. New bespoke spaces
  cost real effort; that is why the capstone reuses Voyager, and why programmatic `.map` generation
  (editor ladder item 4) matters more than the editor for volume work.
- **Navigation authoring — moderate, and mandatory.** A space without waypoint coverage is not
  inhabited by anything. The validator makes this cost visible instead of invisible.
- **New animations — expensive, and excluded from G3.** The original character animation sources exist
  in XSI format, so *modifying* animation is tractable; *authoring* animation is a skill, not a build
  step. G3 forbids new animations for exactly this reason.
- **New voice — money and scheduling.** Existing voice lines, speakers and the bark system are free and
  plentiful. New recorded dialogue is not. The capstone uses existing voice; new voice is a later,
  separately-costed decision.
- **Scripts and data — cheap, scales with agents.** 2,175 scripts as templates, a documented command
  set, and text-based data files (`NPCs.cfg`, `weapons.dat`, `items.dat`). This is where content volume
  actually comes from.
- **Text-only additions — cheap.** Signs, logs, menu text, infostrings.

---

## 11. Execution model — parallel agents, explicit gates

Parallelism is organised around G0's independent items, then around the tracks:
- Agent A — Track A client progression.
- Agent B1 — native script compiler, plus the 2,175-script corpus test.
- Agent B2 — entity dictionary conversion and editor game pack, plus the shipped-map rebuild test.
- Agent B3 — asset and nav-coverage validator with its negative test suite.
- Agent C — Track C design now; implementation only after G1, and only the G3 scope.

Rules for parallel work: no agent consumes another's artifact before that artifact's gate passes; every
hand-off is a file with a schema, not a description; and the gate evidence is produced by the workstream
that owns the artifact. Agent execution removes scheduling pressure. It does not remove the need for
boundaries, artifacts, or verification — it increases it, because more work happens without a human
watching it.

---

## 12. Risks, updated

- **Editor parity (new, moderate).** NetRadiant is the right lineage, but "same lineage" is not proof.
  Mitigated by making the shipped-map rebuild a gate item rather than an assumption, and by the
  fallback ladder in §4.
- **Compiler fidelity (new, low-moderate).** A rebuilt ICARUS compiler may differ subtly from `IBIze`.
  Mitigated by the byte-comparison corpus test — 2,175 samples will find discrepancies fast.
- **Named-crew entities may have hard-coded behaviour (new, moderate).** The shipped NPCs may carry
  assumptions not visible until runtime. Mitigated by G3's narrow scope, which exposes this on one deck
  with five to ten characters rather than across the ship.
- **Navigation coverage on existing decks (moderate).** Assumed adequate; to be **measured** in G3's
  first task, and reported as a number per deck before crew work begins.
- **Performance with many agents (moderate).** Addressed by the performance harness and the
  20–30 NPC / 60 fps criterion at G4.
- **Save size and integrity (moderate).** Agent state has an explicit per-NPC budget and an automated
  save/load assertion.
- **Scope (unchanged, highest).** Countered by the wedge: one deck at G3, one ship at G4, one scenario
  at G5. Nothing generalises before something plays.
- **Build host capacity (low, certain).** Disk needs clearing before toolchains, build trees and game
  data land.

---

## 13. Next actions

Start now, in parallel, all three of which are within days of producing a checkable artifact:

1. **G0 / item 1** — build the singleplayer game module natively for x86_64.
2. **G0 / item 2** — reconstruct the native script compiler and run it against all 2,175 shipped
   scripts.
3. **G0 / item 3** — write the validator with its negative test suite.
4. **G3 / task 0** — measure navigation coverage on the candidate Voyager deck, and report the number,
   because it decides how much crew the deck can hold before any crew code is written.
5. **Track A / M4 prep** — reproduce the Virtual Voyager deck walk early, not at the end, per the
   objection.

The GDK extraction path is already proven, so items 1–4 need nothing further from you. The first thing
that needs your hands is G1's playtest.
