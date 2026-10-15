# Evidence: the re-triage — the unreachable hooks, counted on the current surface

**The finding this closes.** `docs/hook-triage.md` is stale: it was written against a surface of **366**
hooks; `module/ship/ship_core.h` now declares **496**, and the register's own counts drifted through
398, 479 and 496 across passes. Wave 2 of the readiness programme is sized on the genuine-gap count, so
the count had to be re-derived from the register the game actually has, not read off the triage's header.

**Host:** static. The reach closure (`tools/hooks/reach_report.py`) and the register were run here; the
rendered surfaces were not.

## 1. Two sources, and which is the authority

There are two answers to "can a player reach this?", and they are not the same instrument:

- **The tool** (`tools/hooks/reach_report.py`) computes a **transitive closure** from six entry surfaces
  (ui, publish, world, advance, console, harness). It is mechanical and conservative: it cannot see that a
  readout a console *draws* is reached, because that is a judgement about art and UI, so it over-counts
  the unreachable.
- **The register's `reach` column** (`docs/hook-register.md`) is a **person's judgement**, per hook. It is
  the authority for "is this a gap" — the same rule `scripts/hooks-check.sh` states ("It does not judge
  reachability (that is a person's call)").

So the genuine-gap set is derived from **the register's judgement**, and the tool is a cross-check that
flags where the two disagree (it did, both ways: the tool credits `WritePersonalLog` and `MannerLine` as
play, the register holds them console).

```
python3 tools/hooks/reach_report.py        # the closure, 496 rows
python3 tools/hooks/surface.py module/ship/ship_core.h | wc -l   # 496
```

## 2. The census, from the register (the judgement)

| state | hooks |
|---|---|
| play | 299 |
| console | 158 |
| nothing | 34 |
| content | 5 |
| **total** | **496** |

`tools/docs/check_register_counts.py` (class 4 of `scripts/docs-check.sh`) guards this against the rows and
the surface, so it cannot drift silently again.

## 3. The genuine gaps — 61

A hook is a **genuine gap** when the design names it as a player's or a post's act and no in-game surface
reaches it. The triage classified **59** rows as genuine gaps; **5** have since been wired (the reachability
pass: `SetTarget`, `Remodulate`, `RaidVinculum`, `OrderAdvance`; the manner pass: `MannerLine`), leaving
**54**. The triage's judgement of each is kept (the hooks' nature did not change); the count and the
reach are re-derived here.

**The 54** (still `console`/`nothing` by the register):

`AdvanceSector` · `Airponics` · `AssumeCommand` · `AttemptRising` · `BorgAwareness` · `Brief` · `Brig` ·
`CaptainLog` · `ClearLockout` · `Delegate` · `EVA` · `EndHolodeckProgram` · `FabricateRations` · `Hearing` ·
`HoldFuneral` · `ImproveQuarters` · `InterferePreWarp` · `JumpStartFromHolodeck` · `LaunchProbe` ·
`LaunchShuttle` · `LoadAwayKit` · `LockOut` · `MakePromise` · `MineBelt` · `MobileEmitter` ·
`ObservePreWarp` · `OfferRising` · `OfferRisingTo` · `Promises` · `RebuildShuttle` · `RecallShuttle` ·
`ReconcileFactions` · `Refugees` · `RepairDeck` · `Resentment` · `RespondPhenomenon` · `RevokeCredential` ·
`RevokeDelegation` · `RunHolodeck` · `SetAirponics` · `SetMobileEmitter` · `SetRole` · `Sleep` ·
`SpeciesCapability` · `SpeciesNeed` · `SpeciesSusceptibility` · `Survey` · `Suspend` · `TakeSurvivors` ·
`TractorHold` · `TractorWreck` · `Train` · `WorkReading` · `WritePersonalLog`

**Seven new genuine gaps**, from the 69 hooks the triage never saw (the surface grew 366 → 496):

- **the Collective's six reads** — `CollectiveLog`, `CollectiveMarks`, `CollectiveOperates`,
  `CollectiveRefusal`, `CollectiveVoice`, `HeldByCollective`. The design (`docs/the-record-and-the-log.md`,
  `docs/staff-meetings.md`) makes **the ship's computer** the Collective's surface — "the same machinery …
  the Collective speaks in the taken voice" — and the computer path is built; but nothing routes to these
  reads (their only consumers are `g_ship.cpp`'s `ship collective`, a console verb). **Genuine gap.**
- **`SetAirSourceBy`** — the delegated form of setting a deck's air source (an officer acting under a
  standing grant). It has no consumer outside `ship_core`; its surface is the **delegation** gap
  (`Delegate`/`LockOut`, in the 54). **Genuine gap.**

**Total: 61 genuine gaps.**

## 4. The 69 new hooks, classified (so none is lost)

The 69 unreachable-by-the-tool hooks the triage never saw, and what each is:

- **Diagnostic by design (the bulk):** the classifier/novelty plumbing (`AddNoveltyVerdict`,
  `AddOrderNoveltyVerdict`, `ClearNoveltyIndex`, `ClearOrderNoveltyIndex`, `NoveltyKey`,
  `NoveltyVerdictCount`, `OrderNoveltyKey`, `OrderNoveltyVerdictCount`); the appearance derivation's reads
  (`Appearance*`, `NamedHeadModel`, `CanonFaceInPool`, `SpeciesHeadAt`, `SpeciesHeadCount`); the manner's
  switches and names (`SetMannerWired`, `SetMannerVoiceBank`, `MannerEmitName`, `MannerPurpose`); the
  order/meeting name-and-counter readers (`OrderClassCount`, `OrderClassSpecOf`, `OrderLabel`,
  `OrderLineKey`, `OrderNeedName`, `PlanOrderAudio`, `MeetingLineKey`, `CachedDuration`, `CastIsCanon`,
  `VoicePoolCount`, `DamageControlLine`). Consumed by the tests, the harness, the worker or another hook.
- **Reached in play (the surfaces the register already credits):** the order machine (`BuildOrderBrief`,
  `DeriveOrders`, `OrderPermitted`, `OrderRefusalRule`, `OrderSetFault`, `ResolveOrderTarget`,
  `BespokeOrder`, `ClassifyNovelOrder`, `OrderMaintain` — the last is a surface row in
  `module/ship/order_set.def`); the meeting/computer reads (`BuildComputerBrief`, `AddressComputer`,
  `ComputerPillCount`, `ComputerRefusalName`, `ComputerVoice`, `LogScopeForCrew`, `MayOrder`,
  `MeetingLineSeconds`, `MeetingRoomFor`); the air/system reads (`SetAirSource`, `SourceAirCommitment`,
  `SystemIdByName`, `ShieldSectionByName`).
- **Reached by content:** `DepartmentByName` (the loader's department vocabulary, `scenario_core.cpp:291`)
  and the scenario ledger (`FiredScenarios`, `ScenarioFired`, `RecordScenario`) — the register's new
  `content` state.

## 5. What the tool and the register disagree on, named

The tool reports **285 play / 155 console / 52 nothing**; the register's judgement is
**299 / 158 / 34 / 5 content**. The 14-hook play difference and the 18-hook nothing difference are the
readouts a console draws and the surfaces a screen sends, which the closure cannot see and the register's
judgement records. Neither is wrong; they answer different questions. `WritePersonalLog` is the worked
case of the gap: the tool says play (the host calls it), the register says console (the personal-log screen
can only *read* — the write path is built and no key reaches it), and the **register is right**, which is
why the genuine-gap set is taken from it.

## 6. The numbers Wave 2 plans on

- surface **496**; register census **299 / 158 / 34 / 5**.
- **61 genuine gaps** (54 triage + 7 new), over an unreachable pool of **192** (158 console + 34 nothing),
  the rest diagnostic or simulation-raised.

## 7. This pass moved the register's rows

- `docs/hook-triage.md` header: the stale "written against 366" counts are marked and the current counts
  recorded, with this document as the evidence.
- `docs/hook-register.md`'s counts marker is the one `tools/docs/check_register_counts.py` guards.

## 8. What could not be established

- **Whether a diagnostic hook is one a player would want.** Every row is a judgement; the 61 genuine gaps
  are the list to read, and the owner may overrule any row.
- **The seven new rows' exact surface.** The Collective's computer route and the delegation surface are
  **named designs** not yet built; this pass classifies them as gaps and does not build them.
