# The hook triage — which of the unreachable hooks a player should reach

**What this is.** The companion to `docs/hook-register.md`. The register states, per hook, who can
reach it; this classifies every hook the register marks **reached only from the developer console**
(129) or **reached by nothing yet** (35), and says for each whether that is right. The register is the
index; this is the judgement the brief asked for, and the reason it is the deliverable is that the
number is not the defect — *which* of the 164 matter is.

**The method, and it is a judgement.** The classification is a reading of the code, the owning document
and the register's own call sites (`tools/hooks/reach_report.py` gives the call sites by entry surface).
A hook is a **genuine gap** when it is an act or a decision the design names as a player's or a post's,
and no in-game surface reaches it. It is **diagnostic by design** when its only consumers are the tests,
the `g_shipTest` harness, another hook, or a console readout — the checks and the other systems need it,
a player does not. It is **dead** only when there is no caller anywhere *and* no reason and nothing
waiting on it. The brief's three states left one case it could not name, and the register names it (its
"fourth case"): a hook the simulation raises on its own in play, which no player deliberately invokes.
Those are marked **simulation-raised** below, with the player's own lever named beside them.

**Read this with the register's three states in view.** *Diagnostic by design* means "console-only is
correct". *Genuine gap* is the work. *Simulation-raised* is the register's own fourth case, kept as a
named category rather than forced into the wrong one.

## Counts

| group | diagnostic | genuine gap | simulation-raised | dead |
|---|---|---|---|---|
| entry points (console-only, 37 read) | 13 | 23 | 1 | 0 |
| situations (console-only) | 5 | 8 | 5 | 0 |
| outcomes (console-only) | 1 | 19 | 0 | 0 |
| readers (console-only) | 47 | 7 | 0 | 0 |
| reached by nothing | 30 | 5 | 0 | 0 |
| **total (164)** | **96** | **62** | **6** | **0** |

**No hook is dead.** All six with no caller anywhere have a reason and something waiting: the three
species readers want a crew record, and `PromiseKindName`, `LogVisibilityName` and `BandGrantCount` are
the name or counter a display that is planned but not built would read. The brief expected some deaths;
there are none, and that is a finding, not an omission. Deletion stays its own decision and is not taken
here.

## 1. Entry points

