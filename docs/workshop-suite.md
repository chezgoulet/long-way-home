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
