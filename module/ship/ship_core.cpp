// ship_core.cpp -- see ship_core.h. No game headers in this file.
//
// Figures marked [lore] are sourced in docs/lore-ledger.md; figures marked [inv] are invented for
// play and listed there as such.

#include "ship_core.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
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
const float ATMOSPHERE_REGEN_HOURS = 1.0f; // life support at full output refills a deck in this [inv]
const float ATMOSPHERE_STALE_HOURS = 12.0f; // with no life support a sealed deck lasts this [inv]
const float VENT_MINUTES = 5.0f;           // a deck fully open to space empties in this [inv]

static const int DEPT_SIZE[DEPT_COUNT] = {20, 50, 25, 30, 16}; // sums to COMPLEMENT [inv split]
static const int DEPT_DECK[DEPT_COUNT] = {1, 11, 4, 8, 5};     // where department duties are done
const int MESS_DECK = 2;
const int HOLODECK_DECK = 6;

static float Clamp01(float v) { return std::min(1.0f, std::max(0.0f, v)); }

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

// Sickbay: a fixed number of beds (three standard and one surgical), a triage order, and the
// supplies treatment costs. The worst (or the most senior) cases get the beds; anyone left waiting
// worsens, and a critical case nobody reaches dies. That is the decision the gap asks for: more
// casualties than beds is a choice, not a queue that clears itself.
static void TreatCasualties(Ship &s, float shipSeconds)
{
	const float hours = shipSeconds / 3600.0f;
	std::vector<int> hurt;
	for (int i = 0; i < static_cast<int>(s.crew.size()); ++i) {
		s.crew[i].underCare = false;
		if (s.crew[i].status == CREW_INJURED) hurt.push_back(i);
	}
	if (hurt.empty()) return;
	std::stable_sort(hurt.begin(), hurt.end(), [&](int a, int b) {
		if (s.orderTriage == 1) return s.crew[a].rank > s.crew[b].rank; // rank first
		return s.crew[a].severity > s.crew[b].severity;                 // worst first (default)
	});
	int bed = 0;
	for (int i : hurt) {
		CrewMember &c = s.crew[i];
		if (bed < SICKBAY_BEDS && s.systems[SYS_SICKBAY].output > 0.0f && s.stores.medicalSupplies > 0.0f) {
			++bed;
			c.underCare = true;
			c.deck = SICKBAY_DECK;
			c.recovery += s.systems[SYS_SICKBAY].output * shipSeconds / (TREATMENT_HOURS * 3600.0f);
			s.stores.medicalSupplies = std::max(0.0f, s.stores.medicalSupplies - MEDICAL_PER_PATIENT_HOUR * hours);
			if (c.recovery >= 1.0f) {
				c.status = CREW_FIT;
				c.recovery = c.exposure = c.wounds = c.severity = 0.0f;
			}
		} else {
			// No bed, no output, or no supplies: the injury goes on getting worse, and can kill.
			c.severity = std::min(1.0f, c.severity + DETERIORATE_PER_HOUR * hours);
			c.wounds = std::max(c.wounds, c.severity);
			if (c.severity >= 1.0f) { c.status = CREW_DEAD; c.recovery = 0.0f; }
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
	for (Deck &d : s.decks) d.defenders = d.stripping = 0;
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

	for (CrewMember &c : s.crew) {
		if (c.status == CREW_DEAD || c.status == CREW_ASSIMILATED) {
			c.activity = ACT_SLEEP; // lost to the ship: nowhere aboard, doing nothing
			c.deck = 0;
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
				c.activity = ACT_SLEEP;
				c.deck = 0;
				continue;
			}
			if (c.exposure >= EXPOSURE_INJURES && c.status == CREW_FIT) {
				c.status = CREW_INJURED;
				c.severity = std::max(c.severity, std::min(1.0f,
					0.3f + 0.7f * (c.exposure - EXPOSURE_INJURES) / (EXPOSURE_KILLS - EXPOSURE_INJURES)));
			}
		} else if (c.status == CREW_FIT) {
			c.exposure = std::max(0.0f, c.exposure - shipSeconds); // catching their breath
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
		float target = 0.5f;
		if (a == ACT_SLEEP) target = 0.75f;
		else if (a == ACT_MEAL) target = s.systems[SYS_REPLICATORS].output > 0.0f ? 0.8f : 0.5f;
		else if (a == ACT_RECREATION) target = s.systems[SYS_HOLODECKS].output > 0.0f ? 0.9f : 0.65f;
		else if (a == ACT_ON_DUTY) target = s.alert == ALERT_GREEN ? 0.7f : 0.55f;
		if (s.alert == ALERT_RED) target -= 0.15f;
		if (c.deck >= 1 && c.deck <= DECKS && s.decks[c.deck - 1].atmosphere < AIRLESS) target -= 0.2f;
		target -= lossFactor;
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
		}

		placed:
		if (a == ACT_ON_DUTY && c.post < SYS_COUNT) { ++s.systems[c.post].manned; s.systems[c.post].staffing += Effectiveness(c); }

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
		}
	}

	// The work itself: paid for in parts, and it stops when they run out.
	for (int k = 0; k < nWant; ++k) {
		System &sys = s.systems[wantRepair[k]];
		if (!sys.repairing || s.stores.spareParts <= 0.0f) continue;
		float gain = sys.repairing * shipSeconds / (REPAIR_HOURS_PER_SYSTEM * 3600.0f);
		gain = std::min(gain, 1.0f - sys.health);
		gain = std::min(gain, s.stores.spareParts / PARTS_PER_SYSTEM);
		sys.health += gain;
		s.stores.spareParts = std::max(0.0f, s.stores.spareParts - gain * PARTS_PER_SYSTEM);
	}

	// Sickbay last, so it sees the injuries the fight and the air have just caused.
	TreatCasualties(s, shipSeconds);
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
}

