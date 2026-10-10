# Evidence: the three independent sources, and a deck that breathes on its own

`docs/budget-squaring.md`, **Finding two** — *"At least one deck holding air independently of the main
grid"* (VOY "Macrocosm"): *"The three independent sources are the mechanism canon gives us for deck-by-deck
survival, and we do not have it."* Built on `feat/the-air`, cut from `testing`, 2026-10-10, on the builder
host `mikoa`.

**Observed, and the command that produced it.** Every figure below is the output of a command named beside
it, on this tree. The model half is proved here; **the world half is not** — there is no retail game data on
this host, so the engine checks are named as unavailable and their command is given rather than claimed. The
boundary has its own section at the end.

## What was built

The deck-by-deck air mechanism, in `module/ship/ship_core.{h,cpp}`. Three pieces:

- **The three sources, as separate state.** The sources independent of the main grid — the warp core — are
  the three that survive it, with the capacities `docs/budget-squaring.md` Part 5 gives them: the **impulse
  reactors (250)**, the **auxiliary fusion reactor (90)** and the **emergency batteries (60)**. Each already
  carries its own `health`, `online` and `output`; the air feed reads that state so one can be isolated (a
  source switched off), one can fail (health to zero), and one can carry a deck on its own.
- **A per-deck air feed.** `Deck` gains `airSource` (`AIR_GRID` = the pooled main grid, the default; or one
  of the three), `airBy` and `airCrew` — the provenance, as a system's allocation has. `DeckLifeSupport(s,
  deck)` is now the support the simulation and `MinutesOfAir` use: the grid's pooled life support, or the
  independent source's own delivery. So a deck holds its air with the main grid down, and the state says
  which source holds which deck's air.
- **An allocation the player makes.** `SetAirSource` / `SetAirSourceBy` (an officer under a standing grant)
  set the feed; it is never shed by the simulation; and when the decks committed to a source exceed what it
  can hold, `AirShortfall` reports the number. One deck on its own source is served by its own life-support
  plant, whose draw is the ship's own life-support demand (`AIR_PLANT_PER_DECK`, 60 —
  `docs/budget-squaring.md` Part 4b: *"the batteries alone hold life support at exactly full"*), so the
  batteries hold exactly one deck, the auxiliary fusion one, the impulse reactors four.

## The three sources as separate state, one isolated, one failed, one carrying

```
$ test_ship_core --air
== the three independent sources, as separate state
  capacities (docs/budget-squaring.md): impulse 250, auxiliary 90, batteries 60 EPS
  impulse isolated (online=0), auxiliary failed (health 0%), batteries carrying deck 9 (holds 1)
```

`TestTheAir` asserts all three states and the capacities. The numbers are read from
`docs/budget-squaring.md`; nothing is invented.

## A deck holding air on its own source, and the number before and after the crew act

```
== a deck holding air on its own source, with the main grid down
  deck 9 feed auxiliary fusion, support 1.00; deck 8 feed the main grid, support 0.00
  deck 9 breached: 3.8 min of air
  field raised: -1.0 min (-1 = holding)
```

With the warp core off and the ship's own life-support plant down, deck 9 — fed from the auxiliary fusion
reactor — still delivers full support, and deck 8, on the grid, does not. A breach on deck 9 produces a
number (**3.8 minutes**), and the crew act (raise the force field) makes it go (`-1`, holding). This is the
acceptance the air slice was measured against, with a source behind it. `TestTheAir` asserts the breach
number, the field, and both supports.

## The shortfall, as a number

```
== the shortfall is a number, and nothing is shed
  batteries asked for 2 decks, hold 1; committed 120 EPS, short 60 EPS; deck 10 is still on emergency batteries
```

The batteries (60 EPS) can hold one deck; asking them for two is reported (**short 60 EPS**) and the
player's commitment stands — `AirSourceOf` on deck 10 is still `SRC_BATTERIES`. Nothing is shed, and no
increase is refused: the same doctrine as `docs/power-assignment.md`, applied to the air feed.

## The registers, and the save

```
$ scripts/hooks-check.sh
hooks-check: module/ship/ship_core.h declares 441 public functions; docs/hook-register.md registers 441 hooks
PASS  every public function is registered, and every registered hook exists

$ scripts/api-check.sh
verbs.py: module/ship/g_ship.cpp answers 130 verb(s); module/ship/console_api.def enumerates 130
PASS  every console verb is in the ship's API, and every API verb is answered by the console
```

Both directions hold: the ten new public functions are in `docs/hook-register.md`, and the new `air` console
verb is in `module/ship/console_api.def` and answered by `Svcmd_Ship_f`.

**The save.** The new state goes in the save: the feed and its provenance append to each deck's record, and
`SAVE_VERSION` moves **55 → 56**. The cost, named: an older save (55) no longer loads — `Unpack` rejects a
version it does not know — which is normal here, and is the cost of adding saved state. The round-trip:

```
== the feed round-trips a save (version 56)
  unpack ok; deck 9 feed emergency batteries; identical yes
