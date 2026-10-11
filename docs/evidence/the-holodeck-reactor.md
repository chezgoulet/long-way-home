# Evidence: the holodeck reactor as a per-deck source — the fourth, and the incompatible one

`docs/gates.md`, **O2's remainder** — the finding this closes: *"What is left of the canon triad is the
holodeck reactor as a per-deck source, which the brief for this pass did not build."* It is the fourth
source in the shape `docs/evidence/the-air.md` set, and the one canon marks as a trap. Built on
`feat/the-holodeck-reactor`, cut from `testing`, 2026-10-11, on the builder host `mikoa`.

**Observed, and the command that produced it.** Every figure below is the output of a command named
beside it, on this tree. The model half is proved here; **the world half is not** — there is no retail
game data on this host, so the engine checks are named as unavailable and their command is given rather
than claimed. The boundary has its own section at the end.

## The canon line, quoted with its ledger source

`docs/lore-ledger.md`, *Systems and power*:

> | The holodeck matrix is a trap | with the main grid down, tying in a holodeck reactor returns a battery
> charge and wrecks half the system set (canon: "blew half the ship's relays") | canon (VOY "Parallax");
> the modelled cost is invented |

and the constants table in `docs/ship-systems.md`:

> | independent power | shuttlecraft, life support and the holodecks are the only three | USS Voyager ("Macrocosm") |
> | holodeck reactors | separate from the main grid; **matrix incompatible** -- tapping it blew half the
> ship's relays | Holodeck reactor ("Parallax") |

`docs/budget-squaring.md` quotes the same line and this pass added the reactor to the ledger as its own
row: *"the fourth independent source, off the grid: it holds **one** deck's life support … only while the
main grid is down; a tie with the grid up is refused, and a tie left in when the grid returns spends the
reactor."*

## What was built

The holodeck reactor, in `module/ship/ship_core.{h,cpp}`, as `AIR_HOLODECK` — a per-deck air source,
**not** a grid `Source`:

- **The same shape as the three.** A capacity (`HOLODECK_AIR_EPS = 60`), a health and an isolation state
  (`Ship::holodeckReactor`, a `Source`), a provenance (`airBy`/`airCrew`, as every allocation has), a
  place in `SourceDecksHeld`, `AirCommitted`, `AirShortfall`, `DeckLifeSupport`, `AirSourceName`, and the
  save. It holds **one** deck: one life-support plant, the holodeck's own demand (`SYS_HOLODECKS` = 60,
  `docs/budget-squaring.md` Part 4b), so `capacity / AIR_PLANT_PER_DECK` = 1.
- **Off the grid, because canon says it is incompatible.** It is deliberately **not in `sources[]`**, so
  `PowerAvailable`, `PowerCapacityFresh`/`Now` and `MinutesToDark` never count it. That is the difference
  the ledger's line names, expressed as a structural fact rather than a flag.
- **A tie is refused while the main grid is up.** `SetAirSource`/`SetAirSourceBy` refuse `AIR_HOLODECK`
  unless `Coreless(s)` — the same condition `JumpStartFromHolodeck` uses — and log the refusal. Only with
  the main grid down may a deck hold its air on it.
- **The cost, if the grid returns while a tie is in.** `UpdateHolodeckTie` (run each step after
  `UpdatePower`) spends the reactor: its health goes to 0 and the holodeck matrix with it, so the deck
  loses its air and the loss is a shortfall number. The player pre-empts it by moving the deck back to
  the grid before restoring the core — the trap's own decision, in small.

## The source, demonstrated: a deck on the holodeck reactor, grid down

```
$ tests/ship/test_ship_core --air
== the holodeck reactor: the fourth source, and the incompatible one
  as ship power: grid fresh 1800 EPS (the reactor adds 0); tie with the grid up refused
  deck 9 feed holodeck reactor, support 1.00, holds 1 deck(s); deck 8 support 0.00
  asked for 2 decks: short 60 EPS, nothing shed (deck 10 still holodeck reactor)
  the grid returned: reactor health 0%, deck 9 support 0.00, short 120 EPS
```

With the warp core off and the ship's own life-support plant down, deck 9 — fed from the holodeck
reactor — still delivers full support, and deck 8, on the grid, does not. `TestTheAir` asserts all of it
(`tests/ship/test_ship_core.cpp`).

## The incompatibility, demonstrated: refused live, and its cost

- **As ship power it adds nothing, and a tie with the grid up is refused.** The line above shows
  `grid fresh 1800 EPS (the reactor adds 0)`; the same readout's first check is
  `CHECK(!SetAirSource(h, 9, AIR_HOLODECK))` — the refusal, with the grid live — and
  `CHECK(h.PowerCapacityFresh() == 1800)`.
