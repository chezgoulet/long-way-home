// ship_core.h -- the ship as a working system, engine-independent.
//
// Gate S1 of docs/ship-programme.md: power generation and distribution, the major systems,
// compartments and their atmosphere, consumables, the ship's clock, and the crew roster with its
// watches and daily routine. Plain data and deterministic functions with no game header, so it is
// tested on its own (tests/ship) and can be hosted by the single-player module now and by a
// multiplayer server later. Nothing here assumes one player, or any player.
//
// Every number that claims to describe Voyager is listed in docs/lore-ledger.md with its source,
// or marked there as invented. Power is in "EPS units", an invented scale: canon gives no figures.

#ifndef LWH_SHIP_CORE_H
#define LWH_SHIP_CORE_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ship {

const int DECKS = 15;
const int COMPLEMENT = 141;
const int MAX_DUPLICATES = 16; // a transporter copy adds a record beyond the complement, bounded [inv]
const int WATCHES = 3;
const int SECONDS_PER_DAY = 86400;
const int SECONDS_PER_WATCH = SECONDS_PER_DAY / WATCHES;

// ---- systems ----------------------------------------------------------------------------------

enum SystemId : uint8_t {
	SYS_LIFE_SUPPORT = 0,
	SYS_STRUCTURAL_INTEGRITY,
	SYS_INERTIAL_DAMPERS,
	SYS_COMPUTER_CORE,
	SYS_SHIELDS,
	SYS_SENSORS,
	SYS_WARP_DRIVE,
	SYS_IMPULSE_DRIVE,
	SYS_PHASERS,
	SYS_TORPEDO_LAUNCHERS,
	SYS_NAV_DEFLECTOR,
	SYS_COMMUNICATIONS,
	SYS_TRANSPORTERS,
	SYS_SICKBAY,
	SYS_TURBOLIFTS,
	SYS_TRACTOR_BEAM,
	SYS_REPLICATORS,
	SYS_HOLODECKS,
	// The five the budget ruling adds (docs/budget-squaring.md, Part 4b): the science the ship was
	// built for, and the comforts the brownout ladder was missing. Appended, so every existing id is
	// unchanged; the shedding order is the `priority` field, not this order.
	SYS_ASTROMETRICS,
	SYS_SCIENCE_LABS,
	SYS_GRAVITY_PLATING,
	SYS_LIGHTING,
	SYS_CARGO_HANDLING,
	SYS_COUNT
};

// The stations a system is operated from. Engineering distributes power to everything and so sees
// everything; each other station sees and switches only its own systems.
enum Station : uint8_t { STN_ENGINEERING = 0, STN_TACTICAL, STN_OPS, STN_CONN, STN_SICKBAY, STN_COUNT };

enum Department : uint8_t { DEPT_COMMAND = 0, DEPT_ENGINEERING, DEPT_SECURITY, DEPT_SCIENCES, DEPT_MEDICAL, DEPT_COUNT };

struct SystemSpec {
	const char *name;
	int deck;            // where its primary station is
	const char *station; // the compartment that controls it
	Department dept;
	int demand;          // EPS units at full output
	int priority;        // default shedding order: lower is kept longer
	int crewNeeded;      // on-duty crew at the station for full efficiency
};

const SystemSpec &Spec(SystemId id);
Station StationOf(SystemId id);        // the station that operates it (never STN_ENGINEERING: that one sees all)
const char *StationName(Station s);
bool OperatedFrom(SystemId id, Station s);   // may this station see and switch this system?
// Reading and operating are different privileges (owner ruling, 2026-10-07). A station may READ a
// system another station operates -- the sensor picture on Tactical, the comms traffic on Tactical
// while Operations speaks -- and cannot change it. This is the information portfolio, wider than
// OperatedFrom; the console draws an operated system as a control and one of these as a readout.
bool StationReads(Station s, SystemId id);

// Every modelled system has three failure states designed, not just one (docs/failure-is-content.md):
// degraded, offline and destroyed, each named in the log and on the console as it is reached.
enum SystemState : uint8_t { SYS_NOMINAL = 0, SYS_DEGRADED, SYS_OFFLINE, SYS_DESTROYED, SYS_STATE_COUNT };
const char *SystemStateName(uint8_t state);
uint8_t FailureStateOf(float health, bool enabled);

// ---- condition sets the odds, and stress sets the severity (docs/failure-is-content.md) ----------
//
// The owner's ruling, 2026-10-06: a system's chance of failing is a function of its condition, and
// the load at the moment of use sets the severity of what happens. It is general -- every system
// carries it, not the transporter alone. The condition is the capability the rest of the simulation
// already tracks: the system's health (maintenance and parts) capped by what power is actually
// reaching it. The stress is the load the moment puts on it: low on a quiet watch, up to 1.0 at
// battle stations.
//
//   * in the top tenth of capability (0.9 and above) the system is nominal: no anomaly, ever;
//   * below that the odds rise as the condition falls;
//   * at severe degradation, or under severe stress, it lets go visibly at the console.
//
// The three severities are the document's own: degraded, acute, catastrophic. No new vocabulary.

const float NOMINAL_CONDITION = 0.9f;   // the top tenth of capability: nominal, no consequence
const float ANOMALY_ODDS_MAX = 0.85f;   // a wholly gone system's chance of an anomaly on one use [inv]

// The three severities of docs/failure-is-content.md: it still works worse; it hurts now; it is
// permanent. ANOMALY_NONE is "the use was clean", not a fourth severity.
enum AnomalySeverity : uint8_t { ANOMALY_NONE = 0, ANOMALY_DEGRADED, ANOMALY_ACUTE, ANOMALY_CATASTROPHIC, ANOMALY_SEVERITY_COUNT };
const char *AnomalyName(uint8_t severity);

// The chance of an anomaly on one use: 0 in the top tenth, rising as the condition falls.
float AnomalyOdds(float condition);
// The severity of an anomaly at this condition under this load (0..1 stress). Determined by the
// condition first and the load second -- a 40% system run light gives the low severity, the same
// system at battle stations the high one.
uint8_t AnomalySeverityFor(float condition, float stress);
// One use. `roll` is the deterministic 0..UINT32_MAX draw: the odds are checked first, then the
// severity, exactly as the ruling says. Returns ANOMALY_NONE when the use was clean.
uint8_t RollAnomaly(float condition, float stress, uint32_t roll);

// Who set a system's allocation (docs/power-assignment.md): the player, an officer acting under a
// standing delegation, automatic mode, or nobody (the default, full and online). This is the
// provenance the console shows per commitment and the rule that keeps the ladder off a person's
// decision: automatic mode never touches a system whose `allocBy` is not ALLOC_UNSET.
enum AllocationBy : uint8_t { ALLOC_UNSET = 0, ALLOC_PLAYER, ALLOC_DELEGATE, ALLOC_AUTO, ALLOC_BY_COUNT };
const char *AllocationByName(uint8_t by);

struct System {
	float health = 1.0f;    // 0 destroyed .. 1 intact; output can never exceed it
	bool enabled = true;    // switched on at its console
	int priority = 0;       // the automatic mode's shedding order (consoles may still change it)
	int allocated = 0;      // EPS units granted this tick
	int manned = 0;         // on-duty crew at the station this tick
	float staffing = 0.0f;  // ... weighted by how rested and willing they are (morale, fatigue)
	int repairing = 0;      // damage-control crew working on it this tick
	float control = 1.0f;   // 1 = the crew's, 0 = the intruders'; below HIJACKED it answers to them, not to us
	float output = 0.0f;    // 0..1: what the system is actually delivering
	float wear = 0.0f;      // 0..1: deferred maintenance (derived each tick, not saved); at 1 it fails
	uint8_t fault = 0;      // the last named failure state, to log the change (derived, but stored)
	// The allocation a person sets: the fraction of its full demand the system is given (0..1).
	// Operational capacity follows it -- at forty per cent the system underperforms at forty per
	// cent rather than failing. `allocBy` names who set it; `allocCrew` names the officer (a
	// delegation) or -1. `allocDamagedOff` is the last tick's answer to "was a person's allocation
	// removed by damage rather than by a decision", so the change is logged once, as damage.
	float share = 1.0f;
	uint8_t allocBy = ALLOC_UNSET;
	int allocCrew = -1;
	bool allocDamagedOff = false;
};

// The capability a system still has, 0..1: its health, capped by what power is reaching it. This is
// the "condition" of the ruling (docs/failure-is-content.md).
float SystemCondition(const System &sys);

// ---- power ------------------------------------------------------------------------------------

enum SourceId : uint8_t { SRC_WARP_CORE = 0, SRC_IMPULSE_REACTORS, SRC_AUXILIARY, SRC_BATTERIES, SRC_COUNT };

struct Source {
	float health = 1.0f;
	bool online = true;
	int output = 0;         // EPS units supplied this tick
};

// ---- compartments and consumables ---------------------------------------------------------------

// Who physically holds a deck (docs/borg-incursion.md). The Borg pressure writes it, the crew's
// actions write it, and the deck adapter reads it on load to decide what the player walks into.
enum DeckController : uint8_t { CTRL_CREW = 0, CTRL_BORG, CTRL_CONTESTED, CTRL_SEALED, CTRL_UNINHABITABLE, CTRL_COUNT };
// No dwell, no write: an intruder must hold a deck unopposed this long (ship-seconds) before any
// system here or anyone on it is written to (docs/borg-incursion.md). Below the threshold, nothing.
// The three thresholds: compromise begins, then they hold ground (contested), then seizure outright.
const float DWELL_COMPROMISE = 120.0f;
const float DWELL_HOLD = 240.0f;   // ship-seconds unopposed before the deck counts as contested
const float DWELL_SEIZE = 360.0f;  // ship-seconds before the deck's systems are seized outright
const float FIELD_HOLD_MINUTES = 5.0f; // one intruder drains one field level in this many minutes [inv]
const int FIELD_MAX = 10;              // a field's rating; a level-10 field can cut a drone from the Collective
const char *ControllerName(uint8_t c);

struct Deck {
	float atmosphere = 1.0f; // 0 vacuum .. 1 breathable
	float gravity = 1.0f;    // 0 freefall .. 1 standard: the plating, on life support (deck 12)
	float hull = 1.0f;       // 0 open to space .. 1 intact
	float intruders = 0.0f;  // hostile boarders on this deck (fractional while a fight wears them down)
	bool borg = false;       // the boarders here are Borg: they assimilate what they hold
	uint8_t boarderKind = 0; // 0 raider, 1 Borg, 2 hunter: what they are and what they do when they hold
	int objective = 0;       // the deck they are making for; 0 = the nearest of the bridge or Engineering
	float assimilated = 0.0f; // 0 ours .. 1 wholly Borg; at ASSIMILATED the deck's systems are theirs outright
	bool forceField = false; // a field over a hull breach: the deck keeps its air (environmental control)
	float forceFieldLevel = 0.0f; // the field's rating 0..10 (docs/borg-incursion.md); 0 = no field
	float fire = 0.0f;       // 0 none .. 1 an inferno; injures the crew and spreads until it is fought
	int stripping = 0;       // engineers cutting Borg technology out of this deck this tick
	int defenders = 0;       // security crew fighting here this tick
	int sealing = 0;         // engineers sealing this deck's hull this tick
	int firefighting = 0;    // crew fighting this deck's fire this tick
	// The incursion: who holds the deck, whether its systems can be trusted, how long intruders have
	// held unopposed, and whether they were ever here (for the clean-intercept record).
	uint8_t controller = CTRL_CREW;
	bool compromised = false; // an intruder has written to a system here
	float dwell = 0.0f;       // ship-seconds the intruders have held unopposed
	bool engaged = false;     // intruders are (or were) here since the last clean sweep
};

// The phaser array's setting (docs/ship-systems.md, tier 2: "a power setting from stun to vaporize").
// It is Tactical's standing decision and it is the decision that exists when there is no contact: a
// higher setting draws more EPS from the same bank and does more to a hull, and the low settings are
// for repelling boarders without killing the people you are trying to save. [our call] on the ladder.
enum PhaserYield : uint8_t { YIELD_STUN = 0, YIELD_HEAVY_STUN, YIELD_KILL, YIELD_VAPORIZE, YIELD_COUNT };
const char *PhaserYieldName(uint8_t y);

struct Stores {
	float deuterium = 1.0f;      // fraction of tankage; the fusion reactors and the warp core burn it
	float antimatter = 1.0f;     // fraction of pods; the warp core burns it
	float batteries = 1.0f;      // fraction of emergency cell charge
	int torpedoes = 38;          // not replaceable
	float spareParts = 100.0f;   // what repairs are made of; a system rebuilt from nothing costs PARTS_PER_SYSTEM
	float medicalSupplies = 100.0f; // what treatment is made of; without it the injured only get worse
	float rations = 100.0f;      // days of food in the galley's stores; the mess consumes them, the replicators restock
	float materials = 40.0f;     // raw material from salvage; the replicators fabricate it into spare parts
	int probes = 6;              // exploration: a probe looks at something hostile instead of the ship [lore]

	// The away-team kit (the tricorder gap): what leaves the ship, and what it needs.
	int tricorders = 4;          // scanning, and a charge shared by the set
	int phasers = 6;             // the other answer
	int evSuits = 4;             // for the places that have no air
	float tricorderCharge = 1.0f; // 0 = dead .. 1 = fresh; a weak charge reads wrong
	float kitCondition = 1.0f;   // 0 = battered .. 1 = serviceable; rough use and losses wear it

	// The phaser bank's setting (Tactical's standing decision, and the one that exists with no
	// contact): YIELD_STUN..YIELD_VAPORIZE. A higher setting asks for more power and hits harder.
	// The default is STUN, the bank's nominal demand (docs/lore-ledger.md): at the old default of
	// KILL the opening full load was 1,805 against the 1,800 fresh grid, so there was no headroom.
	uint8_t phaserYield = YIELD_STUN;
};

// ---- crew -------------------------------------------------------------------------------------

enum Activity : uint8_t { ACT_ON_DUTY = 0, ACT_MEAL, ACT_RECREATION, ACT_PERSONAL, ACT_SLEEP, ACT_COUNT };
enum CrewStatus : uint8_t { CREW_FIT = 0, CREW_INJURED, CREW_DEAD, CREW_ASSIMILATED };

// ---- the character layer (O8) -------------------------------------------------------------------
//
// docs/character-attributes.md, docs/crew-roster.md, docs/morale.md. The four kinds of modifier,
// kept apart -- skills, traits, conditions, state -- plus the drives, the species, and the
// derivation that produces them from the record and the seed. The one-place account is
// docs/character-derivation.md. Everything here is static content except conditions and the three
// morale reads, which are the save; a generated crew member is a function of the seed, never a
// hand-written table entry.

// Skills: seven, rated 0-5. What a person *can* do. They set repair speed and failure risk.
enum Skill : uint8_t {
	SKILL_ENGINEERING = 0, SKILL_MEDICAL, SKILL_SCIENCE, SKILL_SECURITY, SKILL_OPERATIONS,
	SKILL_COMMAND, SKILL_FLIGHT, SKILL_COUNT
};
const char *SkillName(uint8_t s);
const int SKILL_MAX = 5;
// The skill a department's work leads with (engineering leads engineering, medical medical, and so
// on): the one place the mapping lives, used by the derivation and by the work an operator does.
uint8_t DepartmentSkill(Department d);

// Traits: few, permanent, behavioural -- legible in a log line, never a percentile.
enum Trait : uint8_t {
	TRAIT_STEADY_UNDER_FIRE = 0, TRAIT_LIGHT_SLEEPER, TRAIT_GOOD_WITH_PEOPLE,
	TRAIT_CLAUSTRAPHOBIC, TRAIT_POOR_WITH_AUTHORITY, TRAIT_FIRST_CONTACT_TRAINED, TRAIT_ADAPTABLE,
	TRAIT_QUICK_HEALER, TRAIT_SCROUNGER, TRAIT_COUNT
};
const char *TraitName(uint8_t t);

// Drives, in three parts (docs/crew-roster.md): desire, need, fear.
enum Desire : uint8_t {
	DESIRE_PROMOTION = 0, DESIRE_HOME, DESIRE_A_PERSON, DESIRE_TO_PROVE, DESIRE_TO_BE_LEFT_ALONE,
	DESIRE_COUNT
};
const char *DesireName(uint8_t d);
enum Need : uint8_t { NEED_SLEEP = 0, NEED_FOOD, NEED_COMPANY, NEED_PURPOSE, NEED_MEDICAL, NEED_COUNT };
const char *NeedName(uint8_t n);
enum Fear : uint8_t {
	FEAR_DYING_ALONE = 0, FEAR_DECOMPRESSION, FEAR_THE_BORG, FEAR_USELESSNESS, FEAR_COWARDICE,
	FEAR_COUNT
};
const char *FearName(uint8_t f);

