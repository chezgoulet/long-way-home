# The navigation counter: how far, and how long

The owner's requirement: **a persistent counter in the ship's computer showing how far home is and the
estimated time to get there.** It is the ship's scoreboard, the crew's shared fact, and the number that
makes every other decision legible. Because it costs nothing to compute and is the emotional centre of the
whole design, it should be built early.

## What it shows

Two numbers, and a third that matters most.

1. **Distance to Earth** -- light years remaining, in the same unit as the goal.
2. **Estimated time** -- and this is where care is needed. It is **not** an arithmetic quotient. It is a
   projection from *current capability*: distance remaining divided by the effective speed the ship can
   actually sustain, given crystal integrity, engine health, crew, and the resupply the route offers.
3. **The change since the last log entry.** The derivative is the story. A month in which the estimate went
   *up* is a month the crew lost ground, and that single arrow carries more drama than any crisis.

Show both the **nominal** figure and the **current-capability** figure. Canon's own numbers make the gap
meaningful: 75,000 light years at warp 6.2 is about **75 years**, and the effective projected rate is on the
order of **1,000 c**. So a ship running its crystal down honestly reads *71 years nominal, 76 years at
current capability* -- and the crew can see exactly what the difference is made of.

## Worked example

- Charted and covered so far: 3,760 ly, so **71,240 ly remaining**.
- Nominal capability (warp 6.2, healthy crystal): **~71 years**.
- Current capability (crystal at 62%, one engine overhaul pending, no refuelling site charted): **~76 years**.
- Since the last log entry: **+1.8 years.**

The player now understands a month in one glance, and nothing had to be explained to them.

## Where it lives

- **Astrometrics (deck 8)** -- the wall from which the numbers are read aloud, and the display of the route
  itself. Canon built this room for exactly this.
- **The bridge** -- the standing readout, because the officer on watch is the one who has to say the number
  out loud.
- **The captain's ready room** -- the same numbers, plus the forecasts, which is where the difference between
  ranks lives (below).
- **Any crew console**, by query -- canon's ship's computer answers questions, and the answer should be the
  same on every deck. A crew member can ask. That is how the number becomes *shared* rather than displayed.
- **The ship's log** -- the counter is *recorded* periodically, so the crew can look back: *when we crossed
  that expanse we were sixty-one years out.* That is how a number becomes a memory, and memory is what the
  crew model is for.

## Who sees what -- the rank layer

- **Everyone** sees the distance and the current estimate. It is the crew's fact; hiding it would be cruel
  and would make the world smaller.
- **Command** sees the *forecasts*: the estimate under each available course -- take the anomaly route and it
  is fifty-four years if it works, ninety if it does not. That is the difference between knowing where you
  are and deciding where to go, and it is exactly the line the rank design draws.

## Implementation

It is a **read**, not a system: a pure function over ship state -- distance remaining, crystal integrity,
engine health, route hazards, charted resupply -- plus one small write, the periodic log entry. That makes it
cheap, and it makes it honest: it cannot drift from the simulation, because it *is* the simulation, read out.

Hang it on the panel mechanism the module already has (`module/ui/`) and expose it to the console as a query
so the same number can be printed anywhere.

## The one rule that must not be broken

**The number must never lie.** If the estimate assumes ideal conditions, or ignores a detour the crew
already know about, the drama dies -- the counter becomes decoration and the crew stop believing it. It
should be honest *and conditional*: this is what the journey costs **if nothing changes**, and here is what it
costs at the capability we actually have.

## Acceptance

- It changes when the state changes: wreck the crystal, and the estimate worsens on the spot.
- It survives save and load, and it is recorded in the log over time.
- It is visible without entering a special mode.
- It is readable on every deck through a console query.
- The **nominal vs current** gap is displayed, not hidden.
- And the human check: a player who looks at it after a hard month should feel the arrow move.
