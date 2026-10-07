# Damage, and the two budgets

The owner's addition, 2026-10-05: the ship takes damage and **stays damaged until fixed**, so operating her is
a matter of budgets -- an **energy budget** and a **functionality budget** -- and damage *escalates*, with an
external threat able to do enough of it that the chain leading to a warp core breach forms on its own.

The **sizes** of those budgets -- what the plant supplies, what each system demands, how the two meet as the
dilithium crystal ages, and what the ship gives up first -- are set out in `docs/budget-squaring.md`.

## Two budgets, and they are not the same thing

- **Energy**: what the ship can *feed*. Allocated across systems through EPS; finite. **Who sets the
  allocation is `docs/power-assignment.md`** (owner's ruling, 2026-10-07): the players and the crew decide each
  system's online state and share, nothing sheds itself, and an over-committed plant is reported as a shortfall.
  The brownout ladder in `docs/ship-systems.md` is the chief engineer's recommendation and automatic mode's
  policy, not the mechanism. The three independent sources (life support, holodecks, shuttlecraft) that survive
  the main grid going down remain a gap, as noted there.
- **Functionality**: what the ship can *do*. Integrity per system per compartment: how many phaser arrays are
  still working, which doors open, whether the sensors can see, whether a deck still holds air.

The distinction is the whole design. Power can be restored in a minute by rerouting; functionality comes back
only through work. A ship can be fully powered and barely functional -- which is exactly what Voyager looks
like after a bad month.

## Damage persists, and it is recorded as capability

Every hit writes to functionality, and the write is **a permanent change to what the ship has** until a repair
completes: a destroyed array is a firing arc lost for the next three engagements; a hull breach is a
compartment sealed off; a burnt gel pack is replicators offline; nineteen severed relays are no turbolifts.
The crew therefore operate not a health bar but an **inventory of what still works**, which is a far better
thing to look at when deciding what to do next.

## Cascades: how a core breach actually happens

The rule from `docs/failure-is-content.md` is that the warp core breach must be reachable only by a *chain* of
decisions, never a single roll. Damage is what builds that chain -- and this is the mechanism:

```
Borg cube lands a hit
   -> coolant system damaged            (functionality: coolant loops)
   -> coolant loss, core temperature rising   (warning: minutes, not seconds)
   -> injector assembly stressed        (functionality: M/ARA injectors)
   -> containment field destabilising   (warning: containment %, falling)
   -> BREACH IMMINENT                   (countdown)
   -> eject the core, or lose the ship
```

**Every arrow is an interrupt point.** A damage-control party seals the coolant loop. Engineering bypasses the
injectors. The captain orders the core shut down and finishes the fight on impulse. Or the core is ejected --
which per canon is authorised, unreliable, and recoverable, at the cost of warp until a new one is found, which
costs the journey years. Damage cascades make the failure rule playable rather than fatalistic: the crew watch
the chain form and get four chances to break it.

## Damage control is the third kind of work

The crew economy in `docs/crew-work.md` already has maintenance, repair, build and reclamation. Damage
control is the emergency form of repair: **under fire, with no preparation, competing for the same people**.
It is the highest-priority job on the board and it empties posts to fill -- the security officer holding a
corridor is the person you might need in the coolant bay instead. That trade is the game.

## Abandonment is a decision, and it is the attrition UI

Because there is never enough crew to fix everything, the player must choose what to **write off**: a deck
sealed and left, a system stripped for parts, a compartment marked uninhabitable. Canon's Year of Hell is
exactly this -- seven decks uninhabitable, and the ship kept flying. So the ship carries a written list of
what she has given up, and the player can walk past the sealed hatch and remember why. **That list is the
attrition north star expressed as a user interface.**

## What combat means here

Not "reduce the enemy to zero". It is **holding the ship together long enough** -- to break off, to drive them
off, to survive the interval -- while the cascade tries to form and the crew spend themselves stopping it.
Winning is often leaving with everything still working, and the internal argument afterwards is about what
should have been shut down earlier.

## Decision: how allocation handles time (2026-10-05, owner)

**Single player offers a planning pause; multiplayer runs live.** Resolved in favour of the recommendation,
and it will not be revisited mid-implementation.

Two consequences worth writing down, because they shape the console rather than just the clock:

1. **The pause is relief, not a crutch.** Multiplayer cannot pause -- one player stopping the world would stop
   it for everyone on the ship -- so the damage-control board must be **operable in real time**: legible at a
   glance, allocatable in a few inputs, no nested screens. Design it live-first and offer the pause as the
   single-player courtesy it is. A board that only works paused would break the multiplayer mode the whole
   programme is built toward.
2. **Pausing must not become an exploit.** Allocation, inspection and planning are allowed while paused;
   **nothing completes** while paused. Repair orders, damage-control parties and system restarts are *queued*
   and execute on resume, in the same order the crew would have carried them out. Otherwise the pause is free
   labour, and the crew-hours economy in `docs/crew-work.md` stops meaning anything.

## Acceptance

- Energy and functionality are separate quantities, and both are visible on the damage-control board.
- A single engagement can produce a multi-step cascade, and the crew can interrupt it at more than one point.
- Damage from that engagement persists across a deck change, a save, and a reload -- and the log can narrate
  the chain afterwards, action by action.
- Every written-off compartment and system appears in the ship's own list of what she has given up.
- And the human check: after a bad fight, a player should be able to say what they chose to lose.
