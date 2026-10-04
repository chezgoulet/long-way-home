# Program Proposal v2 — Elite Force: Native Linux Client, Authoring Pipeline, and Living NPCs

Date: 2026-10-04 (supersedes `native-linux-client-proposal.md`)
Prepared for: Christopher Goulet
Status: proposal, awaiting go/no-go and four decisions (§11)

---

## 1. Recommendation

Run this as a **program of three tracks plus a capstone**, not as a single client port:

- **Track A — Native Linux client.** Port the existing open-source singleplayer client to x86_64 Linux.
  **Virtual Voyager is in the MVP**, not deferred.
- **Track B — Story authoring pipeline.** Build the tooling that turns game assets into new
  singleplayer experiences: locations, characters, ships, dialogue, missions — reproducibly, on Linux,
  headless-first.
- **Track C — Living NPCs.** Extend the game's existing autonomous-NPC substrate into a duty, routine
  and social simulation, entirely in-engine, deterministic, no external models.
- **Capstone — one authored experience.** A new immersive singleplayer scenario built with Track B and
  populated by Track C, running on Track A. This is what proves the program worked.

**Probability of the client milestone (Track A, including Virtual Voyager): 70–85%.** Slightly lower
than the v1 estimate for the port alone, because Virtual Voyager widens the acceptance bar — but the
evidence that it is reachable is better than I assumed (§4).

**Honest headline:** this is no longer a four-to-eight-week job. Client playable in roughly two months;
the whole program is a several-month effort at a sustainable pace. The good news is that Track B's
first tools and Track C's design can start immediately, in parallel, without waiting for the port.

---

## 2. What changed in v2, and one finding that matters

Three requirements were added: Virtual Voyager in the MVP; a tooling and application layer for building
story elements from assets; and a local in-game AI for autonomous NPCs.

On the third, I went looking for the substrate before proposing to build one — and it is already there,
in the released singleplayer source, more completely than I expected:

`bstate.h` defines **39 behaviour states**, and its own header comment states the design intent:
*"These take over only if script allows them to be autonomous."* It contains life-shaped states
alongside combat ones — `BS_ROAM` ("roam around, collect stuff"), `BS_WAIT`, `BS_SLEEP` (with an awake
script triggered by sound), `BS_PATROL`, `BS_FORMATION`, `BS_INVESTIGATE` (head to a point, watch and
listen), `BS_SAY` (turn the head, play the talk animation, speak, timed to the audio), `BS_AIM` and
`BS_LOOK` for head/torso tracking, `BS_MEDIC`, `BS_TAKECOVER`, `BS_GET_AMMO`.

Around that sit the systems a living-NPC layer needs: a navigator with waypoints and nav goals
(`g_nav.h`, `g_navigator.cpp`), a perception model driven by per-character parameters
(`earshot`, `visrange`, `vigilance`, `hfov`, `vfov` in `NPCs.cfg`), squad coordination (`AI.h`
`SQUAD_*` states), a command-and-response dialogue system (`say.h`: acknowledge, refuse, bad command,
bad hail, in numbered variants), interactive objects (`g_usable.cpp`, `g_object.cpp`), ambient life
(`g_ambients.cpp`), and ICARUS scripting for scripted sequences.

**So Track C is not "build an AI." It is "extend a combat-shaped autonomy system into a life-shaped
one."** That is a much better starting position, and it is the single most important correction in this
revision. It also tells us exactly where the work plugs in — new behaviour states in `bstate.h`, new
per-character parameters in `NPCs.cfg`, a direction layer that selects among duties, and dialogue that
goes NPC-to-NPC rather than only NPC-to-player.

---

## 3. The program, and what can start now

```
Track A  client port ──────────┐
   (weeks)                     │
Track B  authoring pipeline ───┼──► Capstone: one authored experience
   (starts NOW, headless)      │        (after A+B+C are usable)
Track C  living NPCs ──────────┘
   (design now, code after A is playable)
```

Dependencies are real but not total. Track B's core tools are *file-transform* tools — they compile
maps, compile scripts, validate asset references — and they need no running client. They can be built
and tested on the Linux server today. Track C's design can be written now; its implementation needs a
running client to iterate against, so it waits for Track A's playable milestone.

---

## 4. Track A — Native Linux client, Virtual Voyager included

Everything in v1 stands: the architecture exists in `VoyagerSP-Android` v1.2.0 (July 2026), split into
the game module, an engine that already targets GNU/Linux, and a thin integration bridge whose only
Android coupling is a logging include. Build system, module loading, pointer-width work — all as
described in v1.

**Virtual Voyager moves from "uncertain, investigate later" to MVP. Two reasons it is reachable:**