// Conditions -- the missing layer: temporary, sourced, legible. Buffs and debuffs live here.
// Each names its source, its valence, a small magnitude, when it arrived, what clears it, and who
// can see it. A modifier nobody can see is a hidden penalty, and hidden penalties feel like bugs.
enum ConditionId : uint8_t {
	COND_NONE = 0,
	COND_EXHAUSTED, COND_HUNGRY, COND_HYPOXIC, COND_IRRADIATED, COND_INFECTED, COND_CONCUSSED,
	COND_GRIEVING, COND_AFRAID, COND_PON_FARR, COND_MEDITATION_DUE, COND_EMITTER_LOW,
	COND_HOT_MEAL, COND_COFFEE, COND_SHORE_LEAVE, COND_PROMOTED, COND_SERVICE_HELD, COND_STIMULANT,
	COND_TRUTH_TOLD, COND_RESTED,
	COND_ID_COUNT
};
const char *ConditionName(uint8_t id);
enum ConditionValence : uint8_t { CVAL_DEBUFF = 0, CVAL_BUFF };
const char *ConditionValenceName(uint8_t v);
// clears-when: rest, sickbay, a meal, the end of the watch, or the passage of salience.
enum ConditionClear : uint8_t {
	CLEAR_REST = 0, CLEAR_SICKBAY, CLEAR_MEAL, CLEAR_END_OF_WATCH, CLEAR_SALIENCE, CLEAR_COUNT
};
const char *ConditionClearName(uint8_t c);
// Magnitude: small and legible, not "+7%" -- slower, unreliable, sharper.
enum ConditionMagnitude : uint8_t { CMAG_SLIGHT = 0, CMAG_CLEAR, CMAG_SHARP, CMAG_COUNT };
const char *ConditionMagnitudeName(uint8_t m);
// Visibility bits: whether the player can see it, the crew can, and the log records it. A
// condition with visibility 0 is refused: it would be exactly the hidden penalty this forbids.
enum ConditionVisibility : uint8_t { CVIS_PLAYER = 1 << 0, CVIS_CREW = 1 << 1, CVIS_LOG = 1 << 2 };
const int CONDITION_MAX = 3;        // two or three at a time, never a soup [doc]
const int CONDITION_SOURCE_MAX = 31; // so one named cause cannot bloat the save [inv]
struct Condition {
	uint8_t id = COND_NONE;
	std::string source;                 // the named cause: "concussed in the coolant bay"
	int8_t valence = CVAL_DEBUFF;
	uint8_t magnitude = CMAG_SLIGHT;
	float onset = 0.0f;                 // ship seconds when it arrived
	uint8_t clears = CLEAR_REST;
	uint8_t visible = CVIS_PLAYER | CVIS_CREW | CVIS_LOG;
};

// Species: capabilities, needs and susceptibilities -- never bonuses. The record holds words and
// the condition ids a species is prone to; there is no multiplier anywhere (docs/character-
// attributes.md, the two rules). `restNeedHours`, `needsFood` and `sleeps` are requirements, not
// bonuses: the shape of rest for the watch bill and the galley, not a better one.
enum Species : uint8_t {
	SPECIES_HUMAN = 0, SPECIES_VULCAN, SPECIES_BETAZOID, SPECIES_KLINGON, SPECIES_OCAMPA,
	SPECIES_TALAXIAN, SPECIES_BOLIAN, SPECIES_BORG_RECOVERED, SPECIES_HOLOGRAM, SPECIES_COUNT
};
const char *SpeciesName(uint8_t sp);
const int SPECIES_TRAIT_MAX = 4;
struct SpeciesRecord {
	const char *name;
	const char *capabilities[SPECIES_TRAIT_MAX];     // what they can do that another cannot
	const char *needs[SPECIES_TRAIT_MAX];            // what they require differently
	const char *susceptibilities[SPECIES_TRAIT_MAX]; // what can go wrong for them specifically
	uint8_t prone[SPECIES_TRAIT_MAX];                // condition ids the simulation may derive
	float restNeedHours;                             // hours of rest the body needs in a day [inv]
	bool needsFood;                                  // holograms take nothing by mouth [lore]
	bool sleeps;                                     // false: the rest is a cycle, not sleep
};
const SpeciesRecord &SpeciesOf(uint8_t sp);
const char *SpeciesCapability(uint8_t sp, int i);      // i<0..: null when none
const char *SpeciesNeed(uint8_t sp, int i);
const char *SpeciesSusceptibility(uint8_t sp, int i);

// ---- memory and consequence (docs/memory-and-consequence.md) -----------------------------------
//
// Every character keeps a bounded set of marks, per thing that happened: what, who it involved, how
// they came to know it (provenance), when, how it felt, and how much it still matters. Provenance is
// the whole boundary rule -- a character cannot know what they were never told. Valence toward a
// person, repeated, is a bond: friendship or grudge. Salience decays unless reinforced, and the set
// is bounded, evicting the least salient first.
enum MemorySource : uint8_t { MEM_SAW = 0, MEM_TOLD, MEM_RUMOUR, MEM_LOG, MEM_SOURCE_COUNT };
enum MemoryEvent : uint16_t { MEM_DEATH = 1, MEM_ORDER, MEM_PROMISE, MEM_LIE, MEM_RESCUE, MEM_VIOLATION, MEM_FUNERAL, MEM_LOCKOUT,
	MEM_OVERRULED = 9 }; // an officer's recommendation was refused: he remembers it (docs/power-assignment.md, Task C)
const int MEMORY_MAX = 8;
struct Memory {
	uint16_t event = 0;
	int16_t person = -1;     // the crew member it involved, or -1 for an impersonal event
	uint8_t source = MEM_SAW;
	float time = 0.0f;       // ship seconds when it happened
	float valence = 0.0f;    // -1 grief/resentment .. +1 pride/relief
	float salience = 0.0f;   // 1 fresh; decays unless reinforced
	bool orphaned = false;   // its citation is gone: a MEM_LOG mark after the published logs were purged
};

// A promise is a bond with a claim attached (docs/memory-and-consequence.md). An officer commits to
// something in front of a crew member: the crew member takes a mark naming the promiser, and the
// claim is held so that it can mature. Kept, the mark strengthens and the bond rises; broken, or
// with its deadline passed and nothing said, the mark turns negative and the log carries the reason
// in the crew member's own terms -- not in the ship's.
enum PromiseKind : uint8_t { PROMISE_REPAIR = 0, PROMISE_RESCUE, PROMISE_PROMOTION, PROMISE_WAY_HOME, PROMISE_KIND_COUNT };
const char *PromiseKindName(uint8_t k);
enum PromiseState : uint8_t { PROMISE_OPEN = 0, PROMISE_KEPT, PROMISE_BROKEN, PROMISE_STATE_COUNT };
const int PROMISE_MAX = 16;      // a bounded set of outstanding claims [inv]
struct Promise {
	int promiser = -1;       // who committed
	int beneficiary = -1;    // the crew member left holding the mark
	uint8_t kind = PROMISE_REPAIR;
	std::string what;        // what was promised, in words
	double made = 0.0;       // ship seconds it was made
	double deadline = -1.0;  // ship seconds by which it must be done; -1 = no deadline
	uint8_t state = PROMISE_OPEN;
};

// The month report (docs/the-record-and-the-log.md): the periodic beat, drafted honestly by the
// simulation from the record and edited by the player through the console path. The record keeps
// the diff, so the player can always see what they actually did while the crew can only ever read
// the published version. Its headline is the navigation counter's change since the last entry.
const int REPORT_LINE_MAX = 24;  // [inv]
const int REPORT_MAX = 8;        // signed reports kept, each with the diff [inv]

struct ReportLine {
	std::string scope;     // subject, for filtering (the log's own scopes)
	std::string text;      // as published
	std::string draft;     // as drafted: the record keeps the diff, the crew see only `text`
	uint16_t event = 0;    // the mark this line speaks to, or 0 for a plain claim
	int person = -1;       // ... and who it names
	bool struck = false;   // struck from the published version
	bool added = false;    // added by the player: it has no drafted original
};

// A report published to the crew is read by everyone under its signer; a report filed upward is
// read by nobody below. That is the direction of the toll: lying up costs nothing from below.
enum ReportAudience : uint8_t { REPORT_TO_CREW = 0, REPORT_UPWARD, REPORT_AUDIENCE_COUNT };
const char *ReportAudienceName(uint8_t a);

struct MonthReport {
	int number = 0;
	double time = 0.0;          // ship seconds it was drafted
	float counter = 0.0f;       // the navigation counter at this entry: the estimated years at current capability (-1: no warp)
	float counterChange = 0.0f; // ... since the previous entry: the derivative, the headline
	std::string signer;
	int department = DEPT_COUNT; // the section it covers, or DEPT_COUNT for the captain's whole ship
	uint8_t audience = REPORT_TO_CREW;
	bool open = true;           // still a draft, being edited by the player
	bool signed_ = false;       // signed and published
	std::vector<ReportLine> lines;
};

struct CrewMember {
	std::string name;
	std::string type;        // the game's NPC type used to embody them (an existing character)
	uint8_t rank = 0;        // 0 crewman .. 6 captain
	Department dept = DEPT_COMMAND;
	uint8_t watch = 0;       // 0 alpha (0800-1600), 1 beta (1600-2400), 2 gamma (0000-0800)
	uint8_t post = SYS_COUNT;   // the system whose station they stand; SYS_COUNT = department duties
	uint8_t quartersDeck = 0;
	uint8_t status = CREW_FIT;
	float fatigue = 0.0f;    // 0 rested .. 1 exhausted
	// Morale is a read of three components, never a stored counter (docs/morale.md): deficit
	// (what they are short of), outlook (what they believe about the situation), and holdings
	// (who they hold with). Morale() below derives the reading; nothing writes a scalar.
	float deficit = 0.0f;    // 0 nothing short .. 1 wholly short (sleep, food, care, comfort, company)
	float outlook = 0.75f;   // 0 despair .. 1 hope: are we getting home, is command competent
	float holdings = 0.75f;  // 0 alone and blaming .. 1 held: bonds, allegiances
	float exposure = 0.0f;   // seconds spent on a deck without air; injures, then kills
	float burn = 0.0f;       // seconds spent in fire (derived each tick, not saved); injures, then kills
	float radiation = 0.0f;  // seconds of exposure to a failing core on deck 11 (derived, not saved)
	float wounds = 0.0f;     // 0..1 taken fighting boarders; at 1 they are out of the fight, injured
	float assimScar = 0.0f;  // 0..1 a de-assimilation's lasting residue: the deck reclaimed is a changed deck
	float severity = 0.0f;   // 0 minor .. 1 critical: how badly an injury will go without treatment
	float recovery = 0.0f;   // 0..1 progress of an injured crew member's treatment
	bool underCare = false;  // derived each tick: has a sickbay bed now (not saved)
	bool away = false;       // beamed off the ship on an away mission; on no deck until they return
	uint8_t credentials = 0; // bitmask over Station: cross-qualifications earned by training (S10's career)
	uint8_t faction = 0;     // 0 Starfleet, 1 Maquis: the split, and a drag between the two
	bool brigged = false;    // confined: operates nothing, stands no watch, until the hearing
	float quartersQuality = 0.5f; // 0 bare .. 1 comfortable: where they bunk, and how it colours the mood
	float holoCompulsion = 0.0f;  // 0 .. 1+: time in the holodeck's program, and whether it will not end
	bool quartersSealed = false;  // grief: the quarters of a crew member who has died are sealed
	std::vector<Memory> memories; // bounded marks: what they know, and how they came to know it

	// The character layer (O8). The following are static content, derived from the record and the
	// seed (DeriveCharacter) and never stored: the same seed produces the same person.
	uint8_t species = SPECIES_HUMAN;
	uint8_t skills[SKILL_COUNT] = {1, 1, 1, 1, 1, 1, 1};
	uint16_t traits = 0;          // bitmask over Trait
	uint8_t desire = DESIRE_PROMOTION;
	uint8_t need = NEED_SLEEP;
	uint8_t fear = FEAR_DYING_ALONE;
	// Conditions are dynamic, and therefore in the save.
	Condition conditions[CONDITION_MAX];
	uint8_t conditionCount = 0;

	// derived each tick
	uint8_t activity = ACT_SLEEP;
	uint8_t deck = 0;        // where they are now
};

// ---- the character layer, read and derived -------------------------------------------------------

bool HasTrait(const CrewMember &c, uint8_t trait);
// The derivation: given the record (its identity, department, rank and species) and a seed
// stream, fills the four kinds and the drives. Pure and deterministic; it is a function of the
// record, not a table of hand-written people. Called from BuildRoster, and again on load, so the
// static half is never in the save.
void DeriveCharacter(CrewMember &c, uint32_t &rng);
// Species for a record: named crew carry theirs; a generated member draws one from the seed.
uint8_t DeriveSpecies(uint8_t dept, uint32_t &rng);

// Conditions. Add is bounded (CONDITION_MAX, evicting the oldest debuff), deduplicated by id, and
// refuses a condition with no visibility at all. Find returns null when absent. The Ship-level
// wrappers record the arrival and the cure in the log, which is what "recorded" means.
bool AddCondition(CrewMember &c, uint8_t id, const std::string &source, int8_t valence,
                  uint8_t magnitude, uint8_t clears, float now, uint8_t visible);
bool ClearCondition(CrewMember &c, uint8_t id);
const Condition *FindCondition(const CrewMember &c, uint8_t id);

// Morale, the read (docs/morale.md). Never a hidden counter: Morale() is a function of the three
// components, so a person's morale can always be explained from them, and MoraleReason() names the
// component most responsible, in words, for the log. The weights are ours (docs/character-
// derivation.md), small and recorded so they can be overruled.
float Morale(const CrewMember &c);
const char *MoraleBandName(float morale);       // fit / worn / strained / at breaking point
const char *MoraleReason(const CrewMember &c);  // the component most responsible, in words

// Effective skill: base skill + aptitude traits, reduced by conditions, deficits and haste
// (docs/character-attributes.md, "the derivation"). This is what decides whether a job is quick,
// slow or dangerous.
float EffectiveSkill(const CrewMember &c, uint8_t skill);

// The manner (G14's second half): what a person says at the morale they carry. Deterministic, a
// demonstration for the owner to judge, not a verdict.
const char *MannerLine(const CrewMember &c);

// The Ship-level condition wrappers: they add or clear the condition on a named person and record
// the arrival or the cure in the log, so a condition is legible in the record as well as on the
// person.
struct Ship;
bool Sicken(Ship &s, int crew, uint8_t id, const std::string &source, int8_t valence,
            uint8_t magnitude, uint8_t clears, uint8_t visible);
bool Cure(Ship &s, int crew, uint8_t id);

// ---- the four kinds reach the work: the operator's factors, named (docs/character-attributes.md) --
//
// The owner's ruling: buffs and debuffs must have impacts on the quality of work and the outcomes.
// The failure roll's odds are therefore a function of what the operator can actually do, not only of
// the system's condition. The system still sets the base odds and the load still sets the severity
// (docs/failure-is-content.md); the operator's factors move the odds. Each factor is one of the
// kinds, kept distinct and named, so a worse outcome can be explained rather than reading as a
// random failure:
//
//   * skill      -- what they can do: a competent hand lowers the odds, an unskilled one raises them;
//   * condition  -- temporary and situational: a debuff raises the odds while it holds, a buff lowers;
//   * trait      -- durable and behavioural: it reaches the work by interacting with the moment
//                   (steady under fire helps under load), never as a flat shift;
//   * drive      -- a motive: it biases what a person does and how they bear up, conditionally on the
//                   work (fear of decompression on a hull task), never a flat subtraction;
//   * morale     -- the read from deficit, outlook and holdings: a person who does not believe the
//                   course is worth the cost works like one who does not;
//   * capability -- a species capability is two-sided (a Betazoid's empathy reads a patient's feeling
//                   for good and takes on others' pain for bad), never a bonus.
//
// A modifier the player cannot see is worse than no modifier at all, so every factor carries its
// name and its measured weight, and WorkFactorLine() renders the list the log and the console carry.
enum WorkFactorKind : uint8_t {
	WORK_SKILL = 0, WORK_CONDITION, WORK_TRAIT, WORK_DRIVE, WORK_MORALE, WORK_CAPABILITY,
	WORK_FACTOR_KIND_COUNT
};
const char *WorkFactorKindName(uint8_t k);

// What the work is: the moment a trait or drive is answering. Set by the system a use is made of --
// a hull task, a medical task, a hazardous use -- and available to any other work as needed.
enum WorkContext : uint8_t {
	WORK_ROUTINE = 0, WORK_HAZARD, WORK_HULL, WORK_RECLAIM, WORK_MEDICAL, WORK_CONTEXT_COUNT
};
const char *WorkContextName(uint8_t c);
uint8_t WorkContextOf(SystemId id);   // the context a system's own use is made in

const int WORK_FACTOR_MAX = 16;
struct WorkFactor {
	uint8_t kind = WORK_SKILL;
	std::string name;     // "afraid (slight)", "steady under fire (under load)", "morale (worn): ..."
	float delta = 0.0f;   // signed shift to the odds: + worse, - better
};

// The named factors a person brings to a task of `skill` in `context` under `stress`, in a fixed
// order. Only factors that actually move the odds are returned. Pure and deterministic.
int WorkFactors(const CrewMember &c, uint8_t skill, uint8_t context, float stress,
                WorkFactor *out, int maxOut);

// The odds of an anomaly for one use: the system's condition sets the base, the operator's factors
// move it. Clamped to [0,1], and the top tenth of capability stays nominal whatever the operator, so
// a well-kept system still does not mangle anyone (docs/failure-is-content.md's promise).
float WorkOdds(float condition, const WorkFactor *f, int n);
// One use, as RollAnomaly, with the operator's factors reaching the odds. The severity ladder is
// unchanged: the condition and the load still set it.
uint8_t RollWork(float condition, float stress, const WorkFactor *f, int n, uint32_t roll);
// The factors as one line, for the log and the console: "condition afraid (slight) +0.15; drive
// afraid of decompression (sealing the hull) +0.25". Empty when nothing moved the odds.
std::string WorkFactorLine(const WorkFactor *f, int n);
// The whole picture for one person at one system -- who, what they can do, and every factor that
// moved the odds -- for the personnel console. This is the read that makes "why is this person
// working badly" answerable.
std::string WorkReading(const Ship &s, int crew, SystemId id);

