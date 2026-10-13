# Evidence: preventive maintenance, the seventh job kind (3.23)

The pass that implements the owner's ruling on **register row 3.23** (`docs/designs-against-code.md`),
recorded 2026-10-12 **in his own words**:

> **"the code's six stay, and maintenance joins them."**

The owning documents are **`docs/crew-work.md`** (*the code's seven kinds, and the ruling on the
seventh*) and **`docs/ship-systems.md`** (*Repairs are the counter-play*). The mechanism the kind reads
is the **deferred-maintenance clock** the model already carried (`System.wear`, `MAINTENANCE_DAYS`);
the failure it prevents is the existing one `UpdatePower` fires at wear 1.

Reproduce with:

```
scripts/maintenance-check.sh          # the demonstration and the falsification, built here
scripts/test.sh                       # the unit suites and every content-free check
scripts/docs-check.sh                 # cited paths, owned numbers, register rows
scripts/check.sh --source-map <a shipped .map> --script-corpus <a directory of shipped .txt scripts>
```

`scripts/check.sh` needs the GDK (`--source-map`, `--script-corpus`), read-only and never written to;
its no-content half is `scripts/test.sh`, run above. The static host runs no engine and holds no game
data, so the rendered console line and the job board are the playtest host's and are named under *What
could not be verified*; the model's own numbers are asserted here.

## What the register said, and the ruling

Row 23 of the register's domain 3 read: *"job kind `"maintenance, repair, build, reclaim"` — `contradicted`
— `JobKind` enum"*. The code's enum carried six — repair, seal, reclaim, build, secure, evacuate — and
the document's *maintenance* was not among them. The register handed it back as a **fork** twice: the
code's six were the four the document names plus two the orders pass appended, and adding or removing a
kind changes the queue's contract. The owner ruled the vocabulary, and the code is the side that moves:
**maintenance joins the six, as the seventh kind.**

## What was built

`module/ship/ship_core.h` — `JOB_MAINTENANCE` appended to `JobKind` (so every existing id is unchanged);
`System.maintaining` and `CrewMember.maintSystem` (derived, never saved); the constants; `MaintenanceOdds`
and `OrderMaintain`. `module/ship/ship_core.cpp` — the queue raising the job, the standing party working
it, the wear clock lowered and its parts spent, the failure-odds line in the log, the order's dispatch
and target. `module/ship/order_set.def` — the class `ORDER_MAINTAIN`. `module/ship/console_api.def` and
`module/ship/g_ship.cpp` — the console `ship order maintain <system>`, the queue naming a maintenance
target as a repair's is named, and `UNDER MAINTENANCE` on the system read. `tools/orders/orders.py` —
`JOB_MAINTENANCE` added to the register check. `tests/ship/test_ship_core.cpp` — `TestMaintenanceJob` and
`--maintenance`. `scripts/maintenance-check.sh` — the guard and the falsification.

**The kind.** Repair is restorative: it acts on a system's **damage** (`1 - health`). The seventh kind is
preventive: it acts on a system's **wear**, the deferred-maintenance clock the model already carried. The
kind is appended last, so a save written before it still names the same six.