1. **The expansion's interface is in the released source.** `ui_turbolift.cpp` implements the turbolift
   deck-selection menu and the station's menus: Library, Astrometrics, Personal Log, Medical Log,
   Recipes, Social Calendar, Disease Library, Shooting Range, Weapon Library, Cargo, Engineering
   Library — with an `inHolodeck` flag for the holodeck spaces. This is not a mystery feature we would
   be reverse-engineering; it is code we already have, with deck slots (`deck1`…`deck8`+) already
   written.
2. **Upstream has already been here.** The Android port carries a closed issue titled "Unable to access
   turbolift menu in Virtual Voyager" — meaning the expansion content loads, the failure was in the
   menu path, and the maintainer resolved it. We inherit both the fix and the knowledge of where it
   breaks.

**What "functional Virtual Voyager" means as an acceptance bar** (proposed, you can tighten it):
the ten extra decks load; the turbolift menu opens and moves between decks; the station's interactive
menus open without errors; the holodeck and the shooting range are entered and exited cleanly; a save
taken inside Virtual Voyager reloads correctly.

**Exit criteria by milestone** (unchanged from v1, with VV folded into M4):

| # | Milestone | Proof |
|---|---|---|
| M0 | Game module builds for x86_64 Linux | two shared libraries, four intended exports, no unexpected unresolved symbols |
| M1 | Engine builds and runs on Linux | binary starts, logs version, opens a window |
| M2 | Module loading, first menu | engine log shows module load; LCARS main menu renders |
| M3 | Playable | first base-campaign mission completed, save written and reloaded |
| M4 | Content acceptance | base campaign; **Virtual Voyager per the bar above**; Starbase 11 maps load clean |
| M5 | Packaged | one installable artifact, cutscenes decoding, desktop entry |

Effort: 5–9 weeks of focused work, most variance in M2 and M3.

---

## 5. Track B — The story authoring pipeline

The goal is to make "build a place, put characters in it, give them something to do, ship it as a
playable scenario" a repeatable operation rather than a heroic one-off.

### What already exists (verified inventory)

**Scripting.** The ICARUS system is the game's mission scripting layer, and the official toolchain
survived: `BehavEd.exe` (the behaviour editor), `BehavEd.bhc`, `IBIze.exe` (the batch compiler that
turns script text into the `.ibi` files the game loads), the ICARUS manual, sample scripts, and — most
usefully — `SourceForBehavEd/`, a published set of headers describing the scripting command surface
(`g_functions.h`, `objectives.h`, `bstate.h`, `say.h`, `anims.h`, `g_nav2.h`, `q_shared.h`).

Critically, **the offline script compiler is in the game source we already have**:
`efgame/src/icarus/Interpreter.cpp` and `Tokenizer.cpp` are excluded from the game module build
precisely because they are the offline compiler, not runtime code. That means a native Linux
`ibize`-equivalent can be **built from source we already possess**, rather than reverse-engineered or
run under Wine. This is the first tool I would build, and it is days of work, not weeks.

**Levels.** The official Game Development Kit ships `efRadiant` (editor), `q3map2` (map compiler) and
`bspc` (bot/AI navigation compiler). The compilers are GPL and build on Linux; the editor GUI is a
2000-era Windows application. More importantly, **the source files for every shipped map are public** —
Voyager's decks, the Borg levels, the Holomatch arenas, the Virtual Voyager decks, Stasis, Forge,
Scavenger, Dreadnought, Holodeck, CTF and Brig maps. That is a reference library of the game's own
architecture, in editable form.

**Characters, ships and props.** Player model ASE sources, the original character animation sources in
XSI format, the carcass converter, and MDR tooling for Milkshape. EF's model formats are `.mdr` with
`.tik` configuration files.

**Data.** `NPCs.cfg` (per-character parameters: aggression, aim, intelligence, vigilance, sight and
hearing ranges, movement style, speeds, model parts, per-part joint limits, sound set), `weapons.dat`,
`items.dat`, `addon.npc`, `boltOns.cfg`, `infostrings.dat`. Story elements at the data layer are
editable text.

### What we build

1. **A native script compiler** (`ibize` replacement), built from the game source. *Proof: compile the
   shipped sample scripts and byte-compare behaviour against the originals.*
2. **A headless map + navigation build**, scriptable end to end: source map → `q3map2` → `bspc` →
   packaged `.pk3`. *Proof: take a shipped map source, rebuild it, and load the result.*
3. **An asset manifest and validator** — one command that checks a scenario's maps, models, textures,
   sounds, script bytecode and data files all resolve, and fails loudly naming what is missing. This is
   the tool that prevents "it worked on my machine" content. *Proof: it catches deliberately broken
   references.*