// Boarders: fight, hack, advance. Deterministic -- attrition is a rate, not a roll.
static void UpdateIntruders(Ship &s, float shipSeconds)
{
	const float minutes = shipSeconds / 60.0f;
	float arriving[DECKS] = {0};
	for (int d = 0; d < DECKS; ++d) {
		Deck &deck = s.decks[d];
		if (deck.intruders <= 0.0f) { deck.intruders = 0.0f; continue; }

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
				if (c.wounds >= 1.0f) { c.status = CREW_INJURED; c.severity = std::max(c.severity, 0.5f); }
			}
		}
		// the last of a party that is being killed does not linger as a fraction
		if (deck.intruders < 0.05f && (deck.defenders > 0 || deck.atmosphere < AIRLESS)) deck.intruders = 0.0f;
		if (deck.intruders <= 0.0f) { deck.intruders = 0.0f; continue; }

		// Those not pinned by defenders work on the systems stationed here, sharing themselves out.
		const float free_ = std::max(0.0f, deck.intruders - deck.defenders);
		// (a system already wholly theirs, a dark console and a wreck are not work)
		auto workable = [&](int i) {
			return SPECS[i].deck == d + 1 && s.systems[i].enabled && s.systems[i].health > 0.0f && s.systems[i].control > 0.0f;
		};
		int here = 0;
		for (int i = 0; i < SYS_COUNT; ++i)
			if (workable(i)) ++here;
		if (here > 0) {
			for (int i = 0; i < SYS_COUNT; ++i) {
				System &sys = s.systems[i];
				if (!workable(i)) continue;
				sys.control = std::max(0.0f, sys.control - (free_ / here) * minutes / HACK_MINUTES);
			}
		} else if (deck.defenders == 0) {
			// Nothing to take here and nobody stopping them: on toward the bridge or Engineering.
			const int target = std::abs(d + 1 - BRIDGE_DECK) <= std::abs(d + 1 - ENGINEERING_DECK) ? BRIDGE_DECK : ENGINEERING_DECK;
			if (target != d + 1) {
				// the party moves off a few at a time, and its last stragglers go together
				const float moving = deck.intruders < 0.05f ? deck.intruders : std::min(deck.intruders, deck.intruders * minutes / ADVANCE_MINUTES);
				deck.intruders -= moving;
				arriving[d + (target > d + 1 ? 1 : -1)] += moving;
			}
		}
	}
	for (int d = 0; d < DECKS; ++d) s.decks[d].intruders += arriving[d];

	// The Borg: convert the deck, take the crew on it, and hold its systems outright.
	for (int d = 0; d < DECKS; ++d) {
		Deck &deck = s.decks[d];
		if (deck.intruders <= 0.0f) deck.borg = false;
		const float drones = deck.borg ? std::max(0.0f, deck.intruders - deck.defenders) : 0.0f;
		if (drones > 0.0f) {
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
}

// ---- the outside ------------------------------------------------------------------------------

static void BuildSector(Ship &s)
{
	uint32_t r = (s.cfg.seed ? s.cfg.seed : 1) * 2654435761u;
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
		const uint32_t roll = next() % 10;
		s.sector[i].kind = roll < 4 ? BEACON_EMPTY : roll < 7 ? BEACON_HOSTILE : roll < 9 ? BEACON_DERELICT : BEACON_BORG;
	}
	s.sector[0].visited = true; // where the ship starts: nothing here
}