// ---- rising to the occasion: the act, its cost, and the memory it leaves ------------------------
//
// docs/rising-to-the-occasion.md. The character layer reaches the work (above); this is the other
// direction, and it is built from the same parts rather than beside them. A post must be held and
// nobody who can hold it is available, and the person in front of it may exceed their own effective
// skill. The choice is the heroism, not the success:
//
//   * the drive is the lever, and it is the work-bites realisation of a fear by the task in front
//     of them -- RisingDriveAtStake() reads the WORK_DRIVE factor, it does not reread the drive;
//   * the attempt is made beyond effective skill: the shortfall to a qualified hand worsens the
//     odds, which are worse than the task would be for someone properly qualified and never zero;
//   * the cost is not optional, and its severity is the one the condition and the load set
//     (docs/damage-and-budgets.md's own shape -- condition sets the odds, stress sets the
//     severity -- applied to a person instead of a system);
//   * the witnesses remember by name through the existing mark system, and the mark's salience
//     decays unless reinforced, so a heroism the crew keep retelling persists.
//
// There is no heroism stat, no courage roll and no bar: the act is situational, and the same person
// does it here and does not do it there. The evidence is docs/evidence/rising-to-the-occasion.md;
// the console is `ship rise`; the transcript is `test_ship_core --rising`.

const float RISING_QUALIFIED = 3.0f;      // the effective skill a qualified hand holds the post at [inv]
const float RISING_BEYOND_SLOPE = 0.35f;  // each point beyond their effective skill worsens the odds [inv]
const float RISING_MIN_SUCCESS = 0.05f;   // success is never impossible: a heroism that cannot succeed is a formality [inv]
const float RISING_WITNESS_BOND = 0.7f;   // the valence a witness carries toward the hero [inv]
const float RISING_HOLDINGS_RISE = 0.12f; // the regard (bond and allegiance) that moves with it [inv]

// What came of the offer. RISING_NONE: no post here needs a hand beyond its skill. RISING_REFUSED:
// offered, and they would not -- a decision, not a dice roll. RISING_SUCCEEDED: the post held.
// RISING_FAILED: it let go, and it still cost. RISING_DIED: the extreme -- a heroism that cannot
// kill is a cutscene.
enum RisingOutcome : uint8_t {
	RISING_NONE = 0, RISING_REFUSED, RISING_SUCCEEDED, RISING_FAILED, RISING_DIED,
	RISING_OUTCOME_COUNT
};
const char *RisingOutcomeName(uint8_t o);

// The offer, and the decision that answers it: a situation in which a post must be held and the
// person in front of it is beyond their effective skill, together with why they are the one -- the
// drive the work realises, named. `choice` is RISING_REFUSED when they will not, else RISING_NONE
// (the attempt is AttemptRising's). A pure read: it writes nothing, and the same ship gives the same
// offer.
struct RisingOffer {
	bool offered = false;
	int post = SYS_COUNT;
	uint8_t context = WORK_ROUTINE;
	uint8_t skill = SKILL_ENGINEERING; // the department skill the post asks of a qualified hand
	float condition = 1.0f;            // the post's capability now (SystemCondition)
	float stress = 0.0f;               // the load now (StressNow)
	int candidate = -1;                // the person in front of the post
	int qualified = -1;                // a qualified hand already in the moment, if any: offered is false
	uint8_t choice = RISING_NONE;      // RISING_REFUSED when they will not
	std::string drive;                 // the drive the offer turns on, named; empty when none is at stake
	std::string reason;                // why they rise, or why they will not: the log's own words
};
RisingOffer OfferRising(const Ship &s, int post);
// The same, for a named hand: the console and a scenario address the post, and the simulation
// resolves the post to a person (docs/the-entry-point.md, the post-not-name law). This names one.
RisingOffer OfferRisingTo(const Ship &s, int post, int crew);
// The drive a context realises in a person: the WORK_DRIVE factor the work-bites pass already
// computes (a fear realised by the task in front of them), or empty when none is at stake. It is
// the same reading, not a second one.
std::string RisingDriveAtStake(const CrewMember &c, uint8_t skill, uint8_t context, float stress);

// The odds of the attempt: the person's own factors, moved by how far it is beyond their effective
// skill. Worse than the same post for a qualified hand, and never zero -- and a nominal post still
// fails no one (docs/failure-is-content.md's promise, kept).
float RisingOdds(const RisingOffer &o, const CrewMember &c, const WorkFactor *f, int n);
// One attempt, end to end: the decision from the drives and the morale (a refusal is logged, so it
// is legible), the roll beyond effective skill, the cost (a condition, a wound, and at the extreme
// their life), and the witnesses' memory. `roll` is the deterministic 0..UINT32_MAX draw the engine
// passes from its own counter. Writes the record and the log; returns the outcome, and sets `out`
// to the account in the engine's own words when given.
uint8_t AttemptRising(Ship &s, int post, int crew, uint32_t roll, std::string *out = nullptr);
// The deterministic draw the engine passes to AttemptRising, from the same saved counter UseSystem
// draws on (s.riskRolls), so the act's roll replays identically across a save.
uint32_t NextRisingRoll(Ship &s);
// The crew retell it: the mark is reinforced, so a heroism the crew keep telling persists, and one
// nobody retells fades (a mark's salience decays unless reinforced). Returns how many witnesses
// still carry the mark and had it sharpened.
int RetellRising(Ship &s, int hero);

// Where a crew member is and what they are doing at a time of day, by their watch alone. The day
// is eight hours on duty, then a meal, recreation, personal time and eight hours' sleep.
Activity ScheduledActivity(int watch, int secondOfDay);

// The posts the ship's systems need to be fully manned, summed from the specs. Canon's crew of 100
// ("The 37's") must be able to cover them; below that the ship is flyable but degraded. [inv]
int PostsNeeded();

// The skeleton-crew arithmetic of "The 37's" (docs/ship-systems.md): with `crew` people spread
// evenly over the three watches, how many of the posts one watch can man, taking the most important
// first (the same priority order power is kept in). At 100 every post is covered; below it the least
// important go unmanned while the critical set -- what keeps her alive and moving -- still holds.
struct WatchCoverage { int perWatch = 0; int postsCovered = 0; int postsNeeded = 0; bool critical = true; };
WatchCoverage CoverWithCrew(int crew);

// ---- the outside (S9) ---------------------------------------------------------------------------
//
// The ship crosses a sector of beacons, FTL-fashion: a jump needs the warp drive and burns fuel, and
// what waits at the far end is fixed by the sector's seed. A hostile ship is fought with the ship's
// own systems -- what the phasers and shields deliver is what the simulation says they deliver, so a
// fight is won or lost in Engineering as much as at Tactical -- and what gets through lands on a
// deck and a system. With our shields down, the enemy sends boarders across.

enum BeaconKind : uint8_t { BEACON_EMPTY = 0, BEACON_HOSTILE, BEACON_DERELICT, BEACON_BORG, BEACON_TRADER, BEACON_DISTRESS, BEACON_BELT, BEACON_PREWARP, BEACON_GOAL, BEACON_KIND_COUNT };
const char *BeaconKindName(BeaconKind k); // the one place a beacon kind is named

struct Beacon {
	BeaconKind kind = BEACON_EMPTY;
	bool visited = false;
	bool surveyed = false;       // a sensor survey has said what is here, whether or not we have been
	bool looted = false;         // a derelict has been stripped with the tractor, once
	std::vector<int> links;      // beacons one jump away
	// A phenomenon (docs/exploration-and-science.md): an anomaly with hidden attributes, revealed one
	// scan at a time, whose correct response depends on the set revealed. Misreading it is a number
	// the crew watch getting worse.
	bool phenomenon = false;
	uint8_t phenomAttrs = 0;     // bitmask of the attributes revealed so far
	uint8_t phenomAttrVal[3] = {0, 0, 0}; // the hidden value of each attribute (0..3), seed-derived
	uint8_t phenomTruth = 0;     // the correct response (0..3), a function of the three values
};

// An opponent has systems of its own, not three numbers: its weapons decide what it does to us, its
// engines whether it can run or chase, its shield generator whether its shields come back. More than
// one kind flies the sector, and a raider whose engines survive can follow us between beacons.
enum EnemyKind : uint8_t { ENEMY_RAIDER = 0, ENEMY_WARSHIP, ENEMY_BORG_VESSEL, ENEMY_KIND_COUNT };
enum EnemySubsystem : uint8_t { TARGET_HULL = 0, TARGET_WEAPONS, TARGET_ENGINES, TARGET_SHIELD_GEN, TARGET_COUNT };

struct Enemy {
	bool present = false;
	bool borg = false;           // kept for the Borg path; true iff kind == ENEMY_BORG_VESSEL
	EnemyKind kind = ENEMY_RAIDER;
	float hull = 0.0f;           // 0..1
	float shields = 0.0f;        // 0..1
	float weapons = 1.0f;        // its weapons subsystem: what it does to us scales with this
	float engines = 1.0f;        // its engines: can it run, or chase us between beacons
	float shieldGen = 1.0f;      // its shield generator: shields come back by this
	float firepower = 0.0f;      // shield strength it strips from us per minute, at full weapons
	int boarders = 0;            // it will send these across once, when our shields are down
	float adaptation = 0.0f;     // Borg only: how far it has adapted to our weapons (0 fresh .. 1 immune)
};

// Shuttles are supported but not pilotable (docs/shuttles.md): launching one is a load screen, and
// what the ship tracks is whether each is in the bay, away, or lost. Everything else -- lifeboat
// arithmetic, boarding, a hit on the shuttlebay, away missions, the job queue -- reads that.
enum ShuttleClass : uint8_t { SHUTTLE_CLASS2 = 0, SHUTTLE_TYPE6, SHUTTLE_TYPE8, SHUTTLE_AEROSHUTTLE, SHUTTLE_CLASS_COUNT };
enum ShuttleLocation : uint8_t { SHUTTLE_IN_BAY = 0, SHUTTLE_AWAY, SHUTTLE_LOST };
enum ShuttleComms : uint8_t { COMMS_LINKED = 0, COMMS_INTERMITTENT, COMMS_LOST };

struct Shuttle {
	ShuttleClass cls = SHUTTLE_CLASS2;
	std::string name;             // the crew name it (canon: the Delta Flyer)
	ShuttleLocation location = SHUTTLE_IN_BAY;
	float condition = 1.0f;       // 0 wrecked .. 1 serviceable
	ShuttleComms comms = COMMS_LINKED;
	int awayBeacon = -1;          // the chart row it went to while away; the loss location after
	float awaySince = 0.0f;       // ship time it left; their posts are empty while this stands
	int returnCondition = 0;      // 0 transporter window, 1 rendezvous, 2 its own power
	std::vector<int16_t> manifest;// crew records aboard while away
	float cargoMaterial = 0.0f, cargoRations = 0.0f, cargoParts = 0.0f; // loaded out of stores
};

const int SECTOR_BEACONS = 12;
const int SHUTTLE_BAY_DECK = 10;         // shuttlebay 1, aft dorsal [lore]
const float SHUTTLE_REBUILD_MATERIALS = 30.0f; // material to construct a replacement [inv]
const float SHUTTLE_REBUILD_HOURS = 8.0f;      // crew-hours of the second bay's work [inv]
const float JUMP_DEUTERIUM = 0.01f;      // fraction of tankage per jump [inv]
const float JUMP_ANTIMATTER = 0.01f;
const float PHASER_MINUTES = 6.0f;       // full phasers strip an enemy's shields, or hole a bare hull, in this [inv]
const float TORPEDO_HULL = 0.34f;        // one torpedo on an unshielded hull [inv]
const float SHIELD_RECHARGE_MINUTES = 3.0f; // full shield output restores the shields from nothing in this: faster
                                            // than an ordinary raider strips them, slower than the Borg do [inv]
const float HIT_SYSTEM = 0.15f;          // what a minute's unopposed fire does to the system it lands on [inv]
const float HIT_HULL = 0.10f;            // ... and to that deck's hull
const float SALVAGE_PARTS = 25.0f;       // spare parts recovered from a derelict [inv]
const int SECTORS_TO_CROSS = 3;          // sectors crossed before the ship is home [inv]

// The dilithium constraint (docs/exploration-and-science.md). Canon: an Intrepid core lasts up to
// three years, 75,000 ly at warp 6.2 is some seventy-five years, so one crystal is about 3,000 ly of
// progress. The jump cost and the recomposition numbers are ours.
const float DILITHIUM_LIGHT_YEARS = 3000.0f;   // cannon: one crystal's worth of progress [lore]
const float JUMP_DILITHIUM = 1.0f / 40.0f;     // a fresh crystal lasts this many cruising jumps [inv]
const float MIN_WARP_DILITHIUM = 0.02f;        // below this there is no warp at all
const float RECOMPOSITE_GAIN = 0.35f;          // what one recomposition restores [inv]
const float RECOMPOSITE_CEILING_DROP = 0.15f;  // the crystal ages: each recomposition lowers what can be restored
const float RECOMPOSITE_WARP_MIN = 0.5f;       // the warp core must be up to recomposite
const float DILITHIUM_TRADE_MATERIALS = 40.0f; // what a trader takes for a crystal [inv]
const float CRYSTAL_QUALITY_STEP = 0.1f;       // a researched better crystal shortens every jump
const int PURSUIT_JUMPS = 3;             // jumps a surviving raider follows before it catches us [inv]
const float PURSUIT_REPAIR = 0.5f;       // damage control works at this rate while pursued [inv]
const float TRADE_PARTS = 10.0f;         // spare parts a trader takes for a consignment [inv]
const float TRADE_RATIONS = 20.0f;       // ... and the rations, supplies and fuel given for them [inv]
const float ENEMY_WEAPONS_RATE = 0.5f;   // what a minute's fire does to an enemy subsystem [inv]
const float ENEMY_SHIELD_REGEN = 12.0f;  // minutes for a full shield generator to restore the shields [inv]

// ---- the log (the log-as-an-artifact gap) -------------------------------------------------------
//
// Every event that matters is written down as it happens: ship time, who is speaking, the subject
// (for scoping and search) and the fact. Command sees all of it; a post sees its own scope; the
// player can read or search it. It is bounded, and the oldest entries fall off.

struct Ship;
struct LogEntry {
	double time = 0.0;   // ship seconds since midnight of day 0
	std::string who;     // author
	std::string scope;   // subject, for filtering: bridge, engineering, sickbay, hull, command, outside
	std::string what;    // the fact
};
const int LOG_MAX = 128;
// A log entry's author and subject are short; its fact may be longer -- the opening's log seed
// states the losses, the ship's condition and the player's position, and must survive a save whole
// (docs/start-states.md). The stored cap is 63 for who and scope, LOG_TEXT_MAX for the fact.
const int LOG_TEXT_MAX = 600;
void LogEvent(Ship &s, const std::string &who, const std::string &scope, const std::string &what);

// ---- the two logs (docs/the-record-and-the-log.md) ----------------------------------------------
//
// The log is two documents, not one, and this is the second. The official log (`log`, above) is
// signed, published and scoped: the month report lives there, a post reads its own scope, command
// reads all. The personal log is the other half -- private, one per person, and where the truth goes
// when it cannot go in the report. It carries the same time / who / what shape and a visibility of
// its own: its owner's alone, not a post's scope and not command. The toll is the distance between
// the two.
//
// The law over both: the simulation writes the logs and never reads either. No decision anywhere may
// consult a log; the month report is drafted from the record's marks, never from a log. The official
// read is ReadOfficialLog; the private read is PersonalLog, and it returns one person's entries and
// nobody else's.

enum LogVisibility : uint8_t { LOG_OFFICIAL = 0, LOG_PERSONAL, LOG_VISIBILITY_COUNT };
const char *LogVisibilityName(uint8_t v);

const int PERSONAL_LOG_MAX = 128; // bounded like the official log [inv]
const int PERSONAL_LOG_TEXT_MAX = 200; // a private entry may be longer than an official one [inv]
struct PersonalLogEntry {
	double time = 0.0;                 // ship seconds since midnight of day 0
	int owner = -1;                    // the crew member whose private log this is
	std::string who;                   // the author's voice
	std::string what;                  // the fact
	uint8_t visibility = LOG_PERSONAL; // its own visibility: its owner's alone
};

// Write a private entry. `owner` must be a crew member and `what` non-empty; a full log drops its
// oldest entry. Returns false, changing nothing, otherwise. The player's own hand is `s.player`.
bool WritePersonalLog(Ship &s, int owner, const std::string &what);
// The private read: this person's entries, newest last, and nobody else's -- not a scope, not
// command, not a post's clearance. Every entry returned passes PersonalVisibleTo for this reader.
std::vector<PersonalLogEntry> PersonalLog(const Ship &s, int owner);
// The visibility rule, in one place: a personal entry is visible only to its owner. (The official
// log's scopes are the other half; a personal entry never appears in them, whatever the scope.)
bool PersonalVisibleTo(const PersonalLogEntry &e, int reader);
// The official read, factored out so the search itself is testable: the newest `count` entries,
// optionally filtered to one `scope` (empty = all of them). Personal entries are never returned here.
std::vector<LogEntry> ReadOfficialLog(const Ship &s, int count, const std::string &scope);

// ---- the meeting: the brief, the skeleton, and the seams (docs/staff-meetings.md) ----------------
//
// Phase one of the meeting system is text and state. The simulation enqueues a *meeting brief* and it
// is a read: it never writes (docs/staff-meetings.md, docs/programme-meetings-and-voice.md). A brief
// carries who is present, what is being decided and the enumerated options with their costs, and the
// ship and the people as they are right now. The meeting works with the model absent: every kind has
// an authored **skeleton** -- the outcomes, their costs, and minimal dialogue for each -- which is the
// floor, not a fallback. Every line carries its **delivery direction**, and the seam that hands a line
// to the synthesizer carries that annotation with the text. Nothing here calls a model, owns a sound,
// or builds an audio player: those are phase two and three, named and left.
//
// The invariant: the simulation decides; any model only speaks. A chosen option resolves to one
// enumerated outcome, and the simulation applies it. Nothing that generates text may choose a result.

