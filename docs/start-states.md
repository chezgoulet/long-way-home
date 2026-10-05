# Start states: how a run begins

The owner's requirement, 2026-10-05: **single player should support beginning states where any number of the
command crew died in the first beat, if the player wants to start their game in that role.** Together with the
multiplayer branch, this gives the programme one mechanism and two uses.

## One mechanism

The engine already exists: **vacancies, and a roster that promotes to fill them.** A start state is simply the
initial condition of that engine, expressed as data:

- `casualties[]` -- which named crew records are **closed before the player takes control**, and how;
- `vacancies[]` -- derived from the casualties by the same rules that run during play, not hand-written;
- `player` -- the record, post and rank the player inhabits;
- `ship_state` -- the Caretaker aftermath: twelve or more dead (canon), microfractures in the core, no array,
  the Maquis aboard;
- `log_seed` -- the first entry in the ship's log, which is how the game *tells* the player what they chose.

Because vacancies are derived and posts are data, **a start state costs almost nothing to author.** It is not
bespoke content; it is a different initial condition on the same systems. That is what makes "any number" an
affordable promise rather than a combinatorial explosion.

## The default, and the variants

**The default stays canon**: the senior staff survive, the chain of command is intact, and the player is a
junior officer serving under a captain. That protects the retail feel and the canon cast, and it keeps the
option meaningful rather than mandatory.

The variants then let the player choose **where they stand in the drama** -- and the hierarchy is playable
**from top to bottom**, including the chair itself. There is no post the player is not allowed to start in;
the point is to gamify the chain of command, not to protect it from the player.

- **Ensign**: the ceiling is intact above you; you are sent into the breach, and you find out why the orders
  are what they are.
- **Department head**: you hold a section, you own its people, and you argue with the bridge.
- **Executive officer**: you carry out orders, you disagree with some, and you decide whether to say so before
  or after.
- **Captain**: the conn is yours from the first minute -- the ship, the crew, the fuel, the clock, and every
  decision in the scenario set, along with everyone who thinks they could have made it better.

Every one of those experiences is authored **once**. The scenarios are identical; the position in them is what
changes, and the position is a post plus a situation.

## The rule that replaces gatekeeping

What matters is not whether the menu will let you sit in the chair. It is whether the *world* behaves as
though the authority is real once you are there. So:

> **The menu grants the situation. The fiction supplies the reason. The simulation then holds you to it.**

A captain start state is not a cheat: it comes with a *casualty that created the vacancy* -- the Caretaker took
the captain, or the ranking officer, and the record says so, and the crew know it. You are not handed the
chair; you inherit it, which is the same thing multiplayer does with every senior post. What follows from that
inheritance is the game: the authority is real, the responsibility is real, and the crew's expectations of you
are real.

And with that, **multiplayer is simply a start state with more players in it.** Same casualty list, same
vacancies, same first log entry -- the only difference is how many of the seats are occupied by humans rather
than by the simulation. That is the unification the two modes were pointing at all along.

## The hierarchy as gameplay: one verb per rank

The reason to gamify from the top down is that each level of the chain has a *different thing to do*, and the
same event is a different game depending on which verb you hold:

| rank | what you actually do | what you cannot do |
|---|---|---|
| captain | decide, prioritise, spend, and answer for it | operate anything yourself |
| executive officer | sequence the captain's intent, disagree out loud, hold the deck | change the objective |
| department head | assign your people and defend your section | command outside it |
| junior | execute, notice, report, and live with it | choose the objective, or refuse safely |

**The captain cannot fix anything.** That is the design, and it is what makes the top seat a distinct
experience rather than a stronger version of the others: the captain's only instruments are *people and
priorities*. You cannot seal the coolant loop; you can only decide who goes, what they leave behind, and
whether it is worth it. And the captain's failure mode is not death -- it is **losing the crew's confidence**,
which is why command must be losable: relieved by the ship's own people, or answerable to a Starfleet that is
suddenly reachable. A chair you cannot be removed from would not be a game.

## What the opening must do

A start state is a situation, not a difficulty slider, so the first minutes have to show it:

1. **The log entry.** The ship's own record states the losses, the ship's condition, and that no rescue is
   coming. Later entries are measured against it.
2. **The empty chair, or its absence.** If the captain died, the room and the roster say so without a
   cutscene: the chair is occupied by an acting officer, the roster shows the gap, the crew talk about it.
3. **Who is doing what now.** The promotions are visible as *records* -- the person at the conn has a name and a
   post they did not expect, and their old post is now held by someone else.
4. **The player's own position stated plainly.** What they hold, what they can authorise, who reports to them.

## Acceptance

- Casualties, promotions and the resulting vacancies are visible in the roster and the log before the player
  does anything.
- The same scenario set plays differently from two different start states, and no scenario needed to be
  rewritten to allow it.
- **Every start state comes with a reason the crew accept**, recorded in the log -- including the chair. No
  post is reachable without the fiction supplying the vacancy that created it.
- Authority is honoured by the simulation afterwards: the same decision issued by a junior is refused, and by
  the captain is carried out.
- Command is losable -- through death, relief, or the crew's own decision -- and the ship records who holds the
  conn at every point.
- The default start state is canon-complete, and the variants are labelled as branches.
- And the human check: the opening should feel like a situation to be handed, not a character to be equipped.
