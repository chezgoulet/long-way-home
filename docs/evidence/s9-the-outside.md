# Evidence: S9 — the outside, first slice

Date: 2026-10-06. Gate S9 of `docs/ship-programme.md`. This slice is the rules, in the ship core,
evidenced by `tests/ship`. In the game it is reachable from the console (`ship chart`,
`ship jump <beacon>`, `ship fire`).

## The rules

- **A sector of twelve beacons**, its shape and contents fixed by the ship's seed: a chain that can
  always be crossed, with some shortcuts. Each beacon is empty, holds a hostile ship, a derelict to
  salvage, or the Borg.
- **A jump** goes one link, needs the warp drive delivering at least half its output, and burns
  deuterium and antimatter. A ship cannot jump out of a fight it cannot outrun — it is the same test.
- **A fight is the ship's systems against theirs.** What the phasers deliver strips the enemy's
  shields and then holes its hull. A torpedo is spent against whatever shields remain, and what is
  left of it reaches the hull. Our own shields recharge by what the shield system delivers — faster
  than an ordinary raider strips them, slower than the Borg do — and hold nothing without it.
- **What gets through lands**: on a deck's hull and on the systems stationed there, a different
  deck each time, in an order fixed by the count of hits so far.
- **With our shields down the enemy sends its party across**, once, to Main Engineering — boarders
  from a raider, drones from the Borg — and S7's and S8's rules take it from there.
- **A derelict** yields spare parts, once.

## What the tests establish

| property | test |
|---|---|
| The sector can always be crossed; links run both ways; the same seed is the same sector and another seed is another | the sector |
| A jump goes only along a link, costs fuel, and is refused with a crippled drive | the sector |
| A derelict gives 25 parts on the first visit and none on the second | the sector |
| At condition green we do the enemy no harm, our shields are down, their fire damages the ship, and their boarders are in Main Engineering | ship to ship |
| At red alert their shields fall and ours come up; a torpedo is spent; within twenty minutes the enemy is destroyed and the firing stops | ship to ship |
| With the phasers wrecked and no parts, half an hour does the enemy no harm; a torpedo is absorbed by its shields; with the launchers wrecked none can be fired | ship to ship |
| The Borg send drones: Main Engineering is marked Borg | ship to ship |
| Saved mid-fight, the ship restores and continues identically | ship to ship |

This is where the gates join up. In the first row nothing was arranged by the test beyond jumping
to the beacon: the damage, the casualties that follow from it, and the boarding fight in
Engineering are S6 and S7 running because S9 gave them something to do.

## What S9 still needs

- **Anything to see or hear**: a viewscreen, the ship shaking, an alert klaxon, a sector map on the
  Conn console, weapons control on Tactical. The consoles of S4 are where this is operated from;
  today it is console commands.
- **An opponent with systems of its own** (targetable shields, weapons, engines) rather than three
  numbers; more than one kind; more than one at a time.
- **Choices at a beacon** — hail, trade, run, answer a distress call — and the pressure director the
  design calls for: pursuit, so that the ship cannot simply sit and repair.
- **More sectors**, and an end to reach.
- Every rate is invented and in the lore ledger.
