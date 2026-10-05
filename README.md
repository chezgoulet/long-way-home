# Long Way Home

*One ship. One crew. No reset button.*

Long Way Home is a program of work that returns a twenty-five-year-old Star Trek game to
native Linux, and then fills it with new singleplayer experiences — including a crew that
behaves like a crew rather than a set of props.

It is not a port plan. It is a gated programme: a native client, an authoring pipeline, an
in-engine autonomy layer for NPCs, and — eventually — a persistent multiplayer ship where one
server is one crew and the ship succeeds or fails by whether they work together.

The design target is the playstyle of Voyager's *Year of Hell*: attrition, damage that persists,
resources that run down, and no reset button. Read `docs/design-north-star.md` for what that means
mechanically.

## Status

The gate ledger, `docs/gates.md`, is the authority; this is its summary.

| gate | what | state |
|---|---|---|
| G0 | toolchain: native game module, script compiler, validator, entity dictionary | closed |
| G1 | native client plays the campaign: mission, save, reload | closed, signed off |
| G2 / G6 | Virtual Voyager and the retail game as a mode | reported working by the owner |
| G7 | retail multiplayer over LAN or VPN (cMod, as shipped) | server half proven; needs two machines |
| G3 | reactive crew: 5–10 NPCs holding posts on one deck | implemented and measured; awaiting the owner's judgement |
| S1 | ship core: power, systems, decks, clock, the 141-crew roster and watches | done |
| S2 | the ship running in the game, and Main Engineering's console driving it | built and verified headless; yours to operate |
| S3–S10 | the whole-ship map, every station's console, crew lives, damage, intruders, the Borg, the outside | S3 next; see `docs/ship-programme.md` |

## Building and checking

```
scripts/bootstrap-upstream.sh     # clone the pinned upstream, apply patches/, build the game modules and tools
scripts/test.sh                   # every check that needs no game data (this is what CI runs)
scripts/fetch-gdk.sh              # fetch the official GDK content G0 is checked against, into build/gdk
scripts/check.sh ...              # G0's corpus checks; needs that content
scripts/g3-measure.sh             # G3's measured run; needs your own copy of the game
scripts/build-ship.sh             # S3: stitch the published decks into one map and compile it
scripts/s2-check.sh               # S2: the ship in the game, saved, reloaded, and operated from its console
scripts/run-engine.sh             # play (see docs/playtest.md)
scripts/run-scenario.sh           # play a crewed deck
```

## Layout

| path | what |
|---|---|
| `patches/` | our delta from the pinned upstream port: nine patches, one concern each |
| `engine/` | the native Linux build of the single-player engine (GPL-2.0) |
| `module/` | our game logic, compiled into the game module (STEF licence) — the crew direction layer and the ship simulation |
| `tools/` | script compiler, validator, entity dictionary, map generator, crew authoring |
| `scenarios/` | scenarios as versioned artifacts, each with a `scenario.json` |
| `tests/` | unit tests for the direction layer and the tools |
| `docs/` | the charter, the gate ledger, specifications, and the evidence for every claim |

## Why this exists

A fan project's worth is measured in what survives it. This one starts from what already
exists — Raven Software's officially released singleplayer source, a working open-source
port on another platform, and the official development kit — rather than from ambition.

## Licensing

This repository contains code under two separate licences, deliberately kept apart:

- **Our code, the engine, and all tooling: GPL-2.0** (see `LICENSE`). See `NOTICE` for the
  engine's lineage of prior authors.
- **The singleplayer game module: Raven Software's STEF Game Source License**, which permits
  non-commercial modification and free distribution, and forbids sale. It is never linked
  into the GPL engine binary; it is loaded at runtime as a separate shared library.

**No game assets are included or distributed in this repository, ever.** You need your own
copy of the game.

## Not affiliated

An independent, non-commercial fan project. Star Trek and related marks are the property of
their respective rights holders. This project is not affiliated with, endorsed by, or
sponsored by any of them, and no official status is claimed.

## Documentation

- `docs/design-north-star.md` — what the project is *for*: the attrition design, the multiplayer
  model, and the engine's measured limits. **Read this first.**
- `docs/program-charter-v3.md` — the gated programme charter: gates, scope, arbitration rules,
  acceptance criteria.
- `docs/ship-programme.md` — the operational-Voyager programme: the owner's decisions and gates S1–S10.
- `docs/lore-ledger.md` — every figure in the ship simulation, with its source or marked invented.
- `docs/gates.md` — the gate ledger: what has passed, on what evidence, and what is still open.
- `docs/client-modes.md` — the client's three modes, and why retail multiplayer is cMod as shipped.
- `docs/g3-reactive-crew.md` — the reactive-crew specification, and what building it found.
- `docs/scenario-manifest.md` — the scenario format, including the crew section.
- `docs/playtest.md` — how to run the client, and what to send back.
- `CONTRIBUTING.md` — branch flow, the patch discipline, and the evidence rules.
- `docs/program-proposal-v2.md` — the proposal that preceded it, including the track analysis.
- `docs/native-linux-client-proposal.md` — the original client proposal (v1).
- `docs/elite-force-content-options.md` — what is possible, and under what licence.
- `docs/ef-community-catalog.md` — what the community has built, with evidence.
- `docs/sp-and-rpgx-plan.md` — the singleplayer-extension and RPG-X path.
