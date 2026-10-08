# Long Way Home — how to work in this repository

**This file is deliberately static.** It says *how to work here* and *where the truth lives*. It does not
say what is currently true.

State belongs in the documents this file points to, and only there. If you are about to add a status, a
date, a count, a *"where the work stands"* or a *"what to do next"* to this file — stop, and put it in the
document that owns it instead. A context file that carries state is wrong within a week, and it is the first
file every agent reads, so when it rots it misleads all of them at once.

## What this is

A persistent starship simulator built on *Star Trek: Voyager — Elite Force*. One ship, one crew, systems
that can be operated together, damage that stays until it is repaired, and no reset button. The retail game
and its expansion are **modes of the client**, not a stepping stone: their behaviour must keep working.

Three modes, contract in `docs/client-modes.md`: the retail campaign and expansion, unmodified in
behaviour; retail multiplayer, run as **cMod as shipped** — inherited, zero delta from us; and Long Way
Home, our own single player and multiplayer, built on the released Raven game source.

## Where the truth lives — read the owner, do not guess

**Every question has one document that answers it. Read that one.**

- **what the game means, and how it should feel** — `docs/story-and-semantics.md`, `docs/design-creed.md`
- **the meeting system, the voice mechanism, and their programme** — `docs/staff-meetings.md`, `docs/programme-meetings-and-voice.md`
- **the ship as state: the save, the systems, the load adapter** — `docs/ship-model.md`
- **one system: its state, control, visible effect, failure mode, location** — `docs/ship-systems.md`
- **power, damage, the two budgets, cascades, abandonment** — `docs/damage-and-budgets.md`
- **who decides what is online, and how much power each system gets** — `docs/power-assignment.md`
- **what the ship's numbers should be, and the reasoning** — `docs/budget-squaring.md`
- **which deck hosts what** — `docs/ship-master-map.md`
- **what can harm the ship, and what each faction threatens** — `docs/scenario-atlas.md`
- **the record, the log and the memory; veracity, purge, assimilation** — `docs/the-record-and-the-log.md`
- **rank, access, authority, consoles and screens** — `docs/access-and-authority.md`
- **the crew's work: the queue, the watch, assignment** — `docs/crew-work.md`
- **exploration, survey, contacts, the dilithium constraint** — `docs/exploration-and-science.md`
- **the Borg** — `docs/borg-incursion.md`
- **endings and destinations** — `docs/endings.md`
- **the opening situation, and ranks from ensign to captain** — `docs/start-states.md`
- **how to build a location** — `docs/authoring-a-location.md`, `docs/location-brief-template.md`
- **every number claiming to describe Voyager, with its source** — `docs/lore-ledger.md`
- **what to trust, and how far** — `docs/confidence-and-verification.md`
- **the evidence behind any claim** — `docs/evidence/`
- **what the engine actually does** — the engine source. Read it; never recall it.

**And what is true today, what is proven, what is measured and what is next: `docs/gates.md`.** It is the
ledger and it is the authority. When this file and `docs/gates.md` disagree about the state of the work,
`docs/gates.md` is right and this file is out of date.

## How to work here

- **Game logic lives in `module/`, never in the engine.** Patches carry attach points into our files; a
  patch that contains logic is wrong. See `module/README.md`.
- **The engine may be extended** (`docs/engine-extension-policy.md`), but every extension is cvar-gated and
  **off by default**, keeps retail behaviour and saves intact, and stays a rebasable patch series.
- **Evidence, not claims.** A milestone lands with a written record in `docs/evidence/` — what was
  observed, the command that produced it, and the result. Not an assertion that something works.
- **A green check is not the answer to your question.** Ask what the check proves and whether that is what
  was asked. A check that cannot fail for the reason you care about is decoration.
- **Read the rendered thing, not the code that draws it.** For anything a person looks at, the judgement
  comes from the screen.
- **Measure what is numerical.** Contrast, size, height, distance, time, count — if the claim is a number,
  the evidence carries the measurement that produced it.
- **A wrong number in a document is worse than a missing one**, because it gets quoted back afterwards as
  settled. Correct it visibly rather than quietly.
- **No canon without a source.** If canon is silent or contested, decide once and record the decision as
  ours — recorded, so it can be overruled.
- **Do not repair a promise by weakening it.** If a document says X and the code lacks X, fix the code, or
  say plainly why the document is wrong and change it. A promise nobody can keep is worse than no promise.
- **No game assets, no third-party images, no generated audio in the repository.** Extracted data is
  regenerated by script and never committed.
- **Saves must replay identically.** A change that makes a save load differently is a defect, not a detail.
- **Never commit to `main`.** Branch, open a pull request, merge through it.
- **The code and the docs may be on different branches.** Verify which branch carries a file before citing
  it: **a line number from the wrong branch is a false claim**, and it has happened here more than once.
  The current branch model is recorded in `docs/gates.md`.
- **Name what you copied.** A new location is almost always a re-dress of an existing map, so the brief
  names its sources rather than asserting it copied nothing exactly.
- **Leave no engine processes and no agent sessions running on a shared host.**

## Building and testing

```sh
scripts/bootstrap-upstream.sh      # fetch the pinned upstream and apply the patch series
scripts/check.sh                   # validator: entity dictionary + script round-trip
scripts/test.sh                    # module unit tests (crew, ship) + the no-data checks
scripts/build-map.sh <map>         # compile a map; verify with tools/mapgen/check-bsp.py
scripts/build-ship.sh              # the ship-side build
scripts/s2-check.sh                # ship console / systems checks
scripts/g3-measure.sh              # the crew measurement harness (headless, ten minutes)
scripts/run-engine.sh +map <map>   # run the client; look for CM_LoadMap and "Munro connected"
scripts/run-scenario.sh            # a scenario run, for the owner's own sessions
```

## Traps that have each cost a session

- **Brush winding and texture paths make q3map2 exit 0 with an empty BSP.** Verify the BSP, never the exit
  code.
- **`-fs_basepath` needs a child directory spelled exactly `baseEF`.**
- **`map` is the single-player route; `spmap` is the Holomatch route.** Using the wrong one boots the wrong
  mode.
- **An engine rebuild wipes the module symlinks beside the binary**, which silently boots the retail menu
  instead of our code.
- **A new `.cpp` under `module/` is NOT compiled until cmake re-configures.** The module sources are found by a
  configure-time glob, so adding a file silently keeps the old source set — and because **the module links with
  unresolved symbols ignored**, the forgotten file appears only as "undefined symbol" *when the engine loads the
  module*, never at build time. Re-run the configure after adding a file:
  `cmake -S ../upstream/efgame -B ../upstream/efgame/build-linux -DLWH_MODULE_DIR=$PWD/module`. **A clean build is
  not proof that your new file was built.**

Details in `docs/authoring-a-location.md` and the `lwh-location-authoring` skill.

## Hosts, and where things live

- **Game data** is on `sasquatch` (the playtest host) at the GOG installation, **read-only**. All writes go
  to `build/home`. Never write into the installation.
- The design documents are also checked out read-only on `sasquatch` at `/home/c/big/git/lwh-docs`.
- **`sasquatch` is shared** — other agent sessions run there. Do not switch branches in a tree that has a
  live session in it, and do not leave anything running.
- The builder host and the controller host are different machines. **Check which one you are on before
  running anything**, and run a command on the host that can actually answer it.

## The one rule that covers the rest

When this file, a design document and the code disagree, **say so plainly instead of smoothing it over.**
Then go and find out which of them is right, and correct the other one in the same pass.
