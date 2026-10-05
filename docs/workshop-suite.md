# The workshop suite: every asset ReachLock needs, and how not to build nine tools

A space-western needs more than creatures. But the mistake available here is to answer with a tool per asset
class, because every one of those tools would be the *same tool* with a different grammar: seed determinism,
a schema, generation with constraints, a quality gate, an export, and a release. So this is a map of the asset
domains, a single architecture over all of them, and an honest list of what should never be generated at all.

## Asset domains, and what each really needs

| domain | tool | relationship to what exists |
|---|---|---|
| **creatures** | pixel-life | **exists, at M14.** Nine body plans, procedural animation, spritesheet/GIF/metadata export |
| **people** | pixel-life extension | humanoid body plan, plus the two things animals did not need: **clothing and gear as layers**, and **faces as an identity system** |
| **droids and robots** | pixel-life extension | a *mechanical* body-plan family: hard surfaces, joints, one glowing eye. Same grammar, different primitive library |
| **alien sophonts** | pixel-life extension | non-human body plans with human roles: languages of shape rather than new physics |
| **ships** | pixel-ships | proposed in full: hull grammar, decks, compartments, systems, refits |
| **places** | the place generator | **the big unification.** Ship interiors, stations, settlements, caves, surface sites are one constraint engine with different grammars |
| **planets and space** | pixel-planets | orbital bodies, biomes, atmosphere, rings; starfields, nebulae, asteroid fields, gas giants; **planet portraits** at UI scale |
| **terrain and tilesets** | pixel-planets / place generator | ground tiles per biome, autotiled by adjacency rules; skylines and backdrops |
| **props and items** | pixel-props | weapons, tools, containers, cargo, consoles, furniture — a parts library with material variants, plus single-view inventory icons |
| **insignia, marks, type** | pixel-marks | faction crests, ship names, signage, rank pips, a pixel typeface. Cheap, and identity-dense |
| **effects** | pixel-fx | muzzle flashes, impacts, explosions, engine trails, dust, sparks — spritesheets under a shared palette |
| **sound** | sound workshop | music, ambience and effects generated from parameters, seed-deterministic. GC already does adaptive orchestral music procedurally, so the house has prior art |
| **story and contracts** | story workshop | contracts with objectives, deadlines, risk and reward; names for people, places, ships, factions with per-culture phonotactics; barks with tone tags. **This is the content ReachLock's contract engine actually consumes** |
| **balance and economy** | balance workshop | class stats, weapon performance, trade goods, prices, faction economies — with the budgets closed, the same way a ship's power budget closes |

## The four domains: Life, Ships, Places, Stuff

The owner's cut, and it is better than the asset-type list above, because it is organised by **what the player
perceives** rather than by what a tool happens to emit. Every asset request classifies into one of four nouns:

| domain | what it is | contains | tool |
|---|---|---|---|
| **Life** | things that live and move | creatures, people, droids and robots, alien sophonts | pixel-life |
| **Ships** | things that fly, and the places they contain | hulls, decks, systems, refits | pixel-ships |
| **Places** | things that *are* somewhere | worlds, stations, settlements, caves, surface sites, terrain, skylines | the place generator |
| **Stuff** | things that are held, worn or installed | weapons, tools, cargo, furniture, consoles, props, insignia | a props tool |

Four domains and four grammars, down from eleven rows of list — and, more usefully, **a routing rule**: every
asset request is classified into one of the four, which tells you which tool, which schema, which validator and
which gate. That is what stops tool sprawl: a new request is a grammar, not a project.

### Two amendments

**1. Ships are dual-natured, and they earn their slot from the premise rather than from ontology.** A ship is a
*place* you walk in, a *vehicle* you fly, and a *pile of stuff* you upgrade. Ontologically it is not a fourth kind
of thing at all. It stays top-level because the game is about ships -- a taxonomy inside a project should be
shaped by what the project is about -- and it should be *built* as what it is: the place engine's volume and
circulation machinery, plus the stuff library's parts, plus the motion and crew model from Life. Ships are the
domain where the other three meet, which is exactly why they are the interesting one.