// The kinds of meeting the design names. The first two are the normal case: a brief is generated for
// every meeting, not only the dramatic ones (the dead-layer rule, docs/programme-meetings-and-voice.md
// lesson 6). A watch change always produces one, and an ordinary departmental meeting that resolves
// nothing still produces one.
enum MeetingKind : uint8_t {
	MEET_WATCH_CHANGE = 0, // scheduled: the watch change (every watch; the normal case)
	MEET_DEPARTMENTAL,     // routine: daily, and it may resolve nothing
	MEET_ALLOCATION,       // power is argued and set (docs/power-assignment.md)
	MEET_DILITHIUM,        // the reserve crosses its threshold (docs/exploration-and-science.md)
	MEET_CASUALTIES,       // casualties exceed the beds (docs/gap-triage-and-sickbay.md)
	MEET_BORG,             // Borg pressure rising (docs/borg-incursion.md)
	MEET_DEFERRED,         // an open promise has come due: a decision deferred (docs/memory-and-consequence.md)
	MEET_KIND_COUNT
};
const char *MeetingKindName(uint8_t kind);

// Delivery direction (docs/evidence/voice-review.md, finding three). The synthesizer's `exaggeration`
// defaults to 0.5 whatever the text says, so an emergency order and a request for lunch arrive at the
// same emotional temperature until something asks for a difference. Every line carries one of these,
// and the seam to synthesis carries it with the text. The vocabulary is small on purpose; the values
// are the review's own knob (`exaggeration` / `emotion_adv`).
//
// UNMARKED is a line whose delivery the brief cannot know: it is *marked* rather than guessed, and the
// seam refuses it until a direction is set. It is never silently defaulted to flat.
enum Delivery : uint8_t {
	DELIVERY_ORDER = 0,   // an order or a warning: urgency. exaggeration 0.8 (the review's fix pass)
	DELIVERY_REPORT,      // a factual report or an answer: neutral. 0.5
	DELIVERY_CONFESSION,  // something admitted against interest: low and close. 0.35
	DELIVERY_CONDOLENCE,  // grief said to someone: low and slow. 0.3
	DELIVERY_FLAT,        // procedural, the log read aloud: flat. 0.2
	DELIVERY_UNMARKED,    // the brief cannot know it: marked, not guessed
	DELIVERY_COUNT
};
const char *DeliveryName(uint8_t d);
// What the delivery asks the synthesizer for, in the review's own knob. Negative for UNMARKED: there
// is no value to ask for, and the seam refuses the line.
float DeliveryExaggeration(uint8_t d);
bool DeliveryKnown(uint8_t d);

// A line of the meeting: who says it, what is said, and how it is delivered. The delivery travels with
// the text through the seam to synthesis (Task C).
//
// The authored skeleton cannot name a generated crew member's index, so a speaker is either a roster
// index (>= 0) or one of the roles below, resolved against the room at play time. ResolveSpeaker does
// that, preferring someone already present.
enum MeetingSpeaker : int16_t {
	SPEAK_ROOM = -1,       // the room, or the person the line is answering
	SPEAK_COMMAND = -2,
	SPEAK_ENGINEERING = -3,
	SPEAK_SECURITY = -4,
	SPEAK_SCIENCES = -5,
	SPEAK_MEDICAL = -6,
};
struct MeetingLine {
	int speaker = SPEAK_ROOM;              // roster index, or a MeetingSpeaker role
	std::string text;
	uint8_t delivery = DELIVERY_UNMARKED;  // carried with the text
};

// What an option asks the simulation to do. The options are the enumerated outcomes made visible; the
// simulation applies the chosen one -- the script and any model never do. The model speaks only.
enum MeetingEffect : uint8_t {
	EFFECT_RECORD = 0,      // the decision is minuted; no state changes
	EFFECT_SET_ALLOCATION,  // a person's allocation, with that person's provenance
	EFFECT_SET_POWER_AUTO,  // the ship's own answer: automatic mode on or off
	EFFECT_ACCEPT_RECOMMENDATION,
	EFFECT_REFUSE_RECOMMENDATION,
	EFFECT_ORDER_TRIAGE,    // sickbay: 0 worst first, 1 rank first
	EFFECT_SECURITY_TO_DECK,// 0 = the deck the simulation names (most contested)
	EFFECT_EVACUATE_DECK,   // 0 = the deck the simulation names
	EFFECT_SET_ALERT,       // amount = Alert
	EFFECT_RECOMPOSITE,     // Engineering buys back life in the crystal
	EFFECT_PROMISE_KEPT,    // make good the promise that has come due
	EFFECT_COUNT
};
const char *MeetingEffectName(uint8_t e);

const int MEETING_ALLOC_MAX = 2;
struct MeetingAlloc { uint8_t system = 0xFF; int percent = 100; };

struct MeetingOption {
	uint8_t intent = 0;                    // the intent descriptor: novelty matching and the pill share it
	std::string label;                     // the pill's short description
	std::string cost;                      // what it costs, in words, shown at the choice
	uint8_t effect = EFFECT_RECORD;
	MeetingAlloc alloc[MEETING_ALLOC_MAX]; // EFFECT_SET_ALLOCATION
	int amount = 0;                        // kind-specific magnitude (triage policy, a deck, an alert)
	bool on = true;                        // EFFECT_SET_POWER_AUTO
};

const int MEETING_LINE_MAX = 4;
struct MeetingOutcome {
	MeetingOption option;
	MeetingLine lines[MEETING_LINE_MAX];
	int lineCount = 0;
};

const int MEETING_OUTCOME_MAX = 6;
struct MeetingSkeleton {
	uint8_t kind = MEET_WATCH_CHANGE;
	std::string decision;                  // what is being decided, in one line
	MeetingOutcome outcomes[MEETING_OUTCOME_MAX];
	int outcomeCount = 0;
};
// The authored skeleton for a kind: the outcomes, their costs, and minimal dialogue for each. The
// static floor every meeting sits on. Never null: an unknown kind gets the watch-change skeleton.
const MeetingSkeleton &AuthoredSkeleton(uint8_t kind);

// The brief's per-participant view. A brief is built per participant from that person's marks and the
// log, and from nothing else (docs/the-record-and-the-log.md): what this person knows, the open claims
// they carry, and the log they can read. This is what keeps a meeting from being a room where everyone
// already knows the truth.
const int MEETING_VIEW_MAX = 6;
struct BriefMark { uint16_t event = 0; int person = -1; uint8_t source = 0; float valence = 0.0f; float salience = 0.0f; };
struct BriefPromise { uint8_t kind = 0; int promiser = -1; int beneficiary = -1; std::string what; double deadline = -1.0; };
struct BriefParticipant {
	int crew = -1;
	std::string name;
	uint8_t post = 0;      // SystemId, or SYS_COUNT for department duties
	uint8_t watch = 0;
	float mood = 0.0f;     // morale, so the speaker rail can show it
	BriefMark marks[MEETING_VIEW_MAX]; int markCount = 0;
	BriefPromise promises[MEETING_VIEW_MAX]; int promiseCount = 0;
	LogEntry log[MEETING_VIEW_MAX]; int logCount = 0;
};

const int MEETING_PARTICIPANT_MAX = 6;
const int MEETING_OPTION_MAX = 6;
const int MEETING_QUEUE_MAX = 2;
// One system as it is right now: the commitments the room argues over (docs/power-assignment.md).
struct BriefSystem {
	int allocated = 0;      // EPS granted this tick
	float output = 0.0f;    // 0..1 what it is delivering
	uint8_t allocBy = 0;    // AllocationSource: unset, the player, an officer, automatic
	bool online = true;
};
struct MeetingBrief {
	uint8_t kind = MEET_WATCH_CHANGE;
	double time = 0.0;       // ship seconds when it was emitted
	std::string trigger;     // why this meeting exists, in words
	std::string decision;    // what is being decided
	BriefParticipant present[MEETING_PARTICIPANT_MAX]; int presentCount = 0;
	MeetingOption options[MEETING_OPTION_MAX]; int optionCount = 0; // the enumerated options and their costs
	// The ship and the people as they are right now: instruments (reads that never lie).
	int powerCommitted = 0, powerAvailable = 0, powerShortfall = 0;
	uint8_t alert = 0;
	float dilithium = 0.0f;
	int casualties = 0, beds = 0;
	bool borgPressure = false;
	BriefSystem systems[SYS_COUNT]; // every system, in the order power is given to it

	// A digest of the ship state the brief was built from, so a stale script can be detected
	// (docs/staff-meetings.md: "the script can go stale"). Purely a read.
	uint32_t stateDigest = 0;
};

// Who says a line: a roster index if the skeleton named one, otherwise the participant (or officer) of
// that role. Returns -1 if nobody fits. Resolved against the room at play time.
int ResolveSpeaker(const Ship &s, const MeetingBrief &brief, int speaker);

// The brief generator. A pure read: it writes nothing, and generating a brief leaves the ship's state
// byte-identical (Task A; the acceptance that matters). `kind` is a MeetingKind.
MeetingBrief BuildBrief(const Ship &s, uint8_t kind);

// Is a meeting of this kind due right now? The watch change and the daily departmental meeting are the
// normal-case triggers; the rest are thresholds, latched so one episode produces one meeting.
bool MeetingDue(const Ship &s, uint8_t kind);

// THE EMIT SITE. The simulation enqueues a meeting brief here and nowhere else: for every meeting that
// is due, it builds the brief (BuildBrief) and appends it to the queue the async worker would drain.
// Returns the number emitted. Grep this function, never a bare count (docs/programme-meetings-and-voice.md
// lesson 6). Called by the simulation's own clock, so every meeting -- dramatic or not -- produces one.
int EmitDueMeetings(Ship &s);
const std::vector<MeetingBrief> &PendingMeetings(const Ship &s);
// Drop the oldest queued brief: the worker has taken it. Returns false if the queue is empty.
bool TakeBrief(Ship &s);

// The decision the room reached: which enumerated outcome, who decided it, and whether it was the
// ship's own answer (automatic mode) or a person's. The simulation applies it.
bool ApplyMeetingOutcome(Ship &s, const MeetingBrief &brief, int outcome, int decidedBy, bool automatic);

// ---- the seam to synthesis (docs/evidence/voice-review.md) ---------------------------------------
//
// Named and left: phase two builds the audio player. Nothing here owns a sound. The seam that hands a
// line to the synthesizer carries the delivery annotation with the text, so the line arrives with the
// emotional temperature it was written for. A line whose delivery is UNMARKED is refused rather than
// defaulted, so a missing direction is visible rather than silently flat.
struct SynthesisRequest {
	std::string text;
	float exaggeration = 0.5f;             // the review's knob: `exaggeration` / `emotion_adv`
	uint8_t delivery = DELIVERY_UNMARKED;
};
bool LineToSynthesis(const MeetingLine &line, SynthesisRequest &out);

// ---- the audio plumbing: three sources, one owner each (docs/staff-meetings.md) ------------------
//
// Phase two. A meeting has THREE audio producers where a conversation had one: the pre-rendered
// dialogue lines, the pre-generated non-lexical cues, and a live line on the novel-answer path. That
// is exactly the configuration that cost Hermes Vox a month (docs/programme-meetings-and-voice.md,
// lesson 1), so the ownership is written down here before any of it is built -- and it is testable:
//
//   ONE PRODUCER PER TRACK. A track is a source. Each track has exactly ONE owner (TrackOwner). A
//   producer that does not own a track CANNOT write to it, and CANNOT retire anything on it.
//   ONE RETIREMENT RULE PER REPLY. A reply is retired by the owner of the track it is on, through
//   VoiceRetire, and by no other path. A cue therefore can never stop a dialogue line: the cue track
//   is a different track, and its owner cannot reach a reply on the dialogue track.
//
// Nothing here owns a sound or opens an audio device: this is the model the player (phase three)
// drives. The live producer exists and is named now; phase three fills its track.

enum VoiceTrack : uint8_t { TRACK_DIALOGUE = 0, TRACK_CUE, TRACK_LIVE, TRACK_COUNT };
const char *VoiceTrackName(uint8_t track);

// The producers. Exactly one owns each track.
enum VoiceProducer : uint8_t {
	PROD_RENDERER = 0, // owns TRACK_DIALOGUE: the pre-rendered dialogue lines
	PROD_CUE_PLAYER,   // owns TRACK_CUE: the pre-generated non-lexical cues
	PROD_LIVE,         // owns TRACK_LIVE: the live line (phase three)
	PROD_COUNT
};
const char *VoiceProducerName(uint8_t producer);
uint8_t TrackOwner(uint8_t track);              // the one owner, as an invariant
bool OwnsTrack(uint8_t producer, uint8_t track);

const int VOICE_REPLY_MAX = 1; // one player per source: a track holds one reply at a time
struct VoiceReply {
	int id = -1;           // >= 0 while active; unique within the mixer
	uint8_t track = TRACK_DIALOGUE;
	std::string asset;     // the clip being played
	int cue = -1;          // for TRACK_CUE: the CueId; else -1
};
struct VoiceTrackState {
	VoiceReply replies[VOICE_REPLY_MAX];
	int replyCount = 0;
	int retires = 0;       // how many this track has retired (for tests; never the evidence)
};
struct VoiceMixer {
	VoiceTrackState tracks[TRACK_COUNT];
	int nextId = 0;
};

// Start a reply on `track` as `producer`. Returns a reply id (>= 0), or -1 and changes nothing if the
// producer does not own the track, or the track already has a reply (one player per source).
int VoiceWrite(VoiceMixer &m, uint8_t producer, uint8_t track, const std::string &asset, int cue = -1);
// Retire a reply. THE ONLY retirement rule: the producer must own the track the reply is on. Returns
// false and changes nothing otherwise -- so a cue player cannot retire a dialogue reply, and the
// renderer cannot retire a cue.
bool VoiceRetire(VoiceMixer &m, uint8_t producer, int replyId);
bool TrackBusy(const VoiceMixer &m, uint8_t track);
const VoiceReply *ActiveReply(const VoiceMixer &m, uint8_t track);

// The cue set: small, and clips, never text. A sentence-prosody TTS reading "Mm?" is its worst case
// (lesson 3), so these are PCM clips on their own track, and only real dialogue goes to the
// synthesizer. Each cue has exactly one purpose, and the cue track's owner retires them.
enum CueId : uint8_t {
	CUE_BREATH = 0,  // a breath: the pause is a person, not a machine
	CUE_CHAIR,       // a chair shifting: someone changes posture -- attention, or discomfort
	CUE_PADD_TAP,    // a PADD tap: the room thinking, a beat filled by a hand
	CUE_HMM,         // "Hmm.": acknowledgement while the answer is late
	CUE_HOLDING,     // "I have to think about that.": the short holding line the design names
	CUE_COUNT
};
const char *CueName(uint8_t cue);
const char *CueClip(uint8_t cue);     // the clip in the player-local cue set
const char *CuePurpose(uint8_t cue);  // what it is for, in words
bool CueIsLexical(uint8_t cue);       // the one cue that is a spoken line rather than non-verbal

// THE CUE EMIT SITE. A cue is played here and nowhere else. The cue track's owner retires it; no
// other path may. It writes the emission to the log (scope "meeting") so the emit site can be grepped
// -- never a counter -- and returns the reply id, or -1 if the cue track is already busy.
int EmitCue(Ship &s, VoiceMixer &m, uint8_t cue, const std::string &reason);

// The render queue and the cache. Rendering happens off the critical path, in the async window the
// design already has: the player never waits on it, and a meeting can start with the queue unfinished
// because the skeleton always exists. The cache is keyed so a re-render is avoided -- same text, same
// voice, same delivery direction, same file -- and it lives beside the save, pruned with it, never in
// the repository.
const int VOICE_CACHE_MAX = 64;
const int VOICE_QUEUE_MAX = 32;

// The cache key. Length-prefixed so no text can forge another key's boundary.
std::string RenderKey(const std::string &voice, const std::string &text, uint8_t delivery);

struct RenderJob {
	std::string key;           // RenderKey(voice, text, delivery)
	std::string voice;         // who is cast (the reference the renderer uses)
	std::string text;
	uint8_t delivery = DELIVERY_UNMARKED;
	float exaggeration = 0.5f; // carried from the delivery to the synthesizer's own knob
	int speaker = SPEAK_ROOM;  // resolved against the room
};
struct VoiceCacheEntry { std::string key; std::string file; };
struct VoiceRender {
	std::string dir;                    // beside the save; the host supplies it. Never the repository.
	std::vector<VoiceCacheEntry> cache; // rendered keys -> file
	std::vector<RenderJob> queue;       // queued, not yet rendered (the async worker drains it)
	bool warm = false;                  // the model has been warmed in the async window
	double warmedAt = 0.0;
};

// Where a key's clip lives: <dir>/<key>.wav (the synthesizer's output; never the repository).
std::string VoiceCachePath(const VoiceRender &vr, const std::string &key);
bool VoiceCached(const VoiceRender &vr, const std::string &key);
bool VoiceQueued(const VoiceRender &vr, const std::string &key);
// Enqueue a render. 1 = newly queued; 0 = already cached or already queued (a re-render is a no-op);
// -1 = the delivery is UNMARKED, so the line is not rendered; -2 = the queue is full. A key already
// queued or cached is never enqueued twice.
int QueueRender(VoiceRender &vr, const RenderJob &job);
// A line the renderer finished: it enters the cache and leaves the queue. False if it was not queued
// (so a render nobody asked for cannot enter the cache).
bool CacheRendered(VoiceRender &vr, const std::string &key, const std::string &file);
// The cache is pruned with the save: the index is emptied and the host deletes the directory.
// Returns the number of entries dropped.
int PruneVoiceCache(VoiceRender &vr);
// Plan a meeting's audio: every line of every outcome of its skeleton, on the dialogue track, keyed
// and deduplicated. Resolves each speaker against the room, and marks the unmarked line rather than
// rendering it. Returns the number newly queued. A read: the ship is unchanged.
int PlanMeetingAudio(VoiceRender &vr, const Ship &s, const MeetingBrief &brief, const MeetingSkeleton &sk);
// Warm the model in the async window, off the critical path (lesson 4), so the first meeting anyone
// sees is never the cold one. Where it happens is the async phase, when the worker opens. Returns
// false if it was already warm. Writes the warm to the log.
bool WarmVoice(Ship &s, VoiceRender &vr, double now);