static void Arrive(Ship &s)
{
	Beacon &b = s.sector[s.beacon];
	const bool first = !b.visited;
	b.visited = true;
	s.enemy = Enemy();
	if (!first) return; // what was here has been dealt with, or taken
	if (b.kind == BEACON_HOSTILE || b.kind == BEACON_BORG) {
		s.enemy.present = true;
		s.enemy.borg = b.kind == BEACON_BORG;
		s.enemy.hull = 1.0f;
		s.enemy.shields = 1.0f;
		s.enemy.firepower = s.enemy.borg ? 0.5f : 0.25f;
		s.enemy.boarders = s.enemy.borg ? 4 : 3;
	} else if (b.kind == BEACON_DERELICT) {
		s.stores.spareParts += SALVAGE_PARTS;
	}
}

bool InCombat(const Ship &s) { return s.enemy.present && s.enemy.hull > 0.0f; }

bool Jump(Ship &s, int toBeacon)
{
	if (s.sector.empty() || toBeacon < 0 || toBeacon >= static_cast<int>(s.sector.size())) return false;
	const std::vector<int> &links = s.sector[s.beacon].links;
	if (std::find(links.begin(), links.end(), toBeacon) == links.end()) return false;
	if (s.systems[SYS_WARP_DRIVE].output < 0.5f) return false;
	if (s.stores.deuterium < JUMP_DEUTERIUM || s.stores.antimatter < JUMP_ANTIMATTER) return false;
	s.stores.deuterium -= JUMP_DEUTERIUM;
	s.stores.antimatter -= JUMP_ANTIMATTER;
	s.beacon = toBeacon;
	Arrive(s);
	return true;
}

bool FireTorpedo(Ship &s)
{
	if (!InCombat(s) || s.stores.torpedoes <= 0 || s.systems[SYS_TORPEDO_LAUNCHERS].output <= 0.0f) return false;
	--s.stores.torpedoes;
	// Shields take what they can of it; the rest reaches the hull.
	const float absorbed = std::min(s.enemy.shields, TORPEDO_HULL);
	s.enemy.shields -= absorbed;
	s.enemy.hull = std::max(0.0f, s.enemy.hull - (TORPEDO_HULL - absorbed));
	return true;
}

