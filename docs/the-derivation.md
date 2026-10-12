# The derivation — declaration → primitive → call path

**What this is.** The crossing instrument. `docs/path-to-playtest.md` (*The primitive crossing*) says gap
coverage has two halves and a primitive is ready when four questions are answered; three of the four are other
instruments'. This document is the fourth: **for every field a scenario must be able to declare, which
primitive answers it, and how the engine would call it.**

The frame, in the owner's words (`docs/path-to-playtest.md:178-181`): *content here is declared, not thrown*.
A scenario states its breeding conditions, the systems it stresses, the staff it needs, the cost to answer or
ignore, its residue, its visibility and its warmth, and is instantiated **through the post** rather than a
named person (O15, `docs/omissions.md`). So a **field with no primitive behind it is a field an author cannot
use**, and **a primitive no declaration reaches is inventory, not readiness**. This pass finds both.

**The fields are the design's, not this document's.** They are taken in the design's own words from
`docs/scenario-atlas.md` — *The authoring contract* (`:66-76`), *The post-not-name law* (`:78-87`),
*What a scenario must therefore declare* (`:137-141`) and *A first slate* (`:183-184`) — and summarised in
`docs/omissions.md` **O15**. Nothing here was built or changed; every absent primitive is a lane or a
register row, not a repair (`docs/path-to-playtest.md`: *the pass that measures is not the pass that
repairs*).

**The verdicts, defined, so the table is not read as more than it says.**

| verdict | meaning |
|---|---|
| **ready** | a primitive answers the field, the reconciliation register does not contradict it, and a named call path (console verb, module function, station screen) reaches it |
| **partial** | a primitive answers part of the field, or the primitive is reached by **no** call path an author could use; the missing half is named |
| **absent** | no primitive answers the field at all — the field's own mechanism does not exist |
| **— not ready** | appended where `docs/designs-against-code.md` **contradicts** the answering primitive's design; the row carries the pointer |
| **— unenforced** | appended where the mechanism exists but a rule that binds it has no validator |

**The cross-cutting finding, stated once and not repeated per row.** There is **no scenario runtime**: no
director that recruits an authored event from the ship's state, and no `scenario.json` key for any field
below. `tools/validator/validate.py:125` requires only `name`, `maps` and `scripts`; the optional keys are
`inhabited`, `asset_roots` and `crew` (`docs/scenario-manifest.md:17-24`). So today **no declaration reaches
any primitive** — the crossing is one-sided, and the table below is the *primitive* half of it. That single
absence (walkthrough **G4/G5**; reconciliation 7.34 and 8.56, both `absent`) is the first content-blocker
and is not repeated as a caveat on every row.

## The table

Every row's call path is cited to a file and a symbol. Console verbs are enumerated in
`module/ship/console_api.def`, guarded both ways by `scripts/api-check.sh`; module functions are the public
surface of `module/ship/ship_core.h`, guarded by `scripts/hooks-check.sh`.

