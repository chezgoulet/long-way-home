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

Pre-development. Gate G0 is the current objective: build the singleplayer game module
natively, reconstruct the script compiler, and write the asset validator.

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
- `docs/program-proposal-v2.md` — the proposal that preceded it, including the track analysis.
- `docs/native-linux-client-proposal.md` — the original client proposal (v1).
- `docs/elite-force-content-options.md` — what is possible, and under what licence.
- `docs/ef-community-catalog.md` — what the community has built, with evidence.
- `docs/sp-and-rpgx-plan.md` — the singleplayer-extension and RPG-X path.
