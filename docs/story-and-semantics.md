# Story and semantics: what this game means, and how it should feel

The reading under everything else. `docs/design-creed.md` says what the programme believes and
`docs/design-north-star.md` says what it is for; this document says what the game **means** — the sense an
author needs before writing a scenario, a line of dialogue or a system, so that the parts pull the same way
instead of each being defensible on its own.

Written 2026-10-06 in one session with the owner, against the corpus as it stands on `testing` and
`feature/g3-reactive-crew`. Where a mechanic and a meaning disagree, the disagreement is **named** in
*Where a mechanic and a meaning pull apart* rather than smoothed over.

## The thesis, in one breath

**A tired ship, and a group of people who have decided to keep going anyway.**

Minute to minute the player is **on watch**, and the verb is not *fight* — it is **assign**. A finite number
of rested, willing people are set against a longer list of things that need doing, and the ship's own record
is the only scoreboard. The question is rarely *how do I beat this*; it is **who do I wake up**
(`docs/scenario-atlas.md`).

What the player is responsible for is not the ship. It is **the people they put in the places, and the
consequences of deciding which ones.** The captain cannot fix anything — only decide who goes and what they
leave behind — and the thing this document adds is that this is true at *every* rank. The instrument is always
people and priorities; only the size of the list changes.

## The five things the game has to mean

### A win

Wins are possible; clean wins are not. A win is not the enemy reduced to zero — combat here is *holding the
ship together long enough*, and winning is often leaving with everything still working
(`docs/damage-and-budgets.md`).

**The test of a win is whether the player would sign the log entry.** The month is written up, and the
acceptance criterion is human and single: *would you put your name to this.* That converts a win from an
outcome into a verdict the player writes about themselves, which is what an attrition game needs.

**A clean month is possible**, and must be, or good leadership and good play would not be a factor. It is
possible because it is *small* — survivable, and expensive to sustain. On a long enough timeline the ship
grinds its resources, its capabilities and its people down. A difficulty setting sets the **weather**, never
the physics: pressure rates and starting condition, not repair forgiveness, wear rates or morale decay. If
forgiveness varied between runs, two players' logs would stop being comparable, and the whole idea of a
record rests on them being comparable.

### Damage

Not a health bar. Damage is an **inventory of what still works** — four of seven phaser arrays, eighteen of
nineteen relays, forty-one gel packs and no way to make more — because an inventory is a far better thing to
look at when deciding what to do next than a percentage is.

To which this adds the part that makes it semantics rather than bookkeeping: **damage is legible biography.**
The phaser array's plate cannot be read without reading the fight that took it.

### Abandonment

The crew economy never has enough people, so the player must choose what to **write off**: a deck sealed and
left, a system stripped for parts, a compartment marked uninhabitable. Canon's Year of Hell is exactly this —
seven decks uninhabitable and the ship kept flying.

**Abandonment is the player's signature.** It is the one place where the log is written in the player's own
hand, and the horror is not the sealed hatch. It is that everyone aboard remembers who made the call.

The ship therefore carries a **written list of what she has given up**, so the player can walk past the sealed
hatch and remember why. `docs/damage-and-budgets.md` calls that list the attrition north star expressed as a
user interface, and it is the strongest single sentence in the corpus. **It has no state behind it** — see the
first conflict below.

### A relationship

Not taste, and not a number. **A relationship is a claim on your future decisions**: a person who will go
because you asked, and a person who will not. Bonds fold out of the marks people took, and their meaning is
that they are the only thing that turns an order into a favour or a burden.

`docs/crew-roster.md` stores them bidirectionally because **blame is not symmetric**, which is the most human
line in the corpus. I would make the asymmetry *visible*: the player can believe a relationship is fine and be
wrong. You ask someone back into the coolant bay, they go, and the mark they take is not the one you think you
earned.

### The log

The log is a document, not an instrument, and it has its own document:
`docs/the-record-and-the-log.md`.

## The feel

**Attrition's quiet horror is arithmetic, not fire.** Ten minutes of it is standing at a console reading the
inventory and the estimate — seventy-one years nominal, seventy-six at current capability, up 1.8 since the
last entry (`docs/navigation-counter.md`). Nothing is on fire, and the number is honest. That is the register.

**Those numbers are the design's, not a readout.** The counter is specified in full and, as of this writing,
is not implemented — see conflict 7 below. Until it exists, that paragraph describes what the game is *for*,
not what a console prints.

