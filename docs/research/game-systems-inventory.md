# What the shipped game already gives us, system by system

Measured from the published map sources (147 `.map` files, the game's own level sources), 2026-10-05.
This is the substrate half of the ship-systems design: before deciding how to implement a system, here is
what already exists as entities in the maps. Counts are keyword hits on `targetname`/`message` values
unless stated.

## The deck registry and the lift graph are already built

Voyager's ten Virtual Voyager decks are separate maps, and the turbolift between them is not a menu
bolted on top -- it is a **level-change graph**. `target_level_change` entities carry a `mapname`, and
there are **139 such links** across the deck and campaign maps:

```
deck01 -> tour/deck02, tour/deck03, tour/deck04, tour/deck05, tour/deck08, tour/deck09,
          tour/deck10, tour/deck11, _brig          (9 links)
deck02 -> tour/deck01 x2, tour/deck03..11, _brig, voy15
```

A sample of the standing connections: every VV deck reaches every other VV deck plus the brig, and at
least one campaign interior (`voy15`) is wired in. `target_level_change` is documented as a **HUB**
entity, which is exactly the semantics a ship-wide deck graph needs.

Per-deck interactive counts (entities / doors / usables / scriptrunners / level-changes):

| deck | entities | doors | usable | scriptrunners | level changes | systems visible in names |
|---|---|---|---|---|---|---|
| deck01 | 354 | 6 | 7 | 4 | 9 | turbolift, weapons, hull |
| deck02 | 753 | 26 | 14 | 7 | 11 | turbolift, computer |
| deck03 | 539 | 18 | 8 | 6 | 9 | computer x15, turbolift |
| deck04 | 983 | 66 | 19 | 32 | 25 | transport, holodeck x8, sensors, replicators, engines |
| deck05 | 370 | 9 | 9 | 5 | 9 | turbolift, forcefield, medical |
| deck08 | 663 | 10 | 18 | 40 | 10 | turbolift, crew space, medical, holodeck |
| deck09 | 608 | 34 | 22 | 4 | 9 | turbolift, computer |
| deck10 | 422 | 15 | 18 | 14 | 9 | doors x13, turbolift |
| deck11 | 345 | 4 | 18 | 4 | 9 | computer x28, engines, turbolift |
| deck15 | 186 | 4 | 1 | 7 | 10 | turbolift, holodeck, computer |

Campaign interiors carry the same vocabulary in lower density (`voy7`: shields, transport, turbolift;
`voy8`: forcefields; `voy16`: 23 doors, 29 crew-space names).

## What follows for the design

- **Engineering already has a room with 28 computer-named interactives and engine names** (`deck11`).
  That is where a power/EPS control surface starts, and it exists.
- **The holodeck and its eight holodeck-named objects are on `deck04`** -- which we had rejected as a
  crew space for being an entertainment deck. It is the wrong place for posts and the right place for
  holodeck systems.
- **Medical exists** (`deck05`, `deck08`), **force fields exist** (`deck05`, `deck08`), **doors are
  everywhere** (66 on `deck04` alone), and **turbolift travel is a solved problem in the engine**.
- **What does not exist anywhere: a system with a value that matters.** Every one of these is a local
  script trigger -- a door opens, a light changes, a sound plays. None of them reads or writes ship
  state, because no ship state exists yet. That is precisely the gap the ship model fills: the controls
  are there, the *model behind them* is not.
- **The deck graph gives the ship its topology for free**, so "where is everybody" can be expressed in
  the game's own terms (`tour/deck04`, `voy15`) rather than in an invented coordinate space.