static void UpdateOutside(Ship &s, float shipSeconds)
{
	const float minutes = shipSeconds / 60.0f;
	// Our shields recharge by what the shield system delivers, and hold nothing without it.
	const float shieldOut = s.systems[SYS_SHIELDS].output;
	s.shieldStrength = std::min(shieldOut > 0.0f ? 1.0f : 0.0f, s.shieldStrength + shieldOut * minutes / SHIELD_RECHARGE_MINUTES);
	if (!InCombat(s)) return;

	// Our phasers: their shields first, then their hull.
	float ours = s.systems[SYS_PHASERS].output * minutes / PHASER_MINUTES;
	const float onShields = std::min(ours, s.enemy.shields);
	s.enemy.shields -= onShields;
	s.enemy.hull = std::max(0.0f, s.enemy.hull - (ours - onShields));
	if (s.enemy.hull <= 0.0f) return; // it is over; whatever they sent across is still aboard

	// Their fire: our shields first; what gets through lands on a deck and the systems stationed there.
	float theirs = s.enemy.firepower * minutes;
	const float held = std::min(theirs, s.shieldStrength);
	s.shieldStrength -= held;
	theirs -= held;
	if (theirs > 0.0f) {
		const float share = theirs / s.enemy.firepower; // minutes of unopposed fire
		const int deck = static_cast<int>(s.hits * 7u % DECKS);
		++s.hits;
		s.decks[deck].hull = Clamp01(s.decks[deck].hull - HIT_HULL * share);
		for (int i = 0; i < SYS_COUNT; ++i)
			if (SPECS[i].deck == deck + 1) s.systems[i].health = Clamp01(s.systems[i].health - HIT_SYSTEM * share);
	}

	// With our shields down they send their party across, once, to where it will hurt.
	if (s.shieldStrength <= 0.0f && s.enemy.boarders > 0) {
		if (s.enemy.borg) BoardBorg(s, ENGINEERING_DECK, s.enemy.boarders);
		else Board(s, ENGINEERING_DECK, s.enemy.boarders);
		s.enemy.boarders = 0;
	}
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
	BuildSector(s);
	Tick(s, 0.0f); // so a new ship is already in a consistent state: powered, manned, located
	return s;
}

// ---- modes, rank and the player ---------------------------------------------------------------------

float ClockRate(const Config &cfg) { return cfg.clockMode == CLOCK_ACCELERATED ? cfg.dayScale : 1.0f; }

bool SavesAllowed(const Config &cfg) { return cfg.mode == MODE_HOLODECK; }

static void Advance(Ship &s, double shipSeconds);

void CatchUp(Ship &s, double realSecondsAway)
{
	if (s.cfg.clockMode != CLOCK_WALL || !(realSecondsAway > 0.0)) return;
	Advance(s, std::min(realSecondsAway, static_cast<double>(MAX_CATCH_UP_DAYS) * SECONDS_PER_DAY));
}