// ---- what the ship has given up (docs/damage-and-budgets.md, docs/story-and-semantics.md) --------
//
// Because there is never enough crew to fix everything, the player chooses what to write off: a deck
// sealed and left, a system stripped for parts, a compartment marked uninhabitable. Canon's Year of
// Hell is exactly this -- seven decks uninhabitable and the ship kept flying. The ship therefore
// carries a written list of what she has given up, so the player can walk past the sealed hatch and
// remember why. Each entry is a time, a thing, the kind of loss, and the name of whoever decided.
// The list is bounded and saved, and the oldest entry falls off.

enum LossKind : uint8_t { LOSS_SEALED = 0, LOSS_STRIPPED, LOSS_UNINHABITABLE, LOSS_WRITTEN_OFF, LOSS_KIND_COUNT };
const char *LossKindName(uint8_t k);
const int LOSS_MAX = 24;
struct LossEntry {
	double time = 0.0;   // ship seconds since midnight of day 0
	uint8_t kind = LOSS_WRITTEN_OFF;
	bool system = false; // true = a system, false = a compartment (deck)
	int16_t target = 0;  // the deck (1..DECKS) or the SystemId given up
	std::string what;    // the thing given up, named
	std::string who;     // who decided
};
// Write it off: the one moment the ship gives something up for good. The author is whoever commands
// (docs/story-and-semantics.md: everyone aboard remembers who made the call). A thing already on the
// list is not listed twice; past LOSS_MAX the oldest falls off. Returns false, changing nothing, if
// the target is not a real compartment or system, or the kind is not one of the four.
bool WriteOff(Ship &s, bool system, int target, uint8_t kind);
const std::vector<LossEntry> &WriteOffs(const Ship &s);

// ---- the job queue (docs/crew-work.md) ---------------------------------------------------------
//
// The ship's outstanding work as a queue with a face: one job per thing that needs doing -- a damaged
// system repaired, a breached hull sealed, an assimilated deck reclaimed -- with its kind, its
// target, how far it has got, and its place in the order. The damage-control party works it; command
// sets the order. The queue is bounded and saved, so a thing left undone is still there tomorrow.
enum JobKind : uint8_t { JOB_REPAIR = 0, JOB_SEAL, JOB_RECLAIM, JOB_BUILD, JOB_KIND_COUNT };
const char *JobKindName(uint8_t k);
const int JOB_MAX = 24;
const float BUILD_HOURS_PER_PART = 4.0f; // engineer-hours to fabricate one spare part [inv]
struct Job {
	uint8_t kind = JOB_REPAIR;
	int16_t target = 0;   // a system id for repair, a deck (1..15) for seal/reclaim, parts for build
	float progress = 0.0f; // 0 .. 1 (for build, toward the next part)
	int16_t priority = 0;  // lower is worked sooner; command sets it
};
// The queue as it stands after this tick (rebuilt from the ship's state, plus the player's build jobs,
// order preserved for the jobs that persist).
const std::vector<Job> &Jobs(const Ship &s);
// Command orders a net-new thing built: the crew fabricate spare parts over crew-hours. Given by
// whoever commands; false if it does not apply.
bool OrderBuild(Ship &s, int parts);
// Command sets a job's place in the order (docs/crew-work.md: "priority is where rank lives"). Lower
// is worked sooner. False, changing nothing, if the player does not command or the index is not a job.
// The board of the queue is where this is given; a job that persists keeps the priority set on it.
bool SetJobPriority(Ship &s, int index, int priority);
// The security squad (docs/borg-incursion.md): a fireteam command sends to retake a deck, advancing
// a deck at a time and holding it while the crew restore it. The crew layer embodies them.
const int SQUAD_MAX = 4;
const float SQUAD_TRAVEL_MINUTES = 3.0f; // minutes to advance one deck [inv]
bool OrderAdvance(Ship &s, int deck);

// ---- the ship ---------------------------------------------------------------------------------

enum Alert : uint8_t { ALERT_GREEN = 0, ALERT_YELLOW, ALERT_RED };

// ---- how it is played (S10) -------------------------------------------------------------------------

// Ironman is the game: one ship, no going back. Holodeck is for learning and testing: saves allowed.
enum PlayMode : uint8_t { MODE_IRONMAN = 0, MODE_HOLODECK };
// Accelerated: a ship's day in `dayScale`-compressed game time, only while playing. Real time: a day
// is a day, only while playing. Wall clock: a day is a day and the ship lives on while you are away.
enum ClockMode : uint8_t { CLOCK_ACCELERATED = 0, CLOCK_REAL_TIME, CLOCK_WALL };
// Who the player is. Any post: a crew member, with that member's clearance. In command: the ship
// answers to you. Munro: the Hazard Team's ensign, as in the retail game.
enum PlayerRole : uint8_t { ROLE_ANY_POST = 0, ROLE_IN_COMMAND, ROLE_MUNRO };

// ---- the configurator: the player chooses the start state (docs/the-entry-point.md, Part three) ----
//
// A start state is *data* (docs/start-states.md): the casualties, the vacancies derived from them by
// the same rule play uses, the player's record, and the first log entry. The configurator has four
// dimensions -- who you are, who died, who fills the gaps, and the career path -- and this header
// holds the data the four edit. The surface that starts the run is the surface that chooses it.

// The career path (dimension four). This pass lets the player *choose* it and records it; the arc the
// path implies (resentment, affinity, who trusts you) is the affinities work, authored later.
enum CareerPath : uint8_t {
	CAREER_JUNIOR = 0,      // Starfleet junior: the ceiling is intact above you
	CAREER_LOWER_DECKS,     // lower decks: no ceiling, no authority, the work itself
	CAREER_MAQUIS,          // Maquis integrating into Starfleet: the split is on your record
	CAREER_CHAIR,           // the chair: you command from the first minute
	CAREER_COUNT
};
const char *CareerPathName(uint8_t path);
const char *CareerPathBlurb(uint8_t path);

// The command seats (dimension two: who died -- per seat, survivor or casualty; and dimension three:
// who fills the gaps -- the seat a casualty leaves is filled by the derivation, never by hand). Each
// seat knows the named record that holds it and the department the rule promotes from.
enum CommandSeat : uint8_t {
	SEAT_CAPTAIN = 0,
	SEAT_FIRST_OFFICER,
	SEAT_SECURITY,
	SEAT_ENGINEERING,
	SEAT_MEDICAL,
	SEAT_SCIENCES,
	SEAT_OPERATIONS,
	SEAT_CONN,
	SEAT_COUNT
};
const char *SeatName(uint8_t seat);       // "the chair", "the first officer's seat", ...
const char *SeatType(uint8_t seat);       // the named record that holds it at the canon default
uint8_t     SeatDepartment(uint8_t seat); // the department the derivation promotes from
uint8_t     SeatPost(uint8_t seat);       // the station the seat stands, or SYS_COUNT
uint8_t     SeatRank(uint8_t seat);       // the rank the seat carries (the chair is 6)
// The seat whose named holder is this type, or -1. Used to initialise the canon default and to read
// which casualty created a vacancy.
int SeatForType(const std::string &type);

// A start state, as the configurator edits it. Plain data: the selector keeps them in a list and a
// further state is a row (docs/evidence/the-way-in.md).
struct StartState {
	const char *name = "CANON";
	const char *blurb = "";
	const char *reason = "";
	// Dimension two: per command seat, survivor (false) or casualty (true).
	bool casualty[SEAT_COUNT];
	// Dimension three: the roster is fictitious entirely -- none of the show characters appear. The
	// vacancy derivation still fills every seat, from the generated crew.
	bool fictitious = false;
	// Dimension one and four: the player's record and the path that brought them here.
	const char *playerName = "";   // empty: the roster's own default for the seat/post
	uint8_t playerRank = 1;
	uint8_t playerDept = DEPT_COMMAND;
	int playerSeat = -1;           // the seat the player inherits, or -1 for an ordinary post
	uint8_t career = CAREER_JUNIOR;
	// The opening's cause and the ship's condition (the Caretaker aftermath).
	const char *cause = "the Caretaker's array";
	int shipDead = 12;             // the ship's dead from the Caretaker, canon at twelve or more

	StartState() { for (int i = 0; i < SEAT_COUNT; ++i) casualty[i] = false; }
};

// The shipped proving cases: the canon default, the captain, and an all-fictitious crew. A list as
// data -- the selector renders it, and a further state is an entry rather than a rewrite.
int StartStateCount();
const StartState &StartStateAt(int i);

// Apply a start state to a fresh ship: close the casualties, seat the player (composing with
// character creation), derive the vacancies by the one rule (FillVacancies), and write the log seed.
void ApplyStartState(Ship &s, const StartState &st);

// The vacancy derivation, one rule for the configurator and for play alike (docs/start-states.md):
// a command seat whose holder is gone is filled from the roster by seniority -- the chair and the
// first officer by the senior fit officer, a department seat by the senior fit member of the
// department. It changes only seats that are actually vacant, so it is idempotent and safe to call
// every tick; it records each rise. Returns the number of seats filled. The player is never picked by
// the derivation: the player's seat is the player's choice, and the derivation fills the rest.
int FillVacancies(Ship &s, const std::string &reason);
// The seat a crew member holds, or -1. The read the roster and the log use.
int SeatHeldBy(const Ship &s, int crew);
int SeatHolder(const Ship &s, uint8_t seat);
// The player's own position, stated plainly (the opening's fourth duty): rank and post, what they
// may authorise, and who reports to them. One line, for the log and the console.
std::string PlayerPositionLine(const Ship &s);

// The log seed (docs/start-states.md): the first entry in the ship's log, describing *what the player
// actually configured* -- the losses, the ship's condition, that no rescue is coming, the player's
// own position, and the career path. Later entries are measured against it.
void WriteLogSeed(Ship &s, const StartState &st);
std::string LogSeedText(const Ship &s, const StartState &st);

// The access model's stored shapes, defined here so Ship can hold them (docs/access-and-authority.md).
// A delegation is a shift's grant; an override is one station forced for a while; a lock-out is a
// senior officer shutting a post-holder out.
struct Delegation {
	int grantor = -1;   // the department head who granted it
	int grantee = -1;   // the junior who holds it
	uint8_t station = 0;
	double expires = 0.0; // ship time the shift ends
};
struct Override {
	int station = -1;    // the station being forced, or -1 for none
	int first = -1;      // who began it
	int second = -1;     // the second who agreed, or -1 for a solo force
	double readyAt = 0.0; // ship time it becomes active
	double expires = 0.0; // ship time it lapses
	bool active = false;
	bool solo = false;
};
struct Lockout {
	uint8_t station = 0;
	int lockedBy = -1;
	int locked = -1;
	double time = 0.0;
};

// A delegation of standing authority over a *band of the budget* (docs/power-assignment.md, Task C):
// granted by command to a named officer, held until revoked, and the log says who holds it. While it
// stands, that officer sets the allocation of any system in the band, and each commitment carries
// their name as its provenance. Revocation takes effect immediately.
enum BudgetBand : uint8_t { BAND_PROPULSION = 0, BAND_TACTICAL, BAND_SCIENCE, BAND_OPERATIONS, BAND_COMFORT, BAND_COUNT };
const char *BandName(uint8_t band);
BudgetBand BandOf(SystemId id);
const int GRANT_MAX = 8; // a bounded set of standing grants [inv]

struct BandGrant {
	int grantor = -1;   // who granted it (the player/command)
	int grantee = -1;   // the officer who holds it
	uint8_t band = 0;
	double granted = 0.0; // ship time it was granted
};

struct Config {
	uint32_t seed = 2371;        // roster generation; the same seed is the same crew
	float dayScale = 60.0f;      // ship seconds per simulated second: 60 = a day in 24 minutes
	PlayMode mode = MODE_IRONMAN;
	ClockMode clockMode = CLOCK_ACCELERATED;
	PlayerRole role = ROLE_ANY_POST;
	bool fictitious = false;     // the roster is generated entirely: no show character appears
};

struct Ship {
	Config cfg;
	double clock = 8 * 3600;     // ship seconds since midnight of day 0; starts at 0800, alpha watch
	Alert alert = ALERT_GREEN;
	System systems[SYS_COUNT];
	Source sources[SRC_COUNT];
	Deck decks[DECKS];
	Stores stores;
	std::vector<CrewMember> crew;
	std::vector<Shuttle> shuttles;   // the bay's complement, and where each is (docs/shuttles.md)

	// The dilithium constraint that forces exploration (docs/exploration-and-science.md): without a
	// crystal there is no warp, and finding one means survey, chart and detour. `dilithium` is the
	// crystal's remaining life (0..1); `crystalCeiling` is how much recomposition can still restore,
	// which ages with each recomposition; `crystalQuality` is a better crystal's efficiency (>= 1),
	// which shortens the journey.
	float dilithium = 1.0f;
	float crystalCeiling = 1.0f;
	float crystalQuality = 1.0f;
	int crystalReplacements = 0;

	// the outside
	float shieldStrength = 1.0f; // what stands between enemy fire and the hull; 0 = hits land
	std::vector<Beacon> sector;
	int beacon = 0;              // where the ship is
	Enemy enemy;
	Enemy contact2;              // a second opponent at the same beacon (more than one at a time)
	EnemySubsystem target = TARGET_HULL; // what Tactical aims at once the enemy's shields are down
	uint32_t hits = 0;           // how many hits have landed: decides, deterministically, where the next one does
	uint32_t riskRolls = 0;      // the anomaly draws taken: the deterministic counter behind UseSystem
	int cleanIntercepts = 0;     // intruders cleared before they touched a system: the recorded counter-wins
	// The counter-play kit (docs/borg-incursion.md): the phaser adapter's rotating modulation, and a
	// vinculum raid that severs the Collective's coordination for a while, each at a cost in time.
	float remodulateCooldown = 0.0f;   // ship-seconds until the modulation may be rotated again
	float adaptationSuppressed = 0.0f; // ship-seconds the Borg cannot adapt (a vinculum is down)

	// The warp core cascade (docs/failure-is-content.md): a failing core loses coolant, overheat
	// builds, containment falls, and a breach countdown begins. It is visible, traceable to what was
	// done, and interruptible -- shut the core down, restore the coolant, repair the core, or eject it.
	float coolant = 1.0f;          // coolant loops: fall with a failing core, restored by Engineering
	float coreTemp = 0.0f;         // 0 cool .. 1 runaway
	float containment = 1.0f;      // 1 nominal .. 0 breached
	float breachCountdown = -1.0f; // seconds to breach once containment is critical; -1 = none
	bool coreShutdown = false;     // the core is off: no warp, and the cascade halts
	bool coreEjected = false;      // the core is gone: no warp until a new one is found
	bool lost = false;             // the core breached: the ship is gone (the one unwinnable end)

	// Pursuit: a raider whose engines survived follows the ship between beacons, and while it is on
	// our tail the damage-control party works with one eye behind it.
	bool pursued = false;
	int pursuitJumps = 0;        // jumps until it catches us
	float pursuitStrength = 0.0f;

	// More sectors, and an end to reach: crossing a sector's last beacon opens the next; the third
	// crossed is home.
	int sectorNumber = 0;
	bool reachedEnd = false;     // this sector's last beacon has been reached
	bool won = false;            // the whole run is won

	// Population pressure: survivors and refugees taken aboard spend stores, quarters and air, and
	// crowd the crew. They are not in the roster; they are mouths.
	int refugees = 0;

	// The Maquis split as an arc: resentment between the two factions, which the crew work through.
	float resentment = 0.0f;

	// Borg strategic awareness: what the Collective has learned of this ship. It rises with every
	// Borg contact and makes the next sector more theirs, and their adaptation faster.
	float borgAwareness = 0.0f;

	// The nacelle pylons: the structural arms that hold warp together. A hit can take them, and a
	// ship without them cannot go to warp. The mobile emitter lets the EMH work away from sickbay.
	float pylonHealth = 1.0f;
	bool mobileEmitter = false;

	// The airponics bay: food that grows rather than being replicated, so the replicators can be
	// spared the power -- Kes's answer to a tight budget.
	bool airponics = false;

	// standing orders from whoever commands (S10): -1 / 0 = none
	int orderRepairFirst = -1;   // a system the damage-control party is to see to before any other
	int orderSecurityTo = 0;     // a deck security is to go to, boarders or not
	int orderEvacuate = 0;       // a deck everyone is to leave
	int orderTriage = 0;         // sickbay: 0 worst first, 1 rank first (see docs/gates.md, the gap)
	int advanceDeck = 0;         // the deck a security squad is advancing to retake; 0 = none
	int advanceAt = 0;           // the deck the squad is on now, on its way
	float advanceMs = 0.0f;      // progress toward the next deck

	// Access: delegations granted for a shift, the one emergency override in progress, and the
	// lock-outs a senior officer has imposed (docs/access-and-authority.md). All are recorded in
	// the log, the delegation is temporary, and a lock-out marks the person's memory.
	std::vector<Delegation> delegations;
	Override emergencyOverride;
	std::vector<Lockout> lockouts;