4. **Character and ship assembly** — a documented path from model source to in-game character: model
   conversion, `.tik` configuration, `NPCs.cfg` entry, sound set, spawn entity. *Proof: one new
   character and one new ship appear in a test level.*
5. **A scenario scaffold** — a directory layout and build file that turns "a folder of assets plus
   scripts" into a distributable package, so a scenario is a repository, not a pile of loose files.
6. **Optional: a modern authoring front end.** The 2000-era editors are Windows-only. A reader/writer
   for the script format plus a generator, so scenarios can be produced programmatically (which is also
   what makes machine-assisted authoring possible later, without an LLM in the loop at runtime).

Effort: 6–10 weeks of focused work, front-loaded — tools 1 and 3 land in the first fortnight.

---

## 6. Track C — Living NPCs

### What is already there

Described in §2: 39 autonomy-gated behaviour states, a navigator with waypoints and nav goals, per-NPC
perception parameters, squad coordination, a command/response dialogue system, usable objects, ambient
life, and ICARUS for scripted sequences. The existing system is genuinely capable — it was written by
Raven's engineers for a squad-based shooter and it shows.

### Where it falls short of "living"

- **Motivation is combat-shaped.** Nearly every state is about fighting, guarding, patrolling or
  healing. `BS_ROAM` and `BS_WAIT` are the only obvious life-shaped states. Nothing models *duty*,
  *routine*, or *preference*.
- **Autonomy is script-gated, per moment.** The header comment says behaviour states take over "only if
  script allows them to be autonomous." Someone had to write that permission into the mission. There is
  no system that decides, on the NPC's own behalf, that it is now time to go to the astrometrics lab.
- **Dialogue is command-response, and one-directional.** `say.h` covers acknowledging, refusing and
  reacting to the player. There is no NPC-to-NPC conversation.
- **No social or spatial memory.** An NPC does not know who else is aboard, where they usually are, or
  whether it saw them five minutes ago.
- **Animation breadth is comedy-limited by the animation set.** Life-like action needs idle variety,
  console work, sitting, conversation gestures — new animations, which is Track B's territory.

### The design

A **direction layer** above the existing behaviour states, not a replacement for them:

```
  perception (existing: visrange, earshot, vigilance, LOS)
        │
  world model per NPC  ──  who is here, where things are, what time it is
        │
  duties & routine     ──  schedule + role + current state (hunger/tiredness/social need)
        │
  utility selection    ──  pick the next goal: a duty, a need, a reaction, or an ICARUS script
        │
  behaviour states     ──  existing bstate.h machine, extended with life states
        │
  navigator + anims + say ──  existing steering, animation, dialogue
```

Concretely, what we add:

- **A ship clock and schedules.** Voyager has a day. Crew have shifts. A schedule maps role → deck →
  station → time. NPCs move between posts because the clock says so.
- **Duties as first-class goals.** "Engineering console, 09:00–13:00, maintain the warp core readouts,
  acknowledge the player if addressed." Authored as data, not as mission script.
- **New behaviour states** in `bstate.h` for the life-shaped verbs: travel to post, work at station,
  socialise, eat, rest, converse, follow someone of interest.
- **NPC-to-NPC conversation.** Two NPCs near each other with compatible roles exchange lines, with
  facing, gesture, and a duration — extending the existing `BS_SAY` and `say.h` vocabulary rather than
  inventing a new subsystem.
- **A director.** A low-frequency scheduler that keeps the ship coherent: not everyone in the same
  corridor, quiet decks stay quiet, the bridge stays staffed. This is the difference between NPCs and a
  crowd.
- **Persistence.** Agent state (post, schedule cursor, goals, world model essentials) survives
  save/load, with an honest size budget.

**Explicitly not an LLM.** All of this is deterministic, in-engine, and runs inside the game's frame
budget — no external service, no network, no model weights. The design deliberately uses
parameterised personality (reusing the `NPCs.cfg` parameter idiom), utility scoring, and authored
line sets. If we later want generated dialogue *authoring*, that is an offline tool concern in Track B,
not something the shipped game depends on.

**Performance target:** 20–30 living NPCs on one map at 60 fps on the target desktop, achieved by
running direction and utility selection at a low tick (a few hertz), reusing the existing navigator for
movement, and culling perception aggressively by distance and field of view.

Effort: design in the first weeks; implementation 8–14 weeks for a first genuinely living crew, on one
ship, before it is generalised.

---

## 7. Capstone — one authored experience

The integration test that makes all three tracks real: a single, self-contained immersive scenario —
a deck or two of Voyager, or a station, or a ship of our own design — with authored objectives, new or
repurposed characters, and a crew that goes about its business around the player.

Success is defined by the player, not by the build log: you walk into a space, and it is inhabited.

