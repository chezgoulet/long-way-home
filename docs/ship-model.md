# The ship is a state model, not a place

Design note, 2026-10-05, prompted by the owner naming the requirement directly: the crew must know where
everybody is, carry state and memory, work the ship's systems together, and lose the ship to the Borg if
the Borg are left aboard long enough to organise.

The impasse it answers is real but was misread. Decks are separate maps, each map is a separate server
lifetime, and crew are server entities -- so "walking between decks" appeared impossible. It is not
impossible; it was only unowned. **Nothing in the program was responsible for the ship as a thing that
persists.** Maps were being treated as the ship. They are views of it.

## The mechanism (all shipped, all verified in `src/game/`)

- **A general keyed store inside the save.** The game module appends its own structures under four-character
  tags and reads them back: `gi.AppendToSaveGame('OBJT', client->sess.mission_objectives, sizeof(...))`,
  the same for `TACT` (tactical info), `LCKD` (player lock), `ROFF` (run-off strings). The ship model is
  another tagged blob. Nothing new is needed from the engine, and a level transition is a save and a load,
  so the blob crosses a deck change for free.
- **Deck links exist.** `target_level_change` carries a `mapname` and is documented as a HUB -- the retail
  mechanism for moving between linked maps. The turbolift already uses it.
- **Controls exist.** `func_usable` fires `G_UseTargets(self, activator)` when used, supports states
  (STARTOFF, AUTOANIMATE, ANIM_ONCE, ALWAYS_ON, BLOCKCHECK, PLAYER_USE), and takes damage through
  `func_usable_pain` / `func_usable_die`. A console is therefore already a control surface, and already a
  thing that can be wrecked.
- **Scripts already arbitrate with the world.** ICARUS sequences run above the behaviour states, so any
  system we model must yield to them in the same way the crew driver does.

## The model

One authoritative structure, owned by the game module (behind the module boundary), holding:

- **Deck registry.** Which decks exist, how they connect (the level-change graph), and which one the player
  is in.
- **Crew records.** One per crew member: identity, post, deck, compartment, duty, shift, current order,
  and the small set of memories worth keeping (who they have seen, what they know, flags set by events).
  Bounded per head -- the 256-byte target in the crew spec is a budget for exactly this record.
- **Systems.** Power, life support, doors, sensors, shields, weapons, engines, transport -- each with a
  current value, a target value, a health, and which compartment it lives in.
- **Compartments and damage.** Per **deck** — the code's compartment *is* a deck, and the owner ruled it so on 2026-10-08 (`docs/gates.md`, C4): hull/breach state, atmosphere, temperature, which systems
  it hosts. Damage events write here.
- **Threats.** Borg (and anything later) as pressure per compartment, not as a spawn list.
- **A clock.** Accumulated ship time. This is what lets the ship change while the player is elsewhere:
  advance the model by elapsed time on every load, then apply the result to the deck being entered.

## The contract with a deck

One adapter, two directions, and it is the only place map and model meet:

- **On load**: read the blob, then make the deck match it -- spawn the crew whose records say they are on
  this deck, suppress the deck's own copies of anyone recorded elsewhere, and apply damage and system
  state to the map's entities (dead consoles, open doors, darkness, breach effects).
- **On unload**: harvest the deck back into the blob -- each crew member's position, the state of every
  wired control, anything changed in the world -- and stamp the clock.

Crew crossing decks then becomes what it always should have been: a change to a `deck` field on a record.
Nobody is simulated walking between maps. They are *recorded* as being somewhere, and the deck that loads
spawns them accordingly. The lift version of coming and going is a special case of the same field.

## The three clocks, and the two exits

The owner's ruling, 2026-10-06. It maps onto the three clocks S10 already implements
(`module/ship/ship_core.h`: accelerated, real time, wall clock) almost word for word.

| mode | what it is | how it is reached |
|---|---|---|
| **accelerated** | a ship's day in compressed game time, only while playing | the player sleeps -- incrementally, or all at once |
| **real time** | a day is a day, only while playing | the normal state |
| **wall clock** | a day is a day, and the ship lives on while you are away | exit with a background process; the player is off duty until they return |