**Procedure is the comfort.** The comfort is that there is a right way and it is written down. An ensign
reports *"Chief, we've lost the starboard array; the coolant loop is holding at sixty-two percent,"* and the
chief says *"Again — number first."* Form, held under pressure, by tired people: that is the emotional centre
of the middle of the game, and it is why the damage-control board must be legible at a glance and operable in
a few inputs, live-first, with no nested screens.

**Hope is a discipline with a cost, not a meter that fills.** The player spends on kindness when it is not
optimal — the night on the holodeck when the gel pack wants a hand, the honest repair time instead of the
comfortable one, the funeral when there is work to do. The simulation pays it back **late**, in the third
month, as someone volunteering who did not have to. Paid back immediately it would be a vending machine, and
there would be no discipline in it. `docs/morale.md` states the thesis the whole game runs on: morale is the
resource you spend to survive, and *how you spend it is how you treat people.*

**A good session, from the inside.** You come on at 0300. You read the inventory before the contacts. You let
a warm gel pack run one more shift. You tell a junior the truth and watch them file it away. You spend forty
minutes in a meeting about water. Nothing fires, and you leave able to name the three people you are worried
about.

**A bad session is not a defeat.** `docs/morale.md` names it: *the quiet one — nobody refuses, and the ship
simply stops doing more than it is told.* You were fast, you were correct, the log is full of orders and empty
of people, and you cannot tell from the numbers that you have lost anything.

## Where a mechanic and a meaning pull apart

Named, because a conflict that is written down can be decided and one that is smoothed over cannot.

**1. The abandonment list is the headline claim and it does not exist.** `docs/damage-and-budgets.md` says the
ship carries a written list of what she has given up and calls it the attrition north star as a user
interface. Verified on the committed tree of `feature/g3-reactive-crew`: there is no such list in
`module/ship/ship_core.h`, none in the save, and no version in the changelog that carries one. This is not a
new system competing with the five approved gaps — it is a **read** over decisions the simulation is already
making, needing a date, a compartment and a name. It is the cheapest meaningful thing on the board.

**2. Morale is specified as derived and stored as a scalar.** `docs/morale.md` is explicit: morale is per
person, derived from deficit, outlook and holdings, *never stored as a hidden counter*. `CrewMember` in
`ship_core.h` carries a single `float morale` ("0 broken, 0.5 going through the motions, 1 heart in it"). The
float is not a ship-wide scalar, so the letter of the rule holds, but the three components do not exist and
"the log can say why" cannot yet be answered. Either the components land or the derivations stop being
claimed.

**3. Allegiance does not move, so taste becomes typecasting.** `docs/affinities-and-allegiance.md` fixes
allegiance strength but not what changes it — which means a Maquis-weighted crew member reacts the same way in
year seven as in year one, and the fix for "one person with 141 histories" becomes a different determinism.
**Recommendation:** allegiance strength is itself a fold over marks, so the integration arc is a consequence
of play, is saveable, and is replayable.

**4. A duplicate of the dead is a resurrection — resolved.** `docs/failure-is-content.md` allows a transporter
mishap to produce a clone or a merge. The risk is that the transporter becomes the reset button the programme
exists to refuse. **Resolved by the owner (2026-10-06): the transporter is not exception-handled at all.**
Condition sets the odds (see `docs/failure-is-content.md`, *Condition sets the odds*), and a duplicate is
still not the person — a new record, an ambiguous status, and a crew who know. Fair, canonical, and the whole
accident family goes back into play.

**5. The record a meeting is briefed from.** `docs/staff-meetings.md` says the brief carries "what the ship and
the people look like right now," which reads as the record — and a meeting briefed from the record is a meeting
where everyone already knows the truth. Since the log may now be false
(`docs/the-record-and-the-log.md`), **the brief must be per-participant, built from that person's marks and
the log and nothing else.** This is better anyway: the meeting stops being a status update and becomes six
people in a room with six versions of one afternoon, one of whom is wrong on purpose.

**6. The pause: three answers, one meaning — resolved.** The settled list says no pause anywhere, all real
time; `docs/damage-and-budgets.md` granted single-player a planning pause; the engine has a menu that stops
`Ship_Frame` and a console that explicitly asks not to. **Resolved by the owner (2026-10-06):** the world's
clock cannot be stopped — but the simulation can be shut down, and the player can sleep through time at the
accelerated rate. See `docs/ship-model.md`, *The three clocks and the two exits*.

