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

The variants then let the player choose **where they stand in the drama** -- which is the same idea as the
rank-and-role design, applied to the whole campaign rather than to a single scenario:

- **Ensign**: the ceiling is intact above you; you are sent into the breach, and you find out why things are
  ordered the way they are.
- **Department head**: you hold a section, you own its people, and you argue with the bridge.
- **Executive officer**: you execute orders, you disagree with some, and sometimes you must decide whether to
  say so before or after.
- **Command**: the captain died, the conn is yours, and every decision in the scenario set is now yours to make
  and to answer for. This is the single-player version of what multiplayer does by construction.

Every one of those experiences is authored **once** -- the scenarios are the same; the *position* in them is
what changes. A vacancy list and a post are the whole difference.

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
- A start state with vacancies produces *more* senior authority for the player only if the promotion rules say
  so -- never because the menu granted it.
- The default start state is canon-complete, and the variants are labelled as branches.
- And the human check: the opening should feel like a situation to be handled, not a character to be equipped.
