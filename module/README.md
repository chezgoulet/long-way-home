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

## The environment in the world (`g_env 1`)

The ship's per-deck atmosphere, gravity and breach are shown in the world by the same pattern as
`SyncDamage` and `SyncFire` (both in `module/crew/g_crew.cpp`). Gravity is per person — the engine's
own `ps.gravity` with `SVF_CUSTOM_GRAVITY` — so a deck whose plating has failed floats the player
and the crew (`BS_FLY`), and the deck recovering hands the world's value back. A breach switches on
an authored `trigger_push` aimed at the hole and a `trigger_hurt`; raising the field stops them and
makes the authored `func_usable` brush solid and visible. The player's way back out of freefall is
the magnetic boots, `ship boots`. Off by default (`g_env 0`): with the cvar unset the module
behaves, and saves, exactly as before. Evidence: `docs/evidence/environment-in-the-world.md`;
checks: `scripts/gravity-check.sh` and `scripts/breach-check.sh`.

## The player in the world (`g_player 1`)

The player is a crew record (`Ship::player`), so the causes the model already tracks — an exploding
console, fire, vacuum, a boarder, a weapon — can reach the person holding the controls. The world
reports where the player stands (`SetPlayerDeck`), a degraded console lets go at the player who
worked it (`UseSystemBy`, run from `Crew_PlayerFrame` via `ship operate`), and the world's damage is
written into the same record any casualty carries (`WoundPlayer`). Incapacitation is a state the
ship acts on: a medical hand is sent and named (`AttendIncapacitatedPlayer`), the player is carried
and treated in the ward like anyone else, and a downed player operates nothing. Death closes the
record and is not a reload (patch `0016` blocks the engine's respawn); command passes to the senior
fit officer. Off by default (`g_player 0`): with the cvar unset the module behaves, and saves,
exactly as before. Evidence: `docs/evidence/player-in-the-world.md`; check:
`scripts/playerworld-check.sh`.