The fourth case is not a clock at all: **exit without a background process, and the simulation stops.** Both
exits are supported and the player chooses. The semantic line to hold is that **the world clock cannot be
stopped, and the simulation can be shut down** -- suspending the process does not put the world on hold, it
means no time existed. And the distinction is observable in the record: one run has continuous log entries and
the other has a gap in it. **A run could be marked by whether it was ever left standing.**

Fast-forward and played must converge -- a ship advanced in steps arrives where a played one would, which
`ship_core.cpp` already enforces by cutting long steps up -- so sleeping is never a way to skip a
consequence.

Two things this makes load-bearing:

- **Standing orders are what the ship does while the player sleeps** (save version 7). They stop being a
  pleasant system and become the mechanism by which an unattended night happens at all.
- **The log is the return surface.** The player comes back after eight hours and reads what the ship did
  without them (`docs/the-record-and-the-log.md`). That is the loop, and it is the strongest case yet for
  building the log early.

One guard, offered as a default the owner may overrule: if suspension is available in every mode, the player
suspends exactly the nights they would rather not have, and attrition stops biting. Tie it to the modes
already in the header -- **holodeck may suspend the world; ironman may not**, because ironman means the ship
keeps its own time.

## Borg, as the model expresses it

The owner's framing -- board, and if left long enough to organise, assimilate crew and ship -- is a
progression on pressure, and it falls straight out of the model:

1. **Boarding**: borg presence appears in a compartment.
2. **Organising**: presence grows while unchallenged; the clock advances it. This is the "left long
   enough" condition, and it is a number, not a story beat.
3. **Assimilating crew**: crew records in that compartment transition -- the person is replaced, their
   post is now held by a hostile, and the crew model loses them. This is why knowing where everybody is
   matters: assimilation is a state change on a record, not a scripted death.
4. **Taking the ship**: system by system, as pressure reaches the compartments that host them.
5. **Counter-play**: the crew (player and NPC) can fight it -- restoring systems, sealing decks, retaking
   compartments -- because all of it is state in one place, and the loss is reversible until it is not.

That is the attrition north star with no reset button: the ship accumulates damage and losses across the
whole run, and the save carries it.

## What this costs, honestly

Small and cheap, and worth doing before G3's code rather than after:

- the blob, the crew record, the system table, the clock, and the load/unload adapter -- one structure and
  one seam, ~85% confidence it lands without engine changes, because the mechanism above is shipped.

Large, and the actual body of the game:

- **wiring the ship.** Every deck's consoles and panels must become controls with effects, and every effect
  must be visible where it belongs. Most decks currently have almost nothing interactive (see the crew
  spec's space measurement). Start with two or three systems on one deck -- doors, power/lights, a console
  that matters -- prove the loop end to end (control changes ship state, state is visible on another deck
  after a transition), then extend. ~40% confidence that the first loop takes an afternoon; the remaining
  ninety percent of the wiring is content work, and it is the bulk of the program.
- **damage that models rather than decorates.** Effects that read as consequences across decks -- lights
  out here because power is out there -- need the compartment/system split to be real, not cosmetic.
- **Borg pressure and its counter-play** as a running system, not a scripted encounter. This is the most
  design-heavy piece, and the one where the ship model pays off most.

## Consequences for the gates

- **G3 does not change.** Five crew, one deck, posts, acknowledgement. But its crew record is written into
  **this** shape from the first line of code, because retrofitting a ship model beneath a driver built
  without one is the expensive version of the same work.
- **The ship model is G3's first artifact**, not a G4/G5 redesign: it is a header, a serialisation pair, and
  a stub adapter.
- **G4 (living ship)** grows to own systems and transit; **G5 (capstone)** proves the loop across two decks;
  **damage and the Borg** are their own gate, gated behind the systems being real.
- **Multiplayer stays reachable.** Keep the model serialisable and event-driven so that mode 3's "one ship,
  one server" can host it server-side, with the same deck adapters.