**The axis, with the number.** The axis is **wear** — 0 fresh, 1 the failure. A system at or past
**`MAINTENANCE_DUE = 0.20`** carries a `JOB_MAINTENANCE` job; below it there is nothing to do, so the
queue is not decoration. It is *not* the system's damage (a worn system is whole) and *not* its
criticality (that is the queue's own order).

**What it costs, and what it buys.** The job takes crew time like any other — the same standing party
that answers a repair works it, up to **`MAINTENANCE_TEAM_MAX = 2`** hands — and a little material:
**`MAINTENANCE_PARTS_PER_SYSTEM = 2`** spare parts to restore a wholly worn system. What it buys is a
lower chance of the failure it prevents, and the two connect by the clock's own rule run backwards: an
undermanned station climbs the clock at `1 / MAINTENANCE_DAYS` per day, so one engineer working as long —
**`MAINTENANCE_HOURS_PER_SYSTEM = 240 h`** — restores a fully worn system. **`MaintenanceOdds(wear)`** is
that clock's daily hazard, `1 / ((1 - wear) * MAINTENANCE_DAYS)`, and the `work ordered` log line names
the wear and the odds it sets. The weights are `[inv]`, tunable, and stated beside the rule.

**Who may order it.** The same authority surface the other kinds use. `ORDER_MAINTAIN` is a class in the
register (`module/ship/order_set.def`), derived per recipient from post, department and rank and
dispatched like any other order; the post-holder or engineer is offered "overhaul the {post}", resolving
to the worn system. The officer of the deck's line needs **no special case**: the deck's authority is a
property of the giver, not the class (C-11), so a maintenance order from the deck is permitted exactly as
a repair order is. Command's own hook is `OrderMaintain` (`ship order maintain <system>`), which rides
the job rather than a standing field — a system with no wear left has nothing to maintain.

## The demonstration — the model's own output

`test_ship_core --maintenance` (the same strings the check asserts):

```
== the seventh kind: maintenance, preventive where repair is restorative
  the kinds: repair, seal, reclaim, build, secure, evacuate, maintenance; JOB_KIND_COUNT is 7
  the axis: a system's wear (0 fresh .. 1 the deferred-maintenance failure); the threshold MAINTENANCE_DUE is 0.20
  the party: up to MAINTENANCE_TEAM_MAX 2 standing hands; MAINTENANCE_HOURS_PER_SYSTEM 240 h of one engineer restores a wholly worn system (MAINTENANCE_DAYS 10)
  the price: MAINTENANCE_PARTS_PER_SYSTEM 2 spare parts to restore one from full wear; no parts, no upkeep
  the odds it moves: MaintenanceOdds(wear) = 1 / ((1 - wear) * MAINTENANCE_DAYS): wear 0.00 -> 0.1000, 0.20 -> 0.1250, 0.50 -> 0.2000, 0.90 -> 1.0000, 1.00 -> 1.0000

== the same system, maintained and not: the sensors' whole watch lost, fifteen days
  maintained: health 1.00, wear peak 0.20 (the clock never crossed 1), the failure odds peak 0.1250, 4 maintenance job(s) raised
  neglected (no spare parts): health 0.60, wear peak 1.00, the failure odds peak 1.0000, 2 maintenance job(s) raised, failed for want of maintenance: yes

== the order: who may give it, and what it pushes
  the class ORDER_MAINTAIN is 6 of 6 in the register; the pill reads "overhaul the {post}"; it costs "the crew's hours, and the queue's place"
  offered to Chakotay: pill 3 "overhaul the sensors" -> the sensors (wear 0.50)
  dispatched: given 1, job 0, the job's place in the queue -1 (the head)
  the officer of the deck (Tom Paris) may give it to a senior (Chakotay, rank 4): 1

== the save: the wear clock is saved, so the job survives a load
  format 64 (63 -> 64 this pass: the wear clock is saved, so an older save does not load); the blob is 9448 bytes and loads 1; the maintenance job round-trips (1), the party is re-derived
```

**The effect, both ways, is the same system at a later point.** Both ships lose the sensors' whole watch,
so the clock climbs on the same rate. The **maintained** ship's wear never leaves the band the threshold
sets (`wear peak 0.20`), so the deferred-maintenance failure never fires and the health stays whole
(`1.00`); the failure odds the clock sets peak at `0.1250`. The **neglected** ship — no spare parts, so
no upkeep — runs the clock to 1, fails (`health 0.60`, logged `sensors failed for want of maintenance`),
and the odds reach certainty (`1.0000`). The numbers move; nothing is asserted that the model did not
produce.

## Falsification — a maintenance that changes nothing, and the guard catches it

`scripts/maintenance-check.sh` compiles the unit test against a **broken copy** of `module/ship/` in
which the work loop's wear reduction is planted out (`gain = 0.0f`), so the maintenance job is raised and
worked and **the clock does not move** — exactly the job-shaped decoration the brief names. The test must
**fail** on the seventh kind; then the tree must pass:

```
==> maintenance: the falsification (the job runs and the wear does not move; the test must fail)
      FAIL  maintenance: the seventh kind, its axis and its effect: keptPeak <= MAINTENANCE_DUE + 0.05f (line 9614)
      FAIL  maintenance: the seventh kind, its axis and its effect: MaintenanceOdds(keptPeak) < MaintenanceOdds(neglectedPeak) (line 9624)
    the broken copy fails on the seventh kind; the tree passes
```

The same script asserts every demonstration line above, so a silent regression in any of them fails the
check. A guard that cannot fail for the reason we care about is decoration, so the broken shape is
exactly the one the brief names: **the job runs and the odds do not move.**

## The save — what the kind and its state cost

The kind itself costs the format **nothing**: `JOB_MAINTENANCE` is an appended enum value in a job row
the format already carried, and the maintenance party is derived every tick (`System.maintaining`,
`CrewMember.maintSystem`), as the damage-control party is. **The wear clock is the state that had to be
saved.** It accumulates while a station is undermanned and a maintenance job lowers it, so it is history,
not a read; unsaved, a load forgot how far a system had run down and dropped the maintenance job the wear
raised. The pass therefore saves it (a `float` per system, appended at the end of the record) and **moves
the format `63 -> 64`** — the deliberate cost, named: **an older save does not load.** The blob
round-trips byte for byte with a maintenance job standing and the wear clock carried (`loads 1`, the job
`round-trips (1)`), and a save whose queue holds only the six older kinds still loads. Written in the
`SAVE_VERSION` changelog (`module/ship/ship_core.h`); the value and the documents that assert it were
moved together (`tools/docs/owned_claims.json`, `docs/morale.md`, `docs/designs-against-code.md`), and
`scripts/docs-check.sh` holds them.

## The module builds, and carries the symbols

`module/ship/` is compiled here, on the static host, from this repository's own tree. `scripts/
maintenance-check.sh` builds the simulation half directly; the engine is configured against this lane's
module directory and built in a **scratch** directory, and the symbols are asserted **in the built
library, not the tree** — the trap `AGENTS.md` records:

```
$ export PATH=/home/robot/.cache/lwh-venv/bin:$PATH
$ cmake -S /home/robot/upstream/efgame -B /tmp/opencode/efgame-maint -G Ninja -DLWH_MODULE_DIR=$PWD/module
$ cmake --build /tmp/opencode/efgame-maint --target efgame -j4      # [141/141] Linking libefgame.so
$ nm -C /tmp/opencode/efgame-maint/libefgame.so | grep -E "ship::(MaintenanceOdds|OrderMaintain|JobKindName)"
00000000001863c0 t ship::JobKindName(unsigned char)
00000000001b7c30 t ship::OrderMaintain(ship::Ship&, int)
000000000002f92a t ship::OrderMaintain(ship::Ship&, int) [clone .cold]
000000000017c3f0 t ship::MaintenanceOdds(float)
```

The module the engine loads carries the seventh kind. No engine process was left running, and the build
was made in `/tmp/opencode/efgame-maint`, not a shared one.

## What could not be verified

- **Whether a ship that rewards upkeep feels like a ship worth keeping** — the brief's own exclusion,
  the owner's walkthrough. The model is right; the judgement is his.
- **The rendered console line and the job board.** `ship jobs`, `ship order maintain <system>`, the
  `UNDER MAINTENANCE` read and the `ui_lwh_jobs` row are compiled and their sources are checked
  (`scripts/api-check.sh`, `scripts/orders.py`), but the static host runs no engine and draws no screen,
  so the pixels are the playtest host's. What is asserted here is the model's output.

## Judgment calls, named as calls

1. **The axis is wear, not damage.** The brief lists three candidates — wear, a rising failure chance, a
   past-due interval. Wear is the clock the model already carried; "a rising failure chance" is that
   clock read as a hazard, and "a past-due interval" is the same clock with a threshold. Choosing wear
   keeps the kind reading state the simulation already had, which the brief asks for. Recorded in
   `docs/crew-work.md` so it can be overruled.
2. **The threshold is `MAINTENANCE_DUE = 0.20`.** Stated as a number, a fifth of the way to the failure —
   early enough that the standing party can restore the wear before the clock reaches 1, late enough that
   a fresh or well-watched system raises no job. Tunable.
3. **The maintenance rate is the clock's own rule, run backwards.** `MAINTENANCE_HOURS_PER_SYSTEM = 240 h`
   is one engineer working as long as `MAINTENANCE_DAYS = 10` of neglect took; the party of two therefore
   outruns a wholly dead station's wear by two to one. Chosen so the trade is real rather than trivial:
   three hands hold up to three fully-neglected systems, and no more.
4. **It costs material.** `MAINTENANCE_PARTS_PER_SYSTEM = 2` parts to restore a wholly worn system. Named
   because it is what makes "a ship out of spare parts cannot keep itself up" true — and it is the device
   the demonstration uses to withhold upkeep without touching the code. The alternative (crew time alone)
   is defensible; this is stated so it can be overruled.
5. **The maintenance party is the standing party, not the system's watch.** As with a repair, it is the
   engineers on duty with no station of their own (tier one of C-12); maintenance does **not** draw
   post-holders off their stations, because stripping the bridge to change a filter is not the design.
6. **The order rides the job, not a standing field.** A repair's "see first to" is a saved field
   (`orderRepairFirst`); a maintenance order instead lifts the job it raises to the head, and the job
   leaves the queue when the wear is worked to zero. A system with no wear left has nothing to maintain,
   so there is no field to leave standing; `OrderMaintain` is refused for a whole system.
7. **The wear clock is saved (format 63 -> 64).** The alternative — leaving the format and losing the
   wear on load — would make the seventh kind's own state not replay. Saving it is a behaviour change to
   the existing deferred-maintenance loop (a save taken with a system run down now resumes run down), and
   it is the correct one under *saves must replay identically*; the cost, an older save not loading, is
   named above.
8. **`MaintenanceOdds` is a read of the clock, not a second roll.** The failure itself still fires when
   the clock passes 1 (`UpdatePower`); the function is the daily hazard that clock implies, exposed so the
   job's effect on the odds is a number and the log can name it. It is called in play by the `work ordered`
   line, so it is not a test-only hook.
