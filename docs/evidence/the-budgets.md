# The ship's budgets squared — what was built and what was measured

`docs/budget-squaring.md`, applied on `feat/the-budgets` (cut from `testing`), 2026-10-07.

**Observed, and the command that produced it.** All figures below are the output of the commands named
beside them, on this tree. Save format is **50** (the five added systems raise `SYS_COUNT`, so older saves
are invalid — the cost named in `docs/budget-squaring.md`, Part six).

## The demand table as built

`tests/ship/test_ship_core.cpp` (`TestBudgetShedSequence`, `Spec()` for every system). Sum **1,730**.

| system | deck | demand | keep priority |
|---|---|---|---|
| life support | 12 | 60 | 0 |
| structural integrity | 11 | 80 | 1 |
| inertial dampers | 11 | 40 | 2 |
| computer core | 10 | 60 | 3 |
| warp drive | 11 | 400 | 4 |
| impulse drive | 10 | 100 | 5 |
| sensors | 8 | 60 | 6 |
| shields | 1 | 200 | 7 |
| phasers | 1 | 150 | 8 |
| torpedo launchers | 10 | 30 | 9 |
| astrometrics *(new)* | 8 | 60 | 10 |
| navigational deflector | 11 | 50 | 11 |
| communications | 1 | 20 | 12 |
| transporters | 4 | 60 | 13 |
| science labs *(new)* | 8 | 60 | 14 |
| sickbay | 5 | 30 | 15 |
| turbolifts | 1 | 20 | 16 |
| tractor beam | 10 | 60 | 17 |
| gravity plating *(new)* | 12 | 30 | 18 |
| replicators | 2 | 60 | 19 |
| holodecks | 6 | 60 | 20 |
| non-essential lighting *(new)* | 12 | 20 | 21 |
| cargo handling *(new)* | 10 | 20 | 22 |

The five additions total **+190**; the old eighteen totalled 1,540, so **demand is 1,730**.

## The supply table as built

`TestCrystalCeilingScalesOutput`; `Ship::PowerCapacityFresh()` / `PowerCapacityNow()`.

| source | fresh | at 0.85 | at 0.70 | at 0.55 |
|---|---|---|---|---|
| warp core (1,400 × ceiling) | 1,400 | 1,190 | 980 | 770 |
| impulse reactors | 250 | 250 | 250 | 250 |
| auxiliary fusion | 90 | 90 | 90 | 90 |
| emergency batteries | 60 | 60 | 60 | 60 |
| **supply** | **1,800** | **1,590** | **1,380** | **1,170** |
| against 1,730 | **+70** | −140 | −350 | −560 |

