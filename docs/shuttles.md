# Shuttles: not flown, but spent

The owner's model, 2026-10-05: shuttles are supported, **not pilotable**. Launching one is a load screen --
the crew are somewhere else -- and what the game tracks is whether a given shuttle **is or is not in the
shuttle bay**. That is one boolean, and this document is about how much it carries.

## The record

Per shuttle, in the ship model:

- `id` and `name` -- the crew name them (canon: the Delta Flyer).
- `class` -- Class 2, Type 6, Type 8, the Aeroshuttle, the Delta Flyer. Canon places the first three aboard,
  the Aeroshuttle integrated under the hull, and the Flyer built by the crew themselves.
- `location` -- **in the bay**, **away** (with the chart row it went to), or **lost**.
- `status` -- ready, damaged, fuelled, in maintenance.
- `crew_manifest` -- who is aboard while away, by crew record.
- `cargo` -- what was loaded out of stores, and therefore what is not aboard the ship.
- `away_since` -- ship time, because their posts are empty while this is set.
- `comms` -- linked, intermittent, or lost. Interference is normal, not exceptional.
- `return_condition` -- a transporter window (canon range: 40,000 km standard, 10 km emergency), a
  rendezvous, or *the shuttle comes back on its own*.

## Why that boolean is not a boolean

The owner's line is exactly right, and the reason is that many things read it:

- **Abandon ship**: the shuttle is a lifeboat as well as transport. How many are in the bay decides how many
  people can leave, and whether the number adds up.
- **Boarding**: a shuttle is a way off the ship. An intruder with a bay is an intruder who can leave with
  your shuttle -- which is precisely what raiders and scavengers would want to do, and why the shuttlebay is
  a defended compartment rather than a garage.
- **Damage**: a hit on the shuttlebay destroys what is parked in it. Emptying the bay before a fight is a
  real, canon-shaped decision.
- **Away missions**: the shuttle is the alternative to a transporter window -- the option that works when
  interference blocks beaming, and the option that can be *stranded* when the window closes.
- **The job queue**: a shuttle that comes back damaged is repair work; a shuttle that is lost is a **build**
  job -- canon has the crew constructing replacements in the second shuttlebay, which is a larger, aft bay
  built for construction and repair. Losing a shuttle costs materials and crew hours, not just a line of
  dialogue.

## The lifecycle

1. **Want to go.** A chart row, a resource, a distress call, a site. The mission needs a way there and back.
2. **The load screen -- and it should make you decide something.** While the crew walk in, the screen shows
   the manifest (who is going, and therefore which posts go empty), the loadout (what comes out of stores),
   the destination with its **chart confidence** (so you can see whether you are sending people somewhere you
   barely scanned), the return condition, and the risk. Canon-flavoured: a pre-flight check, not a menu.
3. **Away.** The game moves them; the ship keeps track. Crew records read `away`, the posts are visibly
   empty, comms may drop, and the shuttle is a pending item on the board. **Transit takes ship time**, which
   is the tension a non-pilotable shuttle still has: you commit people and equipment into a window you cannot
   watch.
4. **Return, three ways.** The shuttle comes home; the crew beam back and *leave the shuttle* (a real
   option, canon-shaped, and it costs you a shuttle); or it does not come back -- crashed, captured, destroyed,
   gone.
5. **Absence.** While it is away, the bay is short a shuttle, the lifeboat arithmetic is worse, and if the
   ship has to leave, the crew aboard are left behind or the ship waits. **Waiting is a cost**, and it is the
   cost the game should make you feel.
6. **Replacement.** A build job: materials, crew hours, time in the second shuttlebay, and a new name.

## Canon anchors

Shuttlebay 1 is on deck 10 (aft dorsal, L-shaped, with an arresting field); shuttlebay 2 is larger, aft, built
for construction and repair, depressurizable and launch-capable. Standard complement: Class 2, Type 6 and Type
8 shuttles plus the runabout-sized Aeroshuttle integrated under the hull. The Delta Flyer (2375) was built by
the crew: tetraburnium alloy hull, retractable nacelles, Borg-derived systems. Shuttles are used when
transporters fail or cannot penetrate an atmosphere; shuttlebays are variable gravity and force fields can
stand in for open space doors. All of that is sourced in `docs/research/voyager-interior-systems.md`.

## Acceptance

- The bay's contents are visible and correct after save and load, including a shuttle away and its manifest.
- Losing a shuttle is permanent and produces a build job, not a respawn.
- An away shuttle shows up as empty posts, and the crew records reflect where the people are.
- A shuttle left behind is recorded as a loss with a location -- it should be findable later, or at least
  remembered.
- And the one the owner asked for: at any moment, the ship knows whether a given shuttle is in the bay.
