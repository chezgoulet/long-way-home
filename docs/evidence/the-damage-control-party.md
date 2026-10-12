# Evidence: the damage-control party, three tiers (C-12)

The pass that implements the owner's ruling on **C-12** (`docs/designs-against-code.md`), recorded
2026-10-12 **in his own words**:

> *"There should be standing repair team ready to dispatch, and more egregious repairs will take more
> people, which can take them off of their stations."*

The owning documents are **`docs/crew-work.md`** (*the crew's work*) and **`docs/ship-systems.md`**
(*Repairs are the counter-play*); what a repair *is* in this game is `docs/damage-and-budgets.md`
(*Damage control is the third kind of work*); the departments the party names its people from are
`docs/crew-manifest.md` (the roster's nine).

Reproduce with:

```
export PATH=/home/robot/.cache/lwh-venv/bin:$PATH    # cmake, for the unit-test build (the static host)
scripts/damage-control-check.sh
scripts/test.sh
scripts/docs-check.sh
scripts/check.sh --source-map <a shipped .map> --script-corpus <a directory of shipped .txt scripts>
```

`scripts/check.sh` needs the GDK (`--source-map`, `--script-corpus`); the extraction on this host is
the GDK tree under a sibling lane's `build/gdk` (read-only, never written to). Run there, `check.sh`
**exits 0** — the entity dictionary, the validator negative tests, the compiler corpus (2408 files:
2394 read back and the documented 14 rejections), and the script-reference census all pass; its
no-content half is `scripts/test.sh`, which is run above. The static host runs no engine and holds no
game data, so the rendered console line is the playtest host's and is named as such under *What could
not be verified*; the model's own numbers are asserted here.

## What the register said, and the ruling

The register's row 31 read: *"assigning a repair empties a station" (`ship-systems.md:139`) —
`contradicted` — `module/ship/ship_core.cpp:1744-1746`*. The C-12 entry quoted the code: *"An engineer
on duty with no station of their own joins the damage-control party"*; a post-holder was counted at
their station first, and `ORDER_SEE_TO` only set a queue priority. The register called it a **fork**
and handed it back twice — the stationless party was the code's own deliberate intent, and two
documents promised a cost the code did not charge. The owner ruled with all three of the options at
once, and the code is the side that moves.

## What was built

`module/ship/ship_core.{h,cpp}` — the three tiers; `module/ship/g_ship.cpp` — the console read
`ship party` and the published `lwh_ship_damage_control`; `module/ship/console_api.def` — the
enumerated `party` verb; `tests/ship/test_ship_core.cpp` — `TestDamageControlParty` and
`--damage-control`; `scripts/damage-control-check.sh` — the guard.

- **Tier one, the standing party.** The crew already in the damage-control organisation — the
  engineers on duty with no station of their own, plus the battle bill's damage-control hands — answer
  a dispatched repair. The dispatch is `OrderRepairFirst`, the queue's own order; **the queue's
  priority rules are unchanged** (this adds a party, it does not replace the queue).
- **Tier two, severity scales the party.** `RepairPartySize(severity)` is the rule. The axis is the
  system's **damage** — the fraction of it destroyed, `1 - health` — named as our call. Up to the
  threshold it is the party as it stands (`REPAIR_TEAM_MAX`, 3); past it, one hand per sixth of the
  system gone, to `REPAIR_PARTY_LARGE`, 6 for a wholly destroyed system.
- **Tier three, the cost.** Past `REPAIR_SEVERITY_LARGE`, a large job draws the hands the standing
  party cannot give **off the stations of the watch** — the owning department's first, then any
  department, because the organisation "pulls from every department" (`docs/crew-manifest.md`). A
  post left short delivers less and wears. The party names its people in the roster's nine
  (`DamageControlLine`), with the post they left.
- **No parts, no large job.** A job that cannot be worked does not hold hands off their stations.
  (The standing party is still *assigned* as it always was; only the poaching is gated.)

## The demonstration — the model's own output

`test_ship_core --damage-control` (the same strings the console `ship party` draws):

```
  severity is the system's damage (1 - health), 0..1; the threshold REPAIR_SEVERITY_LARGE is 0.50
  the standing party as it stands (REPAIR_TEAM_MAX) is 3; a wholly destroyed system takes REPAIR_PARTY_LARGE 6
  RepairPartySize(severity): 0.25 -> 3, 0.50 -> 3, 0.60 -> 4, 0.75 -> 5, 0.90 -> 6, 1.00 -> 6
  tier one: dispatched to the sensors, the standing party answers: 3 hands (the party as it stands, severity 0.4)
  tier two: the sensors severed to severity 0.90 take 6 hands (the standing party's 3, plus 3 off their posts)
  tier three: the party -- sensors 6 hands (Engineering 3, Operations 1 off their posts, Security and tactical 1 off their posts, Science 1 off their posts)
    Tuvok                (Security and tactical) left the shields: manned 1 -> 0 of 1, output 0.00 -> 0.00
    Tom Paris            (Operations) left the transporters: manned 1 -> 0 of 1, output 1.00 -> 0.50
    Crewman 090          (Science) left the sensors: manned 2 -> 1 of 2, output 1.00 -> 0.09
  the departments (the roster's nine), quoted: life support 6 hands (Engineering 3, Engineering 3 off their posts); sensors 6 hands (Engineering 1, Operations 2 off their posts, Security and tactical 1 off their posts, Science 1 off their posts, Medical 1 off their posts)
  save: format 62 (unchanged by this pass), the blob is 9394 bytes and loads 1; the party is re-derived, not stored
```

Read as evidence rather than a screenshot, the lines are the acceptance:

- **Tier one, the standing party answering a dispatch.** A repair ordered seen-to on the sensors is
  answered by **3 hands, all of them from the standing party** (`dcFromPost == SYS_COUNT`: they held
  no station). Severity 0.4 is one of the small jobs, so the party is the same size whether it is
  dispatched or not — the queue decides *which*, the party answers.
- **Tier two, severity scaling, with the size shown.** The scale is a scale, not one number:
  `0.25 -> 3, 0.50 -> 3, 0.60 -> 4, 0.75 -> 5, 0.90 -> 6, 1.00 -> 6`. On a real job, the sensors
  severed to 0.90 take **6 hands**: the standing party's 3, **plus 3 off their posts**.
- **Tier three, a post-holder off a station, and the station's emptiness.** Three hands left stations
  for the sensors: Tuvok's shields, Tom Paris's transporters, and Crewman 090's sensors. The station
  each left is visible in the simulation's own fields: `manned 1 -> 0 of 1` for the transporters,
  `manned 2 -> 1 of 2` for the sensors.
- **The departments, quoted.** The party line names each hand's department from the roster's nine —
  `Engineering`, `Operations`, `Security and tactical`, `Science`, `Medical` — and marks the hands
  drawn off their posts. The literal line is in the transcript above.

## The threshold, as a number, and the reasoning

**`REPAIR_SEVERITY_LARGE = 0.50`: a job is *large* when the system is more than half gone.**

The reasoning, recorded so it can be overruled:

1. **The axis is damage, and half is the boundary the code already uses for "more whole than not".**
   A system past half destroyed is one where the remaining rebuild is larger than what stands:
   `HIJACKED` (`0.5`) and `ASSIMILATED` (`0.5`) are the same line drawn elsewhere in this model.
2. **It is the point where the standing party alone can no longer absorb the job inside a watch.** A
   destroyed system is `REPAIR_HOURS_PER_SYSTEM` (6 hours) of one engineer, so a system at half health
   is 3 hours of one engineer, or one hour for the party of three. Below the line the party patches it
   and returns to the routine; at and past the line it is a rebuild, and the watch itself must be
   spent. That is the tier-2 scaling and the tier-3 poaching beginning at the same threshold.

The sizes are chosen with it: the party **doubles** for a wholly destroyed system (`REPAIR_TEAM_MAX`
3 → `REPAIR_PARTY_LARGE` 6). Both numbers are `[inv]`, tunable, and stated in the header beside the
rule.

## What an emptied station costs

It costs, and the demonstration is the station's own numbers:

- **It delivers less.** `UpdatePower` reads the station's `staffing` against its `crewNeeded`, so a
  post short runs below full output — Tom Paris's transporters read
  **`output 1.00 -> 0.50`** (the unattended-automation floor: `0.5 + 0.5 * staffing / need`). A
  system whose need is 3 and whose watch fell to 1 reads lower still.
- **It wears.** A station short of its need accumulates `wear` on the maintenance clock
  (`MAINTENANCE_DAYS = 10`), and at 1 the deferred maintenance *fails the system* by 0.4 health
  (`UpdatePower`, the same clock `docs/crew-work.md` names). So the slow cost of a repair is the next
  failure: exactly the "traceable to jobs deferred earlier" loop the document asks for.

The trade is the one `docs/damage-and-budgets.md:60-61` names: *"the security officer holding a
corridor is the person you might need in the coolant bay instead."* Tuvok's shields left a post too
(and cost nothing visible that tick only because the shields are stood down at green — the test reads
the cost from a post that was carrying output).

## The departments, quoted

The party names its people through the roster's nine, one clause per system, each hand's own
department (`DepartmentName`):

```
life support 6 hands (Engineering 3, Engineering 3 off their posts); sensors 6 hands (Engineering 1, Operations 2 off their posts, Security and tactical 1 off their posts, Science 1 off their posts, Medical 1 off their posts)
```

`Engineering` is the standing party (the damage-control organisation); `Operations`,
`Security and tactical`, `Science` and `Medical` are the post-holders a large job pulled off their
stations. The vocabulary is the roster's own — the same nine as `docs/crew-manifest.md`, not the old
five.

## The save

**The party costs the save nothing, and an older save still loads.** The party is *derived every
tick* from the queue, the watch and the crew's own state — exactly as `manned` and `repairing` always
were — and never stored: the new fields (`CrewMember::dcSystem`, `dcFromPost`) are derived like
`activity` and `deck`, so `Pack` does not carry them. `SAVE_VERSION` therefore **does not move**: it
stays **62**, and a save taken before this pass still loads. The transcript proves the direction:
format `62`, the blob round-trips (`loads 1`) with a large party standing, and `TestDamageControlParty`
checks `Pack(back) == blob` byte for byte. (The format *did* move 61 → 62 earlier on 2026-10-12, for
the complement as hull data — the register's row 8.53 — not for this pass.)

## Falsification — force every repair down the first tier, and the check catches it

`scripts/damage-control-check.sh` compiles the unit test against a **broken copy** of `ship_core.h`
in which the threshold is raised past every severity (`REPAIR_SEVERITY_LARGE = 1.1f`), so **there is
no scaling and no poaching**: every repair is the standing party alone. The test must **fail** on the
three tiers; then the tree must pass:

```
==> damage control: the falsification (every repair down the first tier; the test must fail)
      FAIL  damage control: the three tiers: RepairPartySize(0.75f) > REPAIR_TEAM_MAX && RepairPartySize(0.75f) < REPAIR_PARTY_LARGE (line 818)
      FAIL  damage control: the three tiers: RepairPartySize(1.0f) == REPAIR_PARTY_LARGE (line 819)
      FAIL  damage control: the three tiers: t.systems[SYS_SENSORS].repairing == REPAIR_PARTY_LARGE (line 832)
      FAIL  damage control: the three tiers: drawn > 0 (line 846)
      FAIL  damage control: the three tiers: emptied > 0 (line 847)
      FAIL  damage control: the three tiers: emptiedPost >= 0 (line 848)
    the broken copy fails on the three tiers; the tree passes
```

The same script asserts every transcript line above, so a silent regression in any of them fails the
check. A guard that cannot fail for the reason we care about is decoration, so the broken shape is
exactly the one the brief names: **every repair down the first tier.**

## The module builds, and carries the symbols

`module/ship/ship_core.cpp` and `module/ship/g_ship.cpp` are compiled here, on the static host, from
this repository's `module/`. `scripts/damage-control-check.sh` builds the simulation half directly;
`scripts/api-check.sh` holds the console verb and `console_api.def` together (`party`, both
directions). The engine is configured against this lane's module directory and built, and the symbols
are asserted **in the built library, not the tree** — the trap `AGENTS.md` records:

```
$ export PATH=/home/robot/.cache/lwh-venv/bin:$PATH
$ cmake -S /home/robot/upstream/efgame -B /tmp/opencode/efgame-dc -G Ninja -DLWH_MODULE_DIR=$PWD/module
$ cmake --build /tmp/opencode/efgame-dc --target efgame -j8      # [141/141] Linking libefgame.so
$ nm -C /tmp/opencode/efgame-dc/libefgame.so | grep -E "ship::(RepairPartySize|DamageControlLine)"
000000000017a030 t ship::RepairPartySize(float)
000000000017a0c0 t ship::DamageControlLine[abi:cxx11](ship::Ship const&)
```

Both functions are in `libefgame.so`, so the module the engine loads carries the tiers. The build was
made in a scratch directory (`/tmp/opencode/efgame-dc`) rather than a shared one, and no engine
process was left running. The `ship party` console read and the UI are the playtest host's, below.

## Judgment calls, named as calls

1. **The severity axis is the system's damage (`1 - health`).** The brief names three candidates —
   damage, criticality, the budget. Damage is the honest unit of "how big is this repair"; criticality
   is already the queue's own axis (`priority`, `orderRepairFirst`) and would double-count it; the
   parts budget is the *cost* of a job, not its severity. Recorded in `docs/crew-work.md` and
   `docs/ship-systems.md` so it can be overruled.
2. **The threshold is half the system gone (0.50).** Stated as a number, with the reasoning above.
   Tunable; a different line moves both the scaling and the poaching, because they share it.
3. **The crossing hands come from the owning department first, then any department.** The manifest
   says damage control "pulls from every department", so a large job is not limited to the system's
   own watch; the owning department's people go first because they know the system. This is what makes
   the party line name several of the nine.
4. **A post left empty costs what the model already charges for an undermanned station** — less output
   and more wear. We did **not** add a new penalty (a morale hit, a delay): the two budgets the ship
   already runs are enough, and inventing a third would be a number nobody asked for. If it reads as
   too cheap, the tuning is `MAINTENANCE_DAYS` and the manning floor, both existing.
5. **The standing party's place in one job is `REPAIR_TEAM_MAX` (3), unchanged.** The brief's tiers
   two and three therefore meet at the threshold: a large job's extra hands are the post-holders. The
   alternative — the whole standing party to one job — would have made the observable "more hands"
   depend on how many stationless engineers a watch happens to have, and would have broken the
   existing queue test's "both jobs are worked". Stated so it can be overruled.
6. **No parts, no large job.** A repair that cannot be worked does not hold hands off stations; the
   standing party is still assigned as before. Chosen because holding the watch for a job with no
   materials is a pure loss with no progress to show for it.
7. **The party is derived, never saved, and the format does not move.** The cheapest and safest
   shape: a save replays the party exactly, and no older save is invalidated by this pass. The cost is
   that the party is computed every tick (bounded: at most 23 systems and the roster).

## What could not be verified

- **Whether a repair *feeling* like it costs you a station is the right feel.** That is the owner's
  judgement, named as his; no transcript can answer it. The numbers are here so he can look at the
  trade.
- **The rendered console line.** The static host runs no engine and holds no game data, so
  `g_ship.cpp`'s `ship party` read and `lwh_ship_damage_control` were written and compiled, but not
  driven through `Svcmd_Ship_f` here. The line's own text — the deliverable — is asserted; the pixel
  check is the playtest host's.
- **The engine build's symbol assertion.** Done here: `RepairPartySize` and `DamageControlLine` are
  in `libefgame.so` (transcript above). What was **not** run here is the engine itself: `ship party`
  driven through `Svcmd_Ship_f`, and the console frame drawn. That is the playtest host's, as every
  other screen's is; the model's own numbers are asserted in the transcript.

## Register corrections made in the same pass

- **`docs/designs-against-code.md` C-12** — disposition added: built, with its marker
  (`state=closed marker=check:scripts/damage-control-check.sh`). Domain row 31 moves from
  `contradicted` to **implemented**. The 2026-10-12 code pass's "handed back" bullet gains a
  since-this-pass note.
- **`docs/crew-work.md`** — the ruling written into the document as ours, in the owner's words, with
  the three tiers and the axis.
- **`docs/ship-systems.md`** — the ruling written into *Repairs are the counter-play*, in the owner's
  words, with the threshold, the cost and the save's cost.
- **`docs/hook-register.md`** — two hooks added (`RepairPartySize` play, `DamageControlLine`
  console), the readers count 212 → **214**, and the console-reach list 137 → **138**.