| declaration field | what answers it | call path | verdict |
|---|---|---|---|
| **Breeding conditions** — "which ship state makes it eligible" (`scenario-atlas.md:70`) | the ship's readable state: `Ship::stores` (`deuterium`, `rations`, `materials`, `probes`), `Ship::beacons` / `Beacon::kind` / `Beacon::phenomenon`, `SystemCondition`, `BorgAwareness`, `Refugees`, `Resentment`, `Patients`, `Jobs`, `MinutesOfAir`, `PowerShortfall`, `Coreless`, `Intruders`, `DeckLifeSupport`; the meeting system's own eligibility predicate `MeetingDue(Ship&,uint8_t)` is the precedent | readers: console `status`, `nav`, `patients`, `jobs`, `awareness`, `survivors`, `losses`; module readers. **No predicate evaluates a scenario's breeding conditions** — `MeetingDue` is meetings only. | **absent** — reconciliation 7.34 (`scenario-atlas.md:20` has no scenario/breeding system) and 8.56 (no director) |
| **Systems stressed** — "the systems it reads and writes, by our system IDs" (`:71`) | `SystemIdByName`, `Spec`, `BandOf`, `SystemCondition`, `EffectiveDemand`, `SystemStateName`, `FailureStateOf`; writers `UseSystem`, `SetEnabled`, `SetPriority`, `SetAllocation`, `DamageSystem`, `OrderRepairFirst`, `Repair` | console `on`, `off`, `priority`, `alloc`, `damage`, `repair`, `operate`; module `UseSystem`, `SetEnabled`; enumeration `module/ship/console_api.def` + `scripts/api-check.sh` | **ready** |
| **Staff needed** — "posts it requires, so the crew model is the cost" (`:72`) | `PostsNeeded`, `CoverWithCrew` (the posts the systems need, and coverage), `DepartmentHead`, `SeniorFitOfficer`, `SeatHolder`, `SeatPost`, `ScheduledActivity`, `BillHoles` | `PostsNeeded` / `CoverWithCrew`: **nothing** (reached only by the tests — register `nothing`, triage §5). `DepartmentHead`, `SeniorFitOfficer`, `ScheduledActivity`, `BillHoles`: module (play); console `bill`, `crew` | **partial** — the two primitives that answer "posts it requires" have no call path an author could use |
| **Cost to answer** — what the player spends to address it (`:73`) | effect writers `WriteOff`, `AcquireDilithium`, `TakeSurvivors`, `KillCrew`, `Sicken`, `Promote`, `SetAirSource`, `FabricateParts`, `FabricateRations`, `LaunchShuttle`, `LoadAwayKit`, `UseSystem`; the option-cost model `AuthoredSkeleton` / `ApplyMeetingOutcome` | console `writeoff`, `acquire`, `survivors`, `fabricate`, `alloc`, `air`, `meeting choose`; module | **partial** — the payment exists; a scenario-declared option and its cost have no schema; only the meeting machine carries an option-with-cost |
| **Cost to ignore** — "ignoring something is not failing — it is a different state" (`:25-27`) | the unattended-consequence machinery: `IgniteDeck` (a fire spreads), `BreachDeck`, Borg `dwell` / `DeckAssimilated`, `UpdateSystemStates` | simulation-raised (console `ignite`, `breach`, `borg`); `Tick` | **partial** — consequence accrues by itself, but nothing reads a scenario's declaration of what ignoring costs |
| **Residue** — "what is permanently different afterwards" (`:73`) | `WriteOff` (the written-off list), `KillCrew`, `SealedQuarters`, `Remember`, `AddCondition`, `SignReport`, `PurgeLogs`, per-deck damage; per-crew `assimScar` | console `writeoff`, `losses`, `remember`, `report purge`, `seal`; module | **partial — not ready** — reconciliation 7.8 (`partial`: no per-deck residue field, "the place is fully restored") contradicts the design's *"a sealed deck you can visit later and see what was left behind"* (slate 8); also 6.6 |
| **Visibility** — "what the player sees *where they are*" (`:74`) | the per-post information portfolio (`module/ui/ui_lwh_engineering.cpp`), `StationReads`, `OperatedFrom`, `LogScopeForCrew`, `BuildBrief` / `AddParticipant`, `WorkReading`, `Patients`, `CrewOnDeck` | station screens (play) via `StationReads`; console `status`, `console`, `patients`. `WorkReading` — the operator's own factors — is **console only, no screen** | **partial** — the portfolio exists; `WorkReading` has no surface (triage §5 #29; reconciliation 3.49) |
| **Warmth** — "the human beat it leaves behind" (`:76`) | `RunHolodeck`, `EndHolodeckProgram`, `ImproveQuarters`, `HoldFuneral`, `Promote`, `FabricateRations`, `SetAirponics`, `MakePromise`, `Morale`, `MannerFor` / `EmitManner`, `WallOfNames` | console `holo`, `holoend`, `quarters`, `funeral`, `promote`, `rations`, `airponics`, `promise`, `manner`, `wall`; module `EmitManner` | **ready** |
| **The post** — instantiation "through the post rather than a named person" (`:78-87`) | `OfferRising`, `OfferRisingTo`, `AttemptRising`, `DeriveOrders`, `OrderPermitted`, `ResolveOrderTarget`, `BuildOrderBrief`, `DispatchOrder`, `ResolveSpeaker`, `SeatHolder`, `DepartmentHead`, `SeniorFitOfficer`, `FillVacancies` | console `order`, `rise`, `meeting`; module | **ready — unenforced** — the law is recorded (`scenario-atlas.md:78`) but no validator enforces it (reconciliation 7.35, `partial`) |
| **The positions it can be experienced from** (`:139`) | `CrewMember.rank` / `.post` / `.dept`, `SeatName`, `SeatType`, `SeatPost`, `SeatRank`, `PlayerPositionLine`, `CareerPathName`, `SetRole`, `ApplyStartState`, `FillVacancies` | configurator `module/ui/ui_lwh_start.cpp` and console `startstate`; console `role`. `SetRole` is itself a genuine gap (triage §3 #48) | **partial** — the post model exists; "author the positions as data" per scenario is not a construct |
| — **the information they receive** (`:140`) | per-participant brief `BuildBrief` / `AddParticipant`, `LogScopeForCrew`, `StationReads`, `WorkReading` | meeting screen (play); console `log`, `meeting`; `WorkReading` no screen | **partial** |
| — **the authority they hold** (`:140`) | `MayOperate`, `MayCallAlert`, `MayCommand`, `PlayerMayOperate`, `Qualified`, `OfficerOfTheDeck` / `OodMayOrder`, `DelegatedTo`, `OverrideActive`, `AccessRefusal` | station gates (play) via `MayOperate` / `MayCallAlert`; console `operate`, `override`, `delegate`. C-11 fixed 2026-10-12 (`MayOrder` consults `OodMayOrder`; `TestOodDispatch`) | **ready** |
| — **the people they are responsible for** (`:140`) | `DepartmentHead`, `SeniorFitOfficer`, `OodMayOrder` / `OodRefusal`, `WatchBillLine`, `BattleBillLine`, `EmergencyBillLine`, `BillHoles`, `CrewOnDeck` | console `bill`, `crew`; screen `ui_lwh_bills` in `module/ui/ui_lwh_command.cpp` (`B` at the command console) | **partial** — the department and bill organisation is built (O6), but the reporting rule is not wired (reconciliation 3.8, `partial`) |
| — **the decision they own** (`:140`) | `AuthoredSkeleton`, `BuildBrief`, `ApplyMeetingOutcome`, `DeriveOrders`, `BuildOrderBrief`, `DispatchOrder`, `OrderTriage` | console `meeting choose`, `order give`; screen `ui_lwh_meeting` | **partial** — a decision-with-options is modelled for meetings and orders, not for a scenario |
| **The canon root** — "each names its canon root" (`:184`) | the fiction's own name helpers `BeaconKindName`, `EnemyKindName`, `SpeciesName`, `ControllerName` | **nothing** — no scenario field carries it | **absent** — authoring metadata with no home in the manifest (`docs/scenario-manifest.md:17-24`) |
| **The decision** (the scenario's own) — "each names … its decision" (`:184`) | the decision machinery `AuthoredSkeleton` (a kind's outcomes and their costs), `ApplyMeetingOutcome`, `BuildBrief` | console `meeting`, `order`; screen `ui_lwh_meeting` | **partial** — the same machinery as *the decision they own*, keyed to meetings and orders, not to a scenario |

## The counts

**16 fields**, the design's own set (8 from the authoring contract, 1 from the post-not-name law, 5 from
*what a scenario must therefore declare*, 2 from the slate):

