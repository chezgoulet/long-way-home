# The crew's work: maintenance, repair, build, reclamation

The owner's framing, 2026-10-05: **maintenance, repair and building are supported and needed crew
activities**, and de-assimilating the ship is another task of the same kind. This is the economy that
makes all of it one system -- and the thing that gives the crew model a reason to exist beyond holding
posts.

## The shape

**The ship produces a queue of jobs. The crew are the resource. Posts are the cost.** Every job needs
crew-hours; crew-hours come from people who are otherwise standing at a post, sleeping, or resting; so
every job accepted leaves a station empty and a person tired. Nothing else in the design ties the crew
model to the ship model this tightly, and it is what makes "who do I wake up" a system rather than a
line of dialogue.

## What a job is

In data:

- **kind** -- maintenance, repair, build, reclaim.
- **location** -- the compartment and the system it serves, so damage to a place is work in a place.
- **required skill or post** -- engineering, medical, science, security. Some jobs need a specific
  person, which is how absence becomes a cost rather than an input.
- **crew-hours** -- the honest unit of effort, not wall-clock. Two engineers halve it; a tired engineer
  does not.
- **materials** -- consumed from stores (below).
- **priority** -- set by whoever holds authority (below).
- **blocking** -- what it prevents while undone. This is how a deferred job becomes a future crisis.
- **state** -- queued, assigned, in progress, done, abandoned, failed.

## The four kinds

1. **Maintenance.** Small, recurring, preventive, and the easiest thing to skip. Canon runs on this: gel
   packs running warm, conduits, coolant, manifolds, a deuterium tank that only needs reading. Nobody
   dies from the job you did. *Skipping maintenance is how the ship slowly becomes the Year of Hell* --
   the failures later in a run should be traceable to jobs deferred earlier, and the log should be able
   to say so.
2. **Repair.** Restorative, from damage. Returns a system or a deck toward nominal, at the cost of
   materials and time. Repairing under fire is a different job from repairing in dry dock: it needs
   someone, it takes longer, and it can be interrupted.
3. **Build.** Net-new capability: a hull patch, a fabricated part, a replacement, a modification. Canon's
   precedents are plentiful -- the ship fabricated parts, built the Delta Flyer, Kes created the airponics
   bay to stop the replicators eating the power budget, nanoprobe warheads were made rather than found.
   Build jobs are the answer to *there is no resupply*: the crew make what they cannot buy.
4. **Reclamation.** Stripping Borg modification -- the work the owner described. Long, unpleasant, may
   fail, and **leaves residue**: canon's warning here is that a Borg thing which survives is how the
   threat comes back later (a vinculum outlived the cube it was cut from and infected Seven years on).
   So a reclaimed compartment is usable and not the same, and a *half*-reclaimed one is a future
   scenario.

## The code's seven kinds, and the ruling on the seventh

The four above are the *design's* vocabulary. The code's enum carries **seven** (`JobKind`,
`module/ship/ship_core.h`): `JOB_REPAIR`, `JOB_SEAL`, `JOB_RECLAIM`, `JOB_BUILD`, `JOB_SECURE`,
`JOB_EVACUATE` and `JOB_MAINTENANCE`. Seal, secure and evacuate are the order-created kinds
(`docs/orders.md`): an order to hold a deck or clear one is work with a face in the same queue. The
seventh was ruled by the owner, 2026-10-12, and is recorded here as ours (register row **3.23**,
`docs/designs-against-code.md`): **"the code's six stay, and maintenance joins them."**

**Preventive upkeep is its own job, not a missing word.** Repair is restorative -- it acts on a system's
**damage** (`1 - health`) and is dispatched by severity (the tiers below). The seventh kind is
preventive: it acts on a system's **wear**, the deferred-maintenance clock `module/ship/` already
carries (`System.wear`; a station left undermanned climbs it, and `MAINTENANCE_DAYS` of that fails the
system). Maintenance keeps a thing from breaking; repair fixes what is broken.

**The axis, and the number at which it is worth doing.** The axis is **wear** -- 0 fresh, 1 the failure.
A system at or past **`MAINTENANCE_DUE = 0.20`**, a fifth of the way along the clock, carries a
maintenance job; below it there is nothing to do, so the queue is not decoration. It is *not* the
system's damage (a worn system is whole) and *not* its criticality (that is the queue's own order:
`priority` and `orderRepairFirst`).

