# Evidence: the nine departments, and the complement as hull data (3.3 and 8.53)

Date: **2026-10-12**. Branch `feat/the-departments`, cut from `testing`. **Static host: no engine, no
game data.** The witnesses are `tests/ship/test_ship_core.cpp` (built and run by `scripts/test.sh`),
`scripts/departments-check.sh`, the header `module/ship/ship_core.h` and the module source. The rendered
game is not in reach here.

Reproduce with:

```
export PATH=/home/robot/.cache/lwh-venv/bin:$PATH    # cmake and the venv (the static host)
scripts/departments-check.sh
scripts/test.sh
scripts/docs-check.sh
```

`scripts/check.sh` needs the GDK (`--source-map`, `--script-corpus`), which is not on this static host;
its no-content half is `scripts/test.sh`, run in full above.

## What the register said, and the rulings

`docs/designs-against-code.md`, two rows handed to the owner and ruled 2026-10-12 in his own words:

- **3.3 — "NINE, as designed."** *"The code's departments expand to the roster. Because content addresses
  posts rather than people, the department list is the vocabulary every scenario speaks."* The row had
  the code at `DEPT_SIZE[5] = {20,50,25,30,16}`, five departments.
- **8.53 — "HULL DATA, as the design says."** *"The complement becomes data a start state can vary — a
  constant contradicts the configurator's own promise that the crew may be wholly fictitious."* The row
  had `const int COMPLEMENT = 141` at `ship_core.h:23`.

Both are the crew's composition, so they land together. The owning documents are
**`docs/crew-manifest.md`** (the nine and their sizes) and **`docs/design-north-star.md`** (the
complement). The declaration form is named because the manifest will be written against it: the nine are
the `DEPT_*` identifiers (`DepartmentName`/`DepartmentByName` the lookup) and the complement is
`Complement(s)`, with `COMPLEMENT_DEFAULT`/`HULL_VOYAGER` the hull's own figure.

## The nine, demonstrated

`test_ship_core --departments` (the same names `DepartmentName` returns):

```
  the nine departments (DEPT_COUNT 9), their sizes for the hull's complement 141:
    Command                      5  (5 in the roster)
    Operations                  13  (13 in the roster)
    Engineering                 42  (42 in the roster)
    Security and tactical       25  (25 in the roster)
    Science                      9  (9 in the roster)
    Medical                     12  (12 in the roster)
    Flight and shuttlebay       10  (10 in the roster)
    Support and logistics       19  (19 in the roster)
    Civilians and attached       6  (6 in the roster)
  the sizes add to 141, the complement
  one lookup: DepartmentByName("security") = 3, DepartmentName -> Security and tactical
```

`TestNineDepartments` asserts the roster's own nine names in the manifest's order, that
`DepartmentByName` resolves each, that an unknown word resolves to \(-1\), and that the sizes sum to
`Complement(s)` and every berth is filled by its own department.

**Five numbers moved, and the reason.** The code staffs every modelled system on **every one of the three
watches** from its own department, and the named command crew all stand **alpha**. A department that owns
systems therefore needs about three times its per-watch posts plus a party, and the named crew's alpha
concentration has to be offset. The manifest's first proposal was short in the two departments that own
the most posts: Engineering 36 leaves two hands spare a watch (the damage-control party needs four), and
Security and tactical 18 leaves three spare on alpha and exactly five on beta (the guard posts four).
They were funded from the departments that own few or no stations. The moves, written into
`docs/crew-manifest.md`:

| department | proposal | code | why |
|---|---|---|---|
| Engineering | 36 | **42** | 10 posts a watch plus the damage-control party |
| Security and tactical | 18 | **25** | 5 posts a watch plus the four-strong guard, eight named crew on alpha |
| Command | 9 | **5** | owns no modelled system |
| Operations | 16 | **13** | owns two posts a watch (comms, transporters) |
| Science | 15 | **9** | owns two posts a watch (sensors), day work |

The nine still add to 141.

## The complement varying, demonstrated

The same command prints both states:

```
  the hull (hull data): Intrepid, default complement 141
  start state [0] CANON          complement 141 (state names  0), 141 crew, 141 fit, loads 1
  start state [3] SKELETON       complement  96 (state names 96), 96 crew, 96 fit, loads 1
```