| verdict | fields | which |
|---|---|---|
| **ready** | **4** | systems stressed; warmth; the post; the authority they hold |
| **partial** | **10** | staff needed; cost to answer; cost to ignore; residue; visibility; the positions; the information they receive; the people they are responsible for; the decision they own; the decision |
| **absent** | **2** | breeding conditions; the canon root |
| — of the partial/absent, **not ready** | **1** | residue (reconciliation 7.8 — no per-deck residue) |
| — of the ready, **unenforced** | **1** | the post (reconciliation 7.35 — no validator enforces the post-not-name law) |

## The absent fields, ordered — what must exist before content

Ordered by what a first scenario would hit, following the atlas's own prototype watch (`:6-14`) and its first
slate entry (`:186-189`). Each is a **content-blocker**: a lane or a register row, not a repair here.

1. **Breeding conditions — the director.** Nothing recruits an authored event from the ship's state; the
   state is readable and the eligibility predicate is not written. Without it no scenario fires (walkthrough
   **G4**; reconciliation 7.34 and 8.56, `absent`; `docs/scenario-atlas.md:20`).
2. **The scenario construct — the event and its decision.** There is no schema for a scenario beyond
   `name`, `maps`, `scripts`, `inhabited`, `asset_roots` and `crew` (`docs/scenario-manifest.md:17-24`); the
   positions, the decision, the costs and the residue have nowhere to be written. (`partial`.)