	// Power allocation (docs/power-assignment.md). Automatic mode uses the priority ladder as its
	// policy; it is off by default and never touches a system a person has set. The chief engineer's
	// recommendation is computed on demand (RecommendAllocation) rather than stored: the console
	// shows it, and the player accepts or refuses it. The grants are standing authority over a band,
	// and the log says who holds each.
	bool powerAuto = false;
	std::vector<BandGrant> bandGrants;
	int lastShortfall = 0;          // the shortfall last written to the log (derived; not saved)

	// The meeting (docs/staff-meetings.md). The queue of briefs the simulation has enqueued and the
	// async worker has not yet taken; the schedulers that decide when the normal-case briefs fall
	// (the watch change, and the daily departmental meeting); and the latch that keeps one threshold
	// episode to one meeting. A brief is a read, but the schedule it is queued on is the simulation's
	// own decision, and it persists.
	std::vector<MeetingBrief> pendingMeetings;
	int lastWatchMeeting = -1;      // day*WATCHES+watch of the last watch-change brief
	int lastDeptMeetingDay = -1;    // the day of the last routine departmental meeting
	uint16_t meetingLatches = 0;    // one bit per MeetingKind: a threshold episode that already met

	// the player
	int player = -1;             // index into crew of the player's character; -1 = none chosen
	// The configurator's record (docs/the-entry-point.md, Part three): which crew member holds each
	// command seat, derived from the casualties by the same rule play uses (FillVacancies), and the
	// career path the player chose. seatHolder[seat] is -1 when the seat is vacant. Stored, so a
	// configured start replays identically.
	int16_t seatHolder[SEAT_COUNT];
	uint8_t career = CAREER_JUNIOR;
	uint64_t wallSeconds = 0;    // wall-clock time when the ship was last saved (CLOCK_WALL catches up from it)
	bool leftStanding = false;   // ever exited with a background process: the record's mark (see the two exits)
	std::vector<LogEntry> log;   // the official log: signed, published and scoped, newest last
	std::vector<PersonalLogEntry> personalLog; // the private logs: per person, and nobody else's read
	std::vector<LossEntry> losses; // what the ship has given up, and why (docs/damage-and-budgets.md)
	std::vector<Job> jobs;       // the outstanding work, in the order it is worked (docs/crew-work.md)

	// The month report, the promises and the log's lifecycle (docs/the-record-and-the-log.md,
	// docs/memory-and-consequence.md). The report is the open draft; `reports` is the record of
	// signed reports, each keeping its diff; `promises` holds the claims so they can mature; and
	// `logPurged` says the published logs have been emptied, which orphans the MEM_LOG marks.
	MonthReport report;                // the open draft
	std::vector<MonthReport> reports;  // signed reports, each keeping its diff
	std::vector<Promise> promises;     // promises held, so they can mature
	float navCounterLast = 0.0f;       // the counter (estimated years) at the last entry: the derivative's baseline
	bool logPurged = false;            // the published logs have been purged

	// the away mission and the course (S4): where a beamed party is, and where the conn is making for
	int awayBeacon = -1;         // the site an away team is on, or -1 if none is away
	int course = -1;             // the beacon the conn has been told to make for, or -1

	// sickbay's surgical bay (the triage gap): its sterile force field holds the gravest case steady
	// even without supplies, until the field is lowered or the case can be treated.
	bool surgicalForceField = false;

	// The Emergency Medical Hologram: with the medical staff down, the doctor is a program -- so it
	// needs the computer core, and it can be switched off. It holds the ward at half output.
	bool emhActive = false;

	// The tractor beam has the contact locked: it cannot break off and run while the beam holds.
	bool enemyHeld = false;

	// housekeeping for the log's periodic "why the mood is what it is" line (not saved)
	double lastMoodLog = 0.0;

	// The player in the world (Stage B; not saved). The world reports where the player's body stands
	// each frame, and the ship's own causes then reach the person there; and it records who has come
	// for a downed player. Both are recomputed from the world, so a save is byte-identical to one made
	// without this, and the extension needs no save-format change.
	int playerDeck = 0;          // the deck the player's body is on (0 = unknown: use the roster routine)
	int playerAttended = -1;     // the crew member who has come for a downed player, or -1

	int Day() const { return static_cast<int>(clock / SECONDS_PER_DAY); }
	int SecondOfDay() const { return static_cast<int>(clock) % SECONDS_PER_DAY; }
	int Watch() const;           // the watch on duty now
	int PowerAvailable() const;  // EPS the sources are actually supplying this tick
	int PowerAllocated() const;  // ... and what the systems are drawing of it
	// The power budget the engineering console shows twice (owner ruling, 2026-10-07): what the plant
	// can deliver fresh, and what it can deliver now. Fresh is the nameplate at a full crystal and full
	// health; now is the same at the crystal's ceiling and the sources' health -- the two numbers whose
	// gap is the budget shrinking as the dilithium ages. It is the power half of the navigation counter.
	int PowerCapacityFresh() const;
	int PowerCapacityNow() const;
	int CrewFit() const;
};

// A ship in the state Voyager is in with nothing wrong: every system intact, stores full, the
// roster generated from the seed.
Ship NewShip(const Config &cfg = Config());

// Advances the ship by `seconds` of simulated (not ship) time. Deterministic: the same ship and
// the same sequence of calls give the same result.
void Tick(Ship &s, float seconds);

// The load the ship is under right now: 1.0 at battle stations, less at yellow and green. This is
// the stress a system is used under -- the second half of the ruling.
float StressNow(const Ship &s);
// Use a system under a load. Rolls its odds, names the outcome and the chain in the log (the
// condition, the load, the severity, who was at the console), and -- at the severe end -- lets it
// go at the console, the sparking console of docs/failure-is-content.md. Returns the severity.
// `who` signs the record; empty means the station's senior hand on duty.
uint8_t UseSystem(Ship &s, SystemId id, float stress, const std::string &who);
// The same use, but the operator is named by index -- the player at the console. At the severe end
// the system lets go at *that* person, the one holding the controls (Stage B), rather than at the
// station's own hand. A bad index falls back to UseSystem's behaviour.
uint8_t UseSystemBy(Ship &s, SystemId id, float stress, int operatorCrew);

// ---- what consoles and events do to it ------------------------------------------------------------

void SetAlert(Ship &s, Alert a);
void SetEnabled(Ship &s, SystemId id, bool on);
void SetPriority(Ship &s, SystemId id, int priority); // the automatic mode's order; not the mechanism

// ---- power allocation: the player and the crew decide (docs/power-assignment.md) ------------------
//
// The owner's ruling: the players and the crew define which systems are online and offline and how
// much power each has at any given time. It must not become a predetermined cascade. So a system
// carries a share a person sets, operational capacity follows the share, and nothing sheds itself:
// when the commitments exceed the plant the shortfall is reported as a number and the ship waits.
// The ladder is demoted to the policy of automatic mode, which is off by default and never applied
// to a system a person has set.

// The player sets a system's allocation, as a percentage of its demand (0..100). False if the
// system does not exist, the percentage is out of range, the system is destroyed/offline, or the
// increase would commit more than the plant can supply -- the console refuses more until something
// is freed. A decrease is always allowed.
bool SetAllocation(Ship &s, SystemId id, int percent);
// The same, but set by a named officer acting under a standing delegation over that system's band.
// False if the officer holds no grant for the band: the authority to decide is not theirs.
bool SetAllocationBy(Ship &s, SystemId id, int percent, int officer);
int AllocationPercent(const Ship &s, SystemId id);     // the share a person set, 0..100
uint8_t AllocationSource(const Ship &s, SystemId id);  // ALLOC_PLAYER, ALLOC_DELEGATE, ALLOC_AUTO, ALLOC_UNSET
// The provenance as the console says it: "the player", "an officer (Name)", "automatic mode", "unset".
std::string AllocationProvenance(const Ship &s, SystemId id);

// Committed against available (docs/power-assignment.md). `PowerCommitted` is what the commitments
// ask the plant for; `PowerShortfall` is the number the console reports when they exceed it, and it
// is never resolved by shedding.
int PowerCommitted(const Ship &s);
int PowerShortfall(const Ship &s);

void SetPowerAuto(Ship &s, bool on);
bool PowerAuto(const Ship &s);

// The chief engineer's recommendation (docs/power-assignment.md, Task C). `RecommendedAllocation`
// is the ladder's answer at the current plant, and the reasoning is his. The player accepts or
// refuses; a refusal is recorded and the officer remembers it.
struct Recommendation {
	int by = -1;                  // the officer who recommends
	int percent[SYS_COUNT];       // the allocation he would set, 0..100
	bool pending = false;         // the player has not answered yet
	std::string reasoning;        // his reasoning, in words, for the console
};
Recommendation RecommendAllocation(Ship &s);
bool AcceptRecommendation(Ship &s); // the chief's allocation becomes the player's, in full
bool RefuseRecommendation(Ship &s); // recorded, and the officer takes a mark against whoever refused

// A band delegation (Task C): grantable, held by a name, and revocable immediately.
bool GrantBand(Ship &s, int grantor, int grantee, BudgetBand band);
bool RevokeBand(Ship &s, int grantee, BudgetBand band);
const BandGrant *BandHolder(const Ship &s, BudgetBand band);
int BandGrantCount(const Ship &s);
void SetSourceOnline(Ship &s, SourceId id, bool on);
void DamageSystem(Ship &s, SystemId id, float amount);
void DamageSource(Ship &s, SourceId id, float amount);
void BreachDeck(Ship &s, int deck, float amount);   // hull damage; atmosphere then vents by itself
void Repair(Ship &s, SystemId id, float amount);
void RepairDeck(Ship &s, int deck, float amount);   // seal the hull; life support then refills the deck
void SetForceField(Ship &s, int deck, bool on);     // hold a breached deck's air with a field (level 10)
void SetForceFieldLevel(Ship &s, int deck, int level); // the field's rating 1..10 (0 = none): 10 cuts a drone from the Collective

// Endurance clocks (the air-and-endurance gap): the numbers a compartment or the whole ship is running
// on. MinutesOfAir is the time until a deck's atmosphere reaches AIRLESS, or -1 if it is holding or
// rising. MinutesToDark is the time until the first supplying source runs out -- the ship goes dark --
// or -1 if nothing is supplying.
float MinutesOfAir(const Ship &s, int deck);
float MinutesToDark(const Ship &s);
// The endurance of one source at its current draw, in minutes: the batteries' charge, or a reactor's
// fuel. -1 if that source is not supplying. The gap's contract names battery and auxiliary endurance
// separately, so each is its own number; MinutesToDark is the first of them to fail.
float EnduranceOf(const Ship &s, SourceId id);

// Per-person gravity (the environment in the world). The deck's `gravity` (0 freefall .. 1 standard)
// is how much of the world's gravity a person on it feels: the plating is life support's, sited on
// deck 12. ScaleGravity is that arithmetic, kept here so it can be unit-tested apart from the engine;
// at scale 1 the world's own value stands (which is what the engine's SVF_CUSTOM_GRAVITY flag is for),
// and at 0 there is nothing to hold a body down.
float GravityScale(const Ship &s, int deck);          // 1.0 for a deck we do not track
int ScaleGravity(int worldGravity, float scale);      // the gravity one person feels, rounded, clamped [0, world]

// The surgical bay's force field (the triage gap): raised, it holds the gravest casualty steady even
// with no medical supplies -- the one case that would otherwise be lost while the others wait.
bool SetSurgicalField(Ship &s, bool on);
bool SurgicalField(const Ship &s);

// ---- damage control and casualties (S6) -------------------------------------------------------------
//
// Nothing repairs itself. Engineers on duty who stand no station are the damage-control party: each
// tick they go, up to REPAIR_TEAM_MAX to a system, to whatever is damaged, most critical first, and
// what they restore is paid for in spare parts. With no parts, or no engineers, damage stays.
// A deck without air injures whoever is on it within a minute and kills within five. The injured
// stand no watch; sickbay returns them to duty at a rate set by its own output, a few at a time.

const int REPAIR_TEAM_MAX = 3;
const float REPAIR_HOURS_PER_SYSTEM = 6.0f;  // one engineer rebuilding a destroyed system [inv]
const float PARTS_PER_SYSTEM = 12.0f;        // spare parts to rebuild one from nothing [inv]
const float MAINTENANCE_DAYS = 10.0f;        // a system wholly left unmaintained fails in this [inv]
const float EXPOSURE_INJURES = 60.0f;        // seconds without air [inv]
const float EXPOSURE_KILLS = 300.0f;
const float RADIATION_INJURES = 60.0f;       // seconds in a failing core's radiation at half health [inv]
const float RADIATION_KILLS = 300.0f;
const float AIRLESS = 0.25f;                 // a deck's atmosphere below this cannot be breathed [inv]
const int SICKBAY_BEDS = 4;                  // three standard and one surgical [lore]; the worst get them
const float TREATMENT_HOURS = 12.0f;         // per patient, with sickbay at full output [inv]
const float DETERIORATE_PER_HOUR = 0.03f;    // an untreated injury worsens; at severity 1 they die [inv]
const float MEDICAL_PER_PATIENT_HOUR = 0.4f; // supplies spent treating one patient for an hour [inv]
const int SICKBAY_DECK = 5;
const float SEAL_HOURS_PER_DECK = 4.0f;      // one engineer sealing a breached deck's hull [inv]
const float PARTS_PER_DECK_SEAL = 8.0f;      // spare parts to seal one deck's breach [inv]
const float FIRE_INJURES = 30.0f;            // seconds in a full fire [inv]
const float FIRE_KILLS = 180.0f;             // ... and until it kills [inv]
const float FIRE_SPREAD_PER_HOUR = 0.15f;    // fire spreads to the decks beside it at this rate [inv]
const float FIRE_FIGHT_MINUTES = 20.0f;      // one crew member reduces a deck's fire in this [inv]
const float RATIONS_PER_CREW_DAY = 1.0f;     // days of food one crew member eats in a day [inv]

// A fire can start on a deck (combat, a damaged system, or deliberately). It injures the crew on the
// deck and spreads to the decks beside it until the crew fight it down; it also costs the hull.
void IgniteDeck(Ship &s, int deck, float amount);

// ---- intruders and control of the ship's systems (S7) ----------------------------------------------
//
// Boarders arrive on a deck. Those on a deck with a system's station work at taking it: its control
// falls, and below HIJACKED the system no longer answers to the crew -- it delivers nothing to the
// ship and refuses the consoles -- until it is won back. Control is won back by the crew at the
// station (slowly, by itself), by a successful counter-hack (CounterHack, fed by the breach puzzle),
// or by there being nobody left to hold it. Cutting a system's power stops both sides.
// Security crew on duty who stand no station go to where the boarders are and fight. Boarders who
// find nothing to take on their deck move on toward the bridge or Main Engineering.

const float HIJACKED = 0.5f;
const float HACK_MINUTES = 10.0f;        // one boarder, unopposed, takes a system from the crew in this [inv]
const float RETAKE_MINUTES = 20.0f;      // a full station crew wins an uncontested system back in this [inv]
const float FIGHT_MINUTES = 4.0f;        // one defender accounts for one boarder in this, and the reverse [inv]
const float ADVANCE_MINUTES = 15.0f;     // boarders with nothing to take move a deck in this [inv]
const int BRIDGE_DECK = 1;
const int ENGINEERING_DECK = 11;

void Board(Ship &s, int deck, int boarders);
// A boarding party of a named kind, with an objective deck (0 = the nearest of the bridge or
// Engineering). Raiders loot when they hold a deck with nothing left to take; Borg assimilate;
// hunters come for the crew.
enum BoarderKind : uint8_t { BOARDER_RAIDER = 0, BOARDER_BORG, BOARDER_HUNTER, BOARDER_KIND_COUNT };
const char *BoarderKindName(BoarderKind k);
void BoardAs(Ship &s, int deck, int boarders, BoarderKind kind, int objective);
// A counter-hack at a console: `strength` 0..1 is how well the operator did (BreachScore).
void CounterHack(Ship &s, SystemId id, float strength);
bool Hijacked(const Ship &s, SystemId id);
int Intruders(const Ship &s);            // aboard, all decks, rounded up

// ---- the Borg (S8) ------------------------------------------------------------------------------
//
// Drones are boarders who do not leave things as they found them. Left on a deck, they convert it:
// `assimilated` rises, and past ASSIMILATED the deck's systems are theirs outright -- no console and
// no counter-hack reaches them. They take the crew they find there, and each one taken is another
// drone. Driving them off does not undo it: the deck stays Borg until engineers strip it, which
// takes hours and spare parts, and only then do its systems answer again.

const float ASSIMILATED = 0.5f;
const float ASSIMILATE_DECK_HOURS = 1.0f;    // one unopposed drone converts a whole deck in this [inv]
const float ASSIMILATE_CREW_MINUTES = 10.0f; // one unopposed drone takes one crew member in this [inv]
const float STRIP_HOURS_PER_DECK = 8.0f;     // one engineer strips a wholly assimilated deck in this [inv]
const float PARTS_PER_DECK = 20.0f;          // spare parts to rebuild a wholly assimilated deck [inv]
const int STRIP_TEAM_MAX = 4;
// De-assimilation (docs/borg-incursion.md): reversible in a narrow window, never complete. The cost
// and the lasting residue rise with how far the assimilation got; past RECOVERY_LIMIT it is too late.
const float RECOVERY_LIMIT = 0.8f;           // wounds above this: the nanoprobes have won [inv]
const float RECOVERY_SUPPLIES = 40.0f;       // medical supplies a full recovery costs, scaled by progress [inv]
const float RECOVERY_MATERIALS = 20.0f;      // material the same [inv]
const float SCAR_DRAG = 0.25f;               // a full scar drags the morale target by this [inv]

