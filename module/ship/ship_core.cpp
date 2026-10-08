// ship_core.cpp -- see ship_core.h. No game headers in this file.
//
// Figures marked [lore] are sourced in docs/lore-ledger.md; figures marked [inv] are invented for
// play and listed there as such.

#include "ship_core.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace ship {

// ---- the tables -------------------------------------------------------------------------------

// Demand is in EPS units [inv]. Deck and station are [lore] where the ledger says so. Priority is
// the order power is kept in, and therefore the order it is shed in: the lowest number is fed first
// and given up last. It is the brownout ladder stated as arithmetic (docs/budget-squaring.md, Part
// six), and the five new systems are placed by it: the science and the comforts go first, the warp
// drive goes last of all, after even the comforts. The array is in enum order, one line per SystemId.
static const SystemSpec SPECS[SYS_COUNT] = {
	// name                     deck station                 department        demand prio crew
	{"life support",             12, "Environmental Control", DEPT_ENGINEERING,   60,   0,  1},
	{"structural integrity",     11, "Main Engineering",      DEPT_ENGINEERING,   80,   1,  1},
	{"inertial dampers",         11, "Main Engineering",      DEPT_ENGINEERING,   40,   2,  1},
	{"computer core",            10, "Computer Core",         DEPT_SCIENCES,      60,   3,  1}, // main core, deck 10 (docs/ship-master-map.md); auxiliary on deck 7
	{"shields",                   1, "Bridge, Tactical",      DEPT_SECURITY,     200,   7,  1},
	{"sensors",                   8, "Astrometrics",          DEPT_SCIENCES,      60,   6,  2},
	// The warp drive is shed last of all (docs/budget-squaring.md): the ship gives up her way home
	// only after she has given up everything else, or the priorities invert and she keeps the
	// holodecks lit at the cost of going anywhere.
	{"warp drive",               11, "Main Engineering",      DEPT_ENGINEERING,  400,   4,  3},
	{"impulse drive",            10, "Impulse Engineering",   DEPT_ENGINEERING,  100,   5,  2},
	{"phasers",                   1, "Bridge, Tactical",      DEPT_SECURITY,     150,   8,  2},
	{"torpedo launchers",        10, "Torpedo Bay",           DEPT_SECURITY,      30,   9,  2}, // fore tubes, deck 10; aft tubes deck 4 (docs/ship-master-map.md)
	{"navigational deflector",   11, "Deflector Control",     DEPT_ENGINEERING,   50,  11,  1},
	{"communications",            1, "Bridge, Operations",    DEPT_COMMAND,       20,  12,  1},
	{"transporters",              4, "Transporter Room 1",    DEPT_ENGINEERING,   60,  13,  1},
	{"sickbay",                   5, "Sickbay",               DEPT_MEDICAL,       30,  15,  2},
	{"turbolifts",                1, "Bridge, Operations",    DEPT_ENGINEERING,   20,  16,  0},
	{"tractor beam",             10, "Shuttlebay Control",    DEPT_ENGINEERING,   60,  17,  1},
	{"replicators",               2, "Mess Hall",             DEPT_ENGINEERING,   60,  19,  0},
	{"holodecks",                 6, "Holodeck 2",            DEPT_ENGINEERING,   60,  20,  0}, // deck 6 carries Holodeck 2; Holodeck 1 is on deck 14
	{"astrometrics",              8, "Astrometrics",          DEPT_SCIENCES,      60,  10,  0}, // canon's own facility, deck 8, with its own arrays
	{"science labs",              8, "Science Labs",          DEPT_SCIENCES,      60,  14,  0}, // the mission's own load: she is a science vessel
	{"gravity plating",          12, "Environmental Control", DEPT_ENGINEERING,   30,  18,  0}, // the plating draws power; life support's own deck
	{"non-essential lighting",   12, "Environmental Control", DEPT_ENGINEERING,   20,  21,  0}, // the first thing the show turns off, and the rung the ladder was missing
	{"cargo handling",           10, "Cargo Bay",             DEPT_ENGINEERING,   20,  22,  0}, // the ladder's other missing rung
};

const SystemSpec &Spec(SystemId id) { return SPECS[id < SYS_COUNT ? id : 0]; }

// ---- the bands of the budget (docs/power-assignment.md, Task C) -------------------------------
//
// A delegation names one of these bands and a holder. The band is a function of the system, so the
// console can say what a grant covers without a hand-written list.
const char *BandName(uint8_t band)
{
	static const char *const NAMES[BAND_COUNT] = {
		"the hull and the drive", "the tactical fit", "the science", "the ship's services", "the comforts"
	};
	return NAMES[band < BAND_COUNT ? band : 0];
}

BudgetBand BandOf(SystemId id)
{
	switch (id) {
	case SYS_WARP_DRIVE: case SYS_IMPULSE_DRIVE: case SYS_NAV_DEFLECTOR: case SYS_INERTIAL_DAMPERS:
	case SYS_STRUCTURAL_INTEGRITY:
		return BAND_PROPULSION;
	case SYS_SHIELDS: case SYS_PHASERS: case SYS_TORPEDO_LAUNCHERS: case SYS_TRACTOR_BEAM:
		return BAND_TACTICAL;
	case SYS_SENSORS: case SYS_ASTROMETRICS: case SYS_SCIENCE_LABS: case SYS_COMPUTER_CORE:
		return BAND_SCIENCE;
	case SYS_REPLICATORS: case SYS_HOLODECKS:
		return BAND_COMFORT;
	default:
		return BAND_OPERATIONS; // life support, communications, transporters, sickbay, turbolifts, gravity, lighting, cargo
	}
}

const char *AllocationByName(uint8_t by)
{
	static const char *const NAMES[ALLOC_BY_COUNT] = { "unset", "the player", "an officer", "automatic mode" };
	return NAMES[by < ALLOC_BY_COUNT ? by : 0];
}

// A decision a person made -- as opposed to a default or the ladder -- is one the automatic mode
// must never touch (docs/power-assignment.md: "Never apply the ladder when a person has decided").
static bool PersonSet(uint8_t by) { return by == ALLOC_PLAYER || by == ALLOC_DELEGATE; }

// The posts the ship must man: the systems' own needs summed. Canon's "The 37's" has her operable
// with 100 crew; this is the number that has to fit under that. [inv]
int PostsNeeded()
{
	int n = 0;
	for (int i = 0; i < SYS_COUNT; ++i) n += SPECS[i].crewNeeded;
	return n;
}

// The skeleton-crew arithmetic: one watch's hands are given to the posts in the order power is kept
// in, so the critical set (the four that keep her alive, the impulse drive and the deflector) is the
// last to go unmanned. At 100 crew a watch has 33 hands, and all 22 posts fit under it. [inv]
WatchCoverage CoverWithCrew(int crew)
{
	WatchCoverage c;
	c.perWatch = crew / WATCHES;
	c.postsNeeded = PostsNeeded();
	int order[SYS_COUNT];
	for (int i = 0; i < SYS_COUNT; ++i) order[i] = i;
	std::stable_sort(order, order + SYS_COUNT, [](int a, int b) { return SPECS[a].priority < SPECS[b].priority; });
	int hands = c.perWatch;
	bool critical = true;
	for (int k = 0; k < SYS_COUNT; ++k) {
		const int i = order[k];
		const bool key = i == SYS_LIFE_SUPPORT || i == SYS_STRUCTURAL_INTEGRITY || i == SYS_INERTIAL_DAMPERS
			|| i == SYS_COMPUTER_CORE || i == SYS_IMPULSE_DRIVE || i == SYS_NAV_DEFLECTOR;
		const int need = SPECS[i].crewNeeded;
		if (need == 0) continue;
		if (hands >= need) { hands -= need; c.postsCovered += need; }
		else if (key) critical = false;
	}
	c.critical = critical;
	return c;
}

// Who operates what. [lore] in outline -- tactical has weapons and shields, the conn flies the
// ship, ops runs her services -- and [inv] at the edges (see docs/lore-ledger.md).
Station StationOf(SystemId id)
{
	switch (id) {
	case SYS_SHIELDS: case SYS_PHASERS: case SYS_TORPEDO_LAUNCHERS: case SYS_TRACTOR_BEAM:
		return STN_TACTICAL;
	case SYS_WARP_DRIVE: case SYS_IMPULSE_DRIVE: case SYS_NAV_DEFLECTOR: case SYS_INERTIAL_DAMPERS:
		return STN_CONN;
	case SYS_SICKBAY:
		return STN_SICKBAY;
	default:
		return STN_OPS; // sensors, communications, transporters, life support, the computer, comforts
	}
}

const char *StationName(Station s)
{
	static const char *const NAMES[STN_COUNT] = {"MAIN ENGINEERING", "TACTICAL", "OPERATIONS", "CONN", "SICKBAY"};
	return NAMES[s < STN_COUNT ? s : 0];
}

bool OperatedFrom(SystemId id, Station s)
{
	return s == STN_ENGINEERING || StationOf(id) == s;
}

// Reading and operating are different privileges (owner ruling, 2026-10-07). A system is operated from
// exactly one station -- the S4 invariant above -- and a station may READ a system another station
// operates without being able to change it. This is the station's information portfolio, and it is
// wider than OperatedFrom. The console draws its own systems as controls and these as readouts.
//
// Only where the POST needs a second content type (owner ruling, 2026-10-07): Tactical needs the
// sensor picture and the comms traffic; Sickbay needs the medical record and what the Doctor is
// running. The other consoles carry one type, well, and get no read layer -- a junction panel with
// one readout stays a panel with one readout. Engineering is the ship-wide power distributor and
// already holds every system as a control, so it adds no read layer either.
bool StationReads(Station s, SystemId id)
{
	if (s == STN_ENGINEERING) return true;              // Engineering reads everything it distributes to
	if (StationOf(id) == s) return true;                // a station reads what it operates
	switch (s) {
	case STN_TACTICAL:                                  // the sensor picture, internal and external, and comms traffic
		return id == SYS_SENSORS || id == SYS_COMMUNICATIONS || id == SYS_COMPUTER_CORE;
	case STN_SICKBAY:                                   // life support, and the computer the Doctor runs on
		return id == SYS_LIFE_SUPPORT || id == SYS_COMPUTER_CORE;
	default:
		return false;
	}
}

struct SourceSpec {
	const char *name;
	int capacity;          // EPS units at full health [inv]
	float deuteriumPerDay; // fraction of tankage burned per ship-day at full output [inv]
	float antimatterPerDay;
};

static const SourceSpec SOURCES[SRC_COUNT] = {
	{"warp core", 1400, 0.004f, 0.003f},   // the plant (docs/budget-squaring.md, Parts 4a/5); its output also scales with the crystal's ceiling
	{"impulse reactors", 250, 0.003f, 0.0f}, // the secondary that flies her home
	{"auxiliary fusion", 90, 0.001f, 0.0f},  // keeps the spine alive after the core
	{"emergency batteries", 60, 0.0f, 0.0f}, // exactly life support, at full, for three hours
};

const float BATTERY_HOURS = 3.0f;          // full cells at full draw [inv]
const float REPLICATE_SUPPLIES_PER_HOUR = 2.5f; // medical supplies the replicators can make in an hour [inv]
const float ATMOSPHERE_REGEN_HOURS = 1.0f; // life support at full output refills a deck in this [inv]
const float ATMOSPHERE_STALE_HOURS = 12.0f; // with no life support a sealed deck lasts this [inv]
const float VENT_MINUTES = 5.0f;           // a deck fully open to space empties in this [inv]
const float GRAVITY_FAIL_HOURS = 6.0f;     // with no life support the plating loses hold in this [inv]
const float GRAVITY_REGEN_HOURS = 1.0f;    // ... and a supplied deck gets it back in this [inv]

static const int DEPT_SIZE[DEPT_COUNT] = {20, 50, 25, 30, 16}; // sums to COMPLEMENT [inv split]
static const int DEPT_DECK[DEPT_COUNT] = {1, 11, 4, 8, 5};     // where department duties are done
const int MESS_DECK = 2;
// The deck crew go to for recreation by default -- not "the holodeck". Holodeck 1 is on deck 14 and
// Holodeck 2 on deck 6, per docs/locations/deck14-stasis.brief.md; SYS_HOLODECKS is one system and
// this is the *recreational* deck the routine sends people to, which is deck 6, where the crew
// quarters are. Crew do not model walking to whichever holodeck is nearer: that is a design change,
// not a rename (see the deck-14 evidence, judgement call 6).
const int HOLODECK_RECREATION_DECK = 6;

static float Clamp01(float v) { return std::min(1.0f, std::max(0.0f, v)); }

static void NoteDeath(Ship &s, int deadIndex);      // who was there to see it (memory and consequence)
static void MemoryDecay(Ship &s, float shipSeconds); // salience fades unless reinforced
static void MaturePromises(Ship &s);                 // a deadline that passes unresolved is broken
static void UpdateJobs(Ship &s);                     // the queue of outstanding work (docs/crew-work.md)
static uint32_t AnomalyRoll(uint32_t counter, uint32_t seed); // a deterministic draw (the ruling)

// What the alert condition switches off, whatever its console says. Green: weapons and shields
// stand down. Red: comforts go dark.
static bool SuppressedByAlert(Alert a, SystemId id)
{
	if (a == ALERT_GREEN) return id == SYS_SHIELDS || id == SYS_PHASERS || id == SYS_TORPEDO_LAUNCHERS;
	if (a == ALERT_RED) return id == SYS_HOLODECKS || id == SYS_REPLICATORS;
	return false;
}

// ---- crew -------------------------------------------------------------------------------------

Activity ScheduledActivity(int watch, int secondOfDay)
{
	const int start = ((8 + 8 * watch) % 24) * 3600;
	const int t = ((secondOfDay - start) % SECONDS_PER_DAY + SECONDS_PER_DAY) % SECONDS_PER_DAY;
	const int h = t / 3600;
	if (h < 8) return ACT_ON_DUTY;
	if (h < 9) return ACT_MEAL;
	if (h < 12) return ACT_RECREATION;
	if (h < 15) return ACT_PERSONAL;
	if (h < 23) return ACT_SLEEP;
	return ACT_MEAL; // breakfast, the hour before the watch
}

// ---- the character layer (O8) --------------------------------------------------------------------
//
// The derivation, in one place (docs/character-derivation.md). Every table and every function
// below is a function of the record and the seed: the same seed produces the same person, and
// nothing here is a hand-written individual.

const char *SkillName(uint8_t s)
{
	static const char *const N[SKILL_COUNT] = {"engineering", "medical", "science", "security",
		"operations", "command", "flight"};
	return s < SKILL_COUNT ? N[s] : "skill";
}
uint8_t DepartmentSkill(Department d)
{
	switch (d) {
	case DEPT_ENGINEERING: return SKILL_ENGINEERING;
	case DEPT_MEDICAL: return SKILL_MEDICAL;
	case DEPT_SCIENCES: return SKILL_SCIENCE;
	case DEPT_SECURITY: return SKILL_SECURITY;
	default: return SKILL_COMMAND;
	}
}

const char *TraitName(uint8_t t)
{
	static const char *const N[TRAIT_COUNT] = {"steady under fire", "needs less sleep",
		"good with people", "claustrophobic", "poor with authority", "first-contact trained",
		"adaptable", "quick healer", "scrounger"};
	return t < TRAIT_COUNT ? N[t] : "trait";
}

const char *DesireName(uint8_t d)
{
	static const char *const N[DESIRE_COUNT] = {"promotion", "to go home", "a particular person",
		"to prove something", "to be left alone"};
	return d < DESIRE_COUNT ? N[d] : "desire";
}
const char *NeedName(uint8_t n)
{
	static const char *const N[NEED_COUNT] = {"sleep", "food", "company", "purpose", "medical care"};
	return n < NEED_COUNT ? N[n] : "need";
}
const char *FearName(uint8_t f)
{
	static const char *const N[FEAR_COUNT] = {"dying alone", "decompression", "the Borg",
		"being useless", "being seen as a coward"};
	return f < FEAR_COUNT ? N[f] : "fear";
}

const char *ConditionName(uint8_t id)
{
	static const char *const N[COND_ID_COUNT] = {"none", "exhausted", "hungry", "hypoxic",
		"irradiated", "infected", "concussed", "grieving", "afraid", "pon farr",
		"meditation cycle due", "emitter charge low", "a hot meal", "Neelix's coffee",
		"shore leave", "promoted", "a service held properly", "stimulant",
		"told the truth by someone in authority", "rested"};
	return id < COND_ID_COUNT ? N[id] : "condition";
}
const char *ConditionValenceName(uint8_t v) { return v == CVAL_BUFF ? "buff" : "debuff"; }
const char *ConditionClearName(uint8_t c)
{
	static const char *const N[CLEAR_COUNT] = {"rest", "sickbay", "a meal", "the end of the watch",
		"the passage of salience"};
	return c < CLEAR_COUNT ? N[c] : "clears";
}
const char *ConditionMagnitudeName(uint8_t m)
{
	static const char *const N[CMAG_COUNT] = {"slight", "clear", "sharp"};
	return m < CMAG_COUNT ? N[m] : "modified";
}

// Species: capabilities, needs and susceptibilities, and the conditions a species is prone to.
// There is deliberately no number here that makes one species better at a job than another: the
// two rules in docs/character-attributes.md are that every capability carries a cost, and that
// biology is not culture. `restNeedHours` is a *requirement* (the shape of rest), not a bonus.
static const SpeciesRecord SPECIES[SPECIES_COUNT] = {
	{ "Human",
	  {"none in particular: adaptability is a trait, not a species gift", nullptr, nullptr, nullptr},
	  {"sleep", "food", "air", "company"},
	  {"decompression", "radiation", "infection", "a long war"},
	  {COND_NONE, COND_NONE, COND_NONE, COND_NONE}, 8.0f, true, true },
	{ "Vulcan",
	  {"touch-telepathy", "the nerve pinch", "strength and endurance above the human norm", nullptr},
	  {"meditation in place of sleep", "a cool, dry berth",
	   "copper-based blood is a supply problem for sickbay", nullptr},
	  {"pon farr", "Bendii syndrome",
	   "emotions are managed rather than absent, and managing them takes practice", nullptr},
	  {COND_PON_FARR, COND_MEDITATION_DUE, COND_NONE, COND_NONE}, 4.0f, true, false },
	{ "Betazoid",
	  {"empathy: reads feeling, not thought", nullptr, nullptr, nullptr},
	  {"quiet: the faculty cannot be switched off", nullptr, nullptr, nullptr},
	  {"others' pain arrives uninvited, which is a debuff as often as a buff", nullptr, nullptr, nullptr},
	  {COND_AFRAID, COND_NONE, COND_NONE, COND_NONE}, 8.0f, true, true },
	{ "Klingon",
	  {"redundant physiology: injuries that would kill a human are survivable", nullptr, nullptr, nullptr},
	  {"a high-protein diet", "somewhere to work it off", nullptr, nullptr},
	  {"honour is culture, learned rather than biological, and may be rejected", nullptr, nullptr, nullptr},
	  {COND_NONE, COND_NONE, COND_NONE, COND_NONE}, 6.0f, true, true },
	{ "Ocampa",
	  {"limited telepathy", "a botanical gift", nullptr, nullptr},
	  {"a lifespan measured in single-digit years", nullptr, nullptr, nullptr},
	  {"a tragedy with a clock, which is the best attrition hook the ship has", nullptr, nullptr, nullptr},
	  {COND_NONE, COND_NONE, COND_NONE, COND_NONE}, 8.0f, true, true },
	{ "Talaxian",
	  {"temperament, cooking and scrounging: traits and skills, not species [ours]", nullptr, nullptr, nullptr},
	  {nullptr, nullptr, nullptr, nullptr},
	  {nullptr, nullptr, nullptr, nullptr},
	  {COND_NONE, COND_NONE, COND_NONE, COND_NONE}, 8.0f, true, true },
	{ "Bolian",
	  {"a distinct biology, and little else in canon: ours to fill, honestly [ours]", nullptr, nullptr, nullptr},
	  {nullptr, nullptr, nullptr, nullptr},
	  {nullptr, nullptr, nullptr, nullptr},
	  {COND_NONE, COND_NONE, COND_NONE, COND_NONE}, 8.0f, true, true },
	{ "Borg-recovered",
	  {"strength", "resistance", "regeneration in place of sleep", nullptr},
	  {"the alcove cycle", nullptr, nullptr, nullptr},
	  {"the Collective", "the crew's suspicion, which is social and every bit as mechanical", nullptr, nullptr},
	  {COND_NONE, COND_NONE, COND_NONE, COND_NONE}, 4.0f, true, false },
	{ "Hologram",
	  {"no fatigue, hunger or injury at all", nullptr, nullptr, nullptr},
	  {"emitter charge", "program integrity", nullptr, nullptr},
	  {"a different failure set entirely", "an authority that is situational by definition", nullptr, nullptr},
	  {COND_EMITTER_LOW, COND_NONE, COND_NONE, COND_NONE}, 0.0f, false, false },
};
static_assert(sizeof(SPECIES) / sizeof(SPECIES[0]) == SPECIES_COUNT, "species table size");

const SpeciesRecord &SpeciesOf(uint8_t sp) { return sp < SPECIES_COUNT ? SPECIES[sp] : SPECIES[SPECIES_HUMAN]; }
const char *SpeciesName(uint8_t sp) { return SpeciesOf(sp).name; }
const char *SpeciesCapability(uint8_t sp, int i)
{
	const SpeciesRecord &r = SpeciesOf(sp);
	return i >= 0 && i < SPECIES_TRAIT_MAX ? r.capabilities[i] : nullptr;
}
const char *SpeciesNeed(uint8_t sp, int i)
{
	const SpeciesRecord &r = SpeciesOf(sp);
	return i >= 0 && i < SPECIES_TRAIT_MAX ? r.needs[i] : nullptr;
}
const char *SpeciesSusceptibility(uint8_t sp, int i)
{
	const SpeciesRecord &r = SpeciesOf(sp);
	return i >= 0 && i < SPECIES_TRAIT_MAX ? r.susceptibilities[i] : nullptr;
}

// The deterministic draw the seed stream uses; one place, so a reordering is visible.
static uint32_t NextRandom(uint32_t &r) { r = r * 1664525u + 1013904223u; return r >> 8; }

bool HasTrait(const CrewMember &c, uint8_t trait)
{
	return trait < TRAIT_COUNT && (c.traits & (1u << trait)) != 0;
}

uint8_t DeriveSpecies(uint8_t dept, uint32_t &rng)
{
	(void)dept; // the mix is the ship's, not the department's
	const uint32_t n = NextRandom(rng) % 100u;
	if (n < 55u) return SPECIES_HUMAN;
	if (n < 68u) return SPECIES_BETAZOID;
	if (n < 80u) return SPECIES_VULCAN;
	if (n < 90u) return SPECIES_KLINGON;
	if (n < 94u) return SPECIES_TALAXIAN;
	if (n < 97u) return SPECIES_BOLIAN;
	if (n < 99u) return SPECIES_OCAMPA;
	return SPECIES_BORG_RECOVERED;
}

void DeriveCharacter(CrewMember &c, uint32_t &rng)
{
	// Skills: the department's own skill leads, rank lifts all of them, and the seed jitters the
	// rest. Species contributes nothing here -- a capability is a condition, never a multiplier.
	const uint8_t lead = DepartmentSkill(c.dept);
	for (int i = 0; i < SKILL_COUNT; ++i) {
		int v = 1 + (c.rank >= 4 ? 2 : c.rank >= 2 ? 1 : 0);
		if (i == lead) v += 2;
		v += static_cast<int>(NextRandom(rng) % 3u);
		c.skills[i] = static_cast<uint8_t>(std::min(SKILL_MAX, v));
	}

	// Traits: two, distinct, drawn from the seed.
	uint8_t t1 = static_cast<uint8_t>(NextRandom(rng) % TRAIT_COUNT);
	uint8_t t2 = static_cast<uint8_t>(NextRandom(rng) % TRAIT_COUNT);
	if (t2 == t1) t2 = static_cast<uint8_t>((t2 + 1u) % TRAIT_COUNT);
	c.traits = static_cast<uint16_t>((1u << t1) | (1u << t2));

	// Drives, in three parts.
	c.desire = static_cast<uint8_t>(NextRandom(rng) % DESIRE_COUNT);
	c.need = static_cast<uint8_t>(NextRandom(rng) % NEED_COUNT);
	c.fear = static_cast<uint8_t>(NextRandom(rng) % FEAR_COUNT);

	// State: a fresh crew begins rested and reasonably hopeful; the seed colours the outlook and
	// the holdings so two people do not start identical. Conditions arrive in play.
	c.fatigue = 0.0f;
	c.deficit = 0.0f;
	c.outlook = std::min(1.0f, 0.60f + (NextRandom(rng) % 5u) * 0.05f);
	c.holdings = std::min(1.0f, 0.60f + (NextRandom(rng) % 5u) * 0.05f);
	c.conditionCount = 0;
}

bool AddCondition(CrewMember &c, uint8_t id, const std::string &source, int8_t valence,
                  uint8_t magnitude, uint8_t clears, float now, uint8_t visible)
{
	if (id == COND_NONE || id >= COND_ID_COUNT) return false;
	if (visible == 0) return false;             // a hidden penalty is refused: it would feel like a bug
	if (source.empty()) return false;           // every condition names its cause
	if (FindCondition(c, id)) return false;     // already holds it
	std::string src = source;
	if (src.size() > static_cast<size_t>(CONDITION_SOURCE_MAX)) src.resize(CONDITION_SOURCE_MAX);
	if (c.conditionCount >= CONDITION_MAX) {
		// Two or three at a time, never a soup: evict the oldest debuff, or the oldest if all are buffs.
		int victim = -1;
		for (int i = 0; i < c.conditionCount; ++i)
			if (c.conditions[i].valence == CVAL_DEBUFF
			    && (victim < 0 || c.conditions[i].onset < c.conditions[victim].onset)) victim = i;
		if (victim < 0)
			for (int i = 0; i < c.conditionCount; ++i)
				if (victim < 0 || c.conditions[i].onset < c.conditions[victim].onset) victim = i;
		for (int i = victim; i + 1 < c.conditionCount; ++i) c.conditions[i] = c.conditions[i + 1];
		--c.conditionCount;
	}
	Condition &k = c.conditions[c.conditionCount++];
	k.id = id; k.source = src; k.valence = valence; k.magnitude = magnitude;
	k.onset = now; k.clears = clears; k.visible = visible;
	return true;
}

bool ClearCondition(CrewMember &c, uint8_t id)
{
	for (int i = 0; i < c.conditionCount; ++i) {
		if (c.conditions[i].id != id) continue;
		for (int j = i; j + 1 < c.conditionCount; ++j) c.conditions[j] = c.conditions[j + 1];
		--c.conditionCount;
		return true;
	}
	return false;
}

const Condition *FindCondition(const CrewMember &c, uint8_t id)
{
	for (int i = 0; i < c.conditionCount; ++i)
		if (c.conditions[i].id == id) return &c.conditions[i];
	return nullptr;
}

float Morale(const CrewMember &c)
{
	const float heart = 1.0f - c.deficit;
	const float m = 0.40f * heart + 0.35f * c.outlook + 0.25f * c.holdings;
	return std::min(1.0f, std::max(0.0f, m));
}

const char *MoraleBandName(float morale)
{
	if (morale >= 0.7f) return "fit";
	if (morale >= 0.5f) return "worn";
	if (morale >= 0.3f) return "strained";
	return "at breaking point";
}

const char *MoraleReason(const CrewMember &c)
{
	if (Morale(c) >= 0.70f) return "carrying on"; // no shortfall is doing the work
	const float dragDeficit = 0.40f * c.deficit;
	const float dragOutlook = 0.35f * (1.0f - c.outlook);
	const float dragHoldings = 0.25f * (1.0f - c.holdings);
	if (dragDeficit >= dragOutlook && dragDeficit >= dragHoldings)
		return c.fatigue > 0.5f ? "short on sleep" : "short of what they need";
	if (dragOutlook >= dragHoldings) return "does not believe the course is worth the cost";
	return "alone, or at odds with the crew";
}

float EffectiveSkill(const CrewMember &c, uint8_t skill)
{
	if (skill >= SKILL_COUNT) return 0.0f;
	float v = static_cast<float>(c.skills[skill]);
	// Aptitude traits add; behavioural traits do not (docs/character-attributes.md).
	if (HasTrait(c, TRAIT_FIRST_CONTACT_TRAINED) && (skill == SKILL_SCIENCE || skill == SKILL_COMMAND))
		v += 1.0f;
	// Then conditions, the deficit and haste reduce it, down to nothing.
	float drag = 0.5f * c.deficit + 0.3f * c.fatigue;
	for (int i = 0; i < c.conditionCount; ++i) {
		const Condition &k = c.conditions[i];
		if (k.valence != CVAL_DEBUFF) continue;
		drag += k.magnitude == CMAG_SHARP ? 0.6f : k.magnitude == CMAG_CLEAR ? 0.35f : 0.15f;
	}
	return std::max(0.0f, v - drag);
}

const char *MannerLine(const CrewMember &c)
{
	const float m = Morale(c);
	if (m >= 0.7f) {
		switch (c.desire) {
		case DESIRE_HOME: return "We'll get there. One more day's work, one more mile.";
		case DESIRE_A_PERSON: return "I'm all right. Let me finish this and I'll go and see them.";
		case DESIRE_TO_PROVE: return "Give me the hard one. I want to be the one who did it.";
		default: return "Ready when you are. Let's do it properly.";
		}
	}
	if (m >= 0.4f) {
		switch (c.fear) {
		case FEAR_THE_BORG: return "I'll do it. I'd just rather not be the one who meets them first.";
		case FEAR_USELESSNESS: return "Tell me it mattered. Tell me the work mattered.";
		case FEAR_COWARDICE: return "I'll go. I just need a moment. I'll go.";
		default: return "Another watch. It goes on. It always goes on.";
		}
	}
	switch (c.fear) {
	case FEAR_DYING_ALONE: return "If it goes wrong in there, don't leave me on my own.";
	case FEAR_DECOMPRESSION: return "I can't go back into the bay. I'm sorry. I can't.";
	case FEAR_COWARDICE: return "Don't put me at the front. Not today. Not like this.";
	case FEAR_THE_BORG: return "I've done my part. Find someone else. I can't do that again.";
	default: return "Does it even matter any more? Just tell me what to do.";
	}
}

struct Named {
	const char *name, *type;
	uint8_t rank;
	Department dept;
	uint8_t post;
	uint8_t species;
};

// Senior staff and the Hazard Team, as the game names and models them [lore]. All stand alpha watch.
// Species is biology only: the culture that goes with it is learned, and a character may reject it
// (Torres does), so it is not a disposition and not a bonus.
static const Named NAMED[] = {
	{"Kathryn Janeway", "janeway", 6, DEPT_COMMAND, SYS_COUNT, SPECIES_HUMAN},
	{"Chakotay", "chakotay", 5, DEPT_COMMAND, SYS_COUNT, SPECIES_HUMAN},
	{"Tuvok", "tuvok", 4, DEPT_SECURITY, SYS_SHIELDS, SPECIES_VULCAN},
	{"Tom Paris", "paris", 3, DEPT_COMMAND, SYS_COUNT, SPECIES_HUMAN},
	{"Harry Kim", "kim", 1, DEPT_COMMAND, SYS_COMMUNICATIONS, SPECIES_HUMAN},
	{"B'Elanna Torres", "torres", 3, DEPT_ENGINEERING, SYS_WARP_DRIVE, SPECIES_KLINGON},
	{"The Doctor", "doctor", 3, DEPT_MEDICAL, SYS_SICKBAY, SPECIES_HOLOGRAM},
	{"Seven of Nine", "seven", 0, DEPT_SCIENCES, SYS_SENSORS, SPECIES_BORG_RECOVERED},
	{"Neelix", "neelix", 0, DEPT_COMMAND, SYS_COUNT, SPECIES_TALAXIAN},
	{"Vorik", "vorik", 1, DEPT_ENGINEERING, SYS_WARP_DRIVE, SPECIES_VULCAN},
	{"Les Foster", "Foster", 3, DEPT_SECURITY, SYS_COUNT, SPECIES_HUMAN},
	{"Alexander Munro", "munro", 1, DEPT_SECURITY, SYS_COUNT, SPECIES_HUMAN},
	{"Rick Biessman", "Biessman", 0, DEPT_SECURITY, SYS_COUNT, SPECIES_HUMAN},
	{"Austin Chang", "Chang", 0, DEPT_SECURITY, SYS_COUNT, SPECIES_HUMAN},
	{"Telsia Murphy", "Telsia", 0, DEPT_SECURITY, SYS_COUNT, SPECIES_HUMAN},
	{"Chell", "Chell", 0, DEPT_ENGINEERING, SYS_COUNT, SPECIES_BOLIAN},
	{"Juliet Jurot", "Jurot", 0, DEPT_MEDICAL, SYS_COUNT, SPECIES_BETAZOID},
	{"Kenn", "Kenn", 0, DEPT_SECURITY, SYS_COUNT, SPECIES_HUMAN},
	{"Odell", "Odell", 0, DEPT_SECURITY, SYS_COUNT, SPECIES_HUMAN},
};

// The uniform a generated crew member wears follows their department, as the game's types do.
static const char *GenericType(Department d, int n)
{
	static char buf[16];
	const char *colour = d == DEPT_COMMAND ? "Red" : (d == DEPT_SCIENCES || d == DEPT_MEDICAL) ? "blue" : "Gold";
	const bool female = n % 11 < 3; // the game ships 8 male and 3 female faces per colour
	std::snprintf(buf, sizeof(buf), "%s%c%d", colour, female ? 'F' : 'M', female ? n % 3 + 1 : n % 8 + 1);
	return buf;
}

static void BuildRoster(Ship &s)
{
	s.crew.clear();
	// One seed stream, drawn from for every record's static character data, so the same seed gives
	// the same person -- on the first build and again on load (Unpack calls this).
	uint32_t r = s.cfg.seed ? s.cfg.seed : 1;
	int have[DEPT_COUNT] = {0, 0, 0, 0, 0};
	for (const Named &n : NAMED) {
		CrewMember c;
		c.name = n.name;
		c.type = n.type;
		c.rank = n.rank;
		c.dept = n.dept;
		c.watch = 0;
		c.post = n.post;
		c.species = n.species;
		c.quartersDeck = 3; // senior officers' quarters [lore]
		c.quartersQuality = 0.7f; // senior quarters are the better ones [inv]
		DeriveCharacter(c, r); // skills, traits, drives and starting state, from the record and the seed
		s.crew.push_back(c);
		++have[n.dept];
	}

	// The rest are generated, the same crew for the same seed, and spread evenly over the watches.
	for (int d = 0; d < DEPT_COUNT; ++d) {
		for (int i = have[d]; i < DEPT_SIZE[d]; ++i) {
			CrewMember c;
			char name[32];
			std::snprintf(name, sizeof(name), "Crewman %03d", static_cast<int>(s.crew.size()) + 1);
			c.name = name;
			c.dept = static_cast<Department>(d);
			c.type = GenericType(c.dept, static_cast<int>(NextRandom(r) % 1000));
			c.rank = NextRandom(r) % 5 == 0 ? 1 : 0;
			c.watch = static_cast<uint8_t>(i % WATCHES);
			c.quartersDeck = static_cast<uint8_t>(4 + NextRandom(r) % 6); // crew quarters, decks 4-9 [inv]
			c.quartersQuality = 0.4f + (NextRandom(r) % 5) * 0.1f;        // some bunk better than others [inv]
			c.faction = (NextRandom(r) % 6 == 0) ? 1 : 0;                 // a Maquis alongside the Starfleet crew [lore]
			c.species = DeriveSpecies(c.dept, r);
			DeriveCharacter(c, r);
			s.crew.push_back(c);
		}
	}

	// Stations: every system that needs hands gets them on every watch, from its own department.
	// Named crew keep the posts given above and count toward alpha watch's need.
	for (int id = 0; id < SYS_COUNT; ++id) {
		for (int w = 0; w < WATCHES; ++w) {
			int need = SPECS[id].crewNeeded;
			for (const CrewMember &c : s.crew)
				if (c.post == id && c.watch == w) --need;
			for (CrewMember &c : s.crew) {
				if (need <= 0) break;
				if (c.post != SYS_COUNT || c.watch != w || c.dept != SPECS[id].dept) continue;
				if (c.rank >= 5) continue; // the captain and first officer command; they stand no station
				c.post = static_cast<uint8_t>(id);
				--need;
			}
		}
	}

	// The Maquis split's baseline: a Starfleet crew with Maquis among it, and the resentment that
	// comes with that, until the crew work it through.
	int maquis = 0;
	for (const CrewMember &c : s.crew) if (c.faction == 1) ++maquis;
	s.resentment = std::min(0.3f, maquis * 0.05f);
}

// The bay's complement at the start of the run [lore]: a Class 2, a Type 6 and a Type 8 shuttle and
// the runabout-sized Aeroshuttle. The Delta Flyer is built by the crew, so it is not here at the start.
static void BuildShuttles(Ship &s)
{
	s.shuttles.clear();
	static const ShuttleClass START[4] = { SHUTTLE_CLASS2, SHUTTLE_TYPE6, SHUTTLE_TYPE8, SHUTTLE_AEROSHUTTLE };
	for (int i = 0; i < 4; ++i) {
		Shuttle sh;
		sh.cls = START[i];
		sh.name = ShuttleClassName(START[i]);
		s.shuttles.push_back(sh);
	}
}

static int DutyDeck(const CrewMember &c)
{
	return c.post < SYS_COUNT ? SPECS[c.post].deck : DEPT_DECK[c.dept];
}

// What a post-holder is worth this tick. A rested, willing person is exactly one hand, so an
// ordinary watch is unchanged; only a spent crew member (past half-tired) or a flagging one (morale
// below the halfway mark) costs anything, down to a quarter. That way a bad month bites and a good
// one restores, while the other systems' exact accounting still holds on a fresh, rested crew.
static float Effectiveness(const CrewMember &c)
{
	const float tired = std::max(0.0f, c.fatigue - 0.5f) / 0.5f;
	const float down = std::max(0.0f, 0.5f - Morale(c)) / 0.5f; // read, never a stored counter
	return std::max(0.25f, 1.0f - 0.5f * std::min(1.0f, tired) - 0.5f * std::min(1.0f, down));
}

// The person who signs an event: the senior fit crew member of the department on duty, or a named
// station as a fallback. The log attributes events to people rather than subsystems.
static const char *AuthorFor(const Ship &s, Department dept, const char *fallback)
{
	const CrewMember *best = nullptr;
	for (const CrewMember &c : s.crew) {
		if (c.status != CREW_FIT || c.dept != dept || c.activity != ACT_ON_DUTY) continue;
		if (!best || c.rank > best->rank) best = &c;
	}
	return best ? best->name.c_str() : fallback;
}

// A condition arriving, and a condition cured, each recorded. Conditions are a person's, but the
// record is the ship's: what a modifier nobody can see would be is a hidden penalty, so the log
// carries the arrival and the cure, and the source travels with it.
bool Sicken(Ship &s, int crew, uint8_t id, const std::string &source, int8_t valence,
            uint8_t magnitude, uint8_t clears, uint8_t visible)
{
	if (crew < 0 || crew >= static_cast<int>(s.crew.size())) return false;
	CrewMember &c = s.crew[crew];
	if (!AddCondition(c, id, source, valence, magnitude, clears, static_cast<float>(s.clock), visible))
		return false;
	// Visibility is per condition: if the log is not to record it, the log does not. Automatic
	// conditions from the tick are visible to the player and the crew but not written per head,
	// because a red alert would otherwise bury the log under one line per person.
	if (visible & CVIS_LOG)
		LogEvent(s, AuthorFor(s, DEPT_MEDICAL, "sickbay"), "crew",
			c.name + (valence == CVAL_BUFF ? " takes on " : " comes down with ") + ConditionName(id)
			+ " (" + source + ")");
	return true;
}

bool Cure(Ship &s, int crew, uint8_t id)
{
	if (crew < 0 || crew >= static_cast<int>(s.crew.size())) return false;
	CrewMember &c = s.crew[crew];
	const Condition *was = FindCondition(c, id);
	const bool logged = was && (was->visible & CVIS_LOG);
	if (!ClearCondition(c, id)) return false;
	if (logged)
		LogEvent(s, AuthorFor(s, DEPT_MEDICAL, "sickbay"), "crew", c.name + " is over " + ConditionName(id));
	return true;
}

// Does a species carry a condition among the ones it is prone to?
static bool SpeciesProne(uint8_t species, uint8_t id)
{
	if (id == COND_NONE) return false;
	for (int i = 0; i < SPECIES_TRAIT_MAX; ++i)
		if (SpeciesOf(species).prone[i] == id) return true;
	return false;
}

// Conditions arriving from the state itself, and clearing when their cure is met. Hysteresis, so a
// value resting on a threshold does not flicker the log. A zero-length tick (the derive-on-load)
// moves nothing, so a save replays identically.
static void UpdateConditions(Ship &s, int i, Activity a, bool airless, bool fed, float hours)
{
	if (hours <= 0.0f) return;
	CrewMember &c = s.crew[i];
	const uint8_t all = CVIS_PLAYER | CVIS_CREW; // visible to people, not written per head (see Sicken)
	// Debuffs from the ship's own state, each with its named cause.
	if (c.fatigue > 0.60f && a != ACT_SLEEP)
		Sicken(s, i, COND_EXHAUSTED, "hasn't slept enough", CVAL_DEBUFF, CMAG_CLEAR, CLEAR_REST, all);
	if (!fed)
		Sicken(s, i, COND_HUNGRY, "the galley is empty", CVAL_DEBUFF, CMAG_CLEAR, CLEAR_MEAL, all);
	if (airless)
		Sicken(s, i, COND_HYPOXIC, "the deck has no air", CVAL_DEBUFF, CMAG_SHARP, CLEAR_SICKBAY, all);
	if (s.alert == ALERT_RED)
		Sicken(s, i, COND_AFRAID, "the ship is at battle stations", CVAL_DEBUFF, CMAG_SLIGHT, CLEAR_END_OF_WATCH, all);
	if (Trauma(c) >= 0.50f)
		Sicken(s, i, COND_GRIEVING, "lost someone and still carries it", CVAL_DEBUFF, CMAG_CLEAR, CLEAR_SALIENCE, all);
	// A species' own need, as a condition and not a bonus: the meditation cycle falls due.
	if (SpeciesProne(c.species, COND_MEDITATION_DUE) && c.fatigue > 0.60f && a != ACT_SLEEP)
		Sicken(s, i, COND_MEDITATION_DUE, "the meditation cycle, unkept", CVAL_DEBUFF, CMAG_SLIGHT, CLEAR_REST, all);
	// Cures.
	if (c.fatigue <= 0.15f || a == ACT_SLEEP) Cure(s, i, COND_EXHAUSTED);
	if (fed) Cure(s, i, COND_HUNGRY);
	if (!airless) Cure(s, i, COND_HYPOXIC);
	if (s.alert != ALERT_RED) Cure(s, i, COND_AFRAID);
	if (Trauma(c) < 0.40f) Cure(s, i, COND_GRIEVING);
	if (!(c.fatigue > 0.60f && a != ACT_SLEEP)) Cure(s, i, COND_MEDITATION_DUE);
}

// Whoever commands: the player's character if there is one and is not lost, otherwise the captain or
// first officer. A dead or assimilated player commands nothing -- command has passed on (Stage B).
std::string CommandingOfficer(const Ship &s)
{
	if (s.player >= 0 && s.player < static_cast<int>(s.crew.size())
		&& s.crew[s.player].status != CREW_DEAD && s.crew[s.player].status != CREW_ASSIMILATED)
		return s.crew[s.player].name;
	for (const CrewMember &c : s.crew)
		if (c.status == CREW_FIT && c.rank >= 5) return c.name;
	return "command";
}

// The senior fit officer of a department, preferring the one on duty: who a recommendation comes
// from. The chief engineer is the head of engineering (docs/power-assignment.md, Task C).
int DepartmentHead(const Ship &s, Department dept)
{
	int best = -1, bestRank = -1;
	bool bestOnDuty = false;
	for (int i = 0; i < static_cast<int>(s.crew.size()); ++i) {
		const CrewMember &c = s.crew[i];
		if (c.status != CREW_FIT || c.dept != dept) continue;
		const bool onDuty = c.activity == ACT_ON_DUTY;
		if (best < 0 || (onDuty && !bestOnDuty) || (onDuty == bestOnDuty && c.rank > bestRank)) {
			best = i; bestRank = c.rank; bestOnDuty = onDuty;
		}
	}
	return best;
}

// The ward, in the order the triage standing order treats it. One row per casualty for the screen,
// and the same order the beds are given in below, so the screen and the ward never disagree.
std::vector<int> Patients(const Ship &s)
{
	std::vector<int> hurt;
	for (int i = 0; i < static_cast<int>(s.crew.size()); ++i)
		if (s.crew[i].status == CREW_INJURED) hurt.push_back(i);
	std::stable_sort(hurt.begin(), hurt.end(), [&](int a, int b) {
		if (s.orderTriage == 1) return s.crew[a].rank > s.crew[b].rank; // rank first
		return s.crew[a].severity > s.crew[b].severity;                 // worst first (default)
	});
	return hurt;
}

// Sickbay: a fixed number of beds (three standard and one surgical), a triage order, and the
// supplies treatment costs. The worst (or the most senior) cases get the beds; anyone left waiting
// worsens, and a critical case nobody reaches dies. That is the decision the gap asks for: more
// casualties than beds is a choice, not a queue that clears itself.
static void TreatCasualties(Ship &s, float shipSeconds)
{
	const float hours = shipSeconds / 3600.0f;
	// The EMH is a program: with the computer core up it keeps the ward at half output when the
	// medical staff are down.
	const float sickbay = std::max(s.systems[SYS_SICKBAY].output,
		(s.emhActive && s.systems[SYS_COMPUTER_CORE].output > 0.0f) ? (s.mobileEmitter ? 0.8f : 0.5f) : 0.0f);
	for (int i = 0; i < static_cast<int>(s.crew.size()); ++i) s.crew[i].underCare = false;
	std::vector<int> hurt = Patients(s);
	if (hurt.empty()) return;
	int bed = 0;
	for (size_t k = 0; k < hurt.size(); ++k) {
		const int i = hurt[k];
		CrewMember &c = s.crew[i];
		// The gravest case (or the most senior, under rank-first) has the surgical bay.
		const bool surgical = k == 0;
		if (bed < SICKBAY_BEDS && sickbay > 0.0f && s.stores.medicalSupplies > 0.0f) {
			++bed;
			c.underCare = true;
			c.deck = SICKBAY_DECK;
			c.recovery += sickbay * shipSeconds / (TREATMENT_HOURS * 3600.0f);
			s.stores.medicalSupplies = std::max(0.0f, s.stores.medicalSupplies - MEDICAL_PER_PATIENT_HOUR * hours);
			if (c.recovery >= 1.0f) {
				c.status = CREW_FIT;
				c.recovery = c.exposure = c.wounds = c.severity = 0.0f;
			}
		} else if (surgical && s.surgicalForceField) {
			// The surgical bay's field holds the one case that would otherwise be lost: steady, not
			// healing. It is the difference between a critical patient and a body.
			++bed;
			c.underCare = true;
			c.deck = SICKBAY_DECK;
		} else if (hours > 0.0f) {
			// No bed, no output, or no supplies: the injury goes on getting worse, and can kill.
			// Only with time passing: a zero-length tick is the derive-on-load (see Unpack), and it
			// must not move a saved field, or a save would not replay identically.
			c.severity = std::min(1.0f, c.severity + DETERIORATE_PER_HOUR * hours);
			c.wounds = std::max(c.wounds, c.severity);
			if (c.severity >= 1.0f) { c.status = CREW_DEAD; c.recovery = 0.0f; NoteDeath(s, i); LogEvent(s, AuthorFor(s, DEPT_MEDICAL, "sickbay"), "sickbay", c.name + " died of wounds before a bed freed"); }
		}
	}
}

static void UpdateCrew(Ship &s, float shipSeconds)
{
	const int sod = s.SecondOfDay();
	for (int id = 0; id < SYS_COUNT; ++id) s.systems[id].manned = 0, s.systems[id].staffing = 0.0f, s.systems[id].repairing = 0;

	// Damage control: which systems want hands, most critical first.
	int wantRepair[SYS_COUNT], nWant = 0;
	for (int id = 0; id < SYS_COUNT; ++id)
		if (s.systems[id].health < 1.0f) wantRepair[nWant++] = id;
	std::stable_sort(wantRepair, wantRepair + nWant, [&](int a, int b) {
		// the system the captain has ordered seen to comes before the critical ones
		const bool fa = a == s.orderRepairFirst, fb = b == s.orderRepairFirst;
		if (fa != fb) return fa;
		return s.systems[a].priority < s.systems[b].priority;
	});
	for (Deck &d : s.decks) d.defenders = d.stripping = d.sealing = d.firefighting = 0;
	// The decks on fire, worst first: security and spare engineers fight them.
	int burning[DECKS], nBurning = 0;
	for (int d = 0; d < DECKS; ++d)
		if (s.decks[d].fire > 0.0f) burning[nBurning++] = d;
	std::stable_sort(burning, burning + nBurning, [&](int a, int b) { return s.decks[a].fire > s.decks[b].fire; });
	// Where security is needed: the deck with the most boarders first.
	int hot[DECKS], nHot = 0;
	for (int d = 0; d < DECKS; ++d)
		if (s.decks[d].intruders > 0.0f) hot[nHot++] = d;
	std::stable_sort(hot, hot + nHot, [&](int a, int b) { return s.decks[a].intruders > s.decks[b].intruders; });
	int posted = 0; // security sent to the deck the captain ordered held

	// A standing grief: the fraction of the crew lost, as a drag on everyone's mood.
	float lost = 0.0f;
	for (const CrewMember &c : s.crew)
		if (c.status == CREW_DEAD || c.status == CREW_ASSIMILATED) lost += 1.0f;
	const float lossFactor = s.crew.empty() ? 0.0f : 0.5f * std::min(0.6f, lost / static_cast<float>(s.crew.size()));
	// The Maquis split: a crew divided against itself is a drag on everyone's mood, and so is the
	// resentment the division leaves behind, until the crew work it through.
	float maquis = 0.0f;
	for (const CrewMember &c : s.crew) if (c.faction == 1) maquis += 1.0f;
	const float minority = std::min(maquis, static_cast<float>(s.crew.size()) - maquis);
	const float factionDrag = (s.crew.empty() ? 0.0f : 0.15f * (minority / static_cast<float>(s.crew.size()))) + 0.15f * s.resentment;
	// Population pressure: refugees and survivors crowd the ship and eat its stores.
	const float crowding = 0.05f * static_cast<float>(s.refugees) / COMPLEMENT;
	int buildingHands = 0; // engineers with nothing to mend, seal or fight, building a part

	for (CrewMember &c : s.crew) {
		if (c.status == CREW_DEAD || c.status == CREW_ASSIMILATED) {
			c.activity = ACT_SLEEP; // lost to the ship: nowhere aboard, doing nothing
			c.deck = 0;
			continue;
		}
		if (c.away) {
			c.activity = ACT_PERSONAL; // off the ship on an away mission: on no deck, standing no watch
			c.deck = 0;
			continue;
		}
		if (c.brigged) {
			c.activity = ACT_PERSONAL; // confined to the brig: stands no watch, mans nothing
			c.deck = 8;
			continue;
		}
		if (c.holoCompulsion >= 1.0f) {
			c.activity = ACT_RECREATION; // lost in the program: no watch, no post, and it wears on them
			c.deck = HOLODECK_RECREATION_DECK;
			c.fatigue = std::min(1.0f, c.fatigue + shipSeconds / (16.0f * 3600.0f));
			continue;
		}
		Activity a = ScheduledActivity(c.watch, sod);
		// Red alert is all hands: anyone awake goes to their station.
		if (s.alert == ALERT_RED && a != ACT_SLEEP) a = ACT_ON_DUTY;
		if (c.status == CREW_INJURED) a = ACT_PERSONAL; // off the watch bill until sickbay returns them
		c.activity = a;

		switch (a) {
		case ACT_ON_DUTY: c.deck = static_cast<uint8_t>(DutyDeck(c)); break;
		case ACT_MEAL: c.deck = MESS_DECK; break;
		case ACT_RECREATION: c.deck = s.systems[SYS_HOLODECKS].output > 0.0f ? HOLODECK_RECREATION_DECK : MESS_DECK; break;
		default: c.deck = c.quartersDeck; break;
		}

		// An evacuated deck is left: whoever the routine would put there goes to the mess hall instead,
		// station or not. (Security ordered to that same deck is the exception: that is what a guard is.)
		if (s.orderEvacuate >= 1 && c.deck == s.orderEvacuate && !(c.dept == DEPT_SECURITY && s.orderSecurityTo == s.orderEvacuate)) {
			c.deck = static_cast<uint8_t>(s.orderEvacuate == MESS_DECK ? HOLODECK_RECREATION_DECK : MESS_DECK);
			if (a == ACT_ON_DUTY && c.post < SYS_COUNT && SPECS[c.post].deck == s.orderEvacuate) {
				c.activity = ACT_PERSONAL; // off their station, by order
				a = ACT_PERSONAL;
			}
		}

		// The player's body is where they are standing, not where their schedule says: the world
		// reports it (SetPlayerDeck). Everything below reads c.deck, so the air, the fire and the
		// hazards of the room they are actually in reach the person holding the controls. Once they
		// are under care the ward has them, and that deck governs -- being carried is the same shape
		// as any other casualty.
		if (static_cast<int>(&c - s.crew.data()) == s.player && !c.underCare
			&& s.playerDeck >= 1 && s.playerDeck <= DECKS)
			c.deck = static_cast<uint8_t>(s.playerDeck);

		// A deck without air.
		if (c.deck >= 1 && c.deck <= DECKS && s.decks[c.deck - 1].atmosphere < AIRLESS) {
			c.exposure += shipSeconds;
			if (c.exposure >= EXPOSURE_KILLS) {
				c.status = CREW_DEAD;
				NoteDeath(s, static_cast<int>(&c - s.crew.data()));
				c.activity = ACT_SLEEP;
				c.deck = 0;
				LogEvent(s, AuthorFor(s, DEPT_MEDICAL, "sickbay"), "sickbay", c.name + " dead, no air");
				continue;
			}
			if (c.exposure >= EXPOSURE_INJURES && c.status == CREW_FIT) {
				c.status = CREW_INJURED;
				c.severity = std::max(c.severity, std::min(1.0f,
					0.3f + 0.7f * (c.exposure - EXPOSURE_INJURES) / (EXPOSURE_KILLS - EXPOSURE_INJURES)));
				LogEvent(s, AuthorFor(s, DEPT_MEDICAL, "sickbay"), "sickbay", c.name + " injured without air on deck " + std::to_string(c.deck));
			}
		} else if (c.status == CREW_FIT) {
			c.exposure = std::max(0.0f, c.exposure - shipSeconds); // catching their breath
		}
		// Fire on the deck injures and can kill, as a deck without air does; it eases off a deck that
		// is not burning.
		if (c.deck >= 1 && c.deck <= DECKS && s.decks[c.deck - 1].fire > 0.05f) {
			c.burn += s.decks[c.deck - 1].fire * shipSeconds;
			if (c.burn >= FIRE_KILLS) {
				c.status = CREW_DEAD;
				NoteDeath(s, static_cast<int>(&c - s.crew.data()));
				c.activity = ACT_SLEEP; c.deck = 0;
				LogEvent(s, AuthorFor(s, DEPT_MEDICAL, "sickbay"), "sickbay", c.name + " dead in a fire");
				continue;
			}
			if (c.burn >= FIRE_INJURES && c.status == CREW_FIT) {
				c.status = CREW_INJURED;
				c.severity = std::max(c.severity, std::min(1.0f, 0.3f + 0.7f * (c.burn - FIRE_INJURES) / (FIRE_KILLS - FIRE_INJURES)));
				LogEvent(s, AuthorFor(s, DEPT_MEDICAL, "sickbay"), "sickbay", c.name + " burned on deck " + std::to_string(c.deck));
			}
		} else {
			c.burn = std::max(0.0f, c.burn - shipSeconds);
		}
		// Radiation from a failing core: the engineering watch on deck 11 takes it (another injury
		// cause, S6). A whole core is clean; the worse it is, the faster the dose adds up.
		const float coreHealth = std::min(s.systems[SYS_WARP_DRIVE].health, s.sources[SRC_WARP_CORE].health);
		if (c.deck == ENGINEERING_DECK && coreHealth < 0.5f) {
			c.radiation += (0.5f - coreHealth) * shipSeconds;
			if (c.radiation >= RADIATION_KILLS) {
				c.status = CREW_DEAD;
				NoteDeath(s, static_cast<int>(&c - s.crew.data()));
				c.activity = ACT_SLEEP; c.deck = 0;
				LogEvent(s, AuthorFor(s, DEPT_MEDICAL, "sickbay"), "sickbay", c.name + " died of radiation in Engineering");
				continue;
			}
			if (c.radiation >= RADIATION_INJURES && c.status == CREW_FIT) {
				c.status = CREW_INJURED;
				c.severity = std::max(c.severity, std::min(1.0f, 0.3f + 0.7f * (c.radiation - RADIATION_INJURES) / (RADIATION_KILLS - RADIATION_INJURES)));
				LogEvent(s, AuthorFor(s, DEPT_MEDICAL, "sickbay"), "sickbay", c.name + " irradiated in Engineering");
			}
		} else if (c.radiation > 0.0f) {
			c.radiation = std::max(0.0f, c.radiation - shipSeconds);
		}
		if (c.status != CREW_FIT) continue; // the injured man no station and mend nothing

		const float hours = shipSeconds / 3600.0f;
		const SpeciesRecord &sp = SpeciesOf(c.species);
		// The watch bill reads species: no fatigue for a species that does not rest at all (a
		// hologram), because its failure set is different rather than absent. The rest keep the
		// ordinary curve; a species' rest need is a shape, not a bonus.
		if (sp.restNeedHours <= 0.0f) {
			c.fatigue = 0.0f; // a hologram does not tire
		} else {
			if (a == ACT_ON_DUTY) c.fatigue += hours / 20.0f;       // a double watch leaves you spent [inv]
			else if (a == ACT_SLEEP) c.fatigue -= hours / 8.0f;     // a night's sleep clears it [inv]
			else c.fatigue -= hours / 40.0f;
			c.fatigue = std::min(1.0f, std::max(0.0f, c.fatigue));
		}

		// Morale, read from three components (docs/morale.md): deficit, outlook and holdings. Each
		// is its own quantity and eases toward what the day offers over hours rather than snapping,
		// so a bad afternoon is not a bad week. Nothing here stores a scalar; Morale() reads them.
		// The galley reads species: a meal comes from the rations or the replicators only for a
		// species that eats, so a hologram neither eats nor goes hungry.
		if (a == ACT_MEAL && s.stores.rations > 0.0f && sp.needsFood)
			s.stores.rations = std::max(0.0f, s.stores.rations - RATIONS_PER_CREW_DAY * hours / 24.0f);
		const bool fed = !sp.needsFood
			|| s.systems[SYS_REPLICATORS].output > 0.0f || s.stores.rations > 0.0f;
		const bool airless = c.deck >= 1 && c.deck <= DECKS && s.decks[c.deck - 1].atmosphere < AIRLESS;

		// Deficit: what they are short of -- sleep, food, care, comfort, company.
		float deficitTarget = 0.10f + 0.35f * c.fatigue;
		if (!fed) deficitTarget += 0.25f;                    // hunger
		if (airless) deficitTarget += 0.30f;
		deficitTarget += (0.5f - c.quartersQuality) * 0.30f; // where they bunk
		if (a == ACT_SLEEP) deficitTarget = std::min(deficitTarget, 0.10f);
		if (a == ACT_MEAL && fed) deficitTarget = std::min(deficitTarget, 0.20f);
		if (a == ACT_RECREATION) deficitTarget = std::min(deficitTarget, 0.25f);

		// Outlook: what they believe about the situation -- are we getting home, is command competent.
		float outlookTarget = 0.70f;
		if (a == ACT_SLEEP) outlookTarget = 0.72f;
		else if (a == ACT_MEAL) outlookTarget = fed ? 0.78f : 0.40f;
		else if (a == ACT_RECREATION) outlookTarget = s.systems[SYS_HOLODECKS].output > 0.0f ? 0.88f : 0.65f;
		else if (a == ACT_ON_DUTY) outlookTarget = s.alert == ALERT_GREEN ? 0.70f : 0.58f;
		if (s.alert == ALERT_RED) outlookTarget -= 0.15f;
		outlookTarget -= lossFactor;                  // losses still felt
		outlookTarget -= factionDrag;                 // a crew divided against itself
		outlookTarget -= crowding;                    // refugees and survivors aboard
		outlookTarget += (c.quartersQuality - 0.5f) * 0.10f;
		outlookTarget -= c.assimScar * SCAR_DRAG;     // a de-assimilation's residue never fully fades

		// Holdings: who they hold with -- bonds, grudges, allegiances.
		float holdingsTarget = 0.75f;
		holdingsTarget -= std::min(0.40f, Trauma(c) * 0.20f); // what they carry, until the holodeck fades it
		holdingsTarget -= 0.5f * factionDrag;                 // a crew divided holds with less of itself

		const float ease = std::min(1.0f, hours / 6.0f);      // a few hours to shift the mood [inv]
		c.deficit += (deficitTarget - c.deficit) * ease;
		c.outlook += (outlookTarget - c.outlook) * ease;
		c.holdings += (holdingsTarget - c.holdings) * ease;
		c.deficit = std::min(1.0f, std::max(0.0f, c.deficit));
		c.outlook = std::min(1.0f, std::max(0.0f, c.outlook));
		c.holdings = std::min(1.0f, std::max(0.0f, c.holdings));

		// Conditions arriving from the state, and clearing when their cure is met.
		UpdateConditions(s, static_cast<int>(&c - s.crew.data()), a, airless, fed, hours);


		// Security on duty with no station answers a boarding: enough to outnumber each party, worst first.
		if (a == ACT_ON_DUTY && c.post == SYS_COUNT && c.dept == DEPT_SECURITY) {
			// ordered to a deck: a guard of four goes there first, whether or not anyone has boarded it
			if (s.orderSecurityTo >= 1 && s.orderSecurityTo <= DECKS && posted < 4) {
				++posted;
				c.deck = static_cast<uint8_t>(s.orderSecurityTo);
				if (s.decks[s.orderSecurityTo - 1].intruders > 0.0f) ++s.decks[s.orderSecurityTo - 1].defenders;
				goto placed;
			}
			for (int k = 0; k < nHot; ++k) {
				Deck &d = s.decks[hot[k]];
				if (hot[k] + 1 == s.orderEvacuate) continue; // an evacuated deck is left to whoever is on it
				if (d.defenders > static_cast<int>(d.intruders) + 1) continue;
				++d.defenders;
				c.deck = static_cast<uint8_t>(hot[k] + 1);
				break;
			}
			// No boarders to fight: the worst fire is the security party's to put down.
			if (nHot == 0)
				for (int k = 0; k < nBurning; ++k) {
					Deck &d = s.decks[burning[k]];
					if (d.firefighting >= 4) continue;
					++d.firefighting;
					c.deck = static_cast<uint8_t>(burning[k] + 1);
					break;
				}
		}

		placed:
		if (a == ACT_ON_DUTY && c.post < SYS_COUNT) {
			++s.systems[c.post].manned;
			float eff = Effectiveness(c);
			// Memory read by the simulation: a grudge toward whoever commands is going through the motions.
			const float loyalty = Loyalty(s, static_cast<int>(&c - s.crew.data()));
			if (loyalty < -0.3f) eff *= (1.0f + loyalty);
			s.systems[c.post].staffing += eff;
		}

		// An engineer on duty with no station of their own joins the damage-control party.
		if (a == ACT_ON_DUTY && c.post == SYS_COUNT && c.dept == DEPT_ENGINEERING) {
			bool busy = false;
			for (int k = 0; k < nWant && !busy; ++k) {
				System &sys = s.systems[wantRepair[k]];
				if (sys.repairing >= REPAIR_TEAM_MAX) continue;
				if (SPECS[wantRepair[k]].deck == s.orderEvacuate) continue; // not on a deck that has been cleared
				++sys.repairing;
				c.deck = static_cast<uint8_t>(SPECS[wantRepair[k]].deck); // they go to the work
				busy = true;
			}
			// With nothing broken to mend, they cut Borg technology out of any deck the drones have left.
			for (int d = 0; d < DECKS && !busy; ++d) {
				Deck &deck = s.decks[d];
				if (deck.assimilated <= 0.0f || deck.intruders > 0.0f || deck.stripping >= STRIP_TEAM_MAX) continue;
				++deck.stripping;
				c.deck = static_cast<uint8_t>(d + 1);
				busy = true;
			}
			// Nothing broken and no Borg to strip: seal the worst breach, or fight the fire.
			for (int d = 0; d < DECKS && !busy; ++d) {
				Deck &deck = s.decks[d];
				if (deck.hull >= 1.0f || deck.sealing >= REPAIR_TEAM_MAX) continue;
				++deck.sealing;
				c.deck = static_cast<uint8_t>(d + 1);
				busy = true;
			}
			for (int k = 0; k < nBurning && !busy; ++k) {
				Deck &deck = s.decks[burning[k]];
				if (deck.firefighting >= 4) continue;
				++deck.firefighting;
				c.deck = static_cast<uint8_t>(burning[k] + 1);
				busy = true;
			}
			// Nothing to mend, seal or fight: an engineer can build instead (fabricate a part).
			if (buildingHands < 4)
				for (size_t b = 0; b < s.jobs.size() && !busy; ++b)
					if (s.jobs[b].kind == JOB_BUILD && s.jobs[b].target != 0) { ++buildingHands; busy = true; }
		}
	}

	// Build: the crew fabricate spare parts from the ship's material, over crew-hours; a negative
	// target is a shuttle for the second bay (docs/shuttles.md).
	if (buildingHands > 0) {
		for (Job &j : s.jobs) {
			if (j.kind != JOB_BUILD || j.target == 0) continue;
			if (j.target > 0) {
				j.progress += buildingHands * shipSeconds / (BUILD_HOURS_PER_PART * 3600.0f);
				while (j.progress >= 1.0f && j.target > 0) {
					if (s.stores.materials < 1.0f) { j.progress = 0.0f; break; } // no material, no part
					j.progress -= 1.0f;
					s.stores.materials -= 1.0f;
					s.stores.spareParts += 1.0f;
					--j.target;
				}
			} else {
				j.progress += buildingHands * shipSeconds / (SHUTTLE_REBUILD_HOURS * 3600.0f);
				if (j.progress >= 1.0f) {
					if (s.stores.materials >= SHUTTLE_REBUILD_MATERIALS) {
						s.stores.materials -= SHUTTLE_REBUILD_MATERIALS;
						const ShuttleClass cls = static_cast<ShuttleClass>(-j.target - 1);
						Shuttle *sh = ShuttleByClass(s, cls);
						if (sh) { sh->location = SHUTTLE_IN_BAY; sh->condition = 1.0f; sh->comms = COMMS_LINKED; sh->awayBeacon = -1; }
						LogEvent(s, AuthorFor(s, DEPT_ENGINEERING, "the second bay"), "engineering", std::string("a new ") + ShuttleClassName(cls) + " is complete");
						j.target = 0; // done; the queue drops a zero-target build
					} else j.progress = 1.0f; // finished the work, waiting on material
				}
			}
			break; // one build job at a time
		}
	}

	// The work itself: paid for in parts, and it stops when they run out.
	for (int k = 0; k < nWant; ++k) {
		System &sys = s.systems[wantRepair[k]];
		if (!sys.repairing || s.stores.spareParts <= 0.0f) continue;
		float gain = sys.repairing * shipSeconds / (REPAIR_HOURS_PER_SYSTEM * 3600.0f) * (s.pursued ? PURSUIT_REPAIR : 1.0f);
		gain = std::min(gain, 1.0f - sys.health);
		gain = std::min(gain, s.stores.spareParts / PARTS_PER_SYSTEM);
		sys.health += gain;
		s.stores.spareParts = std::max(0.0f, s.stores.spareParts - gain * PARTS_PER_SYSTEM);
	}

	// Sealing a breached hull: crew, hours and parts, like any other repair. With no parts it stays open.
	for (int d = 0; d < DECKS; ++d) {
		Deck &deck = s.decks[d];
		if (!deck.sealing || deck.hull >= 1.0f || s.stores.spareParts <= 0.0f) continue;
		float gain = deck.sealing * shipSeconds / (SEAL_HOURS_PER_DECK * 3600.0f) * (s.pursued ? PURSUIT_REPAIR : 1.0f);
		gain = std::min(gain, 1.0f - deck.hull);
		gain = std::min(gain, s.stores.spareParts / PARTS_PER_DECK_SEAL);
		deck.hull += gain;
		s.stores.spareParts = std::max(0.0f, s.stores.spareParts - gain * PARTS_PER_DECK_SEAL);
	}

	// Memories fade unless something retells or re-experiences them.
	MemoryDecay(s, shipSeconds);

	// Survivors and refugees eat, once per tick, from the same galley as the crew.
	if (s.refugees > 0)
		s.stores.rations = std::max(0.0f, s.stores.rations - RATIONS_PER_CREW_DAY * s.refugees * shipSeconds / SECONDS_PER_DAY);

	// The airponics bay grows food without the replicators, if life support runs it.
	if (s.airponics && s.systems[SYS_LIFE_SUPPORT].output > 0.0f)
		s.stores.rations = std::min(100.0f, s.stores.rations + AIRPONICS_PER_HOUR * shipSeconds / 3600.0f);

	// The log can say why the crew's mood is what it is: every few hours, one line naming the driver.
	// (The mood itself is tested; this is the record that lets a player reconstruct it.)
	if (shipSeconds > 0.0f && s.clock - s.lastMoodLog >= 6.0 * 3600.0) {
		s.lastMoodLog = s.clock;
		float morale = 0.0f, fatigue = 0.0f;
		int fit = 0, lowest = -1;
		for (int i = 0; i < static_cast<int>(s.crew.size()); ++i) {
			const CrewMember &c = s.crew[i];
			if (c.status != CREW_FIT) continue;
			morale += Morale(c); fatigue += c.fatigue; ++fit;
			if (lowest < 0 || Morale(c) < Morale(s.crew[lowest])) lowest = i;
		}
		if (fit) {
			morale /= fit; fatigue /= fit;
			// The reading is explainable from the three components: name the one most responsible,
			// for the person carrying the most of it, and fall back to the situation when morale is
			// not the driver. The log can always say which component moved and why.
			const char *why = lowest >= 0 && Morale(s.crew[lowest]) < 0.6f ? MoraleReason(s.crew[lowest])
				: s.alert == ALERT_RED ? "red alert"
				: lossFactor > 0.02f ? "losses still felt"
				: s.SecondOfDay() >= 22 * 3600 || s.SecondOfDay() < 6 * 3600 ? "the night watch"
				: "an ordinary watch";
			char note[192];
			std::snprintf(note, sizeof(note), "the crew's mood: morale %d%% (%s), fatigue %d%%",
				static_cast<int>(morale * 100 + 0.5f), why, static_cast<int>(fatigue * 100 + 0.5f));
			LogEvent(s, AuthorFor(s, DEPT_MEDICAL, "the mess"), "crew", note);
		}
		// The director reads the crew's marks: one person's story, from what they carry.
		int worst = -1;
		float worstTrauma = 0.0f;
		for (int i = 0; i < static_cast<int>(s.crew.size()); ++i) {
			if (s.crew[i].status != CREW_FIT) continue;
			const float t = Trauma(s.crew[i]);
			if (t > worstTrauma) { worstTrauma = t; worst = i; }
		}
		if (worst >= 0 && worstTrauma >= 0.5f)
			LogEvent(s, AuthorFor(s, DEPT_MEDICAL, "sickbay"), "crew", s.crew[worst].name + " is still carrying what happened");
	}

	// A downed player is a casualty the ship acts on, not a game over: someone comes for them, and
	// the log names them. Before sickbay, so the log gives the deck they fell on, not the ward.
	AttendIncapacitatedPlayer(s);

	// Sickbay last, so it sees the injuries the fight and the air have just caused.
	TreatCasualties(s, shipSeconds);

	// And the queue of outstanding work, one entry per thing that needs doing.
	UpdateJobs(s);
}

// ---- power ------------------------------------------------------------------------------------

// What a source can deliver right now: its nameplate scaled by its own health and -- for the warp
// core alone -- by the dilithium crystal's ceiling (docs/budget-squaring.md, Part five). The owner's
// mechanic: the ceiling bears on *output*. Recomposition buys the crystal life and permanently
// lowers the plant, so the power budget is what pays for the extra time. [inv, beside the fraction
// rates in docs/lore-ledger.md]
static int SourceCapacity(const Ship &s, SourceId id)
{
	int cap = static_cast<int>(SOURCES[id].capacity * s.sources[id].health + 0.5f);
	if (id == SRC_WARP_CORE) cap = static_cast<int>(cap * Clamp01(s.crystalCeiling) + 0.5f);
	return cap;
}

// The coreless ship (docs/budget-squaring.md, Part 4a): with the warp core gone, only the fusion
// reactors and the batteries are left, and the ship runs the set that keeps her alive and moving --
// the critical four, the impulse drive and the navigational deflector. It is a survival allocation.
bool Coreless(const Ship &s)
{
	const Source &core = s.sources[SRC_WARP_CORE];
	return s.coreEjected || s.coreShutdown || !core.online || core.health <= 0.0f;
}

// The systems the coreless ship still runs. Everything else is dark, however much the crew would
// like the sensors: 390 of the 400 remaining EPS is the whole of the survival set.
static bool CorelessVital(SystemId id)
{
	return id == SYS_LIFE_SUPPORT || id == SYS_STRUCTURAL_INTEGRITY || id == SYS_INERTIAL_DAMPERS
		|| id == SYS_COMPUTER_CORE || id == SYS_IMPULSE_DRIVE || id == SYS_NAV_DEFLECTOR;
}

static void UpdatePower(Ship &s, float shipSeconds)
{
	const float days = shipSeconds / SECONDS_PER_DAY;
	const bool coreless = Coreless(s);

	// What each system asks for. `full` is the power its full capability needs; `commit` is what its
	// set share asks of the plant. A person's share is the mechanism (docs/power-assignment.md); the
	// ladder is the policy of automatic mode alone, and never touches a system a person has set.
	int full[SYS_COUNT];
	int commit[SYS_COUNT];
	int order[SYS_COUNT];
	bool on[SYS_COUNT];
	int wanted = 0;
	for (int i = 0; i < SYS_COUNT; ++i) {
		order[i] = i;
		System &sys = s.systems[i];
		const SystemId id = static_cast<SystemId>(i);
		on[i] = sys.enabled && sys.health > 0.0f && !SuppressedByAlert(s.alert, id)
			&& (!coreless || CorelessVital(id));
		full[i] = on[i] ? EffectiveDemand(s, id) : 0;
		commit[i] = on[i] ? static_cast<int>(full[i] * Clamp01(sys.share) + 0.5f) : 0;
		wanted += commit[i];
	}
	std::stable_sort(order, order + SYS_COUNT, [&](int a, int b) { return s.systems[a].priority < s.systems[b].priority; });

	// What can be supplied. The sources run only as hard as the load asks, in order, and only while
	// they have fuel. The batteries are the last source and cover any shortfall -- not the critical
	// systems alone -- which is what puts them in the 1,800 nameplate (docs/budget-squaring.md).
	int supply = 0;
	for (int i = 0; i < SRC_COUNT; ++i) {
		Source &src = s.sources[i];
		src.output = 0;
		if (!src.online || src.health <= 0.0f) continue;
		const bool fuelled = i == SRC_BATTERIES ? s.stores.batteries > 0.0f
			: s.stores.deuterium > 0.0f && (SOURCES[i].antimatterPerDay == 0.0f || s.stores.antimatter > 0.0f);
		if (!fuelled) continue;
		const int capacity = SourceCapacity(s, static_cast<SourceId>(i));
		src.output = std::max(0, std::min(capacity, wanted - supply));
		supply += src.output;

		const float load = SOURCES[i].capacity ? static_cast<float>(src.output) / SOURCES[i].capacity : 0.0f;
		s.stores.deuterium = std::max(0.0f, s.stores.deuterium - SOURCES[i].deuteriumPerDay * load * days);
		s.stores.antimatter = std::max(0.0f, s.stores.antimatter - SOURCES[i].antimatterPerDay * load * days);
		if (i == SRC_BATTERIES)
			s.stores.batteries = std::max(0.0f, s.stores.batteries - load * shipSeconds / (BATTERY_HOURS * 3600.0f));
	}

	// A plant with no fuel at all is damage, not policy: it removes what the ship has, and every
	// system on it goes dark (docs/power-assignment.md, the one exception). A plant that merely
	// cannot cover the commitments is different: the commitments are honoured and reported.
	const bool plantDead = supply <= 0 && wanted > 0;

	// Distribute. In manual mode -- the default -- every commitment is honoured in full and nothing
	// is shed: an over-committed plant is reported, never resolved (Task A: "Nothing sheds itself").
	// In automatic mode the systems a person has set are honoured first, in full, and the ladder is
	// then the policy for the rest -- and only for the rest.
	for (int i = 0; i < SYS_COUNT; ++i) s.systems[i].allocated = 0;
	if (plantDead) {
		// nothing runs: the dark ship
	} else if (!s.powerAuto) {
		for (int i = 0; i < SYS_COUNT; ++i) s.systems[i].allocated = commit[i];
	} else {
		int left = supply;
		for (int i = 0; i < SYS_COUNT; ++i) {
			System &sys = s.systems[i];
			if (commit[i] <= 0 || !PersonSet(sys.allocBy)) continue;
			sys.allocated = commit[i]; // a person's decision: the ladder does not touch it
			left -= sys.allocated;
		}
		for (int k = 0; k < SYS_COUNT; ++k) {
			const int i = order[k];
			System &sys = s.systems[i];
			if (PersonSet(sys.allocBy)) continue;
			sys.allocated = (full[i] > 0 && left > 0) ? std::min(full[i], left) : 0;
			if (sys.allocated > 0) left -= sys.allocated;
		}
	}

	// The one thing that overrides a person's allocation is damage, and it says so in the log: a
	// destroyed system, a dead conduit, no fuel, or the survival set the coreless ship is left with.
	for (int i = 0; i < SYS_COUNT; ++i) {
		System &sys = s.systems[i];
		const SystemId id = static_cast<SystemId>(i);
		const bool damaged = sys.health <= 0.0f || (coreless && !CorelessVital(id)) || plantDead;
		if (PersonSet(sys.allocBy) && sys.share > 0.0f && damaged) {
			if (!sys.allocDamagedOff) {
				sys.allocDamagedOff = true;
				LogEvent(s, AuthorFor(s, DEPT_ENGINEERING, "damage control"), "engineering",
					std::string(SPECS[i].name) + " is dark: damage, not a decision");
			}
		} else if (on[i] && !plantDead) {
			sys.allocDamagedOff = false;
		}
	}

	// The commitments against the plant, reported as a number, and only when it appears or changes.
	// In manual mode this is the player's own over-commitment; in automatic mode the ladder has
	// already resolved the systems nobody set, so it appears only if a person's own asks exceed the
	// plant on their own.
	int granted = 0;
	for (int i = 0; i < SYS_COUNT; ++i) granted += s.systems[i].allocated;
	const int shortfall = std::max(0, granted - supply);
	if (shortfall != s.lastShortfall) {
		if (shortfall > 0)
			LogEvent(s, AuthorFor(s, DEPT_ENGINEERING, "the bridge"), "engineering",
				"power commitments exceed the plant by " + std::to_string(shortfall) + " EPS; nothing is shed");
		else if (s.lastShortfall > 0)
			LogEvent(s, AuthorFor(s, DEPT_ENGINEERING, "the bridge"), "engineering",
				"power commitments fit the plant again");
		s.lastShortfall = shortfall;
	}

	// Output: what each system delivers, by the power reaching it against its full demand. A system
	// at forty per cent delivers forty per cent: almost-on is a real state (docs/power-assignment.md).
	for (int i = 0; i < SYS_COUNT; ++i) {
		System &sys = s.systems[i];
		const float powered = full[i] ? static_cast<float>(sys.allocated) / full[i] : 0.0f;
		const int need = SPECS[i].crewNeeded;
		// An unattended station still runs, at half effect: automation, not expertise.
		const float manning = need ? 0.5f + 0.5f * std::min(1.0f, sys.staffing / need) : 1.0f;
		// A hijacked system still draws the ship's power; what it does with it is no longer ours.
		sys.output = sys.control < HIJACKED ? 0.0f : sys.health * powered * manning;
	}

	// Deferred maintenance: a system whose station is undermanned wears out, and left long enough it
	// fails -- a failure the queue and the log trace to the work deferred (docs/crew-work.md). A
	// system its own watch keeps up never wears, so an ordinary watch is unchanged.
	for (int i = 0; i < SYS_COUNT; ++i) {
		System &sys = s.systems[i];
		const int need = SPECS[i].crewNeeded;
		if (need <= 0) continue;
		const float manning = std::min(1.0f, static_cast<float>(sys.manned) / need);
		if (manning >= 1.0f) { sys.wear = 0.0f; continue; }
		sys.wear += (1.0f - manning) * days / MAINTENANCE_DAYS;
		if (sys.wear >= 1.0f) {
			sys.wear = 0.0f;
			sys.health = Clamp01(sys.health - 0.4f);
			LogEvent(s, AuthorFor(s, DEPT_ENGINEERING, "damage control"), "engineering", std::string(SPECS[i].name) + " failed for want of maintenance");
		}
	}

	// The replicators restock medical supplies (the triage gap's control): while they run, the ward's
	// shelves refill; dark, broken or stood down, they do not. Read here, after the outputs are set,
	// so the restock follows the same power the simulation just decided.
	if (s.systems[SYS_REPLICATORS].output > 0.0f)
		s.stores.medicalSupplies = std::min(100.0f, s.stores.medicalSupplies + REPLICATE_SUPPLIES_PER_HOUR * shipSeconds / 3600.0f);
}

// ---- allocation: the player and the crew decide (docs/power-assignment.md) ---------------------

// What a plant can give the commitments: the sources that are online and intact. It is the number
// the console measures a new commitment against, so "the console refuses to accept more" is exact.
static int PlantCapacity(const Ship &s)
{
	int n = 0;
	for (int i = 0; i < SRC_COUNT; ++i)
		if (s.sources[i].online && s.sources[i].health > 0.0f) n += SourceCapacity(s, static_cast<SourceId>(i));
	return n;
}

// The commitment a share asks of the plant, and the full demand it is a fraction of.
static int FullDemand(const Ship &s, SystemId id, bool &on)
{
	if (id >= SYS_COUNT) { on = false; return 0; }
	const System &sys = s.systems[id];
	const bool coreless = Coreless(s);
	on = sys.enabled && sys.health > 0.0f && !SuppressedByAlert(s.alert, id) && (!coreless || CorelessVital(id));
	return on ? EffectiveDemand(s, id) : 0;
}

static int CommitOf(const Ship &s, SystemId id)
{
	bool on = false;
	const int full = FullDemand(s, id, on);
	return on ? static_cast<int>(full * Clamp01(s.systems[id].share) + 0.5f) : 0;
}

int PowerCommitted(const Ship &s)
{
	int n = 0;
	for (int i = 0; i < SYS_COUNT; ++i) n += s.systems[i].allocated;
	return n;
}

int PowerShortfall(const Ship &s)
{
	return std::max(0, PowerCommitted(s) - s.PowerAvailable());
}

void SetPowerAuto(Ship &s, bool on)
{
	if (s.powerAuto == on) return;
	s.powerAuto = on;
	if (on)
		LogEvent(s, AuthorFor(s, DEPT_ENGINEERING, "the Chief Engineer"), "engineering",
			"automatic power allocation is on: the ship's own ladder decides the systems nobody has set");
	else
		LogEvent(s, AuthorFor(s, DEPT_ENGINEERING, "the Chief Engineer"), "engineering",
			"automatic power allocation is off: what the player and crew have set stands, in full");
}

bool PowerAuto(const Ship &s) { return s.powerAuto; }

// Would committing `percent` to this system keep the whole commitment within the plant? A decrease
// is always allowed; an increase that would not fit is refused so the console cannot be driven into
// a deficit by a hand, though the plant may fall under an existing commitment on its own, which is
// the shortfall the console reports (docs/power-assignment.md).
static bool WouldFit(const Ship &s, SystemId id, int percent)
{
	int want = 0;
	bool on = false;
	const int full = FullDemand(s, id, on);
	if (full) want = static_cast<int>(full * (percent / 100.0f) + 0.5f);
	for (int i = 0; i < SYS_COUNT; ++i) {
		if (static_cast<SystemId>(i) == id) { want = on ? want : 0; continue; }
		want += CommitOf(s, static_cast<SystemId>(i));
	}
	return want <= PlantCapacity(s);
}

bool SetAllocation(Ship &s, SystemId id, int percent)
{
	if (id >= SYS_COUNT || percent < 0 || percent > 100) return false;
	bool on = false;
	FullDemand(s, id, on);
	if (!on) return false;                       // a dark system has no allocation to set
	if (percent > AllocationPercent(s, id) && !WouldFit(s, id, percent)) return false; // refuses more
	s.systems[id].share = percent / 100.0f;
	s.systems[id].allocBy = ALLOC_PLAYER;
	s.systems[id].allocCrew = s.player;
	s.systems[id].allocDamagedOff = false;
	Tick(s, 0.0f); // the console reads the result immediately
	return true;
}

bool SetAllocationBy(Ship &s, SystemId id, int percent, int officer)
{
	if (id >= SYS_COUNT || percent < 0 || percent > 100) return false;
	if (officer < 0 || officer >= static_cast<int>(s.crew.size())) return false;
	bool granted = false;
	for (const BandGrant &g : s.bandGrants)
		if (g.grantee == officer && static_cast<BudgetBand>(g.band) == BandOf(id)) { granted = true; break; }
	if (!granted) return false; // the authority to decide this band is not theirs
	bool on = false;
	FullDemand(s, id, on);
	if (!on) return false;
	if (percent > AllocationPercent(s, id) && !WouldFit(s, id, percent)) return false;
	s.systems[id].share = percent / 100.0f;
	s.systems[id].allocBy = ALLOC_DELEGATE;
	s.systems[id].allocCrew = officer;
	s.systems[id].allocDamagedOff = false;
	Tick(s, 0.0f);
	return true;
}

int AllocationPercent(const Ship &s, SystemId id)
{
	if (id >= SYS_COUNT) return 0;
	return static_cast<int>(Clamp01(s.systems[id].share) * 100.0f + 0.5f);
}

uint8_t AllocationSource(const Ship &s, SystemId id)
{
	if (id >= SYS_COUNT) return ALLOC_UNSET;
	const System &sys = s.systems[id];
	if (PersonSet(sys.allocBy)) return sys.allocBy;
	// Nobody has set it: the ladder fed it (automatic mode) or it stands at its default.
	if (s.powerAuto && sys.allocated > 0) return ALLOC_AUTO;
	return ALLOC_UNSET;
}

std::string AllocationProvenance(const Ship &s, SystemId id)
{
	const uint8_t by = AllocationSource(s, id);
	if (by == ALLOC_DELEGATE) {
		const System &sys = s.systems[id];
		const std::string who = (sys.allocCrew >= 0 && sys.allocCrew < static_cast<int>(s.crew.size()))
			? s.crew[sys.allocCrew].name : std::string("an officer");
		return "an officer (" + who + ") under standing orders";
	}
	return AllocationByName(by);
}

Recommendation RecommendAllocation(Ship &s)
{
	Recommendation rec;
	rec.by = DepartmentHead(s, DEPT_ENGINEERING);
	rec.pending = true;
	// The ladder's arithmetic at the plant we have: what it can give, spent in the order the chief
	// keeps power in -- the cheapest first and the way home last (docs/budget-squaring.md).
	bool on[SYS_COUNT];
	int full[SYS_COUNT];
	int order[SYS_COUNT];
	int totalFull = 0;
	for (int i = 0; i < SYS_COUNT; ++i) {
		order[i] = i;
		full[i] = FullDemand(s, static_cast<SystemId>(i), on[i]);
		totalFull += full[i];
	}
	int budget = PlantCapacity(s);
	if (budget > totalFull) budget = totalFull;
	std::stable_sort(order, order + SYS_COUNT, [&](int a, int b) { return s.systems[a].priority < s.systems[b].priority; });
	int left = budget;
	int shedCount = 0;
	for (int k = 0; k < SYS_COUNT; ++k) {
		const int i = order[k];
		rec.percent[i] = 0;
		if (full[i] <= 0) continue;
		const int got = std::max(0, std::min(full[i], left));
		left -= got;
		rec.percent[i] = static_cast<int>(static_cast<float>(got) / full[i] * 100.0f + 0.5f);
		if (got <= 0) ++shedCount;
	}
	const std::string who = rec.by >= 0 ? s.crew[rec.by].name : std::string("the Chief Engineer");
	rec.reasoning = who + " recommends: the cheapest first and the way home last";
	if (budget < totalFull)
		rec.reasoning += " - " + std::to_string(totalFull - budget) + " EPS short, so "
			+ std::to_string(shedCount) + " system(s) would go dark";
	else
		rec.reasoning += "; the plant covers every system whole";
	return rec;
}

bool AcceptRecommendation(Ship &s)
{
	const Recommendation rec = RecommendAllocation(s);
	if (rec.by < 0) return false;
	for (int i = 0; i < SYS_COUNT; ++i) {
		s.systems[i].share = rec.percent[i] / 100.0f;
		s.systems[i].allocBy = ALLOC_PLAYER;
		s.systems[i].allocCrew = rec.by;
		s.systems[i].allocDamagedOff = false;
	}
	LogEvent(s, s.crew[rec.by].name, "engineering",
		"the Chief Engineer's allocation is adopted; the player sets it as the ship's");
	Tick(s, 0.0f);
	return true;
}

bool RefuseRecommendation(Ship &s)
{
	const int chief = RecommendAllocation(s).by;
	if (chief < 0) return false;
	// A refusal is recorded, and the officer remembers it (docs/the-record-and-the-log.md): a mark
	// against whoever refused, so the bond carries it and the console can show it.
	if (chief >= 0 && chief < static_cast<int>(s.crew.size())) {
		Remember(s, chief, MEM_OVERRULED, s.player, MEM_SAW, -0.5f);
		LogEvent(s, s.crew[chief].name, "engineering",
			s.crew[chief].name + "'s allocation was refused; he notes who refused it and holds to his own");
	} else {
		LogEvent(s, "the Chief Engineer", "engineering",
			"the Chief Engineer's allocation was refused; he notes it");
	}
	return true;
}

bool GrantBand(Ship &s, int grantor, int grantee, BudgetBand band)
{
	if (band >= BAND_COUNT) return false;
	if (grantor < 0 || grantor >= static_cast<int>(s.crew.size())) return false;
	if (grantee < 0 || grantee >= static_cast<int>(s.crew.size())) return false;
	if (s.crew[grantee].status != CREW_FIT) return false;
	for (const BandGrant &g : s.bandGrants)
		if (g.grantee == grantee && static_cast<BudgetBand>(g.band) == band) return false; // already held
	for (const BandGrant &g : s.bandGrants)
		if (static_cast<BudgetBand>(g.band) == band) return false; // one holder per band
	BandGrant g;
	g.grantor = grantor; g.grantee = grantee; g.band = band; g.granted = s.clock;
	s.bandGrants.push_back(g);
	if (static_cast<int>(s.bandGrants.size()) > GRANT_MAX) s.bandGrants.erase(s.bandGrants.begin());
	LogEvent(s, s.crew[grantor].name, "command",
		s.crew[grantee].name + " holds standing authority over " + BandName(band) + " until it is revoked");
	return true;
}

bool RevokeBand(Ship &s, int grantee, BudgetBand band)
{
	for (size_t i = 0; i < s.bandGrants.size(); ++i) {
		if (s.bandGrants[i].grantee == grantee && static_cast<BudgetBand>(s.bandGrants[i].band) == band) {
			const std::string who = s.crew[grantee].name;
			s.bandGrants.erase(s.bandGrants.begin() + i);
			// The revocation takes effect immediately: the authority ends this tick, and the log says so.
			LogEvent(s, who, "command", who + "'s standing authority over " + BandName(band) + " is revoked, at once");
			return true;
		}
	}
	return false;
}

const BandGrant *BandHolder(const Ship &s, BudgetBand band)
{
	for (const BandGrant &g : s.bandGrants)
		if (static_cast<BudgetBand>(g.band) == band) return &g;
	return nullptr;
}

int BandGrantCount(const Ship &s) { return static_cast<int>(s.bandGrants.size()); }

const char *ControllerName(uint8_t c)
{
	static const char *const NAMES[CTRL_COUNT] = { "crew", "borg", "contested", "sealed", "uninhabitable" };
	return c < CTRL_COUNT ? NAMES[c] : "?";
}

// Boarders: fight, hack, advance. Deterministic -- attrition is a rate, not a roll.
static void UpdateIntruders(Ship &s, float shipSeconds)
{
	const float minutes = shipSeconds / 60.0f;
	float arriving[DECKS] = {0};
	for (int d = 0; d < DECKS; ++d) {
		Deck &deck = s.decks[d];
		if (deck.intruders <= 0.0f) {
			deck.intruders = 0.0f;
			deck.dwell = 0.0f;
			// The clean intercept: the intruders are gone and no system here was ever touched. That
			// is a win, and it is recorded as one (docs/borg-incursion.md).
			if (deck.engaged) {
				deck.engaged = false;
				if (!deck.compromised) {
					++s.cleanIntercepts;
					std::string team;
					for (const CrewMember &c : s.crew)
						if (c.status == CREW_FIT && c.deck == d + 1 && c.dept == DEPT_SECURITY && c.activity == ACT_ON_DUTY)
							team += (team.empty() ? "" : ", ") + c.name;
					// The team's names go in the log's author field (the `what` is capped at 63 chars).
					LogEvent(s, team.empty() ? AuthorFor(s, DEPT_SECURITY, "security") : team, "security",
						"cleared the intruders on deck " + std::to_string(d + 1) + "; no systems compromised");
				}
			}
			continue;
		}
		deck.engaged = true;

		// The fight. Each side wears the other down at the same rate; boarders also die without air.
		const float before = deck.intruders;
		deck.intruders -= deck.defenders * minutes / FIGHT_MINUTES;
		if (deck.atmosphere < AIRLESS) deck.intruders -= before * minutes / (EXPOSURE_KILLS / 60.0f);
		if (deck.defenders > 0) {
			// what the boarders do to the defenders: wounds, one defender at a time until each is out of it
			float hurt = std::min(before, static_cast<float>(deck.defenders)) * minutes / FIGHT_MINUTES;
			for (CrewMember &c : s.crew) {
				if (hurt <= 0.0f) break;
				if (c.status != CREW_FIT || c.deck != d + 1 || c.dept != DEPT_SECURITY || c.activity != ACT_ON_DUTY) continue;
				const float taken = std::min(hurt, 1.0f - c.wounds);
				c.wounds += taken;
				hurt -= taken;
				if (c.wounds >= 1.0f) { c.status = CREW_INJURED; c.severity = std::max(c.severity, 0.5f); LogEvent(s, AuthorFor(s, DEPT_MEDICAL, "sickbay"), "sickbay", c.name + " wounded fighting boarders"); }
			}
		}
		// the last of a party that is being killed does not linger as a fraction
		if (deck.intruders < 0.05f && (deck.defenders > 0 || deck.atmosphere < AIRLESS)) deck.intruders = 0.0f;
		if (deck.intruders <= 0.0f) { deck.intruders = 0.0f; continue; }

		// Those not pinned by defenders work on the systems stationed here, sharing themselves out.
		const float free_ = std::max(0.0f, deck.intruders - deck.defenders);
		// A force field holds them: while it stands they cannot reach a system or advance, and it
		// drains under their pressure. A level-10 field cuts a drone from the Collective
		// (docs/borg-incursion.md): while it holds a Borg deck, the Collective cannot adapt.
		if (deck.forceFieldLevel > 0.0f) {
			if (deck.borg && deck.forceFieldLevel >= static_cast<float>(FIELD_MAX)) s.adaptationSuppressed = std::max(s.adaptationSuppressed, 1.0f);
			deck.forceFieldLevel = std::max(0.0f, deck.forceFieldLevel - free_ * minutes / FIELD_HOLD_MINUTES);
			if (deck.forceFieldLevel <= 0.0f) {
				deck.forceFieldLevel = 0.0f; deck.forceField = false;
				LogEvent(s, AuthorFor(s, DEPT_ENGINEERING, "environmental control"), "hull", "the force field on deck " + std::to_string(d + 1) + " fails");
			}
			deck.dwell = 0.0f; // contained, not holding ground
			continue; // held: no hack, no advance this tick
		}
		// How long the intruders have held unopposed: the clock against which the intercept races.
		if (free_ > 0.0f) deck.dwell += shipSeconds; else deck.dwell = 0.0f;
		// Hunters come for the crew whatever else is on the deck: they wound who they find, held or not.
		if (deck.boarderKind == BOARDER_HUNTER) {
			float hunting = free_ * minutes / FIGHT_MINUTES;
			for (CrewMember &c : s.crew) {
				if (hunting <= 0.0f) break;
				if (c.status != CREW_FIT || c.deck != d + 1) continue;
				const float taken = std::min(hunting, 1.0f - c.wounds);
				c.wounds += taken;
				hunting -= taken;
				if (c.wounds >= 1.0f) { c.status = CREW_INJURED; c.severity = std::max(c.severity, 0.5f); LogEvent(s, AuthorFor(s, DEPT_MEDICAL, "sickbay"), "sickbay", c.name + " wounded by hunters"); }
			}
		}
		// (a system already wholly theirs, a dark console and a wreck are not work)
		auto workable = [&](int i) {
			return SPECS[i].deck == d + 1 && s.systems[i].enabled && s.systems[i].health > 0.0f && s.systems[i].control > 0.0f;
		};
		int here = 0;
		for (int i = 0; i < SYS_COUNT; ++i)
			if (workable(i)) ++here;
		// No dwell, no write: below the threshold the intruders are in the room but the systems are
		// not theirs yet. Security reaching them inside the window is a clean intercept.
		if (here > 0 && deck.dwell >= DWELL_COMPROMISE) {
			for (int i = 0; i < SYS_COUNT; ++i) {
				System &sys = s.systems[i];
				if (!workable(i)) continue;
				sys.control = std::max(0.0f, sys.control - (free_ / here) * minutes / HACK_MINUTES);
			}
		} else if (here > 0) {
			// in the wiring, not yet writing: nothing happens to the ship until the threshold
		}
		// A system on this deck no longer at full control, or assimilation under way, means an
		// intruder has written here: the deck is compromised, and no intercept for it is "clean".
		for (int i = 0; i < SYS_COUNT; ++i) if (SPECS[i].deck == d + 1 && s.systems[i].control < 0.999f) deck.compromised = true;
		if (deck.assimilated > 0.0f) deck.compromised = true;
		// The third threshold: held this long, and (for anyone but the Borg, whose path is
		// assimilation) the deck's systems are seized outright (docs/borg-incursion.md).
		if (deck.dwell >= DWELL_SEIZE && !deck.borg)
			for (int i = 0; i < SYS_COUNT; ++i) if (SPECS[i].deck == d + 1 && s.systems[i].enabled) s.systems[i].control = 0.0f;

		if (here == 0 && deck.defenders == 0) {
			// Nothing to take here and nobody stopping them: on toward their objective -- the deck they
			// were sent for, or the nearest of the bridge or Engineering.
			const int target = deck.objective ? deck.objective
				: (std::abs(d + 1 - BRIDGE_DECK) <= std::abs(d + 1 - ENGINEERING_DECK) ? BRIDGE_DECK : ENGINEERING_DECK);
			if (target != d + 1) {
				// the party moves off a few at a time, and its last stragglers go together
				const float moving = deck.intruders < 0.05f ? deck.intruders : std::min(deck.intruders, deck.intruders * minutes / ADVANCE_MINUTES);
				deck.intruders -= moving;
				arriving[d + (target > d + 1 ? 1 : -1)] += moving;
			} else if (deck.boarderKind == BOARDER_RAIDER) {
				// Arrived with nothing left to take: raiders loot the ship's stores instead.
				const float taking = std::min(deck.intruders, 1.0f) * minutes / HACK_MINUTES;
				s.stores.spareParts = std::max(0.0f, s.stores.spareParts - taking * 4.0f);
				s.stores.materials = std::max(0.0f, s.stores.materials - taking * 4.0f);
			}
		}
	}
	for (int d = 0; d < DECKS; ++d) s.decks[d].intruders += arriving[d];

	// The Borg: convert the deck, take the crew on it, and hold its systems outright.
	for (int d = 0; d < DECKS; ++d) {
		Deck &deck = s.decks[d];
		if (deck.intruders <= 0.0f) deck.borg = false;
		const float drones = deck.borg ? std::max(0.0f, deck.intruders - deck.defenders) : 0.0f;
		// No dwell, no write: the drones must hold the deck unopposed past the threshold before the
		// Collective learns, the compartment turns, or anyone on it is taken.
		if (drones > 0.0f && deck.dwell >= DWELL_COMPROMISE) {
			s.borgAwareness = std::min(1.0f, s.borgAwareness + drones * minutes * 0.0005f); // the Collective learns
			deck.assimilated = std::min(1.0f, deck.assimilated + drones * minutes / (ASSIMILATE_DECK_HOURS * 60.0f));
			float taking = drones * minutes / ASSIMILATE_CREW_MINUTES;
			for (CrewMember &c : s.crew) {
				if (taking <= 0.0f) break;
				if (c.status != CREW_FIT && c.status != CREW_INJURED) continue;
				if (c.deck != d + 1) continue;
				const float taken = std::min(taking, 1.0f - c.wounds);
				c.wounds += taken;
				taking -= taken;
				if (c.wounds >= 1.0f) {
					c.status = CREW_ASSIMILATED; // one of ours is now one of theirs
					c.deck = 0;
					deck.intruders += 1.0f;
					LogEvent(s, AuthorFor(s, DEPT_SECURITY, "security"), "sickbay", c.name + " taken by the Borg");
				}
			}
		}
		// Stripping it back: hours and parts, once the drones are gone.
		if (deck.stripping > 0 && deck.assimilated > 0.0f && s.stores.spareParts > 0.0f) {
			float gain = deck.stripping * minutes / (STRIP_HOURS_PER_DECK * 60.0f);
			gain = std::min(gain, deck.assimilated);
			gain = std::min(gain, s.stores.spareParts / PARTS_PER_DECK);
			deck.assimilated -= gain;
			s.stores.spareParts = std::max(0.0f, s.stores.spareParts - gain * PARTS_PER_DECK);
			if (deck.assimilated < 1e-4f) deck.assimilated = 0.0f;
		}
		if (deck.assimilated >= ASSIMILATED)
			for (int i = 0; i < SYS_COUNT; ++i)
				if (SPECS[i].deck == d + 1) s.systems[i].control = 0.0f;
	}

	// The crew at a station win it back when nobody is contesting it; a powered-down system is frozen.
	for (int i = 0; i < SYS_COUNT; ++i) {
		System &sys = s.systems[i];
		if (sys.control >= 1.0f || !sys.enabled) continue;
		const Deck &deck = s.decks[SPECS[i].deck - 1];
		if (deck.intruders - deck.defenders > 0.0f) continue;
		if (deck.assimilated >= ASSIMILATED) continue; // not until the deck is stripped
		const int need = SPECS[i].crewNeeded > 0 ? SPECS[i].crewNeeded : 1;
		const float crewShare = std::min(1.0f, sys.staffing / need);
		sys.control = std::min(1.0f, sys.control + crewShare * minutes / RETAKE_MINUTES);
	}

	// Who holds each deck (docs/borg-incursion.md): the field the deck adapter reads and the console shows.
	for (int d = 0; d < DECKS; ++d) {
		Deck &deck = s.decks[d];
		if (deck.assimilated >= ASSIMILATED) deck.controller = CTRL_BORG;
		// Contested only past the second threshold (they hold ground); before that the crew still
		// hold it, however the fight is going.
		else if (deck.intruders - deck.defenders > 0.0f && deck.dwell >= DWELL_HOLD) deck.controller = CTRL_CONTESTED;
		else if (deck.forceField) deck.controller = CTRL_SEALED;
		else if (deck.atmosphere < AIRLESS) deck.controller = CTRL_UNINHABITABLE;
		else deck.controller = CTRL_CREW;
	}
}

// ---- the outside ------------------------------------------------------------------------------

static void BuildSector(Ship &s, int number)
{
	uint32_t r = ((s.cfg.seed ? s.cfg.seed : 1) + static_cast<uint32_t>(number) * 2654435761u) * 2654435761u;
	auto next = [&r]() { r = r * 1664525u + 1013904223u; return r >> 9; };
	s.sector.assign(SECTOR_BEACONS, Beacon());
	// A chain from the first beacon to the last, so the sector can always be crossed, plus side links.
	for (int i = 0; i + 1 < SECTOR_BEACONS; ++i) {
		s.sector[i].links.push_back(i + 1);
		s.sector[i + 1].links.push_back(i);
	}
	for (int i = 0; i + 2 < SECTOR_BEACONS; ++i) {
		if (next() % 3) continue;
		s.sector[i].links.push_back(i + 2);
		s.sector[i + 2].links.push_back(i);
	}
	for (int i = 1; i < SECTOR_BEACONS; ++i) {
		const uint32_t roll = next() % 15;
		s.sector[i].kind = roll < 4 ? BEACON_EMPTY : roll < 7 ? BEACON_HOSTILE : roll < 9 ? BEACON_DERELICT
			: roll < 10 ? BEACON_BORG : roll < 11 ? BEACON_TRADER : roll < 12 ? BEACON_DISTRESS
			: roll < 13 ? BEACON_BELT : BEACON_PREWARP;
		// Borg strategic awareness: what the Collective knows makes more of the sector theirs.
		if (s.borgAwareness >= 0.4f && s.sector[i].kind == BEACON_HOSTILE && next() % 4 == 0) s.sector[i].kind = BEACON_BORG;
	}
	s.sector[0].kind = BEACON_EMPTY;
	s.sector[0].visited = true; // where the ship starts: nothing here
	s.sector[SECTOR_BEACONS - 1].kind = BEACON_GOAL; // the far end of the sector: an end to reach
	// A phenomenon (docs/exploration-and-science.md), placed on an empty interior beacon without
	// consuming the seed, so every sector has one anomaly to find and it changes nothing else.
	for (int i = 1; i + 1 < SECTOR_BEACONS; ++i)
		if (s.sector[i].kind == BEACON_EMPTY) {
			s.sector[i].phenomenon = true;
			// Three hidden attribute values, then the correct response as a function of them: the
			// truth can only be worked out once they are all resolved (docs/exploration-and-science.md).
			uint32_t h = s.cfg.seed * 2654435761u + static_cast<uint32_t>(i) * 40503u;
			uint32_t sum = 0;
			for (int k = 0; k < PHENOM_ATTR_COUNT; ++k) { h = h * 1664525u + 1013904223u; uint8_t v = static_cast<uint8_t>((h >> 16) % PHENOM_RESPONSE_COUNT); s.sector[i].phenomAttrVal[k] = v; sum += v; }
			s.sector[i].phenomTruth = static_cast<uint8_t>(sum % PHENOM_RESPONSE_COUNT);
			break;
		}
}

static void Arrive(Ship &s)
{
	Beacon &b = s.sector[s.beacon];
	const bool first = !b.visited;
	b.visited = true;
	s.enemy = Enemy();
	s.contact2 = Enemy();
	if (b.kind == BEACON_GOAL) s.reachedEnd = true; // the sector's far end
	if (!first) return; // what was here has been dealt with, or taken
	switch (b.kind) {
	case BEACON_HOSTILE:
	case BEACON_BORG:
		s.enemy.present = true;
		s.enemy.kind = b.kind == BEACON_BORG ? ENEMY_BORG_VESSEL : (s.beacon % 2 ? ENEMY_WARSHIP : ENEMY_RAIDER);
		s.enemy.borg = b.kind == BEACON_BORG;
		s.enemy.hull = 1.0f;
		s.enemy.shields = 1.0f;
		s.enemy.firepower = s.enemy.borg ? 0.5f : (s.enemy.kind == ENEMY_WARSHIP ? 0.4f : 0.25f);
		s.enemy.boarders = s.enemy.borg ? 4 : 3;
		// More than one at a time: some raiders have a wingman (deterministic by the beacon).
		if (b.kind == BEACON_HOSTILE && s.beacon % 3 == 0) {
			s.contact2 = s.enemy;
			s.contact2.firepower *= 0.6f;
			s.contact2.boarders = 0;
		}
		break;
	case BEACON_DERELICT:
		s.stores.spareParts += SALVAGE_PARTS;
		break;
	default:
		break;
	}
}

bool InCombat(const Ship &s) { return s.enemy.present && s.enemy.hull > 0.0f; }

void SetTarget(Ship &s, EnemySubsystem t) { if (t < TARGET_COUNT) s.target = t; }
EnemySubsystem Target(const Ship &s) { return s.target; }

// The phaser bank's setting: Tactical's standing decision, and the one the console carries when
// there is no contact to shoot at. A higher setting asks the same bank for more power and does more
// to what it hits; a lower one is for repelling boarders without killing the people aboard.
const char *PhaserYieldName(uint8_t y)
{
	static const char *const NAMES[YIELD_COUNT] = { "STUN", "HEAVY STUN", "KILL", "VAPORIZE" };
	return y < YIELD_COUNT ? NAMES[y] : "KILL";
}
bool SetPhaserYield(Ship &s, int y)
{
	if (y < 0 || y >= YIELD_COUNT) return false;
	if (s.stores.phaserYield == static_cast<uint8_t>(y)) return true;
	s.stores.phaserYield = static_cast<uint8_t>(y);
	LogEvent(s, AuthorFor(s, DEPT_SECURITY, "tactical"), "tactical", "the phaser bank is set to " + std::string(PhaserYieldName(s.stores.phaserYield)));
	return true;
}
uint8_t PhaserYieldOf(const Ship &s) { return s.stores.phaserYield < YIELD_COUNT ? s.stores.phaserYield : static_cast<uint8_t>(YIELD_KILL); }

// What the phaser setting costs in power, as a percentage of the bank's nominal demand: a stun shot
// is cheap, a vaporize shot is not. [our call] the ladder.
static int PhaserYieldPowerPercent(uint8_t y)
{
	static const int PCT[YIELD_COUNT] = { 100, 125, 150, 175 };
	return y < YIELD_COUNT ? PCT[y] : PCT[YIELD_KILL];
}
// What the phaser setting does to what it hits, as a multiplier on the bank's output. [our call].
static float PhaserYieldFactor(uint8_t y)
{
	static const float FACTOR[YIELD_COUNT] = { 0.4f, 0.7f, 1.0f, 1.35f };
	return y < YIELD_COUNT ? FACTOR[y] : FACTOR[YIELD_KILL];
}

int EffectiveDemand(const Ship &s, SystemId id)
{
	if (id >= SYS_COUNT) return 0;
	const int base = SPECS[id].demand;
	if (id == SYS_PHASERS) return base * PhaserYieldPowerPercent(PhaserYieldOf(s)) / 100;
	return base;
}

const char *EnemySubsystemName(EnemySubsystem t)
{
	static const char *const NAMES[TARGET_COUNT] = {"the hull", "its weapons", "its engines", "its shields"};
	return t < TARGET_COUNT ? NAMES[t] : "the hull";
}

const char *EnemyKindName(EnemyKind k)
{
	static const char *const NAMES[ENEMY_KIND_COUNT] = {"a raider", "a warship", "a Borg vessel"};
	return k < ENEMY_KIND_COUNT ? NAMES[k] : "a raider";
}

const char *BoarderKindName(BoarderKind k)
{
	static const char *const NAMES[BOARDER_KIND_COUNT] = {"a raider", "a drone", "a hunter"};
	return k < BOARDER_KIND_COUNT ? NAMES[k] : "a boarder";
}

bool Pursued(const Ship &s) { return s.pursued; }
int PursuitJumps(const Ship &s) { return s.pursuitJumps; }
bool AtEnd(const Ship &s) { return s.reachedEnd; }
bool Won(const Ship &s) { return s.won; }
int SectorNumber(const Ship &s) { return s.sectorNumber; }

// The one place a beacon kind is named, so a new kind cannot index a stale table.
const char *BeaconKindName(BeaconKind k)
{
	static const char *const NAMES[BEACON_KIND_COUNT] = {
		"open space", "a hostile ship", "a derelict", "the Borg", "a trader", "a distress call",
		"a resource belt", "a pre-warp civilisation", "the sector's end"
	};
	return k < BEACON_KIND_COUNT ? NAMES[k] : "the site";
}

// The site the ship is at, as the transporter and the log name it. The kind is the survey's, so a
// site nobody has read is "an uncharted contact".
static std::string SiteName(const Ship &s)
{
	if (s.sector.empty() || s.beacon < 0 || s.beacon >= static_cast<int>(s.sector.size())) return "the site";
	const Beacon &b = s.sector[s.beacon];
	if (!b.visited && !b.surveyed) return "an uncharted contact";
	return BeaconKindName(b.kind);
}

// The transporter (S4): a party beamed to the site, and back. The party are fit crew, and the captain
// stays with the ship. A transporter cannot reach through our own shields, so the shields must be
// down -- a real cost to beaming mid-fight.
//
// The transporter is the system the ruling of 2026-10-06 was written about, so it is worked end to
// end here: the pattern buffer is the condition, the load is the alert the beam is made under, and
// the severity lands on the crew records. Everything below the general mechanism is the outcome
// family of docs/failure-is-content.md -- misaligned beam, mangled arrival, copy, merge.
std::string TransporterConditionLine(const Ship &s)
{
	const float condition = SystemCondition(s.systems[SYS_TRANSPORTERS]);
	const int pct = static_cast<int>(condition * 100.0f + 0.5f);
	const int advise = static_cast<int>(TRANSPORTER_ADVISE * 100.0f + 0.5f);
	std::string line = "the pattern buffer is at " + std::to_string(pct) + "%";
	if (condition >= NOMINAL_CONDITION) line += ", nominal: I would send anyone";
	else line += ", and below " + std::to_string(advise) + " I would not send anyone";
	return line;
}

// The beam's draw. The general mechanism decides whether this use is clean; the transporter's own
// outcome family gives a bad draw its face.
static uint8_t TransporterRisk(Ship &s)
{
	const std::string who = AuthorFor(s, DEPT_ENGINEERING, "the transporter room");
	return UseSystem(s, SYS_TRANSPORTERS, StressNow(s), who);
}

// What a bad draw does to the person in the beam. A degraded beam hurts; an acute one mangles; a
// catastrophic one gets the pattern wrong -- a copy, which is a new record and a legal question, or
// a merge, which leaves one where there were two. Written to the log and the records.
static std::string TransporterOutcome(Ship &s, bool away, uint8_t severity)
{
	if (severity == ANOMALY_NONE) return std::string();
	int target = -1;
	for (int i = 0; i < static_cast<int>(s.crew.size()); ++i) {
		const CrewMember &c = s.crew[i];
		if (c.status != CREW_FIT) continue;
		if (away && !c.away) continue;
		target = i; break; // first fit in roster order: deterministic
	}
	if (target < 0) return std::string();
	CrewMember &c = s.crew[target];
	if (severity == ANOMALY_DEGRADED) {
		c.status = CREW_INJURED;
		c.severity = std::max(c.severity, 0.3f);
		LogEvent(s, "the transporter room", "sickbay", c.name + " arrived from a misaligned beam, hurt");
		return c.name + " arrived from a misaligned beam, hurt";
	}
	if (severity == ANOMALY_ACUTE) {
		c.status = CREW_INJURED;
		c.severity = std::max(c.severity, 0.7f);
		LogEvent(s, "the transporter room", "sickbay", c.name + " was mangled in transit: a sickbay problem");
		return c.name + " was mangled in transit: a sickbay problem";
	}
	// Catastrophic: the pattern is not merely hurt, it is wrong, and which way is the draw's next half.
	const bool copied = (AnomalyRoll(s.riskRolls++, s.cfg.seed) & 1u) == 0u;
	if (copied && static_cast<int>(s.crew.size()) < COMPLEMENT + MAX_DUPLICATES) {
		const std::string name = c.name; // before push_back can reallocate the vector
		CrewMember dup = c;              // the same face, and every relationship they had now doubled
		dup.memories.clear();            // a new person: no shared history
		// A copy is not the person: the record is new, the legal status ambiguous, and command must decide.
		LogEvent(s, "the transporter room", "crew", "a second " + name + " materialised: a copy, and not the person");
		LogEvent(s, "the transporter room", "crew", name + "'s record is doubled: a legal question command must decide");
		s.crew.push_back(dup);
		return "a copy of " + name + " materialised";
	}
	// A merge: two become one, and the one who comes back is not either of them.
	int other = -1;
	for (int i = target + 1; i < static_cast<int>(s.crew.size()); ++i) {
		const CrewMember &o = s.crew[i];
		if (o.status != CREW_FIT) continue;
		if (away && !o.away) continue;
		other = i; break;
	}
	if (other < 0) { // nobody to merge with: it is a mangling after all
		c.status = CREW_INJURED;
		c.severity = std::max(c.severity, 0.7f);
		LogEvent(s, "the transporter room", "sickbay", c.name + " was mangled in transit: a sickbay problem");
		return c.name + " was mangled in transit: a sickbay problem";
	}
	CrewMember &o = s.crew[other];
	const std::string a = c.name, b = o.name;
	o.status = CREW_DEAD;
	LogEvent(s, "the transporter room", "crew",
		a + " and " + b + " were merged into one: " + b + " is gone, and " + a + " is not either of them");
	NoteDeath(s, other);
	return a + " and " + b + " merged";
}

bool TransportAway(Ship &s, int party, std::string *outcome)
{
	if (party <= 0) return false;
	if (s.awayBeacon >= 0) return false;                                  // a party is already down there
	if (s.systems[SYS_TRANSPORTERS].output <= 0.0f) return false;         // no transporter, no beam
	if (s.shieldStrength > 0.0f) return false;                            // we cannot beam through our own shields
	int sent = 0;
	for (CrewMember &c : s.crew) {
		if (sent >= party) break;
		if (c.status != CREW_FIT || c.away || c.rank >= 6) continue;      // not the captain
		c.away = true;
		c.deck = 0;
		++sent;
	}
	if (!sent) return false;
	s.awayBeacon = s.beacon;
	LogEvent(s, "the transporter room", "outside",
		"away team of " + std::to_string(sent) + " beamed to " + SiteName(s));
	// The mechanism: the pattern buffer sets the odds, and the alert the beam is made under sets the
	// severity. A beam on a system in its top tenth is nominal and mangles no one.
	const uint8_t severity = TransporterRisk(s);
	const std::string account = TransporterOutcome(s, true, severity);
	if (outcome) *outcome = account;
	// The site itself can be the injury: a phenomenon's environment, or a belt's rock
	// (docs/failure-is-content.md's "poisoned site").
	const Beacon &b = s.sector[s.beacon];
	if (b.phenomenon || b.kind == BEACON_BELT) {
		for (CrewMember &c : s.crew) {
			if (!c.away || c.status != CREW_FIT) continue;
			c.status = CREW_INJURED;
			c.severity = std::max(c.severity, 0.4f);
			LogEvent(s, AuthorFor(s, DEPT_MEDICAL, "sickbay"), "sickbay", c.name + " was hurt by the site");
			break;
		}
	}
	return true;
}

bool TransportBack(Ship &s, std::string *outcome)
{
	if (s.awayBeacon < 0) return false;
	// The beam back is a use too: the same mechanism, the same pattern buffer.
	const uint8_t severity = TransporterRisk(s);
	const std::string account = TransporterOutcome(s, true, severity);
	if (outcome) *outcome = account;
	int back = 0;
	for (CrewMember &c : s.crew)
		if (c.away) { c.away = false; ++back; }
	s.awayBeacon = -1;
	LogEvent(s, "the transporter room", "outside", "away team of " + std::to_string(back) + " beamed back aboard");
	return back > 0;
}

int AwayTeam(const Ship &s)
{
	int n = 0;
	for (const CrewMember &c : s.crew) if (c.away) ++n;
	return n;
}

// Astrometrics (S4): the sensor survey the chart is built from. It reads the beacons one jump away
// and marks them known, whether or not the ship has been there. The tricorder's Scan is the same
// kind of reading at human scale; this is the ship's own eyes.
int Survey(Ship &s)
{
	if (s.systems[SYS_SENSORS].output <= 0.0f || s.sector.empty()) return 0;
	int added = 0;
	if (!s.sector[s.beacon].surveyed) { s.sector[s.beacon].surveyed = true; ++added; }
	for (int l : s.sector[s.beacon].links)
		if (!s.sector[l].surveyed) { s.sector[l].surveyed = true; ++added; }
	LogEvent(s, "astrometrics", "outside",
		added ? std::to_string(added) + " contact(s) surveyed" : std::string("the survey adds nothing new"));
	return added;
}

// A phenomenon (docs/exploration-and-science.md): hidden attributes, revealed one scan at a time.
const char *PhenomenonResponseName(int response)
{
	static const char *const NAMES[PHENOM_RESPONSE_COUNT] = { "shield harmonics", "warp geometry", "distance", "do not touch" };
	return response >= 0 && response < PHENOM_RESPONSE_COUNT ? NAMES[response] : "no response";
}

// The value an attribute resolved to: the crew read these and work the response out from them.
static const char *PhenomenonAttributeName(int value)
{
	static const char *const NAMES[PHENOM_RESPONSE_COUNT] = { "gravimetric", "subspace", "temporal", "tetryon" };
	return value >= 0 && value < PHENOM_RESPONSE_COUNT ? NAMES[value] : "unknown";
}

int RevealPhenomenon(Ship &s)
{
	if (s.sector.empty() || s.systems[SYS_SENSORS].output <= 0.0f) return -1;
	Beacon &b = s.sector[s.beacon];
	if (!b.phenomenon) return -1;
	int revealed = 0;
	for (int i = 0; i < PHENOM_ATTR_COUNT; ++i) if (b.phenomAttrs & (1u << i)) ++revealed;
	if (revealed >= PHENOM_ATTR_COUNT) return revealed; // already fully scanned
	b.phenomAttrs = static_cast<uint8_t>(b.phenomAttrs | (1u << revealed));
	LogEvent(s, AuthorFor(s, DEPT_SCIENCES, "astrometrics"), "outside",
		std::string("a phenomenon: attribute ") + std::to_string(revealed + 1) + " of " + std::to_string(PHENOM_ATTR_COUNT)
		+ " is " + PhenomenonAttributeName(b.phenomAttrVal[revealed]));
	return revealed + 1;
}

bool RespondPhenomenon(Ship &s, int response)
{
	if (s.sector.empty() || response < 0 || response >= PHENOM_RESPONSE_COUNT) return false;
	Beacon &b = s.sector[s.beacon];
	if (!b.phenomenon) return false;
	if (response == b.phenomTruth) {
		b.phenomAttrs = static_cast<uint8_t>((1u << PHENOM_ATTR_COUNT) - 1u); // fully understood
		s.stores.materials += PHENOM_MATERIALS;
		LogEvent(s, AuthorFor(s, DEPT_SCIENCES, "the lab"), "outside",
			std::string("the phenomenon answers to ") + PhenomenonResponseName(response) + ": science data and material");
		return true;
	}
	// Misread: a number the crew watch getting worse.
	const int deck = static_cast<int>((s.hits + static_cast<uint32_t>(response)) % DECKS) + 1;
	BreachDeck(s, deck, 0.3f);
	LogEvent(s, AuthorFor(s, DEPT_SCIENCES, "the lab"), "hull",
		std::string("the phenomenon lashes out at ") + PhenomenonResponseName(response) + ": deck " + std::to_string(deck) + " holed");
	return false;
}

// The Conn's course (S4): the shortest route over the jump links, and setting it.
std::vector<int> PlotCourse(const Ship &s, int toBeacon)
{
	std::vector<int> none;
	if (s.sector.empty() || toBeacon < 0 || toBeacon >= static_cast<int>(s.sector.size())) return none;
	std::vector<int> prev(s.sector.size(), -2);
	std::vector<int> queue;
	queue.push_back(s.beacon);
	prev[s.beacon] = -1;
	for (size_t i = 0; i < queue.size(); ++i) {
		const int u = queue[i];
		if (u == toBeacon) break;
		for (int l : s.sector[u].links)
			if (l >= 0 && l < static_cast<int>(s.sector.size()) && prev[l] == -2) { prev[l] = u; queue.push_back(l); }
	}
	if (prev[toBeacon] == -2) return none;
	std::vector<int> route;
	for (int at = toBeacon; at != -1; at = prev[at]) route.push_back(at);
	std::reverse(route.begin(), route.end());
	return route;
}

bool SetCourse(Ship &s, int toBeacon)
{
	if (PlotCourse(s, toBeacon).empty()) return false;
	s.course = toBeacon;
	LogEvent(s, AuthorFor(s, DEPT_COMMAND, "the conn"), "outside", "course laid in for beacon " + std::to_string(toBeacon));
	return true;
}

int Course(const Ship &s) { return s.course; }

void LoadAwayKit(Ship &s, int tricorders, int phasers, int evSuits, float charge)
{
	s.stores.tricorders = std::max(0, std::min(tricorders, 8));
	s.stores.phasers = std::max(0, std::min(phasers, 12));
	s.stores.evSuits = std::max(0, std::min(evSuits, 8));
	s.stores.tricorderCharge = Clamp01(charge);
	s.stores.kitCondition = 1.0f; // drawn fresh from the ship's locker
	LogEvent(s, "the transporter room", "outside", "away kit: " + std::to_string(s.stores.tricorders) + " tricorders, "
		+ std::to_string(s.stores.phasers) + " phasers, " + std::to_string(s.stores.evSuits) + " EV suits");
}

// A tricorder scan of a site. What it reports is the same kind of write the sensors make to the chart,
// over a smaller radius -- and a weak charge reports it wrong, so the reading cannot be trusted.
int Scan(Ship &s, int beacon)
{
	if (beacon < 0 || beacon >= static_cast<int>(s.sector.size())) return 0;
	if (s.stores.tricorders <= 0 || s.stores.tricorderCharge <= 0.0f) {
		LogEvent(s, "the away team", "outside", "the tricorder is dead: no scan");
		return 0;
	}
	s.stores.tricorderCharge = std::max(0.0f, s.stores.tricorderCharge - 0.1f);
	const bool reliable = s.stores.tricorderCharge >= 0.2f;
	int kind = s.sector[beacon].kind;
	if (!reliable) kind = (kind + 1) % BEACON_KIND_COUNT; // a weak charge reads the next thing along
	s.sector[beacon].visited = true;
	LogEvent(s, "the away team", "outside", std::string("scan: ") + BeaconKindName(static_cast<BeaconKind>(kind)) + (reliable ? "" : " (the tricorder is weak)"));
	return reliable ? 1 : 2;
}

// A tricorder reading of a compartment of the ship itself: the same kind of write the sensors make
// of a site, over a smaller radius. Wears the kit and its charge; a weak charge reads it wrong.
std::string ScanCompartment(Ship &s, int deck)
{
	if (deck < 1 || deck > DECKS) return "no such compartment";
	if (s.stores.tricorders <= 0 || s.stores.tricorderCharge <= 0.0f) {
		LogEvent(s, "the away team", "outside", "the tricorder is dead: no scan");
		return "the tricorder is dead";
	}
	s.stores.tricorderCharge = std::max(0.0f, s.stores.tricorderCharge - 0.05f);
	s.stores.kitCondition = std::max(0.0f, s.stores.kitCondition - 0.02f);
	const Deck &d = s.decks[deck - 1];
	const bool reliable = s.stores.tricorderCharge >= 0.2f;
	int air = static_cast<int>(d.atmosphere * 100.0f + 0.5f);
	if (!reliable) air = std::min(100, air + 15); // a weak charge reads the air better than it is
	char buf[160];
	std::snprintf(buf, sizeof(buf), "deck %d: air %d%%  hull %d%%  life support %s%s", deck, air,
		static_cast<int>(d.hull * 100.0f + 0.5f), s.systems[SYS_LIFE_SUPPORT].output > 0.0f ? "nominal" : "off",
		reliable ? "" : "  (the tricorder is weak)");
	LogEvent(s, "the away team", "outside", std::string("compartment scan: ") + buf);
	return buf;
}

static void StartFight(Ship &s, EnemyKind kind);

bool Jump(Ship &s, int toBeacon)
{
	if (s.sector.empty() || toBeacon < 0 || toBeacon >= static_cast<int>(s.sector.size())) return false;
	const std::vector<int> &links = s.sector[s.beacon].links;
	if (std::find(links.begin(), links.end(), toBeacon) == links.end()) return false;
	if (s.systems[SYS_WARP_DRIVE].output < 0.5f) return false;
	if (!PylonsIntact(s)) return false; // a ship without its nacelle pylons cannot go to warp
	if (s.dilithium <= MIN_WARP_DILITHIUM) return false; // no crystal, no warp: home stops getting closer
	if (s.stores.deuterium < JUMP_DEUTERIUM || s.stores.antimatter < JUMP_ANTIMATTER) return false;
	s.stores.deuterium -= JUMP_DEUTERIUM;
	s.stores.antimatter -= JUMP_ANTIMATTER;
	// Warp use spends the crystal; a better crystal makes each jump cheaper, so it stretches further.
	s.dilithium = std::max(0.0f, s.dilithium - JUMP_DILITHIUM / std::max(1.0f, s.crystalQuality));
	s.beacon = toBeacon;
	Arrive(s);
	// A raider on our tail closes one jump each time we run; when it catches us, it is a fight again.
	if (s.pursued) {
		if (--s.pursuitJumps <= 0) {
			s.pursued = false;
			if (!InCombat(s)) {
				StartFight(s, ENEMY_RAIDER);
				s.enemy.firepower = s.pursuitStrength > 0.0f ? s.pursuitStrength : 0.25f;
				LogEvent(s, AuthorFor(s, DEPT_SECURITY, "tactical"), "outside", "the raider has caught us");
			}
		}
	}
	// The travel systems: a weak deflector lets dust through, undamped, the jump shakes the crew.
	if (s.systems[SYS_NAV_DEFLECTOR].output <= TRACTOR_MIN) {
		const int deck = static_cast<int>((s.hits + 3u) % DECKS) + 1;
		BreachDeck(s, deck, 0.2f);
		LogEvent(s, AuthorFor(s, DEPT_ENGINEERING, "the conn"), "hull", "the deflector was weak: dust holed deck " + std::to_string(deck));
	}
	if (s.systems[SYS_INERTIAL_DAMPERS].output <= TRACTOR_MIN) {
		int shaken = 0;
		for (CrewMember &c : s.crew) {
			if (shaken >= 2) break;
			if (c.status != CREW_FIT) continue;
			c.status = CREW_INJURED;
			c.severity = std::max(c.severity, 0.4f);
			++shaken;
		}
		LogEvent(s, AuthorFor(s, DEPT_MEDICAL, "sickbay"), "sickbay", "the jump shook the crew: " + std::to_string(shaken) + " hurt");
	}
	LogEvent(s, AuthorFor(s, DEPT_COMMAND, "the conn"), "outside", "jumped to beacon " + std::to_string(toBeacon));
	return true;
}

bool FireTorpedo(Ship &s)
{
	if (!InCombat(s) || s.stores.torpedoes <= 0 || s.systems[SYS_TORPEDO_LAUNCHERS].output <= 0.0f) return false;
	--s.stores.torpedoes;
	// Shields take what they can of it; the rest reaches the hull. The Borg adapt to it like phasers.
	float dmg = TORPEDO_HULL;
	if (s.enemy.kind == ENEMY_BORG_VESSEL) {
		dmg *= (1.0f - s.enemy.adaptation);
		if (s.adaptationSuppressed <= 0.0f)
			s.enemy.adaptation = std::min(1.0f, s.enemy.adaptation + dmg * 1.5f);
	}
	const float absorbed = std::min(s.enemy.shields, dmg);
	s.enemy.shields -= absorbed;
	s.enemy.hull = std::max(0.0f, s.enemy.hull - (dmg - absorbed));
	LogEvent(s, AuthorFor(s, DEPT_SECURITY, "tactical"), "outside", std::string("fired a torpedo; enemy hull ") + (s.enemy.hull <= 0.0f ? "destroyed" : "holding"));
	return true;
}

// The phaser adapter's rotating modulation (docs/borg-incursion.md): break the Borg's lock so the
// next shots land again. It costs time and attention -- the crew cannot do it every second.
bool Remodulate(Ship &s)
{
	if (!InCombat(s) || s.remodulateCooldown > 0.0f) return false;
	if (s.enemy.adaptation <= 0.0f && (!s.contact2.present || s.contact2.adaptation <= 0.0f)) return false;
	s.enemy.adaptation = std::max(0.0f, s.enemy.adaptation - REMODULATE_ADAPTATION);
	if (s.contact2.present) s.contact2.adaptation = std::max(0.0f, s.contact2.adaptation - REMODULATE_ADAPTATION);
	s.remodulateCooldown = REMODULATE_COOLDOWN;
	LogEvent(s, AuthorFor(s, DEPT_SECURITY, "tactical"), "outside", "the phaser modulation is rotated; the Borg's lock is broken");
	return true;
}

// A vinculum raid: destroy the local coordination node and the Collective stops adapting for a
// while -- a reprieve with a cost, never a win (docs/borg-incursion.md). Needs Borg here, aboard or
// a vessel, and spends a security party.
bool RaidVinculum(Ship &s)
{
	bool borgAboard = false;
	for (const Deck &d : s.decks) if (d.borg && d.intruders > 0.0f) borgAboard = true;
	const bool borgVessel = s.enemy.present && s.enemy.borg;
	if (!borgAboard && !borgVessel) return false;
	if (s.adaptationSuppressed > 0.0f) return false;
	s.enemy.adaptation = 0.0f;
	if (s.contact2.present) s.contact2.adaptation = 0.0f;
	s.adaptationSuppressed = VINCULUM_SUPPRESS;
	for (CrewMember &c : s.crew)
		if (c.status == CREW_FIT && c.dept == DEPT_SECURITY) { c.status = CREW_INJURED; c.severity = std::max(c.severity, 0.5f); LogEvent(s, AuthorFor(s, DEPT_MEDICAL, "sickbay"), "sickbay", c.name + " was hurt in the raid on the vinculum"); break; }
	LogEvent(s, AuthorFor(s, DEPT_SECURITY, "security"), "security", "a vinculum is destroyed; the Collective's coordination is severed, at a cost");
	return true;
}

// The tractor beam (the backlog's first item): hold a derelict to strip it fully for raw material,
// or lock a contact so it cannot break off. It needs the beam delivering, and its reach is short.
bool TractorWreck(Ship &s)
{
	if (s.sector.empty() || s.beacon < 0 || s.beacon >= static_cast<int>(s.sector.size())) return false;
	if (s.systems[SYS_TRACTOR_BEAM].output <= TRACTOR_MIN) return false;
	Beacon &b = s.sector[s.beacon];
	if (b.kind != BEACON_DERELICT || b.looted) return false;
	b.looted = true;
	s.stores.spareParts += SALVAGE_PARTS;
	s.stores.materials += SALVAGE_MATERIALS;
	LogEvent(s, AuthorFor(s, DEPT_ENGINEERING, "the shuttlebay"), "outside",
		"tractored a derelict and stripped it: " + std::to_string(static_cast<int>(SALVAGE_PARTS)) + " parts, "
		+ std::to_string(static_cast<int>(SALVAGE_MATERIALS)) + " material");
	return true;
}

bool TractorHold(Ship &s)
{
	if (!InCombat(s)) { s.enemyHeld = false; return false; }
	if (s.systems[SYS_TRACTOR_BEAM].output <= TRACTOR_MIN) { s.enemyHeld = false; return false; }
	s.enemyHeld = !s.enemyHeld; // a toggle: hold, or let go
	LogEvent(s, AuthorFor(s, DEPT_SECURITY, "tactical"), "outside", s.enemyHeld ? "the tractor beam holds the contact" : "the tractor beam releases the contact");
	return s.enemyHeld;
}

bool Held(const Ship &s) { return s.enemyHeld; }

// A probe (docs/exploration-and-science.md): consumable exploration and the safe way to look at
// something hostile. It charts the target without the ship going there; it may be lost, which is
// itself information. Launched from the torpedo launcher, so it needs the launchers delivering.
bool LaunchProbe(Ship &s, int beacon)
{
	if (s.stores.probes <= 0) return false;
	if (s.systems[SYS_TORPEDO_LAUNCHERS].output <= 0.0f) return false;
	if (beacon < 0 || beacon >= static_cast<int>(s.sector.size()) || beacon == s.beacon) return false;
	--s.stores.probes;
	Beacon &b = s.sector[beacon];
	const bool first = !b.visited && !b.surveyed;
	b.surveyed = true; // its telemetry says what is there, whether or not we go
	const bool lost = (s.hits + static_cast<uint32_t>(beacon)) % 3u == 0; // deterministic: sometimes it does not come back
	LogEvent(s, AuthorFor(s, DEPT_SCIENCES, "astrometrics"), "outside",
		std::string("a probe launched at beacon ") + std::to_string(beacon) + ": " + BeaconKindName(b.kind)
		+ (first ? " (newly charted)" : "") + (lost ? "; the probe is lost" : ""));
	return true;
}

// Fabrication: the replicators turn raw material into spare parts, the economy's other half.
bool FabricateParts(Ship &s, int parts)
{
	if (parts <= 0 || s.systems[SYS_REPLICATORS].output <= 0.0f) return false;
	if (s.stores.materials < parts) return false;
	s.stores.materials -= parts;
	s.stores.spareParts += parts;
	LogEvent(s, AuthorFor(s, DEPT_ENGINEERING, "the mess"), "engineering",
		"fabricated " + std::to_string(parts) + " spare parts from material");
	return true;
}

// The galley (the backlog's airponics-and-galley item): the replicators turn material into food, so a
// crew whose rations are gone can eat again -- at the cost of the material its repairs need.
bool FabricateRations(Ship &s, int days)
{
	if (days <= 0 || s.systems[SYS_REPLICATORS].output <= 0.0f) return false;
	if (s.stores.materials < days) return false;
	s.stores.materials -= days;
	s.stores.rations = std::min(100.0f, s.stores.rations + days);
	LogEvent(s, AuthorFor(s, DEPT_COMMAND, "the mess"), "crew", "the galley prepared " + std::to_string(days) + " days of rations");
	return true;
}

// Resource acquisition (the backlog's item): at a belt, the ship mines material and siphons gas for
// fuel, once. It needs a working tractor or sensors to reach it.
bool MineBelt(Ship &s)
{
	if (s.sector.empty() || s.beacon < 0 || s.beacon >= static_cast<int>(s.sector.size())) return false;
	Beacon &b = s.sector[s.beacon];
	if (b.kind != BEACON_BELT || b.looted) return false;
	if (s.systems[SYS_TRACTOR_BEAM].output <= TRACTOR_MIN && s.systems[SYS_SENSORS].output <= TRACTOR_MIN) return false;
	b.looted = true;
	s.stores.materials += BELT_MATERIALS;
	s.stores.deuterium = std::min(1.0f, s.stores.deuterium + BELT_DEUTERIUM);
	LogEvent(s, AuthorFor(s, DEPT_SCIENCES, "astrometrics"), "outside",
		"mined a belt: " + std::to_string(static_cast<int>(BELT_MATERIALS)) + " material, and siphoned gas for fuel");
	return true;
}

// Population pressure: survivors or refugees taken aboard are on no watch and hold no post, but they
// eat, they breathe, and they crowd the crew -- the pressure the design names.
bool TakeSurvivors(Ship &s, int n)
{
	if (n <= 0) return false;
	s.refugees += n;
	LogEvent(s, CommandingOfficer(s), "crew", std::to_string(n) + " survivors taken aboard; " + std::to_string(s.refugees) + " now aboard");
	return true;
}

int Refugees(const Ship &s) { return s.refugees; }

// Justice: a hearing. An acquittal releases the confined and lifts them; a conviction keeps them in
// the brig and weighs on the crew.
bool Hearing(Ship &s, int crew, bool guilty)
{
	if (crew < 0 || crew >= static_cast<int>(s.crew.size())) return false;
	CrewMember &c = s.crew[crew];
	if (!c.brigged) return false;
	if (guilty) {
		c.outlook = std::max(0.0f, c.outlook - 0.2f); // convicted: faith in the process falls
		LogEvent(s, CommandingOfficer(s), "crew", c.name + " is convicted at the hearing");
	} else {
		c.brigged = false;
		c.outlook = std::min(1.0f, c.outlook + 0.1f);
		LogEvent(s, CommandingOfficer(s), "crew", c.name + " is acquitted and released");
	}
	return true;
}

// EVA (the backlog's item): a suited party reaches what the tractor and the sensors cannot -- the
// inside of a wreck, or a belt's rock. It needs a party already out there and a spare EV suit.
bool EVA(Ship &s)
{
	if (s.sector.empty() || s.awayBeacon != s.beacon || s.stores.evSuits <= 0) return false;
	Beacon &b = s.sector[s.beacon];
	if (b.looted) return false;
	if (b.kind == BEACON_BELT) {
		b.looted = true;
		s.stores.materials += BELT_MATERIALS;
		LogEvent(s, AuthorFor(s, DEPT_SCIENCES, "the away team"), "outside", "an EVA party worked the belt by hand");
		return true;
	}
	if (b.kind == BEACON_DERELICT) {
		b.looted = true;
		s.stores.spareParts += SALVAGE_PARTS;
		s.stores.materials += SALVAGE_MATERIALS;
		LogEvent(s, AuthorFor(s, DEPT_ENGINEERING, "the away team"), "outside", "an EVA party stripped the wreck by hand");
		return true;
	}
	return false;
}

// First contact: observe a pre-warp civilisation, or interfere and own it.
bool ObservePreWarp(Ship &s)
{
	if (s.sector.empty() || s.sector[s.beacon].kind != BEACON_PREWARP) return false;
	s.sector[s.beacon].visited = true;
	s.stores.materials += 10.0f; // the survey's science yield
	for (CrewMember &c : s.crew) c.outlook = std::min(1.0f, c.outlook + 0.03f); // a discovery, made right
	LogEvent(s, CommandingOfficer(s), "outside", "observed a pre-warp civilisation from orbit; no contact made");
	return true;
}

bool InterferePreWarp(Ship &s)
{
	if (s.sector.empty() || s.sector[s.beacon].kind != BEACON_PREWARP) return false;
	s.sector[s.beacon].visited = true;
	s.stores.materials += 30.0f; // what could be taken, at a price
	s.stores.medicalSupplies = std::min(100.0f, s.stores.medicalSupplies + 20.0f);
	s.resentment = std::min(1.0f, s.resentment + 0.2f); // the crew are divided over it
	for (int i = 0; i < static_cast<int>(s.crew.size()); ++i) Remember(s, i, MEM_VIOLATION, -1, MEM_SAW, -0.6f);
	LogEvent(s, CommandingOfficer(s), "outside", "interfered with a pre-warp civilisation: a Prime Directive violation");
	return true;
}

float Resentment(const Ship &s) { return s.resentment; }

float BorgAwareness(const Ship &s) { return s.borgAwareness; }

// Whoever commands: the player's character, or the senior fit officer.
static int CommandingIndex(const Ship &s)
{
	if (s.player >= 0 && s.player < static_cast<int>(s.crew.size())) return s.player;
	int best = -1;
	for (int i = 0; i < static_cast<int>(s.crew.size()); ++i)
		if (s.crew[i].status == CREW_FIT && s.crew[i].rank >= 5 && (best < 0 || s.crew[i].rank > s.crew[best].rank)) best = i;
	return best;
}

float Loyalty(const Ship &s, int crew)
{
	const int co = CommandingIndex(s);
	if (co < 0 || crew < 0 || crew == co) return 0.0f;
	return Bond(s, crew, co);
}

// Reconciliation: command works the two factions back into one crew. When the resentment is gone, so
// is the split.
bool ReconcileFactions(Ship &s)
{
	if (!PlayerMayCommand(s)) return false;
	s.resentment = std::max(0.0f, s.resentment - 0.2f);
	if (s.resentment <= 0.0f) {
		for (CrewMember &c : s.crew) c.faction = 0;
		LogEvent(s, CommandingOfficer(s), "crew", "the Maquis and Starfleet are one crew now");
	} else {
		LogEvent(s, CommandingOfficer(s), "crew", "the crew work through their differences");
	}
	return true;
}

// The Emergency Medical Hologram: a program, so it needs the computer core, and it holds the ward
// even when every medical officer is down.
bool ActivateEMH(Ship &s, bool on)
{
	if (on && s.systems[SYS_COMPUTER_CORE].output <= 0.0f) return false;
	if (s.emhActive == on) return true;
	s.emhActive = on;
	LogEvent(s, "sickbay", "sickbay", on ? "the EMH is activated" : "the EMH is deactivated");
	return true;
}

bool EMHActive(const Ship &s) { return s.emhActive && s.systems[SYS_COMPUTER_CORE].output > 0.0f; }

// A jump needs the three travel systems: integrity for the hull, the deflector for the dust, the
// dampers for the crew. Any of them weak and the jump is not safe -- not refused, but costly.
bool TravelSafe(const Ship &s)
{
	return s.systems[SYS_STRUCTURAL_INTEGRITY].output > TRACTOR_MIN
		&& s.systems[SYS_NAV_DEFLECTOR].output > TRACTOR_MIN
		&& s.systems[SYS_INERTIAL_DAMPERS].output > TRACTOR_MIN;
}

// A fight begins: the enemy of that kind, at full health, with its systems manned.
static void StartFight(Ship &s, EnemyKind kind)
{
	if (InCombat(s)) return;
	s.enemy = Enemy();
	s.contact2 = Enemy();
	s.enemy.present = true;
	s.enemy.kind = kind;
	s.enemy.borg = kind == ENEMY_BORG_VESSEL;
	s.enemy.hull = 1.0f;
	s.enemy.shields = 1.0f;
	s.enemy.firepower = kind == ENEMY_BORG_VESSEL ? 0.5f : kind == ENEMY_WARSHIP ? 0.4f : 0.25f;
	s.enemy.boarders = kind == ENEMY_BORG_VESSEL ? 4 : 3;
	if (kind == ENEMY_BORG_VESSEL) s.borgAwareness = std::min(1.0f, s.borgAwareness + 0.15f);
}

// Hail (S9): find out what is here. A hostile answers with its weapons.
bool Hail(Ship &s)
{
	if (s.sector.empty()) return false;
	const Beacon &b = s.sector[s.beacon];
	s.sector[s.beacon].visited = true;
	if (b.kind == BEACON_HOSTILE || b.kind == BEACON_BORG) {
		StartFight(s, b.kind == BEACON_BORG ? ENEMY_BORG_VESSEL : ENEMY_RAIDER);
		LogEvent(s, CommandingOfficer(s), "outside", "hailed: no answer, its weapons are powering");
	} else {
		LogEvent(s, CommandingOfficer(s), "outside", std::string("hailed ") + BeaconKindName(b.kind));
	}
	return true;
}

// Trade (S9): at a trader, spare parts buy supplies and fuel.
bool Trade(Ship &s)
{
	if (s.sector.empty() || s.sector[s.beacon].kind != BEACON_TRADER) return false;
	if (s.stores.spareParts < TRADE_PARTS) return false;
	s.stores.spareParts -= TRADE_PARTS;
	s.stores.rations = std::min(100.0f, s.stores.rations + TRADE_RATIONS);
	s.stores.medicalSupplies = std::min(100.0f, s.stores.medicalSupplies + TRADE_RATIONS);
	s.stores.deuterium = std::min(1.0f, s.stores.deuterium + 0.05f);
	s.sector[s.beacon].visited = true;
	LogEvent(s, CommandingOfficer(s), "outside", "traded " + std::to_string(static_cast<int>(TRADE_PARTS)) + " parts for supplies and fuel");
	return true;
}

// AnswerDistress (S9): a distress call is a wreck to help, or a trap. Deterministic for the beacon.
bool AnswerDistress(Ship &s)
{
	if (s.sector.empty() || s.sector[s.beacon].kind != BEACON_DISTRESS) return false;
	s.sector[s.beacon].visited = true;
	if (s.beacon % 2 == 0) {
		s.stores.spareParts += SALVAGE_PARTS * 0.5f;
		s.stores.medicalSupplies = std::min(100.0f, s.stores.medicalSupplies + 10.0f);
		for (CrewMember &c : s.crew) c.outlook = std::min(1.0f, c.outlook + 0.05f);
		LogEvent(s, CommandingOfficer(s), "outside", "answered a distress call: survivors recovered");
	} else {
		StartFight(s, ENEMY_RAIDER);
		LogEvent(s, CommandingOfficer(s), "outside", "answered a distress call: it was a trap");
	}
	return true;
}

// Disengage (S9): break off and run -- the same test as a jump, and a damaged drive cannot.
bool Disengage(Ship &s)
{
	if (s.sector.empty()) return false;
	int best = -1;
	bool bestSafe = false;
	for (int l : s.sector[s.beacon].links) {
		const BeaconKind k = s.sector[l].kind;
		const bool safe = !(k == BEACON_HOSTILE || k == BEACON_BORG);
		if (best < 0 || (safe && !bestSafe)) { best = l; bestSafe = safe; }
	}
	if (best < 0) return false;
	return Jump(s, best);
}

// Cross into the next sector when this one's end is reached; the third crossed is home.
bool AdvanceSector(Ship &s)
{
	if (!s.reachedEnd) return false;
	s.reachedEnd = false;
	++s.sectorNumber;
	if (s.sectorNumber >= SECTORS_TO_CROSS) {
		s.won = true;
		LogEvent(s, CommandingOfficer(s), "outside", "the ship is home");
		return true;
	}
	BuildSector(s, s.sectorNumber);
	s.beacon = 0;
	s.enemy = Enemy();
	s.contact2 = Enemy();
	s.pursued = false;
	s.pursuitJumps = 0;
	LogEvent(s, CommandingOfficer(s), "outside", "crossing into sector " + std::to_string(s.sectorNumber + 1));
	return true;
}

static void UpdateOutside(Ship &s, float shipSeconds)
{
	const float minutes = shipSeconds / 60.0f;
	// Our shields recharge by what the shield system delivers, and hold nothing without it.
	const float shieldOut = s.systems[SYS_SHIELDS].output;
	s.shieldStrength = std::min(shieldOut > 0.0f ? 1.0f : 0.0f, s.shieldStrength + shieldOut * minutes / SHIELD_RECHARGE_MINUTES);
	if (!InCombat(s)) {
		if (s.contact2.present) { s.enemy = s.contact2; s.contact2 = Enemy(); } // the wingman is next
		return;
	}

	// The enemy's shields come back by what its shield generator delivers, and the wingman's too.
	s.enemy.shields = std::min(1.0f, s.enemy.shields + s.enemy.shieldGen * minutes / ENEMY_SHIELD_REGEN);
	if (s.contact2.present) s.contact2.shields = std::min(1.0f, s.contact2.shields + s.contact2.shieldGen * minutes / ENEMY_SHIELD_REGEN);

	// Our phasers: their shields first; once their shields are down, the subsystem Tactical has
	// targeted -- or the hull. A subsystem broken changes what the enemy can do. The Borg adapt: the
	// more we hit them, the less each hit does.
	float ours = s.systems[SYS_PHASERS].output * minutes / PHASER_MINUTES * PhaserYieldFactor(PhaserYieldOf(s));
	if (s.enemy.kind == ENEMY_BORG_VESSEL) {
		ours *= (1.0f - s.enemy.adaptation);
		// A live vinculum lets them adapt; destroyed (adaptationSuppressed), they cannot for a while.
		if (s.adaptationSuppressed <= 0.0f)
			s.enemy.adaptation = std::min(1.0f, s.enemy.adaptation + ours * (2.0f + s.borgAwareness));
	}
	const float onShields = std::min(ours, s.enemy.shields);
	s.enemy.shields -= onShields;
	const float through = ours - onShields;
	if (through > 0.0f) {
		switch (s.target) {
		case TARGET_WEAPONS: s.enemy.weapons = Clamp01(s.enemy.weapons - through); break;
		case TARGET_ENGINES: s.enemy.engines = Clamp01(s.enemy.engines - through); break;
		case TARGET_SHIELD_GEN: s.enemy.shieldGen = Clamp01(s.enemy.shieldGen - through); break;
		default: s.enemy.hull = std::max(0.0f, s.enemy.hull - through); break;
		}
	}
	if (s.enemy.hull <= 0.0f) { // it is over; whatever they sent across is still aboard
		if (s.contact2.present) { s.enemy = s.contact2; s.contact2 = Enemy(); } // the wingman is next
		else return;
	}

	// Their fire, as strong as their weapons still are: our shields first; what gets through lands on
	// a deck and the systems stationed there. Both contacts fire.
	auto fireOnUs = [&](Enemy &e) {
		const float full = e.firepower * e.weapons;
		if (full <= 0.0f) return;
		float theirs = full * minutes;
		const float held = std::min(theirs, s.shieldStrength);
		s.shieldStrength -= held;
		theirs -= held;
		if (theirs <= 0.0f) return;
		const float share = e.weapons * minutes; // equivalent minutes of unopposed fire
		const int deck = static_cast<int>(s.hits * 7u % DECKS);
		++s.hits;
		// Structural integrity is what holds the hull together: failing it, a hit does more.
		const float integrity = s.systems[SYS_STRUCTURAL_INTEGRITY].output;
		s.decks[deck].hull = Clamp01(s.decks[deck].hull - HIT_HULL * share * (2.0f - integrity));
		for (int i = 0; i < SYS_COUNT; ++i)
			if (SPECS[i].deck == deck + 1) s.systems[i].health = Clamp01(s.systems[i].health - HIT_SYSTEM * share);
		// An exploding console: whoever was at the station on the deck that took the hit can be hurt
		// (docs/failure-is-content.md, the injury cause beyond air, fire, wounds and radiation).
		for (CrewMember &c : s.crew) {
			if (c.status != CREW_FIT || c.brigged) continue;
			if (c.post >= SYS_COUNT || SPECS[c.post].deck != deck + 1) continue;
			c.status = CREW_INJURED;
			c.severity = std::max(c.severity, 0.4f);
			LogEvent(s, AuthorFor(s, DEPT_MEDICAL, "sickbay"), "sickbay", c.name + " was hurt by an exploding console");
			break;
		}
		// A hit can start a fire as well as hole the deck: every third one does.
		if ((s.hits % 3u) == 0u) IgniteDeck(s, deck + 1, 0.3f);
		// A hit on Main Engineering can buckle a nacelle pylon, and then there is no warp.
		if (deck + 1 == ENGINEERING_DECK) s.pylonHealth = Clamp01(s.pylonHealth - HIT_HULL * share * 0.5f);
	};
	fireOnUs(s.enemy);
	if (s.contact2.present && s.contact2.hull > 0.0f) fireOnUs(s.contact2);

	// With our shields down they send their party across, once, to where it will hurt.
	if (s.shieldStrength <= 0.0f && s.enemy.boarders > 0) {
		if (s.enemy.borg) BoardBorg(s, ENGINEERING_DECK, s.enemy.boarders);
		else Board(s, ENGINEERING_DECK, s.enemy.boarders);
		s.enemy.boarders = 0;
	}

	// With its hull going but its engines intact, a raider runs -- and follows us. The Borg do not run,
	// and neither does anything the tractor beam is holding.
	if (s.enemy.hull > 0.0f && !s.enemyHeld && s.enemy.hull < 0.15f && s.enemy.engines > 0.5f && s.enemy.kind != ENEMY_BORG_VESSEL) {
		s.pursued = true;
		s.pursuitJumps = PURSUIT_JUMPS;
		s.pursuitStrength = s.enemy.firepower * s.enemy.weapons;
		LogEvent(s, AuthorFor(s, DEPT_SECURITY, "tactical"), "outside", std::string(EnemyKindName(s.enemy.kind)) + " breaks off and runs");
		s.enemy = Enemy();
	}
}

// Fire: the crew have been assigned their firefighting posts this tick; it is put down by what they
// do, spreads to the decks beside it, and is rough on the hull while it burns.
static void UpdateFire(Ship &s, float shipSeconds)
{
	const float hours = shipSeconds / 3600.0f;
	float spread[DECKS] = {0};
	for (int d = 0; d < DECKS; ++d) {
		Deck &deck = s.decks[d];
		if (deck.fire <= 0.0f) continue;
		deck.fire = std::max(0.0f, deck.fire - deck.firefighting * (shipSeconds / 60.0f) / FIRE_FIGHT_MINUTES);
		deck.hull = Clamp01(deck.hull - deck.fire * hours / 20.0f);
		const float toSpread = deck.fire * hours * FIRE_SPREAD_PER_HOUR;
		if (d > 0) spread[d - 1] += toSpread;
		if (d + 1 < DECKS) spread[d + 1] += toSpread;
		// A fire below the threshold goes out -- but only while time is passing, so a zero-time tick
		// (a load) does not change the state it just restored.
		if (shipSeconds > 0.0f && deck.fire < 1e-3f) deck.fire = 0.0f;
	}
	for (int d = 0; d < DECKS; ++d) s.decks[d].fire = Clamp01(s.decks[d].fire + spread[d]);
}

static void UpdateDecks(Ship &s, float shipSeconds)
{
	const float support = s.systems[SYS_LIFE_SUPPORT].output;
	const float hours = shipSeconds / 3600.0f;
	for (Deck &d : s.decks) {
		// A force field over a breach holds the air; without one, an open hull vents to vacuum.
		const float vent = d.forceField ? 0.0f : (1.0f - d.hull) * shipSeconds / (VENT_MINUTES * 60.0f);
		// Life support cannot hold an atmosphere in a compartment open to space.
		const float regen = support * d.hull * hours / ATMOSPHERE_REGEN_HOURS;
		const float stale = (1.0f - support) * hours / ATMOSPHERE_STALE_HOURS;
		d.atmosphere = std::min(1.0f, std::max(0.0f, d.atmosphere + regen - stale - vent));
		// Gravity plating is life support's too (docs/ship-systems.md, the master map's deck 12):
		// with the plant down a deck loses hold, and a supplied one gets it back.
		const float gfail = (1.0f - support) * hours / GRAVITY_FAIL_HOURS;
		const float gregen = support * hours / GRAVITY_REGEN_HOURS;
		d.gravity = std::min(1.0f, std::max(0.0f, d.gravity - gfail + gregen));
	}
}

// ---- the ship ---------------------------------------------------------------------------------

int Ship::Watch() const
{
	const int hour = SecondOfDay() / 3600;
	return hour >= 8 && hour < 16 ? 0 : hour >= 16 ? 1 : 2;
}

int Ship::PowerAvailable() const
{
	int n = 0;
	for (const Source &src : sources) n += src.output;
	return n;
}

int Ship::PowerAllocated() const
{
	int n = 0;
	for (const System &sys : systems) n += sys.allocated;
	return n;
}

// The budget the engineering console shows twice (owner ruling, 2026-10-07): the plant at a fresh
// crystal and full health, and the plant at the crystal's ceiling and today's health. The gap is the
// power half of the navigation counter -- the number that tells the player the budget is shrinking.
int Ship::PowerCapacityFresh() const
{
	int n = 0;
	for (int i = 0; i < SRC_COUNT; ++i) n += SOURCES[i].capacity;
	return n;
}

int Ship::PowerCapacityNow() const
{
	int n = 0;
	for (int i = 0; i < SRC_COUNT; ++i)
		if (sources[i].online && sources[i].health > 0.0f) n += SourceCapacity(*this, static_cast<SourceId>(i));
	return n;
}

int Ship::CrewFit() const
{
	int n = 0;
	for (const CrewMember &c : crew)
		if (c.status == CREW_FIT) ++n;
	return n;
}

Ship NewShip(const Config &cfg)
{
	Ship s;
	s.cfg = cfg;
	for (int i = 0; i < SYS_COUNT; ++i) s.systems[i].priority = SPECS[i].priority;
	BuildRoster(s);
	BuildShuttles(s);
	BuildSector(s, 0);
	Tick(s, 0.0f); // so a new ship is already in a consistent state: powered, manned, located
	// The meeting clock starts settled: the first watch-change brief is the next watch, not the one
	// already in progress, and the first departmental meeting is tomorrow (docs/staff-meetings.md).
	s.lastWatchMeeting = s.Day() * WATCHES + s.Watch();
	s.lastDeptMeetingDay = s.Day();
	// The navigation counter's baseline starts at the opening figure, so the first entry's change is
	// measured from the beginning of the run rather than from zero (docs/navigation-counter.md).
	s.navCounterLast = NavigationCounter(s).currentYears;
	return s;
}

// ---- modes, rank and the player ---------------------------------------------------------------------

float ClockRate(const Config &cfg) { return cfg.clockMode == CLOCK_ACCELERATED ? cfg.dayScale : 1.0f; }

bool SavesAllowed(const Config &cfg) { return cfg.mode == MODE_HOLODECK; }

static void Advance(Ship &s, double shipSeconds);
static void UpdateCore(Ship &s, float shipSeconds);

void CatchUp(Ship &s, double realSecondsAway)
{
	if (s.cfg.clockMode != CLOCK_WALL || !(realSecondsAway > 0.0)) return;
	const double away = std::min(realSecondsAway, static_cast<double>(MAX_CATCH_UP_DAYS) * SECONDS_PER_DAY);
	Advance(s, away);
	// The log is the return surface: a run that was left standing has continuous entries through the
	// absence, where a run that was shut down has a gap (docs/the-record-and-the-log.md).
	if (s.leftStanding)
		LogEvent(s, CommandingOfficer(s), "command", "the ship was left standing; " + std::to_string(static_cast<int>(away / 3600.0 + 0.5)) + " hours passed aboard");
}

// The two exits. Holodeck may suspend the world; ironman may not, because ironman means the ship
// keeps her own time. This is the same guard as SavesAllowed, in the same place (S10).
bool MaySuspend(const Config &cfg) { return cfg.mode == MODE_HOLODECK; }

bool Suspend(Ship &s)
{
	if (!MaySuspend(s.cfg)) return false;
	if (!s.leftStanding)
	{
		s.leftStanding = true;
		LogEvent(s, CommandingOfficer(s), "command", "the ship is left standing; she keeps her own time");
	}
	return true;
}

bool LeftStanding(const Ship &s) { return s.leftStanding; }

// The sleep state: skip time at the accelerated rate, in one jump or in steps. Advance cuts long
// steps up, so both arrive where a played run would (the convergence the ruling requires).
void Sleep(Ship &s, double shipSeconds)
{
	if (!(shipSeconds > 0.0)) return;
	Advance(s, shipSeconds);
}

// Which stations a department's own people work. The mapping MayOperate and the delegation rules
// both read, kept in one place so they cannot drift apart.
static bool DepartmentOperates(Department dept, Station st)
{
	switch (st) {
	case STN_ENGINEERING: return dept == DEPT_ENGINEERING;
	case STN_TACTICAL: return dept == DEPT_SECURITY;
	case STN_OPS: case STN_CONN: return dept == DEPT_COMMAND || dept == DEPT_SCIENCES;
	case STN_SICKBAY: return dept == DEPT_MEDICAL;
	default: return false;
	}
}

bool MayOperate(const CrewMember &who, Station st)
{
	if (who.status != CREW_FIT || who.brigged) return false;
	if (who.rank >= 4) return true;
	if (st < STN_COUNT && (who.credentials & (1u << st))) return true; // a trained cross-qualification
	if (st >= STN_COUNT) return false;
	return DepartmentOperates(who.dept, st);
}

bool MayCallAlert(const CrewMember &who, Station st)
{
	return who.rank >= 3 && (st == STN_ENGINEERING || st == STN_TACTICAL) && MayOperate(who, st);
}

// The Ship-level questions add the delegation, the lock-out and the override: none is visible in a
// record alone.
bool MayOperate(const Ship &s, int crew, Station st)
{
	if (crew < 0 || crew >= static_cast<int>(s.crew.size())) return false;
	if (LockedOut(s, crew, st)) return false; // the console has stopped answering this person
	if (MayOperate(s.crew[crew], st)) return true;
	return DelegatedTo(s, crew, st);
}

bool MayCallAlert(const Ship &s, int crew, Station st)
{
	if (crew < 0 || crew >= static_cast<int>(s.crew.size())) return false;
	const CrewMember &who = s.crew[crew];
	if (who.rank < 3) return false;
	return MayOperate(s, crew, st);
}

bool MayCommand(const CrewMember &who) { return who.status == CREW_FIT && who.rank >= 5; }

// ---- delegation, revocation and override (docs/access-and-authority.md) ---------------------------

// A head may grant a station's access: a lieutenant commander is ship-wide, a lieutenant heads the
// department that works the station, and a cross-qualified lieutenant may grant the one they trained for.
static bool MayGrant(const CrewMember &who, Station st)
{
	if (who.status != CREW_FIT || who.brigged || who.rank < 3) return false;
	if (who.rank >= 4) return true;
	return DepartmentOperates(who.dept, st) || (st < STN_COUNT && (who.credentials & (1u << st)) != 0);
}

bool Delegate(Ship &s, int grantor, int grantee, Station st)
{
	if (grantor < 0 || grantor >= static_cast<int>(s.crew.size())) return false;
	if (grantee < 0 || grantee >= static_cast<int>(s.crew.size())) return false;
	if (grantor == grantee || st >= STN_COUNT) return false;
	CrewMember &g = s.crew[grantor];
	CrewMember &t = s.crew[grantee];
	if (!MayGrant(g, st)) return false;
	if (t.status != CREW_FIT || t.brigged) return false;
	if (t.rank >= 4) return false; // already ship-wide: there is nothing a shift could add
	// One live grant per station and holder: a repeat refreshes the shift rather than stacking.
	for (Delegation &d : s.delegations)
		if (d.station == st && d.grantee == grantee) {
			d.grantor = grantor;
			d.expires = s.clock + DELEGATION_SHIFT_HOURS * 3600.0;
			LogEvent(s, g.name, "crew", g.name + " extends " + t.name + "'s " + StationName(st) + " access for another shift");
			return true;
		}
	if (static_cast<int>(s.delegations.size()) >= DELEGATION_MAX) s.delegations.erase(s.delegations.begin());
	Delegation d;
	d.grantor = grantor; d.grantee = grantee; d.station = static_cast<uint8_t>(st);
	d.expires = s.clock + DELEGATION_SHIFT_HOURS * 3600.0;
	s.delegations.push_back(d);
	LogEvent(s, g.name, "crew", g.name + " grants " + t.name + " " + StationName(st) + " access for the shift");
	return true;
}

bool RevokeDelegation(Ship &s, int revoker, int grantee, Station st)
{
	if (revoker < 0 || revoker >= static_cast<int>(s.crew.size())) return false;
	if (grantee < 0 || grantee >= static_cast<int>(s.crew.size()) || st >= STN_COUNT) return false;
	const CrewMember &r = s.crew[revoker];
	if (!MayGrant(r, st)) return false;
	for (size_t i = 0; i < s.delegations.size(); ++i)
		if (s.delegations[i].station == st && s.delegations[i].grantee == grantee) {
			LogEvent(s, r.name, "crew", r.name + " revokes " + s.crew[grantee].name + "'s " + StationName(st) + " access");
			s.delegations.erase(s.delegations.begin() + i);
			return true;
		}
	return false;
}

bool DelegatedTo(const Ship &s, int crew, Station st)
{
	for (const Delegation &d : s.delegations)
		if (d.grantee == crew && d.station == st && d.expires > s.clock) return true;
	return false;
}

const std::vector<Delegation> &Delegations(const Ship &s) { return s.delegations; }

bool RevokeCredential(Ship &s, int revoker, int holder, Station st)
{
	if (revoker < 0 || revoker >= static_cast<int>(s.crew.size())) return false;
	if (holder < 0 || holder >= static_cast<int>(s.crew.size()) || st >= STN_COUNT) return false;
	const CrewMember &r = s.crew[revoker];
	if (!MayGrant(r, st)) return false;
	CrewMember &c = s.crew[holder];
	if (!(c.credentials & (1u << st))) return false;
	c.credentials = static_cast<uint8_t>(c.credentials & ~(1u << st));
	LogEvent(s, r.name, "crew", r.name + " revokes " + c.name + "'s " + StationName(st) + " qualification");
	return true;
}

bool BeginOverride(Ship &s, int requester, Station st)
{
	if (requester < 0 || requester >= static_cast<int>(s.crew.size()) || st >= STN_COUNT) return false;
	const CrewMember &r = s.crew[requester];
	if (r.status != CREW_FIT || r.brigged) return false;
	Override &o = s.emergencyOverride;
	if (o.station >= 0 && o.active)
	{
		if (o.station == st) return true; // already forced
		return false;                     // one at a time
	}
	o.station = static_cast<uint8_t>(st);
	o.first = requester;
	o.second = -1;
	o.solo = false;
	o.active = false;
	o.readyAt = s.clock + OVERRIDE_SOLO_MINUTES * 60.0; // one hand may force it, slower
	o.expires = 0.0;
	LogEvent(s, r.name, "command", "EMERGENCY OVERRIDE: " + r.name + " moves to force " + StationName(st)
		+ "; a second officer must agree, or one hand may force it in " + std::to_string(static_cast<int>(OVERRIDE_SOLO_MINUTES)) + " minutes");
	return true;
}

bool ConfirmOverride(Ship &s, int second, Station st)
{
	if (second < 0 || second >= static_cast<int>(s.crew.size())) return false;
	Override &o = s.emergencyOverride;
	if (o.station != st || o.active) return false;
	if (second == o.first) return false; // one person cannot be their own second
	const CrewMember &c = s.crew[second];
	if (c.status != CREW_FIT || c.brigged) return false;
	o.second = second;
	o.solo = false;
	o.readyAt = s.clock + OVERRIDE_TWO_MINUTES * 60.0; // two hands: faster
	LogEvent(s, c.name, "command", "EMERGENCY OVERRIDE: " + c.name + " agrees; " + StationName(st)
		+ " will answer to them in " + std::to_string(static_cast<int>(OVERRIDE_TWO_MINUTES)) + " minutes");
	return true;
}

bool OverrideActive(const Ship &s, Station st)
{
	const Override &o = s.emergencyOverride;
	return o.active && o.station == st && s.clock < o.expires;
}

const Override &OverrideState(const Ship &s) { return s.emergencyOverride; }

// ---- the named refusal (docs/access-and-authority.md) ---------------------------------------------

// Who a station's own chain of command says can open it. The name the locked control shows.
static const char *StationHandler(Station st)
{
	switch (st) {
	case STN_ENGINEERING: return "the Chief Engineer";
	case STN_TACTICAL: return "the Chief of Security";
	case STN_OPS: return "the Operations officer";
	case STN_CONN: return "the Conn officer";
	case STN_SICKBAY: return "the Chief Medical Officer";
	default: return "a department head";
	}
}

std::string AccessRefusal(const Ship &s, Station st)
{
	(void)s;
	return std::string("you are not cleared for ") + StationName(st) + "; " + StationHandler(st)
		+ " or a lieutenant commander may open it, or a delegation for the shift";
}

std::string OperatedFromRefusal(SystemId id)
{
	return std::string(Spec(id).name) + " is operated from " + StationName(StationOf(id));
}

// ---- remote call-up and the lock-out (owner decision, 2026-10-07) ---------------------------------

bool MayCallUp(const Ship &s, int crew, Station console, SystemId id)
{
	if (id >= SYS_COUNT) return false;
	if (OperatedFrom(id, console)) return true; // the console's own system: no call-up needed
	if (console >= STN_COUNT) return false;
	if (crew == s.player && s.cfg.role == ROLE_IN_COMMAND) return true; // the ship answers to the player
	if (crew < 0 || crew >= static_cast<int>(s.crew.size())) return false;
	const CrewMember &who = s.crew[crew];
	if (who.status != CREW_FIT || who.brigged) return false;
	return who.rank >= 4; // ship-wide: the captain or first officer calls it up from anywhere
}

bool LockOut(Ship &s, int officer, Station st, int locked)
{
	if (officer < 0 || officer >= static_cast<int>(s.crew.size())) return false;
	if (locked < 0 || locked >= static_cast<int>(s.crew.size()) || st >= STN_COUNT) return false;
	if (officer == locked) return false;
	const CrewMember &o = s.crew[officer];
	const CrewMember &t = s.crew[locked];
	if (o.status != CREW_FIT || o.brigged || t.status != CREW_FIT || t.brigged) return false;
	if (o.rank <= t.rank) return false;    // above the post-holder, and strictly so
	if (!MayGrant(o, st)) return false;    // the department head, or a ship-wide officer
	for (Lockout &l : s.lockouts)
		if (l.station == st && l.locked == locked) {
			l.lockedBy = officer; l.time = s.clock;
			LogEvent(s, o.name, "crew", o.name + " locks " + t.name + " out of " + StationName(st));
			Remember(s, locked, MEM_LOCKOUT, officer, MEM_SAW, -0.6f);
			return true;
		}
	if (static_cast<int>(s.lockouts.size()) >= LOCKOUT_MAX) s.lockouts.erase(s.lockouts.begin());
	Lockout l;
	l.station = static_cast<uint8_t>(st); l.lockedBy = officer; l.locked = locked; l.time = s.clock;
	s.lockouts.push_back(l);
	LogEvent(s, o.name, "crew", o.name + " locks " + t.name + " out of " + StationName(st));
	Remember(s, locked, MEM_LOCKOUT, officer, MEM_SAW, -0.6f);
	return true;
}

bool ClearLockout(Ship &s, int officer, Station st, int locked)
{
	if (officer < 0 || officer >= static_cast<int>(s.crew.size())) return false;
	const CrewMember &o = s.crew[officer];
	if (o.status != CREW_FIT || o.brigged || !MayGrant(o, st)) return false;
	for (size_t i = 0; i < s.lockouts.size(); ++i)
		if (s.lockouts[i].station == st && s.lockouts[i].locked == locked) {
			LogEvent(s, o.name, "crew", o.name + " clears the lock-out on " + s.crew[locked].name + " at " + StationName(st));
			s.lockouts.erase(s.lockouts.begin() + i);
			return true;
		}
	return false;
}

bool LockedOut(const Ship &s, int crew, Station st)
{
	for (const Lockout &l : s.lockouts)
		if (l.locked == crew && l.station == st) return true;
	return false;
}

const std::vector<Lockout> &Lockouts(const Ship &s) { return s.lockouts; }

std::string LockoutNotice(const Ship &s, int crew, Station st)
{
	for (const Lockout &l : s.lockouts)
		if (l.locked == crew && l.station == st && l.lockedBy >= 0 && l.lockedBy < static_cast<int>(s.crew.size())
			&& crew >= 0 && crew < static_cast<int>(s.crew.size()))
			return s.crew[l.lockedBy].name + " has locked " + s.crew[crew].name + " out of " + StationName(st);
	return std::string();
}

// A player who is down operates nothing and commands nothing, whatever their role: being hurt is a
// state the ship acts on (Stage B). The check is on the record, so it holds across a save.
static bool PlayerDown(const Ship &s)
{
	return s.player >= 0 && s.player < static_cast<int>(s.crew.size()) && s.crew[s.player].status != CREW_FIT;
}

bool PlayerMayOperate(const Ship &s, Station st)
{
	if (PlayerDown(s)) return false;
	if (s.cfg.role == ROLE_IN_COMMAND) return true;
	if (OverrideActive(s, st)) return true; // forced at this station for a while
	return MayOperate(s, s.player, st);
}

bool PlayerMayCommand(const Ship &s)
{
	if (PlayerDown(s)) return false;
	if (s.cfg.role == ROLE_IN_COMMAND) return true;
	return s.player >= 0 && s.player < static_cast<int>(s.crew.size()) && MayCommand(s.crew[s.player]);
}

int CreateCharacter(Ship &s, const std::string &name, Department dept, int rank)
{
	if (name.empty() || name.size() > 40 || rank < 0 || rank > 4 || dept >= DEPT_COUNT) return -1;
	const size_t named = sizeof(NAMED) / sizeof(NAMED[0]);
	// The last generated member of the department with no station: nobody's post is taken from them.
	for (size_t i = s.crew.size(); i-- > named;) {
		CrewMember &c = s.crew[i];
		if (c.dept != dept || c.post != SYS_COUNT || c.status != CREW_FIT) continue;
		c.name = name;
		c.rank = static_cast<uint8_t>(rank);
		s.player = static_cast<int>(i);
		return s.player;
	}
	return -1;
}

bool OrderBuild(Ship &s, int parts)
{
	if (!PlayerMayCommand(s) || parts <= 0) return false;
	for (Job &j : s.jobs)
		if (j.kind == JOB_BUILD) {
			j.target = static_cast<int16_t>(std::min(999, j.target + parts));
			LogEvent(s, CommandingOfficer(s), "engineering", "the crew are to build spare parts");
			return true;
		}
	Job j;
	j.kind = JOB_BUILD;
	j.target = static_cast<int16_t>(std::min(999, parts));
	j.priority = 40; // after repairs, seals and reclamation
	s.jobs.push_back(j);
	LogEvent(s, CommandingOfficer(s), "engineering", "the crew are to build " + std::to_string(parts) + " spare parts");
	return true;
}

// The job-queue board's one command-side act (docs/crew-work.md): command sets the order, so the
// same queue reads differently by post. Priority is carried on the job and preserved across ticks by
// UpdateJobs' lookup, so a thing left undone is still worked in the order command set tomorrow.
bool SetJobPriority(Ship &s, int index, int priority)
{
	if (!PlayerMayCommand(s)) return false;
	if (index < 0 || index >= static_cast<int>(s.jobs.size())) return false;
	Job &j = s.jobs[index];
	if (j.priority == priority) return true;
	j.priority = static_cast<int16_t>(priority);
	LogEvent(s, CommandingOfficer(s), "command", std::string("damage control is to see to the ") + JobKindName(j.kind) + " job in this order");
	return true;
}

// The security squad: a fireteam command sends to retake a deck (docs/borg-incursion.md). It musters
// at the security deck (9) and advances a deck at a time; in position it holds the deck while the
// crew restore it, then stands down. The crew layer embodies them.
bool OrderAdvance(Ship &s, int deck)
{
	if (!PlayerMayCommand(s) || deck < 1 || deck > DECKS) return false;
	bool security = false;
	for (const CrewMember &c : s.crew) if (c.status == CREW_FIT && !c.brigged && c.dept == DEPT_SECURITY) { security = true; break; }
	if (!security) return false;
	s.advanceDeck = deck;
	s.advanceAt = 9; // the security deck is the muster
	s.advanceMs = 0.0f;
	LogEvent(s, CommandingOfficer(s), "security", "a squad is to retake deck " + std::to_string(deck));
	return true;
}

static void UpdateSquad(Ship &s, float shipSeconds)
{
	if (s.advanceDeck < 1 || s.advanceDeck > DECKS) { s.advanceDeck = 0; return; }
	const float minutes = shipSeconds / 60.0f;
	if (s.advanceAt != s.advanceDeck) {
		s.advanceMs += minutes;
		while (s.advanceMs >= SQUAD_TRAVEL_MINUTES && s.advanceAt != s.advanceDeck) {
			s.advanceMs -= SQUAD_TRAVEL_MINUTES;
			s.advanceAt += (s.advanceDeck > s.advanceAt ? 1 : -1);
		}
	}
	if (s.advanceAt == s.advanceDeck) {
		Deck &d = s.decks[s.advanceDeck - 1];
		d.defenders += SQUAD_MAX; // in position: the squad fights here
		if (d.intruders <= 0.0f) {
			d.compromised = false; // retaken: the deck's systems are restored, not left suspect
			LogEvent(s, AuthorFor(s, DEPT_SECURITY, "security"), "security", "the squad has retaken deck " + std::to_string(s.advanceDeck) + " and stands down");
			s.advanceDeck = 0; s.advanceAt = 0; s.advanceMs = 0.0f;
		}
	}
}

bool OrderRepairFirst(Ship &s, int system)
{
	if (!PlayerMayCommand(s)) return false;
	s.orderRepairFirst = system >= 0 && system < SYS_COUNT ? system : -1;
	LogEvent(s, CommandingOfficer(s), "command", s.orderRepairFirst >= 0 ? std::string("damage control is to see first to ") + SPECS[s.orderRepairFirst].name : std::string("no priority repair order"));
	return true;
}

bool OrderSecurityTo(Ship &s, int deck)
{
	if (!PlayerMayCommand(s)) return false;
	s.orderSecurityTo = deck >= 1 && deck <= DECKS ? deck : 0;
	LogEvent(s, CommandingOfficer(s), "command", s.orderSecurityTo ? "security to deck " + std::to_string(s.orderSecurityTo) : std::string("security order cleared"));
	return true;
}

bool OrderEvacuate(Ship &s, int deck)
{
	if (!PlayerMayCommand(s)) return false;
	s.orderEvacuate = deck >= 1 && deck <= DECKS ? deck : 0;
	LogEvent(s, CommandingOfficer(s), "command", s.orderEvacuate ? "evacuate deck " + std::to_string(s.orderEvacuate) : std::string("evacuation order cleared"));
	return true;
}

bool OrderTriage(Ship &s, int policy)
{
	if (!PlayerMayCommand(s)) return false;
	s.orderTriage = policy == 1 ? 1 : 0;
	LogEvent(s, CommandingOfficer(s), "sickbay", s.orderTriage == 1 ? std::string("triage: rank first") : std::string("triage: worst first"));
	return true;
}

void SetRole(Ship &s, PlayerRole role)
{
	s.cfg.role = role;
	if (role != ROLE_MUNRO) return;
	for (size_t i = 0; i < s.crew.size(); ++i)
		if (s.crew[i].type == "munro") s.player = static_cast<int>(i);
}

bool Qualified(const CrewMember &who, Station st)
{
	return st < STN_COUNT && (who.credentials & (1u << st)) != 0;
}

bool Train(Ship &s, int crew, Station st)
{
	if (crew < 0 || crew >= static_cast<int>(s.crew.size()) || st >= STN_COUNT) return false;
	CrewMember &c = s.crew[crew];
	if (c.status != CREW_FIT || c.brigged) return false;
	if (Qualified(c, st)) return false; // already trained
	c.credentials = static_cast<uint8_t>(c.credentials | (1u << st));
	LogEvent(s, c.name, "crew", std::string("trained and qualified at ") + StationName(st));
	return true;
}

bool Brigged(const Ship &s, int crew)
{
	return crew >= 0 && crew < static_cast<int>(s.crew.size()) && s.crew[crew].brigged;
}

bool Brig(Ship &s, int crew, bool on)
{
	if (crew < 0 || crew >= static_cast<int>(s.crew.size())) return false;
	CrewMember &c = s.crew[crew];
	if (c.status != CREW_FIT) return false; // the dead and the injured are not the brig's
	c.brigged = on;
	LogEvent(s, CommandingOfficer(s), "crew", c.name + (on ? " is confined to the brig" : " is released from the brig"));
	return true;
}

bool HoldFuneral(Ship &s)
{
	if (!PlayerMayCommand(s)) return false;
	// Whoever commands holds it and leads it: the crew come to stand with them, and that is the
	// bond the funeral strengthens. CommandingOfficer names the body; find that record's index.
	int officiant = s.player;
	if (officiant < 0 || officiant >= static_cast<int>(s.crew.size())) {
		officiant = -1;
		for (int i = 0; i < static_cast<int>(s.crew.size()); ++i)
			if (s.crew[i].status == CREW_FIT && s.crew[i].rank >= 5) { officiant = i; break; }
	}
	int lifted = 0, attended = 0;
	for (int i = 0; i < static_cast<int>(s.crew.size()); ++i) {
		CrewMember &c = s.crew[i];
		c.quartersSealed = false; // the funeral is held: the sealed quarters are opened again
		if (c.status != CREW_FIT) continue;
		++attended;
		// Metabolism, not accumulation (docs/morale.md): the death marks soften toward shared memory.
		for (Memory &m : c.memories)
			if (m.event == MEM_DEATH) { m.valence = std::max(m.valence, -0.2f); m.salience = 1.0f; }
		// And every person who stood together takes a positive mark toward the one who led them
		// through it: a bond that was not there before, strengthened by standing in the same room.
		Remember(s, i, MEM_FUNERAL, officiant == i ? -1 : officiant, MEM_SAW, 0.4f);
		if (Morale(c) < 0.7f) {
			// The funeral is the mechanic that turns a loss into shared resolve: it lifts the
			// outlook and strengthens what the crew hold with (docs/morale.md).
			c.outlook = std::min(1.0f, c.outlook + 0.15f);
			c.holdings = std::min(1.0f, c.holdings + 0.10f);
			++lifted;
		}
	}
	LogEvent(s, CommandingOfficer(s), "crew", "a funeral is held for the lost; " + std::to_string(attended) + " stand together, " + std::to_string(lifted) + " take heart");
	return true;
}

// A death by name and cause. The tick reaches the same end through exposure, fire and wounds; this is
// the door a scenario (or a console) uses to close a record deliberately.
bool KillCrew(Ship &s, int crew, const std::string &cause)
{
	if (crew < 0 || crew >= static_cast<int>(s.crew.size())) return false;
	CrewMember &c = s.crew[crew];
	if (c.status == CREW_DEAD) return false;
	c.status = CREW_DEAD;
	c.recovery = 0.0f;
	LogEvent(s, AuthorFor(s, DEPT_MEDICAL, "sickbay"), "crew", c.name + " is dead: " + (cause.empty() ? std::string("unknown cause") : cause));
	NoteDeath(s, crew);
	return true;
}

// The wall of names (docs/morale.md, docs/gap-analysis.md): the crew the ship has buried, read from
// the records in roster order. Nothing extra is stored -- the dead are the dead.
std::vector<std::string> WallOfNames(const Ship &s)
{
	std::vector<std::string> names;
	for (const CrewMember &c : s.crew)
		if (c.status == CREW_DEAD) names.push_back(c.name);
	return names;
}

// The quarters still shut: the dead whose door the crew have not yet opened at a funeral. The deck
// is carried so a console can say which walk past means something.
std::vector<SealedQuarter> SealedQuarters(const Ship &s)
{
	std::vector<SealedQuarter> sealed;
	for (int i = 0; i < static_cast<int>(s.crew.size()); ++i)
		if (s.crew[i].status == CREW_DEAD && s.crew[i].quartersSealed) {
			SealedQuarter q;
			q.crew = i;
			q.deck = s.crew[i].quartersDeck;
			sealed.push_back(q);
		}
	return sealed;
}

bool Promote(Ship &s, int crew)
{
	if (!PlayerMayCommand(s)) return false;
	if (crew < 0 || crew >= static_cast<int>(s.crew.size())) return false;
	CrewMember &c = s.crew[crew];
	if (c.status != CREW_FIT || c.rank >= 6) return false;
	static const char *const RANKS[] = {"Crewman", "Ensign", "Lt. j.g.", "Lieutenant", "Lt. Commander", "Commander", "Captain"};
	const std::string from = RANKS[c.rank];
	++c.rank;
	LogEvent(s, CommandingOfficer(s), "crew", c.name + " is promoted from " + from + " to " + RANKS[c.rank]);
	// A promise of promotion made in front of this crew member is now a promise kept.
	for (int i = 0; i < static_cast<int>(s.promises.size()); ++i) {
		const Promise &p = s.promises[i];
		if (p.state == PROMISE_OPEN && p.kind == PROMISE_PROMOTION && p.beneficiary == crew) {
			ResolvePromise(s, i, true);
			break;
		}
	}
	return true;
}

bool PlayerIncapacitated(const Ship &s)
{
	if (s.player < 0 || s.player >= static_cast<int>(s.crew.size())) return false;
	const uint8_t st = s.crew[s.player].status;
	return st == CREW_INJURED || st == CREW_DEAD || st == CREW_ASSIMILATED;
}

bool PlayerDead(const Ship &s)
{
	if (s.player < 0 || s.player >= static_cast<int>(s.crew.size())) return false;
	return s.crew[s.player].status == CREW_DEAD;
}

// ---- the player in the world (Stage B) -----------------------------------------------------------

void SetPlayerDeck(Ship &s, int deck)
{
	s.playerDeck = (deck >= 1 && deck <= DECKS) ? deck : 0;
}

int PlayerDeck(const Ship &s) { return s.playerDeck; }

// The health the player's body shows for the record's state. Fit is whole; an untreated injury is a
// body losing ground (severity 0.5 is the body at half); a closed record is no health at all. The
// numbers are our call -- the mapping only has to be monotone, so the world and the record agree.
int PlayerBodyHealth(const Ship &s, int bodyHealth)
{
	if (bodyHealth <= 0) return bodyHealth;
	if (s.player < 0 || s.player >= static_cast<int>(s.crew.size())) return bodyHealth;
	const CrewMember &c = s.crew[s.player];
	if (c.status == CREW_DEAD || c.status == CREW_ASSIMILATED) return 0;
	if (c.status == CREW_FIT) return bodyHealth;
	const int h = static_cast<int>(bodyHealth * (1.0f - Clamp01(c.severity)) + 0.5f);
	return std::max(1, std::min(bodyHealth - 1, h));
}

// The world hurt the player's body. The damage is written into the same record any other casualty
// carries, so the ward treats them, a bad enough wound closes the record, and being down is the same
// state a crew member is in. `amount` is a fraction of the body (0..1).
bool WoundPlayer(Ship &s, float amount, const std::string &cause)
{
	if (!(amount > 0.0f)) return false;
	if (s.player < 0 || s.player >= static_cast<int>(s.crew.size())) return false;
	CrewMember &c = s.crew[s.player];
	if (c.status == CREW_DEAD || c.status == CREW_ASSIMILATED) return false;
	const bool wasFit = c.status == CREW_FIT;
	// The world sends the incremental loss (the body has already lost it), so a fresh injury starts
	// at the amount and a wound already being carried adds to itself.
	c.wounds = Clamp01(c.wounds + amount);
	c.severity = wasFit ? Clamp01(amount) : Clamp01(c.severity + amount);
	c.status = CREW_INJURED;
	if (c.severity >= 1.0f) {
		KillCrew(s, s.player, cause.empty() ? std::string("wounds") : cause);
		return true;
	}
	LogEvent(s, AuthorFor(s, DEPT_MEDICAL, "sickbay"), "sickbay",
		c.name + " is hurt" + (cause.empty() ? std::string() : " (" + cause + ")"));
	return true;
}

// Somebody comes when the player goes down. The senior medical hand who can walk is sent; the log
// names them; the record is carried to the ward by the same treatment path as any other casualty.
int AttendIncapacitatedPlayer(Ship &s)
{
	if (s.player < 0 || s.player >= static_cast<int>(s.crew.size())) return -1;
	CrewMember &p = s.crew[s.player];
	if (p.status == CREW_FIT || p.status == CREW_DEAD || p.status == CREW_ASSIMILATED) { s.playerAttended = -1; return -1; }
	if (s.playerAttended >= 0 && s.playerAttended < static_cast<int>(s.crew.size())) return s.playerAttended; // already with them
	int who = -1;
	for (int i = 0; i < static_cast<int>(s.crew.size()); ++i) {
		const CrewMember &c = s.crew[i];
		if (i == s.player || c.status != CREW_FIT || c.brigged || c.away) continue;
		if (c.dept != DEPT_MEDICAL) continue;
		if (who < 0 || c.rank > s.crew[who].rank) who = i;
	}
	if (who < 0) return -1; // no one to send: the injury still runs its course in the ward
	s.playerAttended = who;
	const int deck = p.deck;
	LogEvent(s, s.crew[who].name, "sickbay",
		s.crew[who].name + " attends " + p.name + (deck >= 1 && deck <= DECKS ? " on deck " + std::to_string(deck) : "")
		+ "; they are carried to sickbay");
	return who;
}

// The senior fit officer, in rank order: who takes the chair when the player is lost.
static int SeniorFitOfficer(const Ship &s)
{
	int best = -1;
	for (int i = 0; i < static_cast<int>(s.crew.size()); ++i) {
		const CrewMember &c = s.crew[i];
		if (c.status != CREW_FIT || c.brigged || c.away) continue;
		if (best < 0 || c.rank > s.crew[best].rank) best = i;
	}
	return best;
}

int AssumeCommand(Ship &s)
{
	if (s.player < 0 || s.player >= static_cast<int>(s.crew.size())) return -1;
	const CrewMember &p = s.crew[s.player];
	if (p.status != CREW_DEAD && p.status != CREW_ASSIMILATED) return s.player; // still there: no devolution
	const int next = SeniorFitOfficer(s);
	if (next < 0 || next == s.player) return -1;
	const std::string fallen = p.name;
	s.player = next;
	LogEvent(s, CommandingOfficer(s), "command",
		"command passes to " + s.crew[next].name + ", relieving " + fallen);
	return next;
}

// ---- memory and consequence -------------------------------------------------------------------

void Remember(Ship &s, int crew, uint16_t event, int person, MemorySource source, float valence)
{
	if (crew < 0 || crew >= static_cast<int>(s.crew.size())) return;
	CrewMember &c = s.crew[crew];
	if (c.status == CREW_DEAD || c.status == CREW_ASSIMILATED) return;
	Memory m;
	m.event = event;
	m.person = static_cast<int16_t>(person);
	m.source = source;
	m.time = static_cast<float>(s.clock);
	m.valence = std::max(-1.0f, std::min(1.0f, valence));
	m.salience = 1.0f;
	// Reinforce: hearing the same thing again sharpens the mark, it does not duplicate it.
	for (Memory &old : c.memories)
		if (old.event == event && old.person == m.person) { old.salience = 1.0f; old.source = source; return; }
	// Bounded: over the cap, the least salient (and oldest, on a tie) falls off.
	if (static_cast<int>(c.memories.size()) >= MEMORY_MAX) {
		size_t worst = 0;
		for (size_t i = 1; i < c.memories.size(); ++i)
			if (c.memories[i].salience < c.memories[worst].salience
				|| (c.memories[i].salience == c.memories[worst].salience && c.memories[i].time < c.memories[worst].time)) worst = i;
		c.memories.erase(c.memories.begin() + worst);
	}
	c.memories.push_back(m);
}

void Brief(Ship &s, uint16_t event, float valence)
{
	for (int i = 0; i < static_cast<int>(s.crew.size()); ++i)
		Remember(s, i, event, -1, MEM_TOLD, valence);
	LogEvent(s, CommandingOfficer(s), "crew", "the crew are told what happened");
}

bool Recall(const CrewMember &who, uint16_t event)
{
	for (const Memory &m : who.memories) if (m.event == event) return true;
	return false;
}

int RecallSource(const CrewMember &who, uint16_t event)
{
	int best = -1;
	for (const Memory &m : who.memories)
		if (m.event == event && (best < 0 || m.salience > 0.0f)) best = m.source;
	return best;
}

int MemoryCount(const CrewMember &who) { return static_cast<int>(who.memories.size()); }

// A bond is remembered valence toward a person, summed: repeated good becomes friendship, repeated
// bad a grudge, and one extreme event can do either in a stroke.
float Bond(const Ship &s, int a, int b)
{
	if (a < 0 || a >= static_cast<int>(s.crew.size())) return 0.0f;
	float bond = 0.0f;
	for (const Memory &m : s.crew[a].memories)
		if (m.person == b) bond += m.valence * m.salience;
	return std::max(-1.0f, std::min(1.0f, bond));
}

float Trauma(const CrewMember &who)
{
	float t = 0.0f;
	for (const Memory &m : who.memories)
		if (m.valence < 0.0f) t += (-m.valence) * m.salience;
	return std::min(1.0f, t);
}

// ---- promises: a bond with a claim attached (docs/memory-and-consequence.md) -------------------

const char *PromiseKindName(uint8_t k)
{
	static const char *const N[PROMISE_KIND_COUNT] = { "a repair", "a rescue", "a promotion", "a way home" };
	return k < PROMISE_KIND_COUNT ? N[k] : "?";
}

int MakePromise(Ship &s, int officer, int crew, PromiseKind kind, const std::string &what, double deadline)
{
	if (officer < 0 || officer >= static_cast<int>(s.crew.size())) return -1;
	if (crew < 0 || crew >= static_cast<int>(s.crew.size())) return -1;
	if (kind >= PROMISE_KIND_COUNT) return -1;
	if (s.crew[officer].status != CREW_FIT || s.crew[crew].status != CREW_FIT) return -1;
	Promise p;
	p.promiser = officer;
	p.beneficiary = crew;
	p.kind = kind;
	p.what = what;
	p.made = s.clock;
	p.deadline = deadline;
	// The mark is written where the promise was heard: it names the promiser, and it is positive
	// and salient enough to survive until the thing is done or found undone.
	Remember(s, crew, MEM_PROMISE, officer, MEM_SAW, 0.6f);
	s.promises.push_back(p);
	if (static_cast<int>(s.promises.size()) > PROMISE_MAX) s.promises.erase(s.promises.begin());
	LogEvent(s, s.crew[officer].name, "crew", s.crew[officer].name + " promises " + s.crew[crew].name + ": " + what);
	return static_cast<int>(s.promises.size()) - 1;
}

// The outcome moves the mark and the bond. Kept, the mark strengthens; broken, it turns negative.
// Either way the log carries it in the crew member's own terms, not the ship's.
bool ResolvePromise(Ship &s, int index, bool kept)
{
	if (index < 0 || index >= static_cast<int>(s.promises.size())) return false;
	Promise &p = s.promises[index];
	if (p.state != PROMISE_OPEN) return false;
	if (p.beneficiary < 0 || p.beneficiary >= static_cast<int>(s.crew.size())) return false;
	p.state = kept ? PROMISE_KEPT : PROMISE_BROKEN;
	CrewMember &c = s.crew[p.beneficiary];
	const std::string promiser = (p.promiser >= 0 && p.promiser < static_cast<int>(s.crew.size())) ? s.crew[p.promiser].name : std::string("command");
	bool found = false;
	for (Memory &m : c.memories)
		if (m.event == MEM_PROMISE && m.person == p.promiser) {
			m.valence = kept ? 1.0f : -0.9f;
			m.salience = 1.0f;
			found = true;
			break;
		}
	if (!found) Remember(s, p.beneficiary, MEM_PROMISE, p.promiser, MEM_SAW, kept ? 0.6f : -0.9f);
	LogEvent(s, c.name, "crew", c.name + ": " + promiser + " said " + p.what
		+ (kept ? ". It was done." : ". It was not done."));
	return true;
}

// A deadline that passes with nothing said is a promise broken (docs/memory-and-consequence.md).
static void MaturePromises(Ship &s)
{
	for (int i = 0; i < static_cast<int>(s.promises.size()); ++i) {
		const Promise &p = s.promises[i];
		if (p.state != PROMISE_OPEN || p.deadline < 0.0) continue;
		if (s.clock > p.deadline) ResolvePromise(s, i, false);
	}
}

const std::vector<Promise> &Promises(const Ship &s) { return s.promises; }

// ---- the month report (docs/the-record-and-the-log.md) -----------------------------------------

// ---- the navigation counter (docs/navigation-counter.md) ------------------------------------------

// The resupply the route offers: a source (a belt to mine, a trader, a derelict to salvage) that has
// been surveyed or visited, so the crew know where to detour for a crystal. A read of the chart.
static bool SupplyCharted(const Ship &s)
{
	for (const Beacon &b : s.sector)
		if ((b.surveyed || b.visited) && (b.kind == BEACON_BELT || b.kind == BEACON_TRADER || b.kind == BEACON_DERELICT))
			return true;
	return false;
}

// Light years still to travel, from the position model already in the save. The crossing is
// SECTORS_TO_CROSS sectors of SECTOR_BEACONS-1 forward jumps each; the ship has made the sectors
// behind her plus the beacons reached in this one, and the goal beacon of the last sector is home.
// The total and the goal are canon; mapping the beacon steps onto it is [inv].
static float DistanceRemaining(const Ship &s)
{
	if (s.won) return 0.0f;
	const int steps = SECTORS_TO_CROSS * (SECTOR_BEACONS - 1);
	int made = s.sectorNumber * (SECTOR_BEACONS - 1) + s.beacon;
	if (made < 0) made = 0;
	if (made > steps) made = steps;
	return NAV_LIGHT_YEARS * static_cast<float>(steps - made) / static_cast<float>(steps);
}

Navigation NavigationCounter(const Ship &s)
{
	Navigation nav;
	nav.distanceLy = DistanceRemaining(s);
	nav.nominalYears = nav.distanceLy / NAV_NOMINAL_C;
	// No crystal, no drive, or no pylons: sublight only. The honest conditional is not a bigger
	// number, it is that home stops getting closer (docs/exploration-and-science.md).
	if (!WarpPossible(s)) {
		nav.warp = false;
		nav.speedC = 0.0f;
		nav.currentYears = -1.0f;
		nav.changeYears = 0.0f;
		return nav;
	}
	// The speed the ship can actually sustain: canon's rate, made the ship's own. Every factor is 1
	// when the ship is whole, so a healthy ship reads the nominal figure bar the route's uncertainty;
	// a researched crystal (quality > 1) is the one factor allowed above 1, and it shortens the
	// journey -- the positive loop of docs/exploration-and-science.md.
	const float crystal = CRYSTAL_SPEED_FLOOR + (1.0f - CRYSTAL_SPEED_FLOOR) * Clamp01(s.dilithium);
	const float engine = ENGINE_SPEED_FLOOR + (1.0f - ENGINE_SPEED_FLOOR) * Clamp01(s.systems[SYS_WARP_DRIVE].output);
	const int need = SPECS[SYS_WARP_DRIVE].crewNeeded;
	const float manning = need > 0 ? std::min(1.0f, s.systems[SYS_WARP_DRIVE].staffing / need) : 1.0f;
	const float crew = CREW_SPEED_FLOOR + (1.0f - CREW_SPEED_FLOOR) * Clamp01(manning);
	const float supply = SupplyCharted(s) ? 1.0f : UNCHARTED_SUPPLY;
	nav.speedC = NAV_NOMINAL_C * std::max(0.0f, s.crystalQuality) * crystal * engine * crew * supply;
	nav.currentYears = nav.speedC > 0.0f ? nav.distanceLy / nav.speedC : -1.0f;
	nav.changeYears = nav.currentYears - s.navCounterLast;
	return nav;
}

std::vector<NavCourse> NavigationForecasts(const Ship &s)
{
	std::vector<NavCourse> out;
	if (s.sector.empty() || s.beacon < 0 || s.beacon >= static_cast<int>(s.sector.size())) return out;
	const Navigation here = NavigationCounter(s);
	const int steps = SECTORS_TO_CROSS * (SECTOR_BEACONS - 1);
	for (int l : s.sector[s.beacon].links) {
		if (l < 0 || l >= static_cast<int>(s.sector.size())) continue;
		NavCourse c;
		c.beacon = l;
		c.kind = s.sector[l].kind;
		c.charted = s.sector[l].surveyed || s.sector[l].visited;
		// Where the jump leaves the ship: a forward link is closer to home, a backward link is a
		// detour and costs the difference. Reaching the goal beacon crosses the sector.
		int made = s.sectorNumber * (SECTOR_BEACONS - 1) + l;
		if (made < 0) made = 0;
		if (made > steps) made = steps;
		c.distanceLy = NAV_LIGHT_YEARS * static_cast<float>(steps - made) / static_cast<float>(steps);
		c.years = (here.warp && here.speedC > 0.0f) ? c.distanceLy / here.speedC : -1.0f;
		out.push_back(c);
	}
	return out;
}

// The counter is written to the log periodically, so the crew can look back -- "when we crossed that
// expanse we were sixty-one years out" (docs/navigation-counter.md). Doing it here also moves the
// derivative's baseline, so the report's change means "since the counter was last written down".
static void RecordNavigation(Ship &s)
{
	const Navigation nav = NavigationCounter(s);
	char buf[160];
	if (!nav.warp)
		std::snprintf(buf, sizeof(buf), "navigation: no warp, home stops getting closer; %d light years out",
			static_cast<int>(nav.distanceLy + 0.5f));
	else
		std::snprintf(buf, sizeof(buf), "navigation: %d light years from home, %d years nominal, %d at current capability",
			static_cast<int>(nav.distanceLy + 0.5f), static_cast<int>(nav.nominalYears + 0.5f),
			static_cast<int>(nav.currentYears + 0.5f));
	LogEvent(s, AuthorFor(s, DEPT_COMMAND, "the bridge"), "bridge", buf);
	s.navCounterLast = nav.warp ? nav.currentYears : 0.0f;
}

const char *ReportAudienceName(uint8_t a)
{
	static const char *const N[REPORT_AUDIENCE_COUNT] = { "the crew", "upward" };
	return a < REPORT_AUDIENCE_COUNT ? N[a] : "?";
}

static std::string NameAt(const Ship &s, int person)
{
	if (person >= 0 && person < static_cast<int>(s.crew.size())) return s.crew[person].name;
	return "someone";
}

// Did this character see the event this line speaks to? Only a mark sourced MEM_SAW is proof
// against a signed report (docs/the-record-and-the-log.md).
static bool HoldsSaw(const CrewMember &who, uint16_t event, int person)
{
	for (const Memory &m : who.memories)
		if (m.event == event && m.person == person && m.source == MEM_SAW) return true;
	return false;
}

// Do they know of it at all? A told or rumoured version is not overwritten by reading the log.
static bool HoldsMark(const CrewMember &who, uint16_t event, int person)
{
	for (const Memory &m : who.memories)
		if (m.event == event && m.person == person) return true;
	return false;
}

// How a mark of this event would normally feel, for the crew who read of it rather than saw it.
static float EventValence(uint16_t event)
{
	switch (event) {
	case MEM_DEATH: return -0.6f;
	case MEM_RESCUE: return 0.7f;
	case MEM_VIOLATION: return -0.5f;
	case MEM_ORDER: return 0.1f;
	case MEM_FUNERAL: return 0.4f;
	case MEM_LOCKOUT: return -0.6f;
	default: return 0.0f;
	}
}

// The time the last signed report covered to; the draft covers everything since.
static double LastReportTime(const Ship &s)
{
	return s.reports.empty() ? 0.0 : s.reports.back().time;
}

void DraftReport(Ship &s, int department)
{
	MonthReport r;
	r.number = static_cast<int>(s.reports.size()) + 1;
	r.time = s.clock;
	r.department = (department >= 0 && department < DEPT_COUNT) ? department : DEPT_COUNT;
	const Navigation nav = NavigationCounter(s);
	r.counter = nav.warp ? nav.currentYears : -1.0f;
	r.counterChange = nav.warp ? nav.currentYears - s.navCounterLast : 0.0f;

	char buf[256];
	// The headline: the counter's change since the last entry. The derivative is the story.
	if (!nav.warp)
		std::snprintf(buf, sizeof(buf), "the ship has no warp: %d light years out, and home stops getting closer",
			static_cast<int>(nav.distanceLy + 0.5f));
	else
		std::snprintf(buf, sizeof(buf), "home %d light years; %d years nominal, %d now, %+.1f since the last entry",
			static_cast<int>(nav.distanceLy + 0.5f), static_cast<int>(nav.nominalYears + 0.5f),
			static_cast<int>(nav.currentYears + 0.5f), static_cast<double>(r.counterChange));
	{ ReportLine h; h.scope = "bridge"; h.draft = buf; h.text = buf; r.lines.push_back(h); }

	// The ship's condition, honestly, from the record.
	int lost = 0, wounded = 0, assimilated = 0;
	for (const CrewMember &c : s.crew) {
		if (c.status == CREW_DEAD) ++lost;
		else if (c.status == CREW_ASSIMILATED) ++assimilated;
		else if (c.status == CREW_INJURED) ++wounded;
	}
	std::snprintf(buf, sizeof(buf), "the crew stands at %d of %d; %d wounded, %d lost, %d assimilated",
		s.CrewFit(), static_cast<int>(s.crew.size()), wounded, lost, assimilated);
	{ ReportLine l; l.scope = "crew"; l.draft = buf; l.text = buf; r.lines.push_back(l); }

	// The period's witnessed events. These are the lines a signed report can be held against: each
	// speaks to a mark whose source is MEM_SAW, and the same event seen by three people drafts once.
	const double since = LastReportTime(s);
	std::vector<uint32_t> seen;
	for (const CrewMember &c : s.crew) {
		if (c.status == CREW_DEAD || c.status == CREW_ASSIMILATED) continue;
		for (const Memory &m : c.memories) {
			if (m.source != MEM_SAW) continue;
			if (m.time <= since) continue;
			if (m.event == MEM_LIE || m.event == MEM_PROMISE) continue; // private, not the record's business
			const uint32_t key = (static_cast<uint32_t>(m.event) << 16) | static_cast<uint16_t>(m.person + 1);
			bool dup = false;
			for (uint32_t k : seen) if (k == key) { dup = true; break; }
			if (dup) continue;
			ReportLine l;
			l.event = m.event;
			l.person = m.person;
			switch (m.event) {
			case MEM_DEATH: l.scope = "crew"; l.draft = "we lost " + NameAt(s, m.person); break;
			case MEM_RESCUE: l.scope = "crew"; l.draft = "we brought back " + NameAt(s, m.person); break;
			case MEM_VIOLATION: l.scope = "command"; l.draft = "the Prime Directive was set aside"; break;
			case MEM_ORDER: l.scope = "command"; l.draft = "an order was given and carried out"; break;
			case MEM_FUNERAL: l.scope = "crew"; l.draft = "we buried the lost together and took heart"; break;
			case MEM_LOCKOUT: l.scope = "command"; l.draft = NameAt(s, m.person) + " locked a hand out of a station"; break;
			default: continue;
			}
			l.text = l.draft;
			seen.push_back(key);
			r.lines.push_back(l);
			if (static_cast<int>(r.lines.size()) >= REPORT_LINE_MAX) break;
		}
	}
	s.report = r;
}

const MonthReport &OpenReport(const Ship &s) { return s.report; }
const std::vector<MonthReport> &Reports(const Ship &s) { return s.reports; }
bool LogsPurged(const Ship &s) { return s.logPurged; }

bool StrikeReportLine(Ship &s, int line)
{
	if (!s.report.open || line < 0 || line >= static_cast<int>(s.report.lines.size())) return false;
	s.report.lines[line].struck = true;
	return true;
}

bool EditReportLine(Ship &s, int line, const std::string &text)
{
	if (!s.report.open || line < 0 || line >= static_cast<int>(s.report.lines.size())) return false;
	if (text.empty() || text.size() > 96) return false;
	s.report.lines[line].text = text;
	s.report.lines[line].struck = false;
	return true;
}

// Soften a number: the claim most often softened is the one with a figure in it. The first run of
// digits in the line is scaled, and the record keeps the drafted number beside it.
bool SoftenReportLine(Ship &s, int line, float factor)
{
	if (!s.report.open || line < 0 || line >= static_cast<int>(s.report.lines.size())) return false;
	if (!(factor > 0.0f && factor < 1.0f)) return false;
	ReportLine &l = s.report.lines[line];
	std::string &t = l.text;
	size_t i = 0;
	while (i < t.size() && !(t[i] >= '0' && t[i] <= '9')) ++i;
	if (i >= t.size()) return false;
	size_t j = i;
	while (j < t.size() && t[j] >= '0' && t[j] <= '9') ++j;
	const long v = std::atol(t.substr(i, j - i).c_str());
	const long nv = static_cast<long>(static_cast<double>(v) * factor);
	t = t.substr(0, i) + std::to_string(nv) + t.substr(j);
	l.struck = false;
	return true;
}

bool AddReportLine(Ship &s, const std::string &scope, const std::string &text)
{
	if (!s.report.open || text.empty() || text.size() > 96) return false;
	if (static_cast<int>(s.report.lines.size()) >= REPORT_LINE_MAX) return false;
	ReportLine l;
	l.scope = scope.empty() ? "bridge" : scope;
	l.text = text;
	l.added = true;
	s.report.lines.push_back(l);
	return true;
}

// Sign and publish. This is where the lie is made with the player's hands: a signed line that
// contradicts a MEM_SAW mark puts a lie in the witness, and -- only where the witness is under the
// signer and reads the report -- the witness's bond toward the signer falls. Lying up, or into a
// report nobody below reads, costs nothing from below (docs/the-record-and-the-log.md).
bool SignReport(Ship &s, int signer, uint8_t audience)
{
	if (!s.report.open || s.report.lines.empty()) return false;
	if (signer < 0 || signer >= static_cast<int>(s.crew.size())) return false;
	if (audience >= REPORT_AUDIENCE_COUNT) return false;
	MonthReport &r = s.report;
	r.audience = audience;
	r.signer = s.crew[signer].name;
	r.time = s.clock;

	for (const ReportLine &l : r.lines)
		if (!l.struck && !l.text.empty()) LogEvent(s, r.signer, l.scope, l.text);

	const CrewMember &sig = s.crew[signer];
	for (ReportLine &l : r.lines) {
		if (l.event == 0 || l.person < 0) continue;
		const bool contradicted = l.struck || l.text != l.draft;
		if (!contradicted) continue;
		if (audience != REPORT_TO_CREW) continue; // filed upward: nobody below reads it
		for (int i = 0; i < static_cast<int>(s.crew.size()); ++i) {
			CrewMember &w = s.crew[i];
			if (w.status == CREW_DEAD || w.status == CREW_ASSIMILATED) continue;
			if (w.rank >= sig.rank) continue; // the toll is paid downward, only
			if (!HoldsSaw(w, l.event, l.person)) continue;
			// Divergent allegiance amplifies, alignment suppresses; a bad history amplifies too.
			// The magnitudes are invented [inv]; the direction is the document's.
			float factor = (w.faction != sig.faction) ? 1.6f : 0.5f;
			if (Bond(s, i, signer) < 0.0f) factor *= 1.3f;
			const float v = std::max(-1.0f, -0.6f * factor);
			Remember(s, i, MEM_LIE, signer, MEM_SAW, v);
			LogEvent(s, w.name, "crew", w.name + " saw " + l.draft + ", and heard it denied");
		}
	}

	// The crew who did not see it read the published version and remember it as read (MEM_LOG):
	// the crew can only ever see what was signed. A purge orphans this class of mark.
	if (audience == REPORT_TO_CREW) {
		for (int i = 0; i < static_cast<int>(s.crew.size()); ++i) {
			CrewMember &w = s.crew[i];
			if (w.status == CREW_DEAD || w.status == CREW_ASSIMILATED) continue;
			for (const ReportLine &l : r.lines) {
				if (l.event == 0 || l.person < 0 || l.struck) continue;
				if (HoldsMark(w, l.event, l.person)) continue; // they already know it, by some provenance
				Remember(s, i, l.event, l.person, MEM_LOG, EventValence(l.event));
			}
		}
	}

	r.open = false;
	r.signed_ = true;
	if (r.counter >= 0.0f) s.navCounterLast = r.counter; // the derivative's baseline (no warp: leave it)
	s.reports.push_back(r);
	if (static_cast<int>(s.reports.size()) > REPORT_MAX) s.reports.erase(s.reports.begin());
	s.report = MonthReport(); // a fresh, empty draft
	return true;
}

std::string ReportDiff(const MonthReport &r)
{
	std::string out;
	for (const ReportLine &l : r.lines) {
		if (l.struck) { out += "- " + l.draft + "\n"; continue; }
		if (l.added) { out += "+ " + l.text + "\n"; continue; }
		if (l.text != l.draft) { out += "- " + l.draft + "\n+ " + l.text + "\n"; }
	}
	return out;
}

// Purge the published logs (docs/the-record-and-the-log.md). It defends against readers and never
// against assimilation: the Collective takes the log from the mind. What it leaves is the marks
// sourced read-it-in-the-log, still held, with their citation gone. It is the *published* log it
// takes: the personal log is private, not published, and is left exactly as it was.
bool PurgeLogs(Ship &s)
{
	if (s.log.empty()) return false;
	s.log.clear(); // the hole where the log was; the record keeps only the diff
	s.logPurged = true;
	for (CrewMember &c : s.crew)
		for (Memory &m : c.memories)
			if (m.source == MEM_LOG) m.orphaned = true;
	return true;
}

// The holodeck's uses. Recreation lifts the mood; training grants a credential; therapy fades the
// salience of what a character carries; forensic reconstruction reports what they remember.
bool RunHolodeck(Ship &s, HolodeckUse use, int crew)
{
	if (s.systems[SYS_HOLODECKS].output <= 0.0f) return false;
	if (crew < 0 || crew >= static_cast<int>(s.crew.size())) return false;
	CrewMember &c = s.crew[crew];
	if (c.status != CREW_FIT || c.brigged) return false;
	switch (use) {
	case HOLO_RECREATION:
		c.outlook = std::min(1.0f, c.outlook + 0.15f);  // a night on the holodeck lifts the outlook
		c.deficit = std::max(0.0f, c.deficit - 0.15f);  // and eases what they are short of
		c.holoCompulsion = std::min(1.5f, c.holoCompulsion + 0.3f); // time in the program adds up
		LogEvent(s, c.name, "crew", c.name + " takes recreation on the holodeck");
		break;
	case HOLO_TRAINING: {
		Station st = STN_OPS;
		switch (c.dept) {
		case DEPT_ENGINEERING: st = STN_ENGINEERING; break;
		case DEPT_SECURITY: st = STN_TACTICAL; break;
		case DEPT_MEDICAL: st = STN_SICKBAY; break;
		case DEPT_COMMAND: case DEPT_SCIENCES: st = STN_CONN; break;
		default: break;
		}
		if (Qualified(c, st)) return false;
		c.credentials = static_cast<uint8_t>(c.credentials | (1u << st));
		LogEvent(s, c.name, "crew", c.name + " trains on the holodeck and qualifies at " + std::string(StationName(st)));
		break; }
	case HOLO_THERAPY:
		for (Memory &m : c.memories) if (m.valence < 0.0f) m.salience *= 0.4f;
		c.holdings = std::min(1.0f, c.holdings + 0.10f); // what they carry weighs less on who they hold with
		LogEvent(s, c.name, "sickbay", c.name + " works through it in the holodeck");
		break;
	case HOLO_FORENSIC:
		LogEvent(s, "the holodeck", "crew", c.name + "'s account is reconstructed (" + std::to_string(MemoryCount(c)) + " marks)");
		break;
	default:
		return false;
	}
	return true;
}

// The program that will not end: pulling a crew member out of the holodeck when its hold on them has
// gone too far.
bool EndHolodeckProgram(Ship &s, int crew)
{
	if (crew < 0 || crew >= static_cast<int>(s.crew.size())) return false;
	CrewMember &c = s.crew[crew];
	if (c.holoCompulsion < 1.0f) return false;
	c.holoCompulsion = 0.0f;
	LogEvent(s, CommandingOfficer(s), "crew", c.name + " is pulled out of the holodeck");
	return true;
}

// The holodeck matrix is a trap, not a solution (docs/ship-systems.md, "Macrocosm"/"Parallax"): when
// the main grid is down, the holodeck reactors look like an independent source to jump-start from,
// and the matrices are incompatible -- tapping one blows half the ship's relays. It gives back a
// little power now and wrecks systems across her, so it is worse than the problem it solves.
bool JumpStartFromHolodeck(Ship &s)
{
	if (!Coreless(s)) {
		LogEvent(s, AuthorFor(s, DEPT_ENGINEERING, "engineering"), "engineering",
			"there is no need to jump-start from a holodeck reactor: the main grid is up");
		return false;
	}
	if (s.systems[SYS_HOLODECKS].health <= 0.0f) {
		LogEvent(s, AuthorFor(s, DEPT_ENGINEERING, "engineering"), "engineering",
			"the holodeck reactors are wrecked; there is nothing to jump-start from");
		return false;
	}
	// The temptation: the cells take a charge off the holodeck reactor, so some power comes back.
	s.stores.batteries = std::min(1.0f, s.stores.batteries + 0.5f);
	// The cost: the matrices are incompatible, and the cross-tie blows relays across her. Half the
	// systems take the hit, deterministically, and the holodecks do not survive it. [inv number;
	// canon gives "half the ship's relays"]
	int ruined = 0;
	for (int i = 0; i < SYS_COUNT; ++i) {
		if (i == SYS_HOLODECKS) { s.systems[i].health = 0.0f; ++ruined; continue; }
		if (i % 2 == 0) { s.systems[i].health = Clamp01(s.systems[i].health - 0.5f); ++ruined; }
	}
	LogEvent(s, AuthorFor(s, DEPT_ENGINEERING, "engineering"), "engineering",
		"jump-started from a holodeck reactor: the matrices are incompatible and the cross-tie blows half the ship's relays ("
		+ std::to_string(ruined) + " systems ruined)");
	return true;
}

// Living conditions: better quarters, at a cost in material.
bool ImproveQuarters(Ship &s)
{
	if (s.stores.materials < 10.0f) return false;
	s.stores.materials -= 10.0f;
	for (CrewMember &c : s.crew) c.quartersQuality = std::min(1.0f, c.quartersQuality + 0.1f);
	LogEvent(s, CommandingOfficer(s), "crew", "the crew's quarters are improved");
	return true;
}

// Salience fades unless something retells or re-experiences the mark.
static void MemoryDecay(Ship &s, float shipSeconds)
{
	const float hours = shipSeconds / 3600.0f;
	for (CrewMember &c : s.crew)
		for (Memory &m : c.memories)
			m.salience = std::max(0.0f, m.salience - 0.01f * hours); // slow fade [inv]
}

// A death is seen by those on the deck and told to the rest only if command tells them.
static void NoteDeath(Ship &s, int deadIndex)
{
	CrewMember &dead = s.crew[deadIndex];
	// Grief's paperwork: the casualty is notified to command, and their quarters are sealed.
	dead.quartersSealed = true;
	LogEvent(s, AuthorFor(s, DEPT_COMMAND, "the bridge"), "crew", dead.name + "'s quarters are sealed");
	for (int i = 0; i < static_cast<int>(s.crew.size()); ++i) {
		if (i == deadIndex || s.crew[i].status != CREW_FIT) continue;
		if (dead.deck >= 1 && s.crew[i].deck == dead.deck) Remember(s, i, MEM_DEATH, deadIndex, MEM_SAW, -0.8f);
	}
}

void Tick(Ship &s, float seconds)
{
	if (!(seconds >= 0.0f)) return;
	Advance(s, static_cast<double>(seconds) * ClockRate(s.cfg));
}

// The load the ship is under now. Battle stations is the worst: everything is run at once, at the
// edge of the budget. Yellow is a ship with a problem. Green is a quiet watch.
float StressNow(const Ship &s)
{
	return s.alert == ALERT_RED ? 1.0f : s.alert == ALERT_YELLOW ? 0.5f : 0.15f;
}

// Use a system under a load: the general mechanism. The odds come from the condition; the severity,
// when the draw is bad, from the condition and the load. Every anomaly is written down with the
// chain -- condition, load, what happened, who was at the console -- and at the severe end the
// system lets go at the console the operator is holding (docs/failure-is-content.md).
static uint8_t UseSystemAt(Ship &s, SystemId id, float stress, const std::string &who, int operatorCrew)
{
	if (id >= SYS_COUNT) return ANOMALY_NONE;
	System &sys = s.systems[id];
	const float condition = SystemCondition(sys);

	// The operator: the named person, or the station's own hand. The four kinds and morale reach the
	// odds through the factors, each named, so a bad outcome says what did it (docs/character-
	// attributes.md, "the wiring").
	int op = operatorCrew;
	if (op < 0) {
		for (int i = 0; i < static_cast<int>(s.crew.size()); ++i) {
			const CrewMember &c = s.crew[i];
			if (c.status == CREW_FIT && !c.brigged && c.post == id) { op = i; break; }
		}
	}
	WorkFactor factors[WORK_FACTOR_MAX];
	int nFactors = 0;
	if (op >= 0 && op < static_cast<int>(s.crew.size()))
		nFactors = WorkFactors(s.crew[op], DepartmentSkill(SPECS[id].dept), WorkContextOf(id),
			stress, factors, WORK_FACTOR_MAX);

	const uint8_t sev = RollWork(condition, stress, factors, nFactors,
		AnomalyRoll(s.riskRolls++, s.cfg.seed));
	if (sev == ANOMALY_NONE) return sev;

	const std::string author = who.empty() ? std::string(AuthorFor(s, SPECS[id].dept, SPECS[id].station)) : who;
	const int condPct = static_cast<int>(condition * 100.0f + 0.5f);
	const int loadPct = static_cast<int>(Clamp01(stress) * 100.0f + 0.5f);
	// The reason travels with the work: the log line names every factor that moved the odds, so the
	// outcome is attributable rather than reading as a random failure.
	std::string line = std::string(SPECS[id].name) + ": " + AnomalyName(sev) + " anomaly at "
		+ std::to_string(condPct) + "% condition under " + std::to_string(loadPct) + "% load";
	if (nFactors > 0) line += " -- " + WorkFactorLine(factors, nFactors);
	LogEvent(s, author, "engineering", line);
	// A scar at the least; a system that has just bitten is worse than it was.
	sys.health = Clamp01(sys.health - (sev == ANOMALY_DEGRADED ? 0.05f : sev == ANOMALY_ACUTE ? 0.15f : 0.3f));
	// The visible let-go: the console arcs at whoever is holding the controls, the one place on the
	// ship the operator is standing. This is canon's exploding console, and it is the mechanism, not
	// a flourish. When the use is named (the player at the panel) it lands on that person; otherwise
	// it lands on the station's own hand.
	if (sev >= ANOMALY_ACUTE) {
		const int victim = op; // the operator, named or resolved above as the station's own hand
		if (victim >= 0) {
			CrewMember &c = s.crew[victim];
			c.status = CREW_INJURED;
			c.severity = std::max(c.severity, sev == ANOMALY_ACUTE ? 0.5f : 0.8f);
			LogEvent(s, AuthorFor(s, DEPT_MEDICAL, "sickbay"), "sickbay",
				c.name + " was hurt when the " + std::string(SPECS[id].name) + " console let go");
		}
	}
	return sev;
}

uint8_t UseSystem(Ship &s, SystemId id, float stress, const std::string &who)
{
	return UseSystemAt(s, id, stress, who, -1);
}

uint8_t UseSystemBy(Ship &s, SystemId id, float stress, int operatorCrew)
{
	const std::string who = (operatorCrew >= 0 && operatorCrew < static_cast<int>(s.crew.size()))
		? s.crew[operatorCrew].name : std::string();
	return UseSystemAt(s, id, stress, who, operatorCrew);
}

const char *SystemStateName(uint8_t state)
{
	static const char *const NAMES[SYS_STATE_COUNT] = { "nominal", "degraded", "offline", "destroyed" };
	return state < SYS_STATE_COUNT ? NAMES[state] : "?";
}

uint8_t FailureStateOf(float health, bool enabled)
{
	if (health <= 0.0f) return SYS_DESTROYED;
	if (!enabled || health < 0.33f) return SYS_OFFLINE;
	if (health < 0.66f) return SYS_DEGRADED;
	return SYS_NOMINAL;
}

// ---- condition sets the odds, and stress sets the severity (docs/failure-is-content.md) ----------

const char *AnomalyName(uint8_t severity)
{
	static const char *const NAMES[ANOMALY_SEVERITY_COUNT] = { "none", "degraded", "acute", "catastrophic" };
	return severity < ANOMALY_SEVERITY_COUNT ? NAMES[severity] : "?";
}

float SystemCondition(const System &sys)
{
	// Health is maintenance and parts; output is the power actually reaching it. A system is only as
	// good as the worse of the two -- an intact system with no power is not capable, and neither is a
	// powered one falling apart.
	return Clamp01(std::min(sys.health, sys.output));
}

float AnomalyOdds(float condition)
{
	if (condition >= NOMINAL_CONDITION) return 0.0f; // the top tenth: nominal, no consequence
	const float d = (NOMINAL_CONDITION - condition) / NOMINAL_CONDITION; // 0 at the line, 1 at nothing
	return ANOMALY_ODDS_MAX * d * d; // rising as the condition falls [inv]
}

uint8_t AnomalySeverityFor(float condition, float stress)
{
	const float c = Clamp01(condition);
	const float s = Clamp01(stress);
	// Both matter: how degraded the system is, and how hard it is being driven. A 40% system run
	// light gives the low severity; the same system at battle stations the high one. The condition
	// floor means a system at the end of its life lets go even on a quiet watch.
	const float bad = (1.0f - c) * (0.35f + 0.65f * s);
	if (bad < 0.33f) return ANOMALY_DEGRADED;
	if (bad < 0.60f) return ANOMALY_ACUTE;
	return ANOMALY_CATASTROPHIC;
}

uint8_t RollAnomaly(float condition, float stress, uint32_t roll)
{
	const float odds = AnomalyOdds(condition);
	if (odds <= 0.0f) return ANOMALY_NONE;
	const double u = static_cast<double>(roll) / 4294967296.0;
	if (u >= static_cast<double>(odds)) return ANOMALY_NONE;
	return AnomalySeverityFor(condition, stress);
}

// A deterministic draw for the next use: the counter and the seed, mixed so consecutive uses are
// not correlated. The counter is saved, so a load resumes the same sequence.
static uint32_t AnomalyRoll(uint32_t counter, uint32_t seed)
{
	uint32_t x = counter * 2654435761u + (seed ? seed : 1u);
	x ^= x >> 16; x *= 2246822519u; x ^= x >> 13; x *= 3266489917u; x ^= x >> 16;
	return x;
}

// ---- the four kinds reach the work: the operator's factors, named ---------------------------------
//
// docs/character-attributes.md, "the wiring". Each factor is the odds-shift of one kind, kept apart
// and named; a factor that does not move the odds is not returned, so the list is exactly what
// touched the outcome. The weights are small and invented [inv], so a condition can be *seen* to
// matter without any one of them deciding the roll alone.

const float WORK_SKILL_STANDARD = 2.0f;  // effective skill 2 is the competent hand for a use [inv]
const float WORK_SKILL_SLOPE = 0.10f;    // each point off it moves the odds a tenth [inv]
const float WORK_COND_SLIGHT = 0.15f;    // a slight debuff [inv]
const float WORK_COND_CLEAR = 0.30f;     // a clear debuff [inv]
const float WORK_COND_SHARP = 0.50f;     // a sharp debuff [inv]
const float WORK_MORALE_SLOPE = 0.40f;   // morale 0.5 is neutral; the shortfall scales the odds [inv]
const float WORK_MULT_MAX = 4.0f;        // the factor multiplier is bounded: no operator is a wall [inv]

const char *WorkFactorKindName(uint8_t k)
{
	static const char *const N[WORK_FACTOR_KIND_COUNT] = {"skill", "condition", "trait", "drive",
		"morale", "capability"};
	return k < WORK_FACTOR_KIND_COUNT ? N[k] : "factor";
}

const char *WorkContextName(uint8_t c)
{
	static const char *const N[WORK_CONTEXT_COUNT] = {"routine", "hazardous", "hull", "reclaim",
		"medical"};
	return c < WORK_CONTEXT_COUNT ? N[c] : "work";
}

uint8_t WorkContextOf(SystemId id)
{
	switch (id) {
	case SYS_STRUCTURAL_INTEGRITY: return WORK_HULL;
	case SYS_SICKBAY: return WORK_MEDICAL;
	case SYS_TRANSPORTERS: return WORK_HAZARD;
	default: return WORK_ROUTINE;
	}
}

static void AddWorkFactor(WorkFactor *out, int &n, int maxOut, uint8_t kind,
                          const std::string &name, float delta)
{
	if (delta == 0.0f || n >= maxOut) return;
	out[n].kind = kind; out[n].name = name; out[n].delta = delta; ++n;
}

int WorkFactors(const CrewMember &c, uint8_t skill, uint8_t context, float stress,
                WorkFactor *out, int maxOut)
{
	if (out == nullptr || maxOut <= 0) return 0;
	if (skill >= SKILL_COUNT || context >= WORK_CONTEXT_COUNT) return 0;
	int n = 0;
	const float s = Clamp01(stress);

	// Skill: what they can do. The rating plus the one aptitude trait that adds to it; the drag that
	// conditions and deficit impose is named below, so it is not counted twice here.
	float base = static_cast<float>(c.skills[skill]);
	if (HasTrait(c, TRAIT_FIRST_CONTACT_TRAINED) && (skill == SKILL_SCIENCE || skill == SKILL_COMMAND))
		base += 1.0f;
	{
		char buf[64];
		std::snprintf(buf, sizeof(buf), "%s skill %.1f", SkillName(skill), base);
		AddWorkFactor(out, n, maxOut, WORK_SKILL, buf, (WORK_SKILL_STANDARD - base) * WORK_SKILL_SLOPE);
	}

	// Conditions: temporary and situational. A debuff raises the odds while it holds; a buff lowers
	// them, by half as much [inv]. The magnitude word is the design's own; the weight is its number.
	for (int i = 0; i < c.conditionCount; ++i) {
		const Condition &k = c.conditions[i];
		if (k.id == COND_NONE) continue;
		const float mag = k.magnitude == CMAG_SHARP ? WORK_COND_SHARP
			: k.magnitude == CMAG_CLEAR ? WORK_COND_CLEAR : WORK_COND_SLIGHT;
		const float delta = k.valence == CVAL_DEBUFF ? mag : -0.5f * mag;
		AddWorkFactor(out, n, maxOut, WORK_CONDITION,
			std::string(ConditionName(k.id)) + " (" + ConditionMagnitudeName(k.magnitude) + ")", delta);
	}

	// Traits: durable and behavioural, and they reach the work by interacting with the moment rather
	// than as a flat shift. Steady under fire helps only under load; claustrophobia bites hardest on
	// a hull or hazardous task; adaptable softens the state drag. The physiological traits (needs
	// less sleep, quick healer) shape whether and how a person recovers, not the odds of one use.
	if (HasTrait(c, TRAIT_STEADY_UNDER_FIRE))
		AddWorkFactor(out, n, maxOut, WORK_TRAIT, "steady under fire (under load)", -0.30f * s);
	if (HasTrait(c, TRAIT_CLAUSTRAPHOBIC)) {
		const float tight = (context == WORK_HULL || context == WORK_HAZARD) ? 1.5f : 1.0f;
		AddWorkFactor(out, n, maxOut, WORK_TRAIT, "claustrophobic (under load)",
			(0.12f + 0.18f * s) * tight);
	}
	if (HasTrait(c, TRAIT_GOOD_WITH_PEOPLE)
	    && (skill == SKILL_MEDICAL || skill == SKILL_COMMAND || context == WORK_MEDICAL))
		AddWorkFactor(out, n, maxOut, WORK_TRAIT, "good with people", -0.12f);
	if (HasTrait(c, TRAIT_POOR_WITH_AUTHORITY))
		AddWorkFactor(out, n, maxOut, WORK_TRAIT, "poor with authority (under orders)", 0.10f * s);
	if (HasTrait(c, TRAIT_ADAPTABLE) && c.deficit > 0.05f)
		AddWorkFactor(out, n, maxOut, WORK_TRAIT, "adaptable", -0.10f);

	// Drives: a motive, and it biases what a person does and how they bear up, conditionally on the
	// work -- never a flat subtraction. A fear is realised by the task in front of them; a desire
	// leans in or holds back under load.
	if (c.fear == FEAR_DECOMPRESSION && context == WORK_HULL)
		AddWorkFactor(out, n, maxOut, WORK_DRIVE, "afraid of decompression (sealing the hull)", 0.25f);
	if (c.fear == FEAR_THE_BORG && context == WORK_RECLAIM)
		AddWorkFactor(out, n, maxOut, WORK_DRIVE, "afraid of the Borg (reclaiming a deck)", 0.25f);
	if (c.fear == FEAR_DYING_ALONE && context == WORK_HAZARD)
		AddWorkFactor(out, n, maxOut, WORK_DRIVE, "afraid of dying alone (a hazardous use)", 0.15f);
	if (s >= 0.5f) {
		if (c.desire == DESIRE_TO_PROVE || c.desire == DESIRE_PROMOTION)
			AddWorkFactor(out, n, maxOut, WORK_DRIVE, "wants promotion, and leans in", -0.10f);
		else if (c.desire == DESIRE_HOME)
			AddWorkFactor(out, n, maxOut, WORK_DRIVE, "wants to go home, and hesitates", 0.08f);
		else if (c.desire == DESIRE_TO_BE_LEFT_ALONE)
			AddWorkFactor(out, n, maxOut, WORK_DRIVE, "wants to be left alone, and does the minimum", 0.10f);
		if (c.fear == FEAR_USELESSNESS)
			AddWorkFactor(out, n, maxOut, WORK_DRIVE, "afraid of being useless, and works harder", -0.10f);
		if (c.fear == FEAR_COWARDICE)
			AddWorkFactor(out, n, maxOut, WORK_DRIVE, "afraid of being seen a coward (under load)", 0.15f);
	}

	// Morale: the read from the three components. A person who does not believe the course is worth
	// the cost works like one who does not; the reason names the component, so the log can say which.
	const float m = Morale(c);
	if (std::fabs(m - 0.5f) > 0.02f)
		AddWorkFactor(out, n, maxOut, WORK_MORALE,
			std::string("morale (") + MoraleBandName(m) + "): " + MoraleReason(c),
			(0.5f - m) * WORK_MORALE_SLOPE);

	// A species capability, two-sided. It helps where the faculty fits and costs where it does not;
	// the same faculty is the cost, so it is never a bonus. A Betazoid reads a patient's feeling for
	// good, and takes on the pain of a hull full of the hurt for bad.
	if (c.species == SPECIES_BETAZOID) {
		if (context == WORK_MEDICAL)
			AddWorkFactor(out, n, maxOut, WORK_CAPABILITY,
				"empathy: reads the feeling, not the thought", -0.15f);
		else if (context == WORK_HULL)
			AddWorkFactor(out, n, maxOut, WORK_CAPABILITY,
				"empathy: others' pain arrives uninvited", 0.15f);
	}

	return n;
}

float WorkOdds(float condition, const WorkFactor *f, int n)
{
	const float base = AnomalyOdds(condition);
	if (base <= 0.0f) return 0.0f; // the top tenth stays nominal, whatever the operator
	float mult = 1.0f;
	for (int i = 0; i < n; ++i) mult += f[i].delta;
	mult = std::max(0.0f, std::min(WORK_MULT_MAX, mult));
	return std::min(1.0f, base * mult);
}

uint8_t RollWork(float condition, float stress, const WorkFactor *f, int n, uint32_t roll)
{
	const float odds = WorkOdds(condition, f, n);
	if (odds <= 0.0f) return ANOMALY_NONE;
	const double u = static_cast<double>(roll) / 4294967296.0;
	if (u >= static_cast<double>(odds)) return ANOMALY_NONE;
	return AnomalySeverityFor(condition, stress);
}

std::string WorkFactorLine(const WorkFactor *f, int n)
{
	std::string line;
	char buf[96];
	for (int i = 0; i < n; ++i) {
		std::snprintf(buf, sizeof(buf), "%s %s %+.2f", WorkFactorKindName(f[i].kind),
			f[i].name.c_str(), f[i].delta);
		if (!line.empty()) line += "; ";
		line += buf;
	}
	return line;
}

std::string WorkReading(const Ship &s, int crew, SystemId id)
{
	if (crew < 0 || crew >= static_cast<int>(s.crew.size()) || id >= SYS_COUNT) return std::string();
	const CrewMember &c = s.crew[crew];
	const uint8_t skill = DepartmentSkill(SPECS[id].dept);
	WorkFactor factors[WORK_FACTOR_MAX];
	const int n = WorkFactors(c, skill, WorkContextOf(id), StressNow(s), factors, WORK_FACTOR_MAX);
	char eff[32];
	std::snprintf(eff, sizeof(eff), "%.1f", EffectiveSkill(c, skill));
	std::string line = c.name + " at the " + SPECS[id].name + " (" + WorkContextName(WorkContextOf(id))
		+ " work; effective " + SkillName(skill) + " " + eff + "): ";
	line += n > 0 ? WorkFactorLine(factors, n) : "no modifier applies";
	return line;
}

// Name every change of a system's failure state, so the log carries the risk and the consequence.
static void UpdateSystemStates(Ship &s)
{
	for (int i = 0; i < SYS_COUNT; ++i) {
		System &sys = s.systems[i];
		const uint8_t st = FailureStateOf(sys.health, sys.enabled);
		if (st == sys.fault) continue;
		LogEvent(s, AuthorFor(s, DEPT_ENGINEERING, "engineering"), "engineering",
			std::string(SPECS[i].name) + (st == SYS_NOMINAL ? " is restored" : std::string(" is ") + SystemStateName(st)));
		sys.fault = st;
	}
}

// Delegations lapse with the shift; an override takes when its clock runs out and lapses the same
// way. Both are acts with a beginning and an end, and both are written down when the state changes
// (docs/access-and-authority.md). A force by one hand costs the crew something: they saw command
// reach around the chain, and they remember it -- the log says who did it.
static void UpdateAccess(Ship &s)
{
	for (size_t i = 0; i < s.delegations.size();) {
		if (s.delegations[i].expires <= s.clock) s.delegations.erase(s.delegations.begin() + i);
		else ++i;
	}
	Override &o = s.emergencyOverride;
	if (o.station < 0) return;
	if (o.active) {
		if (s.clock >= o.expires) {
			LogEvent(s, AuthorFor(s, DEPT_COMMAND, "the bridge"), "command",
				std::string("the emergency override at ") + StationName(static_cast<Station>(o.station)) + " has lapsed");
			o = Override();
		}
		return;
	}
	if (s.clock < o.readyAt) return;
	o.active = true;
	o.solo = o.second < 0;
	o.expires = s.clock + OVERRIDE_DURATION_MINUTES * 60.0;
	LogEvent(s, AuthorFor(s, DEPT_COMMAND, "the bridge"), "command",
		std::string("EMERGENCY OVERRIDE ACTIVE: ") + StationName(static_cast<Station>(o.station))
		+ " answers to " + (o.solo ? "one hand" : "two officers") + " for "
		+ std::to_string(static_cast<int>(OVERRIDE_DURATION_MINUTES)) + " minutes");
	for (CrewMember &c : s.crew)
		if (c.status == CREW_FIT) c.outlook = std::max(0.0f, c.outlook - (o.solo ? 0.05f : 0.02f));
}

static void Advance(Ship &s, double shipSecondsTotal)
{
	// Long steps are cut up, so a paused or fast-forwarded ship arrives where a played one would.
	double shipSeconds = shipSecondsTotal;
	do {
		const float step = static_cast<float>(std::min(shipSeconds, 60.0));
		const double was = s.clock;
		s.clock += step;
		// The counter-play cooldowns age first, so anything that refreshes them this step survives it.
		s.remodulateCooldown = std::max(0.0f, s.remodulateCooldown - step);
		s.adaptationSuppressed = std::max(0.0f, s.adaptationSuppressed - step);
		UpdateCrew(s, step);
		UpdateAccess(s); // a shift's delegation lapses; an override takes or lapses
		MaturePromises(s); // a deadline that passes with nothing said is a promise broken
		UpdateSquad(s, step); // the squad's defenders are added before the fight is worked
		UpdateIntruders(s, step);
		UpdatePower(s, step);
		UpdateCore(s, step); // the warp core cascade, after power decides the core's condition
		UpdateOutside(s, step);
		UpdateDecks(s, step);
		UpdateFire(s, step);
		UpdateSystemStates(s); // name every system's failure-state change, from the tick's final health
		// The navigation counter, written to the log about weekly so the crew can look back -- and so
		// the report's change since the last entry has a baseline (docs/navigation-counter.md).
		if (step > 0.0f) {
			const double period = static_cast<double>(NAV_LOG_DAYS) * SECONDS_PER_DAY;
			if (std::floor(was / period) != std::floor(s.clock / period)) RecordNavigation(s);
		}
		shipSeconds -= step;
	} while (shipSeconds > 0.0);
	// The meeting clock (docs/staff-meetings.md): with time actually passing, the simulation emits any
	// brief that has fallen due. This is the emit site's caller -- every meeting produces a brief, the
	// ordinary ones as much as the dramatic. A zero-length tick (a load, a console read) emits nothing.
	if (shipSecondsTotal > 0.0) EmitDueMeetings(s);
}

void SetAlert(Ship &s, Alert a)
{
	if (s.alert == a) return;
	s.alert = a;
	static const char *const N[3] = { "green", "yellow", "red" };
	LogEvent(s, AuthorFor(s, DEPT_COMMAND, "the bridge"), "bridge", std::string("condition ") + N[a]);
}

// A hijacked system refuses its console. Power can still be cut at the source (SetSourceOnline),
// and the system can still be shot (DamageSystem): both are ways of denying it to the boarders.
void SetEnabled(Ship &s, SystemId id, bool on)
{
	if (id < SYS_COUNT && !Hijacked(s, id)) s.systems[id].enabled = on;
}

void SetPriority(Ship &s, SystemId id, int priority)
{
	if (id < SYS_COUNT && !Hijacked(s, id)) s.systems[id].priority = priority;
}

bool Hijacked(const Ship &s, SystemId id) { return id < SYS_COUNT && s.systems[id].control < HIJACKED; }

void BoardAs(Ship &s, int deck, int boarders, BoarderKind kind, int objective)
{
	if (deck < 1 || deck > DECKS || boarders <= 0) return;
	Deck &d = s.decks[deck - 1];
	d.intruders += boarders;
	d.boarderKind = static_cast<uint8_t>(kind < BOARDER_KIND_COUNT ? kind : BOARDER_RAIDER);
	d.borg = d.boarderKind == BOARDER_BORG;
	d.objective = objective >= 1 && objective <= DECKS ? objective : 0;
	LogEvent(s, AuthorFor(s, DEPT_SECURITY, "security"), "hull",
		std::to_string(boarders) + " boarders (" + BoarderKindName(static_cast<BoarderKind>(d.boarderKind)) + ") on deck " + std::to_string(deck));
}

void Board(Ship &s, int deck, int boarders) { BoardAs(s, deck, boarders, BOARDER_RAIDER, 0); }

void BoardBorg(Ship &s, int deck, int drones)
{
	if (deck < 1 || deck > DECKS || drones <= 0) return;
	s.decks[deck - 1].intruders += drones;
	s.decks[deck - 1].borg = true;
	s.decks[deck - 1].boarderKind = BOARDER_BORG;
}

bool DeckAssimilated(const Ship &s, int deck)
{
	return deck >= 1 && deck <= DECKS && s.decks[deck - 1].assimilated >= ASSIMILATED;
}

// De-assimilation (docs/borg-incursion.md): the person is only in the window while the nanoprobes
// are still counterable -- wounds in (0, RECOVERY_LIMIT). The cost and the lasting residue rise with
// how far it got, and it is never complete: they come back with a scar that colours them forever.
bool RecoverCaptive(Ship &s, int crew)
{
	if (crew < 0 || crew >= static_cast<int>(s.crew.size())) return false;
	CrewMember &c = s.crew[crew];
	if (c.status == CREW_DEAD || c.status == CREW_ASSIMILATED) return false; // already gone
	if (!(c.wounds > 0.0f) || c.wounds >= RECOVERY_LIMIT) return false;      // too late, or nothing to reverse
	if (s.systems[SYS_SICKBAY].output < TRACTOR_MIN) return false;
	const float supplies = c.wounds * RECOVERY_SUPPLIES;
	const float materials = c.wounds * RECOVERY_MATERIALS;
	if (s.stores.medicalSupplies < supplies || s.stores.materials < materials) return false;
	s.stores.medicalSupplies -= supplies;
	s.stores.materials -= materials;
	// The nanoprobes are pushed back; the residue is permanent.
	c.assimScar = std::max(c.assimScar, c.wounds);
	c.wounds = 0.0f;
	c.status = CREW_INJURED;
	c.severity = std::max(c.severity, c.assimScar * 0.5f);
	Remember(s, crew, MEM_VIOLATION, -1, MEM_SAW, -0.6f); // "I was taken, and I came back changed"
	LogEvent(s, AuthorFor(s, DEPT_MEDICAL, "sickbay"), "sickbay", c.name + " is de-assimilated, with lasting residue");
	return true;
}

void CounterHack(Ship &s, SystemId id, float strength)
{
	if (id >= SYS_COUNT || !(strength > 0.0f)) return;
	if (DeckAssimilated(s, SPECS[id].deck)) return; // there is no console left to hack from
	System &sys = s.systems[id];
	sys.control = std::min(1.0f, sys.control + std::min(1.0f, strength) * 0.5f); // a perfect run is worth half the system
}

int Intruders(const Ship &s)
{
	float n = 0.0f;
	for (const Deck &d : s.decks) n += d.intruders;
	return static_cast<int>(std::ceil(n - 1e-4f));
}

Breach MakeBreach(uint32_t seed)
{
	static const char *const CODES[] = {"1C", "55", "7A", "BD", "E9", "FF"};
	Breach b;
	uint32_t r = seed ? seed : 1;
	auto next = [&r]() { r = r * 1664525u + 1013904223u; return r >> 10; };
	for (int i = 0; i < b.size * b.size; ++i) b.grid.push_back(CODES[next() % 6]);
	// Targets are laid along a legal path through the grid, so every puzzle can be solved in full:
	// one walk of `buffer` cells, cut into a short, a medium and a long sequence that overlap.
	std::vector<int> path;
	std::vector<bool> used(b.grid.size(), false);
	int row = 0, col = static_cast<int>(next() % b.size);
	for (int step = 0; step < b.buffer; ++step) {
		int cell = row * b.size + col;
		for (int tries = 0; used[cell] && tries < b.size; ++tries) { // slide along the line to a free cell
			if (step % 2 == 0) col = (col + 1) % b.size; else row = (row + 1) % b.size;
			cell = row * b.size + col;
		}
		used[cell] = true;
		path.push_back(cell);
		if (step % 2 == 0) row = static_cast<int>(next() % b.size); else col = static_cast<int>(next() % b.size);
	}
	auto slice = [&](int from, int n) {
		std::vector<std::string> t;
		for (int i = from; i < from + n; ++i) t.push_back(b.grid[path[i]]);
		return t;
	};
	b.targets = {slice(0, 2), slice(1, 3), slice(3, 4)};
	return b;
}

float BreachScore(const Breach &b, const std::vector<int> &picks)
{
	const int cells = b.size * b.size;
	if (picks.empty() || static_cast<int>(picks.size()) > b.buffer || static_cast<int>(b.grid.size()) != cells) return 0.0f;
	std::vector<bool> used(cells, false);
	for (size_t i = 0; i < picks.size(); ++i) {
		const int c = picks[i];
		if (c < 0 || c >= cells || used[c]) return 0.0f;
		used[c] = true;
		if (i == 0) { if (c / b.size != 0) return 0.0f; continue; }          // start in the top row
		const int p = picks[i - 1];
		if (i % 2 == 1 ? c % b.size != p % b.size : c / b.size != p / b.size) return 0.0f; // down a column, then along a row
	}
	float got = 0.0f, total = 0.0f;
	for (size_t t = 0; t < b.targets.size(); ++t) {
		const std::vector<std::string> &want = b.targets[t];
		const float value = static_cast<float>(t + 1);
		total += value;
		for (size_t at = 0; at + want.size() <= picks.size(); ++at) {
			bool match = true;
			for (size_t k = 0; k < want.size() && match; ++k) match = b.grid[picks[at + k]] == want[k];
			if (match) { got += value; break; }
		}
	}
	return total > 0.0f ? got / total : 0.0f;
}

void SetSourceOnline(Ship &s, SourceId id, bool on)
{
	if (id < SRC_COUNT) s.sources[id].online = on;
}


void DamageSystem(Ship &s, SystemId id, float amount)
{
	if (id < SYS_COUNT && amount > 0.0f) {
		s.systems[id].health = Clamp01(s.systems[id].health - amount);
		LogEvent(s, AuthorFor(s, DEPT_ENGINEERING, "damage control"), "engineering", std::string(SPECS[id].name) + " damaged");
	}
}

void DamageSource(Ship &s, SourceId id, float amount)
{
	if (id < SRC_COUNT && amount > 0.0f) s.sources[id].health = Clamp01(s.sources[id].health - amount);
}

void DamagePylon(Ship &s, float amount)
{
	if (amount <= 0.0f) return;
	s.pylonHealth = Clamp01(s.pylonHealth - amount);
	LogEvent(s, AuthorFor(s, DEPT_ENGINEERING, "damage control"), "engineering", "the nacelle pylons are damaged");
}

bool PylonsIntact(const Ship &s) { return s.pylonHealth > 0.5f; }

// ---- the dilithium constraint (docs/exploration-and-science.md) --------------------------------

bool WarpPossible(const Ship &s)
{
	return !s.coreShutdown && !s.coreEjected && !s.lost
		&& s.dilithium > MIN_WARP_DILITHIUM
		&& s.systems[SYS_WARP_DRIVE].output >= 0.5f
		&& PylonsIntact(s);
}

// The warp core cascade (docs/failure-is-content.md): coolant loss -> overheat -> falling containment
// -> a breach countdown. The consoles show it; the crew can interrupt it several ways.
void UpdateCore(Ship &s, float shipSeconds)
{
	if (s.lost) return;
	const float minutes = shipSeconds / 60.0f;
	if (s.coreEjected) { s.coolant = 1.0f; s.coreTemp = 0.0f; s.containment = 1.0f; s.breachCountdown = -1.0f; return; }
	const float coreHealth = s.systems[SYS_WARP_DRIVE].health;
	if (!s.coreShutdown && coreHealth < 0.999f)
		s.coolant = std::max(0.0f, s.coolant - (1.0f - coreHealth) * minutes / COOLANT_LOSS_MINUTES);
	else
		s.coolant = std::min(1.0f, s.coolant + minutes / (COOLANT_LOSS_MINUTES * 2.0f));
	if (!s.coreShutdown && s.coolant < 0.999f)
		s.coreTemp = std::min(1.0f, s.coreTemp + (1.0f - s.coolant) * minutes / CONTAINMENT_MINUTES);
	else
		s.coreTemp = std::max(0.0f, s.coreTemp - minutes / (CONTAINMENT_MINUTES * 0.5f));
	if (s.coreTemp <= 0.5f)
		s.containment = std::min(1.0f, s.containment + minutes / (CONTAINMENT_MINUTES * 2.0f));
	else
		s.containment = std::max(0.0f, s.containment - (s.coreTemp - 0.5f) * minutes / CONTAINMENT_MINUTES);

	// A shut-down or ejected core cannot breach: the countdown only arms while the core is running.
	if (!s.coreShutdown && !s.coreEjected && s.containment <= CONTAINMENT_CRITICAL && s.breachCountdown < 0.0f) {
		s.breachCountdown = BREACH_SECONDS;
		LogEvent(s, AuthorFor(s, DEPT_ENGINEERING, "engineering"), "engineering",
			"core breach imminent: containment at " + std::to_string(static_cast<int>(s.containment * 100.0f + 0.5f)) + "%");
	}
	if (s.breachCountdown >= 0.0f) {
		if (s.containment > CONTAINMENT_CRITICAL + 0.05f) s.breachCountdown = -1.0f; // stabilised in time
		else {
			s.breachCountdown -= shipSeconds;
			if (s.breachCountdown <= 0.0f) { s.lost = true; LogEvent(s, AuthorFor(s, DEPT_ENGINEERING, "engineering"), "engineering", "the warp core breaches; the ship is lost"); }
		}
	}
}

bool ShutDownCore(Ship &s)
{
	if (s.coreShutdown || s.coreEjected || s.lost) return false;
	s.coreShutdown = true;
	s.breachCountdown = -1.0f;
	SetEnabled(s, SYS_WARP_DRIVE, false);
	LogEvent(s, CommandingOfficer(s), "engineering", "the warp core is shut down; the cascade is stopped, and warp with it");
	return true;
}

bool RestartCore(Ship &s)
{
	if (!s.coreShutdown || s.coreEjected || s.lost) return false;
	if (s.containment < 0.5f) return false; // too dangerous to restart yet
	s.coreShutdown = false;
	SetEnabled(s, SYS_WARP_DRIVE, true);
	LogEvent(s, CommandingOfficer(s), "engineering", "the warp core is restarted");
	return true;
}

bool EjectCore(Ship &s)
{
	if (s.coreEjected || s.lost) return false;
	s.coreEjected = true;
	s.coreShutdown = true;
	s.breachCountdown = -1.0f;
	s.coolant = 1.0f; s.coreTemp = 0.0f; s.containment = 1.0f;
	LogEvent(s, CommandingOfficer(s), "engineering", "the warp core is ejected; no warp until a new one is found");
	return true;
}

bool RestoreCoolant(Ship &s)
{
	if (s.coreEjected || s.lost) return false;
	if (s.coolant >= 0.999f) return false;
	if (s.stores.materials < COOLANT_MATERIALS) return false;
	s.stores.materials -= COOLANT_MATERIALS;
	s.coolant = std::min(1.0f, s.coolant + 0.5f);
	LogEvent(s, AuthorFor(s, DEPT_ENGINEERING, "engineering"), "engineering", "the coolant loops are refilled");
	return true;
}

bool CoreBreached(const Ship &s) { return s.lost; }

bool Recomposite(Ship &s)
{
	if (s.systems[SYS_WARP_DRIVE].output < RECOMPOSITE_WARP_MIN) return false;
	bool engineer = false;
	for (const CrewMember &c : s.crew) if (c.status == CREW_FIT && !c.brigged && c.dept == DEPT_ENGINEERING) { engineer = true; break; }
	if (!engineer) return false;
	const float target = std::min(s.crystalCeiling, s.dilithium + RECOMPOSITE_GAIN);
	if (target <= s.dilithium + 1e-4f) return false; // nothing left that recomposition can reach
	s.dilithium = target;
	s.crystalCeiling = std::max(MIN_WARP_DILITHIUM, s.crystalCeiling - RECOMPOSITE_CEILING_DROP);
	LogEvent(s, AuthorFor(s, DEPT_ENGINEERING, "engineering"), "engineering", "the dilithium is recomposited in the articulation frame");
	return true;
}

// A survey to locate a dilithium source (docs/exploration-and-science.md): chart the nearest belt,
// trader or derelict reachable through the sector, so the crew can detour before the crystal runs dry.
int LocateDilithium(Ship &s)
{
	if (s.sector.empty() || s.beacon < 0 || s.beacon >= static_cast<int>(s.sector.size())) return -1;
	if (s.systems[SYS_SENSORS].output <= 0.0f) return -1;
	// Breadth-first over the chart's links: the nearest beacon that could supply a crystal.
	std::vector<int> queue;
	std::vector<char> seen(s.sector.size(), 0);
	queue.push_back(s.beacon);
	seen[s.beacon] = 1;
	for (size_t i = 0; i < queue.size(); ++i) {
		const int at = queue[i];
		const BeaconKind k = s.sector[at].kind;
		if (at != s.beacon && (k == BEACON_BELT || k == BEACON_TRADER || k == BEACON_DERELICT)) {
			s.sector[at].surveyed = true;
			LogEvent(s, AuthorFor(s, DEPT_SCIENCES, "astrometrics"), "outside",
				std::string("the survey finds a dilithium source at beacon ") + std::to_string(at) + " (" + BeaconKindName(k) + ")");
			return at;
		}
		for (int l : s.sector[at].links)
			if (l >= 0 && l < static_cast<int>(s.sector.size()) && !seen[l]) { seen[l] = 1; queue.push_back(l); }
	}
	LogEvent(s, AuthorFor(s, DEPT_SCIENCES, "astrometrics"), "outside", "the survey finds no dilithium source in this sector");
	return -1;
}

bool AcquireDilithium(Ship &s, DilithiumWay way)
{
	if (s.sector.empty() || s.beacon < 0 || s.beacon >= static_cast<int>(s.sector.size())) return false;
	const Beacon &b = s.sector[s.beacon];
	switch (way) {
	case DIL_MINE:
		if (b.kind != BEACON_BELT) return false; // a belt to mine, an away mission into a hazard
		break;
	case DIL_TRADE:
		if (b.kind != BEACON_TRADER) return false;
		if (s.stores.materials < DILITHIUM_TRADE_MATERIALS) return false;
		s.stores.materials -= DILITHIUM_TRADE_MATERIALS;
		break;
	case DIL_SALVAGE:
		if (b.kind != BEACON_DERELICT) return false; // someone else's bad luck
		break;
	case DIL_RESEARCH:
		if (s.systems[SYS_COMPUTER_CORE].output < TRACTOR_MIN) return false; // the lab needs the computer core
		s.crystalQuality += CRYSTAL_QUALITY_STEP;
		LogEvent(s, AuthorFor(s, DEPT_SCIENCES, "the lab"), "outside", "a better dilithium: the lab improves the crystal's efficiency");
		return true;
	default:
		return false;
	}
	// A new crystal: full life, the ceiling reset, and one more replacement on the count.
	++s.crystalReplacements;
	s.dilithium = 1.0f;
	s.crystalCeiling = 1.0f;
	LogEvent(s, AuthorFor(s, DEPT_COMMAND, "the conn"), "outside", std::string("a new dilithium crystal ") + DilithiumWayName(way));
	return true;
}

int DilithiumRange(const Ship &s)
{
	return static_cast<int>(s.dilithium * DILITHIUM_LIGHT_YEARS * s.crystalQuality + 0.5f);
}

const char *DilithiumWayName(DilithiumWay way)
{
	static const char *const NAMES[DIL_WAY_COUNT] = { "mined from a belt", "traded for", "salvaged", "researched in the lab" };
	return way < DIL_WAY_COUNT ? NAMES[way] : "obtained";
}

// ---- shuttles (docs/shuttles.md) ---------------------------------------------------------------

const char *ShuttleClassName(ShuttleClass c)
{
	static const char *const NAMES[SHUTTLE_CLASS_COUNT] = { "Class 2", "Type 6", "Type 8", "Aeroshuttle" };
	return c < SHUTTLE_CLASS_COUNT ? NAMES[c] : "shuttle";
}

const char *ShuttleLocationName(ShuttleLocation l)
{
	static const char *const NAMES[3] = { "in the bay", "away", "lost" };
	return l <= SHUTTLE_LOST ? NAMES[l] : "?";
}

const Shuttle *ShuttleByClass(const Ship &s, ShuttleClass c)
{
	for (const Shuttle &sh : s.shuttles) if (sh.cls == c) return &sh;
	return NULL;
}

Shuttle *ShuttleByClass(Ship &s, ShuttleClass c)
{
	for (Shuttle &sh : s.shuttles) if (sh.cls == c) return &sh;
	return NULL;
}

int ShuttlesInBay(const Ship &s)
{
	int n = 0;
	for (const Shuttle &sh : s.shuttles) if (sh.location == SHUTTLE_IN_BAY) ++n;
	return n;
}

int ShuttlesAway(const Ship &s)
{
	int n = 0;
	for (const Shuttle &sh : s.shuttles) if (sh.location == SHUTTLE_AWAY) ++n;
	return n;
}

bool LaunchShuttle(Ship &s, ShuttleClass c, int beacon, const std::vector<int> &manifest)
{
	Shuttle *sh = ShuttleByClass(s, c);
	if (!sh || sh->location != SHUTTLE_IN_BAY || sh->condition <= 0.0f) return false;
	for (int idx : manifest) if (idx < 0 || idx >= static_cast<int>(s.crew.size()) || s.crew[idx].status != CREW_FIT) return false;
	sh->location = SHUTTLE_AWAY;
	sh->awayBeacon = beacon;
	sh->awaySince = static_cast<float>(s.clock);
	sh->comms = COMMS_LINKED;
	sh->manifest.clear();
	for (int idx : manifest) { s.crew[idx].away = true; sh->manifest.push_back(static_cast<int16_t>(idx)); }
	// What it carried is not aboard the ship any more.
	sh->cargoMaterial = std::min(s.stores.materials, 10.0f); s.stores.materials -= sh->cargoMaterial;
	sh->cargoRations = std::min(s.stores.rations, 10.0f); s.stores.rations -= sh->cargoRations;
	sh->cargoParts = std::min(s.stores.spareParts, 10.0f); s.stores.spareParts -= sh->cargoParts;
	LogEvent(s, AuthorFor(s, DEPT_COMMAND, "the conn"), "outside", std::string(sh->name) + " launches for beacon " + std::to_string(beacon) + " with " + std::to_string(sh->manifest.size()) + " aboard");
	return true;
}

bool RecallShuttle(Ship &s, ShuttleClass c)
{
	Shuttle *sh = ShuttleByClass(s, c);
	if (!sh || sh->location != SHUTTLE_AWAY) return false;
	for (int16_t idx : sh->manifest) if (idx >= 0 && idx < static_cast<int>(s.crew.size())) s.crew[idx].away = false;
	sh->manifest.clear();
	sh->location = SHUTTLE_IN_BAY;
	sh->awaySince = 0.0f;
	sh->comms = COMMS_LINKED;
	s.stores.materials += sh->cargoMaterial; s.stores.rations += sh->cargoRations; s.stores.spareParts += sh->cargoParts;
	sh->cargoMaterial = sh->cargoRations = sh->cargoParts = 0.0f;
	LogEvent(s, AuthorFor(s, DEPT_COMMAND, "the conn"), "outside", std::string(sh->name) + " is back in the bay");
	return true;
}

bool StrandShuttle(Ship &s, ShuttleClass c)
{
	Shuttle *sh = ShuttleByClass(s, c);
	if (!sh || sh->location != SHUTTLE_AWAY) return false;
	// The crew beam back and the shuttle is left where it is: a real option, and it costs a shuttle.
	for (int16_t idx : sh->manifest) if (idx >= 0 && idx < static_cast<int>(s.crew.size())) s.crew[idx].away = false;
	sh->manifest.clear();
	sh->location = SHUTTLE_LOST;
	sh->condition = 0.0f;
	LogEvent(s, AuthorFor(s, DEPT_COMMAND, "the conn"), "outside", std::string(sh->name) + " is left behind at beacon " + std::to_string(sh->awayBeacon));
	return true;
}

static void OrderShuttleBuildImpl(Ship &s, ShuttleClass c);

bool LoseShuttle(Ship &s, ShuttleClass c)
{
	Shuttle *sh = ShuttleByClass(s, c);
	if (!sh || sh->location == SHUTTLE_LOST) return false;
	// Everyone aboard is lost with it.
	for (int16_t idx : sh->manifest) if (idx >= 0 && idx < static_cast<int>(s.crew.size())) { s.crew[idx].away = false; s.crew[idx].status = CREW_DEAD; s.crew[idx].deck = 0; }
	sh->manifest.clear();
	sh->location = SHUTTLE_LOST;
	sh->condition = 0.0f;
	LogEvent(s, AuthorFor(s, DEPT_COMMAND, "the conn"), "outside", std::string(sh->name) + " is lost at beacon " + std::to_string(sh->awayBeacon));
	// Losing one is permanent: a replacement is a build job in the second bay, not a respawn.
	OrderShuttleBuildImpl(s, c);
	return true;
}

// The second bay's build order: a JOB_BUILD with a negative target names a shuttle class to build.
static void OrderShuttleBuildImpl(Ship &s, ShuttleClass c)
{
	for (const Job &j : s.jobs) if (j.kind == JOB_BUILD && j.target == -static_cast<int16_t>(c + 1)) return;
	if (static_cast<int>(s.jobs.size()) >= JOB_MAX) return;
	Job j;
	j.kind = JOB_BUILD;
	j.target = -static_cast<int16_t>(c + 1); // negative: a shuttle, not spare parts
	j.priority = 45;                          // after repairs, seals, reclamations and part builds
	s.jobs.push_back(j);
	LogEvent(s, CommandingOfficer(s), "engineering", std::string("the second bay is to build a ") + ShuttleClassName(c));
}

bool ShuttleBayHit(Ship &s, float severity)
{
	bool any = false;
	for (Shuttle &sh : s.shuttles)
	{
		if (sh.location != SHUTTLE_IN_BAY) continue;
		sh.condition = std::max(0.0f, sh.condition - severity);
		any = true;
		if (sh.condition <= 0.0f)
			LogEvent(s, "damage control", "hull", sh.name + " is wrecked in the bay");
	}
	return any;
}

bool RebuildShuttle(Ship &s, ShuttleClass c) { OrderShuttleBuildImpl(s, c); return ShuttleByClass(s, c) != NULL; }

bool SetMobileEmitter(Ship &s, bool on)
{
	s.mobileEmitter = on;
	LogEvent(s, "sickbay", "sickbay", on ? "the mobile emitter is brought aboard; the EMH can leave sickbay" : "the mobile emitter is stowed");
	return true;
}

bool MobileEmitter(const Ship &s) { return s.mobileEmitter; }

bool SetAirponics(Ship &s, bool on)
{
	if (s.airponics == on) return true;
	s.airponics = on;
	LogEvent(s, AuthorFor(s, DEPT_ENGINEERING, "the mess"), "crew", on ? "the airponics bay is growing food" : "the airponics bay is shut down");
	return true;
}

bool Airponics(const Ship &s) { return s.airponics; }

void BreachDeck(Ship &s, int deck, float amount)
{
	if (deck >= 1 && deck <= DECKS && amount > 0.0f) {
		s.decks[deck - 1].hull = Clamp01(s.decks[deck - 1].hull - amount);
		LogEvent(s, AuthorFor(s, DEPT_ENGINEERING, "damage control"), "hull", "hull breached on deck " + std::to_string(deck));
	}
}

void IgniteDeck(Ship &s, int deck, float amount)
{
	if (deck >= 1 && deck <= DECKS && amount > 0.0f) {
		s.decks[deck - 1].fire = Clamp01(s.decks[deck - 1].fire + amount);
		LogEvent(s, "damage control", "hull", "fire on deck " + std::to_string(deck));
	}
}

void Repair(Ship &s, SystemId id, float amount)
{
	if (id < SYS_COUNT && amount > 0.0f) s.systems[id].health = Clamp01(s.systems[id].health + amount);
}

void RepairDeck(Ship &s, int deck, float amount)
{
	if (deck >= 1 && deck <= DECKS && amount > 0.0f) {
		s.decks[deck - 1].hull = Clamp01(s.decks[deck - 1].hull + amount);
		LogEvent(s, AuthorFor(s, DEPT_ENGINEERING, "damage control"), "hull", "hull sealed on deck " + std::to_string(deck));
	}
}

void SetForceField(Ship &s, int deck, bool on)
{
	SetForceFieldLevel(s, deck, on ? FIELD_MAX : 0);
}

void SetForceFieldLevel(Ship &s, int deck, int level)
{
	if (deck < 1 || deck > DECKS) return;
	Deck &d = s.decks[deck - 1];
	d.forceFieldLevel = static_cast<float>(level < 0 ? 0 : (level > FIELD_MAX ? FIELD_MAX : level));
	d.forceField = d.forceFieldLevel > 0.0f;
	LogEvent(s, AuthorFor(s, DEPT_ENGINEERING, "environmental control"), "hull",
		d.forceField ? "force field raised on deck " + std::to_string(deck) + " at level " + std::to_string(static_cast<int>(d.forceFieldLevel))
			: "force field lowered on deck " + std::to_string(deck));
}

// The air clock: how long this deck has before it cannot be breathed, at the rates UpdateDecks uses.
// -1 means it is holding or refilling -- there is no countdown.
float MinutesOfAir(const Ship &s, int deck)
{
	if (deck < 1 || deck > DECKS) return -1.0f;
	const Deck &d = s.decks[deck - 1];
	if (d.atmosphere < AIRLESS) return 0.0f;
	const float support = s.systems[SYS_LIFE_SUPPORT].output;
	const float ventPerHour = d.forceField ? 0.0f : (1.0f - d.hull) * 60.0f / VENT_MINUTES;
	const float regenPerHour = support * d.hull / ATMOSPHERE_REGEN_HOURS;
	const float stalePerHour = (1.0f - support) / ATMOSPHERE_STALE_HOURS;
	const float net = regenPerHour - stalePerHour - ventPerHour;
	if (net >= 0.0f) return -1.0f;
	return (d.atmosphere - AIRLESS) / (-net) * 60.0f;
}

// The endurance of one source at its current draw, in minutes. -1 if it is not supplying now.
float EnduranceOf(const Ship &s, SourceId id)
{
	if (id >= SRC_COUNT) return -1.0f;
	const Source &src = s.sources[id];
	const SourceSpec &sp = SOURCES[id];
	if (!src.online || src.health <= 0.0f || src.output <= 0 || sp.capacity <= 0) return -1.0f;
	const float load = static_cast<float>(src.output) / sp.capacity;
	if (id == SRC_BATTERIES) return s.stores.batteries / load * BATTERY_HOURS * 60.0f;
	float days = 1.0e9f;
	if (sp.deuteriumPerDay > 0.0f) days = std::min(days, s.stores.deuterium / (sp.deuteriumPerDay * load));
	if (sp.antimatterPerDay > 0.0f) days = std::min(days, s.stores.antimatter / (sp.antimatterPerDay * load));
	return days * 24.0f * 60.0f;
}

// The power clock: how long until the first source supplying now runs out. -1 if nothing supplies.
float MinutesToDark(const Ship &s)
{
	float best = -1.0f;
	for (int i = 0; i < SRC_COUNT; ++i) {
		const float minutes = EnduranceOf(s, static_cast<SourceId>(i));
		if (minutes >= 0.0f && (best < 0.0f || minutes < best)) best = minutes;
	}
	return best;
}

// Per-person gravity: the world's gravity scaled by the deck's plating. A person on a deck whose
// plating has failed feels less of it; on a deck with hold, the world's own value stands.
float GravityScale(const Ship &s, int deck)
{
	if (deck < 1 || deck > DECKS) return 1.0f;
	return Clamp01(s.decks[deck - 1].gravity);
}

int ScaleGravity(int worldGravity, float scale)
{
	if (scale >= 1.0f) return worldGravity;
	if (scale <= 0.0f) return 0;
	const int scaled = static_cast<int>(static_cast<float>(worldGravity) * scale + 0.5f);
	return scaled < 0 ? 0 : (scaled > worldGravity ? worldGravity : scaled);
}

bool SetSurgicalField(Ship &s, bool on)
{
	if (s.surgicalForceField == on) return true;
	s.surgicalForceField = on;
	LogEvent(s, "sickbay", "sickbay", on ? "the surgical bay's force field is up" : "the surgical bay's force field is down");
	return true;
}

bool SurgicalField(const Ship &s) { return s.surgicalForceField; }

void LogEvent(Ship &s, const std::string &who, const std::string &scope, const std::string &what)
{
	LogEntry e;
	e.time = s.clock;
	e.who = who;
	e.scope = scope;
	e.what = what;
	s.log.push_back(e);
	if (static_cast<int>(s.log.size()) > LOG_MAX) s.log.erase(s.log.begin());
}

// ---- the two logs (docs/the-record-and-the-log.md) ----------------------------------------------
//
// The official log is above; this is the personal one, and the law over both: the simulation writes
// them and never reads either. Nothing here consults a log, and the month report is drafted from the
// record's marks. The private read returns one person's entries and nobody else's.

const char *LogVisibilityName(uint8_t v)
{
	static const char *const NAMES[LOG_VISIBILITY_COUNT] = { "official", "personal" };
	return v < LOG_VISIBILITY_COUNT ? NAMES[v] : "unknown";
}

// Write a private entry, in its owner's name. A full log drops its oldest entry, like the official
// one; a bad owner or empty text changes nothing. This is the player's own hand when `owner` is
// `s.player`, and it is the only way an entry enters the store.
bool WritePersonalLog(Ship &s, int owner, const std::string &what)
{
	if (owner < 0 || owner >= static_cast<int>(s.crew.size())) return false;
	if (what.empty()) return false;
	if (static_cast<int>(what.size()) > PERSONAL_LOG_TEXT_MAX) return false;
	PersonalLogEntry e;
	e.time = s.clock;
	e.owner = owner;
	e.who = s.crew[owner].name;
	e.what = what;
	e.visibility = LOG_PERSONAL;
	s.personalLog.push_back(e);
	if (static_cast<int>(s.personalLog.size()) > PERSONAL_LOG_MAX) s.personalLog.erase(s.personalLog.begin());
	return true;
}

// The visibility rule, in one place: a personal entry is visible only to its owner. The official
// log's scopes are the other half of the rule; a personal entry never appears in them.
bool PersonalVisibleTo(const PersonalLogEntry &e, int reader) { return e.owner == reader; }

// The private read: only this person's entries, newest last. It is a read of one store and returns
// nothing of any other person's.
std::vector<PersonalLogEntry> PersonalLog(const Ship &s, int owner)
{
	std::vector<PersonalLogEntry> out;
	if (owner < 0 || owner >= static_cast<int>(s.crew.size())) return out;
	for (const PersonalLogEntry &e : s.personalLog)
		if (PersonalVisibleTo(e, owner)) out.push_back(e);
	return out;
}

// The official read, factored out so the search is testable: the newest first, optionally filtered
// to one scope. Personal entries are never returned here, whatever the scope asked for.
std::vector<LogEntry> ReadOfficialLog(const Ship &s, int count, const std::string &scope)
{
	std::vector<LogEntry> out;
	for (int i = static_cast<int>(s.log.size()) - 1; i >= 0 && static_cast<int>(out.size()) < count; --i) {
		const LogEntry &e = s.log[i];
		if (!scope.empty() && e.scope != scope) continue;
		out.push_back(e);
	}
	return out;
}

// ---- what the ship has given up -----------------------------------------------------------------

const char *LossKindName(uint8_t k)
{
	static const char *const NAMES[LOSS_KIND_COUNT] = { "sealed", "stripped", "uninhabitable", "written off" };
	return k < LOSS_KIND_COUNT ? NAMES[k] : "lost";
}

bool WriteOff(Ship &s, bool system, int target, uint8_t kind)
{
	if (kind >= LOSS_KIND_COUNT) return false;
	int16_t t;
	std::string what;
	if (system) {
		if (target < 0 || target >= SYS_COUNT) return false;
		t = static_cast<int16_t>(target);
		what = SPECS[target].name;
	} else {
		if (target < 1 || target > DECKS) return false;
		t = static_cast<int16_t>(target);
		what = "deck " + std::to_string(target);
	}
	// A thing is on the list once: the first time it is given up is the moment that matters.
	for (const LossEntry &e : s.losses)
		if (e.system == system && e.target == t) return false;
	LossEntry e;
	e.time = s.clock;
	e.kind = kind;
	e.system = system;
	e.target = t;
	e.what = what;
	e.who = CommandingOfficer(s);
	s.losses.push_back(e);
	if (static_cast<int>(s.losses.size()) > LOSS_MAX) s.losses.erase(s.losses.begin());
	return true;
}

const std::vector<LossEntry> &WriteOffs(const Ship &s) { return s.losses; }

// ---- the job queue ----------------------------------------------------------------------------

const char *JobKindName(uint8_t k)
{
	static const char *const NAMES[JOB_KIND_COUNT] = {"repair", "seal", "reclaim", "build"};
	return k < JOB_KIND_COUNT ? NAMES[k] : "job";
}

const std::vector<Job> &Jobs(const Ship &s) { return s.jobs; }

// Rebuild the queue from the ship's state each tick: one job per damaged system, breached hull and
// assimilated deck, in the order it is worked. A job that persists keeps any priority command set on
// it; the captain's "see first to" puts a repair at the head.
static void UpdateJobs(Ship &s)
{
	std::vector<Job> next;
	// Build jobs are the player's, not derived from state: they carry through until they are done.
	for (const Job &j : s.jobs) if (j.kind == JOB_BUILD && j.target != 0) next.push_back(j);
	auto keepPriority = [&](uint8_t kind, int target, int16_t def) {
		for (const Job &j : s.jobs) if (j.kind == kind && j.target == target) return j.priority;
		return def;
	};
	for (int i = 0; i < SYS_COUNT; ++i) {
		if (s.systems[i].health >= 1.0f) continue;
		Job j;
		j.kind = JOB_REPAIR;
		j.target = static_cast<int16_t>(i);
		j.progress = 1.0f - s.systems[i].health;
		j.priority = static_cast<int16_t>(i == s.orderRepairFirst ? -1 : keepPriority(JOB_REPAIR, i, SPECS[i].priority));
		next.push_back(j);
	}
	for (int d = 0; d < DECKS; ++d) {
		if (s.decks[d].hull >= 1.0f) continue;
		Job j;
		j.kind = JOB_SEAL; j.target = static_cast<int16_t>(d + 1); j.progress = 1.0f - s.decks[d].hull;
		j.priority = static_cast<int16_t>(keepPriority(JOB_SEAL, d + 1, 20));
		next.push_back(j);
	}
	for (int d = 0; d < DECKS; ++d) {
		if (s.decks[d].assimilated <= 0.0f) continue;
		Job j;
		j.kind = JOB_RECLAIM; j.target = static_cast<int16_t>(d + 1); j.progress = 1.0f - s.decks[d].assimilated;
		j.priority = static_cast<int16_t>(keepPriority(JOB_RECLAIM, d + 1, 30));
		next.push_back(j);
	}
	std::stable_sort(next.begin(), next.end(), [](const Job &a, const Job &b) { return a.priority < b.priority; });
	if (static_cast<int>(next.size()) > JOB_MAX) next.resize(JOB_MAX);
	// A job that was not there before is written down, so the log traces the work that is due (and a
	// later failure can be traced to it).
	for (const Job &j : next) {
		bool existed = false;
		for (const Job &old : s.jobs) if (old.kind == j.kind && old.target == j.target) { existed = true; break; }
		if (!existed) {
			std::string what = std::string(JobKindName(j.kind)) + " ";
			what += j.kind == JOB_REPAIR ? SPECS[j.target].name
				: (j.kind == JOB_BUILD && j.target < 0 ? std::string(ShuttleClassName(static_cast<ShuttleClass>(-j.target - 1)))
				: (std::string("deck ") + std::to_string(j.target)));
			LogEvent(s, "damage control", j.kind == JOB_REPAIR ? "engineering" : "hull", std::string("work ordered: ") + what);
		}
	}
	s.jobs = next;
}

// ---- the meeting: the brief, the skeleton, and the seams (docs/staff-meetings.md) ---------------
//
// Phase one: text and state. No model is called, no sound is owned, no audio player is built. The
// simulation enqueues a brief (a read); every kind of meeting has an authored skeleton (the floor); and
// a chosen option resolves to one enumerated outcome the simulation applies. The invariant holds
// throughout: the simulation decides, and any model only speaks.

const char *MeetingKindName(uint8_t kind)
{
	static const char *const NAMES[MEET_KIND_COUNT] = {
		"watch-change", "departmental", "allocation", "dilithium", "casualties", "Borg", "deferred"
	};
	return kind < MEET_KIND_COUNT ? NAMES[kind] : "?";
}

const char *MeetingEffectName(uint8_t e)
{
	static const char *const NAMES[EFFECT_COUNT] = {
		"record", "set allocation", "automatic mode", "accept the chief's plan", "refuse the chief's plan",
		"triage order", "security to a deck", "evacuate a deck", "set the alert", "recomposite", "keep a promise"
	};
	return e < EFFECT_COUNT ? NAMES[e] : "?";
}

const char *DeliveryName(uint8_t d)
{
	static const char *const NAMES[DELIVERY_COUNT] = {
		"order", "report", "confession", "condolence", "flat", "unmarked"
	};
	return d < DELIVERY_COUNT ? NAMES[d] : "?";
}

// The review's own knob (docs/evidence/voice-review.md, finding three): `exaggeration` enters the
// model as `emotion_adv`. Up for urgency, down for the flat and procedural. UNMARKED asks for nothing.
float DeliveryExaggeration(uint8_t d)
{
	switch (d) {
		case DELIVERY_ORDER: return 0.8f;
		case DELIVERY_REPORT: return 0.5f;
		case DELIVERY_CONFESSION: return 0.35f;
		case DELIVERY_CONDOLENCE: return 0.3f;
		case DELIVERY_FLAT: return 0.2f;
		default: return -1.0f; // UNMARKED, or out of range: there is no value to ask for
	}
}

bool DeliveryKnown(uint8_t d) { return d < DELIVERY_UNMARKED; }

bool LineToSynthesis(const MeetingLine &line, SynthesisRequest &out)
{
	if (!DeliveryKnown(line.delivery)) return false; // marked, not guessed: refuse rather than default
	out.text = line.text;
	out.delivery = line.delivery;
	out.exaggeration = DeliveryExaggeration(line.delivery);
	return true;
}

int ResolveSpeaker(const Ship &s, const MeetingBrief &brief, int speaker)
{
	if (speaker >= 0) return speaker < static_cast<int>(s.crew.size()) ? speaker : -1;
	Department want = DEPT_COUNT;
	switch (speaker) {
		case SPEAK_COMMAND: want = DEPT_COMMAND; break;
		case SPEAK_ENGINEERING: want = DEPT_ENGINEERING; break;
		case SPEAK_SECURITY: want = DEPT_SECURITY; break;
		case SPEAK_SCIENCES: want = DEPT_SCIENCES; break;
		case SPEAK_MEDICAL: want = DEPT_MEDICAL; break;
		default: break; // SPEAK_ROOM
	}
	if (want < DEPT_COUNT) {
		for (int i = 0; i < brief.presentCount; ++i) {
			const int c = brief.present[i].crew;
			if (c >= 0 && c < static_cast<int>(s.crew.size()) && s.crew[c].dept == want) return c;
		}
		return DepartmentHead(s, want);
	}
	if (brief.presentCount > 0) return brief.present[0].crew; // the room: the senior person present
	return -1;
}

// ---- the authored skeleton, which is the floor --------------------------------------------------
//
// For every kind of meeting: the enumerated outcomes, their costs, and minimal dialogue for each --
// enough that the scene plays and the decision is legible with nothing generated at all. The intents
// are the branch descriptors the pills and novelty matching share. The speaker is a role, resolved
// against the room at play time.

static MeetingOption Opt(uint8_t intent, const char *label, const char *cost, uint8_t effect = EFFECT_RECORD,
	int amount = 0, bool on = true)
{
	MeetingOption o;
	o.intent = intent;
	o.label = label;
	o.cost = cost;
	o.effect = effect;
	o.amount = amount;
	o.on = on;
	return o;
}

static void SetAlloc(MeetingOption &o, int slot, SystemId system, int percent)
{
	if (slot < 0 || slot >= MEETING_ALLOC_MAX) return;
	o.alloc[slot].system = static_cast<uint8_t>(system);
	o.alloc[slot].percent = percent;
}

static void AddOutcome(MeetingSkeleton &sk, const MeetingOption &o)
{
	if (sk.outcomeCount >= MEETING_OUTCOME_MAX) return;
	sk.outcomes[sk.outcomeCount].option = o;
	sk.outcomes[sk.outcomeCount].lineCount = 0;
	++sk.outcomeCount;
}

static MeetingOutcome &LastOutcome(MeetingSkeleton &sk)
{
	return sk.outcomes[sk.outcomeCount > 0 ? sk.outcomeCount - 1 : 0];
}

static void AddLine(MeetingSkeleton &sk, int speaker, const char *text, uint8_t delivery)
{
	MeetingOutcome &oc = LastOutcome(sk);
	if (oc.lineCount >= MEETING_LINE_MAX) return;
	MeetingLine &l = oc.lines[oc.lineCount++];
	l.speaker = speaker;
	l.text = text;
	l.delivery = delivery;
}

static void BuildSkeletons(MeetingSkeleton (&sk)[MEET_KIND_COUNT])
{
	{ // the watch change: the normal case, and it always produces a brief
		MeetingSkeleton &m = sk[MEET_WATCH_CHANGE];
		m.kind = MEET_WATCH_CHANGE;
		m.decision = "the watch handover, and what the incoming watch carries";
		AddOutcome(m, Opt(1, "Carry on as briefed", "nothing"));
		AddLine(m, SPEAK_COMMAND, "You have the watch. Carry on.", DELIVERY_ORDER);
		AddLine(m, SPEAK_ENGINEERING, "Engineering holds at the allocation we set.", DELIVERY_REPORT);
		AddOutcome(m, Opt(2, "Watch the reserve", "Engineering's attention this watch"));
		AddLine(m, SPEAK_ENGINEERING, "I will watch the dilithium reserve and report any fall.", DELIVERY_REPORT);
		AddLine(m, SPEAK_COMMAND, "Do that.", DELIVERY_ORDER);
		AddOutcome(m, Opt(3, "Bring her to yellow", "the watch runs hot and the plant draws harder",
			EFFECT_SET_ALERT, ALERT_YELLOW));
		AddLine(m, SPEAK_SECURITY, "Condition yellow, ship-wide.", DELIVERY_REPORT);
		AddLine(m, SPEAK_COMMAND, "Make it so.", DELIVERY_ORDER);
		AddOutcome(m, Opt(4, "Note what the department raised", "nothing yet"));
		AddLine(m, SPEAK_SCIENCES, "The reading is not something I can call yet.", DELIVERY_UNMARKED);
	}
	{ // the ordinary departmental meeting: it resolves nothing and still produces a brief
		MeetingSkeleton &m = sk[MEET_DEPARTMENTAL];
		m.kind = MEET_DEPARTMENTAL;
		m.decision = "the department's business this watch";
		AddOutcome(m, Opt(1, "No change", "nothing"));
		AddLine(m, SPEAK_ENGINEERING, "Nothing here needs the captain. No change.", DELIVERY_REPORT);
		AddOutcome(m, Opt(2, "Take the backlog in hand", "the department's hours this watch"));
		AddLine(m, SPEAK_ENGINEERING, "We will work the backlog and report at the next watch.", DELIVERY_REPORT);
		AddOutcome(m, Opt(3, "Raise it to the staff", "a place on the staff's agenda"));
		AddLine(m, SPEAK_ENGINEERING, "Then I will bring it to the staff myself.", DELIVERY_REPORT);
	}
	{ // the allocation meeting: where the FTL argument and the allocation seam live
		MeetingSkeleton &m = sk[MEET_ALLOCATION];
		m.kind = MEET_ALLOCATION;
		m.decision = "where the ship's power goes";
		AddOutcome(m, Opt(1, "The chief's plan", "adopted as set", EFFECT_ACCEPT_RECOMMENDATION));
		AddLine(m, SPEAK_ENGINEERING, "The plan holds every system the plant can feed.", DELIVERY_REPORT);
		AddLine(m, SPEAK_COMMAND, "Adopted. Set it.", DELIVERY_ORDER);
		AddOutcome(m, Opt(2, "Refuse the chief", "he notes who refused, and the argument stands",
			EFFECT_REFUSE_RECOMMENDATION));
		AddLine(m, SPEAK_ENGINEERING, "Then the plant is short, and it is on the record who chose it.", DELIVERY_REPORT);
		AddLine(m, SPEAK_COMMAND, "Noted.", DELIVERY_FLAT);
		AddOutcome(m, Opt(3, "Run the ship's own ladder", "automatic mode: the ship decides the unset systems",
			EFFECT_SET_POWER_AUTO, 0, true));
		AddLine(m, SPEAK_ENGINEERING, "The computer will keep the cheapest and give up the rest.", DELIVERY_REPORT);
		AddOutcome(m, Opt(4, "The holodecks full, the shields dark", "no shields while the holodecks run",
			EFFECT_SET_ALLOCATION));
		{
			MeetingOption &o = LastOutcome(m).option;
			SetAlloc(o, 0, SYS_HOLODECKS, 100);
			SetAlloc(o, 1, SYS_SHIELDS, 0);
		}
		AddLine(m, SPEAK_ENGINEERING, "That is the choice, then, and it is yours to make.", DELIVERY_REPORT);
		AddLine(m, SPEAK_COMMAND, "Do it.", DELIVERY_ORDER);
	}
	{ // the crystal ages: the dilithium threshold
		MeetingSkeleton &m = sk[MEET_DILITHIUM];
		m.kind = MEET_DILITHIUM;
		m.decision = "what to do as the crystal ages";
		AddOutcome(m, Opt(1, "Recomposite at the next quiet watch", "Engineering's hours and material", EFFECT_RECOMPOSITE));
		AddLine(m, SPEAK_ENGINEERING, "The frame can take another recomposition, if we spend the hours.", DELIVERY_REPORT);
		AddOutcome(m, Opt(2, "Make for the nearest source", "a detour from the route home"));
		AddLine(m, SPEAK_COMMAND, "Then we detour. Set the course.", DELIVERY_ORDER);
		AddOutcome(m, Opt(3, "Run the crystal to the end", "no warp when it fails"));
		AddLine(m, SPEAK_ENGINEERING, "We will get every light year out of it before it goes.", DELIVERY_REPORT);
	}
	{ // more casualties than beds: the triage gap as a decision
		MeetingSkeleton &m = sk[MEET_CASUALTIES];
		m.kind = MEET_CASUALTIES;
		m.decision = "who gets the beds";
		AddOutcome(m, Opt(1, "Treat the worst first", "a senior case may wait", EFFECT_ORDER_TRIAGE, 0));
		AddLine(m, SPEAK_MEDICAL, "Worst first. I will not rank them.", DELIVERY_REPORT);
		AddOutcome(m, Opt(2, "Treat rank first", "a critical junior may be lost", EFFECT_ORDER_TRIAGE, 1));
		AddLine(m, SPEAK_MEDICAL, "Rank first. That is on your head, not mine.", DELIVERY_REPORT);
		AddOutcome(m, Opt(3, "Raise the surgical field", "it holds one case and heals none"));
		AddLine(m, SPEAK_MEDICAL, "The field will hold the gravest, and nothing more than that.", DELIVERY_REPORT);
	}
	{ // Borg pressure rising
		MeetingSkeleton &m = sk[MEET_BORG];
		m.kind = MEET_BORG;
		m.decision = "how to meet the Borg pressure";
		AddOutcome(m, Opt(1, "Send security", "the squad is committed", EFFECT_SECURITY_TO_DECK, 0));
		AddLine(m, SPEAK_SECURITY, "We will hold the deck. Nobody gets behind us.", DELIVERY_ORDER);
		AddOutcome(m, Opt(2, "Evacuate the deck", "the deck's work stops", EFFECT_EVACUATE_DECK, 0));
		AddLine(m, SPEAK_SECURITY, "Evacuate. Nobody stays on that deck.", DELIVERY_ORDER);
		AddOutcome(m, Opt(3, "Withdraw and seal it", "the deck is given up"));
		AddLine(m, SPEAK_COMMAND, "Seal it. We will take it back when we can.", DELIVERY_ORDER);
	}
	{ // a promise has come due: a decision the player has been deferring
		MeetingSkeleton &m = sk[MEET_DEFERRED];
		m.kind = MEET_DEFERRED;
		m.decision = "the promise that has come due";
		AddOutcome(m, Opt(1, "Make it good now", "what it takes, now", EFFECT_PROMISE_KEPT));
		AddLine(m, SPEAK_COMMAND, "It is owed. Do it.", DELIVERY_ORDER);
		AddOutcome(m, Opt(2, "Let it stand", "the promise is broken, and remembered"));
		AddLine(m, SPEAK_COMMAND, "I cannot do it. Note it, and let it stand.", DELIVERY_CONFESSION);
		AddOutcome(m, Opt(3, "Attend to it later", "a deferred decision, and a mark that waits"));
		AddLine(m, SPEAK_COMMAND, "Not yet. It waits.", DELIVERY_FLAT);
	}
}

const MeetingSkeleton &AuthoredSkeleton(uint8_t kind)
{
	static MeetingSkeleton sk[MEET_KIND_COUNT];
	static bool built = false;
	if (!built) { built = true; BuildSkeletons(sk); }
	return sk[kind < MEET_KIND_COUNT ? static_cast<int>(kind) : static_cast<int>(MEET_WATCH_CHANGE)];
}

// ---- the brief generator: a pure read ------------------------------------------------------------

// The log scope a post reads (access and authority: a post reads its own scope, command reads all).
static const char *ScopeForDept(Department d)
{
	switch (d) {
		case DEPT_COMMAND: return "command";
		case DEPT_ENGINEERING: return "engineering";
		case DEPT_SECURITY: return "security";
		case DEPT_SCIENCES: return "outside";
		case DEPT_MEDICAL: return "sickbay";
		default: return "crew";
	}
}

static int WatchToken(const Ship &s) { return s.Day() * WATCHES + s.Watch(); }

static int CasualtyCount(const Ship &s)
{
	int n = 0;
	for (const CrewMember &c : s.crew) if (c.status == CREW_INJURED) ++n;
	return n;
}

// The deck the simulation names when the room does not name one: the most contested, lowest first.
static int MostThreatenedDeck(const Ship &s)
{
	int best = 0, bestN = 0;
	for (int d = 0; d < DECKS; ++d) {
		const int n = static_cast<int>(std::ceil(s.decks[d].intruders));
		if (n > bestN) { bestN = n; best = d + 1; }
	}
	return best;
}

// The oldest open promise whose deadline is within a day (or past): the decision deferred.
static int DuePromise(const Ship &s)
{
	int best = -1;
	double bestMade = 0.0;
	for (int i = 0; i < static_cast<int>(s.promises.size()); ++i) {
		const Promise &p = s.promises[i];
		if (p.state != PROMISE_OPEN || p.deadline < 0.0) continue;
		if (s.clock < p.deadline - SECONDS_PER_DAY) continue;
		if (best < 0 || p.made < bestMade) { best = i; bestMade = p.made; }
	}
	return best;
}

// A stable digest of the facts a brief was built from, so a stale script can be detected.
static uint32_t StateDigest(const Ship &s)
{
	uint32_t h = 2166136261u;
	auto mix = [&h](uint32_t v) { h ^= v; h *= 16777619u; };
	mix(static_cast<uint32_t>(std::llround(s.clock)));
	mix(s.alert);
	mix(static_cast<uint32_t>(PowerCommitted(s)));
	mix(static_cast<uint32_t>(s.PowerAvailable()));
	mix(static_cast<uint32_t>(std::llround(s.dilithium * 1000.0f)));
	mix(static_cast<uint32_t>(CasualtyCount(s)));
	mix(static_cast<uint32_t>(Intruders(s)));
	mix(static_cast<uint32_t>(std::llround(s.borgAwareness * 1000.0f)));
	for (int i = 0; i < SYS_COUNT; ++i) {
		mix(static_cast<uint32_t>(std::llround(s.systems[i].health * 1000.0f)));
		mix(static_cast<uint32_t>(s.systems[i].allocated));
	}
	return h;
}

static std::vector<LogEntry> LogForParticipant(const Ship &s, int crew)
{
	if (crew < 0 || crew >= static_cast<int>(s.crew.size())) return {};
	const CrewMember &c = s.crew[crew];
	if (MayCommand(c)) return ReadOfficialLog(s, MEETING_VIEW_MAX, std::string());
	return ReadOfficialLog(s, MEETING_VIEW_MAX, std::string(ScopeForDept(c.dept)));
}

static bool AddParticipant(MeetingBrief &b, const Ship &s, int crew)
{
	if (crew < 0 || crew >= static_cast<int>(s.crew.size())) return false;
	const CrewMember &c = s.crew[crew];
	if (c.status == CREW_DEAD || c.status == CREW_ASSIMILATED) return false;
	for (int i = 0; i < b.presentCount; ++i) if (b.present[i].crew == crew) return false;
	if (b.presentCount >= MEETING_PARTICIPANT_MAX) return false;
	BriefParticipant &p = b.present[b.presentCount++];
	p.crew = crew;
	p.name = c.name;
	p.post = c.post;
	p.watch = c.watch;
	p.mood = Morale(c);
	// What this person knows: their marks, most salient first.
	int idx[MEMORY_MAX];
	int n = 0;
	for (int i = 0; i < static_cast<int>(c.memories.size()) && n < MEMORY_MAX; ++i) idx[n++] = i;
	std::stable_sort(idx, idx + n, [&c](int a, int bb) { return c.memories[a].salience > c.memories[bb].salience; });
	for (int i = 0; i < n && p.markCount < MEETING_VIEW_MAX; ++i) {
		const Memory &m = c.memories[idx[i]];
		BriefMark bm;
		bm.event = m.event; bm.person = m.person; bm.source = m.source;
		bm.valence = m.valence; bm.salience = m.salience;
		p.marks[p.markCount++] = bm;
	}
	// The open claims they carry.
	for (const Promise &pr : s.promises) {
		if (pr.state != PROMISE_OPEN) continue;
		if (pr.promiser != crew && pr.beneficiary != crew) continue;
		if (p.promiseCount >= MEETING_VIEW_MAX) break;
		BriefPromise &bp = p.promises[p.promiseCount++];
		bp.kind = pr.kind; bp.promiser = pr.promiser; bp.beneficiary = pr.beneficiary;
		bp.what = pr.what; bp.deadline = pr.deadline;
	}
	// The log they can read.
	for (const LogEntry &e : LogForParticipant(s, crew)) {
		if (p.logCount >= MEETING_VIEW_MAX) break;
		p.log[p.logCount++] = e;
	}
	return true;
}

static void ParticipantsFor(const Ship &s, uint8_t kind, MeetingBrief &b)
{
	std::vector<int> ids;
	auto add = [&ids](int id) { if (id >= 0) ids.push_back(id); };
	if (s.player >= 0) add(s.player); // the player is in the room
	for (int i = 0; i < static_cast<int>(s.crew.size()); ++i)
		if (s.crew[i].status == CREW_FIT && s.crew[i].rank >= 5) { add(i); break; } // whoever commands
	switch (kind) {
		case MEET_ALLOCATION:
			add(DepartmentHead(s, DEPT_ENGINEERING));
			break;
		case MEET_DILITHIUM:
			add(DepartmentHead(s, DEPT_ENGINEERING));
			add(DepartmentHead(s, DEPT_SCIENCES));
			break;
		case MEET_CASUALTIES:
			add(DepartmentHead(s, DEPT_MEDICAL));
			break;
		case MEET_BORG:
			add(DepartmentHead(s, DEPT_SECURITY));
			add(DepartmentHead(s, DEPT_ENGINEERING));
			break;
		case MEET_DEFERRED:
			for (const Promise &p : s.promises) if (p.state == PROMISE_OPEN) { add(p.promiser); add(p.beneficiary); }
			break;
		case MEET_DEPARTMENTAL: {
			int best = -1, bestN = -1;
			for (int d = 0; d < DEPT_COUNT; ++d) {
				int cnt = 0;
				for (const CrewMember &c : s.crew)
					if (c.status == CREW_FIT && !c.brigged && c.dept == d && c.activity == ACT_ON_DUTY) ++cnt;
				if (cnt > bestN) { bestN = cnt; best = d; }
			}
			for (int i = 0; i < static_cast<int>(s.crew.size()); ++i) {
				const CrewMember &c = s.crew[i];
				if (c.status == CREW_FIT && !c.brigged && c.dept == best) add(i);
			}
			break;
		}
		case MEET_WATCH_CHANGE:
		default:
			for (int d = 0; d < DEPT_COUNT; ++d) add(DepartmentHead(s, static_cast<Department>(d)));
			break;
	}
	for (int id : ids) {
		if (b.presentCount >= MEETING_PARTICIPANT_MAX) break;
		AddParticipant(b, s, id);
	}
}

static std::string TriggerFor(const Ship &s, uint8_t kind)
{
	char buf[160];
	switch (kind) {
		case MEET_WATCH_CHANGE:
			std::snprintf(buf, sizeof(buf), "the watch changes: day %d, %s watch", s.Day(), s.Watch() == 0 ? "alpha" : s.Watch() == 1 ? "beta" : "gamma");
			return buf;
		case MEET_DEPARTMENTAL:
			std::snprintf(buf, sizeof(buf), "the day's departmental meeting (day %d)", s.Day());
			return buf;
		case MEET_ALLOCATION:
			std::snprintf(buf, sizeof(buf), "the commitments exceed the plant by %d EPS", PowerShortfall(s));
			return buf;
		case MEET_DILITHIUM:
			std::snprintf(buf, sizeof(buf), "the crystal is at %d%%", static_cast<int>(s.dilithium * 100.0f + 0.5f));
			return buf;
		case MEET_CASUALTIES:
			std::snprintf(buf, sizeof(buf), "%d casualties for %d beds", CasualtyCount(s), SICKBAY_BEDS);
			return buf;
		case MEET_BORG:
			std::snprintf(buf, sizeof(buf), "%d intruders aboard, Borg awareness %d%%", Intruders(s), static_cast<int>(s.borgAwareness * 100.0f + 0.5f));
			return buf;
		case MEET_DEFERRED: {
			const int i = DuePromise(s);
			if (i >= 0 && i < static_cast<int>(s.promises.size())) {
				const Promise &p = s.promises[i];
				const std::string who = (p.beneficiary >= 0 && p.beneficiary < static_cast<int>(s.crew.size())) ? s.crew[p.beneficiary].name : "a crew member";
				return "a promise to " + who + " has come due";
			}
			return "a decision has been deferred";
		}
		default:
			return "a meeting is due";
	}
}

MeetingBrief BuildBrief(const Ship &s, uint8_t kind)
{
	MeetingBrief b;
	if (kind >= MEET_KIND_COUNT) kind = MEET_WATCH_CHANGE;
	b.kind = kind;
	b.time = s.clock;
	const MeetingSkeleton &sk = AuthoredSkeleton(kind);
	b.decision = sk.decision;
	b.optionCount = std::min(sk.outcomeCount, MEETING_OPTION_MAX);
	for (int i = 0; i < b.optionCount; ++i) b.options[i] = sk.outcomes[i].option;
	ParticipantsFor(s, kind, b);
	b.powerCommitted = PowerCommitted(s);
	b.powerAvailable = s.PowerAvailable();
	b.powerShortfall = PowerShortfall(s);
	b.alert = s.alert;
	b.dilithium = s.dilithium;
	b.casualties = CasualtyCount(s);
	b.beds = SICKBAY_BEDS;
	b.borgPressure = Intruders(s) > 0 || s.borgAwareness > 0.5f;
	for (int i = 0; i < SYS_COUNT; ++i) {
		b.systems[i].allocated = s.systems[i].allocated;
		b.systems[i].output = s.systems[i].output;
		b.systems[i].allocBy = AllocationSource(s, static_cast<SystemId>(i));
		b.systems[i].online = s.systems[i].enabled;
	}
	b.trigger = TriggerFor(s, kind);
	b.stateDigest = StateDigest(s);
	return b;
}

// The raw threshold, without the latch: whether a fact still calls for this meeting. Used to clear a
// latch when its episode passes, so a later episode can call a meeting of its own.
static bool ThresholdHolds(const Ship &s, uint8_t kind)
{
	switch (kind) {
		case MEET_ALLOCATION: return PowerShortfall(s) > 0;
		case MEET_DILITHIUM: return s.dilithium < 0.25f;
		case MEET_CASUALTIES: return CasualtyCount(s) > SICKBAY_BEDS;
		case MEET_BORG: return Intruders(s) > 0 || s.borgAwareness > 0.5f;
		case MEET_DEFERRED: return DuePromise(s) >= 0;
		default: return false;
	}
}

bool MeetingDue(const Ship &s, uint8_t kind)
{
	if (kind >= MEET_KIND_COUNT) return false;
	const uint16_t bit = static_cast<uint16_t>(1u << kind);
	switch (kind) {
		case MEET_WATCH_CHANGE: return WatchToken(s) != s.lastWatchMeeting;
		case MEET_DEPARTMENTAL: return s.Day() != s.lastDeptMeetingDay;
		default: return !(s.meetingLatches & bit) && ThresholdHolds(s, kind);
	}
}

// THE EMIT SITE. A brief is enqueued here and nowhere else. Called by the simulation's own clock, so
// every meeting -- the ordinary ones included -- generates a brief. Grep this function, not a count.
int EmitDueMeetings(Ship &s)
{
	// A threshold episode that has passed releases its latch, so the next one can call a meeting.
	for (int k = MEET_ALLOCATION; k < MEET_KIND_COUNT; ++k) {
		const uint16_t bit = static_cast<uint16_t>(1u << k);
		if ((s.meetingLatches & bit) && !ThresholdHolds(s, static_cast<uint8_t>(k)))
			s.meetingLatches = static_cast<uint16_t>(s.meetingLatches & ~bit);
	}
	int emitted = 0;
	for (int k = 0; k < MEET_KIND_COUNT; ++k) {
		if (!MeetingDue(s, static_cast<uint8_t>(k))) continue;
		// One at a time: the async worker drains the queue; a full queue waits for it.
		if (static_cast<int>(s.pendingMeetings.size()) >= MEETING_QUEUE_MAX) break;
		MeetingBrief b = BuildBrief(s, static_cast<uint8_t>(k));
		s.pendingMeetings.push_back(b);
		s.meetingLatches = static_cast<uint16_t>(s.meetingLatches | (1u << k));
		if (k == MEET_WATCH_CHANGE) s.lastWatchMeeting = WatchToken(s);
		if (k == MEET_DEPARTMENTAL) s.lastDeptMeetingDay = s.Day();
		LogEvent(s, "the bridge", "command",
			std::string("a ") + MeetingKindName(static_cast<uint8_t>(k)) + " meeting is called: " + b.decision);
		++emitted;
	}
	if (static_cast<int>(s.pendingMeetings.size()) > MEETING_QUEUE_MAX)
		s.pendingMeetings.resize(MEETING_QUEUE_MAX);
	return emitted;
}

const std::vector<MeetingBrief> &PendingMeetings(const Ship &s) { return s.pendingMeetings; }

bool TakeBrief(Ship &s)
{
	if (s.pendingMeetings.empty()) return false;
	s.pendingMeetings.erase(s.pendingMeetings.begin());
	return true;
}

// The simulation applies a chosen outcome. A person in the room decided it (decidedBy), or the ship
// did (automatic). The model never applies anything.
bool ApplyMeetingOutcome(Ship &s, const MeetingBrief &brief, int outcome, int decidedBy, bool automatic)
{
	const MeetingSkeleton &sk = AuthoredSkeleton(brief.kind);
	if (outcome < 0 || outcome >= sk.outcomeCount) return false;
	const MeetingOption &o = sk.outcomes[outcome].option;
	const std::string who = (decidedBy >= 0 && decidedBy < static_cast<int>(s.crew.size()))
		? s.crew[decidedBy].name : CommandingOfficer(s);
	bool ok = false;
	switch (o.effect) {
		case EFFECT_RECORD:
			ok = true; // minuted; nothing in the ship changes
			break;
		case EFFECT_SET_ALLOCATION: {
			// The ruling (docs/power-assignment.md): the player and the crew decide. The meeting cannot
			// set an allocation as the ship -- that is automatic mode -- and an officer without the
			// authority to decide the band cannot set one either.
			if (automatic) {
				LogEvent(s, who, "command",
					"the meeting will not set an allocation as the ship; a person must decide it, or automatic mode must be granted");
				return false;
			}
			if (decidedBy < 0 || decidedBy >= static_cast<int>(s.crew.size())) return false;
			const bool isPlayer = decidedBy == s.player || s.player < 0;
			// Decreases first, so an increase is measured against the power they free.
			for (int pass = 0; pass < 2; ++pass) {
				for (int i = 0; i < MEETING_ALLOC_MAX; ++i) {
					const MeetingAlloc &a = o.alloc[i];
					if (a.system >= SYS_COUNT || a.percent < 0) continue;
					const int cur = AllocationPercent(s, static_cast<SystemId>(a.system));
					if ((pass == 0) != (a.percent <= cur)) continue;
					const bool done = isPlayer
						? SetAllocation(s, static_cast<SystemId>(a.system), a.percent)
						: SetAllocationBy(s, static_cast<SystemId>(a.system), a.percent, decidedBy);
					ok = ok || done;
				}
			}
			break;
		}
		case EFFECT_SET_POWER_AUTO:
			SetPowerAuto(s, o.on);
			ok = true;
			break;
		case EFFECT_ACCEPT_RECOMMENDATION:
			ok = AcceptRecommendation(s);
			break;
		case EFFECT_REFUSE_RECOMMENDATION:
			ok = RefuseRecommendation(s);
			break;
		case EFFECT_ORDER_TRIAGE:
			ok = OrderTriage(s, o.amount);
			break;
		case EFFECT_SECURITY_TO_DECK:
			ok = OrderSecurityTo(s, o.amount > 0 ? o.amount : MostThreatenedDeck(s));
			break;
		case EFFECT_EVACUATE_DECK:
			ok = OrderEvacuate(s, o.amount > 0 ? o.amount : MostThreatenedDeck(s));
			break;
		case EFFECT_SET_ALERT:
			if (o.amount >= 0 && o.amount < 3) { SetAlert(s, static_cast<Alert>(o.amount)); ok = true; }
			break;
		case EFFECT_RECOMPOSITE:
			ok = Recomposite(s);
			break;
		case EFFECT_PROMISE_KEPT: {
			const int i = DuePromise(s);
			if (i >= 0) ok = ResolvePromise(s, i, true);
			break;
		}
		default:
			ok = false;
			break;
	}
	LogEvent(s, who, "command", std::string("the meeting decided: ") + o.label);
	Tick(s, 0.0f); // the console reads the result immediately
	return ok;
}

// ---- the audio plumbing: three sources, one owner each (docs/staff-meetings.md) ----------------
//
// ONE PRODUCER PER TRACK, ONE RETIREMENT RULE PER REPLY. The ownership is a function (TrackOwner),
// and every write and every retirement goes through the checks below, so a non-owner is refused by
// construction rather than by convention. The cue track is not the dialogue track: EmitCue never
// touches a line, and VoiceRetire(PROD_CUE_PLAYER, <a line>) is refused.
//
// No sound is owned here and no device is opened. This is the model phase two builds and phase three
// drives.

const char *VoiceTrackName(uint8_t track)
{
	static const char *const NAMES[TRACK_COUNT] = { "dialogue", "cue", "live" };
	return track < TRACK_COUNT ? NAMES[track] : "?";
}

const char *VoiceProducerName(uint8_t producer)
{
	static const char *const NAMES[PROD_COUNT] = { "the renderer", "the cue player", "the live line" };
	return producer < PROD_COUNT ? NAMES[producer] : "?";
}

uint8_t TrackOwner(uint8_t track)
{
	switch (track) {
		case TRACK_DIALOGUE: return PROD_RENDERER;
		case TRACK_CUE: return PROD_CUE_PLAYER;
		case TRACK_LIVE: return PROD_LIVE;
		default: return PROD_COUNT;
	}
}

bool OwnsTrack(uint8_t producer, uint8_t track) { return producer == TrackOwner(track); }

int VoiceWrite(VoiceMixer &m, uint8_t producer, uint8_t track, const std::string &asset, int cue)
{
	if (track >= TRACK_COUNT || !OwnsTrack(producer, track)) return -1; // not yours to write
	VoiceTrackState &ts = m.tracks[track];
	if (ts.replyCount >= VOICE_REPLY_MAX) return -1; // one player per source: the track is busy
	VoiceReply &r = ts.replies[ts.replyCount++];
	r.id = m.nextId++;
	r.track = track;
	r.asset = asset;
	r.cue = cue;
	return r.id;
}

bool VoiceRetire(VoiceMixer &m, uint8_t producer, int replyId)
{
	for (int t = 0; t < TRACK_COUNT; ++t) {
		VoiceTrackState &ts = m.tracks[t];
		for (int i = 0; i < ts.replyCount; ++i) {
			if (ts.replies[i].id != replyId) continue;
			if (!OwnsTrack(producer, ts.replies[i].track)) return false; // not yours to retire
			for (int j = i; j + 1 < ts.replyCount; ++j) ts.replies[j] = ts.replies[j + 1];
			--ts.replyCount;
			++ts.retires;
			return true;
		}
	}
	return false;
}

bool TrackBusy(const VoiceMixer &m, uint8_t track)
{
	return track < TRACK_COUNT && m.tracks[track].replyCount > 0;
}

const VoiceReply *ActiveReply(const VoiceMixer &m, uint8_t track)
{
	if (track >= TRACK_COUNT || m.tracks[track].replyCount == 0) return nullptr;
	return &m.tracks[track].replies[0];
}

const char *CueName(uint8_t cue)
{
	static const char *const NAMES[CUE_COUNT] = { "breath", "chair", "PADD tap", "Hmm.", "I have to think about that." };
	return cue < CUE_COUNT ? NAMES[cue] : "?";
}

const char *CueClip(uint8_t cue)
{
	static const char *const CLIPS[CUE_COUNT] = {
		"cue/breath.wav", "cue/chair.wav", "cue/padd_tap.wav", "cue/hmm.wav", "cue/holding.wav"
	};
	return cue < CUE_COUNT ? CLIPS[cue] : "";
}

const char *CuePurpose(uint8_t cue)
{
	static const char *const PURPOSES[CUE_COUNT] = {
		"the pause is a person, not a machine",
		"someone changes posture: attention, or discomfort",
		"the room thinking, a beat filled by a hand",
		"acknowledgement while the answer is late",
		"the short holding line the design names, before a deferral"
	};
	return cue < CUE_COUNT ? PURPOSES[cue] : "";
}

bool CueIsLexical(uint8_t cue) { return cue == CUE_HOLDING; }

int EmitCue(Ship &s, VoiceMixer &m, uint8_t cue, const std::string &reason)
{
	if (cue >= CUE_COUNT) return -1;
	const int id = VoiceWrite(m, PROD_CUE_PLAYER, TRACK_CUE, CueClip(cue), cue);
	if (id < 0) return -1; // the cue track is busy: nothing is written, and nothing else plays
	LogEvent(s, "the room", "meeting", std::string("cue: ") + CueName(cue) + " -- " + reason);
	return id;
}

std::string RenderKey(const std::string &voice, const std::string &text, uint8_t delivery)
{
	uint64_t h = 1469598103934665603ULL; // FNV-1a, 64-bit
	const uint8_t *vb = reinterpret_cast<const uint8_t *>(voice.data());
	const uint8_t *tb = reinterpret_cast<const uint8_t *>(text.data());
	const uint32_t vl = static_cast<uint32_t>(voice.size());
	const uint32_t tl = static_cast<uint32_t>(text.size());
	// Length prefixes first, so "ab"+"c" and "a"+"bc" cannot collide through the concatenation.
	for (int i = 0; i < 4; ++i) { h ^= (vl >> (8 * i)) & 0xFF; h *= 1099511628211ULL; }
	for (size_t i = 0; i < voice.size(); ++i) { h ^= vb[i]; h *= 1099511628211ULL; }
	for (int i = 0; i < 4; ++i) { h ^= (tl >> (8 * i)) & 0xFF; h *= 1099511628211ULL; }
	for (size_t i = 0; i < text.size(); ++i) { h ^= tb[i]; h *= 1099511628211ULL; }
	h ^= delivery; h *= 1099511628211ULL;
	char buf[17];
	std::snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(h));
	return std::string(buf);
}

std::string VoiceCachePath(const VoiceRender &vr, const std::string &key)
{
	return vr.dir.empty() ? key + ".wav" : vr.dir + "/" + key + ".wav";
}

bool VoiceCached(const VoiceRender &vr, const std::string &key)
{
	for (const VoiceCacheEntry &e : vr.cache) if (e.key == key) return true;
	return false;
}

bool VoiceQueued(const VoiceRender &vr, const std::string &key)
{
	for (const RenderJob &j : vr.queue) if (j.key == key) return true;
	return false;
}

int QueueRender(VoiceRender &vr, const RenderJob &job)
{
	if (!DeliveryKnown(job.delivery)) return -1; // UNMARKED: marked, not guessed, and not rendered
	if (VoiceCached(vr, job.key) || VoiceQueued(vr, job.key)) return 0; // a re-render is a no-op
	if (static_cast<int>(vr.queue.size()) >= VOICE_QUEUE_MAX) return -2;
	vr.queue.push_back(job);
	return 1;
}

bool CacheRendered(VoiceRender &vr, const std::string &key, const std::string &file)
{
	bool queued = false;
	for (size_t i = 0; i < vr.queue.size(); ++i)
		if (vr.queue[i].key == key) { vr.queue.erase(vr.queue.begin() + i); queued = true; break; }
	if (!queued) return false; // a render nobody asked for does not enter the cache
	if (VoiceCached(vr, key)) return true;
	if (static_cast<int>(vr.cache.size()) >= VOICE_CACHE_MAX) vr.cache.erase(vr.cache.begin());
	VoiceCacheEntry e;
	e.key = key;
	e.file = file.empty() ? VoiceCachePath(vr, key) : file;
	vr.cache.push_back(e);
	return true;
}

int PruneVoiceCache(VoiceRender &vr)
{
	const int n = static_cast<int>(vr.cache.size());
	vr.cache.clear(); // the host deletes the directory: nothing survives a prune
	return n;
}

// The name a line's role resolves to when no one in the room fills it. For the renderer's benefit,
// not the player's: it is the casting identity, and phase three casts our own crew.
static const char *SpeakerRoleName(int speaker)
{
	switch (speaker) {
		case SPEAK_COMMAND: return "the commanding officer";
		case SPEAK_ENGINEERING: return "the chief engineer";
		case SPEAK_SECURITY: return "the security chief";
		case SPEAK_SCIENCES: return "the science officer";
		case SPEAK_MEDICAL: return "the chief medical officer";
		default: return "the room";
	}
}

int PlanMeetingAudio(VoiceRender &vr, const Ship &s, const MeetingBrief &brief, const MeetingSkeleton &sk)
{
	int added = 0;
	for (int i = 0; i < sk.outcomeCount; ++i) {
		const MeetingOutcome &oc = sk.outcomes[i];
		for (int l = 0; l < oc.lineCount; ++l) {
			const MeetingLine &line = oc.lines[l];
			SynthesisRequest req;
			if (!LineToSynthesis(line, req)) continue; // the unmarked line is marked, not rendered
			const int who = ResolveSpeaker(s, brief, line.speaker);
			RenderJob job;
			job.voice = (who >= 0 && who < static_cast<int>(s.crew.size())) ? s.crew[who].name
				: SpeakerRoleName(line.speaker);
			job.text = req.text;
			job.delivery = req.delivery;
			job.exaggeration = req.exaggeration;
			job.speaker = line.speaker;
			job.key = RenderKey(job.voice, job.text, job.delivery);
			if (QueueRender(vr, job) == 1) ++added;
		}
	}
	return added;
}

bool WarmVoice(Ship &s, VoiceRender &vr, double now)
{
	if (vr.warm) return false;
	vr.warm = true;
	vr.warmedAt = now;
	// Where the warm happens is the async phase, when the worker opens -- off the critical path, so
	// the first meeting anyone sees is never the cold one (lesson 4).
	LogEvent(s, "the computer", "meeting", "voice: the model is warmed in the async window, before the meeting plays");
	return true;
}

// ---- persistence ------------------------------------------------------------------------------

namespace {

struct Writer {
	std::vector<uint8_t> b;
	void U8(uint8_t v) { b.push_back(v); }
	void U16(uint16_t v) { U8(static_cast<uint8_t>(v)); U8(static_cast<uint8_t>(v >> 8)); }
	void U32(uint32_t v) { U16(static_cast<uint16_t>(v)); U16(static_cast<uint16_t>(v >> 16)); }
	void U64(uint64_t v) { U32(static_cast<uint32_t>(v)); U32(static_cast<uint32_t>(v >> 32)); }
	void F(float v) { uint32_t u; std::memcpy(&u, &v, 4); U32(u); }
};

struct Reader {
	const uint8_t *p;
	size_t left;
	bool ok = true;
	uint8_t U8() { if (left < 1) { ok = false; return 0; } --left; return *p++; }
	uint16_t U16() { const uint16_t lo = U8(); return static_cast<uint16_t>(lo | (U8() << 8)); }
	uint32_t U32() { const uint32_t lo = U16(); return lo | (static_cast<uint32_t>(U16()) << 16); }
	uint64_t U64() { const uint64_t lo = U32(); return lo | (static_cast<uint64_t>(U32()) << 32); }
	float F() { const uint32_t u = U32(); float v; std::memcpy(&v, &u, 4); return v; }
	float Unit() { const float v = F(); if (!(v >= 0.0f && v <= 1.0f)) ok = false; return v; }
};

// A length-capped string, so one long fact or claim cannot bloat the save.
void WriteStr(Writer &w, const std::string &s)
{
	const int n = std::min(static_cast<int>(s.size()), 63);
	w.U8(static_cast<uint8_t>(n));
	for (int i = 0; i < n; ++i) w.U8(static_cast<uint8_t>(s[i]));
}

std::string ReadStr(Reader &r)
{
	std::string s;
	const int n = r.U8();
	for (int i = 0; i < n && r.ok; ++i) s.push_back(static_cast<char>(r.U8()));
	return s;
}

// The personal log carries a person's own words, so its `what` is allowed to be longer than the
// official entry's 63: up to the entry's own cap, stated in docs/lore-ledger.md. The length is still
// bounded, so one long private entry cannot bloat the save.
void WriteStrCap(Writer &w, const std::string &s, int cap)
{
	const int n = std::min(static_cast<int>(s.size()), cap);
	w.U8(static_cast<uint8_t>(n));
	for (int i = 0; i < n; ++i) w.U8(static_cast<uint8_t>(s[i]));
}

std::string ReadStrCap(Reader &r, int cap)
{
	std::string s;
	const int n = r.U8();
	if (n > cap) { r.ok = false; return s; }
	for (int i = 0; i < n && r.ok; ++i) s.push_back(static_cast<char>(r.U8()));
	return s;
}

} // namespace

std::vector<uint8_t> Pack(const Ship &s)
{
	Writer w;
	w.U32(SAVE_MAGIC);
	w.U16(SAVE_VERSION);
	w.U16(static_cast<uint16_t>(s.crew.size()));
	w.U32(s.cfg.seed);
	w.F(s.cfg.dayScale);
	w.U8(s.cfg.mode); w.U8(s.cfg.clockMode); w.U8(s.cfg.role);
	w.U16(static_cast<uint16_t>(s.player));
	w.U8(static_cast<uint8_t>(s.orderRepairFirst + 1)); w.U8(static_cast<uint8_t>(s.orderSecurityTo)); w.U8(static_cast<uint8_t>(s.orderEvacuate)); w.U8(static_cast<uint8_t>(s.orderTriage));
	w.U64(s.wallSeconds);
	// A created character's name and rank are not in the seed.
	const bool custom = s.player >= 0 && s.player < static_cast<int>(s.crew.size());
	const std::string name = custom ? s.crew[s.player].name : std::string();
	w.U8(static_cast<uint8_t>(name.size()));
	for (char ch : name) w.U8(static_cast<uint8_t>(ch));
	w.U8(custom ? s.crew[s.player].rank : 0);
	w.U64(static_cast<uint64_t>(std::llround(s.clock * 1000.0)));
	w.U8(s.alert);
	for (const System &sys : s.systems) { w.F(sys.health); w.U8(sys.enabled); w.U16(static_cast<uint16_t>(sys.priority)); w.F(sys.control); }
	for (const Source &src : s.sources) { w.F(src.health); w.U8(src.online); }
	for (const Deck &d : s.decks) { w.F(d.atmosphere); w.F(d.gravity); w.F(d.hull); w.F(d.intruders); w.U8(d.borg); w.F(d.assimilated); w.F(d.forceFieldLevel); w.F(d.fire); w.U8(d.boarderKind); w.U8(static_cast<uint8_t>(d.objective)); w.U8(d.compromised ? 1 : 0); w.F(d.dwell); w.U8(d.engaged ? 1 : 0); }
	w.F(s.stores.deuterium); w.F(s.stores.antimatter); w.F(s.stores.batteries);
	w.U16(static_cast<uint16_t>(s.stores.torpedoes));
	w.F(s.stores.spareParts); w.F(s.stores.medicalSupplies); w.F(s.stores.rations); w.F(s.stores.materials); w.U8(static_cast<uint8_t>(s.stores.probes)); w.U8(static_cast<uint8_t>(s.stores.tricorders)); w.U8(static_cast<uint8_t>(s.stores.phasers)); w.U8(static_cast<uint8_t>(s.stores.evSuits)); w.F(s.stores.tricorderCharge); w.F(s.stores.kitCondition); w.U8(PhaserYieldOf(s));
	w.F(s.dilithium); w.F(s.crystalCeiling); w.F(s.crystalQuality); w.U16(static_cast<uint16_t>(s.crystalReplacements));
	// The shuttles: where each is, and what an away one carried (docs/shuttles.md).
	w.U8(static_cast<uint8_t>(s.shuttles.size()));
	for (const Shuttle &sh : s.shuttles) {
		w.U8(static_cast<uint8_t>(sh.cls)); w.U8(static_cast<uint8_t>(sh.location)); w.F(sh.condition); w.U8(static_cast<uint8_t>(sh.comms));
		w.U16(static_cast<uint16_t>(sh.awayBeacon + 1)); w.F(sh.awaySince); w.U8(static_cast<uint8_t>(sh.returnCondition));
		w.U8(static_cast<uint8_t>(sh.name.size())); for (char ch : sh.name) w.U8(static_cast<uint8_t>(ch));
		w.U8(static_cast<uint8_t>(sh.manifest.size())); for (int16_t m : sh.manifest) w.U16(static_cast<uint16_t>(m));
		w.F(sh.cargoMaterial); w.F(sh.cargoRations); w.F(sh.cargoParts);
	}
	// The sector's shape comes back from the seed; where the ship is in it, and what it has met, is stored.
	w.F(s.shieldStrength);
	w.U8(static_cast<uint8_t>(s.beacon));
	w.U32(s.hits);
	w.U16(static_cast<uint16_t>(s.cleanIntercepts));
	w.F(s.remodulateCooldown); w.F(s.adaptationSuppressed);
	uint32_t visited = 0, surveyed = 0, looted = 0;
	for (size_t i = 0; i < s.sector.size() && i < 32; ++i) {
		if (s.sector[i].visited) visited |= 1u << i;
		if (s.sector[i].surveyed) surveyed |= 1u << i;
		if (s.sector[i].looted) looted |= 1u << i;
	}
	w.U32(visited);
	w.U32(surveyed);
	w.U32(looted);
	// A phenomenon's revealed attributes (its presence and truth come from the seed).
	for (size_t i = 0; i < s.sector.size() && i < 32; ++i) w.U8(s.sector[i].phenomAttrs);
	w.U8(static_cast<uint8_t>(s.awayBeacon + 1));
	w.U8(static_cast<uint8_t>(s.course + 1));
	w.U8(s.surgicalForceField ? 1 : 0);
	w.U8(s.emhActive ? 1 : 0);
	w.U8(s.enemyHeld ? 1 : 0);
	w.U8(s.enemy.present); w.U8(s.enemy.borg); w.U8(s.enemy.kind); w.F(s.enemy.hull); w.F(s.enemy.shields);
	w.F(s.enemy.weapons); w.F(s.enemy.engines); w.F(s.enemy.shieldGen); w.F(s.enemy.firepower);
	w.F(s.enemy.adaptation);
	w.U8(static_cast<uint8_t>(s.enemy.boarders));
	w.U8(s.contact2.present); w.U8(s.contact2.borg); w.U8(s.contact2.kind); w.F(s.contact2.hull); w.F(s.contact2.shields);
	w.F(s.contact2.weapons); w.F(s.contact2.engines); w.F(s.contact2.shieldGen); w.F(s.contact2.firepower);
	w.F(s.contact2.adaptation); w.U8(static_cast<uint8_t>(s.contact2.boarders));
	w.U8(s.target);
	w.U8(s.pursued ? 1 : 0); w.U8(static_cast<uint8_t>(s.pursuitJumps)); w.F(s.pursuitStrength);
	w.U8(static_cast<uint8_t>(s.sectorNumber)); w.U8(s.reachedEnd ? 1 : 0); w.U8(s.won ? 1 : 0);
	w.U16(static_cast<uint16_t>(s.refugees));
	w.F(s.pylonHealth); w.U8(s.mobileEmitter ? 1 : 0); w.F(s.resentment); w.U8(s.airponics ? 1 : 0); w.F(s.borgAwareness);
	// Names, types, departments and stations come back from the seed; only what changes is stored.
	// A transporter copy is the exception: it is a new record beyond the complement, so its name and
	// type have no seed to come back from and are stored.
	for (size_t i = 0; i < s.crew.size(); ++i) {
		const CrewMember &c = s.crew[i];
		if (i >= static_cast<size_t>(COMPLEMENT)) {
			w.U8(static_cast<uint8_t>(std::min<int>(static_cast<int>(c.name.size()), 31)));
			for (size_t k = 0; k < c.name.size() && k < 31; ++k) w.U8(static_cast<uint8_t>(c.name[k]));
			w.U8(static_cast<uint8_t>(std::min<int>(static_cast<int>(c.type.size()), 31)));
			for (size_t k = 0; k < c.type.size() && k < 31; ++k) w.U8(static_cast<uint8_t>(c.type[k]));
		}
		w.U8(c.status); w.F(c.fatigue); w.U8(c.watch); w.U8(c.post); w.F(c.exposure); w.F(c.recovery); w.F(c.wounds); w.F(c.assimScar); w.F(c.severity);
		w.U8(c.away ? 1 : 0); w.U8(c.credentials); w.U8(c.faction); w.U8(c.brigged ? 1 : 0); w.F(c.quartersQuality); w.F(c.holoCompulsion); w.U8(c.quartersSealed ? 1 : 0);
		const int mem = std::min(static_cast<int>(c.memories.size()), MEMORY_MAX);
		w.U8(static_cast<uint8_t>(mem));
		for (int k = 0; k < mem; ++k) {
			const Memory &m = c.memories[k];
			w.U16(m.event); w.U16(static_cast<uint16_t>(m.person + 1)); w.U8(m.source);
			w.F(m.time); w.F(m.valence); w.F(m.salience); w.U8(m.orphaned ? 1 : 0);
		}
	}
	// The log: bounded, and each string length-capped so one long fact cannot bloat the save.
	{
		const int n = std::min(static_cast<int>(s.log.size()), LOG_MAX);
		w.U16(static_cast<uint16_t>(n));
		for (int i = 0; i < n; ++i) {
			const LogEntry &e = s.log[s.log.size() - n + i];
			w.U64(static_cast<uint64_t>(std::llround(e.time * 1000.0)));
			for (const std::string *str : { &e.who, &e.scope, &e.what }) {
				const int m = std::min(static_cast<int>(str->size()), 63);
				w.U8(static_cast<uint8_t>(m));
				for (int k = 0; k < m; ++k) w.U8(static_cast<uint8_t>((*str)[k]));
			}
		}
	}
	// The job queue: bounded, each entry a kind, a target, how far and its place.
	{
		const int n = std::min(static_cast<int>(s.jobs.size()), JOB_MAX);
		w.U16(static_cast<uint16_t>(n));
		for (int i = 0; i < n; ++i) {
			const Job &j = s.jobs[i];
			w.U8(j.kind); w.U16(static_cast<uint16_t>(j.target)); w.F(j.progress); w.U16(static_cast<uint16_t>(j.priority + 0x8000));
		}
	}
	// Newer fields go at the end, so corrupting an earlier byte still lands on the field it names.
	w.U8(static_cast<uint8_t>(s.advanceDeck)); w.U8(static_cast<uint8_t>(s.advanceAt)); w.F(s.advanceMs);
	w.F(s.coolant); w.F(s.coreTemp); w.F(s.containment); w.F(s.breachCountdown);
	w.U8(s.coreShutdown ? 1 : 0); w.U8(s.coreEjected ? 1 : 0); w.U8(s.lost ? 1 : 0);
	for (const System &sys : s.systems) w.U8(sys.fault); // the systems' last named failure states
	// The written-off list: bounded, each entry a time, a kind, a thing and its author.
	{
		const int n = std::min(static_cast<int>(s.losses.size()), LOSS_MAX);
		w.U16(static_cast<uint16_t>(n));
		for (int i = 0; i < n; ++i) {
			const LossEntry &e = s.losses[s.losses.size() - n + i];
			w.U64(static_cast<uint64_t>(std::llround(e.time * 1000.0)));
			w.U8(e.kind); w.U8(e.system ? 1 : 0); w.U16(static_cast<uint16_t>(e.target));
			for (const std::string *str : { &e.what, &e.who }) {
				const int m = std::min(static_cast<int>(str->size()), 63);
				w.U8(static_cast<uint8_t>(m));
				for (int k = 0; k < m; ++k) w.U8(static_cast<uint8_t>((*str)[k]));
			}
		}
	}
	w.U32(s.riskRolls); // the anomaly draws taken: the deterministic counter behind UseSystem
	w.U8(s.leftStanding ? 1 : 0); // the record's mark: was this run ever left standing?
	// The month report and its diff, the promises held, and the log's lifecycle (version 44).
	w.F(s.navCounterLast);
	w.U8(s.logPurged ? 1 : 0);
	auto writeLine = [&w](const ReportLine &l) {
		WriteStr(w, l.scope); WriteStr(w, l.text); WriteStr(w, l.draft);
		w.U16(l.event); w.U16(static_cast<uint16_t>(l.person + 1));
		w.U8(l.struck ? 1 : 0); w.U8(l.added ? 1 : 0);
	};
	auto writeReport = [&w, &writeLine](const MonthReport &rep) {
		w.U16(static_cast<uint16_t>(rep.number));
		w.U64(static_cast<uint64_t>(std::llround(rep.time * 1000.0)));
		w.F(rep.counter); w.F(rep.counterChange);
		WriteStr(w, rep.signer);
		w.U8(static_cast<uint8_t>(rep.department)); w.U8(rep.audience);
		w.U8(rep.open ? 1 : 0); w.U8(rep.signed_ ? 1 : 0);
		const int n = std::min(static_cast<int>(rep.lines.size()), REPORT_LINE_MAX);
		w.U8(static_cast<uint8_t>(n));
		for (int i = 0; i < n; ++i) writeLine(rep.lines[i]);
	};
	writeReport(s.report); // the open draft, and its `open` flag
	const int nReports = std::min(static_cast<int>(s.reports.size()), REPORT_MAX);
	w.U8(static_cast<uint8_t>(nReports));
	for (int i = 0; i < nReports; ++i) writeReport(s.reports[i]);
	const int nPromises = std::min(static_cast<int>(s.promises.size()), PROMISE_MAX);
	w.U8(static_cast<uint8_t>(nPromises));
	for (int i = 0; i < nPromises; ++i) {
		const Promise &p = s.promises[i];
		w.U16(static_cast<uint16_t>(p.promiser + 1)); w.U16(static_cast<uint16_t>(p.beneficiary + 1));
		w.U8(p.kind); w.U8(p.state);
		w.U64(static_cast<uint64_t>(std::llround(p.made * 1000.0)));
		w.F(static_cast<float>(p.deadline));
		WriteStr(w, p.what);
	}
	// The personal log (version 47): a distinct store from the official log, private per person.
	{
		const int n = std::min(static_cast<int>(s.personalLog.size()), PERSONAL_LOG_MAX);
		w.U16(static_cast<uint16_t>(n));
		for (int i = 0; i < n; ++i) {
			const PersonalLogEntry &e = s.personalLog[s.personalLog.size() - n + i];
			w.U16(static_cast<uint16_t>(e.owner + 1));
			w.U64(static_cast<uint64_t>(std::llround(e.time * 1000.0)));
			w.U8(e.visibility);
			WriteStrCap(w, e.who, 63);
			WriteStrCap(w, e.what, PERSONAL_LOG_TEXT_MAX);
		}
	}
	// Access (version 48): the grants for a shift, the one override in progress, and the lock-outs.
	{
		const int n = std::min(static_cast<int>(s.delegations.size()), DELEGATION_MAX);
		w.U8(static_cast<uint8_t>(n));
		for (int i = 0; i < n; ++i) {
			const Delegation &d = s.delegations[i];
			w.U16(static_cast<uint16_t>(d.grantor + 1)); w.U16(static_cast<uint16_t>(d.grantee + 1));
			w.U8(d.station);
			w.U64(static_cast<uint64_t>(std::llround(d.expires * 1000.0)));
		}
		w.U16(static_cast<uint16_t>(s.emergencyOverride.station + 1));
		w.U16(static_cast<uint16_t>(s.emergencyOverride.first + 1));
		w.U16(static_cast<uint16_t>(s.emergencyOverride.second + 1));
		w.U64(static_cast<uint64_t>(std::llround(s.emergencyOverride.readyAt * 1000.0)));
		w.U64(static_cast<uint64_t>(std::llround(s.emergencyOverride.expires * 1000.0)));
		w.U8(s.emergencyOverride.active ? 1 : 0); w.U8(s.emergencyOverride.solo ? 1 : 0);
		const int nl = std::min(static_cast<int>(s.lockouts.size()), LOCKOUT_MAX);
		w.U8(static_cast<uint8_t>(nl));
		for (int i = 0; i < nl; ++i) {
			const Lockout &l = s.lockouts[i];
			w.U8(l.station);
			w.U16(static_cast<uint16_t>(l.lockedBy + 1)); w.U16(static_cast<uint16_t>(l.locked + 1));
			w.U64(static_cast<uint64_t>(std::llround(l.time * 1000.0)));
		}
	}
	// Power allocation (version 51, docs/power-assignment.md): each system's share and who set it,
	// automatic mode, the pending recommendation, and the band delegations.
	for (const System &sys : s.systems) {
		w.F(sys.share); w.U8(sys.allocBy);
		w.U16(static_cast<uint16_t>(sys.allocCrew + 1));
		w.U8(sys.allocDamagedOff ? 1 : 0);
	}
	w.U8(s.powerAuto ? 1 : 0);
	w.U16(static_cast<uint16_t>(s.lastShortfall));
	const int nGrants = std::min(static_cast<int>(s.bandGrants.size()), GRANT_MAX);
	w.U8(static_cast<uint8_t>(nGrants));
	for (int i = 0; i < nGrants; ++i) {
		const BandGrant &g = s.bandGrants[i];
		w.U16(static_cast<uint16_t>(g.grantor + 1)); w.U16(static_cast<uint16_t>(g.grantee + 1));
		w.U8(g.band);
		w.U64(static_cast<uint64_t>(std::llround(g.granted * 1000.0)));
	}
	// The meeting (version 52, docs/staff-meetings.md): the queued briefs and the schedule they fall on.
	w.U16(static_cast<uint16_t>(s.lastWatchMeeting + 1));
	w.U16(static_cast<uint16_t>(s.lastDeptMeetingDay + 1));
	w.U16(s.meetingLatches);
	const int nMeetings = std::min(static_cast<int>(s.pendingMeetings.size()), MEETING_QUEUE_MAX);
	w.U8(static_cast<uint8_t>(nMeetings));
	for (int i = 0; i < nMeetings; ++i) {
		const MeetingBrief &b = s.pendingMeetings[i];
		w.U8(b.kind);
		w.U64(static_cast<uint64_t>(std::llround(b.time * 1000.0)));
		WriteStr(w, b.trigger);
		WriteStr(w, b.decision);
		const int np = std::min(b.presentCount, MEETING_PARTICIPANT_MAX);
		w.U8(static_cast<uint8_t>(np));
		for (int k = 0; k < np; ++k) {
			const BriefParticipant &p = b.present[k];
			w.U16(static_cast<uint16_t>(p.crew + 1));
			WriteStr(w, p.name);
			w.U8(p.post); w.U8(p.watch); w.F(p.mood);
			const int nm = std::min(p.markCount, MEETING_VIEW_MAX);
			w.U8(static_cast<uint8_t>(nm));
			for (int m = 0; m < nm; ++m) {
				const BriefMark &mk = p.marks[m];
				w.U16(mk.event); w.U16(static_cast<uint16_t>(mk.person + 1)); w.U8(mk.source);
				w.F(mk.valence); w.F(mk.salience);
			}
			const int npr = std::min(p.promiseCount, MEETING_VIEW_MAX);
			w.U8(static_cast<uint8_t>(npr));
			for (int m = 0; m < npr; ++m) {
				const BriefPromise &pr = p.promises[m];
				w.U8(pr.kind);
				w.U16(static_cast<uint16_t>(pr.promiser + 1));
				w.U16(static_cast<uint16_t>(pr.beneficiary + 1));
				w.F(static_cast<float>(pr.deadline));
				WriteStr(w, pr.what);
			}
			const int nl = std::min(p.logCount, MEETING_VIEW_MAX);
			w.U8(static_cast<uint8_t>(nl));
			for (int m = 0; m < nl; ++m) {
				const LogEntry &e = p.log[m];
				w.U64(static_cast<uint64_t>(std::llround(e.time * 1000.0)));
				WriteStr(w, e.who); WriteStr(w, e.scope); WriteStr(w, e.what);
			}
		}
		const int no = std::min(b.optionCount, MEETING_OPTION_MAX);
		w.U8(static_cast<uint8_t>(no));
		for (int k = 0; k < no; ++k) {
			const MeetingOption &o = b.options[k];
			w.U8(o.intent); WriteStr(w, o.label); WriteStr(w, o.cost); w.U8(o.effect);
			for (int a = 0; a < MEETING_ALLOC_MAX; ++a) { w.U8(o.alloc[a].system); w.U16(static_cast<uint16_t>(o.alloc[a].percent + 1)); }
			w.U16(static_cast<uint16_t>(o.amount + 0x8000)); w.U8(o.on ? 1 : 0);
		}
		w.U32(static_cast<uint32_t>(b.powerCommitted));
		w.U32(static_cast<uint32_t>(b.powerAvailable));
		w.U32(static_cast<uint32_t>(b.powerShortfall));
		w.U8(b.alert); w.F(b.dilithium);
		w.U16(static_cast<uint16_t>(b.casualties)); w.U16(static_cast<uint16_t>(b.beds));
		w.U8(b.borgPressure ? 1 : 0); w.U32(b.stateDigest);
		for (int k = 0; k < SYS_COUNT; ++k) {
			const BriefSystem &sys = b.systems[k];
			w.U16(static_cast<uint16_t>(sys.allocated));
			w.F(sys.output);
			w.U8(sys.allocBy);
			w.U8(sys.online ? 1 : 0);
		}
	}
	// The character layer (version 53, O8): the three morale reads and each person's bounded
	// conditions. The static half -- species, skills, traits, drives -- is derived from the seed by
	// BuildRoster and is deliberately not stored, so the save stays small and characters stay
	// editable (docs/crew-roster.md, "static character data is content").
	for (const CrewMember &c : s.crew) {
		w.F(c.deficit); w.F(c.outlook); w.F(c.holdings);
		const int nc = std::min<int>(c.conditionCount, CONDITION_MAX);
		w.U8(static_cast<uint8_t>(nc));
		for (int k = 0; k < nc; ++k) {
			const Condition &cd = c.conditions[k];
			w.U8(cd.id); w.U8(static_cast<uint8_t>(cd.valence)); w.U8(cd.magnitude);
			w.F(cd.onset); w.U8(cd.clears); w.U8(cd.visible);
			WriteStrCap(w, cd.source, CONDITION_SOURCE_MAX);
		}
	}
	return w.b;
}

bool Unpack(const uint8_t *data, size_t len, Ship &out)
{
	if (!data) return false;
	Reader r{data, len};
	if (r.U32() != SAVE_MAGIC || r.U16() != SAVE_VERSION || !r.ok) return false;
	const size_t count = r.U16();

	Config cfg;
	cfg.seed = r.U32();
	cfg.dayScale = r.F();
	const uint8_t mode = r.U8(), clockMode = r.U8(), role = r.U8();
	if (!r.ok || !(cfg.dayScale > 0.0f && cfg.dayScale <= 86400.0f)) return false;
	if (mode > MODE_HOLODECK || clockMode > CLOCK_WALL || role > ROLE_MUNRO) return false;
	cfg.mode = static_cast<PlayMode>(mode); cfg.clockMode = static_cast<ClockMode>(clockMode); cfg.role = static_cast<PlayerRole>(role);
	Ship s;
	s.cfg = cfg;
	BuildRoster(s);
	BuildShuttles(s);
	BuildSector(s, 0);
	// The complement, plus any transporter copies beyond it (bounded).
	if (count < static_cast<size_t>(COMPLEMENT) || count > static_cast<size_t>(COMPLEMENT + MAX_DUPLICATES)) return false;
	s.player = static_cast<int16_t>(r.U16());
	s.orderRepairFirst = r.U8() - 1; s.orderSecurityTo = r.U8(); s.orderEvacuate = r.U8(); s.orderTriage = r.U8();
	if (s.orderRepairFirst >= SYS_COUNT || s.orderSecurityTo > DECKS || s.orderEvacuate > DECKS || s.orderTriage > 1) return false;
	s.wallSeconds = r.U64();
	std::string name;
	for (int n = r.U8(); n > 0 && r.ok; --n) name.push_back(static_cast<char>(r.U8()));
	const uint8_t playerRank = r.U8();
	if (s.player < -1 || s.player >= static_cast<int>(s.crew.size()) || playerRank > 6) return false;
	if (s.player >= 0) { s.crew[s.player].name = name; s.crew[s.player].rank = playerRank; }

	s.clock = static_cast<double>(r.U64()) / 1000.0;
	const uint8_t alert = r.U8();
	if (alert > ALERT_RED) return false;
	s.alert = static_cast<Alert>(alert);
	for (System &sys : s.systems) { sys.health = r.Unit(); sys.enabled = r.U8() != 0; sys.priority = static_cast<int16_t>(r.U16()); sys.control = r.Unit(); }
	for (Source &src : s.sources) { src.health = r.Unit(); src.online = r.U8() != 0; }
	for (Deck &d : s.decks) {
		d.atmosphere = r.Unit(); d.gravity = r.Unit(); d.hull = r.Unit(); d.intruders = r.F();
		if (!(d.intruders >= 0.0f && d.intruders <= 10000.0f)) return false;
		d.borg = r.U8() != 0; d.assimilated = r.Unit(); d.forceFieldLevel = r.F(); d.forceField = d.forceFieldLevel > 0.0f; d.fire = r.Unit();
		d.boarderKind = r.U8(); d.objective = r.U8();
		d.compromised = r.U8() != 0; d.dwell = r.F(); d.engaged = r.U8() != 0;
		if (d.boarderKind >= BOARDER_KIND_COUNT || d.objective > DECKS) return false;
	}
	s.stores.deuterium = r.Unit(); s.stores.antimatter = r.Unit(); s.stores.batteries = r.Unit();
	s.stores.torpedoes = r.U16();
	s.stores.spareParts = r.F(); s.stores.medicalSupplies = r.F(); s.stores.rations = r.F(); s.stores.materials = r.F(); s.stores.probes = r.U8(); s.stores.tricorders = r.U8(); s.stores.phasers = r.U8(); s.stores.evSuits = r.U8(); s.stores.tricorderCharge = r.Unit(); s.stores.kitCondition = r.Unit(); s.stores.phaserYield = r.U8();
	if (s.stores.phaserYield >= YIELD_COUNT) return false;
	s.dilithium = r.Unit(); s.crystalCeiling = r.Unit(); s.crystalQuality = r.F(); s.crystalReplacements = r.U16();
	s.shuttles.clear();
	const uint8_t nShuttles = r.U8();
	for (int i = 0; i < nShuttles; ++i) {
		Shuttle sh;
		sh.cls = static_cast<ShuttleClass>(r.U8());
		sh.location = static_cast<ShuttleLocation>(r.U8());
		sh.condition = r.Unit();
		sh.comms = static_cast<ShuttleComms>(r.U8());
		sh.awayBeacon = static_cast<int>(r.U16()) - 1;
		sh.awaySince = r.F();
		sh.returnCondition = r.U8();
		const uint8_t nl = r.U8();
		sh.name.clear(); for (int k = 0; k < nl; ++k) sh.name.push_back(static_cast<char>(r.U8()));
		const uint8_t nm = r.U8();
		for (int k = 0; k < nm; ++k) sh.manifest.push_back(static_cast<int16_t>(r.U16()));
		sh.cargoMaterial = r.F(); sh.cargoRations = r.F(); sh.cargoParts = r.F();
		if (!r.ok || sh.cls >= SHUTTLE_CLASS_COUNT || sh.location > SHUTTLE_LOST || sh.comms > COMMS_LOST || !(sh.condition >= 0.0f && sh.condition <= 1.0f)) return false;
		s.shuttles.push_back(sh);
	}
	if (!(s.stores.materials >= 0.0f && s.stores.materials <= 100000.0f)) return false;
	if (!(s.stores.spareParts >= 0.0f && s.stores.spareParts <= 100000.0f)) return false;
	if (!(s.stores.medicalSupplies >= 0.0f && s.stores.medicalSupplies <= 100000.0f)) return false;
	s.shieldStrength = r.Unit();
	s.beacon = r.U8();
	if (s.beacon >= static_cast<int>(s.sector.size())) return false;
	s.hits = r.U32();
	s.cleanIntercepts = r.U16();
	s.remodulateCooldown = r.F(); s.adaptationSuppressed = r.F();
	const uint32_t visited = r.U32();
	const uint32_t surveyed = r.U32();
	const uint32_t looted = r.U32();
	for (size_t i = 0; i < s.sector.size() && i < 32; ++i) {
		s.sector[i].visited = (visited >> i) & 1u;
		s.sector[i].surveyed = (surveyed >> i) & 1u;
		s.sector[i].looted = (looted >> i) & 1u;
	}
	for (size_t i = 0; i < s.sector.size() && i < 32; ++i) s.sector[i].phenomAttrs = r.U8();
	s.awayBeacon = r.U8() - 1;
	s.course = r.U8() - 1;
	s.surgicalForceField = r.U8() != 0;
	s.emhActive = r.U8() != 0;
	s.enemyHeld = r.U8() != 0;
	if (s.awayBeacon >= static_cast<int>(s.sector.size())) return false;
	if (s.course >= static_cast<int>(s.sector.size())) return false;
	s.enemy.present = r.U8() != 0; s.enemy.borg = r.U8() != 0;
	const uint8_t ekind = r.U8();
	if (ekind >= ENEMY_KIND_COUNT) return false;
	s.enemy.kind = static_cast<EnemyKind>(ekind);
	s.enemy.hull = r.Unit(); s.enemy.shields = r.Unit();
	s.enemy.weapons = r.Unit(); s.enemy.engines = r.Unit(); s.enemy.shieldGen = r.Unit();
	s.enemy.firepower = r.F();
	if (!(s.enemy.firepower >= 0.0f && s.enemy.firepower <= 100.0f)) return false;
	s.enemy.adaptation = r.Unit();
	s.enemy.boarders = r.U8();
	s.contact2.present = r.U8() != 0; s.contact2.borg = r.U8() != 0;
	const uint8_t ekind2 = r.U8();
	if (ekind2 >= ENEMY_KIND_COUNT) return false;
	s.contact2.kind = static_cast<EnemyKind>(ekind2);
	s.contact2.hull = r.Unit(); s.contact2.shields = r.Unit();
	s.contact2.weapons = r.Unit(); s.contact2.engines = r.Unit(); s.contact2.shieldGen = r.Unit();
	s.contact2.firepower = r.F();
	if (!(s.contact2.firepower >= 0.0f && s.contact2.firepower <= 100.0f)) return false;
	s.contact2.adaptation = r.Unit();
	s.contact2.boarders = r.U8();
	const uint8_t tgt = r.U8();
	if (tgt >= TARGET_COUNT) return false;
	s.target = static_cast<EnemySubsystem>(tgt);
	s.pursued = r.U8() != 0; s.pursuitJumps = r.U8(); s.pursuitStrength = r.F();
	s.sectorNumber = r.U8(); s.reachedEnd = r.U8() != 0; s.won = r.U8() != 0;
	s.refugees = r.U16();
	if (s.refugees > 100000) return false;
	s.pylonHealth = r.Unit(); s.mobileEmitter = r.U8() != 0; s.resentment = r.Unit(); s.airponics = r.U8() != 0; s.borgAwareness = r.Unit();
	for (size_t i = 0; i < count; ++i) {
		if (i >= s.crew.size()) {
			// A transporter copy: its name and type came from no seed, so they are in the record.
			CrewMember extra;
			const uint8_t nl = r.U8();
			for (int k = 0; k < nl && r.ok; ++k) extra.name.push_back(static_cast<char>(r.U8()));
			const uint8_t tl = r.U8();
			for (int k = 0; k < tl && r.ok; ++k) extra.type.push_back(static_cast<char>(r.U8()));
			if (!r.ok) return false;
			s.crew.push_back(extra);
		}
		CrewMember &c = s.crew[i];
		c.status = r.U8();
		c.fatigue = r.Unit();
		c.watch = r.U8();
		c.post = r.U8();
		c.exposure = r.F();
		c.recovery = r.Unit();
		c.wounds = r.Unit();
		c.assimScar = r.Unit();
		c.severity = r.Unit();
		c.away = r.U8() != 0;
		c.credentials = r.U8();
		c.faction = r.U8();
		c.brigged = r.U8() != 0;
		c.quartersQuality = r.Unit();
		c.holoCompulsion = r.F();
		if (!(c.holoCompulsion >= 0.0f && c.holoCompulsion <= 10.0f)) return false;
		c.quartersSealed = r.U8() != 0;
		const int mem = r.U8();
		if (mem > MEMORY_MAX) return false;
		c.memories.clear();
		for (int k = 0; k < mem && r.ok; ++k) {
			Memory m;
			m.event = r.U16();
			m.person = static_cast<int16_t>(r.U16()) - 1;
			m.source = r.U8();
			m.time = r.F();
			m.valence = r.F();
			m.salience = r.Unit();
			m.orphaned = r.U8() != 0;
			if (m.source >= MEM_SOURCE_COUNT || m.person < -1 || m.person >= static_cast<int16_t>(count)) return false;
			c.memories.push_back(m);
		}
		if (!(c.exposure >= 0.0f && c.exposure <= 1.0e6f)) return false;
		if (c.status > CREW_ASSIMILATED || c.watch >= WATCHES || c.post > SYS_COUNT || c.faction > 1) return false;
	}
	s.log.clear();
	const int logCount = r.U16();
	for (int i = 0; i < logCount && r.ok; ++i) {
		LogEntry e;
		e.time = static_cast<double>(r.U64()) / 1000.0;
		for (std::string *str : { &e.who, &e.scope, &e.what }) {
			const int m = r.U8();
			str->clear();
			for (int k = 0; k < m && r.ok; ++k) *str += static_cast<char>(r.U8());
		}
		s.log.push_back(e);
	}
	s.jobs.clear();
	const int jobCount = r.U16();
	if (jobCount > JOB_MAX) return false;
	for (int i = 0; i < jobCount && r.ok; ++i) {
		Job j;
		j.kind = r.U8(); j.target = static_cast<int16_t>(r.U16()); j.progress = r.Unit();
		j.priority = static_cast<int16_t>(r.U16() - 0x8000);
		if (j.kind >= JOB_KIND_COUNT) return false;
		s.jobs.push_back(j);
	}
	// The newer fields at the end (see Pack).
	s.advanceDeck = r.U8(); s.advanceAt = r.U8(); s.advanceMs = r.F();
	s.coolant = r.Unit(); s.coreTemp = r.Unit(); s.containment = r.Unit(); s.breachCountdown = r.F();
	s.coreShutdown = r.U8() != 0; s.coreEjected = r.U8() != 0; s.lost = r.U8() != 0;
	for (System &sys : s.systems) sys.fault = r.U8();
	s.losses.clear();
	const int lossCount = r.U16();
	if (lossCount > LOSS_MAX) return false;
	for (int i = 0; i < lossCount && r.ok; ++i) {
		LossEntry e;
		e.time = static_cast<double>(r.U64()) / 1000.0;
		e.kind = r.U8(); e.system = r.U8() != 0; e.target = static_cast<int16_t>(r.U16());
		for (std::string *str : { &e.what, &e.who }) {
			const int m = r.U8();
			str->clear();
			for (int k = 0; k < m && r.ok; ++k) *str += static_cast<char>(r.U8());
		}
		if (e.kind >= LOSS_KIND_COUNT) return false;
		if (e.system ? (e.target < 0 || e.target >= SYS_COUNT) : (e.target < 1 || e.target > DECKS)) return false;
		s.losses.push_back(e);
	}
	s.riskRolls = r.U32(); // the anomaly draws taken (see Pack)
	s.leftStanding = r.U8() != 0;
	// The month report and its diff, the promises held, and the log's lifecycle (version 44).
	auto readLine = [&r]() {
		ReportLine l;
		l.scope = ReadStr(r); l.text = ReadStr(r); l.draft = ReadStr(r);
		l.event = r.U16(); l.person = static_cast<int16_t>(r.U16()) - 1;
		l.struck = r.U8() != 0; l.added = r.U8() != 0;
		return l;
	};
	auto readReport = [&r, &readLine]() {
		MonthReport rep;
		rep.number = r.U16();
		rep.time = static_cast<double>(r.U64()) / 1000.0;
		rep.counter = r.F(); rep.counterChange = r.F();
		rep.signer = ReadStr(r);
		rep.department = r.U8(); rep.audience = r.U8();
		rep.open = r.U8() != 0; rep.signed_ = r.U8() != 0;
		const int n = r.U8();
		for (int i = 0; i < n && r.ok; ++i) rep.lines.push_back(readLine());
		return rep;
	};
	s.navCounterLast = r.F();
	s.logPurged = r.U8() != 0;
	s.report = readReport();
	if (s.report.department > DEPT_COUNT || s.report.audience >= REPORT_AUDIENCE_COUNT) return false;
	if (static_cast<int>(s.report.lines.size()) > REPORT_LINE_MAX) return false;
	for (const ReportLine &l : s.report.lines) if (l.event != 0 && (l.person < -1 || l.person >= static_cast<int>(count))) return false;
	const int nReports = r.U8();
	if (nReports > REPORT_MAX) return false;
	for (int i = 0; i < nReports && r.ok; ++i) {
		s.reports.push_back(readReport());
		if (s.reports.back().department > DEPT_COUNT || s.reports.back().audience >= REPORT_AUDIENCE_COUNT) return false;
	}
	const int nPromises = r.U8();
	if (nPromises > PROMISE_MAX) return false;
	for (int i = 0; i < nPromises && r.ok; ++i) {
		Promise p;
		p.promiser = static_cast<int16_t>(r.U16()) - 1;
		p.beneficiary = static_cast<int16_t>(r.U16()) - 1;
		p.kind = r.U8(); p.state = r.U8();
		p.made = static_cast<double>(r.U64()) / 1000.0;
		p.deadline = r.F();
		p.what = ReadStr(r);
		if (p.kind >= PROMISE_KIND_COUNT || p.state >= PROMISE_STATE_COUNT) return false;
		if (p.beneficiary < 0 || p.beneficiary >= static_cast<int>(count)) return false;
		if (p.promiser < -1 || p.promiser >= static_cast<int>(count)) return false;
		s.promises.push_back(p);
	}
	// The personal log (version 47): a distinct store, read back beside the official one.
	s.personalLog.clear();
	const int personalCount = r.U16();
	if (personalCount > PERSONAL_LOG_MAX) return false;
	for (int i = 0; i < personalCount && r.ok; ++i) {
		PersonalLogEntry e;
		e.owner = static_cast<int16_t>(r.U16()) - 1;
		e.time = static_cast<double>(r.U64()) / 1000.0;
		e.visibility = r.U8();
		e.who = ReadStrCap(r, 63);
		e.what = ReadStrCap(r, PERSONAL_LOG_TEXT_MAX);
		if (e.owner < 0 || e.owner >= static_cast<int>(count)) return false;
		if (e.visibility >= LOG_VISIBILITY_COUNT) return false;
		s.personalLog.push_back(e);
	}
	// Access (version 48): the grants for a shift, the one override in progress, and the lock-outs.
	s.delegations.clear();
	const int delegationCount = r.U8();
	if (delegationCount > DELEGATION_MAX) return false;
	for (int i = 0; i < delegationCount && r.ok; ++i) {
		Delegation d;
		d.grantor = static_cast<int16_t>(r.U16()) - 1;
		d.grantee = static_cast<int16_t>(r.U16()) - 1;
		d.station = r.U8();
		d.expires = static_cast<double>(r.U64()) / 1000.0;
		if (d.station >= STN_COUNT) return false;
		if (d.grantor < 0 || d.grantor >= static_cast<int>(count)) return false;
		if (d.grantee < 0 || d.grantee >= static_cast<int>(count)) return false;
		s.delegations.push_back(d);
	}
	s.emergencyOverride.station = static_cast<int16_t>(r.U16()) - 1;
	s.emergencyOverride.first = static_cast<int16_t>(r.U16()) - 1;
	s.emergencyOverride.second = static_cast<int16_t>(r.U16()) - 1;
	s.emergencyOverride.readyAt = static_cast<double>(r.U64()) / 1000.0;
	s.emergencyOverride.expires = static_cast<double>(r.U64()) / 1000.0;
	s.emergencyOverride.active = r.U8() != 0;
	s.emergencyOverride.solo = r.U8() != 0;
	if (s.emergencyOverride.station >= STN_COUNT) return false;
	if (s.emergencyOverride.first < -1 || s.emergencyOverride.first >= static_cast<int>(count)) return false;
	if (s.emergencyOverride.second < -1 || s.emergencyOverride.second >= static_cast<int>(count)) return false;
	s.lockouts.clear();
	const int lockoutCount = r.U8();
	if (lockoutCount > LOCKOUT_MAX) return false;
	for (int i = 0; i < lockoutCount && r.ok; ++i) {
		Lockout l;
		l.station = r.U8();
		l.lockedBy = static_cast<int16_t>(r.U16()) - 1;
		l.locked = static_cast<int16_t>(r.U16()) - 1;
		l.time = static_cast<double>(r.U64()) / 1000.0;
		if (l.station >= STN_COUNT) return false;
		if (l.lockedBy < 0 || l.lockedBy >= static_cast<int>(count)) return false;
		if (l.locked < 0 || l.locked >= static_cast<int>(count)) return false;
		s.lockouts.push_back(l);
	}
	if (s.advanceDeck > DECKS || s.advanceAt > DECKS) return false;
	// Power allocation (version 51): each system's share and who set it, automatic mode, the pending
	// recommendation, and the band delegations.
	for (System &sys : s.systems) {
		sys.share = r.Unit();
		sys.allocBy = r.U8();
		if (sys.allocBy >= ALLOC_BY_COUNT) return false;
		sys.allocCrew = static_cast<int16_t>(r.U16()) - 1;
		if (sys.allocCrew < -1 || sys.allocCrew >= static_cast<int>(count)) return false;
		sys.allocDamagedOff = r.U8() != 0;
	}
	s.powerAuto = r.U8() != 0;
	s.lastShortfall = r.U16();
	s.bandGrants.clear();
	const int grantCount = r.U8();
	if (grantCount > GRANT_MAX) return false;
	for (int i = 0; i < grantCount && r.ok; ++i) {
		BandGrant g;
		g.grantor = static_cast<int16_t>(r.U16()) - 1;
		g.grantee = static_cast<int16_t>(r.U16()) - 1;
		g.band = r.U8();
		g.granted = static_cast<double>(r.U64()) / 1000.0;
		if (g.band >= BAND_COUNT) return false;
		if (g.grantor < 0 || g.grantor >= static_cast<int>(count)) return false;
		if (g.grantee < 0 || g.grantee >= static_cast<int>(count)) return false;
		s.bandGrants.push_back(g);
	}
	// The meeting (version 52): the schedule and the queued briefs.
	s.lastWatchMeeting = static_cast<int>(r.U16()) - 1;
	s.lastDeptMeetingDay = static_cast<int>(r.U16()) - 1;
	s.meetingLatches = r.U16();
	s.pendingMeetings.clear();
	const int meetingCount = r.U8();
	if (meetingCount > MEETING_QUEUE_MAX) return false;
	for (int i = 0; i < meetingCount && r.ok; ++i) {
		MeetingBrief b;
		b.kind = r.U8();
		b.time = static_cast<double>(r.U64()) / 1000.0;
		b.trigger = ReadStr(r);
		b.decision = ReadStr(r);
		const int np = r.U8();
		if (np > MEETING_PARTICIPANT_MAX) return false;
		b.presentCount = np;
		for (int k = 0; k < np && r.ok; ++k) {
			BriefParticipant &p = b.present[k];
			p.crew = static_cast<int16_t>(r.U16()) - 1;
			if (p.crew < 0 || p.crew >= static_cast<int>(count)) return false;
			p.name = ReadStr(r);
			p.post = r.U8(); p.watch = r.U8(); p.mood = r.F();
			if (p.post > SYS_COUNT || p.watch >= WATCHES) return false;
			const int nm = r.U8();
			if (nm > MEETING_VIEW_MAX) return false;
			p.markCount = nm;
			for (int m = 0; m < nm && r.ok; ++m) {
				BriefMark &mk = p.marks[m];
				mk.event = r.U16();
				mk.person = static_cast<int16_t>(r.U16()) - 1;
				mk.source = r.U8(); mk.valence = r.F(); mk.salience = r.F();
				if (mk.source >= MEM_SOURCE_COUNT) return false;
			}
			const int npr = r.U8();
			if (npr > MEETING_VIEW_MAX) return false;
			p.promiseCount = npr;
			for (int m = 0; m < npr && r.ok; ++m) {
				BriefPromise &pr = p.promises[m];
				pr.kind = r.U8();
				pr.promiser = static_cast<int16_t>(r.U16()) - 1;
				pr.beneficiary = static_cast<int16_t>(r.U16()) - 1;
				pr.deadline = r.F();
				pr.what = ReadStr(r);
				if (pr.kind >= PROMISE_KIND_COUNT) return false;
				if (pr.promiser < -1 || pr.promiser >= static_cast<int>(count)) return false;
				if (pr.beneficiary < -1 || pr.beneficiary >= static_cast<int>(count)) return false;
			}
			const int nl = r.U8();
			if (nl > MEETING_VIEW_MAX) return false;
			p.logCount = nl;
			for (int m = 0; m < nl && r.ok; ++m) {
				LogEntry &e = p.log[m];
				e.time = static_cast<double>(r.U64()) / 1000.0;
				e.who = ReadStr(r); e.scope = ReadStr(r); e.what = ReadStr(r);
			}
		}
		const int no = r.U8();
		if (no > MEETING_OPTION_MAX) return false;
		b.optionCount = no;
		for (int k = 0; k < no && r.ok; ++k) {
			MeetingOption &o = b.options[k];
			o.intent = r.U8(); o.label = ReadStr(r); o.cost = ReadStr(r); o.effect = r.U8();
			for (int a = 0; a < MEETING_ALLOC_MAX; ++a) {
				o.alloc[a].system = r.U8();
				o.alloc[a].percent = static_cast<int>(r.U16()) - 1;
			}
			o.amount = static_cast<int>(r.U16()) - 0x8000;
			o.on = r.U8() != 0;
			if (o.effect >= EFFECT_COUNT) return false;
		}
		b.powerCommitted = static_cast<int>(r.U32());
		b.powerAvailable = static_cast<int>(r.U32());
		b.powerShortfall = static_cast<int>(r.U32());
		b.alert = r.U8();
		b.dilithium = r.F();
		b.casualties = r.U16(); b.beds = r.U16();
		b.borgPressure = r.U8() != 0;
		b.stateDigest = r.U32();
		for (int k = 0; k < SYS_COUNT; ++k) {
			b.systems[k].allocated = r.U16();
			b.systems[k].output = r.F();
			b.systems[k].allocBy = r.U8();
			b.systems[k].online = r.U8() != 0;
			if (b.systems[k].allocBy >= ALLOC_BY_COUNT) return false;
		}
		if (b.kind >= MEET_KIND_COUNT || b.alert >= 3) return false;
		s.pendingMeetings.push_back(b);
	}
	// The character layer (version 53, O8): the three morale reads and the conditions. The static
	// half came back with BuildRoster above.
	for (CrewMember &c : s.crew) {
		c.deficit = r.Unit(); c.outlook = r.Unit(); c.holdings = r.Unit();
		const int nc = r.U8();
		if (nc > CONDITION_MAX) return false;
		c.conditionCount = static_cast<uint8_t>(nc);
		for (int k = 0; k < nc && r.ok; ++k) {
			Condition &cd = c.conditions[k];
			cd.id = r.U8();
			cd.valence = static_cast<int8_t>(r.U8());
			cd.magnitude = r.U8();
			cd.onset = r.F();
			cd.clears = r.U8();
			cd.visible = r.U8();
			cd.source = ReadStrCap(r, CONDITION_SOURCE_MAX);
			if (cd.id == COND_NONE || cd.id >= COND_ID_COUNT) return false;
			if (cd.clears >= CLEAR_COUNT || cd.magnitude >= CMAG_COUNT) return false;
			if (cd.visible == 0) return false; // no hidden penalties
		}
		for (int k = nc; k < CONDITION_MAX; ++k) c.conditions[k] = Condition();
	}
	if (!r.ok || r.left != 0) return false;

	// Derive allocation and locations from the restored state. Power is derived first, so the crew's
	// placement (a patient goes to sickbay only if sickbay is delivering) is a function of the
	// restored power rather than of the zero the outputs start at -- otherwise a save would not
	// replay identically (docs/gates.md, A3).
	UpdatePower(s, 0.0f);
	Tick(s, 0.0f); // derive manning and locations from the restored state
	s.clock = static_cast<double>(std::llround(s.clock * 1000.0)) / 1000.0;
	out = s;
	return true;
}

std::vector<int> CrewOnDeck(const Ship &s, int deck)
{
	std::vector<int> out;
	for (size_t i = 0; i < s.crew.size(); ++i)
		if (s.crew[i].deck == deck && s.crew[i].status != CREW_DEAD && s.crew[i].status != CREW_ASSIMILATED)
			out.push_back(static_cast<int>(i));
	return out;
}

// ---- report -----------------------------------------------------------------------------------

// The captain's log: the situation summarised, in the captain's voice, from the ship's own state.
std::string CaptainLog(const Ship &s)
{
	static const char *const ALERTS[] = {"green", "yellow", "red"};
	char line[200];
	std::string out;
	const int sod = s.SecondOfDay();
	std::snprintf(line, sizeof(line), "Captain's log, day %d, %02d%02d hours. Condition %s.\n", s.Day(), sod / 3600, sod % 3600 / 60, ALERTS[s.alert]);
	out += line;
	int lost = 0, assimilated = 0, injured = 0;
	for (const CrewMember &c : s.crew) {
		if (c.status == CREW_DEAD) ++lost;
		else if (c.status == CREW_ASSIMILATED) ++assimilated;
		else if (c.status == CREW_INJURED) ++injured;
	}
	std::snprintf(line, sizeof(line), "The crew stands at %d of %d; %d wounded, %d lost, %d assimilated.\n",
		s.CrewFit(), static_cast<int>(s.crew.size()), injured, lost, assimilated);
	out += line;
	std::string names;
	int damaged = 0, breached = 0;
	for (int i = 0; i < SYS_COUNT; ++i)
		if (s.systems[i].health < 1.0f) { if (damaged) names += ", "; names += SPECS[i].name; ++damaged; }
	for (int d = 0; d < DECKS; ++d) if (s.decks[d].hull < 1.0f) ++breached;
	if (damaged) { std::snprintf(line, sizeof(line), "%d systems need attention: %s.\n", damaged, names.c_str()); out += line; }
	if (breached) { std::snprintf(line, sizeof(line), "%d decks are open to space.\n", breached); out += line; }
	if (Intruders(s) > 0) { std::snprintf(line, sizeof(line), "%d intruders are aboard.\n", Intruders(s)); out += line; }
	if (InCombat(s)) {
		std::snprintf(line, sizeof(line), "We are engaged with a %s at beacon %d.\n", s.enemy.borg ? "Borg vessel" : "hostile vessel", s.beacon);
		out += line;
	} else if (!s.sector.empty()) {
		std::snprintf(line, sizeof(line), "We are at beacon %d of the sector%s.\n", s.beacon, s.course >= 0 ? ", making for a beacon on course" : ", holding");
		out += line;
	}
	return out;
}

std::string Describe(const Ship &s)
{
	static const char *const ALERTS[] = {"green", "yellow", "red"};
	static const char *const WATCH[] = {"alpha", "beta", "gamma"};
	std::string out;
	char line[256];
	const int sod = s.SecondOfDay();
	std::snprintf(line, sizeof(line), "day %d %02d:%02d  %s watch  condition %s  crew fit %d of %d\n", s.Day(), sod / 3600,
		sod % 3600 / 60, WATCH[s.Watch()], ALERTS[s.alert], s.CrewFit(), static_cast<int>(s.crew.size()));
	out += line;
	if (s.leftStanding) out += "left standing (the ship keeps her own time)\n";
	std::snprintf(line, sizeof(line), "power %d supplied, %d committed (short %d)  budget %d fresh, %d now  %s%s  deuterium %.1f%%  antimatter %.1f%%  batteries %.0f%%  torpedoes %d  parts %.0f  material %.0f  medical %.0f  rations %.0f\n",
		s.PowerAvailable(), PowerCommitted(s), PowerShortfall(s), s.PowerCapacityFresh(), s.PowerCapacityNow(),
		s.powerAuto ? "AUTOMATIC MODE" : "the player sets the allocation",
		Coreless(s) ? "  (CORELESS: survival power only)" : "",
		s.stores.deuterium * 100, s.stores.antimatter * 100, s.stores.batteries * 100, s.stores.torpedoes,
		s.stores.spareParts, s.stores.materials, s.stores.medicalSupplies, s.stores.rations);
	out += line;
	std::snprintf(line, sizeof(line), "  beacon %d of %d in sector %d (%s)  shields at %.0f%%", s.beacon, static_cast<int>(s.sector.size()) - 1,
		s.sectorNumber + 1, s.sector.empty() ? "?" : BeaconKindName(s.sector[s.beacon].kind), s.shieldStrength * 100);
	out += line;
	if (s.course >= 0) { std::snprintf(line, sizeof(line), "  course: beacon %d", s.course); out += line; }
	out += "\n";
	if (s.awayBeacon >= 0) {
		std::snprintf(line, sizeof(line), "  away team of %d on beacon %d\n", AwayTeam(s), s.awayBeacon);
		out += line;
	}
	if (s.enemy.present) {
		std::snprintf(line, sizeof(line), "  %s: hull %.0f%%  shields %.0f%%  weapons %.0f%%  engines %.0f%%  shield gen %.0f%%  (%s)%s\n",
			s.enemy.kind == ENEMY_BORG_VESSEL ? "BORG VESSEL" : s.enemy.kind == ENEMY_WARSHIP ? "WARSHIP" : "RAIDER",
			s.enemy.hull * 100, s.enemy.shields * 100, s.enemy.weapons * 100, s.enemy.engines * 100, s.enemy.shieldGen * 100,
			EnemySubsystemName(s.target), s.enemy.hull <= 0.0f ? "  DESTROYED" : "");
		out += line;
	}
	if (s.contact2.present && s.contact2.hull > 0.0f) {
		std::snprintf(line, sizeof(line), "  second contact: hull %.0f%%  shields %.0f%%  weapons %.0f%%\n",
			s.contact2.hull * 100, s.contact2.shields * 100, s.contact2.weapons * 100);
		out += line;
	}
	if (s.pursued) {
		std::snprintf(line, sizeof(line), "  PURSUED: a raider is %d jump(s) behind\n", s.pursuitJumps);
		out += line;
	}
	for (int d = 0; d < DECKS; ++d) {
		if (s.decks[d].intruders <= 0.0f) continue;
		std::snprintf(line, sizeof(line), "  %s deck %d: %.0f, opposed by %d\n", s.decks[d].borg ? "BORG" : "INTRUDERS", d + 1,
			std::ceil(s.decks[d].intruders), s.decks[d].defenders);
		out += line;
	}
	for (int d = 0; d < DECKS; ++d) {
		if (s.decks[d].assimilated <= 0.0f) continue;
		std::snprintf(line, sizeof(line), "  deck %d is %.0f%% assimilated%s\n", d + 1, s.decks[d].assimilated * 100,
			s.decks[d].stripping ? ", being stripped" : "");
		out += line;
	}
	for (int d = 0; d < DECKS; ++d) {
		if (s.decks[d].fire <= 0.0f) continue;
		std::snprintf(line, sizeof(line), "  deck %d is burning: fire %.0f%%%s\n", d + 1, s.decks[d].fire * 100,
			s.decks[d].firefighting ? ", being fought" : "");
		out += line;
	}
	for (int d = 0; d < DECKS; ++d) {
		if (s.decks[d].gravity >= 0.99f) continue; // only the decks that have lost hold are named
		std::snprintf(line, sizeof(line), "  deck %d: gravity %.0f%%\n", d + 1, s.decks[d].gravity * 100);
		out += line;
	}
	// Grief, made legible: the sealed quarters (what a closed door is) and the wall of names.
	{
		const std::vector<SealedQuarter> sealed = SealedQuarters(s);
		if (!sealed.empty()) {
			std::string names;
			for (size_t i = 0; i < sealed.size(); ++i)
				names += (i ? ", " : "") + s.crew[sealed[i].crew].name + " (deck " + std::to_string(sealed[i].deck) + ")";
			out += "  quarters sealed: " + names + "\n";
		}
		const std::vector<std::string> wall = WallOfNames(s);
		if (!wall.empty()) {
			std::string names;
			for (size_t i = 0; i < wall.size(); ++i) names += (i ? ", " : "") + wall[i];
			out += "  the wall of names: " + names + "\n";
		}
	}
	for (int i = 0; i < SRC_COUNT; ++i) {
		std::snprintf(line, sizeof(line), "  source %-22s %4d of %4d  health %3.0f%%%s\n", SOURCES[i].name, s.sources[i].output,
			SourceCapacity(s, static_cast<SourceId>(i)), s.sources[i].health * 100, s.sources[i].online ? "" : "  OFFLINE");
		out += line;
	}
	for (int i = 0; i < SYS_COUNT; ++i) {
		const System &sys = s.systems[i];
		std::snprintf(line, sizeof(line), "  %-24s deck %2d  alloc %3d%%  power %3d/%3d  output %3.0f%%  manned %d/%d  health %3.0f%%  %s%s%s\n", SPECS[i].name,
			SPECS[i].deck, AllocationPercent(s, static_cast<SystemId>(i)), sys.allocated, SPECS[i].demand, sys.output * 100,
			sys.manned, SPECS[i].crewNeeded, sys.health * 100, AllocationProvenance(s, static_cast<SystemId>(i)).c_str(),
			sys.enabled ? "" : "  OFF", sys.repairing ? "  UNDER REPAIR" : "");
		if (sys.control < 1.0f) {
			out.erase(out.size() - 1);
			std::snprintf(line, sizeof(line), "  control %3.0f%%%s\n", sys.control * 100, sys.control < HIJACKED ? "  HIJACKED" : "");
			out += line;
		}
		out += line;
	}
	return out;
}

} // namespace ship