**What each leaves as a default.** CANON names no complement (`state names 0`) and therefore leaves the
hull's own `HULL_VOYAGER.complement` (141); SKELETON names 96, and `ApplyStartState` rebuilds the roster
to it before the casualties, seats and log seed are worked. `TestComplementIsHullData` asserts exactly
this, that the two complements differ, that `DepartmentSize(d, complement)` still sums to each
complement, and that both save and reload with the complement and the roster intact, byte-for-byte
(`Pack(back) == blob`).

## What a varying complement touched — the honest list

The brief asked for this, and for anything that assumed a constant to be **called out rather than quietly
repaired**:

- **`const int COMPLEMENT` is gone** (`ship_core.h`). It was read at five sites, each now `Complement(s)`:
  the roster size in `BuildRoster`; the crowded-ship term (`0.05 * refugees / complement`); the
  transporter-duplicate bound (`complement + MAX_DUPLICATES`); `Pack`'s decision of which records carry a
  stored name; and `Unpack`'s bounds check on the crew count. `COLLECTIVE_MAX` (one held mind per person)
  was a sixth: it was `= COMPLEMENT`, and is now `Complement(s)` at its three sites, because a larger
  hull may hold more minds than 141.
- **The roster** is built to `s.complement`, and the nine department sizes are scaled to it
  (`DepartmentSize`) by largest remainder, so the parts always add to the whole — a 96-complement hull
  has Engineering 29, not 42.
- **The watch** is the code's own and was called out, not quietly repaired: the named command crew all
  stand alpha, so the generated hands are placed on their department's *thinnest* watch rather than by a
  bare `i % 3`. Without it, a thin department is short of its stations on beta or gamma. This is a
  deterministic change, so a save still replays identically.
- **The save** carries the complement (below).
- **Not assumed, and left alone:** the bays (`BuildShuttles`), the sector, and every other system —
  nothing else reads the crew size. `Ship::CrewFit()` already reported the fit crew, which is the
  statistic attrition moves; the complement is what the roster was built for.

## The save

`SAVE_VERSION` moves **61 → 62** (`module/ship/ship_core.h`): a `uint16` complement, written beside the
configurator flags and **read before the roster is rebuilt**, so a state that fields 96 replays its own
crew. **An older save (61) does not load** — the header rejects a version it does not know rather than
read the complement from the wrong offset, and the payload after the flags has shifted by two bytes. The
cost is measured:

```
CANON            complement=141 crew=141 blob=9652 loads=1
THE CHAIR        complement=141 crew=141 blob=10029 loads=1
ALL-FICTITIOUS   complement=141 crew=141 blob=9667 loads=1
SKELETON         complement= 96 crew= 96 blob=7267 loads=1
```

The four states' own save sizes differ because their logs and casualties differ; the point the ruling
needs is that all four load, and that a 96-complement blob round-trips its own roster
(`TestComplementIsHullData`, `TestStartStateSaveRoundTrip`).

## Falsification — break one of the two, and the check catches it

`scripts/departments-check.sh` compiles the unit test against a **broken copy** of `ship_core.cpp`, once
per ruling, and requires it to fail for the ruling's own reason:

```
==> departments: the falsification (a department not in the roster; the nine must fail)
      FAIL  the nine departments, and their sizes adding to the complement: std::string(DepartmentName(static_cast<Department>(d))) == NINE[d] (line 457)
    the broken copy fails on the nine; the tree passes
==> departments: the falsification (Complement ignores its data; the complement must fail)
      FAIL  the complement is hull data, and a start state may vary it: Complement(b) == named.complement (line 496)
      FAIL  the complement is hull data, and a start state may vary it: Complement(b) != Complement(a) (line 497)
    the broken copy fails on the complement; the tree passes
```

The first plants Science under the old five-department word **"Sciences"**, which the roster does not
have; the second makes `Complement` return the one default instead of the ship's own data. Both are the
exact shapes the brief names. The same script asserts every line of the demonstration above, so a silent
regression in any of them fails the check.

## The module builds, and carries the symbols

The module half is compiled on the static host from this repository's `module/`, against a scratch build
tree, and the symbols asserted **in the built library, not the tree** — the trap `AGENTS.md` records:

```
$ export PATH=/home/robot/.cache/lwh-venv/bin:$PATH
$ cmake -S ../upstream/efgame -B /tmp/opencode/efgame-build -G Ninja -DLWH_MODULE_DIR=$PWD/module
$ cmake --build /tmp/opencode/efgame-build --target efgame -j4      # [141/141] Linking libefgame.so
$ nm -C /tmp/opencode/efgame-build/libefgame.so | grep -E "ship::(Complement|DepartmentName|DepartmentByName|DepartmentSize)"
000000000017b300 t ship::Complement(ship::Ship const&)
0000000000178000 t ship::DepartmentName(ship::Department)
00000000001780f0 t ship::DepartmentSize(ship::Department, int)
0000000000178030 t ship::DepartmentByName(char const*)
```

The four symbols are in `libefgame.so`; `scripts/hooks-check.sh` fixes the header's public surface at
**489** functions and requires the register to agree. No engine process was left running, and the build
was made in a scratch tree (`/tmp/opencode/efgame-build`) rather than the shared checkout.

## What could not be verified

- **Whether nine departments *reads* as a ship's organisation rather than a list.** That is the owner's
  judgement, named as his; no transcript answers it.
- **The rendered menu and the configurator screen.** The static host runs no engine and holds no game
  data, so the four proving cases were exercised through the model, not the rendered menu. The SKELETON
  state's presence in the selector's list (`StartStateAt(3)`) is a state fact, asserted; a person seeing
  four rows and a complement on the screen is the playtest host's.
- **Any save taken before this pass.** By design it does not load (format 62).

## Judgement calls, named as calls

1. **The five moved numbers.** The manifest calls its sizes *"ours — a proposal, to be tuned against the
   ship's actual needs"*; the code's needs are the watch model above. I moved the minimum to satisfy the
   stations, the damage-control party and the guard, and funded it from the station-light departments. A
   reader may move them differently — the values live in one array and `DepartmentSize` keeps the sum
   honest.
2. **The watch-placement change.** Placing generated hands on the thinnest watch is a change to the
   roster, recorded here and in the code; it is what lets a department of 25 with eight named alpha
   stand five posts on beta.
3. **The triage fix.** The roster change exposed a pre-existing medical pathology: with a mass casualty,
   a patient part-way through treatment was displaced every tick by a marginally worse arrival, so
   nobody's course ever completed and `TestCasualties`' "sickbay returns its first patients" stopped
   holding. `Patients` now keeps a patient whose course is under way ahead of a new arrival (within each
   group the worst/the most senior is still chosen). It is a small change to the medical model, named
   because it is not the crew's composition — the brief's subject.
4. **The save round-trip test.** The same exposure moved the fight's transient counters
   (`defenders`/`stripping`/`sealing`/`firefighting`), which the next tick recomputes from the pre-fight
   boarder count; `TestBoarding` now zeroes those four derived fields on both sides before its
   whole-state comparison, rather than pretending a recomputed field round-trips. The underlying
   path-dependence is pre-existing and already recorded in `docs/evidence/the-sector-split.md`.
5. **The complement names.** `HULL_VOYAGER` names the one hull the code carries (Voyager, an Intrepid);
   `COMPLEMENT_DEFAULT` is its figure in the owner's `owned_claims` form. A second hull is a row.

## Register corrections made in the same pass

- **`docs/designs-against-code.md`** — rows **3.3** and **8.53** move from `contradicted` to
  `implemented`, each with its disposition and its closed marker
  (`D3-3` → `TestNineDepartments`, `D8-53` → `TestComplementIsHullData`).
- **`docs/crew-manifest.md`** — the nine departments and the tuned sizes written in as ours, with the
  five moves and their reason.
- **`docs/design-north-star.md`** — the complement ruling written in as built.
- **`docs/the-entry-point.md`** and `scripts/configurator-check.sh` — the fourth proving case
  (the under-crewed hull).
- **`docs/morale.md`** and **`tools/docs/owned_claims.json`** — the asserted save version moves to 62.
- **`docs/hook-register.md`** — four hooks added (`Complement`, `DepartmentName`, `DepartmentByName`,
  `DepartmentSize`), the readers re-derived 208 → 212, the register 485 → **489**.