void BoardBorg(Ship &s, int deck, int drones);
bool DeckAssimilated(const Ship &s, int deck);
// Recover someone the Borg have begun to assimilate (wounds in (0, RECOVERY_LIMIT)): sickbay and
// supplies against the nanoprobes. Returns false if they are not in the window or it cannot be paid.
bool RecoverCaptive(Ship &s, int crew);

// The breach puzzle. A square grid of two-character codes and a set of target sequences. The
// operator picks cells alternately along a row and then a column, starting in the top row, without
// reusing a cell, up to `buffer` picks; every target sequence that appears in the picks, in order
// and unbroken, counts. Deterministic for a seed, so the screen and a test see the same puzzle.
struct Breach {
	int size = 5;
	int buffer = 7;
	std::vector<std::string> grid;                   // size * size codes, row by row
	std::vector<std::vector<std::string>> targets;   // each a sequence of codes; later ones are longer and worth more
};
Breach MakeBreach(uint32_t seed);
// `picks` are cell indices (row * size + column). Returns 0 for an illegal path, otherwise the
// fraction of the total target value achieved, 0..1.
float BreachScore(const Breach &b, const std::vector<int> &picks);

// Jump to a linked beacon. Refused (false) if it is not one jump away, the warp drive is delivering
// less than half its output, or there is no fuel -- and a ship cannot jump out of a fight it cannot
// outrun: the same test, so running needs a working drive.
// The away-team kit: load a party's kit out of the ship's stores, and scan a site with the tricorders.
// Scan returns 0 if there is no tricorder or no charge (and says so in the log), 1 for a clean reading,
// 2 for a reading a weak charge has made suspect.
void LoadAwayKit(Ship &s, int tricorders, int phasers, int evSuits, float charge);
int Scan(Ship &s, int beacon);
// A tricorder reading of the ship's own compartments, at human scale -- the same kind of reading the
// sensors make of a site, over a smaller radius. Returns a one-line reading of that deck's air, hull
// and life support, and writes it to the log. Wears the kit's condition, and a worn kit reads less.
std::string ScanCompartment(Ship &s, int deck);

bool Jump(Ship &s, int toBeacon);
// One torpedo at the enemy. False if there is no enemy, none left, or the launchers are not delivering.
bool FireTorpedo(Ship &s);
bool InCombat(const Ship &s);
// The counter-play kit (docs/borg-incursion.md): the phaser adapter's rotating modulation, and a
// vinculum raid. Each buys back the crew's weapons for a while, at a cost.
bool Remodulate(Ship &s);
bool RaidVinculum(Ship &s);

// Shuttles (docs/shuttles.md): supported, not pilotable. The ship always knows whether a given one
// is in the bay; losing one is permanent and becomes a build job.
const char *ShuttleClassName(ShuttleClass c);
const char *ShuttleLocationName(ShuttleLocation l);
int ShuttlesInBay(const Ship &s);            // the lifeboat arithmetic
int ShuttlesAway(const Ship &s);
Shuttle *ShuttleByClass(Ship &s, ShuttleClass c);
const Shuttle *ShuttleByClass(const Ship &s, ShuttleClass c);
bool LaunchShuttle(Ship &s, ShuttleClass c, int beacon, const std::vector<int> &manifest); // the load screen's commit
bool RecallShuttle(Ship &s, ShuttleClass c);  // it comes home; the crew are aboard again
bool StrandShuttle(Ship &s, ShuttleClass c);  // the crew beam back and the shuttle is left behind (a loss)
bool LoseShuttle(Ship &s, ShuttleClass c);    // crashed, captured, destroyed: the crew aboard are lost
bool ShuttleBayHit(Ship &s, float severity);  // a hit on the bay damages or wrecks what is parked
bool RebuildShuttle(Ship &s, ShuttleClass c); // order the second bay to build a replacement (a build job)

// The counter-play kit (docs/borg-incursion.md). Adaptation cannot be absolute: the crew can rotate
// the phaser modulation (at a cost in attention) and raid a vinculum to buy back their weapons.
const float REMODULATE_ADAPTATION = 0.5f;  // adaptation broken by one rotation [inv]
const float REMODULATE_COOLDOWN = 120.0f;  // ship-seconds before another rotation [inv]
const float VINCULUM_SUPPRESS = 300.0f;    // ship-seconds the Borg cannot adapt after a vinculum raid [inv]

// The dilithium constraint (docs/exploration-and-science.md): the ratchet that makes exploration the
// way home. Warp use spends the crystal; Engineering recomposites what it can; eventually it cannot
// and the ship must find a new crystal -- by mine, trade, salvage or research -- or home stops
// getting closer. No crystal, no warp; the ship still runs sublight.
enum DilithiumWay : uint8_t { DIL_MINE = 0, DIL_TRADE, DIL_SALVAGE, DIL_RESEARCH, DIL_WAY_COUNT };
bool WarpPossible(const Ship &s);            // a crystal left, the pylons up and the warp drive delivering
bool Recomposite(Ship &s);                   // Engineering buys back life in the crystal's frame
bool AcquireDilithium(Ship &s, DilithiumWay way); // at a source: mine a belt, trade, salvage, research
int  DilithiumRange(const Ship &s);          // light-years of progress the crystal can still buy (the scoreboard)
const char *DilithiumWayName(DilithiumWay way);

// The warp core cascade (docs/failure-is-content.md): the breach is reachable only by a chain, and
// each arrow is an interrupt. Coolant loss -> overheat -> falling containment -> a breach countdown.
const float COOLANT_LOSS_MINUTES = 20.0f;   // a failing core loses its coolant in this [inv]
const float CONTAINMENT_MINUTES = 10.0f;    // at full overheat, containment falls to critical in this [inv]
const float BREACH_SECONDS = 180.0f;        // the countdown once containment is critical [inv]
const float CONTAINMENT_CRITICAL = 0.1f;
const float COOLANT_MATERIALS = 15.0f;      // material to refill the coolant loops [inv]
// A dedicated survey to locate a dilithium source: the design's "she must locate a source, which
// means survey, chart, detour" (docs/exploration-and-science.md). Charts the nearest belt, trader or
// derelict reachable through the sector, so the crew know where to go before the crystal runs out.
int LocateDilithium(Ship &s);    // the beacon charted, or -1

bool ShutDownCore(Ship &s);      // stop the cascade and give up warp until it is started again
bool RestartCore(Ship &s);
bool EjectCore(Ship &s);         // the canon last resort: no warp until a new core is found
bool RestoreCoolant(Ship &s);    // Engineering refills the coolant loops, at a cost in material
bool CoreBreached(const Ship &s);// the one unwinnable end: the ship is gone

// The coreless ship (docs/budget-squaring.md, Part 4a). With the warp core gone -- destroyed,
// offline, shut down or ejected -- the fusion reactors and the batteries are all that remain, and
// the ship runs only the set that keeps her alive and moving: the critical four, the impulse drive
// and the navigational deflector. Nothing else runs at all -- no weapons, no sensors, no comms --
// because there is not the power for it. It is a survival allocation, not a shedding choice. [our
// call: the doc's Part 4a sets the arithmetic; the mechanism that forces it is ours.]
bool Coreless(const Ship &s);

// The holodeck matrix is a trap, not a solution (docs/ship-systems.md; VOY "Parallax"). When the
// main grid is down, jump-starting from a holodeck reactor is tempting -- the holodecks are a canon
// independent source -- and it ruins the ship: the matrices are incompatible, and the cross-tie
// blows relays across her. It restores some power now and writes lasting damage, so it is worse than
// the problem it solves. A designed failure mode, and the crew are warned in the log. [lore: the
// incompatible matrix; the relay damage is canon's "blew half the ship's relays"]
bool JumpStartFromHolodeck(Ship &s);

// The materials economy (the backlog's first item): salvage is the door to it. The tractor beam holds
// a derelict to strip it fully, or locks a contact so it cannot run; raw material becomes spare parts
// at the replicators. Travel is at the mercy of the structural integrity, the navigational deflector
// and the inertial dampers -- safe travel needs all three delivering, and a weak drive still jumps
// but costs a hull breach or a shaken crew.
const float SALVAGE_MATERIALS = 20.0f;   // raw material recovered from a derelict [inv]
const float TRACTOR_MIN = 0.5f;          // output the tractor and travel systems need to be doing their job [inv]
bool TractorWreck(Ship &s);              // salvage the derelict here, fully, with the tractor
bool TractorHold(Ship &s);               // lock the contact: it cannot break off while held
// A probe (docs/exploration-and-science.md): the safe way to look at something hostile -- launch one
// instead of the ship. It consumes a probe, and its telemetry charts the target or is lost.
bool LaunchProbe(Ship &s, int beacon);

// A phenomenon (docs/exploration-and-science.md): reveal its hidden attributes one scan at a time,
// then respond. The right response depends on the set revealed; the wrong one is damage.
const int PHENOM_ATTR_COUNT = 3;         // attributes to reveal
const int PHENOM_RESPONSE_COUNT = 4;     // shield harmonics, warp geometry, distance, do not touch
const float PHENOM_MATERIALS = 30.0f;    // the science reward for a correct response [inv]
int RevealPhenomenon(Ship &s);           // one scan: reveal the next attribute; -1 if none here
bool RespondPhenomenon(Ship &s, int response); // true if it was the correct response
const char *PhenomenonResponseName(int response);
bool Held(const Ship &s);
bool FabricateParts(Ship &s, int parts); // with the replicators running, material becomes spare parts
bool FabricateRations(Ship &s, int days); // the galley: the replicators turn material into food
const float BELT_MATERIALS = 30.0f;       // raw material mined from a belt [inv]
const float BELT_DEUTERIUM = 0.08f;       // deuterium siphoned from a gas belt [inv]
bool MineBelt(Ship &s);                   // resource acquisition at a belt: mining and siphoning
bool EVA(Ship &s);                        // a suited party reaches what the tractor cannot
bool TakeSurvivors(Ship &s, int n);       // population pressure: take survivors or refugees aboard
int Refugees(const Ship &s);
bool Hearing(Ship &s, int crew, bool guilty); // justice: a hearing releases or confirms a confinement
// First contact with a pre-warp civilisation: observe it, or interfere and own the consequence.
bool ObservePreWarp(Ship &s);
bool InterferePreWarp(Ship &s);
// The Maquis split as an arc: resentment between the two, and what reconciles them.
float Resentment(const Ship &s);
bool ReconcileFactions(Ship &s);
// Borg strategic awareness: how much the Collective knows of the ship, and what a crew member's
// memory of command does to their work.
float BorgAwareness(const Ship &s);
float Loyalty(const Ship &s, int crew); // remembered valence toward whoever commands (-1 .. +1)
bool ActivateEMH(Ship &s, bool on);      // the Emergency Medical Hologram (needs the computer core)
bool EMHActive(const Ship &s);
bool TravelSafe(const Ship &s);          // integrity, deflector and dampers all delivering enough
void DamagePylon(Ship &s, float amount);// the nacelle pylons, without which there is no warp
bool PylonsIntact(const Ship &s);
bool SetMobileEmitter(Ship &s, bool on); // the EMH's mobile emitter, an artifact
bool MobileEmitter(const Ship &s);
bool EndHolodeckProgram(Ship &s, int crew); // pull a crew member out of the program that will not end
const float AIRPONICS_PER_HOUR = 0.5f;   // days of food the airponics bay grows in an hour [inv]
bool SetAirponics(Ship &s, bool on);     // the airponics bay, growing food without the replicators
bool Airponics(const Ship &s);

// An opponent's systems (S9): Tactical picks what to aim at, and its weapons, engines and shield
// generator are things to break in their own right.
void SetTarget(Ship &s, EnemySubsystem t);
EnemySubsystem Target(const Ship &s);
const char *EnemySubsystemName(EnemySubsystem t);
const char *EnemyKindName(EnemyKind k);

// The phaser bank's setting (Tactical's standing decision, and the one that has an effect with no
// contact: it changes what the bank asks of the power budget and what it does to a hull). False,
// changing nothing, if the setting is not one of the four.
bool SetPhaserYield(Ship &s, int y);
uint8_t PhaserYieldOf(const Ship &s);
// The demand a system asks of the power budget this tick, before distribution. For the phasers it
// includes the setting: a vaporize setting asks more of the same bank than a stun one. Every other
// system's demand is its spec's, unchanged.
int EffectiveDemand(const Ship &s, SystemId id);

// Choices at a beacon (S9): hail, trade, answer a distress call, or run. Each returns false, changing
// nothing, if it does not apply here.
bool Hail(Ship &s);
bool Trade(Ship &s);
bool AnswerDistress(Ship &s);
bool Disengage(Ship &s);

// Pursuit (S9): a raider whose engines survived follows the ship, and repairs suffer while it does.
bool Pursued(const Ship &s);
int PursuitJumps(const Ship &s);

// More sectors, and an end to reach (S9): reaching the last beacon marks the end; crossing it opens
// the next sector, and the third crossed is home. Returns true if a sector was crossed.
bool AtEnd(const Ship &s);
bool Won(const Ship &s);
int SectorNumber(const Ship &s);
bool AdvanceSector(Ship &s);

// ---- the stations' purposes (S4) ------------------------------------------------------------------
//
// Each console does more than switch its systems on and off: the transporter beams a party to a site,
// astrometrics makes a survey, the Conn lays in a course, sickbay reads its ward. These are the core
// functions behind those controls; the console wiring and the screens call them.

// The transporter: beam a party of fit crew to the site the ship is at, and bring them back. Beaming
// needs the transporters delivering and the shields down -- a transporter cannot reach through our
// own shields -- and a party already away is brought back first. While away the party stands no
// watch and is on no deck. Returns false, changing nothing, if the beam cannot be made.
//
// The transporter is the system the ruling was written about, and it is worked end to end here. The
// pattern buffer is its condition; a beam inside the top tenth is nominal and mangles no one. Below
// that the odds rise, and the load at the moment of the beam sets the severity: a misaligned beam
// (degraded), a mangled arrival (acute), and at the worst a copy or a merge (catastrophic) -- each
// of which writes to the crew records, and none of which is a reset button, since a copy is not the
// person. `outcome`, when given, is set to the anomaly's account, or empty for a clean beam.
const float TRANSPORTER_ADVISE = 0.4f; // below this condition the console says: do not send anyone
bool TransportAway(Ship &s, int party, std::string *outcome = nullptr);
bool TransportBack(Ship &s, std::string *outcome = nullptr);
int AwayTeam(const Ship &s);                 // how many are off the ship now
// The instrument: an honest reading of the transporter's condition, stated before the act. One line,
// of the shape "the pattern buffer is at 62%, and below 40 I would not send anyone". It is a read of
// state and cannot lie (docs/the-record-and-the-log.md).
std::string TransporterConditionLine(const Ship &s);

// Astrometrics: a sensor survey of the beacons one jump away -- what the chart is built from. Needs
// the sensors delivering. Returns how many readings it added, and logs the survey.
int Survey(Ship &s);

// The Conn's course: the shortest route from here to a beacon (BFS over the links; the first entry is
// where the ship is now), plotting it, and reading it back. An empty route means no way there.
std::vector<int> PlotCourse(const Ship &s, int toBeacon);
bool SetCourse(Ship &s, int toBeacon);
int Course(const Ship &s);                   // the beacon the conn is making for, or -1

// Sickbay's ward: the casualties, in the order the triage standing order treats them -- worst first
// by default, or rank first. This is what the triage screen draws, one row per casualty.
std::vector<int> Patients(const Ship &s);

// ---- modes, rank and the player (S10) -------------------------------------------------------------

// Ship seconds that pass per second played, for the configured clock.
float ClockRate(const Config &cfg);
// May the game be saved and loaded at will? Not in ironman: the ship is saved for you, and only forward.
bool SavesAllowed(const Config &cfg);
// The ship was away from the player for this long. Under the wall clock she lived through it -- at
// most MAX_CATCH_UP_DAYS of it, so a year's absence is not a year's simulation. Other clocks: nothing.
const float MAX_CATCH_UP_DAYS = 30.0f;
void CatchUp(Ship &s, double realSecondsAway);

// ---- the three clocks and the two exits (docs/ship-model.md) --------------------------------------
//
// Exit with a background process and the ship is left standing: the world keeps its own time, the run
// is marked as one that was left standing, and what happened while the player was away is continuous
// in the record. Exit without one and the simulation stops: no time existed, and the record has a gap.
// The guard is tied to the play mode already in the header -- holodeck may suspend the world, ironman
// may not, because ironman means the ship keeps her own time.
bool MaySuspend(const Config &cfg);
bool Suspend(Ship &s);              // false, changing nothing, in ironman
bool LeftStanding(const Ship &s);   // was this run ever left standing? (the record's mark)
// The sleep state: the player skips time at the accelerated rate -- incrementally, or all at once --
// and the ship advances as it would have. It is an act, not a rate, so it works whatever the
// configured clock; long steps are cut up inside, so a sleep arrives where a played run would.
void Sleep(Ship &s, double shipSeconds);

// Clearance. A crew member operates the station their department works: Engineering for engineers,
// Tactical for security, Operations and the Conn for command and sciences, Sickbay for medical. A
// lieutenant commander or above may operate any. The alert is called by a department head or above
// (lieutenant, rank 3) at Engineering or Tactical. Only the captain and first officer command.
bool MayOperate(const CrewMember &who, Station st);
bool MayCallAlert(const CrewMember &who, Station st);
bool MayCommand(const CrewMember &who);
// The same questions, with the delegation in view: a station's access may have been granted for a
// shift (docs/access-and-authority.md, source 3), which the record alone cannot answer.
bool MayOperate(const Ship &s, int crew, Station st);
bool MayCallAlert(const Ship &s, int crew, Station st);