Effort: 4–8 weeks of authoring once Tracks B and C are usable. It will also, inevitably, generate the
next round of tool requirements — which is the point of doing it.

---

## 8. Sequencing and effort

- **Months 1–2:** Track A to playable, with Virtual Voyager in the acceptance bar by the end. In
  parallel from week one: Track B tools 1–3 (native script compiler, headless map build, validator) and
  Track C's written design.
- **Months 2–4:** Track A finished and packaged; Track B character/ship assembly and scenario scaffold;
  Track C implementation begins on one ship.
- **Months 4–6+:** Track C matured; capstone authored; tooling hardened by contact with real content.

Total: roughly four to seven months of focused effort at a sustainable pace. Ranges, not commitments —
I will report progress against them rather than defend them.

---

## 9. Risks

**Virtual Voyager widens the MVP (new, medium).** Mitigation: the interface code is in the source and
upstream already fixed a turbolift bug; verify the deck walk early in M4 rather than at the end, and if
a specific activity proves broken, document it precisely instead of silently narrowing the claim.

**Pointer-width audit on x86_64 (likely, low).** 64-bit ARM hit 245 truncations across 24 files, and
the upstream repository catalogues them. Our starting list is that audit.

**Integration assumptions in the bridge (moderate).** Android lifecycle and surface handling differ
from desktop; the bridge is small and readable, so instrument it early.

**Navigation coverage on new maps (moderate, Track C).** NPCs can only live where the navigation mesh
exists, which means `bspc` must be part of every scenario build. This is exactly why the validator
(Track B tool 3) exists, and why Track B precedes Track C in implementation order.

**Animation sparsity (moderate, Track C).** Life-like behaviour needs life-like animation. Mitigation:
start with what `anims.h` already provides, and let the capstone drive the demand for new animation
work through Track B's asset path.

**Savegame size and integrity (moderate).** Persisting agent state inflates saves; upstream already
documents save fragility. Mitigation: budget explicitly, keep agent state compact, test save/load in
Virtual Voyager and in occupied spaces.

**Scope (certain, highest).** This program has a real appetite for scope. The counter is the wedge:
client first, one ship for Track C, one scenario for the capstone. Generalising comes after something
plays.

**Build host capacity (certain, low).** The build machine's disk sits near capacity and this program
needs toolchains, build trees and game data (~541 MB for the base archive alone). Clear space first.

---

## 10. Licence posture

- **Engine:** GPLv2 (id Software → ioquake3 → lilium-voyager → Quake3e). Source obligations attach on
  distribution; personal use triggers nothing.
- **Game module:** Raven's source licence (2002) — the right to create modifications and distribute them
  free of charge, non-commercially; no sale, no commercial exploitation, no reverse engineering of the
  shipped game. Our whole program sits inside it.
- **Tools:** the 2000-era tools (BehavEd, IBIze, GDK) ship under Raven's tools EULA. Note that we do
  **not** need to redistribute them: the script compiler is rebuilt from the released game source, and
  the map compilers are GPL. We use the old binaries as reference, on our own machines.
- **Assets:** never redistributed. The client requires a copy you own, as upstream does.
- **Boundary discipline:** the game module stays a dynamically loaded file, not linked into the GPL
  engine — the arrangement the retail game itself used, and the one that keeps the two licences
  separable if anything is ever published.

## 11. Decisions required

1. **Program shape** — accept the three-track-plus-capstone structure, or cut Track B or C from this
   phase.
2. **Virtual Voyager acceptance bar** — adopt the proposed bar in §4, or tighten it.
3. **Capstone setting** — Voyager (reuses existing decks, fastest and most familiar) or somewhere new
   (proves the location pipeline harder). Recommendation: Voyager for the first one, because it tests
   *inhabitation* rather than asset volume, and assets are the more expensive variable.
4. **Track C fidelity** — a "living ship" (schedules, duties, social life on stations) versus a
   narrower "reactive crew" (crew acknowledge and move around but do not keep a routine). The first is
   the real goal; the second is a milestone on the way to it. Recommendation: build the narrow one
   first, on the way to the full one, and call it what it is.
5. **Upstream strategy** — develop Track A as a branch offered upstream (recommended in v1 and still),
   while Tracks B and C remain ours.

## 12. Immediate first steps, if approved

Three things start now, none of which wait for the others:

1. **Track A / M0** — build the singleplayer game module natively for x86_64 Linux; produce the two
   libraries and the symbol report.
2. **Track B / tool 1** — reconstruct the native script compiler from the game source, and validate it
   against the shipped sample scripts.
3. **Track B / tool 3** — write the asset validator, because it is the cheapest thing that makes every
   later step verifiable, and because it is useful even before the client runs.

All three produce artifacts that either exist or don't, and all three are within days of starting.
