# module/ — game logic compiled into the single-player game module

Code here is built *into* `libefgame.so`, alongside Raven Software's released single-player
source. It includes that source's headers and links against it, so it is distributed under the
same terms as the module it becomes part of: **Raven's STEF Game Source License** — free,
non-commercial, never sold. It is never linked into the GPL engine binary; the engine loads the
module at runtime. See `CONTRIBUTING.md` and the top-level `README.md`.

## How it gets built

`scripts/bootstrap-upstream.sh` configures the upstream module build with
`-DLWH_MODULE_DIR=<this directory>`. Patch `0005` adds that option and a handful of one-line
attach points; every `.cpp` under here is then compiled into the module. Without the option the
attach points are empty inlines and the module is exactly upstream's.

That is the whole of the arrangement, and the reason for it: **new game logic lives here, not in a
patch.** A patch is a rebase cost on every upstream release; a directory of our own files is not.

## crew/ — the reactive-crew direction layer (gate G3)

| file | what it is |
|---|---|
| `crew_core.h/.cpp` | Arbitration, post assignment, the director's restaffing rule, run statistics, the save record. Plain data and pure functions with **no game header**, so it is unit-tested on its own (`tests/crew`). |
| `g_crew.h/.cpp` | The half that touches entities: reads signals from an NPC, applies the core's decision, spawns declared crew, writes the report. |

Specification and the measured results: `docs/g3-reactive-crew.md`. Off by default (`g_crew 0`):
with the cvar unset the module behaves, and saves, exactly as it did before the layer existed.

Console, once a map is loaded with `g_crew 1`:

```
crew report     the run so far: coverage, yields, stuck events, acknowledgements, verdict
crew posts      the post table
crew address    address each crew member in turn, as the player would (self-test)
crew write      the report, written to crew/<map>.report.json under the home path
```

`g_crewDebug 1` prints every change of decision; `2` adds a once-a-second trace of anyone walking.

## ship/ — the ship simulation (gates S1, S2)

| file | what it is |
|---|---|
| `ship_core.h/.cpp` | The ship as a working system: power, systems, decks, stores, clock, the 141-crew roster and its watches. No game header; tested by `tests/ship`. |
| `g_ship.h/.cpp` | Hosts it in the game: ticks it, saves it, carries it across level changes, and exposes the `ship` console command. Off by default (`g_ship 0`). |

Programme and gates: `docs/ship-programme.md`. Every figure: `docs/lore-ledger.md`.
