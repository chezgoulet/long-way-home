# Memory and consequence: what the crew remember, and how the world reads it

Two questions from the owner, 2026-10-05: can player input shape how the universe around a playthrough
develops, and can every character have their own memories? Yes to both, and they are the same mechanism seen
from two ends. **Memories are what the world reads.**

Because the whole of it is contained inside the mod -- no operator memory, no outside scope -- the boundary work
in `docs/memory-boundaries.md` reduces to one rule: **provenance.** A character cannot know what they were never
told.

## Part 1: player input shapes the universe, through records and not through writes

Three kinds of input, with different teeth:

1. **Actions.** What the player *does* is the strongest and most legitimate driver: taking the shortcut, writing
   off a deck, ejecting the core, turning away a refugee, spending the last torpedo. Actions write ship state,
   crew memory marks, faction reputation and the chart -- all of which already exist.
2. **Words.** What the player *says* -- to the crew, to a stranger, over an open channel -- writes **memories
   and debts**, never world state. A promise is a bond with a claim attached. A lie is a memory that will be
   found out, because someone else was there, or the evidence survives. This is how speech gets consequence
   without becoming magic.
3. **Configuration.** Naming the ship, choosing a starting state, setting a course: that is setup, not
   simulation, and it should be visibly separable so nobody mistakes it for a game outcome.

**The rule that keeps this honest: player input writes records; the world reads them.** Nothing the player says
directly edits the universe, because then the result would be neither deterministic nor saveable nor testable.
Instead the world's shape becomes a **function of the record**:

- scenarios have *breeding conditions* and the record supplies them (`docs/scenario-atlas.md`);
- factions carry reputation and awareness, and the record moves them (`docs/exploration-and-science.md`);
- the chart holds what is known and how confidently, and the record is where knowing comes from;
- and the crew carry what happened to them, which decides whether they obey, volunteer, or quietly stop.

Two rules keep it from going wrong. **Reactions must be attributable** -- the player should be able to connect
what changed to what they did, or the universe reads as random. And **some of the universe must stay
indifferent**: if everything echoes the player, the setting becomes solipsistic and the frontier stops being a
frontier. Most of the Delta Quadrant should never hear of Voyager at all.

## Part 2: every character keeps their own memory

A mark, per person, per thing that happened:

```
event id · source · time · valence · salience
```

- **source** -- *saw it myself* / *was told by X* / *heard it as rumour* / *read it in the log*. Provenance is
  the whole of the boundary rule, and it is what stops the crew being omniscient.
- **valence** -- how it felt: grief, pride, fear, resentment, relief.
- **salience** -- how much it still matters, which is what fades.

What that buys, and it is a lot for a small structure:

- **Different people hold different versions.** Told-about events carry the teller's version; rumours arrive
  distorted, and *the distortion is attributable* rather than random -- the person holds the rumour they heard,
  not a corrupted copy of the truth. Ask three crew about the same afternoon and get three answers.
- **Bonds are remembered valence with a person attached.** Repeated good valence becomes friendship; repeated
  bad becomes a grudge; a single extreme event can do either in one go. This is the same edge set as
  `docs/crew-roster.md`, fed by play instead of authored.
- **Salience decays unless reinforced.** A memory fades if nothing retells or re-experiences it -- and sharpens
  on an anniversary, a returning place, or a name. Canon does this constantly.
- **Storage is bounded per person**, evicting the oldest and least salient marks. That is a design choice *and*
  the reason this is affordable: eight marks per person is roughly twenty-seven kilobytes for all 141, which
  fits the save without a conversation.
- **"What does she know?" is a query.** It drives dialogue, it tells command who has to be briefed, and it is
  the single best debugging tool the design has -- when a character behaves unexpectedly, the answer is usually
  in their marks.

## The interaction, which is the point

The player's footprint is *what the crew remember*, and the crew's memories are what the world reads. So:

- a promise made in act one is a bond with a claim in act three;
- a deck written off is a mark in everyone who was aboard, and a rumour in everyone who was not;
- a captain who spends people accumulates resentments with names and dates attached;
- and the log, which is the ship's own memory, can be searched -- while the crew's memory cannot be, except by
  asking them.

## Acceptance

- Every character has marks with source, time, valence and salience, and a character who was not present and
  was not told cannot recall an event.
- Ask three crew who were differently placed in the same event and get three accounts.
- A promise, a lie and a rumour each produce a consequence later, traceable to the mark that carried it.
- Storage stays bounded: eviction is by age and salience, and the per-crew budget holds.
- And the world's reactions are attributable to something the player did -- while most of the galaxy never
  notices them at all.