**7. The counter is the design's emotional centre, and the code had a fuel gauge wearing its name.**
`docs/navigation-counter.md` calls itself "the emotional centre of the whole design" and says it should be
built early, because it costs nothing to compute and makes every other decision legible. It specifies three
numbers: the distance to Earth; an estimate of the years home, **projected from the capability the ship can
actually sustain** rather than divided out arithmetically; and the change since the last log entry — with the
nominal and current-capability figures shown side by side, so the crew can see what the difference is made of.

**Status at the time of writing (2026-10-06, builder branch `67b5790`): it did not exist.** What existed was
`DilithiumRange` — `dilithium × crystalQuality × 3000 light years`, the progress the crystal can still buy —
and a function literally named `NavigationCounter` that returned it. A fuel gauge under the name of a journey,
which is worse than no name at all: a name that satisfies a search while returning a different quantity hides
the gap. A session was put on building the real one — **check `NavigationCounter` in
`module/ship/ship_core.cpp` and its evidence in `docs/evidence/` before relying on this entry.**

What makes it cheap, and the reason the previous session's "this needs a new model" was wrong: the position
model already exists and is saved. `Ship::sector` holds the beacons and the jumps between them, `Ship::beacon`
is "where the ship is", and `SECTOR_BEACONS = 12` with `SECTORS_TO_CROSS = 3 // sectors crossed before the
ship is home` give the distance remaining. The inputs to effective speed are saved too. **This is a new
projection over an existing model, not a new model.**

## The laws

Short, and meant to be cited when a decision is being argued.

1. **Instruments cannot lie; people can.** The counter, the inventory and the damage board are reads of state
   and are always honest. The log is a document and may be false. The gap between them is the game.
2. **The simulation writes the log and never reads it.** Nothing in the ship model may make a decision by
   consulting the log.
3. **A document can be destroyed; a memory can only be destroyed by destroying the person.** Therefore the
   only secure secret is one nobody knows.
4. **No destination is better than another.** Endings are readers, not verdicts. The core breach remains the
   only unwinnable end state.
5. **The record is authoritative and must be readable back as something the player would sign.**
6. **The cost is paid in the thing you were trying to keep.** Every mechanic above resolves this way: the
   purge costs the ending, the lie costs the crew, the shortcut costs the year.

## The one thing

If we got only one thing right: **the record is authoritative, and it must be readable back as something the
player would sign.** Every consequence writes into it, it replays identically, and the player can reconstruct
what happened and why, in their own words, without a status screen.

That single property is what makes attrition *mean* something instead of merely accumulating — and it tells us
exactly what to refuse:

- the **reset button**, because it erases the record;
- a **model that writes state**, because it makes the record unattributable;
- **unreproducible generation**, because a record you cannot replay is not a record;
- **synthesised lore-character voice**, because it puts an artifact in the record whose title we do not hold.

Four refusals, one principle: **nothing enters the record that we cannot answer for.**

Its human face is smaller and better: an empty post, and a name in the log, and the two of them together must
cost the player something they can name.

## What an author must declare

Beyond the contract in `docs/scenario-atlas.md` (breeding conditions, systems stressed, staff needed, cost to
answer, cost to ignore, residue, visibility, warmth) and the positions each scenario can be played from, every
piece of content declares:

- **what it writes to the record** — the state change, and therefore what the log will be able to say;
- **what it writes to memory** — which marks, with what source and valence, and for whom;
- **how the player reads it back** — the instrument or document where the consequence becomes visible,
  because a consequence the player cannot perceive did not happen;
- **which reader it is building toward**, where it touches the endgame (`docs/endings.md`);
- and **its warmth** — a scenario with no human beat is a hazard, not a story.

## Open

- **Allegiance drift** (conflict 3) needs a decision before affinities are authored.
- **The arrival's reader** — whether the record is audited on arrival is settled as *yes, by whoever the
  destination is* (`docs/endings.md`); whether the player may decline to file is recommended and not yet
  ruled.
- **The player's own body.** `docs/gap-analysis.md` lists it as absent, and the owner's ruling that degraded
  systems can spark at the console makes it load-bearing: a station that can hurt the player requires the
  player to be a record with a body.
