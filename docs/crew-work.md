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

## Where the crew-hours come from

Crew exist in three states: **on watch** at a post, **off watch** resting or living, and **assigned** to
a job. Assigning is therefore a real trade with three costs, all of which the model already supports:
the post empties; the person accumulates fatigue; and the people who notice -- a department head, a
friend, the captain -- form the memory that later dialogue and drama run on.

The cleanest expression: **a job queue always longer than the crew can serve.** A queue that can be
cleared is decoration; a queue that cannot be dented is despair. The target is permanently behind,
always triaged, never hopeless -- which is precisely the attrition the programme is for.

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