```

`TestTheAir` asserts the round-trip and that `Pack(back) == blob`.

## The console

`ship air` reads the feed back; `ship air <deck> grid|impulse|auxiliary|batteries` sets it (environmental
control's, so Operations'). It is published for the Operations panel as `lwh_ship_air` (the decks on their
own source, with `AIR SHORT <n> EPS` appended in a shortfall), `lwh_ship_air_committed` and
`lwh_ship_air_shortfall`, and drawn in `module/ui/ui_lwh_engineering.cpp` (red on a shortfall). `Describe`
carries the per-source feed lines too.

## What was proven, and the commands

```
scripts/test.sh   -> all checks passed (crew, ship, tools, hooks, api, orders, harness, docs, python, shellcheck, 20 patches)
scripts/check.sh --source-map <voy1.map> --script-corpus <shipped scripts> --build-root <upstream>
                  -> ALL PASS; 2,016/2,024 scripts, 8 known rejections
tests/ship/test_ship_core -> ship_core: all checks passed   (TestTheAir among them)
test_ship_core --air      -> the readout above
```

`scripts/test.sh` exits 0 on this host. `scripts/check.sh` exits 0 with the pinned upstream's `voy1.map`
and the shipped script corpus supplied to it (the source map and corpus are game content and are not in the
repository).

## The boundary — what this host could not prove

There is **no retail game data on `mikoa`**, so nothing that needs the client running is proved here, and
none of it is claimed:

- **The air clock read at a deck, and a compartment photographed after a breach.** The playtest host runs:
  `scripts/air-check.sh` — one headless run (`g_shipTest 98`), which puts a deck on its own source with the
  core down, breaches it, raises the field, isolates and fails the other sources, over-commits the
  batteries, and writes `build/g3-home/baseEF/ship/air.txt`.
- The engine checks that drive the client (`scripts/breach-check.sh`, `scripts/forcefield-check.sh`,
  `scripts/breach-check.sh`) are unavailable here for the same want of data; `scripts/air-check.sh` is the
  same shape.
- **Whether it reads as dramatic** — one deck holding while the one next door goes dark — is the owner's
  walkthrough, not a number.

## Judgement calls, named

1. **Which three.** The brief quotes canon's triad — life support, the holodecks and shuttlecraft — but also
   says *"the design already carries the sources and their capacities"* and *"read the numbers from
   `docs/budget-squaring.md`; do not invent or restate them."* The design's sources with capacities are the
   four in `SOURCES[]`; the three independent of the main grid are the impulse reactors, the auxiliary
   fusion and the batteries (250/90/60, reads straight from `docs/budget-squaring.md` Part 5, and the set
   Part 4a calls the surviving plant). Those are the three modelled here. Canon's triad is the *reason* the
   mechanism exists; the holodeck reactor's use is the trap, which the brief defers and which is already
   built as `JumpStartFromHolodeck` (`docs/ship-systems.md`). The alternative reading — modelling the
   life-support, holodeck and shuttlecraft reactors as three new sources — would require capacities the
   design does not carry for the shuttle, so it would mean inventing numbers, which the brief forbids.
2. **A deck on its own source is served by its own life-support plant.** The per-deck draw is the ship's own
   life-support demand (60), so a source holds `capacity / 60` decks (batteries 1, auxiliary 1, impulse 4).
   This is what makes the commitment able to go into shortfall — *"a budget that cannot go into shortfall is
   decoration"* — and it rests on the one number the design gives: `SYS_LIFE_SUPPORT`'s demand, which
   `docs/budget-squaring.md` Part 4b equates with the batteries' whole capacity.
3. **The independent feed is a dedicated draw, not a share of the pool.** A deck held on its own source does
   not also reduce `SYS_LIFE_SUPPORT`'s pooled load; the shortfall is where the money is counted. Keeping it
   out of the pooled allocator is what lets the feed survive the grid going down, which is the whole point.
   Recording it as a call, because a future lane may prefer to fold it into `UpdatePower`.
4. **`SetAllocation`'s set-time refusal is untouched.** The existing power console refuses an increase that
   would oversubscribe the plant (and `docs/power-assignment.md` says so); the brief's *"a number, not a
   refusal to act"* is honoured for the **air** allocation, which reports and never refuses or sheds. This
   leaves the older power contract as it was, and its tests passing.

## Files

- `module/ship/ship_core.h`, `module/ship/ship_core.cpp` — the feed, the per-deck support, the save.
- `module/ship/g_ship.cpp` — the `air` verb, the Operations station, the published cvars, `g_shipTest 98`.
- `module/ship/console_api.def` — the `air` verb.
- `module/ui/ui_lwh_engineering.cpp` — the Operations line.
- `tests/ship/test_ship_core.cpp` — `TestTheAir`, `--air`.
- `scripts/air-check.sh` — the world-side demonstration (unavailable here; the playtest command).
- `docs/hook-register.md`, `docs/budget-squaring.md`, `docs/ship-systems.md`,
  `docs/damage-and-budgets.md`, `docs/evidence/the-budgets.md`, `docs/gates.md`, `docs/omissions.md` —
  the register and the corrected "not yet built" statements.
