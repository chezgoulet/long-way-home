# Evidence: the shield sections — canon's six arcs (C-14)

The pass that implements the owner's ruling on **C-14** (`docs/designs-against-code.md`), recorded
2026-10-12 in his own words: *"BUILD THE SET — canon's six arcs."* The owning document is
**`docs/ship-systems.md`** (tier 2, *Shields*); the routing the ruling changes is `docs/damage-and-budgets.md`;
the declaration it exists for is `docs/scenario-atlas.md` (*Systems stressed*); the canon figures,
with their sources, are `docs/lore-ledger.md`.

Reproduce with:

```
export PATH=/home/robot/.cache/lwh-venv/bin:$PATH    # cmake, for the unit-test build (the static host)
scripts/shields-check.sh
scripts/test.sh
```

`scripts/check.sh` needs the GDK (`--source-map`, `--script-corpus`), which is not on this static host;
its no-content half is `scripts/test.sh`, which is run in full above. The static host runs no engine and
holds no game data, so the rendered console frame is the playtest host's and is named as such under
*What could not be verified*; the read-out's own text is asserted here.

## What the register said, and the ruling

The register's row 15 read: *"fourteen external grids; six sections" (`ship-systems.md:39`) —
`contradicted` — `module/ship/ship_core.h` one scalar*. The C-14 entry quoted the code: `float
shieldStrength = 1.0f;` — **one number, used by the combat path; no section, no grid, no per-arc
state.** The document was ahead of the build. The owner's ruling settles the fork the register had
handed him: the set is built, and the codeside moves.

## What was built

`module/ship/ship_core.{h,cpp}` — the state, the routing and the read-out; `module/ship/g_ship.cpp` —
the console verb `ship shield` and the published read-out; `module/ui/ui_lwh_engineering.cpp` — the
Tactical console draws the set; `module/ship/console_api.def` — the enumerated `shield` verb;
`tests/ship/test_ship_core.cpp` — `TestShieldSections` and `--shields`; `scripts/shields-check.sh` —
the guard.

- **The state.** `enum ShieldSection { SHIELD_FWD, SHIELD_PORT, SHIELD_STBD, SHIELD_AFT, SHIELD_DORSAL,
  SHIELD_VENTRAL, SHIELD_SECTION_COUNT }` and `float shields[SHIELD_SECTION_COUNT]`, canon's arcs in
  canon's order. **The scalar is gone** — the declaration and every read of `shieldStrength` are
  removed.
- **The names.** `ShieldSectionName` gives each arc its canon word (*forward, port, starboard, aft,
  dorsal, ventral*); `ShieldSectionByName` resolves a word (short or full, case-insensitive) to its
  arc — the one lookup the console and, later, the manifest use.
- **The routing.** `ShieldHit(s, sec, amount)` strips the named arc and returns the overflow that
  reaches the hull. Enemy fire calls it with `ShieldSectionForHit(s)` — the arc the ship's own hit
  counter names (`hits % 6`) — because no bearing is modelled. A hit that reaches no section, and one
  that reaches all of them, are the two broken shapes the falsification plants below.
- **The gates.** `ShieldsDown(s)` is true only when every arc is down. Boarders cross on it, and the
  transporter needs it (`TransportAway`).
- **The read-out.** `ShieldReadout(s)` is the six arcs as one line; `g_ship.cpp` publishes it in
  `lwh_ship_shields`, and the Tactical console draws it (below). `Describe` carries it too.

## The demonstration — the model's own output

`test_ship_core --shields` (the same strings the screen draws):

```
  the six sections (SHIELD_SECTION_COUNT 6): forward port starboard aft dorsal ventral
  the read-out the Tactical console draws: SHIELDS  FWD 100%  PORT 100%  STBD 100%  AFT 100%  DORSAL 100%  VENTRAL 100%
  a hit of 60% on the port arc: port 100% -> 40%, overflow 0%; the other five unchanged
  after:  SHIELDS  FWD 100%  PORT 40%  STBD 100%  AFT 100%  DORSAL 100%  VENTRAL 100%
  unrouted: hits 0, so a shot with no section named strikes the forward arc
  unrouted: hits 2, so the next such shot strikes the starboard arc
  all six arcs down: ShieldsDown 1 (the boarding and beaming gate reads this)
  save: format 61, the blob is 9123 bytes and loads 1; the same blob at version 60 loads 0
```