| hook | triage | why |
|---|---|---|
| `OfferRising` | **genuine gap** | A player should be offered the chance to rise when a post must be held beyond the hand in front of it and nobody qualified is free (`docs/rising-to-the-occasion.md`); only `ship rise` offers it. The offer wants a crisis, which the fights of this pass supply but no director yet raises (walkthrough G4). |
| `OfferRisingTo` | **genuine gap** | The same offer, addressed by post to a named hand; only the console reaches it. Same crisis owner as `OfferRising`. |
| `AttemptRising` | **genuine gap** | The one attempt end to end — the odds beyond effective skill, the cost, and the memory left in the witnesses; only `ship rise` and the ship tests reach it. |
| `NextRisingRoll` | diagnostic | The deterministic draw `AttemptRising` consumes and the rising unit tests read; a play surface calls `AttemptRising`, which draws it internally. |
| `WritePersonalLog` | **genuine gap** | The private log screen (`ui_lwh_personal`) can only read; a player should write in their own log, and the write path is built and tested. |
| `VoiceWrite` | diagnostic | Voice-pipeline seams; read only by the audio harness (g_shipTest 81) and the audio/meeting unit tests (`TestAudioSaveRoundTrip`, `TestTrackOwnership`, `TestCueCannotStopALine`). |
| `VoiceRetire` | diagnostic | Voice-pipeline seams; read only by the audio harness (g_shipTest 81) and the audio/meeting unit tests (`TestAudioSaveRoundTrip`, `TestTrackOwnership`, `TestCueCannotStopALine`). |
| `EmitCue` | diagnostic | The cue-emit seam; harness 81 and `TestCueEmitSite`. The meeting overlay (the meeting programme's phase three) is its eventual play surface and is not built. |
| `QueueRender` | diagnostic | Render-queue and finished-render seams; harness 81 and `TestRenderKeyAndCache`. |
| `CacheRendered` | diagnostic | Render-queue and finished-render seams; harness 81 and `TestRenderKeyAndCache`. |
| `PruneVoiceCache` | diagnostic | The cache-prune that goes with the save; the console and `TestRenderKeyAndCache` drive it. |
| `PlanMeetingAudio` | diagnostic | Plans a meeting's every line for synthesis; harness 81 and `TestPlanMeetingAudio`. The meeting programme owns its surface. |
| `WarmVoice` | diagnostic | Warms the model off the critical path; harness 81 and `TestPlanMeetingAudio`. Meeting programme. |
| `Repair` | diagnostic | The repair economy's model lever; the job queue (`Jobs`, play) and the unit tests drive it, and the player's own lever is `OrderRepairFirst` (play). |
| `RepairDeck` | **genuine gap** | Sealing a holed deck is a damage-control decision (`docs/scenario-atlas.md`, scenario 8); only `ship seal` seals. The force field (play) holds air but does not seal. |
| `LoadAwayKit` | **genuine gap** | Loading an away kit — tricorders, phasers, EV suits, one charge — is the player's decision (`docs/exploration-and-science.md`); the survey screen spends the charge but nothing loads the kit. |
| `LaunchShuttle` | **genuine gap** | Launching a shuttle with a manifest is a player decision; the load screen is deliberately unbuilt (walkthrough G19), so this waits on that screen. |
| `RecallShuttle` | **genuine gap** | A shuttle docking is the other half of the launch decision; waits on the same load screen (G19). |
| `LoseShuttle` | diagnostic | The model's loss path; `TestShuttles` and the console drive it. The simulation wrecks shuttles through `ShuttleBayHit`. |
| `ShuttleBayHit` | simulation-raised | A consequence of an enemy hit on the bay; the console can raise it and the player meets it, but a player does not hit their own bay. |
| `RebuildShuttle` | **genuine gap** | Ordering a replacement shuttle is command's — the second bay builds it as a job (`docs/crew-work.md`); only `ship rebuild`. |
| `TractorWreck` | **genuine gap** | The tractor beam stripping a derelict is a Tactical or shuttlebay decision; only `ship tractor`. |
| `TractorHold` | **genuine gap** | Holding a contact with the tractor beam is the other Tactical decision there; only `ship tractor`. |
| `LaunchProbe` | **genuine gap** | A probe is the safe way to look at something hostile (`docs/exploration-and-science.md`); only `ship probe`. |
| `RespondPhenomenon` | **genuine gap** | Answering a phenomenon — harmonics, geometry, distance, or do not touch — is the science decision; only `ship study`. |
| `FabricateParts` | diagnostic | Material into spare parts is the model lever; `OrderBuild` (play) is command's order and the job queue works it, so the direct lever stays a console/test entry. |
| `FabricateRations` | **genuine gap** | Preparing rations is the galley decision and morale's driver (walkthrough deck 2); only `ship rations`. |
| `MineBelt` | **genuine gap** | Working a resource belt — mining and siphoning — is an acquisition decision; only `ship mine`. |
| `EVA` | **genuine gap** | An away party working a belt or a wreck by hand is the away decision; only `ship eva`. |
| `TakeSurvivors` | **genuine gap** | Taking survivors or refugees aboard is the population-pressure decision (`docs/scenario-atlas.md`); only `ship survivors`. |
| `ObservePreWarp` | **genuine gap** | First contact: observing a pre-warp civilisation from orbit; only `ship observe`. |
| `InterferePreWarp` | **genuine gap** | The Prime Directive choice, the other half of first contact; only `ship interfere`. |
| `ReconcileFactions` | **genuine gap** | Reconciling the Maquis and Starfleet is command's act (the reconciliation arc); only `ship reconcile`. |
| `Survey` | **genuine gap** | Astrometrics' survey of the beacons one jump away builds the chart (walkthrough deck 8); only `ship survey`. The survey screen spends a tricorder on the site or a compartment, not on the beacon chart. |
| `CatchUp` | diagnostic | The wall-clock catch-up the clock runs on return; harness 20 and `TestModesAndClocks`. |
| `Suspend` | **genuine gap** | Leaving the ship standing is one of the two exits a player chooses (`docs/ship-model.md`); only `ship suspend` (ironman refuses it). |
| `Sleep` | **genuine gap** | The sleep state — the player skips time at the accelerated rate, in steps or all at once — is a player action (`docs/ship-model.md`); only `ship sleep`. |

## 2. Situations the simulation generates on its own

| hook | triage | why |
|---|---|---|
| `EffectiveSkill` | diagnostic | The derivation read the rising unit tests and harness 60 use; the console prints it through `WorkReading`. |
| `WorkContextName` | diagnostic | A name read; the console and the work-bit tests read it. |
| `WorkReading` | **genuine gap** | The whole picture for one person at one system — who, what they can do, every factor that moved the odds — is meant for the personnel console (`docs/character-attributes.md`); no screen shows it and the station gate refuses `operate`. |
| `PendingMeetings` | **genuine gap** | The meeting queue is the staff-meeting system's surface; no screen shows it, so a player cannot attend a meeting (`docs/programme-meetings-and-voice.md`, phase three). |
| `TakeBrief` | **genuine gap** | Dropping a queued brief is the worker taking it; only the console and the harness. Same unbuilt meeting surface. |
| `ApplyMeetingOutcome` | **genuine gap** | The decision a meeting reached is the player's at a meeting screen; only `ship meeting decide`. Same surface. |
| `DamageSystem` | diagnostic | The model's damage lever, driven by the g_shipTest harness and the unit tests; combat writes system health directly in `UpdateOutside`, so this is the authoring/test entry. |
| `DamageSource` | diagnostic | The source-damage lever, driven by harness and tests; the simulation degrades sources through the power model. |
| `BreachDeck` | simulation-raised | A hull breach is raised in play by a weakly-deflected jump (`Jump` is play) and by combat; a player does not hole a deck on purpose. The player's response is the field (play) and the seal (`RepairDeck`, a genuine gap). |
| `IgniteDeck` | simulation-raised | A combat hit starts a fire every third penetrating blow (`UpdateOutside`); the player fights it through the crew's posts and does not start it. |
| `Board` | simulation-raised | An enemy sends its party when our shields drop; the player's acts are the responses — `OrderSecurityTo` (play) and `OrderAdvance` (wired below). |
| `BoardAs` | simulation-raised | An enemy sends its party when our shields drop; the player's acts are the responses — `OrderSecurityTo` (play) and `OrderAdvance` (wired below). |
| `BoardBorg` | simulation-raised | As `Board`: the Borg send drones when our shields drop; the player's acts are the counter-play (wired below) and the squad. |
| `Remodulate` | **genuine gap** | The phaser adapter's rotating modulation is the player's counter when the Borg adapt (`docs/borg-incursion.md`); only `ship remodulate` — **wired in this pass** (Tactical console key `M`). |
| `RaidVinculum` | **genuine gap** | A vinculum raid severs the Collective's local coordination, at a cost; only `ship vinculum` — **wired in this pass** (Tactical console key `N`). |
| `JumpStartFromHolodeck` | **genuine gap** | Tying in a holodeck reactor with the main grid down is a player's desperate choice, a trap with a cost (`docs/ship-systems.md`; VOY "Parallax"); only `ship jumpstart`. |
| `Pursued` | diagnostic | A read: whether a raider follows and for how many jumps; harness 19 and `TestEnemySystemsAndOutsideChoices`. |
| `AdvanceSector` | **genuine gap** | Crossing into the next sector is the destination decision at the end of a sector (`docs/endings.md`); only `ship cross` (the command chart sees the end). |

## 3. Outcomes content can ask for

| hook | triage | why |
|---|---|---|
| `OrderAdvance` | **genuine gap** | Command sends a security squad to retake a deck (`docs/borg-incursion.md`); only `ship advance` — **wired in this pass** (command console key `S`). |
| `SetAllocationBy` | diagnostic | The named-officer allocation under a standing band delegation; the meeting seam and `TestRecommendationAndDelegation` drive it. The player's own form is `SetAllocation` (play). |
| `Hearing` | **genuine gap** | A hearing — guilty or not — is command's justice act (`docs/access-and-authority.md`); only `ship hearing`. |
| `SetMobileEmitter` | **genuine gap** | The mobile emitter is an artifact that lets the EMH work away from sickbay; only `ship emitter`. |
| `EndHolodeckProgram` | **genuine gap** | Ending a holodeck program is the holodeck post's act; only `ship holoend`. |
| `SetAirponics` | **genuine gap** | Growing food in the airponics bay is a galley decision, the non-replicated half of feeding the crew; only `ship airponics`. |
| `SetTarget` | **genuine gap** | Tactical picks which enemy subsystem to break once the shields are down — the first combat decision (`docs/ship-systems.md`); only `ship target` — **wired in this pass** (Tactical console key `A`). |
| `Delegate` | **genuine gap** | A department head grants a shift's access to a station (post+person); only `ship delegate`. |
| `RevokeDelegation` | **genuine gap** | Taking a grant back, at once; only `ship revoke`. |
| `RevokeCredential` | **genuine gap** | The crew can take back a cross-qualification too; only `ship credrevoke`. |
| `LockOut` | **genuine gap** | A senior officer shuts a post-holder out of a station (the two-lock model); only `ship lockout`. |
| `ClearLockout` | **genuine gap** | Clearing a lock-out; only `ship unlock`. |
| `Train` | **genuine gap** | A crew member earns a cross-qualification by training; only `ship train`. |
| `Brig` | **genuine gap** | Discipline and justice: confining a crew member; only `ship brig`. |
| `HoldFuneral` | **genuine gap** | A funeral, when there is time, lifts the crew who have been lost and are missed (`docs/morale.md`); command's act, only `ship funeral`. |
| `Brief` | **genuine gap** | Command tells the crew of an event, writing the mark to all of them; only `ship brief`. The meeting programme will carry the surface. |
| `MakePromise` | **genuine gap** | An officer commits in front of a crew member — a promise that can be kept or broken; only `ship promise`. |
| `RunHolodeck` | **genuine gap** | Running a holodeck use — recreation, training, therapy or forensic — is the holodeck post's act; only `ship holo`. |
| `ImproveQuarters` | **genuine gap** | Improving the crew's quarters at a cost in material (morale); only `ship quarters`. |
| `SetRole` | **genuine gap** | The player's own role — any post, in command, or Munro — should be chosen in play; today only cvar plus `ship role`. The start-state configurator (the other lane) owns the surface. |

## 4. Readers content can consult

| hook | triage | why |
|---|---|---|
| `OperatedFrom` | diagnostic | The remote call-up test; g_ship's call-up line and `TestAccessAndAuthority`/`TestStations` read it. |
| `AllocationByName` | diagnostic | A name read for allocation provenance; no caller but the console. |
| `ControllerName` | diagnostic | The controller name for `ship controller`; the player's own read is the intruder line (`lwh_ship_aboard`). |
| `RisingOutcomeName` | diagnostic | A name read; the console and the rising tests. |
| `RisingDriveAtStake` | diagnostic | The drive the offer reads to decide who rises; the offer's reason text and the rising tests consume it. |
| `RisingOdds` | diagnostic | The odds read for an offer; the rising tests. |
| `DeliveryName` | diagnostic | Voice-delivery reads (name, synthesis knob, known); harness 80/81 and the audio/meeting tests. |
| `DeliveryExaggeration` | diagnostic | Voice-delivery reads (name, synthesis knob, known); harness 80/81 and the audio/meeting tests. |
| `DeliveryKnown` | diagnostic | Voice-delivery reads (name, synthesis knob, known); harness 80/81 and the audio/meeting tests. |
| `MeetingEffectName` | diagnostic | A meeting-effect name; the console and `TestMeetingSaveRoundTrip`. |
| `ResolveSpeaker` | diagnostic | Who says a meeting line; the meeting plumbing (tests), owned by the meeting programme. |
| `LineToSynthesis` | diagnostic | A meeting line turned into a synthesis request; harness 80/81 and the audio/meeting tests. |
| `VoiceProducerName` | diagnostic | Voice-mixer and track-ownership reads; harness 81 and `TestTrackOwnership`/the cue tests. |
| `TrackOwner` | diagnostic | Voice-mixer and track-ownership reads; harness 81 and `TestTrackOwnership`/the cue tests. |
| `OwnsTrack` | diagnostic | Voice-mixer and track-ownership reads; harness 81 and `TestTrackOwnership`/the cue tests. |
| `TrackBusy` | diagnostic | Voice-mixer and track-ownership reads; harness 81 and `TestTrackOwnership`/the cue tests. |
| `CueName` | diagnostic | Cue reads; harness 81 and `TestCueSet`/the audio tests. |
| `CueClip` | diagnostic | Cue reads; harness 81 and `TestCueSet`/the audio tests. |
| `CuePurpose` | diagnostic | Cue reads; harness 81 and `TestCueSet`/the audio tests. |
| `RenderKey` | diagnostic | Render-cache reads; harness 81 and `TestRenderKeyAndCache`. |
| `VoiceCachePath` | diagnostic | Render-cache reads; harness 81 and `TestRenderKeyAndCache`. |
| `VoiceCached` | diagnostic | Render-cache reads; harness 81 and `TestRenderKeyAndCache`. |
| `VoiceQueued` | diagnostic | Render-cache reads; harness 81 and `TestRenderKeyAndCache`. |
| `AllocationProvenance` | diagnostic | The provenance string for the console; the console's WHO column already carries it. |
| `ShuttleLocationName` | diagnostic | Shuttle reads for `ship shuttle`; `TestShuttles`. The shuttle screen is unbuilt (G19). |
| `ShuttlesInBay` | diagnostic | Shuttle reads for `ship shuttle`; `TestShuttles`. The shuttle screen is unbuilt (G19). |
| `ShuttlesAway` | diagnostic | Shuttle reads for `ship shuttle`; `TestShuttles`. The shuttle screen is unbuilt (G19). |
| `PhenomenonResponseName` | diagnostic | A phenomenon-response name; the console. |
| `Refugees` | **genuine gap** | The number of survivors and refugees aboard is population pressure a player should see; only `ship survivors` prints it. |
| `Resentment` | **genuine gap** | The Maquis split as an arc — resentment between the two, and what reconciles them; only `ship interfere`/`ship reconcile` print it, and no screen carries it. |
| `BorgAwareness` | **genuine gap** | The Borg's strategic awareness is the clock behind the incursion (`docs/borg-incursion.md`); only `ship awareness` prints it. |
| `MobileEmitter` | **genuine gap** | Whether the mobile emitter is aboard, a sickbay read the player acts on; only `ship emitter`. |
| `Airponics` | **genuine gap** | Whether the airponics bay is growing food, a galley read; only `ship airponics`. |
| `Target` | diagnostic | The target read; harness 19 and `TestEnemySystemsAndOutsideChoices`. The console shows it through `lwh_ship_enemy` and the new `lwh_ship_target_idx`. |
| `MaySuspend` | diagnostic | Ironman policy and the leave-standing mark; harness 51 and `TestSleepAndExits`. |
| `LeftStanding` | diagnostic | Ironman policy and the leave-standing mark; harness 51 and `TestSleepAndExits`. |
| `MayOperate` | diagnostic | The crew's permission read; the access and rank tests. |
| `MayCallAlert` | diagnostic | May this hand call the alert; the console gate and `TestRankAndRoles`. |
| `DelegatedTo` | diagnostic | Access-and-authority reads the gate and the consoles use; `TestAccessAndAuthority` and the rank tests read them. |
| `OverrideActive` | diagnostic | Access-and-authority reads the gate and the consoles use; `TestAccessAndAuthority` and the rank tests read them. |
| `LockedOut` | diagnostic | Access-and-authority reads the gate and the consoles use; `TestAccessAndAuthority` and the rank tests read them. |
| `LockoutNotice` | diagnostic | Access-and-authority reads the gate and the consoles use; `TestAccessAndAuthority` and the rank tests read them. |
| `MayCallUp` | diagnostic | Access-and-authority reads the gate and the consoles use; `TestAccessAndAuthority` and the rank tests read them. |
| `AccessRefusal` | diagnostic | Access-and-authority reads the gate and the consoles use; `TestAccessAndAuthority` and the rank tests read them. |
| `OperatedFromRefusal` | diagnostic | Access-and-authority reads the gate and the consoles use; `TestAccessAndAuthority` and the rank tests read them. |
| `Qualified` | diagnostic | Access-and-authority reads the gate and the consoles use; `TestAccessAndAuthority` and the rank tests read them. |
| `Brigged` | diagnostic | Whether a crew member is confined; the console and the justice tests. |
| `PlayerDead` | diagnostic | Whether the player is dead; harness 56 and `TestPlayerInTheWorld`. |
| `Recall` | diagnostic | What a crew member recalls, its provenance, and how many marks; the console and the memory tests. |
| `RecallSource` | diagnostic | What a crew member recalls, its provenance, and how many marks; the console and the memory tests. |
| `MemoryCount` | diagnostic | What a crew member recalls, its provenance, and how many marks; the console and the memory tests. |
| `Promises` | **genuine gap** | The promises held, so they can mature — a read the officer who made them should have; only `ship promises`. |
| `PlayerMayOperate` | diagnostic | Access-and-authority reads the gate and the consoles use; `TestAccessAndAuthority` and the rank tests read them. |
| `CaptainLog` | **genuine gap** | The captain's log as an authored artifact (`docs/the-record-and-the-log.md`) is written and read at the command console, but the console has no key for it; only `ship captain`. |

## 5. Reached by nothing yet

| hook | triage | why |
|---|---|---|
| `RollAnomaly` | diagnostic | A pure roll helper; `TestConditionOdds` and `TestCrewOnDeck` drive it. |
| `TraitName` | diagnostic | Names for the character derivation's enums; `TestRisingSaveRoundTrip` reads them and no surface shows them yet. |
| `DesireName` | diagnostic | Names for the character derivation's enums; `TestRisingSaveRoundTrip` reads them and no surface shows them yet. |
| `NeedName` | diagnostic | Names for the character derivation's enums; `TestRisingSaveRoundTrip` reads them and no surface shows them yet. |
| `FearName` | diagnostic | Names for the character derivation's enums; `TestRisingSaveRoundTrip` reads them and no surface shows them yet. |
| `ConditionValenceName` | diagnostic | Names for the character derivation's enums; `TestRisingSaveRoundTrip` reads them and no surface shows them yet. |
| `ConditionClearName` | diagnostic | Names for the character derivation's enums; `TestRisingSaveRoundTrip` reads them and no surface shows them yet. |
| `SpeciesName` | diagnostic | Names for the character derivation's enums; `TestRisingSaveRoundTrip` reads them and no surface shows them yet. |
| `SpeciesCapability` | **genuine gap** | The species capability/need/susceptibility readers are built and no surface uses them; a crew record should say what a species can do and what it needs (`docs/character-derivation.md`). The register's G5 shape one layer down. |
| `SpeciesNeed` | **genuine gap** | The species capability/need/susceptibility readers are built and no surface uses them; a crew record should say what a species can do and what it needs (`docs/character-derivation.md`). The register's G5 shape one layer down. |
| `SpeciesSusceptibility` | **genuine gap** | The species capability/need/susceptibility readers are built and no surface uses them; a crew record should say what a species can do and what it needs (`docs/character-derivation.md`). The register's G5 shape one layer down. |
| `PromiseKindName` | diagnostic | A promise-kind name; no caller yet — the promise display's name. Waits on the promise surface (`Promises`, a genuine gap). |
| `MannerLine` | **genuine gap** | The crew's manner — what a person says at the morale they carry — is the open half of the morale contract (walkthrough G14); nothing reads it. |
| `RetellRising` | diagnostic | The crew retell a heroism, so a mark persists only if it is told; the witness tests drive it, and a retelling surface does not exist. It is the model act the simulation would raise. |
| `PostsNeeded` | diagnostic | The crew model's post coverage (posts needed, and how far the crew cover them); `TestHundredCrew`. |
| `CoverWithCrew` | diagnostic | The crew model's post coverage (posts needed, and how far the crew cover them); `TestHundredCrew`. |
| `LogVisibilityName` | diagnostic | A log-visibility name; no caller anywhere, and it is the log display's name helper. |
| `VoiceTrackName` | diagnostic | A voice-track name; `TestAudioSaveRoundTrip`. |
| `ActiveReply` | diagnostic | The active reply on a track; `TestCueCannotStopALine` and `TestTrackOwnership`. |
| `CueIsLexical` | diagnostic | Whether a cue is lexical; `TestCueSet`. |
| `NewShip` | diagnostic | The default ship constructor; every ship test builds from it, and g_ship builds the game's ship. |
| `BandGrantCount` | diagnostic | A count of band grants; no caller anywhere — a counter a band display would read (`docs/power-assignment.md`, Task C). |
| `StrandShuttle` | diagnostic | The shuttle-stranded model act; `TestShuttles`. |
| `Held` | diagnostic | Whether the tractor holds the contact; `TestMaterialsAndTravel`. |
| `TravelSafe` | diagnostic | Whether a jump is safe (the three travel systems); the travel tests. |
| `PursuitJumps` | diagnostic | How many jumps the pursuit has left; `TestEnemySystemsAndOutsideChoices`. |
| `AtEnd` | diagnostic | The sector's end, the win, and the sector number; `TestEnemySystemsAndOutsideChoices`. The console prints them. |
| `Won` | diagnostic | The sector's end, the win, and the sector number; `TestEnemySystemsAndOutsideChoices`. The console prints them. |
| `SectorNumber` | diagnostic | The sector's end, the win, and the sector number; `TestEnemySystemsAndOutsideChoices`. The console prints them. |
| `Course` | diagnostic | The course set; `TestStationPurposes`. The Conn reads `lwh_ship_course`. |
| `SavesAllowed` | diagnostic | Whether saving is allowed (the ironman policy); `TestModesAndClocks`. |
| `PlayerDeck` | diagnostic | The player's deck; `TestPlayerInTheWorld`. |
| `AssumeCommand` | **genuine gap** | Command should devolve from a player who is dead or assimilated to the senior fit officer; nothing calls this, so the ship does not do it. A genuine gap with no surface at all. |
| `LogsPurged` | diagnostic | Whether the published logs have been purged; `TestMonthReportAndToll` and `TestTheTwoLogs`. |
| `Describe` | diagnostic | The one-screen status report for the console and the tests' failure messages. |

## The prioritised list — the 62 genuine gaps, ordered

The order is **what a player meets first**, and second **what the scenario content needs** (the atlas's
scenarios, `docs/scenario-atlas.md`). G5 leads as the brief requires: the systems under stress are the
first thing a player meets and the thing no surface reaches. The four at the very top were **wired in
this pass** and are the slice (`docs/evidence/reachability.md`). The rest are the backlog, in order.

**Tier 1 — the fight (G5).** A player meets a fight before anything else, by hailing a hostile beacon
or answering a distress call at Operations. The needs, in the order a fight presents them:

1. **`SetTarget`** — what to break once their shields are down: hull, weapons, engines or the shield
   generator. *Wired (Tactical `A`).*
2. **`Remodulate`** — the phaser adapter's rotating modulation, the only answer when the Borg adapt.
   *Wired (Tactical `M`).*
3. **`RaidVinculum`** — sever the Collective's local coordination, at a cost in a wounded security
   hand. *Wired (Tactical `N`).*
4. **`OrderAdvance`** — command sends the security squad to retake an assimilated deck, one deck at a
   time. *Wired (Command `S`).*
5. **`RepairDeck`** — seal a holed deck; the field holds air, but only a seal makes the deck whole.
6. **`TractorHold`** / **`TractorWreck`** — the beam that locks a contact so it cannot run, and strips
   a derelict for parts.
7. **`JumpStartFromHolodeck`** — the trap that can follow a core loss; a fight can start the cascade,
   and the choice to tie in a holodeck reactor is the player's.
8. **`AdvanceSector`** — crossing the end of a sector, the destination decision G4's content will need.

**Tier 2 — the ship and the record, which a player meets every sitting.**

9. **`Sleep`** / 10. **`Suspend`** — the two exits of `docs/ship-model.md`: skip time, or leave the ship
   standing.
11. **`WritePersonalLog`** — the private log screen reads and cannot write.
12. **`CaptainLog`** — the command console's captain's log, written but with no key.
13. **`Promises`** — the claims an officer is holding, so they can mature.
14. **`MannerLine`** — the crew's manner, the open half of the morale contract (walkthrough G14).

**Tier 3 — the science and the stores.**

15. **`Survey`** / 16. **`LaunchProbe`** / 17. **`RespondPhenomenon`** — explore, look safely, answer.
18. **`LoadAwayKit`** — load the away kit the survey screen's charge is spent from.
19. **`MineBelt`** / 20. **`EVA`** — work a belt by beam or by hand.
21. **`FabricateRations`** / 22. **`Airponics`** — feed the crew, replicated and grown.
23. **`TakeSurvivors`** / 24. **`Refugees`** — population pressure, taken on and read.

**Tier 4 — first contact and the factions.**

25. **`ObservePreWarp`** / 26. **`InterferePreWarp`** — the Prime Directive choice.
27. **`ReconcileFactions`** / 28. **`Resentment`** — the Maquis arc, its act and its read.

**Tier 5 — the crew, the post and command's acts.** These are the post-not-name hooks: a player
holding the post should exercise them, and a screen does not exist. They need a personnel/security
surface as much as wiring.

29. **`WorkReading`** — the operator's own factors before a roll (the personnel read).
30. **`PendingMeetings`** / 31. **`TakeBrief`** / 32. **`ApplyMeetingOutcome`** — the staff meeting,
    whose screen is the meeting programme's phase three.
33. **`Delegate`** / 34. **`RevokeDelegation`** / 35. **`RevokeCredential`** / 36. **`LockOut`** /
    37. **`ClearLockout`** — the two-lock model in play.
38. **`Train`** — a cross-qualification earned.
39. **`Brig`** / 40. **`Hearing`** — discipline and justice.
41. **`HoldFuneral`** / 42. **`Brief`** / 43. **`MakePromise`** — grief, the crew told, and a promise
    made in front of a person.
44. **`RunHolodeck`** / 45. **`EndHolodeckProgram`** / 46. **`SetMobileEmitter`** — the holodeck and
    the Doctor away from sickbay.
47. **`ImproveQuarters`** — living conditions, at a cost.
48. **`SetRole`** — the player's own role; the start-state configurator (the other lane) owns it.

**Tier 6 — rising to the occasion.** Needs a crisis to offer the act (G5's fights; ultimately a
director, G4).

49. **`OfferRising`** / 50. **`OfferRisingTo`** / 51. **`AttemptRising`**.

**Tier 7 — the ship's own state, and the last of the nothing-list.**

52. **`BorgAwareness`** — the incursion's clock, read.
53. **`MobileEmitter`** — the emitter aboard, read.
54. **`AssumeCommand`** — command devolves from a dead player; nothing calls it at all.
55. **`SpeciesCapability`** / 56. **`SpeciesNeed`** / 57. **`SpeciesSusceptibility`** — a crew record
    should say what a species can do and needs.
58. **`LaunchShuttle`** / 59. **`RecallShuttle`** / 60. **`RebuildShuttle`** — the shuttle load screen
    (walkthrough G19) is the gate; the launch, dock and rebuild wait on it.
61. **`FabricateParts`** — the direct material-to-parts lever (the order, `OrderBuild`, is already
    play).
62. **`SetAllocationBy`** — the delegated-officer allocation; the meeting seam is its surface.

## What needs `ship_core`, and therefore waits

**Nothing in the wired slice touched `module/ship/ship_core.{h,cpp}`.** The four hooks already existed;
the pass only reached them from the station surfaces. Three genuine gaps do need the model (or the
simulation's own calls), and they are **reported, not implemented**, as the brief requires:

- **The rising offer's trigger.** `OfferRising`/`OfferRisingTo`/`AttemptRising` are built and reached by
  `ship rise`, but nothing in the simulation *offers* the act: no crisis calls `OfferRising`. Making the
  offer arrive needs the simulation to raise it (a `ship_core` change, `docs/rising-to-the-occasion.md`),
  and its judgement — which post, which drive — is the model's. **Waits on the configurator lane and the
  owner.**
- **`AssumeCommand`.** The mechanism is built and nothing calls it. When the player is dead or
  assimilated, command should devolve to the senior fit officer. Whether the call belongs in `ship_core`
  (with the crew arbitration) or in `g_ship` (which hosts the player's body) is a design call; it is not
  made in this pass.
- **The provocation director (G4).** A player cannot deliberately bring on a boarding, a fire or a
  breach; the simulation raises them once a fight starts, and a fight starts in play (hail/answer). A
  director that recruits the stress situations from the ship's state is `docs/scenario-atlas.md`'s
  content work and the walkthrough's G4, and it is a `ship_core`/`module` change. **Not this pass.** The
  player's *responses* — the wired slice — are what this pass could honestly reach.

## Judgement calls, named as calls

1. **The triage is a judgement, and the criterion is written above.** A hook is a genuine gap when the
   design names it as a player's or a post's act and no surface reaches it; diagnostic when only the
   tests, the harness, another hook or a console readout consume it. The owner may overrule any row;
   the genuine-gap list is the one to read.
2. **The register's "fourth case" is kept as its own category (simulation-raised).** The brief's three
   states cannot hold it: `Board`, `BoardBorg`, `IgniteDeck` and `BreachDeck` are raised by the
   simulation in play, not invoked by a player. Forcing them into *diagnostic* would hide them; forcing
   them into *genuine gap* would imply a "start a boarding" button is wanted, which it is not. The
   category is named and every member carries the player's own lever.
3. **The fourth-case members are not all the same.** `DamageSystem` and `DamageSource` are classified
   *diagnostic*: the simulation writes system and source health directly in `UpdateOutside`, so these
   hooks are the harness/tests' and the authoring console's entry, not a player's act. The register's
   list groups them with the raised ones; the code does not.
4. **"Reached in play" was not claimed for the raised hooks.** The reachability tool counts a hook the
   simulation raises from `Advance` as play; the register's fourth-case call counts only deliberate
   reach. This triage keeps the register's stricter reading (a call), so the register's counts and the
   tool still differ by exactly the fourth-case members. They are named.
5. **Many genuine gaps are post-specific.** `Delegate`, `LockOut`, `Train`, `Brig` and the rest are a
   *post's* acts. A player holding that post should reach them; a player holding another should not.
   That is the post-not-name law, and it means the remedy is a surface that resolves the post to the
   hand, not a button every player sees.
6. **`SetRole` is left to the configurator lane.** The player's role and start state are that lane's
   surface; it is listed here as a genuine gap so it is not lost, and marked as owned elsewhere.
7. **No hook was deleted, and none is called dead.** The brief expected some; there are none, and the
   six with no caller are reported with what waits on them instead.