- **The cost, when the grid returns while a deck is still tied.** The last line: `reactor health 0%,
  deck 9 support 0.00, short 120 EPS`. The reactor is spent, the deck loses its air, and the loss is a
  number — not a silent shed.

This is the design's answer to *"what happens when something tries to use it the way the ship's power is
used"*: **the refusal** while the grid is live, and **the cost** when the incompatible matrices meet. It
is a judgement call that the incompatibility is a refusal-and-cost rather than a shortfall; named below.

## The over-commitment stays honest

The same shape as the three: `asked for 2 decks: short 60 EPS, nothing shed (deck 10 still holodeck
reactor)`. `AirShortfall` reports the number, `AirSourceOf` on deck 10 is unchanged, and nothing is
shed — the doctrine of `docs/power-assignment.md`, applied to the fourth source as to the others.
`TestTheAir` asserts it, and also that isolating the reactor (`SetSourceOnline(AIR_HOLODECK, false)`)
leaves the decision standing and the shortfall a number.

## The trap, retained: `JumpStartFromHolodeck`, unchanged

The trap is not touched. `TestHolodeckTrap` still asserts it, and still passes in the suite:

```
$ tests/ship/test_ship_core
ship_core: all checks passed
```

`TestHolodeckTrap` (`tests/ship/test_ship_core.cpp`) is unchanged and still runs:
`CHECK(!JumpStartFromHolodeck(s))` with the grid up; then with the core gone,
`CHECK(JumpStartFromHolodeck(d))`, `CHECK(d.stores.batteries > batteries)` — it buys back power —
`CHECK(d.systems[SYS_HOLODECKS].health == 0.0f)`, and `CHECK(ruined >= SYS_COUNT / 3)` — canon's "half the
ship's relays". The new per-deck source is a *separate* act from the whole-grid jump-start; the two
compose (a jump-start wrecks `SYS_HOLODECKS`, which also makes the air source unavailable) but neither
replaces the other.

## The falsification: break the new source, and the check catches it

The break, planted in `SourceAirCapacity` (the holodeck branch returns 0 instead of the real capacity),
then a rebuild and the test:

```
FAIL  the three sources are separate, and a deck breathes on its own when the grid is down: SourceAirCapacity(h, HOLO) == 60 (line 939)
FAIL  the three sources are separate, and a deck breathes on its own when the grid is down: DeckLifeSupport(h, 9) == 1.0f (line 949)
FAIL  the three sources are separate, and a deck breathes on its own when the grid is down: SourceDecksHeld(h, HOLO) == 1 (line 951)
FAIL  the three sources are separate, and a deck breathes on its own when the grid is down: AirShortfall(h) == AIR_PLANT_PER_DECK (line 954)
FAIL  the three sources are separate, and a deck breathes on its own when the grid is down: AirShortfall(h) == 0 (line 957)
FAIL  the three sources are separate, and a deck breathes on its own when the grid is down: SourceAirCapacity(iso, HOLO) == 60 (line 969)
6 check(s) failed
```

Restored, the test exits 0. **The check can fail for the reason this pass cares about**, and it does.

## The registers, and the save

```
$ scripts/hooks-check.sh
hooks-check: module/ship/ship_core.h declares 479 public functions; docs/hook-register.md registers 479 hooks
PASS  every public function is registered, and every registered hook exists

$ scripts/api-check.sh
PASS  every console verb is in the ship's API, and every API verb is answered by the console
```

The pass adds **no** public function — it extends the existing air-feed surface (`SourceAirCapacity`,
`SourceDecksHeld`, `AirShortfall`, `SetAirSource`, …) and `SetSourceOnline`/`DamageSource` — so the
register's count is unchanged and still agrees. The `air` console verb's usage gains `holodeck`
(`module/ship/console_api.def`), and the console answers it.

**The save.** The new state goes in the save: the holodeck reactor's health and isolation, beside the
grid sources'. `SAVE_VERSION` moves **59 → 60**, and `tools/docs/owned_claims.json` moves with it. The
cost, named: **an older save (59) no longer loads** — `Unpack` rejects a version it does not know — which
is normal here, and is the cost of adding saved state. The round-trip:

```
$ tests/ship/test_ship_core --air
== the feed round-trips a save (version 60)
  unpack ok; deck 9 feed emergency batteries; identical yes
```

`TestTheAir` asserts a deck tied to the holodeck reactor and a reactor the cross-tie has already spent
(`health 0.4`) round-trip, and that `Pack(back) == blob` for both.

## The console

`ship air` reads the feed back and now shows a deck on the holodeck reactor; `ship air <deck> holodeck`
sets it (refused while the grid is up, with the reason in the log); `ship source holodeck on|off`
isolates the reactor. The Operations panel's `lwh_ship_air` string carries the deck on its own source,
and `Describe` prints the holodeck reactor row with a grid output of **0** — the incompatibility in the
readout.