**What it costs, and what it buys.** The job takes crew time like any other: the same standing party
that answers a repair works it, up to **`MAINTENANCE_TEAM_MAX = 2`** hands, and it is paid for in a
little material -- **`MAINTENANCE_PARTS_PER_SYSTEM = 2`** spare parts to restore a wholly worn system --
so a ship out of spare parts cannot keep itself up. What it buys is a lower chance of the failure it
prevents, and the two connect by the clock's own rule run backwards: an undermanned station climbs the
clock at `1 / MAINTENANCE_DAYS` per day, so one engineer working as long --
**`MAINTENANCE_HOURS_PER_SYSTEM = 240 h`** -- restores a fully worn system. **`MaintenanceOdds(wear)`**
is that clock's daily hazard, `1 / ((1 - wear) * MAINTENANCE_DAYS)`: a tenth of a chance a day fresh,
rising to certainty as wear approaches 1. A job that lowers the wear lowers this number with it, and the
`work ordered` log line names both. **A maintenance that does not move the clock is a job-shaped
decoration**, and the guard that catches it is `scripts/maintenance-check.sh`.

**Who may order it, and where it shows.** The same authority surface the other kinds use, and no new
one: `ORDER_MAINTAIN` is a class in the register (`module/ship/order_set.def`), derived per recipient
from post, department and rank, and dispatched like any other order -- the post-holder or engineer is
offered "overhaul the {post}", resolving to the worn system. The officer of the deck needs no special
case, because the deck's authority is a property of the giver and not of the class (C-11), so a
maintenance order from the deck is permitted exactly as a repair order is; command's own hook is
`OrderMaintain` (the console `ship order maintain <system>`), which rides the job rather than a standing
field: a system with no wear left has nothing to maintain. The kind has a face like the rest --
`JobKindName` reads "maintenance", and the console `ship jobs` and the board name its target as a
repair's is named. The wear clock is saved (version 64), so a system run down, and the job it raised,
replay.

## Where the crew-hours come from

Crew exist in three states: **on watch** at a post, **off watch** resting or living, and **assigned** to
a job. Assigning is therefore a real trade with three costs, all of which the model already supports:
the post empties; the person accumulates fatigue; and the people who notice -- a department head, a
friend, the captain -- form the memory that later dialogue and drama run on.

The cleanest expression: **a job queue always longer than the crew can serve.** A queue that can be
cleared is decoration; a queue that cannot be dented is despair. The target is permanently behind,
always triaged, never hopeless -- which is precisely the attrition the programme is for.

## The damage-control party, and its three tiers

The owner's ruling, 2026-10-12, in his own words: *"There should be standing repair team ready to
dispatch, and more egregious repairs will take more people, which can take them off of their
stations."* It is the sentence above -- *assigning a repair empties a station* -- made real, and it
is **three tiers, not one**:

1. **A standing party, ready to dispatch.** The crew who are already the damage-control organisation:
   the engineers on duty with no station of their own, and the hands the battle bill sends to damage
   control. A repair is dispatched to it (`OrderRepairFirst`, the queue's own order) and it answers.
   The queue's priority rules are unchanged; this adds a party, not a replacement for the queue.
2. **Severity scales the party.** The axis is the system's **damage** -- the fraction of it destroyed,
   `1 - health` -- not its criticality (the queue's own order) and not the parts it costs (the
   budget). A job up to half gone takes the party as it stands (three hands); past it the party
   scales to six for a wholly destroyed system.
3. **Past the threshold, a large job draws post-holders off their stations.** The threshold is
   **more than half the system gone**. The hands the standing party cannot give come off the watch --
   the owning department's first, then any department, because damage control "pulls from every
   department" (`docs/crew-manifest.md`) -- and a post left short is the cost: the station delivers
   less and, kept short, it wears. This is the link the whole document is about: accepting a job is
   accepting an empty station, and a repair names its people in the roster's nine.

## Materials

Replicator reserves (rations as energy, canon-style), cargo stock, and salvage from what the ship finds
or kills. Build and repair consume them; reclamation sometimes *returns* them, which is a rare and
welcome upside. Scarcity is the point: the number of hull patches aboard is a number, and so is the
number of torpedoes.

## Priority is where rank lives

This is the mechanism that connects the rank-and-role design to the ship: **department heads queue their
department's work; the captain sets ship priority.** So the same job list reads differently by post --
the engineer sees eleven jobs and knows which three matter; the captain sees the whole ship and knows
which department starves this watch. A junior assigned the job they think is wrong is one of the best
positions in the game, and its escalation channel is the report nobody acted on.

## What to measure

Backlog by age and kind; crew-hours per kind per watch; mean time from damage to repair; hours a post sat
vacant because its holder was on a job; jobs failed; and deferred maintenance outstanding at the time of
each failure. If a later failure cannot be traced to an earlier deferred job, the loop is not closed.

## Anti-goals

- A job queue the player never looks at is a decoration tax. The queue needs a face: a board, a
  department head who tells you, a console that is honest.
- Infinite crew-hours would delete the drama; so would jobs that take forty minutes of real time to
  finish. Crew-hours are ship-time, compressed, and tuned once.
- Work that never fails is not work. Some jobs should fail, and the failure should be visible before it
  happens to anyone paying attention.
