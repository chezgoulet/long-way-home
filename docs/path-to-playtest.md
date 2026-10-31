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

**Slice 1 built 2026-10-31 — the priors.** Every crew member now carries `likes[]`, `dislikes[]`,
`pursuits[]` and an `allegiance` with a strength and what it is to. The record is generated for the whole
roster from the roster seed and authored over for the named few where canon speaks; every row says which
it is, and the two items the design document could not verify (Seven's music studies, Chakotay's boxing)
are deliberately not authored. Taste enters at the two points the document names and no others:
`EffectiveValence` at the single mark writer, so the same event lands differently on two people; and
`PreferredProgram`/`PreferredTask`/`VolunteerJob` at the schedule and the job queue. The bond store is
untouched and the record is static content, so the save format does not move (66). Evidence:
`docs/evidence/affinities.md`, `TestAffinityRecord`, `TestAffinityValence`,
`TestBondFoldUnchangedWithoutAffinity`, `TestAffinityChoice`, `scripts/affinity-check.sh`.

**Slice 2 built 2026-10-31 — the brief, the agenda and the report.** The meeting overlay and the async
generator were already built in the meeting lane (M3/M4); this slice makes the brief they consume
**per participant**: the declared affinities, a derived want and will-not-say with the row each comes
from, a stance on every enumerated option with the affinity that explains it, and an agenda grounded
in the call, an instrument, a mark, a promise and a log line. The worker reads that brief and writes
the disagreement; the model absent leaves the authored skeleton. After the decision the simulation
publishes a report into the official log that addresses every agenda item and names the dissent. No
save change (66). Evidence: `docs/evidence/staff-meetings.md`, `TestMeetingBriefAffinity`,
`TestMeetingReportAddressesAgenda`, `tests/tools/test_meeting_worker.py`, `scripts/meeting-check.sh`.

**Slice 3 built 2026-10-31 — the two logs, and what a purge leaves behind.** The two stores were
already separate: the published report is a document in the official log, and a person's marks are in
their own `memories`. What was missing was the meeting-facing writer and the reader. A meeting now
writes a `MEM_LOG` mark (`MEM_MEETING`) into everyone who could read the report and was **not** in
the room; those in the room were there and are not given a read mark they did not earn. A purge
removes the report and orphans those marks — held, not erased, and not reattributed; `ship recall`
says the report they came from has been purged. No save change (66): `event` already carries the new
id and `orphaned` already existed.
Evidence: `docs/evidence/two-logs.md`, `TestPurgeLeavesMeetingMarksOrphaned`,
`scripts/two-log-check.sh`.

**Exit criteria, observable:**
- ✅ **The same event lands differently on two crew members, traceably to a declared affinity.** Built and
  demonstrated: the evidence prints one event, the liker and the disliker, their declared rows, the two
  marks and the bond each fold reads (`scripts/affinity-check.sh`, `test_ship_core --affinities`).
- ✅ **A meeting plays with the model absent, and its brief is built per participant from marks, the
  log and affinity.** Built in slice 2, 2026-10-31: the brief carries each participant's declared
  likes, dislikes and allegiance; a derived want and will-not-say with the row each comes from; a
  stance on every enumerated option with the affinity that explains it; and an agenda grounded in
  the call, an instrument, a mark, a promise and a log line. The authored skeleton is the floor when
  the model is absent, and the published report addresses every agenda item and names the dissent.
  Evidence: `docs/evidence/staff-meetings.md`, `TestMeetingBriefAffinity`,
  `TestMeetingReportAddressesAgenda`, `tests/tools/test_meeting_worker.py`,
  `scripts/meeting-check.sh`.
- ✅ **A purge removes the published report and leaves the `MEM_LOG`-sourced marks orphaned, not erased.**
  Built in slice 3, 2026-10-31: a meeting publishes its report into the official log, the people
  outside the room carry it as a `MEM_LOG` mark (`MEM_MEETING`), and a purge removes the report while
  leaving those marks held, orphaned and not reattributed. The reader is explicit: `ship recall` says
  the report it came from has been purged, and the brief still builds. The purge/orphan mechanism itself
  was already demonstrated in the memory work (`TestPurgeMovesRegard`); this slice supplies the
  meeting-facing half. Evidence: `docs/evidence/two-logs.md`,
  `TestPurgeLeavesMeetingMarksOrphaned`, `scripts/two-log-check.sh`.