**2. Two layers cut across all four, rather than sitting inside them.**

- **Events** -- contracts, incidents, barks, effects, history. Temporal, not spatial: a contract is not an
  object, it is something that happens to people in places, and an effect is something an event does to the
  world for a second. This is where ReachLock's contract engine lives.
- **Medium** -- every domain exports into several: pixel sprites, 3D meshes, deck plans, data records, and
  **audio**. Sound is not a fifth domain, it is the auditory medium of all four: a ship's engine, a settlement's
  ambience, a creature's call, a weapon's report.

So the shape is a matrix rather than a list: **four domains across, two cross-cutting layers through**, with the
grammars of the domains sharing a core and the layers reading every domain's schema.

### How the four refer to each other

The domains are not independent, and the references between them are the wiring that has to exist from the
start:

- **Life inhabits Places and Ships** -- the crew of a hull, the population of a settlement;
- **Stuff is carried by Life and installed in Ships and Places** -- a weapon in a hand, a console in a room;
- **Ships are Places, and contain both Life and Stuff**;
- **Places contain Ships, Life and Stuff** -- a hangar, a dock, a town;
- and **Events** reference all four: a contract names a person, a ship, a place and a thing, and moves them.

Which means the four schemas share one **containment and attachment graph**, and that graph -- not the art -- is
the product. It is what lets a generated freighter arrive at a generated settlement carrying generated cargo,
with a generated crew who have somewhere to be.

## The architecture: one core, many grammars

Every row above decomposes into the same six things:

1. **a seed and a deterministic core** (exact RNG stream state, so a seed reproduces a result bit-for-bit);
2. **a schema** — genes, parts, or records, declared as data and versioned;
3. **a generator** — assembly under constraints;
4. **a validator** — the invariants that decide whether the result is plausible;
5. **an exporter** — geometry, sprites, plans, data, or audio;
6. **a gate** — build, tests, probes, an app self-test, exiting non-zero.

Which means the honest plan is not eight tools. It is **one shared core** (`gen-core`: determinism, schema,
budgets, validation, export, gate) with **a grammar per domain** and **one app shell with modes** — overview
grid, studio, test area, as the creature workshop already has. The creature tool is the proven first customer of
that core, and pixel-ships is the second. Adding a domain then costs a grammar, not a project.

That also means the **intermediate representations are the real product**: a hull definition, a place definition,
a contract record. Those are what consumers — ReachLock, Long Way Home, the mod — actually read, and they are
what should be designed against the schemas already written rather than invented fresh.

## The unification worth insisting on

**Ship interiors, stations, settlements and caves are one tool, not four.** All four are: a volume, sliced into
levels; rooms with function tags; adjacency constraints by function; circulation that must reach everything;
egress that must exist; an affordance check; and a playable export. Only the grammar differs — decks and
turbolifts versus streets and doors versus tunnels and ladders. Building the constraint engine once and giving
it four grammars is the difference between a suite and a sprawl.

## What order, and why

Ordered by what blocks ReachLock:

1. **ships** and **places**, because they are structural — nothing plays without them;
2. **people** and **droids**, because they are the cast, and the creature tool is already most of the way;
3. **story and contracts**, because the contract engine is ReachLock's core loop and it consumes text;
4. **planets and space**, because they are the setting;
5. **props, marks, fx**, because they are cheap and make everything else read;
6. **sound**, because it is a domain of its own and the least coupled to the rest;
7. **balance**, running alongside from the start, because it is the thing that makes generated content cohere.

## What should not be generated

**Reuse first, generate second, author third** — the rule from `docs/design-creed.md`, applied to the suite.
Generation is for *volume and variation*: the tenth freighter, the fortieth face, the hundredth planet. It is
not for the things that carry the identity of the setting:

- a faction's theme music, and its one or two signature ships;
- hero characters, and anyone with a name the player is expected to remember;
- the set-piece locations a story turns on;
- anything the player will look at for hours, where familiarity matters more than variety.

A suite that knows which of those it should *not* attempt is worth more than one that generates everything,
because the gap between generated and authored should be a deliberate gradient, not an accident.