Read as evidence rather than a screenshot, the lines are the acceptance:

- **the six sections, named** — `forward port starboard aft dorsal ventral`, canon's order;
- **a hit on a named arc** — the port arc falls `100% -> 40%`, and **the other five unchanged**;
- **the unrouted hit, with where it goes** — no section named, so it strikes the arc the hit counter
  names (`forward` at `hits 0`, `starboard` at `hits 2`), deterministically;
- **the gate** — with every arc down `ShieldsDown` is `1`;
- **the save** — format `61` loads, and the same blob at version `60` does not.

**And the scalar is gone**, by search:

```
$ grep -rn shieldStrength module/ tests/ ; echo $?
1
```

`grep` exits `1` — no match — so nothing in the module or the tests reads the old one number. The
`--shields` assertion set and this search are held by `scripts/shields-check.sh`, so a silent
regression fails the check.

## The visible effect — the read-out

The read-out is part of the deliverable, not a follow-up. The Tactical console draws the six arcs:
`module/ui/ui_lwh_engineering.cpp` reads `lwh_ship_shields` (which `g_ship.cpp` fills from
`ship::ShieldReadout`) and draws the line as it stands, so what a person reads is what the damage
routed to:

```
SHIELDS  FWD 100%  PORT 100%  STBD 100%  AFT 100%  DORSAL 100%  VENTRAL 100%
```

After a hit on the port arc the same surface reads `PORT 40%` and the other five still `100%`. The
console can also *name* the arc it hits — `ship shield <fwd|port|stbd|aft|dorsal|ventral|auto> <0..1>`
— so a person and a scenario address the same six names.

## The save

The set costs more than the scalar, and the cost is named. `SAVE_VERSION` moves **60 → 61**
(`module/ship/ship_core.h`): one `float` becomes six in canon's order, **+20 bytes** of ship state.
**An older save (60) does not load** — `Unpack` rejects a version it does not know rather than misread
the wider set, exactly as every bump has invalidated the one before. The transcript above proves both
directions: the new blob loads (`loads 1`), the same blob at version 60 is refused (`loads 0`), and
`TestShieldSections` round-trips the six arcs byte-for-byte through `Pack`/`Unpack`.

## Falsification — break the routing, and the check catches it

`scripts/shields-check.sh` compiles the unit test against a **broken copy** of `ship_core.cpp` in which
the routing no longer reaches one arc — the hit strips **every** arc instead of the named one — and
requires the test to **fail** on the routing; then it requires the tree to pass:

```
==> shields: the falsification (the routing broken so a hit reaches every arc; the test must fail)
      FAIL  shields are canon's six arcs, and damage routes to one: s.shields[SHIELD_FWD] == 1.0f && ... VENTRAL == 1.0f
      FAIL  shields are canon's six arcs, and damage routes to one: !ShieldsDown(s)
      FAIL  shields are canon's six arcs, and damage routes to one: changed == 1
    the broken copy fails on the shield routing; the tree passes
```

The same script asserts each transcript line above, so a silent regression in any of them fails the
check. A guard that cannot fail for the reason we care about is decoration, so the two broken shapes
in the brief — a hit that reaches no section, and one that reaches all of them — are the ones the
falsification plants.

## The module builds, and carries the symbol

