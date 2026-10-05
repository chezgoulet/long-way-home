# The Borg incursion: taking, holding, and reclaiming the ship

Owner's framing, 2026-10-05: the Borg board and assimilate ship and crew; the crew then have to
**de-assimilate** what the Borg changed, and the fight to retake strategic parts of the ship becomes a
Battlefront-like contest of places. This records the shape, the canon behind it, the one field the ship
model needs for it, and the two traps that would spoil it.

## Canon gives us the whole arc already

- **Boarding** happens after shields fail: tractor lock, shield defeat, then a cutting beam, transported
  drones, or both. So the incursion is a *consequence* of a defensive failure -- never a scripted ambush.
- **Drones ignore intruders** on their own decks unless interfered with. The Borg are not a patrol; they
  are an infection with a purpose, and that is more frightening.
- **A taken ship is stripped for parts** and, if possible, towed into a hangar. Capture is the real loss
  condition, not destruction.
- **Ships regenerate from damage and self-destruct when critically compromised** -- which is the shape of
  the counter-play: push them past a threshold, or they cannot be pushed.

## The one field the ship model needs

Everything below hangs on a single addition, and it should land *before* the incursion code does:

**Per compartment: `controller`.** One of `crew`, `borg`, `contested`, `sealed`, `uninhabitable`. The
Borg pressure system writes it; the crew's actions write it; the deck adapter reads it on load to decide
what the player walks into. Infection spread, retaking, sealing and scuttling are then all operations on
one field rather than a set of special cases. Build this first and the incursion is mostly data.

## What the Borg gain from each place they take

Strategic places are strategic because of what holding them *gives* the Borg. This is the raid map:

- **Main engineering and the warp core (deck 11)** -- power, and the loss condition: with the core they
  can breach it, eject it, or turn the ship's own energy against it.
- **The computer core and gel-pack grid** -- coordination, and **the crew roster**. This is the
  terrifying one: if they take the computer, they inherit what the ship knows, *including where
  everybody is*. Our own "know where everybody is" model becomes their intelligence. Nothing else in the
  design makes the ship's memory feel as dangerous as this.
- **Sickbay (deck 5)** -- bodies. Assimilating the wounded converts our own casualties into their
  reinforcements, which is the cleanest reason in canon to defend a medical bay.
- **The transporter rooms (deck 4)** -- mobility. With them, drones move between decks without using the
  corridors, which is how they bypass every chokepoint the crew sets up.
- **The shuttlebay (deck 10)** -- our way off and their way in. Losing it removes the option to leave.
- **Sensors and astrometrics (deck 8)** -- they see what we see, so hiding stops working.
- **Environmental control (deck 12)** -- atmosphere. Canon precedent for decompression as a weapon: with
  this, the Borg can empty *our* compartments rather than merely occupy them.
- **A vinculum aboard** -- if they establish one, they coordinate and adapt faster. Canon: every Borg
  vessel has one; destroying it severs local coordination, **but severed drones keep executing secondary
  objectives**. So a vinculum raid is a reprieve with a cost, never a win condition.

## Growth, not respawn

Battlefront's *feel* transfers: sector-by-sector fighting over places that matter, with chokepoints and
consequences. Its **economy must not.** There are no Borg spawn points and no reinforcement tickets. The
Borg instead **grow from what they hold**: a captured sickbay produces drones from our wounded, a
captured engineering feeds their systems, a captured transporter moves them. Retaking a section is
permanent -- the Borg do not take it back by spawning, only by spending what they have left. That is how
a Battlefront-like contest survives a game with no reset button.

## De-assimilation: possible, and never complete

This is the best part of the owner's idea and canon is unusually generous with it:

- Reversal is possible only in a **narrow window** -- nanoprobes are counterable in the first minutes.
- Recovered people are **never fully restored**: Picard's trauma and alterations lingered for decades;
  Seven of Nine was never fully restored after twenty-four years.
- 80,000 severed drones removed "most, but not all" implants and became their own society.
- A vinculum that survived destruction later infected Seven.

So the rule for us: **you can reclaim a deck, and the deck is not the same afterwards.** Borg
modifications leave permanent residue -- a conduit that runs hot, alcoves that cannot be fully stripped,
a bulkhead that was cut open and plated over. And a crew member who was assimilated and recovered carries
it in their record forever, which the crew model already supports. The reclaimed ship is a **Changed
ship**, which is exactly the north star: no reset, only consequence.

## The counter-play kit, and the pace trap

Canon gives the crew real tools: **force fields** (rated 1-10; a level-10 field can cut a drone from the
Collective), **explosive decompression** (canon: an entire cargo bay, with only Seven surviving by
clinging to a ridge), **weapon remodulation** (the phaser adapter's rotating modulation allowed at most
twelve shots before adaptation), **nanoprobe reversal** in the minutes after injection, and **the
vinculum raid** above.

**The trap: adaptation cannot be absolute.** Canon's Borg nullify nearly any energy weapon within
minutes and adapt collective-wide, so a literal implementation ends every firefight before it starts and
the game becomes hide-and-seek. Therefore adaptation must be (a) **per weapon type**, (b) **counterable
by remodulation** at a cost in time and attention, and (c) **tied to a resource the crew can attack** --
killing a vinculum should buy back the crew's weapons for a while. Without those three, a
Battlefront-like fight is not merely hard, it is impossible: you cannot have a firefight with an enemy
who is immune to fire.

**The second trap: the fight has to be winnable at the level of places.** Retaking needs to be possible
with the crew we have, which means the crew are **defenders at posts first and a squad second**. Our crew
layer positions people and holds stations; it does not yet command a fireteam. So sequence it: security
holds a junction and engineers seal a hatch *before* anyone expects the crew to advance room by room.

## The loss condition, and it must be legible

Borg reach the core: the ship is taken, or scuttled by her own crew. The player must be able to *see*
that path -- decks lost, systems taken, the countdown -- because a loss nobody saw coming is not
attrition, it is a bug report. The time-to-organise clock in `docs/ship-systems.md` is the visible
instrument; this document is what the clock is counting toward.

## Sequencing

Needs, in order: the compartment model and `controller`; power, doors, fields and life support (tier 1);
weapons and shields (tier 2); damage and the repair economy (tier 3) -- because a Borg incursion is only
frightening if the ship can be hurt and the crew can be lost; then the incursion itself. Building it
before tier 3 gives a threat that cannot wound and a crew that cannot be spent.

## Open questions for the owner

- Can the player be **assimilated** -- and if so, is that an ending, or a window in which someone else
  can save them?
- Does an incursion ever **end well**? Canon says the cube is destroyed or departs; our north star says
  the cost stays.
- How long is the **reversal window** in ship time -- minutes, as canon implies, or longer so that a
  player can realistically reach the person in time?
