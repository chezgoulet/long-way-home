# The path to playtest

The ordered path, as distinct from the ledger. `docs/gates.md` holds *what is closed and what is measured*;
this document holds *in what order the remainder is done*, and it is the file to update when the order changes.

Naming the goal plainly: the owner's bar for sitting down at the ship himself is **a much more complete version
of what we mean to ship**, not a build with one new thing in it. This path is ordered to reach that bar, and it
says which stage is the one after which a play session is worth his evening.

**No durations anywhere. A stage is bounded by its exit criteria, not by a calendar.**

## The reframe this path rests on

Almost the entire "then, in rough order" queue in `docs/gap-analysis.md` is now closed, and the ledger records
each with evidence. What remains is **one category, and the ledger names it in its own words twice**: the
player's own body is recorded as done *as a model*, with "**the model in the world**" named as what is left, and
the airponics bay and quarters are recorded as done *as systems*, with "**as a mapped place**" named as what is
left.

So the remainder is not a feature list. It is the list of places where a number in the save has no counterpart
in the room: the ship's condition that the player cannot see, and the player's own body that the ship cannot
touch. Four sessions built rich state; the world side of it is thin.

**The ordering rule, in one sentence: world-visible before world-modelled, the player before the crew, then
content, then the ending.**

## Stage A — the environment, in the world  *(built 2026-10-06; `docs/evidence/environment-in-the-world.md`)*

**Goal.** The ship's environmental state becomes something the player is subject to, not a number.

**Deliverable.** Per-person gravity driven from the deck's own gravity value, using the engine's
`SVF_CUSTOM_GRAVITY` opt-out so a deck that recovers is restored; a breached compartment that visibly and
physically pulls; a force field that holds air and can be seen. Extends the existing `SyncDamage`/`SyncFire`
pattern — no second mechanism.

**Exit criteria, observable:**
- With a deck's gravity driven to zero the player does not fall, **and can still leave**.
- The deck's gravity returns to normal when the plating is supplied — the case the engine's own FIXME says it
  cannot do.
- A breached compartment with the field off loses air observably and physically; with the field on, the air
  holds and the state agrees.
- With the cvar off, the ship behaves and saves exactly as before.
- `scripts/test.sh` and `scripts/check.sh` exit 0.

**Blocks.** Nothing. The engine question this depended on is settled (see *Decisions taken*).

## Stage B — the player in the world  *(built 2026-10-06; `docs/evidence/player-in-the-world.md`)*

**Goal.** The player stops being an observer of a ship that can be hurt.

**Deliverable.** `PlayerIncapacitated` already exists as state. Wire it to the world: the player takes damage
from the causes the model already tracks — an exploding console, fire, vacuum, a boarder — can be incapacitated,
carried, recovered in sickbay, and lost.

**Exit criteria, observable:**
- A player standing at a console on a degraded grid can be hurt by it.
- A player in a breached, airless compartment is affected, and can leave or be recovered.
- Incapacitation is a state the ship acts on: someone comes, and the log says who.
- Death of the player is reachable and is not a reload — it is the attrition the programme exists for.

**Blocks.** Stage A, because a body is what makes an environment land on a person.

## Stage C — places, as places  *(in progress: all five absent decks are built — `docs/ship-master-map.md`; the owner's walkthrough is the open exit criterion)*

**Goal.** The decks become somewhere a person works, rather than blockouts.

**Deliverable.** Deck 12 and deck 13 re-dressed from a shipped Voyager interior — `docs/authoring-a-location.md`
is the procedure and its first rule is reuse; name what was copied in the commit message. Then the same for the
remaining generated decks. Then the bays and quarters as mapped places, and the blocked station markers on the
published decks.

**Exit criteria, observable:**
- A deck built by re-dressing loads, is reachable on the lift, hosts its system, and passes `check-bsp.py`.
- The commit message names the map it re-dressed.
- **The owner's walkthrough** — the one check that closes a deck, and the one that cannot be automated.
- The blocked station markers are resolved or the blocker is explained afresh.

**Blocks.** Nothing technically. It is last of the world stages because it is the most expensive and the least
load-bearing: a re-dressed deck on a ship whose environment does nothing is scenery.