// ---- delegation and revocation (docs/access-and-authority.md) ------------------------------------
//
// Source 3 of access: a department head grants a junior their section's access for a shift, and can
// take it back. The grant is temporary and it names both hands -- who gave it and, when it is
// revoked, who turned it off -- because the record is the point. Access stays technical and binary:
// nothing here consults morale. It is the department head's authority, not command's, that is spent.
// The Delegation, Override and Lockout structs are defined above, before Ship.
const int DELEGATION_MAX = 24;
const float DELEGATION_SHIFT_HOURS = 8.0f; // a watch [inv]
bool Delegate(Ship &s, int grantor, int grantee, Station st);
bool RevokeDelegation(Ship &s, int revoker, int grantee, Station st);
bool DelegatedTo(const Ship &s, int crew, Station st);
const std::vector<Delegation> &Delegations(const Ship &s);
// The crew can revoke a credential too: a qualification once earned is not held forever.
bool RevokeCredential(Ship &s, int revoker, int holder, Station st);

// ---- emergency override (docs/access-and-authority.md, source 4) ---------------------------------
//
// The fallback for a skeleton crew: slow, loud, logged, and usually needing two people to agree. One
// person may force it alone, but it takes longer and the log says so. It grants technical access to
// one station for a while; it does not change who commands, and it does not clear a compromised
// system -- hardware beats hierarchy, so a hijacked system refuses the override as it refuses rank.
const float OVERRIDE_TWO_MINUTES = 15.0f;      // two officers agree: until it takes [inv]
const float OVERRIDE_SOLO_MINUTES = 45.0f;     // one person forces it: slower still [inv]
const float OVERRIDE_DURATION_MINUTES = 20.0f; // how long the forced access holds [inv]
bool BeginOverride(Ship &s, int requester, Station st);
bool ConfirmOverride(Ship &s, int second, Station st);
bool OverrideActive(const Ship &s, Station st);
const Override &OverrideState(const Ship &s);

// ---- remote call-up and the lock-out (owner decision, 2026-10-07) ---------------------------------
//
// An officer above the post-holder may call up any system's controls from any console, command it
// remotely, and lock the post-holder out. Command travels; physical work does not -- a repair, a
// seal, a hatch or a valve still needs a hand standing there (the crew's own jobs, unchanged). The
// resolution of the conflict in docs/access-and-authority.md: full access is no longer tied to
// standing beside the system, but the work still is. The lock-out is first-class: it is logged, it
// names its author, the console that has stopped answering names the person locked out, and it
// writes a negative saw-it-myself mark on that person's record (docs/memory-and-consequence.md).
const int LOCKOUT_MAX = 16;
bool LockOut(Ship &s, int officer, Station st, int locked);
bool ClearLockout(Ship &s, int officer, Station st, int locked);
bool LockedOut(const Ship &s, int crew, Station st);
const std::vector<Lockout> &Lockouts(const Ship &s);
// The lock-out as it reads on the console that has stopped answering: empty unless `crew` is locked
// out of `st`, otherwise it names both hands.
std::string LockoutNotice(const Ship &s, int crew, Station st);
// Command may travel to a console away from the system; the crew's physical jobs may not. True when
// this operator may call up this system from this console: it is operated there, or the operator is
// ship-wide (rank 4 or above, or the ship's role is command).
bool MayCallUp(const Ship &s, int crew, Station console, SystemId id);

// The named refusal: the reason a control is locked, naming who can open it. This is the string the
// consoles show, and the one the player learns the chain of command from.
std::string AccessRefusal(const Ship &s, Station st);
std::string OperatedFromRefusal(SystemId id);

// Training and credentials: a crew member earns a cross-qualification by training, and a credential
// lets them operate a station their department does not own. The player's career is the same shape.
bool Train(Ship &s, int crew, Station st);
bool Qualified(const CrewMember &who, Station st);
// Discipline and justice: the brig, a hearing, and release.
bool Brig(Ship &s, int crew, bool on);
bool Brigged(const Ship &s, int crew);
// Grief: a funeral, when there is time to hold one, lifts the crew who have been lost and are missed.
// It also metabolises the loss: the death marks soften and every fit attendee takes a positive
// MEM_FUNERAL mark toward whoever held it, so the crew stand together instead of carrying dread alone.
bool HoldFuneral(Ship &s);
// A death, by name and cause: the record is closed, command is notified, the quarters are sealed and
// whoever is on the deck remembers it. A scenario or a console uses this; the causes in play are the
// ship's own (exposure, fire, wounds), which the tick reaches by itself. False for a bad index.
bool KillCrew(Ship &s, int crew, const std::string &cause);
// The wall of names (docs/morale.md): the crew the ship has buried, in roster order. Recorded, never
// stored: it is read from the records, the same way the dead are. SealedQuarters names the bereaved
// quarters still shut (the dead's), with their deck, so the player can read what a closed door means.
std::vector<std::string> WallOfNames(const Ship &s);
struct SealedQuarter { int crew = -1; int deck = 0; };
std::vector<SealedQuarter> SealedQuarters(const Ship &s);
// The player's career: a promotion within the complement, given the trust and the rank.
bool Promote(Ship &s, int crew);
// The player's body: the state the player is in, as a crew record.
bool PlayerIncapacitated(const Ship &s);
bool PlayerDead(const Ship &s);
// The heart of the player-in-the-world bridge. The world tells the model which deck the player's
// body occupies (SetPlayerDeck), so a deck without air or a deck on fire reaches the person there;
// and it reads the state back as the health the body should show (PlayerBodyHealth: whole while fit,
// losing ground under an untreated injury, nothing when the record is closed). WoundPlayer is the
// other direction -- a boarder, a weapon or a hazard hurt the body, and the injury is written into
// the same record any other casualty carries.
void SetPlayerDeck(Ship &s, int deck);
int PlayerDeck(const Ship &s);
int PlayerBodyHealth(const Ship &s, int bodyHealth);
bool WoundPlayer(Ship &s, float amount, const std::string &cause);
// The ship's response to a downed player: a medical hand is sent, and the log names who came. This is
// the same shape as any other casualty -- they are carried and treated in the ward. Returns the crew
// member who attends, or -1 if none can.
int AttendIncapacitatedPlayer(Ship &s);
// Command devolves from a player who is dead or assimilated to the senior fit officer. This is the
// roster promoting to fill the gap, not a resurrection: the closed record is never restored. Returns
// the new player index, or -1. (The succession the world acts on when death is reached.)
int AssumeCommand(Ship &s);

// The senior fit officer, in rank order: the roster rule that fills the chair, used both by the
// configurator's vacancy derivation and by play's succession. Exposed so the two are one rule.
int SeniorFitOfficer(const Ship &s);

// Memory and consequence. A mark is written where it happens (Remember); command can tell the whole
// crew a thing (Brief); a query asks whether a character holds an event and how they came to (Recall,
// RecallSource), how many marks they hold (MemoryCount), and how they feel about a person (Bond).
void Remember(Ship &s, int crew, uint16_t event, int person, MemorySource source, float valence);
void Brief(Ship &s, uint16_t event, float valence);
bool Recall(const CrewMember &who, uint16_t event);
int RecallSource(const CrewMember &who, uint16_t event);
int MemoryCount(const CrewMember &who);
float Bond(const Ship &s, int a, int b);
// The unprocessed weight a character carries: their negative marks, still salient. It drags on their
// mood until therapy fades them. This is memory read by the simulation, not only by a query.
float Trauma(const CrewMember &who);

// A promise: an officer commits in front of a crew member; the mark is written naming the promiser,
// and the claim is held. ResolvePromise moves that mark's valence and the bond; a deadline that
// passes unresolved is broken. Returns the promise's index, or -1.
int MakePromise(Ship &s, int officer, int crew, PromiseKind kind, const std::string &what, double deadline = -1.0);
bool ResolvePromise(Ship &s, int index, bool kept);
const std::vector<Promise> &Promises(const Ship &s);

// ---- the navigation counter (docs/navigation-counter.md) ------------------------------------------
//
// The ship's scoreboard, and the document calls it the emotional centre of the design: how far home
// is, how long it will take, and how much that has changed since it was last written down. It is a
// **read**, not a system -- a pure projection over state already in the save. `docs/outside-the-ship.md`
// says the counter is a readout *of* the chart, not a separate thing, and so it is: the distance comes
// from the position model (the sectors still to cross, `sectorNumber`/`SECTORS_TO_CROSS`, and the
// beacons still to reach within the current one, `beacon`/`SECTOR_BEACONS`), and the time is that
// distance over the effective speed the ship can **actually sustain** -- the crystal's integrity and
// quality, the warp drive's output, the crew at the post, and whether any resupply is charted -- not
// an arithmetic quotient of a nominal speed. It never lies: `nominalYears` is what the journey costs
// if nothing changes, `currentYears` what it costs at the capability we have, and the gap between
// them is the honest difference, shown rather than hidden.
const float NAV_LIGHT_YEARS = 75000.0f; // the Delta Quadrant: canon's 75,000 light years home [lore]
const float NAV_NOMINAL_C = 1000.0f;    // canon's effective projected rate, on the order of 1,000 c [lore]
// The floors of the speed factors: a ship not at its best still goes, only worse. All four are [inv].
const float CRYSTAL_SPEED_FLOOR = 0.92f; // a crystal near the end of its life warps, but not as well
const float ENGINE_SPEED_FLOOR = 0.90f;  // a warp drive at half output still drives
const float CREW_SPEED_FLOOR = 0.95f;    // a station at half manning still runs, on automation
const float UNCHARTED_SUPPLY = 0.98f;    // no dilithium source charted: the route home is less certain
const float NAV_LOG_DAYS = 7.0f;         // the counter is written to the log this often [inv]

struct Navigation {
	float distanceLy = 0.0f;   // light years remaining, in the same unit as the goal
	float nominalYears = 0.0f; // the journey at nominal capability: what it costs if nothing changes
	float currentYears = 0.0f; // ... at the capability the ship can actually sustain now (-1: no warp)
	float changeYears = 0.0f;  // currentYears since the counter was last written down: the derivative
	float speedC = 0.0f;       // the effective speed the current capability sustains, in c
	bool warp = true;          // false: no warp at all, and home stops getting closer
};
// The read. Pure: the same ship gives the same counter.
Navigation NavigationCounter(const Ship &s);

// The forecasts -- what command sees that the crew do not (docs/navigation-counter.md): the estimate
// under each available course, one jump from here. Everyone sees where they are; command decides
// where to go, and that is the line the rank design draws.
struct NavCourse {
	int beacon = -1;             // a beacon one jump away
	float distanceLy = 0.0f;     // the distance remaining once there (a backward link is a detour)
	float years = 0.0f;          // the estimate from there, at current capability (-1: no warp)
	bool charted = false;        // has it been surveyed or visited?
	uint8_t kind = BEACON_EMPTY;
};
std::vector<NavCourse> NavigationForecasts(const Ship &s);

// The month report (docs/the-record-and-the-log.md). The simulation drafts it honestly from the
// record; the player edits it; the record keeps the diff; a purge empties the published logs and
// leaves the MEM_LOG-sourced marks orphaned, not erased.
void DraftReport(Ship &s, int department);  // draft the month report from the record
const MonthReport &OpenReport(const Ship &s);
bool StrikeReportLine(Ship &s, int line);   // strike a line from the published version
bool SoftenReportLine(Ship &s, int line, float factor = 0.5f); // scale a number down
bool EditReportLine(Ship &s, int line, const std::string &text);
bool AddReportLine(Ship &s, const std::string &scope, const std::string &text); // add a claim
bool SignReport(Ship &s, int signer, uint8_t audience); // publish; the lie is made here
std::string ReportDiff(const MonthReport &r);           // the diff, player-facing
const std::vector<MonthReport> &Reports(const Ship &s);
bool PurgeLogs(Ship &s);                    // defend against readers, never against assimilation
bool LogsPurged(const Ship &s);

// The holodeck's uses: recreation, training, therapy (fading trauma) and forensic reconstruction.
// All need the holodeck delivering. Console `ship holo <recreation|training|therapy|forensic> <crew>`.
enum HolodeckUse : uint8_t { HOLO_RECREATION = 0, HOLO_TRAINING, HOLO_THERAPY, HOLO_FORENSIC, HOLO_USE_COUNT };
bool RunHolodeck(Ship &s, HolodeckUse use, int crew);
// Living conditions: improve the crew's quarters, at a cost in material.
bool ImproveQuarters(Ship &s);
// The name that signs command's acts: the player's character if there is one, otherwise the captain
// or first officer, otherwise "command". The captain's log is written under this name.
std::string CommandingOfficer(const Ship &s);
// The senior fit officer of a department (the chief engineer for DEPT_ENGINEERING), preferring the
// one on duty, or -1 if the department has nobody fit. The person a recommendation comes from.
int DepartmentHead(const Ship &s, Department dept);
// The same questions for the player, whose role may widen or fix the answer.
bool PlayerMayOperate(const Ship &s, Station st);
bool PlayerMayCommand(const Ship &s);

// Character creation: the player takes the place of a generated crew member of that department --
// the complement does not grow -- with the name and rank chosen (rank 0..4: nobody is created a
// commander). Returns the roster index, or -1 if the name is empty or the rank out of range.
int CreateCharacter(Ship &s, const std::string &name, Department dept, int rank);
// Orders. Given by whoever commands; the crew carry them out until they are changed. They are the
// ship's standing orders, so they persist when the one who gave them walks away -- and in the save.
//   repair first   the damage-control party goes to this system before any other, however minor
//   security to    security with no station goes to this deck, boarders or not, before answering others
//   evacuate       nobody stays on this deck: stations there are left, off-duty crew go to the mess
// Each returns false, changing nothing, if the player may not command. -1 / 0 clears an order.
bool OrderRepairFirst(Ship &s, int system);
bool OrderSecurityTo(Ship &s, int deck);
bool OrderEvacuate(Ship &s, int deck);
bool OrderTriage(Ship &s, int policy);   // 0 worst first, 1 rank first

// Sets the role; ROLE_MUNRO makes the player Alexander Munro.
void SetRole(Ship &s, PlayerRole role);

// ---- persistence ------------------------------------------------------------------------------

const uint32_t SAVE_MAGIC = 0x50494853; // 'SHIP'
const uint16_t SAVE_VERSION = 54;  // 54: the configurator -- the command-seat occupancy and the career path; the roster may be fictitious entirely (no canon crew), and each person's rank is now stored so a derived promotion survives a save (docs/the-entry-point.md). 53: the character layer -- the three morale reads (deficit, outlook, holdings) and each person's bounded conditions, with their source, cure and visibility (O8, docs/character-derivation.md). The old single `float morale` is gone; the static half (species, skills, traits, drives) is derived from the seed and not stored; 52: the meeting -- the queued briefs and the schedule they fall on (docs/staff-meetings.md); 51: power allocation -- each system's share and who set it, automatic mode, the pending recommendation and the band grants (docs/power-assignment.md); 2: parts, exposure; 3: control, intruders; 4: the Borg; 5: the outside; 6: modes, the player; 7: orders; 8: morale; 9: severity, supplies, triage; 10: force fields; 11: the log; 12: the away kit; 13: the away mission, the course, surveys; 14: kit condition, the surgical field; 15: fire, rations; 16: materials, the EMH, looted wrecks, the tractor hold; 17: credentials, faction, the brig, Borg adaptation; 18: crew memories; 19: resource belts and refugees; 20: quarters quality; 21: pylons, the mobile emitter, holodeck compulsion; 22: pre-warp contact and Maquis resentment; 23: the airponics bay; 24: boarder kinds and objectives; 25: Borg strategic awareness; 26: sealed quarters; 27: a second contact; 28: the job queue; 29: build jobs; 30: dilithium; 31: shuttles; 32: incursion controller, compromise and the clean-intercept count; 33: the counter-play kit (remodulation cooldown, vinculum suppression); 34: de-assimilation (the lasting scar); 35: force-field rating; 36: probes; 37: phenomena and their revealed attributes; 38: the security squad's advance; 39: the warp core cascade; 40: each system's named failure state; 41: the written-off list (what the ship has given up); 42: the anomaly draw counter, and transporter copies beyond the complement; 43: the left-standing mark; 44: the month report and its diff, the promises held, the orphaned mark, and the purge; 45: the navigation counter -- navCounterLast is the estimated years at the last entry, and the report's counter and change are that estimate, not the fuel range; 46: per-deck gravity, the plating life support holds; 47: the personal log (docs/the-record-and-the-log.md) -- the private store, distinct from the official log; 48: delegations for a shift, and the emergency override (docs/access-and-authority.md); 49: the phaser bank's setting (Tactical's standing decision, there when there is no contact); 50: the five budget systems (astrometrics, science labs, gravity plating, non-essential lighting, cargo handling) raise SYS_COUNT, and the warp core's output now scales with the dilithium crystal's ceiling

std::vector<uint8_t> Pack(const Ship &s);
// False, leaving `s` untouched, on a truncated, foreign or newer record.
bool Unpack(const uint8_t *data, size_t len, Ship &s);

// Who is on a deck right now: indices into Ship::crew, in roster order. This is what decides which
// crew are embodied where the player is (S5); the dead and the assimilated are on no deck.
std::vector<int> CrewOnDeck(const Ship &s, int deck);

// A one-screen status report, for the console command and for tests' failure messages.
std::string Describe(const Ship &s);
// The captain's log as an authored, summarised artifact, distinct from the raw feed: the situation
// in the captain's words, generated from the ship's state. Written under CommandingOfficer's name.
std::string CaptainLog(const Ship &s);

} // namespace ship

#endif