## What was proven, and the commands

```
scripts/test.sh   -> all checks passed (crew, ship, tools, hooks, api, orders, harness, docs, python, shellcheck, patches)
tests/ship/test_ship_core -> ship_core: all checks passed   (TestTheAir and TestHolodeckTrap among them)
test_ship_core --air      -> the readout above
scripts/docs-check.sh     -> 34 register row(s) annotated, all agree with the tree
cmake --build (LWH_MODULE_DIR=module) -> libefgame.so links; g_ship.cpp and ship_core.cpp compile
scripts/check.sh  -> NAMED, NOT RUN: it needs the pinned upstream's source map and the shipped script
                     corpus, which are game content and are not on this host. The command is
                     scripts/check.sh --source-map <voy1.map> --script-corpus <shipped scripts> --build-root <upstream>
```

`scripts/test.sh` exits 0 on this host; `scripts/check.sh` and `scripts/docs-check.sh` are the validator
and the docs check (the latter exits 0 here). The output of the first and last is quoted above.

## The boundary — what this host could not prove

There is **no retail game data on `mikoa`**, so nothing that needs the client running is proved here, and
none of it is claimed:

- **The air clock read at a deck, and the cross-tie in the world.** The playtest host runs
  `scripts/air-check.sh` — one headless run (`g_shipTest 98`), which now also puts a deck on the holodeck
  reactor with the core off, refuses a tie with the grid up, over-commits it, restores the core, and
  shows the reactor spent.
- The engine checks that drive the client (`scripts/breach-check.sh`, `scripts/forcefield-check.sh`) are
  unavailable here for the same want of data; `scripts/air-check.sh` is the same shape.
- **Whether the reactor *feels* like a distinct, slightly dangerous thing** — one deck living on a
  reactor canon calls a trap — is the owner's walkthrough, not a number. This pass does not claim it.

## Judgement calls, named

1. **Off the grid, not a fifth `Source`.** Canon says the holodeck matrix is incompatible with the ship's
   power, so the reactor is not added to `sources[]`: it never feeds the pooled grid. The alternative —
   a fifth `SRC_HOLODECK` in `sources[]`, excluded from every grid loop — would have made the power
   console list it as an ordinary source and risked the 1,800 nameplate. Keeping it out is the
   incompatibility, not a workaround.
2. **The capacity is the holodeck's own demand (60 EPS), so it holds one deck.** Canon gives no figure
   for the reactor; the only number the design carries is `SYS_HOLODECKS`'s demand, which
   `docs/budget-squaring.md` Part 4b equates with one life-support plant. `[our call]`, recorded in the
   ledger.
3. **The incompatibility is a refusal *and* a cost, not a shortfall.** A tie with the grid up is refused
   (the cross-tie into live power is the first step of the trap); a tie left in when the grid returns
   spends the reactor. The brief allowed "the refusal, the shortfall or the cost"; the design takes the
   refusal for the live-grid case and the cost for the return, because the shortfall is already the
   over-commitment behaviour and would not say anything canon does not.
4. **`Coreless` is "the main grid is down".** The gate on the tie is the same condition
   `JumpStartFromHolodeck` uses and the same one `docs/evidence/the-air.md` used, so the per-deck source
   and the trap agree on what "the grid is down" means.
5. **Isolation is offered, and does not touch the trap.** `SetSourceOnline(AIR_HOLODECK, false)` is a
   control (cut the incompatible reactor out); `JumpStartFromHolodeck` does not consult it, so the trap's
   behaviour is unchanged. Flagged because a future lane may prefer the trap to respect the isolation;
   doing so here would have changed the trap this pass must retain.

## Files

- `module/ship/ship_core.h`, `module/ship/ship_core.cpp` — `AIR_HOLODECK`, the reactor's state, the feed,
  the refusal, the cost, the save.
- `module/ship/g_ship.cpp` — the `air`/`source` console, `FindSource`, the Operations readout, `g_shipTest 98`.
- `module/ship/console_api.def` — the `air` verb's usage.
- `tests/ship/test_ship_core.cpp` — `TestTheAir`, `--air`.
- `scripts/air-check.sh` — the world-side demonstration (unavailable here; the playtest command).
- `tools/docs/owned_claims.json` — the save-version owner, 59 → 60.
- `docs/gates.md`, `docs/omissions.md`, `docs/ship-systems.md`, `docs/budget-squaring.md`,
  `docs/damage-and-budgets.md`, `docs/lore-ledger.md`, `docs/path-to-playtest.md`,
  `docs/evidence/the-air.md` — the register and the corrected statements.