3. **Staff needed.** `PostsNeeded` and `CoverWithCrew` — the primitives that answer "posts it requires" —
   are reached by **nothing** but the tests (register `nothing`; triage §5). A scenario cannot declare its
   post set, so the crew model cannot be the cost it is meant to be. (`partial`.)
4. **Cost to answer, cost to ignore, residue.** The payment and the accruing-consequence machinery exist,
   but no option carries a declared cost or ignore-branch; and place-residue is contradicted — **the deck is
   fully restored** (reconciliation 7.8). The sealed deck the player can revisit, which slate 8 promises, is
   the absent half. (`partial`; residue **not ready**.)
5. **Visibility.** `WorkReading` — the operator's own factors — has no screen (triage §5 #29). A consequence
   the player cannot perceive is, by the design's own rule, one that did not happen. (`partial`.)
6. **The canon root.** A one-line authoring field with no home (`absent`).

**The answer to "what must exist before the engine [carries content]": the director and the scenario schema
(1 and 2) come first; the rest are primitives to wire, in the order above.**

## The inventory — primitives no declaration reaches

Because no declaration layer exists, **every hook in the register is reached by no declaration today**. That
blunt fact is the crossing's one-sidedness. The usable inventory is narrower: the public functions among the
**479** in `module/ship/ship_core.h` that **no field row above names** — **349**, by section:

| register section | in the inventory |
|---|---|
| 1. entry points | 53 |
| 2. situations the simulation generates | 73 |
| 3. outcomes content can ask for | 29 |
| 3b. orders | 14 |
| 3c. the bills | 13 |
| 4. readers | 159 |
| 5. the configurator | 8 |
| **total** | **349** |

