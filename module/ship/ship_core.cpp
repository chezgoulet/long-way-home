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
// the order power is shed in: life before structure before everything else.
static const SystemSpec SPECS[SYS_COUNT] = {
	// name                     deck station                 department        demand prio crew
	{"life support",             12, "Environmental Control", DEPT_ENGINEERING,   60,   0,  1},
	{"structural integrity",     11, "Main Engineering",      DEPT_ENGINEERING,   80,   1,  1},
	{"inertial dampers",         11, "Main Engineering",      DEPT_ENGINEERING,   40,   2,  1},
	{"computer core",             9, "Computer Core",         DEPT_SCIENCES,      60,   3,  1},
	{"shields",                   1, "Bridge, Tactical",      DEPT_SECURITY,     200,   4,  1},
	{"sensors",                   8, "Astrometrics",          DEPT_SCIENCES,      60,   5,  2},
	{"warp drive",               11, "Main Engineering",      DEPT_ENGINEERING,  400,   9,  3},
	{"impulse drive",            10, "Impulse Engineering",   DEPT_ENGINEERING,  100,   6,  2},
	{"phasers",                   1, "Bridge, Tactical",      DEPT_SECURITY,     150,   7,  2},
	{"torpedo launchers",         9, "Torpedo Bay",           DEPT_SECURITY,      30,   8,  2},
	{"navigational deflector",   11, "Deflector Control",     DEPT_ENGINEERING,   50,  10,  1},
	{"communications",            1, "Bridge, Operations",    DEPT_COMMAND,       20,  11,  1},
	{"transporters",              4, "Transporter Room 1",    DEPT_ENGINEERING,   60,  12,  1},
	{"sickbay",                   5, "Sickbay",               DEPT_MEDICAL,       30,  13,  2},
	{"turbolifts",                1, "Bridge, Operations",    DEPT_ENGINEERING,   20,  14,  0},
	{"tractor beam",             10, "Shuttlebay Control",    DEPT_ENGINEERING,   60,  15,  1},
	{"replicators",               2, "Mess Hall",             DEPT_ENGINEERING,   60,  16,  0},
	{"holodecks",                 6, "Holodeck 1",            DEPT_ENGINEERING,   60,  17,  0},
};

const SystemSpec &Spec(SystemId id) { return SPECS[id < SYS_COUNT ? id : 0]; }

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

struct SourceSpec {
	const char *name;
	int capacity;          // EPS units at full health [inv]
	float deuteriumPerDay; // fraction of tankage burned per ship-day at full output [inv]
	float antimatterPerDay;
};

