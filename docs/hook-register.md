# The hook register — what the simulation offers, and who can reach it

**What this is.** The bridge between the simulation and the scenario set. `docs/scenario-atlas.md`
says the scenarios will be authored *against* the systems that exist; this is the index of what
those systems offer, and — the part that matters — **whether a player can reach each one**. Two
mechanisms have already been built and wasted because nothing reaches them (`OfferRising`, and the
systems under stress: `docs/walkthrough.md` finding **G5**). This register is the check that finds
the next one before it is wasted.

**This is an index, not a source of truth.** Every row points at the document that owns the
design. Where the design belongs in `docs/power-assignment.md`, it is not restated here.

The primary source is the public surface of `module/ship/ship_core.h`; the live behaviour is
`module/ship/ship_core.cpp`, and the surfaces that reach it are `module/ship/g_ship.cpp` (the
developer console and the `Publish` that fills the console cvars) and `module/ui/` (the in-game
station screens).

## How to re-derive this, rather than re-write it

```sh
# the public surface (the check's source of truth):
python3 tools/hooks/surface.py module/ship/ship_core.h
# collect the declaration, its signature and its header line:
python3 tools/hooks/surface.py --self-test
# the reachability evidence (call sites by entry surface), not a verdict:
python3 tools/hooks/reach_report.py --detail
# and the freshness check that fails when a function is missing from this register:
scripts/hooks-check.sh
```

`scripts/hooks-check.sh` re-derives the surface and compares it to the hook names in the tables
below. **It does not judge reachability** — that is a person's call, made here from the code and
the evidence. It catches the one thing a person always misses: **a public function that exists in
the code and not in this register.** If it fails, add the hook; do not weaken the check.

## How to read the columns