The 349, named (by the register's own sections):

- **Entry points (53):** `NextRisingRoll`, `RetellRising`, `LogEvent`, `TakeMind`, `VoiceWrite`,
  `VoiceRetire`, `EmitCue`, `QueueRender`, `CacheRendered`, `PruneVoiceCache`, `PlanMeetingAudio`,
  `WarmVoice`, `SubmitNovelAnswer`, `UseSystemBy`, `RecommendAllocation`, `RepairDeck`, `MakeBreach`,
  `Scan`, `ScanCompartment`, `FireTorpedo`, `ShuttleByClass`, `RecallShuttle`, `StrandShuttle`,
  `LoseShuttle`, `ShuttleBayHit`, `RebuildShuttle`, `Recomposite`, `LocateDilithium`, `TractorWreck`,
  `TractorHold`, `RevealPhenomenon`, `ActivateEMH`, `DamagePylon`, `Hail`, `Trade`, `AnswerDistress`,
  `Disengage`, `TransportAway`, `TransportBack`, `SetCourse`, `CatchUp`, `Suspend`, `Sleep`, `SetPlayerDeck`,
  `WoundPlayer`, `AttendIncapacitatedPlayer`, `DraftReport`, `StrikeReportLine`, `SoftenReportLine`,
  `EditReportLine`, `AddReportLine`, `CreateCharacter`, `Unpack`.
- **Situations (73):** `AnomalyOdds`, `AnomalySeverityFor`, `RollAnomaly`, `HasTrait`, `DeriveCharacter`,
  `DeriveSpecies`, `DeriveAppearance`, `AppearanceHeadModel`, `AppearanceBuildName`, `AppearanceColourName`,
  `NamedHeadModel`, `IsCanonFace`, `CanonFaceInPool`, `AppearancePoolCount`, `AppearancePoolDirectoryAt`,
  `AppearancePoolHeadAt`, `AppearancePoolSpeciesAt`, `SpeciesHeadCount`, `SpeciesHeadAt`, `FindCondition`,
  `MoraleBandName`, `MoraleReason`, `Holdings`, `EffectiveSkill`, `MannerLine`, `WorkFactorKindName`,
  `WorkContextName`, `WorkContextOf`, `WorkFactors`, `WorkOdds`, `RollWork`, `WorkFactorLine`, `MannerName`,
  `MannerPurpose`, `MannerTriggerName`, `MannerEmitName`, `SetMannerWired`, `MannerWired`,
  `SetMannerVoiceBank`, `MannerVoiceBank`, `EmitDueMeetings`, `PendingMeetings`, `TakeBrief`,
  `ClassifyNovelInput`, `NoveltyKey`, `ClearNoveltyIndex`, `AddNoveltyVerdict`, `NoveltyVerdictCount`,
  `BuildComputerBrief`, `AddressComputer`, `DamageSource`, `Board`, `BoardAs`, `CounterHack`, `Hijacked`,
  `BoardBorg`, `Jump`, `InCombat`, `Remodulate`, `RaidVinculum`, `ShutDownCore`, `RestartCore`, `EjectCore`,
  `RestoreCoolant`, `CoreBreached`, `JumpStartFromHolodeck`, `Pursued`, `PursuitJumps`, `AtEnd`, `Won`,
  `SectorNumber`, `AdvanceSector`, `Trauma`.
- **Outcomes (29):** `ClearCondition`, `Cure`, `OrderBuild`, `SetJobPriority`, `OrderAdvance`, `SetAlert`,
  `SetAllocationBy`, `SetPowerAuto`, `AcceptRecommendation`, `RefuseRecommendation`, `GrantBand`,
  `RevokeBand`, `SetSourceOnline`, `SetForceField`, `SetForceFieldLevel`, `SetAirSourceBy`,
  `SetSurgicalField`, `RecoverCaptive`, `SetMobileEmitter`, `SetTarget`, `SetPhaserYield`,
  `RevokeDelegation`, `RevokeCredential`, `BeginOverride`, `ConfirmOverride`, `ClearLockout`,
  `ResolvePromise`, `OrderSecurityTo`, `OrderEvacuate`.
- **Orders (14):** `OrderClassCount`, `OrderClassSpecOf`, `OrderNeedName`, `OrderLabel`, `OrderRefusalRule`,
  `OrderSetFault`, `OrderNoveltyKey`, `ClearOrderNoveltyIndex`, `AddOrderNoveltyVerdict`,
  `OrderNoveltyVerdictCount`, `ClassifyNovelOrder`, `BespokeOrder`, `OrderLineKey`, `PlanOrderAudio`.
- **The bills (13):** `BillRoleName`, `EmergencyKindName`, `EmergencyDeckPhrase`, `EngineerOfTheWatch`,
  `CallBattleStations`, `StandDownBattleStations`, `BattleStations`, `BattleCalledBy`, `CallEmergency`,
  `StandDownEmergency`, `Emergency`, `EmergencyDeck`, `EmergencyCalledBy`.
- **Readers (159):** `StationOf`, `StationName`, `AnomalyName`, `AllocationByName`, `PhaserYieldName`,
  `SkillName`, `DepartmentSkill`, `TraitName`, `DesireName`, `NeedName`, `FearName`, `ConditionName`,
  `ConditionValenceName`, `ConditionClearName`, `ConditionMagnitudeName`, `SpeciesOf`, `PromiseKindName`,
  `ReportAudienceName`, `RisingOutcomeName`, `RisingDriveAtStake`, `RisingOdds`, `LogVisibilityName`,
  `PersonalLog`, `PersonalVisibleTo`, `ReadOfficialLog`, `HeldByCollective`, `CollectiveLog`,
  `CollectiveMarks`, `CollectiveOperates`, `CollectiveRefusal`, `CollectiveVoice`, `ConsoleVerbCount`,
  `ConsoleVerbAt`, `FindConsoleVerb`, `MeetingKindName`, `MeetingRoomCount`, `MeetingRoomAt`,
  `MeetingRoomFor`, `MeetingSubjectDepartment`, `DeliveryName`, `DeliveryExaggeration`, `DeliveryKnown`,
  `MeetingEffectName`, `ComputerVoice`, `ComputerRefusal`, `ComputerRefusalName`, `ComputerPillCount`,
  `ComputerPillVerb`, `ComputerPillLabel`, `ComputerPillCost`, `LineToSynthesis`, `VoiceTrackName`,
  `VoiceProducerName`, `TrackOwner`, `OwnsTrack`, `TrackBusy`, `ActiveReply`, `CueName`, `CueClip`,
  `CuePurpose`, `CueIsLexical`, `RenderKey`, `VoiceCachePath`, `VoiceCached`, `VoiceQueued`,
  `VoicePoolCount`, `VoicePoolName`, `VoiceIsCanonType`, `CastVoice`, `CastIsCanon`, `CachedDuration`,
  `MeetingLineKey`, `MeetingLineSeconds`, `LossKindName`, `WriteOffs`, `JobKindName`, `BandName`, `Day`,
  `SecondOfDay`, `Watch`, `PowerAvailable`, `PowerAllocated`, `PowerCapacityFresh`, `PowerCapacityNow`,
  `CrewFit`, `NewShip`, `StressNow`, `AllocationPercent`, `AllocationSource`, `AllocationProvenance`,
  `PowerCommitted`, `PowerAuto`, `BandHolder`, `BandGrantCount`, `MinutesToDark`, `EnduranceOf`,
  `AirSourceOf`, `AirSourceName`, `AirCommitted`, `SourceAirCommitment`, `SourceAirCapacity`,
  `SourceDecksHeld`, `AirShortfall`, `GravityScale`, `ScaleGravity`, `SurgicalField`, `BoarderKindName`,
  `BreachScore`, `ShuttleClassName`, `ShuttleLocationName`, `ShuttlesInBay`, `ShuttlesAway`, `WarpPossible`,
  `DilithiumRange`, `DilithiumWayName`, `PhenomenonResponseName`, `Held`, `Loyalty`, `EMHActive`,
  `TravelSafe`, `PylonsIntact`, `MobileEmitter`, `Target`, `EnemySubsystemName`, `PhaserYieldOf`, `AwayTeam`,
  `TransporterConditionLine`, `PlotCourse`, `Course`, `ClockRate`, `SavesAllowed`, `MaySuspend`,
  `LeftStanding`, `Delegations`, `OverrideState`, `LockedOut`, `Lockouts`, `LockoutNotice`, `MayCallUp`,
  `OperatedFromRefusal`, `Brigged`, `PlayerIncapacitated`, `PlayerDead`, `PlayerDeck`, `PlayerBodyHealth`,
  `Recall`, `RecallSource`, `MemoryCount`, `Bond`, `NavigationCounter`, `NavigationForecasts`, `OpenReport`,
  `ReportDiff`, `Reports`, `LogsPurged`, `CommandingOfficer`, `PlayerMayCommand`, `Pack`, `Describe`.
- **Configurator (8):** `CareerPathBlurb`, `SeatDepartment`, `SeatForType`, `StartStateCount`,
  `StartStateAt`, `SeatHeldBy`, `WriteLogSeed`, `LogSeedText`.

**Read it by kind, and the kind is the finding.** The mass of it is **back-office by design** — the
voice/render/meeting plumbing, the appearance and species derivation, the name/counter helpers, the save
seams — which `docs/hook-triage.md` already classifies as *diagnostic*; the derivation confirms that no
scenario declares them and none should. A second mass is **reached by other instruments**: the power, access,
memory and log reads that answer *systems*, not the scenario contract. The third, smaller mass is the one
that matters — **player and post acts no declaration reaches**, which are the genuine gaps of
`docs/hook-triage.md` seen from the content side: `AssumeCommand` (command devolves from a dead or
assimilated player — reached by nothing at all), the three species readers `SpeciesCapability` /
`SpeciesNeed` / `SpeciesSusceptibility`, `WritePersonalLog`, `CaptainLog`, `Promises`, `Survey`,
`LaunchProbe`, `RespondPhenomenon`, `LoadAwayKit`, `MineBelt`, `EVA`, `FabricateRations`, `Airponics`,
`TakeSurvivors`, `ObservePreWarp`, `InterferePreWarp`, `ReconcileFactions`, `Delegate` / `LockOut` /
`Train`, `Brig` / `Hearing`, `MakePromise` / `Brief` / `HoldFuneral`, `SetRole`, and the rising offer's
trigger.

`docs/hook-triage.md` already owns that backlog and its order; this pass cites it rather than re-triages it
(the brief forbids a re-triage). **What the derivation adds is the reason those are inventory rather than
readiness**: no declaration field reaches them, so none of them is part of the scenario contract yet — they
are built capability waiting for a declaration, or for a decision that they never need one.

## The instrument, falsified

Three rows marked **ready** were checked by a witness that is not this document. A call path verified only by
reading is a reading.

1. **Systems stressed** — call path *console `on` / `off` / `priority` / `alloc` / `damage` / `repair`;
   enumeration `module/ship/console_api.def`*. Witness: `bash scripts/api-check.sh` →
   `PASS every console verb is in the ship's API, and every API verb is answered by the console`; and
   `bash scripts/hooks-check.sh` → `PASS every public function is registered`. The verbs are independently
   derived from `module/ship/g_ship.cpp` and compared to the enumeration in both directions.
2. **The authority they hold** — call path *station gates via `MayOperate` / `MayCallAlert`*. Witness:
   `scripts/test.sh` builds and runs `tests/ship/test_ship_core.cpp` (exit 0; `ship_core: all checks
   passed`) — the suite containing `TestRankAndRoles` (`:3613`), `TestAccessAndAuthority` (`:3678`) and
   `TestOodDispatch` (`:8600`), the last of which the reconciliation names as the proof that `MayOrder`
   consults the deck's authority (C-11, fixed 2026-10-12).
3. **Warmth** — call path *console `holo` / `funeral` / `quarters` / `promote` / `rations`*. Witness:
   `bash scripts/api-check.sh` proves those verbs are enumerated and answered, and
   `bash scripts/manner-check.sh` → `PASS the manner is selected from the state … handed to the voice …`
   — the warmth primitive `EmitManner` reached end to end.

Each witness is a script that fails on a planted error, not just passes on the tree (`api-check.sh`,
`hooks-check.sh` and `manner-check.sh` each demonstrate their own failure mode in
`docs/evidence/the-computer-api.md`, `docs/evidence/the-manner.md` and the register's own falsifications).

## What could not be established

- **Anything needing the engine or the game data.** This is a static host. Whether the warmth beat *reads*
  as warmth, whether a scenario's visibility is legible at the post, and whether the post-not-name law holds
  across authored content are the owner's judgements and are not decidable here. The host that can run them
  would use `scripts/run-lwh.sh` and `scripts/run-scenario.sh` in a logged-in session (the same instruments
  named in `docs/path-to-playtest.md`: *what only the owner can close*).
- **The rendered surfaces.** That a station screen shows the portfolio is asserted from
  `module/ui/ui_lwh_engineering.cpp` and the register; the rendered check is the owner's.
- **The full list of inventory primitives, counted independently.** The inventory is a single mechanical
  pass: the 479 names from `tools/hooks/surface.py` minus the hook names this document cites (the command is
  in `docs/evidence/the-derivation.md`). It is reproducible but not independently witnessed; the grouping by
  register section is the same reading, not a second instrument. Re-triaging the hooks is forbidden here, so
  the triage's own categories are cited rather than re-derived.

## Judgement calls, named as calls

1. **The field list is the design's set, taken in full.** The 16 rows are `docs/scenario-atlas.md`'s
   authoring contract (8), the post-not-name law (1), *what a scenario must therefore declare* (5) and *a
   first slate*'s two named fields. O15's summary omits the positions and the canon root; the atlas names
   them, and the brief says to read the atlas rather than trust the summary.
2. **"Ready" is a property of the primitive half, and said so.** Because no declaration layer exists, a
   strict reading would make every field not-ready. The verdicts are defined above as *can the engine answer
   this field and be reached*, with the declaration-layer absence recorded once, as the cross-cutting
   finding, instead of appended to all 16 rows.
3. **"Nothing" is treated as a call path, not a failure to look.** `PostsNeeded` and `CoverWithCrew` are
   *partial* precisely because their only reach is the tests; the canon root is *absent* because it has no
   home at all.
4. **The register rows were left untouched.** The brief's *what is NOT in this pass* says "do not touch …
   the register rows". The derivation is therefore a sibling document, the way
   `docs/designs-against-code.md` is a sibling of `docs/omissions.md`, and no existing row was edited.
5. **The roadmap was not updated.** Marking `docs/path-to-playtest.md`'s queued item 3 done is the ledger's
   business, and this pass repairs nothing. It is named here so it is not lost.