bool MayOperate(const CrewMember &who, Station st)
{
	if (who.status != CREW_FIT) return false;
	if (who.rank >= 4) return true;
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

bool OrderRepairFirst(Ship &s, int system)
{
	if (!PlayerMayCommand(s)) return false;
	s.orderRepairFirst = system >= 0 && system < SYS_COUNT ? system : -1;
	return true;
}

bool OrderSecurityTo(Ship &s, int deck)
{
	if (!PlayerMayCommand(s)) return false;
	s.orderSecurityTo = deck >= 1 && deck <= DECKS ? deck : 0;
	return true;
}

bool OrderEvacuate(Ship &s, int deck)
{
	if (!PlayerMayCommand(s)) return false;
	s.orderEvacuate = deck >= 1 && deck <= DECKS ? deck : 0;
	return true;
}

bool OrderTriage(Ship &s, int policy)
{
	if (!PlayerMayCommand(s)) return false;
	s.orderTriage = policy == 1 ? 1 : 0;
	return true;
}

void SetRole(Ship &s, PlayerRole role)
{
	s.cfg.role = role;
	if (role != ROLE_MUNRO) return;
	for (size_t i = 0; i < s.crew.size(); ++i)
		if (s.crew[i].type == "munro") s.player = static_cast<int>(i);
}

void Tick(Ship &s, float seconds)
{
	if (!(seconds >= 0.0f)) return;
	Advance(s, static_cast<double>(seconds) * ClockRate(s.cfg));
}

static void Advance(Ship &s, double shipSecondsTotal)
{
	// Long steps are cut up, so a paused or fast-forwarded ship arrives where a played one would.
	double shipSeconds = shipSecondsTotal;
	do {
		const float step = static_cast<float>(std::min(shipSeconds, 60.0));
		s.clock += step;
		UpdateCrew(s, step);
		UpdateIntruders(s, step);
		UpdatePower(s, step);
		UpdateOutside(s, step);
		UpdateDecks(s, step);
		shipSeconds -= step;
	} while (shipSeconds > 0.0);
}

void SetAlert(Ship &s, Alert a) { s.alert = a; }

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

void Board(Ship &s, int deck, int boarders)
{
	if (deck >= 1 && deck <= DECKS && boarders > 0) s.decks[deck - 1].intruders += boarders;
}

void BoardBorg(Ship &s, int deck, int drones)
{
	if (deck < 1 || deck > DECKS || drones <= 0) return;
	s.decks[deck - 1].intruders += drones;
	s.decks[deck - 1].borg = true;
}

bool DeckAssimilated(const Ship &s, int deck)
{
	return deck >= 1 && deck <= DECKS && s.decks[deck - 1].assimilated >= ASSIMILATED;
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
	if (id < SYS_COUNT && amount > 0.0f) s.systems[id].health = Clamp01(s.systems[id].health - amount);
}

void DamageSource(Ship &s, SourceId id, float amount)
{
	if (id < SRC_COUNT && amount > 0.0f) s.sources[id].health = Clamp01(s.sources[id].health - amount);
}

void BreachDeck(Ship &s, int deck, float amount)
{
	if (deck >= 1 && deck <= DECKS && amount > 0.0f) s.decks[deck - 1].hull = Clamp01(s.decks[deck - 1].hull - amount);
}

void Repair(Ship &s, SystemId id, float amount)
{
	if (id < SYS_COUNT && amount > 0.0f) s.systems[id].health = Clamp01(s.systems[id].health + amount);
}

void RepairDeck(Ship &s, int deck, float amount)
{
	if (deck >= 1 && deck <= DECKS && amount > 0.0f) s.decks[deck - 1].hull = Clamp01(s.decks[deck - 1].hull + amount);
}

void SetForceField(Ship &s, int deck, bool on)
{
	if (deck >= 1 && deck <= DECKS) s.decks[deck - 1].forceField = on;
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

// The power clock: how long until the first source supplying now runs out. -1 if nothing supplies.
float MinutesToDark(const Ship &s)
{
	float best = -1.0f;
	for (int i = 0; i < SRC_COUNT; ++i) {
		const Source &src = s.sources[i];
		const SourceSpec &sp = SOURCES[i];
		if (!src.online || src.health <= 0.0f || src.output <= 0 || sp.capacity <= 0) continue;
		const float load = static_cast<float>(src.output) / sp.capacity;
		float minutes;
		if (i == SRC_BATTERIES) {
			minutes = s.stores.batteries / load * BATTERY_HOURS * 60.0f;
		} else {
			float days = 1.0e9f;
			if (sp.deuteriumPerDay > 0.0f) days = std::min(days, s.stores.deuterium / (sp.deuteriumPerDay * load));
			if (sp.antimatterPerDay > 0.0f) days = std::min(days, s.stores.antimatter / (sp.antimatterPerDay * load));
			minutes = days * 24.0f * 60.0f;
		}
		if (minutes >= 0.0f && (best < 0.0f || minutes < best)) best = minutes;
	}
	return best;
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
	for (const Deck &d : s.decks) { w.F(d.atmosphere); w.F(d.hull); w.F(d.intruders); w.U8(d.borg); w.F(d.assimilated); w.U8(d.forceField ? 1 : 0); }
	w.F(s.stores.deuterium); w.F(s.stores.antimatter); w.F(s.stores.batteries);
	w.U16(static_cast<uint16_t>(s.stores.torpedoes));
	w.F(s.stores.spareParts); w.F(s.stores.medicalSupplies);
	// The sector's shape comes back from the seed; where the ship is in it, and what it has met, is stored.
	w.F(s.shieldStrength);
	w.U8(static_cast<uint8_t>(s.beacon));
	w.U32(s.hits);
	uint32_t visited = 0;
	for (size_t i = 0; i < s.sector.size() && i < 32; ++i)
		if (s.sector[i].visited) visited |= 1u << i;
	w.U32(visited);
	w.U8(s.enemy.present); w.U8(s.enemy.borg); w.F(s.enemy.hull); w.F(s.enemy.shields); w.F(s.enemy.firepower);
	w.U8(static_cast<uint8_t>(s.enemy.boarders));
	// Names, types, departments and stations come back from the seed; only what changes is stored.
	for (const CrewMember &c : s.crew) { w.U8(c.status); w.F(c.fatigue); w.F(c.morale); w.U8(c.watch); w.U8(c.post); w.F(c.exposure); w.F(c.recovery); w.F(c.wounds); w.F(c.severity); }
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
	BuildSector(s);
	if (count != s.crew.size()) return false;
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
	for (Deck &d : s.decks) { d.atmosphere = r.Unit(); d.hull = r.Unit(); d.intruders = r.F(); if (!(d.intruders >= 0.0f && d.intruders <= 10000.0f)) return false; d.borg = r.U8() != 0; d.assimilated = r.Unit(); d.forceField = r.U8() != 0; }
	s.stores.deuterium = r.Unit(); s.stores.antimatter = r.Unit(); s.stores.batteries = r.Unit();
	s.stores.torpedoes = r.U16();
	s.stores.spareParts = r.F(); s.stores.medicalSupplies = r.F();
	if (!(s.stores.spareParts >= 0.0f && s.stores.spareParts <= 100000.0f)) return false;
	if (!(s.stores.medicalSupplies >= 0.0f && s.stores.medicalSupplies <= 100000.0f)) return false;
	s.shieldStrength = r.Unit();
	s.beacon = r.U8();
	if (s.beacon >= static_cast<int>(s.sector.size())) return false;
	s.hits = r.U32();
	const uint32_t visited = r.U32();
	for (size_t i = 0; i < s.sector.size() && i < 32; ++i) s.sector[i].visited = (visited >> i) & 1u;
	s.enemy.present = r.U8() != 0; s.enemy.borg = r.U8() != 0;
	s.enemy.hull = r.Unit(); s.enemy.shields = r.Unit(); s.enemy.firepower = r.F();
	if (!(s.enemy.firepower >= 0.0f && s.enemy.firepower <= 100.0f)) return false;
	s.enemy.boarders = r.U8();
	for (CrewMember &c : s.crew) {
		c.status = r.U8();
		c.fatigue = r.Unit();
		c.morale = r.Unit();
		c.watch = r.U8();
		c.post = r.U8();
		c.exposure = r.F();
		c.recovery = r.Unit();
		c.wounds = r.Unit();
		c.severity = r.Unit();
		if (!(c.exposure >= 0.0f && c.exposure <= 1.0e6f)) return false;
		if (c.status > CREW_ASSIMILATED || c.watch >= WATCHES || c.post > SYS_COUNT) return false;
	}
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
	std::snprintf(line, sizeof(line), "power %d supplied, %d allocated  deuterium %.1f%%  antimatter %.1f%%  batteries %.0f%%  torpedoes %d  parts %.0f\n",
		s.PowerAvailable(), s.PowerAllocated(), s.stores.deuterium * 100, s.stores.antimatter * 100, s.stores.batteries * 100, s.stores.torpedoes,
		s.stores.spareParts);
	out += line;
	static const char *const KINDS[] = {"empty space", "a hostile ship", "a derelict", "the Borg"};
	std::snprintf(line, sizeof(line), "  beacon %d of %d (%s)  shields at %.0f%%\n", s.beacon, static_cast<int>(s.sector.size()) - 1,
		s.sector.empty() ? "?" : KINDS[s.sector[s.beacon].kind], s.shieldStrength * 100);
	out += line;
	if (s.enemy.present) {
		std::snprintf(line, sizeof(line), "  %s: hull %.0f%%  shields %.0f%%%s\n", s.enemy.borg ? "BORG VESSEL" : "HOSTILE VESSEL",
			s.enemy.hull * 100, s.enemy.shields * 100, s.enemy.hull <= 0.0f ? "  DESTROYED" : "");
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
