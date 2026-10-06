# What Long Way Home proves for ReachLock

Long Way Home is a proving ground. It is small enough to finish, old enough that its engine imposes real
constraints, and close enough in shape to ReachLock that the design work transfers -- **but not uniformly**.
Some of what we built are rules. Some are workarounds for an engine from 2001. Copying the second kind into a
modern stack would be inheriting a limitation with none of the cause.

## The rules, which transfer

1. **Persistent state is the spine.** The world lives in one authoritative structure; places are *views* of it,
   not the thing itself. In Long Way Home that is a data file plus a save blob; in ReachLock it is a seed plus
   deltas. **Same shape**: static content is versioned and editable, dynamic state is what accumulates.
2. **A population model, not a cast list.** Records carry needs, drives, fears, allegiances and skills, and
   behaviour falls out of them. Depth is earned by attention -- a small authored core, a large generated body,
   and life supplied by play. Promotion by attrition means the spotlight is inherited, which is how a hundred
   and forty people can all be real without a hundred and forty arcs being written.
3. **Bonds grown by the simulation beat bonds written down.** Authored seeds for the few; everyone else's
   relationships emerge from what actually happened, and both land in the same graph.
4. **Authority is social; access is technical.** Two different locks that fail differently. Rank gates the
   console absolutely and governs compliance only by degrees. Any org-shaped game has this: licences,
   clearance, contracts, crew roles.
5. **Failure is content.** Three severities per system; warnings before every avoidable failure; consequences
   that trace back to a decision; and the one unwinnable end gated behind a *chain*, never a roll. Rolled
   catastrophes feel like bugs; earned ones become war stories.
6. **One watched number measures the run.** Distance remaining against capability, with the *derivative* --
   whether the month improved it -- carrying more weight than the value.
7. **Scarcity that forces play outward.** Fuel, parts, stores: the reason to leave, fight, trade and explore
   should be a resource the player can see running down. It is the ratchet that stops turtling.
8. **Emergence from authored primitives.** Scenarios have *breeding conditions*, not timers: the state of the
   world recruits the content. Authored pieces, emergent narrative, no generative system required.
9. **The log is memory.** If the game can narrate what happened and why, the world feels remembered, and it
   becomes the save's human face.
10. **Reuse first, generate second, author third.** Count what exists before building. Generate from tables
    with a collision check that fails the build. Author by hand only where attention will actually land.
11. **Process: gates with transcripts.** Nothing is done without evidence, claims are not evidence, and a
    criterion whose evidence is missing is not "probably fine" -- it is unmeasured, and that fails.

## The workarounds, which do not transfer

These are artifacts of riding a fixed-map id Tech 3 engine. They are good solutions *there*, and they would be
self-inflicted wounds in a Rust/Bevy stack with real 3D:

- **Decks as separate maps, and a turbolift that is a level-change graph.** That exists because the engine
  loads one BSP. A modern client can have a contiguous ship and real interiors.
- **Non-pilotable shuttles as a load screen.** A workaround for no flight model. ReachLock should fly.
- **No terrain, no EVA, no exteriors** -- hence "the chart is data, and sites are re-dressed maps". With a real
  engine, exteriors become playable space rather than a skybox and a loading screen.
- **The save blob written through a 25-year-old serialiser.** ReachLock's deterministic seed protocol is a
  better persistence model; keep the *split* (static vs dynamic) and discard the blob.
- **Ship-shaped authority.** Rank, watches and departments fit a starship. ReachLock's hierarchy is more likely
  contractual: who may accept, who has clearance, who is owed. The two-lock rule transfers; the ladder does not.

## What to do with this

The transferable rules are worth lifting into ReachLock's own documentation **with its own evidence**, once its
design work resumes -- not as a copy of this programme's documents, but as the same questions asked against its
own systems. The value Long Way Home provides is that these rules have been made to *work* somewhere, at small
scale, with transcripts, before anyone bets a larger project on them.