**Still deferred inside Stage D:** allegiance is recorded and now weights a meeting's **stance**, but
not the valence of an event or an order's compliance, so the document's Maquis/Starfleet morale
consequence is not built; a pursuit does not yet produce an artifact or an event the log can point
at; and the liked program observed on the deck in the running game (as opposed to the model's choice)
needs a session. The Collective's voice is the remaining slice; the meeting overlay was built in the
meeting lane before this slice, and its own in-engine run was not repeated here.

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
- **Every dated line in the ledger is a day ahead of the wall clock** — for the sessions of 2026-10-05
  and 2026-10-06 only, and the convention is now recorded rather than the history rewritten. A date in
  the ledger names the **working session**; the commit timestamp is the machine's record of landing;
  the two disagree by a day for those two batches (15 of the 17 dated evidence documents in the window;
  one ran two days ahead, one a day behind) and agree everywhere else. **Do not renumber history**; the
  convention lives in `docs/gates.md`'s header. The find is kept: a plan that dates its own work must
  say which clock it means. Measured in `docs/evidence/the-stale-rows.md`.

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

## The primitive crossing — what is queued, and in what order

**Gap coverage has two halves, and they are different instruments.** `docs/omissions.md` asks *is this
commitment registered?* — it finds what nobody wrote down. The **designs-against-the-code** pass asks *does the
game do what the document says?* — it finds where what was written down and what was built disagree, which the
register cannot see by construction because those commitments are already on the roadmap. Both belong before
content, and neither replaces the other.

**A primitive is ready when three questions and a fourth are answered** — and the third and fourth are what the
content engine needs, not the player:

1. **Does it exist?** — `docs/hook-register.md`, guarded both ways by `scripts/hooks-check.sh` (479 public
   functions, all registered).
2. **Does it behave as the design says?** — the designs-against-the-code reconciliation.
3. **Can a caller reach it?** — `docs/hook-triage.md`: of the register's hooks, 122 are reachable only from the
   developer console and 35 from nothing at all, each sorted into *diagnostic by design*, *simulation-raised*,
   or **genuine gap** (62 of them, per the reachability pass).
4. **Which declaration does it answer?** — because content here is *declared*, not thrown: a scenario states its
   breeding conditions, the systems it stresses, the staff it needs, the cost to answer or ignore, its residue,
   visibility and warmth, and is instantiated **through the post** rather than a named person (O15, and the
   post-not-name law that makes an all-fictitious crew possible).

**Queued, in this order, decided 2026-10-11:**

1. ~~**The deck-12 environmental readout** (walkthrough G18)~~ — **built 2026-10-11** on
   `feat/the-deck12-readout` (`docs/evidence/the-deck12-readout.md`,
   `scripts/deck12-readout-check.sh`): the deck's air, its countdown and the source holding it, on a
   surface at the deck-12 watch console, so the per-deck air ruling (C4) is legible to a person
   standing there.
2. **The holodeck reactor as a per-deck source** — the last piece of canon's three independent power sources
   (O2's remainder), behind the sources that already survive the main grid. **BUILT 2026-10-11**
   (`feat/the-holodeck-reactor`, `docs/evidence/the-holodeck-reactor.md`, save version 60): the fourth
   source, off the grid (incompatible), refused while the grid is up, a shortfall when over-committed, and
   spent by a cross-tie. O2 is closed.
3. **The derivation — declaration → primitive → call path** — the crossing instrument: for every field a
   scenario must be able to state, the primitive that answers it and the way the engine calls it. A primitive
   nothing can declare is neither a gap nor finished; it is inventory. **This is the table a content author
   works against, and it is the check that says the crossing is safe.**
4. **Then content** (O15's nine-scenario slate), authored under the two laws already recorded: the
   post-not-name rule, and the delivery direction per line.

**Ruled 2026-10-12, and what each queues** (recorded here because the register carries the decision and
this document carries the order):

1. **Shields become per-section — canon's six arcs** (C-14). Queues the largest of these lanes: damage
   routes to a section, the save carries them, and a scenario can name the arc it struck. Until it lands,
   no declaration can say *where* a hit went.
2. **Nine departments, as designed** (3.3), and **the complement as hull data** (8.53). Queued together:
   both are the crew's composition, both are what the manifest's staff and condition fields will be
   written against, and doing them apart would settle the vocabulary twice.
3. **The officer of the deck wired into `DispatchOrder`** (C-11). Queued: small, and it makes the bills
   and the orders chain actually bind, which is the difference between a rule and a decoration.
4. **The failure roll measured before it is ruled** — whether a condition reaches the roll today comes
   back as a number, then the fork is decided on the number rather than on the document.

**Ruled 2026-10-12 (second batch), and what each queues:**

5. **The sector honours its stated split** (C-4): the roll moves to `% 10`, and the four kinds the ledger
   omits are added to it. Queued with the roll's own tests.
6. **Damage control is three-tiered** (C-12, the owner's own design): a standing party, a severity that
   scales its size, and large jobs that draw post-holders off their stations. Queued after the departments
   lane, because *which* stations it draws from depends on how many departments there are.
7. **Canon's twelve-shot adaptation, with the ramp inside it** (C-15). **BUILT 2026-10-12** on
   `feat/the-adaptation` (`docs/evidence/the-adaptation.md`, save version 61): the adapter is a per-weapon
   count of penetrating shots whose twelfth adapts the weapon, with the ramp inside the bound; it changes
   the odds the counter-play kit pushes against, so it landed with its own check (`TestTheAdapter`,
   `test_ship_core --adapt`, `scripts/counterplay-check.sh`).
8. **Maintenance becomes a seventh job kind** (3.23). **BUILT 2026-10-13** on `feat/the-maintenance`
   (`docs/evidence/the-maintenance-kind.md`, save version 64): the kind is appended to `JobKind`, its
   axis is a system's **wear** with `MAINTENANCE_DUE = 0.20`, the standing party lowers the clock and the
   failure odds it sets, the order rides the same authority surface (`ORDER_MAINTAIN`), and the wear clock
   is saved -- one small job, landed with its own check (`TestMaintenanceJob`, `test_ship_core --maintenance`,
   `scripts/maintenance-check.sh`).
9. **The played-character ceiling corrected** (8.52): 64 multiplayer, 1 single-player. Queued as a document
   correction, no code.

10. **The failure-roll document correction** (C-23, ruled 2026-10-12): the document names `EffectiveSkill`; the
    measurement shows conditions already reach the odds through `WorkFactors`, and the change that looks like
    fixing the code would weaken them 8.3x and double-count the deficit. Queued as a document correction, no code.