- **hook** — the public function, the name a scenario or the console calls.
- **takes** — its parameter list, exactly as declared.
- **does** — the header's own one-line description where it has one, otherwise the name read out
  (`surface.py`'s rule: the register does not invent design detail).
- **reach** — one of the three states, below.
- **law** — the post-not-name law (`docs/the-entry-point.md`, part three) applied where it binds:
  `post` = addressed by a post/system id (the compliant form); `person` = takes a resolved crew
  index, so content must resolve a post to that person and never write a name; `post+person` = the
  post-addressed and the person-addressed forms both exist, and content must use the post.
- **owner** — the document that owns the design. `—` means the header names none; that is a
  document to write, not a gap in this register.

### The three states of reachability (Task B)

These are not the same claim, and the third is the one that hides:

- **play** — a player can get here without the developer console: an in-game screen's key,
  a scheduled event, or the simulation raising it in normal play.
- **console** — it works, and only the developer console (`ship ...`, `Svcmd_Ship_f`) or the
  `g_shipTest` harness reaches it. A player cannot get here by playing.
- **nothing** — no path in the game reaches it at all: the mechanism exists and nothing calls it
  outside the unit tests. This is the state that hides, and the reason to look.

**A fourth case the brief's three states did not separate, named as a call:** a hook the
simulation raises on its own in play, but which no player can deliberately invoke. It is marked
`console` (because the only *deliberate* reach is the console) and flagged in the unreachable list
below. `Board`, `BoardAs`, `BoardBorg`, `IgniteDeck`, `BreachDeck`, `DamageSystem` and
`DamageSource` are the members: the simulation starts a fight's consequences itself, and no play
surface starts the fight. Counting them under `console` is the honest reading of *can a player reach
it on purpose*.

**And the brief's own account is corrected here, because it is wrong on one point and the code is the
authority.** The brief calls `OfferRising` *reached by nothing yet*; the code and
`docs/evidence/rising-to-the-occasion.md` show `ship rise` reaches it, so it is *reached only from the
developer console*, one state short of the brief's claim. **Nothing in play reaches it**, which is the
substance of the brief's point and stands.

## The register, by group

### 1. Entry points — what content can call  
*74 hooks.*

| hook | takes | does (the header's line, or the name) | reach | law | owner |
|---|---|---|---|---|---|
| `OfferRising` | `const Ship &s, int post` | offer rising | console | post | — |
| `OfferRisingTo` | `const Ship &s, int post, int crew` | The same, for a named hand: the console and a scenario address the post, and the simulation resolves the post to a person (docs/the-entry-point.md, the post-not-name law) | console | post+person | docs/the-entry-point.md |
| `AttemptRising` | `Ship &s, int post, int crew, uint32_t roll, std::string *out = nullptr` | One attempt, end to end: the decision from the drives and the morale (a refusal is logged, so it is legible), the roll beyond effective skill, the cost (a condition,... | console | post+person | — |
| `NextRisingRoll` | `Ship &s` | The deterministic draw the engine passes to AttemptRising, from the same saved counter UseSystem draws on (s.riskRolls), so the act's roll replays identically across... | console | — | — |
| `RetellRising` | `Ship &s, int hero` | The crew retell it: the mark is reinforced, so a heroism the crew keep telling persists, and one nobody retells fades (a mark's salience decays unless reinforced) | nothing | person | — |
| `LogEvent` | `Ship &s, const std::string &who, const std::string &scope, const std::string &what` | log event | play | — | — |
| `WritePersonalLog` | `Ship &s, int owner, const std::string &what` | Write a private entry | console | person | — |
| `VoiceWrite` | `VoiceMixer &m, uint8_t producer, uint8_t track, const std::string &asset, int cue = -1` | Start a reply on `track` as `producer` | console | — | — |
| `VoiceRetire` | `VoiceMixer &m, uint8_t producer, int replyId` | Retire a reply | console | — | — |
| `EmitCue` | `Ship &s, VoiceMixer &m, uint8_t cue, const std::string &reason` | THE CUE EMIT SITE | console | — | — |
| `QueueRender` | `VoiceRender &vr, const RenderJob &job` | Enqueue a render | console | — | — |
| `CacheRendered` | `VoiceRender &vr, const std::string &key, const std::string &file, float duration = 0.0f` | A line the renderer finished: it enters the cache and leaves the queue, with the clip's measured duration | console | — | — |
| `PruneVoiceCache` | `VoiceRender &vr` | The cache is pruned with the save: the index is emptied and the host deletes the directory | console | — | — |
| `PlanMeetingAudio` | `VoiceRender &vr, const Ship &s, const MeetingBrief &brief, const MeetingSkeleton &sk` | Plan a meeting's audio: every line of every outcome of its skeleton, on the dialogue track, keyed and deduplicated | console | — | — |
| `WarmVoice` | `Ship &s, VoiceRender &vr, double now` | Warm the model in the async window, off the critical path (lesson 4), so the first meeting anyone sees is never the cold one | console | — | — |
| `SubmitNovelAnswer` | `Ship &s, VoiceMixer &m, const MeetingBrief &brief, const std::string &text, float modelSeconds` | Submit a novel answer: emit the cue immediately, then resolve inside the pause or defer | play | — | docs/staff-meetings.md |
| `Tick` | `Ship &s, float seconds` | Advances the ship by `seconds` of simulated (not ship) time | play | — | — |
| `UseSystem` | `Ship &s, SystemId id, float stress, const std::string &who` | Use a system under a load | play | post | docs/failure-is-content.md |
| `UseSystemBy` | `Ship &s, SystemId id, float stress, int operatorCrew` | The same use, but the operator is named by index -- the player at the console | play | post | — |
| `RecommendAllocation` | `Ship &s` | recommend allocation | play | — | — |
| `Repair` | `Ship &s, SystemId id, float amount` | repair | console | post | — |
| `RepairDeck` | `Ship &s, int deck, float amount` | repair deck | console | — | — |
| `MakeBreach` | `uint32_t seed` | make breach | play | — | — |
| `LoadAwayKit` | `Ship &s, int tricorders, int phasers, int evSuits, float charge` | Jump to a linked beacon | console | — | — |
| `Scan` | `Ship &s, int beacon` | scan | play | — | — |
| `ScanCompartment` | `Ship &s, int deck` | A tricorder reading of the ship's own compartments, at human scale -- the same kind of reading the sensors make of a site, over a smaller radius | play | — | — |
| `FireTorpedo` | `Ship &s` | One torpedo at the enemy | play | — | — |
| `ShuttleByClass` | `Ship &s, ShuttleClass c` | shuttle by class | play | — | — |
| `LaunchShuttle` | `Ship &s, ShuttleClass c, int beacon, const std::vector<int> &manifest` | launch shuttle | console | — | — |
| `RecallShuttle` | `Ship &s, ShuttleClass c` | recall shuttle | console | — | — |
| `StrandShuttle` | `Ship &s, ShuttleClass c` | strand shuttle | nothing | — | — |
| `LoseShuttle` | `Ship &s, ShuttleClass c` | lose shuttle | console | — | — |
| `ShuttleBayHit` | `Ship &s, float severity` | shuttle bay hit | console | — | — |
| `RebuildShuttle` | `Ship &s, ShuttleClass c` | rebuild shuttle | console | — | — |
| `Recomposite` | `Ship &s` | recomposite | play | — | — |
| `AcquireDilithium` | `Ship &s, DilithiumWay way` | acquire dilithium | play | — | — |
| `LocateDilithium` | `Ship &s` | A dedicated survey to locate a dilithium source: the design's "she must locate a source, which means survey, chart, detour" (docs/exploration-and-science.md) | play | — | docs/exploration-and-science.md |
| `TractorWreck` | `Ship &s` | tractor wreck | console | — | — |
| `TractorHold` | `Ship &s` | tractor hold | console | — | — |
| `LaunchProbe` | `Ship &s, int beacon` | A probe (docs/exploration-and-science.md): the safe way to look at something hostile -- launch one instead of the ship | console | — | docs/exploration-and-science.md |
| `RevealPhenomenon` | `Ship &s` | reveal phenomenon | play | — | — |
| `RespondPhenomenon` | `Ship &s, int response` | respond phenomenon | console | — | — |
| `FabricateParts` | `Ship &s, int parts` | fabricate parts | console | — | — |
| `FabricateRations` | `Ship &s, int days` | fabricate rations | console | — | — |
| `MineBelt` | `Ship &s` | mine belt | console | — | — |
| `EVA` | `Ship &s` | eva | console | — | — |
| `TakeSurvivors` | `Ship &s, int n` | take survivors | console | — | — |
| `ObservePreWarp` | `Ship &s` | First contact with a pre-warp civilisation: observe it, or interfere and own the consequence | console | — | — |
| `InterferePreWarp` | `Ship &s` | interfere pre warp | console | — | — |
| `ReconcileFactions` | `Ship &s` | reconcile factions | console | — | — |
| `ActivateEMH` | `Ship &s, bool on` | activate emh | play | — | — |
| `DamagePylon` | `Ship &s, float amount` | damage pylon | play | — | — |
| `Hail` | `Ship &s` | Choices at a beacon (S9): hail, trade, answer a distress call, or run | play | — | — |
| `Trade` | `Ship &s` | trade | play | — | — |
| `AnswerDistress` | `Ship &s` | answer distress | play | — | — |
| `Disengage` | `Ship &s` | disengage | play | — | — |
| `TransportAway` | `Ship &s, int party, std::string *outcome = nullptr` | transport away | play | — | — |
| `TransportBack` | `Ship &s, std::string *outcome = nullptr` | transport back | play | — | — |
| `Survey` | `Ship &s` | Astrometrics: a sensor survey of the beacons one jump away -- what the chart is built from | console | — | — |
| `SetCourse` | `Ship &s, int toBeacon` | set course | play | — | — |
| `CatchUp` | `Ship &s, double realSecondsAway` | catch up | console | — | — |
| `Suspend` | `Ship &s` | suspend | console | — | — |
| `Sleep` | `Ship &s, double shipSeconds` | The sleep state: the player skips time at the accelerated rate -- incrementally, or all at once -- and the ship advances as it would have | console | — | — |
| `SetPlayerDeck` | `Ship &s, int deck` | The heart of the player-in-the-world bridge | play | — | — |
| `WoundPlayer` | `Ship &s, float amount, const std::string &cause` | wound player | play | — | — |
| `AttendIncapacitatedPlayer` | `Ship &s` | The ship's response to a downed player: a medical hand is sent, and the log names who came | play | — | — |
| `AssumeCommand` | `Ship &s` | Command devolves from a player who is dead or assimilated to the senior fit officer | nothing | — | — |
| `DraftReport` | `Ship &s, int department` | The month report (docs/the-record-and-the-log.md) | play | — | docs/the-record-and-the-log.md |
| `StrikeReportLine` | `Ship &s, int line` | strike report line | play | — | — |
| `SoftenReportLine` | `Ship &s, int line, float factor = 0.5f` | soften report line | play | — | — |
| `EditReportLine` | `Ship &s, int line, const std::string &text` | edit report line | play | — | — |
| `AddReportLine` | `Ship &s, const std::string &scope, const std::string &text` | add report line | play | — | — |
| `CreateCharacter` | `Ship &s, const std::string &name, Department dept, int rank` | Character creation: the player takes the place of a generated crew member of that department -- the complement does not grow -- with the name and rank chosen (rank... | play | — | — |
| `Unpack` | `const uint8_t *data, size_t len, Ship &s` | False, leaving `s` untouched, on a truncated, foreign or newer record | play | — | — |

### 2. Situations the simulation generates on its own  
*74 hooks.*

| hook | takes | does (the header's line, or the name) | reach | law | owner |
|---|---|---|---|---|---|
| `SystemStateName` | `uint8_t state` | system state name | play | — | — |
| `FailureStateOf` | `float health, bool enabled` | failure state of | play | — | — |
| `AnomalyOdds` | `float condition` | The chance of an anomaly on one use: 0 in the top tenth, rising as the condition falls | play | — | — |
| `AnomalySeverityFor` | `float condition, float stress` | The severity of an anomaly at this condition under this load (0..1 stress) | play | — | — |
| `RollAnomaly` | `float condition, float stress, uint32_t roll` | roll anomaly | nothing | — | — |
| `HasTrait` | `const CrewMember &c, uint8_t trait` | has trait | play | — | — |
| `DeriveCharacter` | `CrewMember &c, uint32_t &rng` | The derivation: given the record (its identity, department, rank and species) and a seed stream, fills the four kinds and the drives | play | — | — |
| `DeriveSpecies` | `uint8_t dept, uint32_t &rng` | Species for a record: named crew carry theirs; a generated member draws one from the seed | play | — | — |
| `DeriveAppearance` | `CrewMember &c, uint32_t &rng` | The appearance derivation: the face is part of the person, so it comes from the same seed as the traits and is never stored | play | — | docs/character-attributes.md |
| `AppearanceHeadModel` | `const Appearance &a` | The head model ("dir/skin") a derived appearance resolves to, or null for a named record | play | — | docs/character-attributes.md |
| `AppearanceBuildName` | `uint8_t build` | The body build name: crewthin or crewfemale | play | — | — |
| `AppearanceColourName` | `uint8_t colour` | The department-colour name: default, red, gold or blue | play | — | — |
| `NamedHeadModel` | `const std::string &type` | The model a named record keeps ("type/default"), or null for a generated one: one list for the named boundary | play | — | docs/character-attributes.md |
| `IsCanonFace` | `const char *directory` | The canon boundary in one place: is this head directory a show command-crew face the derivation may not produce? | play | — | docs/asset-doctrine.md |
| `CanonFaceInPool` | `const char *const *dirs, int n` | The first directory that resolves to a canon face, or null: the whole-pool check, plantable | play | — | — |
| `AppearancePoolCount` | — | How many shipped head parts the derivation may draw from | play | — | — |
| `AppearancePoolDirectoryAt` | `int i` | The head directory of one pool entry | play | — | — |
| `AppearancePoolHeadAt` | `int i` | The head model ("dir/skin") of one pool entry | play | — | — |
| `AppearancePoolSpeciesAt` | `int i` | The species one pool entry belongs to | play | — | — |
| `SpeciesHeadCount` | `uint8_t species` | How many face parts a species may draw from | play | — | docs/character-attributes.md |
| `SpeciesHeadAt` | `uint8_t species, int i` | The face ("dir/skin") one species pool entry resolves to | play | — | docs/character-attributes.md |
| `FindCondition` | `const CrewMember &c, uint8_t id` | find condition | play | — | — |
| `Morale` | `const CrewMember &c` | Morale, the read (docs/morale.md) | play | — | docs/morale.md |
| `MoraleBandName` | `float morale` | morale band name | play | — | — |
| `MoraleReason` | `const CrewMember &c` | morale reason | play | — | — |
| `EffectiveSkill` | `const CrewMember &c, uint8_t skill` | Effective skill: base skill + aptitude traits, reduced by conditions, deficits and haste (docs/character-attributes.md, "the derivation") | console | — | docs/character-attributes.md |
| `MannerLine` | `const CrewMember &c` | The manner (G14's second half): what a person says at the morale they carry | nothing | — | — |
| `WorkFactorKindName` | `uint8_t k` | work factor kind name | play | — | — |
| `WorkContextName` | `uint8_t c` | work context name | console | — | — |
| `WorkContextOf` | `SystemId id` | work context of | play | post | — |
| `WorkFactors` | `const CrewMember &c, uint8_t skill, uint8_t context, float stress, WorkFactor *out, int maxOut` | The named factors a person brings to a task of `skill` in `context` under `stress`, in a fixed order | play | — | — |
| `WorkOdds` | `float condition, const WorkFactor *f, int n` | The odds of an anomaly for one use: the system's condition sets the base, the operator's factors move it | play | — | docs/failure-is-content.md |
| `RollWork` | `float condition, float stress, const WorkFactor *f, int n, uint32_t roll` | One use, as RollAnomaly, with the operator's factors reaching the odds | play | — | — |
| `WorkFactorLine` | `const WorkFactor *f, int n` | The factors as one line, for the log and the console: "condition afraid (slight) +0.15; drive afraid of decompression (sealing the hull) +0.25" | play | — | — |
| `WorkReading` | `const Ship &s, int crew, SystemId id` | The whole picture for one person at one system -- who, what they can do, and every factor that moved the odds -- for the personnel console | console | post+person | — |
| `BuildBrief` | `const Ship &s, uint8_t kind` | build brief | play | — | — |
| `MeetingDue` | `const Ship &s, uint8_t kind` | Is a meeting of this kind due right now? The watch change and the daily departmental meeting are the normal-case triggers; the rest are thresholds, latched so one... | play | — | — |
| `EmitDueMeetings` | `Ship &s` | THE EMIT SITE | play | — | docs/programme-meetings-and-voice.md |
| `PendingMeetings` | `const Ship &s` | pending meetings | play | — | — |
| `TakeBrief` | `Ship &s` | Drop the oldest queued brief: the worker has taken it | play | — | — |
| `ApplyMeetingOutcome` | `Ship &s, const MeetingBrief &brief, int outcome, int decidedBy, bool automatic` | The decision the room reached: which enumerated outcome, who decided it, and whether it was the ship's own answer (automatic mode) or a person's | play | — | — |
| `LogScopeForCrew` | `const Ship &s, int crew` | The log scope a person reads: command reads all, everyone else their own department's scope. The display form of the per-participant brief | play | — | docs/the-record-and-the-log.md |
| `ClassifyNovelInput` | `const Ship &s, const MeetingBrief &brief, const std::string &text` | The novelty seam: typed input matched against the option intent descriptors. With the embedding classifier absent every input is reported novel and nothing is applied, so typed text is never a branch by accident; with a verdict loaded (M4) a genuine match plays its branch | play | — | docs/staff-meetings.md |
| `NoveltyKey` | `const MeetingBrief &brief, const std::string &text` | The content-addressed key a typed input and a meeting's options hash to. The classifier's verdicts are keyed by it, so the same input against the same options always resolves the same way (M4) | play | — | docs/staff-meetings.md |
| `ClearNoveltyIndex` | `` | Empty the classifier's verdicts. The host calls it as it (re)loads the worker's output | play | — | docs/staff-meetings.md |
| `AddNoveltyVerdict` | `uint32_t key, bool matched, int outcome, const std::string &note` | One verdict the worker wrote: this input matches this enumerated outcome, or is novel (M4) | play | — | docs/staff-meetings.md |
| `NoveltyVerdictCount` | `` | How many verdicts are loaded, for the console and the check | console | — | docs/staff-meetings.md |
| `DamageSystem` | `Ship &s, SystemId id, float amount` | damage system | console | post | — |
| `DamageSource` | `Ship &s, SourceId id, float amount` | damage source | console | — | — |
| `BreachDeck` | `Ship &s, int deck, float amount` | breach deck | console | — | — |
| `IgniteDeck` | `Ship &s, int deck, float amount` | A fire can start on a deck (combat, a damaged system, or deliberately) | console | — | — |
| `Board` | `Ship &s, int deck, int boarders` | board | console | — | — |
| `BoardAs` | `Ship &s, int deck, int boarders, BoarderKind kind, int objective` | board as | console | — | — |
| `CounterHack` | `Ship &s, SystemId id, float strength` | A counter-hack at a console: `strength` 0..1 is how well the operator did (BreachScore) | play | post | — |
| `Hijacked` | `const Ship &s, SystemId id` | hijacked | play | post | — |
| `BoardBorg` | `Ship &s, int deck, int drones` | board borg | console | — | — |
| `DeckAssimilated` | `const Ship &s, int deck` | deck assimilated | play | — | — |
| `Jump` | `Ship &s, int toBeacon` | jump | play | — | — |
| `InCombat` | `const Ship &s` | in combat | play | — | — |
| `Remodulate` | `Ship &s` | The counter-play kit (docs/borg-incursion.md): the phaser adapter's rotating modulation, and a vinculum raid | play | — | docs/borg-incursion.md |
| `RaidVinculum` | `Ship &s` | raid vinculum | play | — | — |
| `ShutDownCore` | `Ship &s` | shut down core | play | — | — |
| `RestartCore` | `Ship &s` | restart core | play | — | — |
| `EjectCore` | `Ship &s` | eject core | play | — | — |
| `RestoreCoolant` | `Ship &s` | restore coolant | play | — | — |
| `CoreBreached` | `const Ship &s` | core breached | play | — | — |
| `JumpStartFromHolodeck` | `Ship &s` | The holodeck matrix is a trap, not a solution (docs/ship-systems.md; VOY "Parallax") | console | — | docs/ship-systems.md |
| `Pursued` | `const Ship &s` | Pursuit (S9): a raider whose engines survived follows the ship, and repairs suffer while it does | console | — | — |
| `PursuitJumps` | `const Ship &s` | pursuit jumps | nothing | — | — |
| `AtEnd` | `const Ship &s` | More sectors, and an end to reach (S9): reaching the last beacon marks the end; crossing it opens the next sector, and the third crossed is home | nothing | — | — |
| `Won` | `const Ship &s` | won | nothing | — | — |
| `SectorNumber` | `const Ship &s` | sector number | nothing | — | — |
| `AdvanceSector` | `Ship &s` | advance sector | console | — | — |
| `Trauma` | `const CrewMember &who` | The unprocessed weight a character carries: their negative marks, still salient | play | — | — |
| `BuildComputerBrief` | `const Ship &s` | The computer is addressed, not convened: the same brief shape as a meeting's, built from the computer skeleton, whose pills are the ship's API | play | — | docs/staff-meetings.md |
| `AddressComputer` | `const Ship &s, const std::string &input` | What the computer says to an input: membership in the ship's API (FindConsoleVerb); a recognised command answered from the authored line, an input outside the set refused in character ("that function is not available") with nothing invented and no state written | play | — | docs/staff-meetings.md |

### 3. Outcomes content can ask for  
*54 hooks.*

| hook | takes | does (the header's line, or the name) | reach | law | owner |
|---|---|---|---|---|---|
| `AddCondition` | `CrewMember &c, uint8_t id, const std::string &source, int8_t valence, uint8_t magnitude, uint8_t clears, float now, uint8_t visible` | add condition | play | — | — |
| `ClearCondition` | `CrewMember &c, uint8_t id` | clear condition | play | — | — |
| `Sicken` | `Ship &s, int crew, uint8_t id, const std::string &source, int8_t valence, uint8_t magnitude, uint8_t clears, uint8_t visible` | sicken | play | person | — |
| `Cure` | `Ship &s, int crew, uint8_t id` | cure | play | person | — |
| `WriteOff` | `Ship &s, bool system, int target, uint8_t kind` | Write it off: the one moment the ship gives something up for good | play | — | docs/story-and-semantics.md |
| `OrderBuild` | `Ship &s, int parts` | Command orders a net-new thing built: the crew fabricate spare parts over crew-hours | play | — | — |
| `SetJobPriority` | `Ship &s, int index, int priority` | Command sets a job's place in the order (docs/crew-work.md: "priority is where rank lives") | play | — | docs/crew-work.md |
| `OrderAdvance` | `Ship &s, int deck` | order advance | play | — | — |
| `SetAlert` | `Ship &s, Alert a` | set alert | play | — | — |
| `SetEnabled` | `Ship &s, SystemId id, bool on` | set enabled | play | post | — |
| `SetPriority` | `Ship &s, SystemId id, int priority` | set priority | play | post | — |
| `SetAllocation` | `Ship &s, SystemId id, int percent` | The player sets a system's allocation, as a percentage of its demand (0..100) | play | post | — |
| `SetAllocationBy` | `Ship &s, SystemId id, int percent, int officer` | The same, but set by a named officer acting under a standing delegation over that system's band | console | post+person | — |
| `SetPowerAuto` | `Ship &s, bool on` | set power auto | play | — | — |
| `AcceptRecommendation` | `Ship &s` | accept recommendation | play | — | — |
| `RefuseRecommendation` | `Ship &s` | refuse recommendation | play | — | — |
| `GrantBand` | `Ship &s, int grantor, int grantee, BudgetBand band` | A band delegation (Task C): grantable, held by a name, and revocable immediately | play | person | — |
| `RevokeBand` | `Ship &s, int grantee, BudgetBand band` | revoke band | play | person | — |
| `SetSourceOnline` | `Ship &s, SourceId id, bool on` | set source online | play | — | — |
| `SetForceField` | `Ship &s, int deck, bool on` | set force field | play | — | — |
| `SetForceFieldLevel` | `Ship &s, int deck, int level` | set force field level | play | — | — |
| `SetSurgicalField` | `Ship &s, bool on` | The surgical bay's force field (the triage gap): raised, it holds the gravest casualty steady even with no medical supplies -- the one case that would otherwise be... | play | — | — |
| `RecoverCaptive` | `Ship &s, int crew` | Recover someone the Borg have begun to assimilate (wounds in (0, RECOVERY_LIMIT)): sickbay and supplies against the nanoprobes | play | person | — |
| `Hearing` | `Ship &s, int crew, bool guilty` | hearing | console | person | — |
| `SetMobileEmitter` | `Ship &s, bool on` | set mobile emitter | console | — | — |
| `EndHolodeckProgram` | `Ship &s, int crew` | end holodeck program | console | person | — |
| `SetAirponics` | `Ship &s, bool on` | set airponics | console | — | — |
| `SetTarget` | `Ship &s, EnemySubsystem t` | An opponent's systems (S9): Tactical picks what to aim at, and its weapons, engines and shield generator are things to break in their own right | play | — | — |
| `SetPhaserYield` | `Ship &s, int y` | The phaser bank's setting (Tactical's standing decision, and the one that has an effect with no contact: it changes what the bank asks of the power budget and what it... | play | — | — |
| `Delegate` | `Ship &s, int grantor, int grantee, Station st` | delegate | console | post+person | — |
| `RevokeDelegation` | `Ship &s, int revoker, int grantee, Station st` | revoke delegation | console | post+person | — |
| `RevokeCredential` | `Ship &s, int revoker, int holder, Station st` | The crew can revoke a credential too: a qualification once earned is not held forever | console | post+person | — |
| `BeginOverride` | `Ship &s, int requester, Station st` | begin override | play | post | — |
| `ConfirmOverride` | `Ship &s, int second, Station st` | confirm override | play | post | — |
| `LockOut` | `Ship &s, int officer, Station st, int locked` | lock out | console | post+person | — |
| `ClearLockout` | `Ship &s, int officer, Station st, int locked` | clear lockout | console | post+person | — |
| `Train` | `Ship &s, int crew, Station st` | Training and credentials: a crew member earns a cross-qualification by training, and a credential lets them operate a station their department does not own | console | post+person | — |
| `Brig` | `Ship &s, int crew, bool on` | Discipline and justice: the brig, a hearing, and release | console | person | — |
| `HoldFuneral` | `Ship &s` | Grief: a funeral, when there is time to hold one, lifts the crew who have been lost and are missed | console | — | — |
| `KillCrew` | `Ship &s, int crew, const std::string &cause` | A death, by name and cause: the record is closed, command is notified, the quarters are sealed and whoever is on the deck remembers it | play | person | — |
| `Promote` | `Ship &s, int crew` | The player's career: a promotion within the complement, given the trust and the rank | play | person | — |
| `Remember` | `Ship &s, int crew, uint16_t event, int person, MemorySource source, float valence` | remember | play | person | — |
| `Brief` | `Ship &s, uint16_t event, float valence` | brief | console | — | — |
| `MakePromise` | `Ship &s, int officer, int crew, PromiseKind kind, const std::string &what, double deadline = -1.0` | A promise: an officer commits in front of a crew member; the mark is written naming the promiser, and the claim is held | console | person | — |
| `ResolvePromise` | `Ship &s, int index, bool kept` | resolve promise | play | — | — |
| `SignReport` | `Ship &s, int signer, uint8_t audience` | sign report | play | person | — |
| `PurgeLogs` | `Ship &s` | purge logs | play | — | — |
| `RunHolodeck` | `Ship &s, HolodeckUse use, int crew` | run holodeck | console | person | — |
| `ImproveQuarters` | `Ship &s` | Living conditions: improve the crew's quarters, at a cost in material | console | — | — |
| `OrderRepairFirst` | `Ship &s, int system` | order repair first | play | — | — |
| `OrderSecurityTo` | `Ship &s, int deck` | order security to | play | — | — |
| `OrderEvacuate` | `Ship &s, int deck` | order evacuate | play | — | — |
| `OrderTriage` | `Ship &s, int policy` | order triage | play | — | — |
| `SetRole` | `Ship &s, PlayerRole role` | Sets the role; ROLE_MUNRO makes the player Alexander Munro | console | — | — |

### 4. Readers content can consult  
*174 hooks.*

| hook | takes | does (the header's line, or the name) | reach | law | owner |
|---|---|---|---|---|---|
| `Spec` | `SystemId id` | spec | play | post | — |
| `StationOf` | `SystemId id` | station of | play | post | — |
| `StationName` | `Station s` | station name | play | post | — |
| `OperatedFrom` | `SystemId id, Station s` | operated from | console | post | — |
| `StationReads` | `Station s, SystemId id` | Reading and operating are different privileges (owner ruling, 2026-10-07) | play | post | — |
| `AnomalyName` | `uint8_t severity` | anomaly name | play | — | — |
| `AllocationByName` | `uint8_t by` | allocation by name | console | — | — |
| `SystemCondition` | `const System &sys` | The capability a system still has, 0..1: its health, capped by what power is reaching it | play | — | docs/failure-is-content.md |
| `ControllerName` | `uint8_t c` | controller name | console | — | — |
| `PhaserYieldName` | `uint8_t y` | phaser yield name | play | — | — |
| `SkillName` | `uint8_t s` | skill name | play | — | — |
| `DepartmentSkill` | `Department d` | The skill a department's work leads with (engineering leads engineering, medical medical, and so on): the one place the mapping lives, used by the derivation and by... | play | — | — |
| `TraitName` | `uint8_t t` | trait name | nothing | — | — |
| `DesireName` | `uint8_t d` | desire name | nothing | — | — |
| `NeedName` | `uint8_t n` | need name | nothing | — | — |
| `FearName` | `uint8_t f` | fear name | nothing | — | — |
| `ConditionName` | `uint8_t id` | condition name | play | — | — |
| `ConditionValenceName` | `uint8_t v` | condition valence name | nothing | — | — |
| `ConditionClearName` | `uint8_t c` | condition clear name | nothing | — | — |
| `ConditionMagnitudeName` | `uint8_t m` | condition magnitude name | play | — | — |
| `SpeciesName` | `uint8_t sp` | species name | nothing | — | — |
| `SpeciesOf` | `uint8_t sp` | species of | play | — | — |
| `SpeciesCapability` | `uint8_t sp, int i` | species capability | nothing | — | — |
| `SpeciesNeed` | `uint8_t sp, int i` | species need | nothing | — | — |
| `SpeciesSusceptibility` | `uint8_t sp, int i` | species susceptibility | nothing | — | — |
| `PromiseKindName` | `uint8_t k` | promise kind name | nothing | — | — |
| `ReportAudienceName` | `uint8_t a` | report audience name | play | — | — |
| `RisingOutcomeName` | `uint8_t o` | rising outcome name | console | — | — |
| `RisingDriveAtStake` | `const CrewMember &c, uint8_t skill, uint8_t context, float stress` | The drive a context realises in a person: the WORK_DRIVE factor the work-bites pass already computes (a fear realised by the task in front of them), or empty when... | console | — | — |
| `RisingOdds` | `const RisingOffer &o, const CrewMember &c, const WorkFactor *f, int n` | The odds of the attempt: the person's own factors, moved by how far it is beyond their effective skill | console | — | docs/failure-is-content.md |
| `ScheduledActivity` | `int watch, int secondOfDay` | Where a crew member is and what they are doing at a time of day, by their watch alone | play | — | — |
| `PostsNeeded` | `` | The posts the ship's systems need to be fully manned, summed from the specs | nothing | — | — |
| `CoverWithCrew` | `int crew` | cover with crew | nothing | person | — |
| `BeaconKindName` | `BeaconKind k` | beacon kind name | play | — | — |
| `LogVisibilityName` | `uint8_t v` | log visibility name | nothing | — | — |
| `PersonalLog` | `const Ship &s, int owner` | The private read: this person's entries, newest last, and nobody else's -- not a scope, not command, not a post's clearance | play | person | — |
| `PersonalVisibleTo` | `const PersonalLogEntry &e, int reader` | The visibility rule, in one place: a personal entry is visible only to its owner | play | — | — |
| `ReadOfficialLog` | `const Ship &s, int count, const std::string &scope` | The official read, factored out so the search itself is testable: the newest `count` entries, optionally filtered to one `scope` (empty = all of them) | play | — | — |
| `MeetingKindName` | `uint8_t kind` | meeting kind name | play | — | — |
| `DeliveryName` | `uint8_t d` | delivery name | console | — | — |
| `DeliveryExaggeration` | `uint8_t d` | What the delivery asks the synthesizer for, in the review's own knob | console | — | — |
| `DeliveryKnown` | `uint8_t d` | delivery known | console | — | — |
| `MeetingEffectName` | `uint8_t e` | meeting effect name | console | — | — |
| `AuthoredSkeleton` | `uint8_t kind` | The authored skeleton for a kind: the outcomes, their costs, and minimal dialogue for each | play | — | — |
| `ResolveSpeaker` | `const Ship &s, const MeetingBrief &brief, int speaker` | Who says a line: a roster index if the skeleton named one, otherwise the participant (or officer) of that role | console | — | — |
| `LineToSynthesis` | `const MeetingLine &line, SynthesisRequest &out` | line to synthesis | console | — | — |
| `VoiceTrackName` | `uint8_t track` | voice track name | nothing | — | — |
| `VoiceProducerName` | `uint8_t producer` | voice producer name | console | — | — |
| `TrackOwner` | `uint8_t track` | track owner | console | — | — |
| `OwnsTrack` | `uint8_t producer, uint8_t track` | owns track | console | — | — |
| `TrackBusy` | `const VoiceMixer &m, uint8_t track` | track busy | console | — | — |
| `ActiveReply` | `const VoiceMixer &m, uint8_t track` | active reply | nothing | — | — |
| `CueName` | `uint8_t cue` | cue name | console | — | — |
| `CueClip` | `uint8_t cue` | cue clip | console | — | — |
| `CuePurpose` | `uint8_t cue` | cue purpose | console | — | — |
| `CueIsLexical` | `uint8_t cue` | cue is lexical | nothing | — | — |
| `RenderKey` | `const std::string &voice, const std::string &text, uint8_t delivery` | The cache key | console | — | — |
| `VoiceCachePath` | `const VoiceRender &vr, const std::string &key` | Where a key's clip lives: <dir>/<key>.wav (the synthesizer's output; never the repository) | console | — | — |
| `VoiceCached` | `const VoiceRender &vr, const std::string &key` | voice cached | console | — | — |
| `VoiceQueued` | `const VoiceRender &vr, const std::string &key` | voice queued | console | — | — |
| `VoicePoolCount` | — | the non-canon pool's size | console | — | docs/the-entry-point.md |
| `VoicePoolName` | `int i` | a pool voice by index, or "" out of range | console | — | docs/the-entry-point.md |
| `VoiceIsCanonType` | `const std::string &type` | a show command-crew record: its voice is its own | console | — | docs/the-entry-point.md |
| `CastVoice` | `const Ship &s, int crew` | The reference identity of a person's voice, or "" when there is no voice | console | person | docs/the-entry-point.md |
| `CastIsCanon` | `const Ship &s, int crew` | True when the voice is the character's own retail voice rather than a pool casting | console | person | docs/the-entry-point.md |
| `CachedDuration` | `const VoiceRender &vr, const std::string &key` | The rendered duration of a keyed clip, in seconds; 0 when no clip has been rendered for it | console | — | — |
| `MeetingLineKey` | `const Ship &s, const MeetingBrief &brief, const MeetingLine &line` | The key a meeting line renders under, resolved through the cast map; "" when the line has no voice | console | post | docs/staff-meetings.md |
| `MeetingLineSeconds` | `const VoiceRender &vr, const Ship &s, const MeetingBrief &brief, const MeetingLine &line` | The seconds a line runs: its rendered clip's duration when there is one, else a text-length fallback | play | post | docs/staff-meetings.md |
| `LossKindName` | `uint8_t k` | loss kind name | play | — | — |
| `WriteOffs` | `const Ship &s` | write offs | play | — | — |
| `JobKindName` | `uint8_t k` | job kind name | play | — | — |
| `Jobs` | `const Ship &s` | The queue as it stands after this tick (rebuilt from the ship's state, plus the player's build jobs, order preserved for the jobs that persist) | play | — | — |
| `BandName` | `uint8_t band` | band name | play | — | — |
| `BandOf` | `SystemId id` | band of | play | post | — |
| `Day` | `` | day | play | — | — |
| `SecondOfDay` | `` | second of day | play | — | — |
| `Watch` | `` | watch | play | — | — |
| `PowerAvailable` | `` | power available | play | — | — |
| `PowerAllocated` | `` | power allocated | play | — | — |
| `PowerCapacityFresh` | `` | The power budget the engineering console shows twice (owner ruling, 2026-10-07): what the plant can deliver fresh, and what it can deliver now | play | — | — |
| `PowerCapacityNow` | `` | power capacity now | play | — | — |
| `CrewFit` | `` | crew fit | play | — | — |
| `NewShip` | `const Config &cfg = Config()` | A ship in the state Voyager is in with nothing wrong: every system intact, stores full, the roster generated from the seed | nothing | — | — |
| `StressNow` | `const Ship &s` | The load the ship is under right now: 1.0 at battle stations, less at yellow and green | play | — | — |
| `AllocationPercent` | `const Ship &s, SystemId id` | allocation percent | play | post | — |
| `AllocationSource` | `const Ship &s, SystemId id` | allocation source | play | post | — |
| `AllocationProvenance` | `const Ship &s, SystemId id` | The provenance as the console says it: "the player", "an officer (Name)", "automatic mode", "unset" | console | post | — |
| `PowerCommitted` | `const Ship &s` | Committed against available (docs/power-assignment.md) | play | — | docs/power-assignment.md |
| `PowerShortfall` | `const Ship &s` | power shortfall | play | — | — |
| `PowerAuto` | `const Ship &s` | power auto | play | — | — |
| `BandHolder` | `const Ship &s, BudgetBand band` | band holder | play | — | — |
| `BandGrantCount` | `const Ship &s` | band grant count | nothing | — | — |
| `MinutesOfAir` | `const Ship &s, int deck` | Endurance clocks (the air-and-endurance gap): the numbers a compartment or the whole ship is running on | play | — | — |
| `MinutesToDark` | `const Ship &s` | minutes to dark | play | — | — |
| `EnduranceOf` | `const Ship &s, SourceId id` | The endurance of one source at its current draw, in minutes: the batteries' charge, or a reactor's fuel | play | — | — |
| `GravityScale` | `const Ship &s, int deck` | Per-person gravity (the environment in the world) | play | — | — |
| `ScaleGravity` | `int worldGravity, float scale` | scale gravity | play | — | — |
| `SurgicalField` | `const Ship &s` | surgical field | play | — | — |
| `BoarderKindName` | `BoarderKind k` | boarder kind name | play | — | — |
| `Intruders` | `const Ship &s` | intruders | play | — | — |
| `BreachScore` | `const Breach &b, const std::vector<int> &picks` | `picks` are cell indices (row * size + column) | play | — | — |
| `ShuttleClassName` | `ShuttleClass c` | Shuttles (docs/shuttles.md): supported, not pilotable | play | — | docs/shuttles.md |
| `ShuttleLocationName` | `ShuttleLocation l` | shuttle location name | console | — | — |
| `ShuttlesInBay` | `const Ship &s` | shuttles in bay | console | — | — |
| `ShuttlesAway` | `const Ship &s` | shuttles away | console | — | — |
| `WarpPossible` | `const Ship &s` | warp possible | play | — | — |
| `DilithiumRange` | `const Ship &s` | dilithium range | play | — | — |
| `DilithiumWayName` | `DilithiumWay way` | dilithium way name | play | — | — |
| `Coreless` | `const Ship &s` | The coreless ship (docs/budget-squaring.md, Part 4a) | play | — | docs/budget-squaring.md |
| `PhenomenonResponseName` | `int response` | phenomenon response name | console | — | — |
| `Held` | `const Ship &s` | held | nothing | — | — |
| `Refugees` | `const Ship &s` | refugees | console | — | — |
| `Resentment` | `const Ship &s` | The Maquis split as an arc: resentment between the two, and what reconciles them | console | — | — |
| `BorgAwareness` | `const Ship &s` | Borg strategic awareness: how much the Collective knows of the ship, and what a crew member's memory of command does to their work | console | — | — |
| `Loyalty` | `const Ship &s, int crew` | loyalty | play | person | — |
| `EMHActive` | `const Ship &s` | emhactive | play | — | — |
| `TravelSafe` | `const Ship &s` | travel safe | nothing | — | — |
| `PylonsIntact` | `const Ship &s` | pylons intact | play | — | — |
| `MobileEmitter` | `const Ship &s` | mobile emitter | console | — | — |
| `Airponics` | `const Ship &s` | airponics | console | — | — |
| `Target` | `const Ship &s` | target | console | — | — |
| `EnemySubsystemName` | `EnemySubsystem t` | enemy subsystem name | play | — | — |
| `EnemyKindName` | `EnemyKind k` | enemy kind name | play | — | — |
| `PhaserYieldOf` | `const Ship &s` | phaser yield of | play | — | — |
| `EffectiveDemand` | `const Ship &s, SystemId id` | The demand a system asks of the power budget this tick, before distribution | play | post | — |
| `AwayTeam` | `const Ship &s` | away team | play | — | — |
| `TransporterConditionLine` | `const Ship &s` | The instrument: an honest reading of the transporter's condition, stated before the act | play | — | docs/the-record-and-the-log.md |
| `PlotCourse` | `const Ship &s, int toBeacon` | The Conn's course: the shortest route from here to a beacon (BFS over the links; the first entry is where the ship is now), plotting it, and reading it back | play | — | — |
| `Course` | `const Ship &s` | course | nothing | — | — |
| `Patients` | `const Ship &s` | Sickbay's ward: the casualties, in the order the triage standing order treats them -- worst first by default, or rank first | play | — | — |
| `ClockRate` | `const Config &cfg` | Ship seconds that pass per second played, for the configured clock | play | — | — |
| `SavesAllowed` | `const Config &cfg` | May the game be saved and loaded at will? Not in ironman: the ship is saved for you, and only forward | nothing | — | — |
| `MaySuspend` | `const Config &cfg` | may suspend | console | — | docs/ship-model.md |
| `LeftStanding` | `const Ship &s` | left standing | console | — | — |
| `MayOperate` | `const CrewMember &who, Station st` | may operate | console | post | — |
| `MayCallAlert` | `const CrewMember &who, Station st` | may call alert | console | post | — |
| `MayCommand` | `const CrewMember &who` | may command | play | — | — |
| `DelegatedTo` | `const Ship &s, int crew, Station st` | delegated to | console | post+person | — |
| `Delegations` | `const Ship &s` | delegations | play | — | — |
| `OverrideActive` | `const Ship &s, Station st` | override active | console | post | — |
| `OverrideState` | `const Ship &s` | override state | play | — | — |
| `LockedOut` | `const Ship &s, int crew, Station st` | locked out | console | post+person | — |
| `Lockouts` | `const Ship &s` | lockouts | play | — | — |
| `LockoutNotice` | `const Ship &s, int crew, Station st` | The lock-out as it reads on the console that has stopped answering: empty unless `crew` is locked out of `st`, otherwise it names both hands | console | post+person | — |
| `MayCallUp` | `const Ship &s, int crew, Station console, SystemId id` | Command may travel to a console away from the system; the crew's physical jobs may not | console | post+person | — |
| `AccessRefusal` | `const Ship &s, Station st` | The named refusal: the reason a control is locked, naming who can open it | console | post | — |
| `OperatedFromRefusal` | `SystemId id` | operated from refusal | console | post | — |
| `Qualified` | `const CrewMember &who, Station st` | qualified | console | post | — |
| `Brigged` | `const Ship &s, int crew` | brigged | console | person | — |
| `WallOfNames` | `const Ship &s` | The wall of names (docs/morale.md): the crew the ship has buried, in roster order | play | — | docs/morale.md |
| `SealedQuarters` | `const Ship &s` | sealed quarters | play | — | — |
| `PlayerIncapacitated` | `const Ship &s` | The player's body: the state the player is in, as a crew record | play | — | — |
| `PlayerDead` | `const Ship &s` | player dead | console | — | — |
| `PlayerDeck` | `const Ship &s` | player deck | nothing | — | — |
| `PlayerBodyHealth` | `const Ship &s, int bodyHealth` | player body health | play | — | — |
| `Recall` | `const CrewMember &who, uint16_t event` | recall | console | — | — |
| `RecallSource` | `const CrewMember &who, uint16_t event` | recall source | console | — | — |
| `MemoryCount` | `const CrewMember &who` | memory count | console | — | — |
| `Bond` | `const Ship &s, int a, int b` | bond | play | — | — |
| `Promises` | `const Ship &s` | promises | console | — | — |
| `NavigationCounter` | `const Ship &s` | navigation counter | play | — | — |
| `NavigationForecasts` | `const Ship &s` | navigation forecasts | play | — | — |
| `OpenReport` | `const Ship &s` | open report | play | — | — |
| `ReportDiff` | `const MonthReport &r` | report diff | play | — | — |
| `Reports` | `const Ship &s` | reports | play | — | — |
| `LogsPurged` | `const Ship &s` | logs purged | nothing | — | — |
| `CommandingOfficer` | `const Ship &s` | The name that signs command's acts: the player's character if there is one, otherwise the captain or first officer, otherwise "command" | play | — | — |
| `DepartmentHead` | `const Ship &s, Department dept` | The senior fit officer of a department (the chief engineer for DEPT_ENGINEERING), preferring the one on duty, or -1 if the department has nobody fit | play | — | — |
| `PlayerMayOperate` | `const Ship &s, Station st` | The same questions for the player, whose role may widen or fix the answer | console | post | — |
| `PlayerMayCommand` | `const Ship &s` | player may command | play | — | — |
| `Pack` | `const Ship &s` | pack | play | — | — |
| `CrewOnDeck` | `const Ship &s, int deck` | Who is on a deck right now: indices into Ship::crew, in roster order | play | — | — |
| `Describe` | `const Ship &s` | A one-screen status report, for the console command and for tests' failure messages | nothing | — | — |
| `CaptainLog` | `const Ship &s` | The captain's log as an authored, summarised artifact, distinct from the raw feed: the situation in the captain's words, generated from the ship's state | console | — | — |
| `ConsoleVerbCount` | `` | the size of the ship's API, the enumerated command surface (M6) | play | — | docs/staff-meetings.md |
| `ConsoleVerbAt` | `int i` | a command's verb, arguments and owning station, or empty out of range | play | — | docs/staff-meetings.md |
| `FindConsoleVerb` | `const char *word` | The enumerated verb a word names, whole-token and case-insensitive; -1 when it is not in the set. The computer's membership test, and nothing else's | play | — | docs/staff-meetings.md |
| `ComputerVoice` | `` | The computer's own voice reference ("computer"), assigned where the cast lives rather than by a pool draw | play | — | docs/staff-meetings.md |
| `ComputerRefusal` | `` | The canonical refusal, in one place: "that function is not available" | play | — | docs/staff-meetings.md |
| `ComputerPillCount` | `` | the computer's pills | play | — | docs/staff-meetings.md |
| `ComputerPillVerb` | `int i` | the API verb a computer pill submits | play | — | docs/staff-meetings.md |
| `ComputerPillLabel` | `int i` | a computer pill's short description | play | — | docs/staff-meetings.md |
| `ComputerPillCost` | `int i` | a computer pill's cost | play | — | docs/staff-meetings.md |

## The counts (Task B)

**By group.**

| group | hooks |
|---|---|
| entry points | 74 |
| situations the simulation generates | 63 |
| outcomes | 54 |
| readers | 183 |
| the configurator | 18 |
| **total** | **392** |

**By reachability.**

| state | hooks |
|---|---|
| reached in play | 227 |
| reached only from the developer console | 130 |
| reached by nothing yet | 35 |
| — of those, reached only by the tests | 29 |
| — of those, no caller anywhere | 6 |
| unknown | 0 |

The six with no caller anywhere: `SpeciesCapability`, `SpeciesNeed`, `SpeciesSusceptibility`, `PromiseKindName`, `LogVisibilityName`, `BandGrantCount`.

**Re-derived 2026-10-08, the meeting overlay (M3).** `PendingMeetings`, `TakeBrief` and
`ApplyMeetingOutcome` moved from *console* to *play*: the meetings screen opens the queue and takes a
brief (`PendingMeetings`, `TakeBrief`) and a pill resolves to an outcome the simulation applies
(`ApplyMeetingOutcome`). Two hooks were added with them: `LogScopeForCrew` (the per-participant scope
the room shows) and `ClassifyNovelInput` (the novelty seam, every input reported novel with the
classifier absent). The counts above are the rows of this register, counted, not carried over:
73 + 57 + 54 + 166 + 18 = 368, and 211 + 122 + 35 = 368. Evidence: `docs/evidence/meeting-overlay.md`.

**Re-derived 2026-10-09, the async generator and the novelty classifier (M4).** Four hooks are added
for the classifier's verdict index -- `NoveltyKey`, `ClearNoveltyIndex`, `AddNoveltyVerdict` and
`NoveltyVerdictCount` -- three reached in play (the host loads the worker's verdicts and keys a typed
input) and one from the console (`ship meeting verdicts`). `ClassifyNovelInput` is no longer the
always-NOVEL seam: with a verdict loaded a genuine match plays its branch, and with none it is still
novel. The counts above are the rows of this register, counted: 73 + 61 + 54 + 166 + 18 = 372, and
214 + 123 + 35 = 372. Evidence: `docs/evidence/model-call.md`.

**Re-derived 2026-10-09, the casting map and voice out (M5).** Nine hooks are added. `SubmitNovelAnswer`
is an entry point reached in play: the meeting's typed-input path emits its pause cue immediately and
resolves inside the pause or defers. Eight readers carry the casting map and the pacing: `VoicePoolCount`,
`VoicePoolName`, `VoiceIsCanonType`, `CastVoice`, `CastIsCanon`, `CachedDuration`, `MeetingLineKey` and
`MeetingLineSeconds`; `MeetingLineSeconds` is reached in play (the room publishes each line's measured
duration for the pills) and the rest are console. The counts above are the rows of this register,
counted: 74 + 61 + 54 + 174 + 18 = 381, and 216 + 130 + 35 = 381. Evidence:
`docs/evidence/voice-and-casting.md`.

**Re-derived 2026-10-09, the ship's computer (M6a).** Eleven hooks are added. The ship's API becomes
readable: `ConsoleVerbCount`, `ConsoleVerbAt` and `FindConsoleVerb` (the enumerated command surface, and
the computer's membership test), `ComputerVoice` and `ComputerRefusal` (the computer's own voice and its
canonical refusal), and the computer's pills (`ComputerPillCount`, `ComputerPillVerb`,
`ComputerPillLabel`, `ComputerPillCost`). `BuildComputerBrief` and `AddressComputer` are the same
machinery a meeting uses, addressed to the computer. All eleven are reached in play: the computer is
addressed through the meeting overlay, whose keys send `ship meeting say` and `ship meeting choose`, and
the host routes them to `AddressComputer`. The counts above are the rows of this register, counted:
74 + 63 + 54 + 183 + 18 = 392, and 227 + 130 + 35 = 392. Evidence:
`docs/evidence/the-computer-api.md`.

**Re-derived 2026-10-10, voice in (M6b). No hook is added, and that is the finding.** Speech-to-text
feeds the free-text pill, and the pill already emits text, so the transcribed string takes the one path
the typed string takes (`SubmitRoomInput`, the host function behind both `ship meeting say` and the new
`ship dictate say`); no new public function in `ship_core.h` is needed, and adding one would have been
the second path the design forbids. The counts are unchanged: 392 (227 play / 130 console / 35 nothing).
The new console verb `dictate` is enumerated in `module/ship/console_api.def` and checked by
`scripts/api-check.sh`, not by this register (which is the `ship_core.h` surface). Evidence:
`docs/evidence/voice-in.md`.

### 5. The configurator — the player chooses the start state

*docs/the-entry-point.md, part three; docs/start-states.md.* The entry point is a configurator with
four dimensions: who you are, who died, who fills the gaps, and the career path. A start state is
data; the vacancies are derived by the same rule play uses. **The menu reaches the mechanism in
play** (the "Long Way Home" line writes the run's cvars, and `Ship_Init` applies the state); the
developer console reaches it too (`ship startstate ...`).

| hook | takes | does (the header's line, or the name) | reach | law | owner |
|---|---|---|---|---|---|
| `CareerPathName` | `uint8_t path` | career path name | play | — | docs/the-entry-point.md |
| `CareerPathBlurb` | `uint8_t path` | career path blurb | play | — | docs/the-entry-point.md |
| `SeatName` | `uint8_t seat` | seat name | play | post | docs/the-entry-point.md |
| `SeatType` | `uint8_t seat` | the named record that holds the seat at the canon default | play | post | docs/the-entry-point.md |
| `SeatDepartment` | `uint8_t seat` | the department the derivation promotes from | play | post | docs/the-entry-point.md |
| `SeatPost` | `uint8_t seat` | seat post | play | post | docs/the-entry-point.md |
| `SeatRank` | `uint8_t seat` | seat rank | play | post | docs/the-entry-point.md |
| `SeatForType` | `const std::string &type` | the seat whose named holder is this type, or -1 | play | post | docs/the-entry-point.md |
| `StartStateCount` | — | the shipped start states: a list as data | play | — | docs/the-entry-point.md |
| `StartStateAt` | `int i` | start state at | play | — | docs/the-entry-point.md |
| `ApplyStartState` | `Ship &s, const StartState &st` | Apply a start state to a fresh ship: close the casualties, seat the player, derive the vacancies, write the log seed | play | post | docs/the-entry-point.md |
| `FillVacancies` | `Ship &s, const std::string &reason` | The vacancy derivation, one rule for the configurator and for play alike | play | post | docs/start-states.md |
| `SeatHeldBy` | `const Ship &s, int crew` | the seat a crew member holds, or -1 | play | person | docs/the-entry-point.md |
| `SeatHolder` | `const Ship &s, uint8_t seat` | seat holder | play | post | docs/the-entry-point.md |
| `PlayerPositionLine` | `const Ship &s` | The player's own position, stated plainly: rank and post, what they may authorise, and who reports to them | play | person | docs/the-entry-point.md |
| `WriteLogSeed` | `Ship &s, const StartState &st` | The log seed: the first entry, describing what the player actually configured | play | — | docs/start-states.md |
| `LogSeedText` | `const Ship &s, const StartState &st` | log seed text | play | — | docs/start-states.md |
| `SeniorFitOfficer` | `const Ship &s` | The senior fit officer, in rank order: the roster rule that fills the chair | play | — | docs/start-states.md |

## The unreachable list, in full — the work nobody knew was waiting

**Not reached in play** — the developer console only, or nothing at all. This is the page a
scenario author and the owner should read first: every one of these is a mechanism the simulation
has and the game does not yet offer.

### Reached only from the developer console (130)

**1. Entry points** (37): `OfferRising`, `OfferRisingTo`, `AttemptRising`, `NextRisingRoll`, `WritePersonalLog`, `VoiceWrite`, `VoiceRetire`, `EmitCue`, `QueueRender`, `CacheRendered`, `PruneVoiceCache`, `PlanMeetingAudio`, `WarmVoice`, `Repair`, `RepairDeck`, `LoadAwayKit`, `LaunchShuttle`, `RecallShuttle`, `LoseShuttle`, `ShuttleBayHit`, `RebuildShuttle`, `TractorWreck`, `TractorHold`, `LaunchProbe`, `RespondPhenomenon`, `FabricateParts`, `FabricateRations`, `MineBelt`, `EVA`, `TakeSurvivors`, `ObservePreWarp`, `InterferePreWarp`, `ReconcileFactions`, `Survey`, `CatchUp`, `Suspend`, `Sleep`

**2. Situations the simulation generates on its own** (14): `EffectiveSkill`, `WorkContextName`, `WorkReading`, `DamageSystem`, `DamageSource`, `BreachDeck`, `IgniteDeck`, `Board`, `BoardAs`, `BoardBorg`, `JumpStartFromHolodeck`, `Pursued`, `AdvanceSector`, `NoveltyVerdictCount`

**3. Outcomes content can ask for** (18): `SetAllocationBy`, `Hearing`, `SetMobileEmitter`, `EndHolodeckProgram`, `SetAirponics`, `Delegate`, `RevokeDelegation`, `RevokeCredential`, `LockOut`, `ClearLockout`, `Train`, `Brig`, `HoldFuneral`, `Brief`, `MakePromise`, `RunHolodeck`, `ImproveQuarters`, `SetRole`

**4. Readers content can consult** (61): `OperatedFrom`, `AllocationByName`, `ControllerName`, `RisingOutcomeName`, `RisingDriveAtStake`, `RisingOdds`, `DeliveryName`, `DeliveryExaggeration`, `DeliveryKnown`, `MeetingEffectName`, `ResolveSpeaker`, `LineToSynthesis`, `VoiceProducerName`, `TrackOwner`, `OwnsTrack`, `TrackBusy`, `CueName`, `CueClip`, `CuePurpose`, `RenderKey`, `VoiceCachePath`, `VoiceCached`, `VoiceQueued`, `VoicePoolCount`, `VoicePoolName`, `VoiceIsCanonType`, `CastVoice`, `CastIsCanon`, `CachedDuration`, `MeetingLineKey`, `AllocationProvenance`, `ShuttleLocationName`, `ShuttlesInBay`, `ShuttlesAway`, `PhenomenonResponseName`, `Refugees`, `Resentment`, `BorgAwareness`, `MobileEmitter`, `Airponics`, `Target`, `MaySuspend`, `LeftStanding`, `MayOperate`, `MayCallAlert`, `DelegatedTo`, `OverrideActive`, `LockedOut`, `LockoutNotice`, `MayCallUp`, `AccessRefusal`, `OperatedFromRefusal`, `Qualified`, `Brigged`, `PlayerDead`, `Recall`, `RecallSource`, `MemoryCount`, `Promises`, `PlayerMayOperate`, `CaptainLog`

### Reached by nothing yet (35)

- `RollAnomaly` — reached only by the tests (situation)
- `TraitName` — reached only by the tests (reader)
- `DesireName` — reached only by the tests (reader)
- `NeedName` — reached only by the tests (reader)
- `FearName` — reached only by the tests (reader)
- `ConditionValenceName` — reached only by the tests (reader)
- `ConditionClearName` — reached only by the tests (reader)
- `SpeciesName` — reached only by the tests (reader)
- `SpeciesCapability` — no caller anywhere (reader)
- `SpeciesNeed` — no caller anywhere (reader)
- `SpeciesSusceptibility` — no caller anywhere (reader)
- `PromiseKindName` — no caller anywhere (reader)
- `MannerLine` — reached only by the tests (situation)
- `RetellRising` — reached only by the tests (entry)
- `PostsNeeded` — reached only by the tests (reader)
- `CoverWithCrew` — reached only by the tests (reader)
- `LogVisibilityName` — no caller anywhere (reader)
- `VoiceTrackName` — reached only by the tests (reader)
- `ActiveReply` — reached only by the tests (reader)
- `CueIsLexical` — reached only by the tests (reader)
- `NewShip` — reached only by the tests (reader)
- `BandGrantCount` — no caller anywhere (reader)
- `StrandShuttle` — reached only by the tests (entry)
- `Held` — reached only by the tests (reader)
- `TravelSafe` — reached only by the tests (reader)
- `PursuitJumps` — reached only by the tests (situation)
- `AtEnd` — reached only by the tests (situation)
- `Won` — reached only by the tests (situation)
- `SectorNumber` — reached only by the tests (situation)
- `Course` — reached only by the tests (reader)
- `SavesAllowed` — reached only by the tests (reader)
- `PlayerDeck` — reached only by the tests (reader)
- `AssumeCommand` — reached only by the tests (entry)
- `LogsPurged` — reached only by the tests (reader)
- `Describe` — reached only by the tests (reader)

## The post-not-name law (Task B's last acceptance)

`docs/the-entry-point.md`, part three: **content addresses a post; the simulation resolves the
post to a person.** The `law` column marks every hook the rule binds.

- **Takes a person** (`36`): the hook accepts a resolved crew index, so content must resolve a
  post to that person and must never write a name into a scenario. They are: `Sicken`, `Cure`, `WorkReading`, `OfferRisingTo`, `AttemptRising`, `RetellRising`, `CoverWithCrew`, `WritePersonalLog`, `PersonalLog`, `SetAllocationBy`, `GrantBand`, `RevokeBand`, `RecoverCaptive`, `Hearing`, `Loyalty`, `EndHolodeckProgram`, `Delegate`, `RevokeDelegation`, `DelegatedTo`, `RevokeCredential`, `LockOut`, `ClearLockout`, `LockedOut`, `LockoutNotice`, `MayCallUp`, `Train`, `Brig`, `Brigged`, `KillCrew`, `Promote`, `Remember`, `MakePromise`, `SignReport`, `RunHolodeck`, `CastVoice`, `CastIsCanon`.
- **Addressed by a post** (`46`), the compliant form: `Spec`, `StationOf`, `StationName`, `OperatedFrom`, `StationReads`, `WorkContextOf`, `OfferRising`, `BandOf`, `UseSystem`, `UseSystemBy`, `SetEnabled`, `SetPriority`, `SetAllocation`, `AllocationPercent`, `AllocationSource`, `AllocationProvenance`, `DamageSystem`, `Repair`, `CounterHack`, `Hijacked`, `EffectiveDemand`, `MayOperate`, `MayCallAlert`, `BeginOverride`, `ConfirmOverride`, `OverrideActive`, `AccessRefusal`, `OperatedFromRefusal`, `Qualified`, `PlayerMayOperate`, `MeetingLineKey`, `MeetingLineSeconds`.
- **Both forms exist** (`14`; content must use the post): `WorkReading`, `OfferRisingTo`, `AttemptRising`, `SetAllocationBy`, `Delegate`, `RevokeDelegation`, `DelegatedTo`, `RevokeCredential`, `LockOut`, `ClearLockout`, `LockedOut`, `LockoutNotice`, `MayCallUp`, `Train`.

## What this register cannot answer

- **Whether it is complete in *intent*.** It proves every public function is present (the
  check), but not that every situation a scenario will want is provided. Find that by asking
  what `docs/scenario-atlas.md`'s nine scenarios need and seeing whether the list answers.
- **Whether a reached hook is *good* to reach.** That a console shows a number is not that the
  decision is legible; only the owner's sitting can say.
- **The exact authoring surface for scenarios.** No scenario runs these hooks yet; there is no
  director (`docs/walkthrough.md` G4/G5). The register says what *can* be called; the scenarios
  that call it do not exist.