The module half (`g_ship.cpp`'s published read-out and the console verb, the UI's draw) is compiled
here, on the static host, from this repository's `module/`, and the symbol is asserted **in the built
library, not the tree** — the trap `AGENTS.md` records. The engine is configured against this lane's
module directory (the cache's default pointed at another clone):

```
$ export PATH=/home/robot/.cache/lwh-venv/bin:$PATH
$ cmake -S ../upstream/efgame -B /tmp/opencode/efgame-build -G Ninja -DLWH_MODULE_DIR=$PWD/module
$ cmake --build /tmp/opencode/efgame-build --target efgame -j4      # [141/141] Linking libefgame.so
$ cmake --build /tmp/opencode/efgame-build --target efui  -j4       # [28/28]  Linking libefui.so
$ nm -C /tmp/opencode/efgame-build/libefgame.so | grep -E "ship::Shield(SectionName|SectionByName|sDown|SectionForHit|Hit|Readout)"
000000000017a560 t ship::ShieldsDown(ship::Ship const&)
000000000017a5d0 t ship::ShieldReadout[abi:cxx11](ship::Ship const&)
000000000017a410 t ship::ShieldSectionName(ship::ShieldSection)
000000000017a440 t ship::ShieldSectionByName(char const*)
000000000017a5a0 t ship::ShieldSectionForHit(ship::Ship const&)
000000000019f2f0 t ship::ShieldHit(ship::Ship&, ship::ShieldSection, float)
```

All six functions are in `libefgame.so`, and `module/ui/ui_lwh_engineering.cpp` compiled into
`libefui.so` (`[25/28] Building CXX object ... ui_lwh_engineering.cpp.o`), so the surface the person
reads is in the built UI library. No engine process was left running, and the build was made in a
scratch directory (`/tmp/opencode/efgame-build`) rather than the shared `../upstream/efgame/build-linux`
the cache had pointed at `lwh-reachability`.

## Judgement calls, named as calls

1. **The unrouted hit goes to the arc the hit counter names (`hits % 6`).** The model has no bearing,
  so a hit with no section named must go somewhere defensible and deterministic (a save must replay
  it). The counter advances when a hit gets through an arc to the hull, so unnamed fire presses one
  arc until it falls, then moves on. Stated here and in `docs/ship-systems.md` so it can be overruled.
2. **Each arc recharges at the shield system's own rate.** `SHIELD_RECHARGE_MINUTES` restores *an arc*
  from nothing, which is how the constant already read. The consequence, flagged for the owner: a lone
  attacker must now roll through six arcs rather than break one number, so the per-arc set makes the
  ship tougher against a single opponent — a real change to the engagement's arithmetic, and the
  owner's to tune. This is the one balance consequence of building the larger option.
3. **The gates read the whole set, not one arc.** We do not model the bearing of an incoming beam or a
  boarding party, so a single arc still up is still a shield: boarders cross only with every arc down,
  and the transporter needs the same. Recorded so it can be overruled.
4. **The names and labels are ours where canon is orthographic.** Canon names the three axes
  (fore/aft, port/starboard, dorsal/ventral); `fwd`/`stbd` are the console's short forms, and the
  read-out's `FWD`/`PORT`/`STBD`/`AFT`/`DORSAL`/`VENTRAL` labels are chosen to fit the 640px console
  face. One lookup (`ShieldSectionByName`) accepts both forms, so a name means one thing.
5. **The fourteen grids are deliberately not built.** The ruling names six sections, and the brief
  forbids the grids. The register's row 15 is marked **partial** for exactly this: six sections built,
  the grids as they are.

## What could not be verified

- **Whether per-arc damage *feels* like being hit on a particular quarter.** That is the owner's
  judgement, named as his; no transcript can answer it.
- **The rendered console frame.** The static host runs no engine and holds no game data, so
  `ui_lwh_engineering.cpp` was compiled (the module build above) but not drawn here. The read-out's
  own text — the deliverable — is asserted above; the pixel check is the playtest host's, as every
  other screen's is.
- **The console verb in the running engine.** `ship shield` is in the dispatch and enumerated in
  `module/ship/console_api.def` (`scripts/api-check.sh` holds the two together), and the module builds,
  but it was not driven through `Svcmd_Ship_f` on this host.

## Register corrections made in the same pass

- **`docs/designs-against-code.md` C-14** — disposition added: built, with its marker
  (`state=closed marker=symbol:module/ship/ship_core.h:SHIELD_SECTION_COUNT`). The domain row 15 moves
  from `contradicted` to **partial** (six sections built; the grids deliberately not).
- **`docs/ship-systems.md`** — the ruling written into the owning document as ours, with the two
  flagged calls and a row in *Where canon is silent*.
- **`docs/scenario-atlas.md`** — *Systems stressed* notes that the shields now reach the six named arcs.
- **`docs/morale.md`** and **`tools/docs/owned_claims.json`** — the asserted save version moves to 61
  (the 2026-10-11 correction kept as history and extended), so the number cannot go stale.
- **`docs/hook-register.md`** — six hooks added (`ShieldHit`, `ShieldSectionName`, `ShieldSectionByName`,
  `ShieldsDown`, `ShieldSectionForHit`, `ShieldReadout`), the section counts re-derived, and the
  register's own count moved 479 → **485**.