A fresh ship therefore leaves port **seventy EPS in hand** (`PowerCapacityFresh() − demand == 70`). The
warp core's output scales with the dilithium crystal's ceiling: each recomposition buys life and
permanently lowers the plant (the owner's mechanic).

## The shed sequence as it actually runs

A purpose-built readout (`/tmp/opencode/shed.cpp`, compiled against `module/ship/ship_core.cpp`) stepping
the crystal ceiling and printing every system whose allocation is below its demand. Reproduced by
`TestBudgetShedSequence` at 1.00, 0.85, 0.70 and 0.55 as the *set* of shed systems.

| ceiling | supply | running | shed |
|---|---|---|---|
| 1.00 | 1,800 | all 23 | — |
| 0.85 | 1,590 | …astrometrics, science labs, gravity plating | replicators (20/60), holodecks, non-essential lighting, cargo handling |
| 0.70 | 1,380 | …transporters, astrometrics | sickbay, turbolifts, tractor beam, replicators, holodecks, science labs (10/60), gravity plating, non-essential lighting, cargo handling |
| 0.55 | 1,170 | …phasers | torpedo launchers (20/30), navigational deflector, communications, transporters, sickbay, turbolifts, tractor beam, replicators, holodecks, astrometrics, science labs, gravity plating, non-essential lighting, cargo handling |
| 0.50 | 1,100 | …impulse drive | phasers (100/150) and everything above |
| 0.30 | 820 | …impulse drive | shields (20/200) and everything above |
| 0.20 | 680 | life support, structural integrity, inertial dampers, computer core, **warp drive** | everything else, including the impulse drive (40/100) |

Read the last two rows together: **the warp drive is the last non-critical system still fed.** At ceiling
0.20 the shields, the sensors and the impulse drive are all gone and the warp drive runs whole. This is the
demonstration the brief asks for: the ship gives up going home last of all, after even the comforts.

## Coreless

`TestCorelessShip`; the same readout after `DamageSource(SRC_WARP_CORE, 1.0)`:

```
coreless=1 supply 400 allocated 390
   life support 60/60
   structural integrity 80/80
   inertial dampers 40/40
   computer core 60/60
   impulse drive 100/100
   navigational deflector 50/50
```

The critical four, the impulse drive and the deflector are 390 of the 400 that remain (impulse reactors
250 + auxiliary fusion 90 + batteries 60), and **nothing else on the ship runs** — no weapons, no sensors,
no comms. The batteries, now the last source and covering any shortfall rather than the critical systems
alone, are what lets the survival set fit.

## The batteries alone

`TestBatteriesKeepTheCrewAlive`: with every reactor offline the batteries' 60 EPS holds **life support at
exactly full** — and nothing else — draining in **three hours**. Not a system short of it.

## Task B — the two things the player sees

- **The engineering console shows the budget twice.** The console header now reads
  `POWER <n> SUPPLIED  <n> ALLOCATED   BUDGET <fresh> FRESH  <now> NOW` (`g_ship.cpp`, `Publish`), and
  `Describe` carries `budget 1800 fresh, 1800 now`. Seen live in the S2 console run below: at red alert,
  `BUDGET 1800 FRESH  1800 NOW`.
- **The torpedoes start at 38 and deplete.** `TestTorpedoComplement`: a new ship carries 38, and 38
  torpedoes fired one at a time bring the count to 0, after which `FireTorpedo` is refused. Canon's
  waypoints (eleven at Day 32, six at Day 226 of Year of Hell) are recorded in `docs/ship-systems.md` and
  the ledger.

## Task C — canon's three checks

1. **Operable with 100 crew** — **built.** `PostsNeeded() == 22`; `CoverWithCrew(100)` gives a watch of
   33 hands covering all 22 posts, so the ship is fully manned at a hundred. At 60 crew a watch has 20
   hands, covers 20 posts, and the critical set still holds — degraded but flyable.
2. **At least one deck holds air independently of the main grid** — **measured, not built: the largest
   structural gap.** The size, in numbers: the model has **one** `SYS_LIFE_SUPPORT` and a single global
   `support` applied to all fifteen decks in `UpdateDecks`; there are **20** references to
   `SYS_LIFE_SUPPORT`, **17** to a deck's `atmosphere`, and **11** to `MinutesOfAir` across four files
   (`module/ship/ship_core.{h,cpp}`, `module/ship/g_ship.cpp`, `module/crew/g_crew.cpp`). To let a deck
   hold air on its own you must give the deck its own life-support state, give the three canon sources
   (shuttlecraft, life support, the holodecks) per-deck delivery, teach `UpdatePower`/`UpdateDecks`/
   `MinutesOfAir`/`SystemCondition` the per-deck case, surface it on the console and the deck adapter, and
   bump the save again. That is a gate-sized change — a new per-deck source model, not a string — which is
   why it is reported rather than half-built. The acceptance is met for the two small checks; this one
   needs its own lane.
3. **The holodeck matrix is a trap** — **built.** `JumpStartFromHolodeck` (`ship jumpstart`): with the
   main grid up it is refused and changes nothing; with the core gone it returns a battery charge **and
   wrecks half the ship's systems**, the holodecks among them. `TestHolodeckTrap` asserts both the charge
   and the damage.

## The commands, and what they returned

```
scripts/test.sh            -> all checks passed (crew, ship, tools, python, shellcheck, 19 patches)
scripts/check.sh --source-map build/gdk/maps/eliteforce_voyager_maps/voy1.map \
                 --script-corpus build/gdk/scripts      -> ALL PASS; 2,016/2,024 scripts, 8 known rejections
scripts/s2-check.sh        -> PASS reloaded ship matches saved; PASS alert/damage/priority persisted;
                              PASS the Engineering console changed the ship (BUDGET 1800 FRESH 1800 NOW seen)
tests/ship/test_ship_core  -> ship_core: all checks passed
```

The remaining `scripts/*-check.sh` engine checks are being run over the tree; their results are recorded
below as they land.

## What could not be verified

- Whether any of it is **fun**, and whether the ship feels tight rather than punishing. The brief reserves
  that for the owner's walkthrough.
- The **canon torpedo moments** (eleven, six) as built *moments*: the count depletes and the number is 38,
  but no script stages the Day 32 / Day 226 scenes — that is content, not the model.
- **Deck-by-deck air** (Task C2): measured and sized, not built, as above.

## Judgement calls, named

1. **The batteries are the last source and cover any shortfall, not the critical systems alone.** The
   document's fresh supply of 1,800 and its shed arithmetic both count the batteries in the grid; with the
   old "critical only" rule the 0.85 row could not match the table. This is the change that makes the
   numbers work.
2. **Coreless is a survival allocation, not a priority outcome.** With one static priority order the
   document's 0.55 table (the deflector shed) and its Part 4a coreless set (the deflector kept) cannot both
   hold. The priority order produces the shed table exactly; a separate rule — with the warp core gone the
   ship runs only the survival set — produces coreless exactly. Reported because it is an extra mechanism.
3. **The phaser bank's opening setting is now STUN.** The ledger says the bank's nominal demand is a stun
   shot, and the budget table is nominal; at the old default (KILL) the opening full load was 1,805 against
   the 1,800 grid — five over, so no headroom. STUN restores the seventy. Two combat tests now set KILL
   explicitly, because a fight is not the peacetime setting.
4. **The five new systems carry no posts** (`crewNeeded` 0): the comfort loads are automated and the science
   runs from the existing sensor watch, so "the systems carry 22 posts" (the brief's figure) stays true.
5. **Systems on a deck share a boarding party** (unchanged code, new consequence): deck 8 now carries three
   systems, so two boarders hack each more slowly. `TestBoarding` was updated to the six-minute seize
   threshold. This is the model behaving as designed, not a regression.