static const SourceSpec SOURCES[SRC_COUNT] = {
	{"warp core", 1000, 0.004f, 0.003f},
	{"impulse reactors", 300, 0.003f, 0.0f},
	{"auxiliary fusion", 120, 0.001f, 0.0f},
	{"emergency batteries", 80, 0.0f, 0.0f},
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
const int HOLODECK_DECK = 6;

static float Clamp01(float v) { return std::min(1.0f, std::max(0.0f, v)); }

static void NoteDeath(Ship &s, int deadIndex);      // who was there to see it (memory and consequence)
static void MemoryDecay(Ship &s, float shipSeconds); // salience fades unless reinforced
static void MaturePromises(Ship &s);                 // a deadline that passes unresolved is broken
static void UpdateJobs(Ship &s);                     // the queue of outstanding work (docs/crew-work.md)
static uint32_t AnomalyRoll(uint32_t counter, uint32_t seed); // a deterministic draw (the ruling)

static bool Critical(SystemId id) { return SPECS[id].priority <= SPECS[SYS_COMPUTER_CORE].priority; }

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

struct Named {
	const char *name, *type;
	uint8_t rank;
	Department dept;
	uint8_t post;
};

// Senior staff and the Hazard Team, as the game names and models them [lore]. All stand alpha watch.
static const Named NAMED[] = {
	{"Kathryn Janeway", "janeway", 6, DEPT_COMMAND, SYS_COUNT},
	{"Chakotay", "chakotay", 5, DEPT_COMMAND, SYS_COUNT},
	{"Tuvok", "tuvok", 4, DEPT_SECURITY, SYS_SHIELDS},
	{"Tom Paris", "paris", 3, DEPT_COMMAND, SYS_COUNT},
	{"Harry Kim", "kim", 1, DEPT_COMMAND, SYS_COMMUNICATIONS},
	{"B'Elanna Torres", "torres", 3, DEPT_ENGINEERING, SYS_WARP_DRIVE},
	{"The Doctor", "doctor", 3, DEPT_MEDICAL, SYS_SICKBAY},
	{"Seven of Nine", "seven", 0, DEPT_SCIENCES, SYS_SENSORS},
	{"Neelix", "neelix", 0, DEPT_COMMAND, SYS_COUNT},
	{"Vorik", "vorik", 1, DEPT_ENGINEERING, SYS_WARP_DRIVE},
	{"Les Foster", "Foster", 3, DEPT_SECURITY, SYS_COUNT},
	{"Alexander Munro", "munro", 1, DEPT_SECURITY, SYS_COUNT},
	{"Rick Biessman", "Biessman", 0, DEPT_SECURITY, SYS_COUNT},
	{"Austin Chang", "Chang", 0, DEPT_SECURITY, SYS_COUNT},
	{"Telsia Murphy", "Telsia", 0, DEPT_SECURITY, SYS_COUNT},
	{"Chell", "Chell", 0, DEPT_ENGINEERING, SYS_COUNT},
	{"Juliet Jurot", "Jurot", 0, DEPT_MEDICAL, SYS_COUNT},
	{"Kenn", "Kenn", 0, DEPT_SECURITY, SYS_COUNT},
	{"Odell", "Odell", 0, DEPT_SECURITY, SYS_COUNT},
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
	int have[DEPT_COUNT] = {0, 0, 0, 0, 0};
	for (const Named &n : NAMED) {
		CrewMember c;
		c.name = n.name;
		c.type = n.type;
		c.rank = n.rank;
		c.dept = n.dept;
		c.watch = 0;
		c.post = n.post;
		c.quartersDeck = 3; // senior officers' quarters [lore]
		c.quartersQuality = 0.7f; // senior quarters are the better ones [inv]
		s.crew.push_back(c);
		++have[n.dept];
	}

	// The rest are generated, the same crew for the same seed, and spread evenly over the watches.
	uint32_t r = s.cfg.seed ? s.cfg.seed : 1;
	auto next = [&r]() { r = r * 1664525u + 1013904223u; return r >> 8; };
	for (int d = 0; d < DEPT_COUNT; ++d) {
		for (int i = have[d]; i < DEPT_SIZE[d]; ++i) {
			CrewMember c;
			char name[32];
			std::snprintf(name, sizeof(name), "Crewman %03d", static_cast<int>(s.crew.size()) + 1);
			c.name = name;
			c.dept = static_cast<Department>(d);
			c.type = GenericType(c.dept, static_cast<int>(next() % 1000));
			c.rank = next() % 5 == 0 ? 1 : 0;
			c.watch = static_cast<uint8_t>(i % WATCHES);
			c.quartersDeck = static_cast<uint8_t>(4 + next() % 6); // crew quarters, decks 4-9 [inv]
			c.quartersQuality = 0.4f + (next() % 5) * 0.1f;        // some bunk better than others [inv]
			c.faction = (next() % 6 == 0) ? 1 : 0;                 // a Maquis alongside the Starfleet crew [lore]
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
	const float down = std::max(0.0f, 0.5f - c.morale) / 0.5f;
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

// Whoever commands: the player's character if there is one, otherwise the captain or first officer.
std::string CommandingOfficer(const Ship &s)
{
	if (s.player >= 0 && s.player < static_cast<int>(s.crew.size())) return s.crew[s.player].name;
	for (const CrewMember &c : s.crew)
		if (c.status == CREW_FIT && c.rank >= 5) return c.name;
	return "command";
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
			c.deck = HOLODECK_DECK;
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
		case ACT_RECREATION: c.deck = s.systems[SYS_HOLODECKS].output > 0.0f ? HOLODECK_DECK : MESS_DECK; break;
		default: c.deck = c.quartersDeck; break;
		}

		// An evacuated deck is left: whoever the routine would put there goes to the mess hall instead,
		// station or not. (Security ordered to that same deck is the exception: that is what a guard is.)
		if (s.orderEvacuate >= 1 && c.deck == s.orderEvacuate && !(c.dept == DEPT_SECURITY && s.orderSecurityTo == s.orderEvacuate)) {
			c.deck = static_cast<uint8_t>(s.orderEvacuate == MESS_DECK ? HOLODECK_DECK : MESS_DECK);
			if (a == ACT_ON_DUTY && c.post < SYS_COUNT && SPECS[c.post].deck == s.orderEvacuate) {
				c.activity = ACT_PERSONAL; // off their station, by order
				a = ACT_PERSONAL;
			}
		}

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
		if (a == ACT_ON_DUTY) c.fatigue += hours / 20.0f;       // a double watch leaves you spent [inv]
		else if (a == ACT_SLEEP) c.fatigue -= hours / 8.0f;     // a night's sleep clears it [inv]
		else c.fatigue -= hours / 40.0f;
		c.fatigue = std::min(1.0f, std::max(0.0f, c.fatigue));

		// Morale moves toward what the day offers: rest, a meal (if the replicators run), recreation
		// (if the holodeck does), a watch, and how many have been lost and are still missed. It eases
		// toward that target over hours rather than snapping, so a bad afternoon is not a bad week.
		// A meal comes from the galley's rations or the replicators; with neither there is none.
		if (a == ACT_MEAL && s.stores.rations > 0.0f)
			s.stores.rations = std::max(0.0f, s.stores.rations - RATIONS_PER_CREW_DAY * hours / 24.0f);
		float target = 0.5f;
		if (a == ACT_SLEEP) target = 0.75f;
		else if (a == ACT_MEAL) target = (s.systems[SYS_REPLICATORS].output > 0.0f || s.stores.rations > 0.0f) ? 0.8f : 0.4f;
		else if (a == ACT_RECREATION) target = s.systems[SYS_HOLODECKS].output > 0.0f ? 0.9f : 0.65f;
		else if (a == ACT_ON_DUTY) target = s.alert == ALERT_GREEN ? 0.7f : 0.55f;
		if (s.alert == ALERT_RED) target -= 0.15f;
		if (c.deck >= 1 && c.deck <= DECKS && s.decks[c.deck - 1].atmosphere < AIRLESS) target -= 0.2f;
		if (s.stores.rations <= 0.0f && s.systems[SYS_REPLICATORS].output <= 0.0f) target -= 0.1f; // hunger
		target -= lossFactor;
		target -= factionDrag;
		target -= crowding;
		target += (c.quartersQuality - 0.5f) * 0.1f; // where they bunk colours the mood
		target -= std::min(0.3f, Trauma(c) * 0.15f); // what they carry, until the holodeck fades it
		target -= c.assimScar * SCAR_DRAG;           // a de-assimilation's residue never fully fades
		c.morale += (target - c.morale) * std::min(1.0f, hours / 6.0f); // a few hours to shift the mood [inv]
		c.morale = std::min(1.0f, std::max(0.0f, c.morale));


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
		int fit = 0;
		for (const CrewMember &c : s.crew)
			if (c.status == CREW_FIT) { morale += c.morale; fatigue += c.fatigue; ++fit; }
		if (fit) {
			morale /= fit; fatigue /= fit;
			const char *why = s.alert == ALERT_RED ? "red alert"
				: lossFactor > 0.02f ? "losses still felt"
				: s.SecondOfDay() >= 22 * 3600 || s.SecondOfDay() < 6 * 3600 ? "the night watch"
				: "an ordinary watch";
			char note[160];
			std::snprintf(note, sizeof(note), "the crew's mood: morale %d%%, fatigue %d%% (%s)",
				static_cast<int>(morale * 100 + 0.5f), static_cast<int>(fatigue * 100 + 0.5f), why);
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

	// Sickbay last, so it sees the injuries the fight and the air have just caused.
	TreatCasualties(s, shipSeconds);

	// And the queue of outstanding work, one entry per thing that needs doing.
	UpdateJobs(s);
}

// ---- power ------------------------------------------------------------------------------------

static void UpdatePower(Ship &s, float shipSeconds)
{
	const float days = shipSeconds / SECONDS_PER_DAY;

	// What is asked for, in shedding order.
	int order[SYS_COUNT];
	int demand[SYS_COUNT];
	int wanted = 0, critical = 0;
	for (int i = 0; i < SYS_COUNT; ++i) {
		order[i] = i;
		const System &sys = s.systems[i];
		const bool on = sys.enabled && sys.health > 0.0f && !SuppressedByAlert(s.alert, static_cast<SystemId>(i));
		demand[i] = on ? SPECS[i].demand : 0;
		wanted += demand[i];
		if (Critical(static_cast<SystemId>(i))) critical += demand[i];
	}
	std::stable_sort(order, order + SYS_COUNT, [&](int a, int b) { return s.systems[a].priority < s.systems[b].priority; });

	// What can be supplied. Reactors run only as hard as the load asks, in order, and only while
	// they have fuel; the batteries discharge only to keep the critical systems alive.
	int supply = 0;
	for (int i = 0; i < SRC_COUNT; ++i) {
		Source &src = s.sources[i];
		src.output = 0;
		if (!src.online || src.health <= 0.0f) continue;
		const bool fuelled = i == SRC_BATTERIES ? s.stores.batteries > 0.0f
			: s.stores.deuterium > 0.0f && (SOURCES[i].antimatterPerDay == 0.0f || s.stores.antimatter > 0.0f);
		if (!fuelled) continue;
		const int capacity = static_cast<int>(SOURCES[i].capacity * src.health);
		const int need = i == SRC_BATTERIES ? critical - supply : wanted - supply;
		src.output = std::max(0, std::min(capacity, need));
		supply += src.output;

		const float load = SOURCES[i].capacity ? static_cast<float>(src.output) / SOURCES[i].capacity : 0.0f;
		s.stores.deuterium = std::max(0.0f, s.stores.deuterium - SOURCES[i].deuteriumPerDay * load * days);
		s.stores.antimatter = std::max(0.0f, s.stores.antimatter - SOURCES[i].antimatterPerDay * load * days);
		if (i == SRC_BATTERIES)
			s.stores.batteries = std::max(0.0f, s.stores.batteries - load * shipSeconds / (BATTERY_HOURS * 3600.0f));
	}

	// Distribute. A system takes its whole demand or what is left; nothing is created or lost.
	int left = supply;
	for (int k = 0; k < SYS_COUNT; ++k) {
		const int i = order[k];
		System &sys = s.systems[i];
		sys.allocated = std::min(demand[i], left);
		left -= sys.allocated;
		const float powered = demand[i] ? static_cast<float>(sys.allocated) / demand[i] : 0.0f;
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
		c.morale = std::max(0.0f, c.morale - 0.2f);
		LogEvent(s, CommandingOfficer(s), "crew", c.name + " is convicted at the hearing");
	} else {
		c.brigged = false;
		c.morale = std::min(1.0f, c.morale + 0.1f);
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
	for (CrewMember &c : s.crew) c.morale = std::min(1.0f, c.morale + 0.03f);
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
		for (CrewMember &c : s.crew) c.morale = std::min(1.0f, c.morale + 0.05f);
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
	float ours = s.systems[SYS_PHASERS].output * minutes / PHASER_MINUTES;
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

bool MayOperate(const CrewMember &who, Station st)
{
	if (who.status != CREW_FIT || who.brigged) return false;
	if (who.rank >= 4) return true;
	if (st < STN_COUNT && (who.credentials & (1u << st))) return true; // a trained cross-qualification
	if (st >= STN_COUNT) return false;
	switch (st) {
	case STN_ENGINEERING: return who.dept == DEPT_ENGINEERING;
	case STN_TACTICAL: return who.dept == DEPT_SECURITY;
	case STN_OPS: case STN_CONN: return who.dept == DEPT_COMMAND || who.dept == DEPT_SCIENCES;
	case STN_SICKBAY: return who.dept == DEPT_MEDICAL;
	default: return false;
	}
}

bool MayCallAlert(const CrewMember &who, Station st)
{
	return who.rank >= 3 && (st == STN_ENGINEERING || st == STN_TACTICAL) && MayOperate(who, st);
}

bool MayCommand(const CrewMember &who) { return who.status == CREW_FIT && who.rank >= 5; }

bool PlayerMayOperate(const Ship &s, Station st)
{
	if (s.cfg.role == ROLE_IN_COMMAND) return true;
	return s.player >= 0 && s.player < static_cast<int>(s.crew.size()) && MayOperate(s.crew[s.player], st);
}

bool PlayerMayCommand(const Ship &s)
{
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
		if (c.morale < 0.7f) { c.morale = std::min(1.0f, c.morale + 0.15f); ++lifted; }
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
// sourced read-it-in-the-log, still held, with their citation gone.
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
		c.morale = std::min(1.0f, c.morale + 0.15f);
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
		c.morale = std::min(1.0f, c.morale + 0.05f);
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
uint8_t UseSystem(Ship &s, SystemId id, float stress, const std::string &who)
{
	if (id >= SYS_COUNT) return ANOMALY_NONE;
	System &sys = s.systems[id];
	const float condition = SystemCondition(sys);
	const uint8_t sev = RollAnomaly(condition, stress, AnomalyRoll(s.riskRolls++, s.cfg.seed));
	if (sev == ANOMALY_NONE) return sev;

	const std::string author = who.empty() ? std::string(AuthorFor(s, SPECS[id].dept, SPECS[id].station)) : who;
	const int condPct = static_cast<int>(condition * 100.0f + 0.5f);
	const int loadPct = static_cast<int>(Clamp01(stress) * 100.0f + 0.5f);
	LogEvent(s, author, "engineering", std::string(SPECS[id].name) + ": " + AnomalyName(sev) + " anomaly at "
		+ std::to_string(condPct) + "% condition under " + std::to_string(loadPct) + "% load");
	// A scar at the least; a system that has just bitten is worse than it was.
	sys.health = Clamp01(sys.health - (sev == ANOMALY_DEGRADED ? 0.05f : sev == ANOMALY_ACUTE ? 0.15f : 0.3f));
	// The visible let-go: the console arcs at whoever is manning this system, the one place on the
	// ship the operator is standing. This is canon's exploding console, and it is the mechanism, not
	// a flourish.
	if (sev >= ANOMALY_ACUTE) {
		for (CrewMember &c : s.crew) {
			if (c.status != CREW_FIT || c.brigged) continue;
			if (c.post != id) continue;
			c.status = CREW_INJURED;
			c.severity = std::max(c.severity, sev == ANOMALY_ACUTE ? 0.5f : 0.8f);
			LogEvent(s, AuthorFor(s, DEPT_MEDICAL, "sickbay"), "sickbay",
				c.name + " was hurt when the " + std::string(SPECS[id].name) + " console let go");
			break;
		}
	}
	return sev;
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
	w.F(s.stores.spareParts); w.F(s.stores.medicalSupplies); w.F(s.stores.rations); w.F(s.stores.materials); w.U8(static_cast<uint8_t>(s.stores.probes)); w.U8(static_cast<uint8_t>(s.stores.tricorders)); w.U8(static_cast<uint8_t>(s.stores.phasers)); w.U8(static_cast<uint8_t>(s.stores.evSuits)); w.F(s.stores.tricorderCharge); w.F(s.stores.kitCondition);
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
		w.U8(c.status); w.F(c.fatigue); w.F(c.morale); w.U8(c.watch); w.U8(c.post); w.F(c.exposure); w.F(c.recovery); w.F(c.wounds); w.F(c.assimScar); w.F(c.severity);
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
	s.stores.spareParts = r.F(); s.stores.medicalSupplies = r.F(); s.stores.rations = r.F(); s.stores.materials = r.F(); s.stores.probes = r.U8(); s.stores.tricorders = r.U8(); s.stores.phasers = r.U8(); s.stores.evSuits = r.U8(); s.stores.tricorderCharge = r.Unit(); s.stores.kitCondition = r.Unit();
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
		c.morale = r.Unit();
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
	if (s.advanceDeck > DECKS || s.advanceAt > DECKS) return false;
	if (!r.ok || r.left != 0) return false;

	Tick(s, 0.0f); // derive allocation, manning and locations from the restored state
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
	char line[160];
	std::string out;
	const int sod = s.SecondOfDay();
	std::snprintf(line, sizeof(line), "day %d %02d:%02d  %s watch  condition %s  crew fit %d of %d\n", s.Day(), sod / 3600,
		sod % 3600 / 60, WATCH[s.Watch()], ALERTS[s.alert], s.CrewFit(), static_cast<int>(s.crew.size()));
	out += line;
	if (s.leftStanding) out += "left standing (the ship keeps her own time)\n";
	std::snprintf(line, sizeof(line), "power %d supplied, %d allocated  deuterium %.1f%%  antimatter %.1f%%  batteries %.0f%%  torpedoes %d  parts %.0f  material %.0f  medical %.0f  rations %.0f\n",
		s.PowerAvailable(), s.PowerAllocated(), s.stores.deuterium * 100, s.stores.antimatter * 100, s.stores.batteries * 100, s.stores.torpedoes,
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
			SOURCES[i].capacity, s.sources[i].health * 100, s.sources[i].online ? "" : "  OFFLINE");
		out += line;
	}
	for (int i = 0; i < SYS_COUNT; ++i) {
		const System &sys = s.systems[i];
		std::snprintf(line, sizeof(line), "  %-24s deck %2d  power %3d/%3d  manned %d/%d  health %3.0f%%  output %3.0f%%%s%s\n", SPECS[i].name,
			SPECS[i].deck, sys.allocated, SPECS[i].demand, sys.manned, SPECS[i].crewNeeded, sys.health * 100, sys.output * 100,
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