**After this stage a play session is worth the owner's evening** — the ship is somewhere, the environment acts
on him, and his body is subject to it.

## Stage D — the crew in the world

**Goal.** The crew stop being records that react correctly and become people in the room.

**Deliverable.** The taste and allegiance priors (`docs/affinities-and-allegiance.md`); the per-participant
meeting brief the record's rules require; the two logs as separate stores; the meeting overlay and its async
generator (`docs/staff-meetings.md`, M2–M4); the Collective speaking in an assimilated person's voice.

**Exit criteria, observable:**
- The same event lands differently on two crew members, traceably to a declared affinity.
- A meeting plays with the model absent, and its brief is built per participant from marks and the log.
- A purge removes the published report and leaves the `MEM_LOG`-sourced marks orphaned, not erased.

**Blocks.** Stage C only in the sense of budget, not dependency.

## Stage E — an ending

**Goal.** A run can finish, several ways, and the log is the last frame.

**Deliverable.** The destinations as readers, the arrival clip, the transwarp route, and the question of
whether the player may decline to file (`docs/endings.md`).

**Exit criteria, observable:**
- A run can reach more than one destination, and the reading differs by reader for the same record.
- No destination is presented as better than another, in text, UI or log.
- The arrival is three seconds of our own engine's warp-in, and the log is what follows.

## Cross-cutting: what only the owner can close

These are not stages and they do not block the path, but they are the gates the code is already waiting on, and
each is an afternoon of his: **S2 at the console, S7's breach puzzle played by hand, S10's playthrough, G3's
does the deck feel inhabited, S3's someone walking each deck, and G7's two-machine LAN.** The code has run ahead
of the judgement, and that gap grows with every stage.

## Decisions taken (dated)

- **2026-10-06 — per-person gravity is supported and stable.** `pm->ps->gravity` is the per-player field the
  movement code uses; `g_active.cpp` refreshes it from the cvar **only when `SVF_CUSTOM_GRAVITY` is unset**;
  `target_gravity_change_use`'s non-global path sets the field and raises the flag. The way back is to clear the
  flag — which the engine's own comment says it lacks. Verified in
  `/home/c/big/git/upstream/efgame/src/game/`.
- **2026-10-06 — the engine has no notion of atmosphere.** Air is modelled in the save and shown in the world.
  No pressure model will be written; the honest implementation is the faked one.
- **2026-10-06 — gravity is per person, never per region.** The worldspawn value is per map and the stitched
  ship is one map.
- **2026-10-06 — the deck re-dress is a re-dress.** Seventeen Voyager interiors ship in the map sources, and the
  authoring document's first rule is reuse. Generated blockouts were a misreading of "no map for that deck".

## Defects found in the corpus while writing this

- **`docs/gates.md` contradicts itself on the Maquis — CORRECTED 2026-10-07.** One entry recorded "resentment
  and integration as an arc remain" while a later entry records both as done. The stale parenthetical was the
  candidate, and the omissions sweep removed it; `docs/gates.md` now carries the arc as done in one place
  (`docs/omissions.md`, "Ledger corrections made in the same pass"). This defect note is kept as the record of
  the find; the contradiction is no longer present. *(A re-verification on 2026-10-09 found only the two
  consistent entries.)*
- **Every dated line in the ledger is a day ahead of the wall clock**, consistently, across days. Either the
  dates are wrong or the convention is unexplained. A plan that dates its own work wrongly cannot be audited.

## Boundary rules

- Do not begin a stage until the previous stage's exit criteria are verified with evidence.
- One stage, one branch, one pull request into `feature/g3-reactive-crew`. CI green before the merge; a red
  check is worked, never re-run past.
- New work discovered mid-stage is appended to the *next* stage's criteria, never absorbed into the current one.
- **After any document lands on `testing`, merge `testing` into the builder trunk before dispatching the
  next brief.** A document the builder cannot see does not exist — that is design creed #9, and it was
  re-learned when this path document landed on `testing` and a session was dispatched onto a branch that
  predated it, so its first act was a failed read.
- Every stage closes with its own evidence document under `docs/evidence/`, in the house form: PASS lines
  stating what was observed, and the command that produced them.
