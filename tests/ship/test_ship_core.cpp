// Tests for module/ship/ship_core: gate S1's exit criteria, one function per property.
// `test_ship_core --day` instead prints a simulated day, hour by hour, as evidence.

#include "ship_core.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>

using namespace ship;

static int g_failures = 0;
static const char *g_test = "";

#define CHECK(cond) \
	do { \
		if (!(cond)) { \
			std::printf("FAIL  %s: %s (line %d)\n", g_test, #cond, __LINE__); \
			++g_failures; \
		} \
	} while (0)

// Simulated seconds that advance the ship by the given ship-hours.
static float Hours(const Ship &s, float h) { return h * 3600.0f / s.cfg.dayScale; }

static void TestNominalShip()
{
	g_test = "a new ship is whole and consistent";
	const Ship s = NewShip();
	CHECK(static_cast<int>(s.crew.size()) == COMPLEMENT);
	CHECK(s.CrewFit() == COMPLEMENT);
	CHECK(s.Watch() == 0 && s.SecondOfDay() == 8 * 3600);
	CHECK(s.PowerAllocated() == s.PowerAvailable());
	// Condition green: everything but weapons and shields runs at full output.
	for (int i = 0; i < SYS_COUNT; ++i) {
		const bool standDown = i == SYS_SHIELDS || i == SYS_PHASERS || i == SYS_TORPEDO_LAUNCHERS;
		if (standDown) CHECK(s.systems[i].output == 0.0f);
		else CHECK(s.systems[i].output == 1.0f);
	}
	CHECK(s.sources[SRC_BATTERIES].output == 0); // the cells are for emergencies
	for (const Deck &d : s.decks) CHECK(d.atmosphere == 1.0f && d.hull == 1.0f);
}

static void TestPowerIsConserved()
{
	g_test = "power is conserved";
	Ship s = NewShip();
	for (int step = 0; step < 6; ++step) {
		if (step == 1) SetAlert(s, ALERT_RED);
		if (step == 2) DamageSource(s, SRC_WARP_CORE, 0.6f);
		if (step == 3) SetSourceOnline(s, SRC_IMPULSE_REACTORS, false);
		if (step == 4) DamageSystem(s, SYS_SHIELDS, 0.5f);
		if (step == 5) SetSourceOnline(s, SRC_WARP_CORE, false);
		Tick(s, 1.0f);
		CHECK(s.PowerAllocated() <= s.PowerAvailable());
		for (int i = 0; i < SYS_COUNT; ++i) {
			CHECK(s.systems[i].allocated >= 0 && s.systems[i].allocated <= Spec(static_cast<SystemId>(i)).demand);
			CHECK(s.systems[i].output >= 0.0f && s.systems[i].output <= s.systems[i].health);
		}
		for (int i = 0; i < SRC_COUNT; ++i) CHECK(s.sources[i].output >= 0);
	}
}

// Battle stations cannot be fully powered: the demand exceeds the supply, by design, and what is
// shed is whatever stands last in the priority order.
static void TestSheddingFollowsPriority()
{
	g_test = "shedding follows priority";
	Ship s = NewShip();
	SetAlert(s, ALERT_RED);
	Tick(s, 1.0f);
	CHECK(s.systems[SYS_LIFE_SUPPORT].output == 1.0f);
	CHECK(s.systems[SYS_SHIELDS].allocated == Spec(SYS_SHIELDS).demand);
	CHECK(s.systems[SYS_HOLODECKS].allocated == 0 && s.systems[SYS_REPLICATORS].allocated == 0);

	DamageSource(s, SRC_WARP_CORE, 1.0f); // the core is gone: 420 units for a ship that wants 1400
	Tick(s, 1.0f);
	CHECK(s.systems[SYS_LIFE_SUPPORT].output == 1.0f);
	CHECK(s.systems[SYS_STRUCTURAL_INTEGRITY].output == 1.0f);
	CHECK(s.systems[SYS_WARP_DRIVE].allocated == 0);
	bool starved = false;
	for (int p = 0; p < SYS_COUNT; ++p)
		for (int i = 0; i < SYS_COUNT; ++i) {
			if (s.systems[i].priority != p) continue;
			const bool full = s.systems[i].allocated == Spec(static_cast<SystemId>(i)).demand || !s.systems[i].enabled;
			const bool wants = Spec(static_cast<SystemId>(i)).demand > 0 && i != SYS_HOLODECKS && i != SYS_REPLICATORS;
			if (starved && wants) CHECK(s.systems[i].allocated == 0); // once power runs out, nothing later gets any
			if (wants && !full && s.systems[i].allocated < Spec(static_cast<SystemId>(i)).demand) starved = true;
		}
	CHECK(starved);

	// A console can reorder the list: put the warp drive first and it is fed before the shields.
	SetPriority(s, SYS_WARP_DRIVE, -1);
	Tick(s, 1.0f);
	CHECK(s.systems[SYS_WARP_DRIVE].allocated == Spec(SYS_WARP_DRIVE).demand);
	CHECK(s.systems[SYS_SHIELDS].allocated == 0);
}

static void TestBatteriesKeepTheCrewAlive()
{
	g_test = "batteries carry the critical systems, then run out";
	Ship s = NewShip();
	for (int i = 0; i < SRC_BATTERIES; ++i) SetSourceOnline(s, static_cast<SourceId>(i), false);
	Tick(s, 1.0f);
	CHECK(s.sources[SRC_BATTERIES].output == 80);
	CHECK(s.systems[SYS_LIFE_SUPPORT].output == 1.0f);          // 60 of the 80
	CHECK(s.systems[SYS_STRUCTURAL_INTEGRITY].allocated == 20); // what is left; nothing for anyone else
	CHECK(s.systems[SYS_COMPUTER_CORE].allocated == 0 && s.systems[SYS_SENSORS].allocated == 0);

	Tick(s, Hours(s, 2.0f));
	CHECK(s.stores.batteries > 0.25f && s.stores.batteries < 0.40f); // three hours of charge, two spent
	Tick(s, Hours(s, 1.5f));
	CHECK(s.stores.batteries == 0.0f);
	CHECK(s.PowerAvailable() == 0 && s.systems[SYS_LIFE_SUPPORT].output == 0.0f);

	// With no life support the air goes stale, slowly; it does not vanish.
	const float before = s.decks[4].atmosphere;
	Tick(s, Hours(s, 3.0f));
	CHECK(s.decks[4].atmosphere < before && s.decks[4].atmosphere > 0.5f);

	// Bring a reactor back and the ship recovers by itself.
	SetSourceOnline(s, SRC_AUXILIARY, true);
	Tick(s, Hours(s, 2.0f));
	CHECK(s.systems[SYS_LIFE_SUPPORT].output == 1.0f);
	CHECK(s.decks[4].atmosphere == 1.0f);
}

static void TestHullBreach()
{
	g_test = "a breached deck vents, and only that deck";
	Ship s = NewShip();
	BreachDeck(s, 9, 1.0f);
	Tick(s, Hours(s, 0.05f)); // three minutes
	CHECK(s.decks[8].atmosphere < 0.6f && s.decks[8].atmosphere > 0.2f);
	Tick(s, Hours(s, 0.1f));
	CHECK(s.decks[8].atmosphere == 0.0f); // life support cannot fill a compartment open to space
	CHECK(s.decks[7].atmosphere == 1.0f && s.decks[9].atmosphere == 1.0f);
	BreachDeck(s, 0, 1.0f);  // out of range: ignored
	BreachDeck(s, 16, 1.0f);
	Tick(s, 1.0f);
	for (int d = 0; d < DECKS; ++d)
		if (d != 8) CHECK(s.decks[d].hull == 1.0f);
}

static void TestFuel()
{
	g_test = "fuel is spent by what runs, and an empty tank stops the reactors";
	Ship s = NewShip();
	Tick(s, Hours(s, 24.0f));
	CHECK(s.stores.deuterium < 1.0f && s.stores.deuterium > 0.98f);
	CHECK(s.stores.antimatter < 1.0f && s.stores.antimatter > 0.99f);

	Ship idle = NewShip();
	SetEnabled(idle, SYS_WARP_DRIVE, false); // a ship at rest burns less than one under way
	Tick(idle, Hours(idle, 24.0f));
	CHECK(idle.stores.antimatter > s.stores.antimatter);

	s.stores.antimatter = 0.0f; // the core needs both; the fusion reactors only deuterium
	Tick(s, 1.0f);
	CHECK(s.sources[SRC_WARP_CORE].output == 0);
	CHECK(s.sources[SRC_IMPULSE_REACTORS].output > 0);
	s.stores.deuterium = 0.0f;
	Tick(s, 1.0f);
	CHECK(s.sources[SRC_IMPULSE_REACTORS].output == 0 && s.sources[SRC_AUXILIARY].output == 0);
	CHECK(s.sources[SRC_BATTERIES].output > 0);
}

static void TestRoster()
{
	g_test = "the roster";
	const Ship s = NewShip();
	std::set<std::string> names;
	int perWatch[WATCHES] = {0, 0, 0};
	for (const CrewMember &c : s.crew) {
		CHECK(!c.name.empty() && !c.type.empty());
		names.insert(c.name);
		CHECK(c.watch < WATCHES && c.dept < DEPT_COUNT);
		CHECK(c.quartersDeck >= 1 && c.quartersDeck <= DECKS);
		++perWatch[c.watch];
	}
	CHECK(names.size() == s.crew.size()); // everyone is someone
	CHECK(s.crew[0].name == "Kathryn Janeway" && s.crew[0].rank == 6 && s.crew[0].post == SYS_COUNT);
	for (int w = 0; w < WATCHES; ++w) CHECK(perWatch[w] >= 35);

	// Every station that needs hands has them on every watch.
	for (int id = 0; id < SYS_COUNT; ++id)
		for (int w = 0; w < WATCHES; ++w) {
			int n = 0;
			for (const CrewMember &c : s.crew)
				if (c.post == id && c.watch == w) ++n;
			CHECK(n >= Spec(static_cast<SystemId>(id)).crewNeeded);
		}

	// The same seed is the same crew; another seed is another.
	const Ship again = NewShip();
	Config other;
	other.seed = 99;
	const Ship different = NewShip(other);
	bool differs = false;
	for (size_t i = 0; i < s.crew.size(); ++i) {
		CHECK(again.crew[i].type == s.crew[i].type && again.crew[i].quartersDeck == s.crew[i].quartersDeck);
		if (different.crew[i].type != s.crew[i].type || different.crew[i].quartersDeck != s.crew[i].quartersDeck) differs = true;
	}
	CHECK(differs);
}

static void TestSchedule()
{
	g_test = "the daily routine";
	// Each watch: eight hours' duty, eight hours' sleep, and the rest lived; exactly one watch on duty at any hour.
	for (int w = 0; w < WATCHES; ++w) {
		int hours[ACT_COUNT] = {0, 0, 0, 0, 0};
		for (int h = 0; h < 24; ++h) ++hours[ScheduledActivity(w, h * 3600 + 1800)];
		CHECK(hours[ACT_ON_DUTY] == 8 && hours[ACT_SLEEP] == 8);
		CHECK(hours[ACT_MEAL] == 2 && hours[ACT_RECREATION] == 3 && hours[ACT_PERSONAL] == 3);
	}
	for (int h = 0; h < 24; ++h) {
		int onDuty = 0;
		for (int w = 0; w < WATCHES; ++w)
			if (ScheduledActivity(w, h * 3600) == ACT_ON_DUTY) ++onDuty;
		CHECK(onDuty == 1);
	}
	CHECK(ScheduledActivity(0, 8 * 3600) == ACT_ON_DUTY && ScheduledActivity(0, 7 * 3600 + 3599) == ACT_MEAL);
	CHECK(ScheduledActivity(2, 0) == ACT_ON_DUTY && ScheduledActivity(1, 23 * 3600) == ACT_ON_DUTY);
}

static void TestADayAboard()
{
	g_test = "a day aboard";
	Ship s = NewShip();
	int minOutput100 = 0;
	for (int h = 0; h < 48; ++h) {
		Tick(s, Hours(s, 1.0f));
		const int watch = s.Watch();
		int onDuty = 0, mess = 0, asleep = 0;
		for (const CrewMember &c : s.crew) {
			CHECK(c.deck >= 1 && c.deck <= DECKS);
			CHECK(c.fatigue >= 0.0f && c.fatigue <= 1.0f);
			if (c.activity == ACT_ON_DUTY) { ++onDuty; CHECK(c.watch == watch); }
			if (c.activity == ACT_MEAL) { ++mess; CHECK(c.deck == 2); }
			if (c.activity == ACT_SLEEP) { ++asleep; CHECK(c.deck == c.quartersDeck); }
		}
		CHECK(onDuty >= 35 && asleep >= 35);
		// Through two full days, every watch change included, no station is ever short-handed.
		for (int i = 0; i < SYS_COUNT; ++i)
			CHECK(s.systems[i].manned >= Spec(static_cast<SystemId>(i)).crewNeeded);
		if (s.systems[SYS_WARP_DRIVE].output == 1.0f) ++minOutput100;
	}
	CHECK(minOutput100 == 48);
	CHECK(s.Day() == 2 && s.SecondOfDay() == 8 * 3600);
	// A routine with sleep in it is sustainable: nobody ends two days exhausted.
	for (const CrewMember &c : s.crew) CHECK(c.fatigue < 0.6f);
}

static void TestRedAlertAndCasualties()
{
	g_test = "red alert is all hands; the dead stand no watch";
	Ship s = NewShip();
	Tick(s, 1.0f);
	const int normal = s.systems[SYS_WARP_DRIVE].manned;
	SetAlert(s, ALERT_RED);
	Tick(s, 1.0f);
	CHECK(s.systems[SYS_WARP_DRIVE].manned > normal);
	for (const CrewMember &c : s.crew)
		if (ScheduledActivity(c.watch, s.SecondOfDay()) == ACT_SLEEP) CHECK(c.activity == ACT_SLEEP);

	// Lose the alpha-watch engineers at the warp drive: the station is short and the drive shows it.
	SetAlert(s, ALERT_GREEN);
	for (CrewMember &c : s.crew)
		if (c.post == SYS_WARP_DRIVE && c.watch == 0) c.status = CREW_DEAD;
	Tick(s, 1.0f);
	CHECK(s.systems[SYS_WARP_DRIVE].manned == 0);
	CHECK(s.systems[SYS_WARP_DRIVE].output == 0.5f); // automation holds it at half
	CHECK(s.CrewFit() < COMPLEMENT);
}

static void TestDamageAndRepair()
{
	g_test = "damage caps output; repair restores it";
	Ship s = NewShip();
	s.stores.spareParts = 0.0f; // nothing to mend with: damage stays exactly where it is put
	DamageSystem(s, SYS_SENSORS, 0.4f);
	Tick(s, 1.0f);
	CHECK(std::fabs(s.systems[SYS_SENSORS].output - 0.6f) < 1e-5f);
	DamageSystem(s, SYS_SENSORS, 5.0f);
	Tick(s, 1.0f);
	CHECK(s.systems[SYS_SENSORS].health == 0.0f && s.systems[SYS_SENSORS].allocated == 0); // a wreck draws nothing
	Repair(s, SYS_SENSORS, 0.25f);
	DamageSystem(s, SYS_SENSORS, -1.0f); // negative damage is not a repair
	Tick(s, 1.0f);
	CHECK(std::fabs(s.systems[SYS_SENSORS].output - 0.25f) < 1e-5f);
	SetEnabled(s, SYS_SENSORS, false);
	Tick(s, 1.0f);
	CHECK(s.systems[SYS_SENSORS].output == 0.0f);
}

static void TestMoraleAndFatigue()
{
	g_test = "a spent or disillusioned post is worse; rest restores it, and morale saves";
	Ship s = NewShip();
	Tick(s, 1.0f);
	CHECK(s.systems[SYS_SENSORS].output == 1.0f); // a fresh, rested watch is a whole hand

	// The same hands, spent and gone through the motions, deliver much less.
	for (CrewMember &c : s.crew)
		if (c.post == SYS_SENSORS) { c.fatigue = 1.0f; c.morale = 0.2f; }
	Tick(s, 0.0f);
	const float spent = s.systems[SYS_SENSORS].output;
	CHECK(spent < 1.0f);
	CHECK(spent >= 0.24f);                         // tired and flagging is not the same as absent
	CHECK(s.systems[SYS_SENSORS].manned >= Spec(SYS_SENSORS).crewNeeded); // the hands are there

	// Rested and willing again: the post is a whole hand once more.
	for (CrewMember &c : s.crew)
		if (c.post == SYS_SENSORS) { c.fatigue = 0.0f; c.morale = 1.0f; }
	Tick(s, 0.0f);
	CHECK(s.systems[SYS_SENSORS].output == 1.0f);

	// The drivers move: a watch raises fatigue, and green lifts the mood.
	Ship t = NewShip();
	t.crew[0].watch = 0; t.crew[0].post = SYS_SENSORS; t.crew[0].dept = DEPT_SCIENCES;
	t.crew[0].fatigue = 0.0f; t.crew[0].morale = 0.5f;
	Tick(t, Hours(t, 8.0f));                       // 0800 to 1600, alpha watch on duty throughout
	CHECK(t.crew[0].fatigue > 0.0f);
	CHECK(t.crew[0].morale > 0.5f);

	// Morale survives a save and a load.
	for (CrewMember &c : t.crew) c.morale = 0.42f;
	std::vector<uint8_t> blob = Pack(t);
	Ship u = NewShip();
	CHECK(Unpack(blob.data(), blob.size(), u));
	CHECK(std::fabs(u.crew[0].morale - 0.42f) < 1e-4f);
}

static void TestDeterminismAndStepSize()
{
	g_test = "deterministic, and independent of how time is sliced";
	Ship a = NewShip(), b = NewShip();
	SetAlert(a, ALERT_YELLOW); SetAlert(b, ALERT_YELLOW);
	DamageSource(a, SRC_WARP_CORE, 0.5f); DamageSource(b, SRC_WARP_CORE, 0.5f);
	BreachDeck(a, 3, 0.2f); BreachDeck(b, 3, 0.2f);
	Tick(a, Hours(a, 6.0f));                              // one long step
	for (int i = 0; i < 360; ++i) Tick(b, Hours(b, 1.0f / 60.0f)); // a minute at a time
	CHECK(std::fabs(a.clock - b.clock) < 1.0);
	CHECK(std::fabs(a.stores.deuterium - b.stores.deuterium) < 1e-4f);
	CHECK(std::fabs(a.decks[2].atmosphere - b.decks[2].atmosphere) < 1e-3f);
	for (int i = 0; i < SYS_COUNT; ++i) CHECK(a.systems[i].allocated == b.systems[i].allocated);
	for (size_t i = 0; i < a.crew.size(); ++i) {
		CHECK(a.crew[i].activity == b.crew[i].activity && a.crew[i].deck == b.crew[i].deck);
		CHECK(std::fabs(a.crew[i].fatigue - b.crew[i].fatigue) < 1e-3f);
	}
	Tick(a, -5.0f); // time does not run backwards
	CHECK(std::fabs(a.clock - b.clock) < 1.0);
}

static void TestSave()
{
	g_test = "the save is the ship";
	Ship s = NewShip();
	SetAlert(s, ALERT_RED);
	DamageSystem(s, SYS_SHIELDS, 0.3f);
	DamageSource(s, SRC_WARP_CORE, 0.2f);
	SetSourceOnline(s, SRC_AUXILIARY, false);
	SetPriority(s, SYS_WARP_DRIVE, -3);
	SetEnabled(s, SYS_TRACTOR_BEAM, false);
	BreachDeck(s, 7, 0.5f);
	s.stores.torpedoes = 31;
	s.crew[40].status = CREW_DEAD;
	s.crew[41].status = CREW_ASSIMILATED;
	s.crew[42].status = CREW_INJURED;
	s.crew[42].recovery = 0.4f;
	s.stores.spareParts = 37.5f;
	Tick(s, Hours(s, 5.5f));

	const std::vector<uint8_t> blob = Pack(s);
	CHECK(blob.size() < 8192); // the whole ship and its 141 crew
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(Pack(back) == blob);
	CHECK(Describe(back) == Describe(s));
	for (size_t i = 0; i < s.crew.size(); ++i) {
		CHECK(back.crew[i].name == s.crew[i].name && back.crew[i].status == s.crew[i].status);
		CHECK(back.crew[i].deck == s.crew[i].deck && back.crew[i].activity == s.crew[i].activity);
	}
	// And it carries on identically from there.
	Tick(s, Hours(s, 3.0f));
	Tick(back, Hours(back, 3.0f));
	CHECK(Describe(back) == Describe(s));

	Ship untouched = NewShip();
	const std::string before = Describe(untouched);
	CHECK(!Unpack(nullptr, 0, untouched));
	CHECK(!Unpack(blob.data(), blob.size() - 1, untouched));
	std::vector<uint8_t> longer = blob;
	longer.push_back(0);
	CHECK(!Unpack(longer.data(), longer.size(), untouched));
	std::vector<uint8_t> bad = blob;
	bad[0] ^= 0xff;
	CHECK(!Unpack(bad.data(), bad.size(), untouched));
	bad = blob; bad[4] = 200; // a newer version
	CHECK(!Unpack(bad.data(), bad.size(), untouched));
	bad = blob; bad[6] = 7; // a different complement
	CHECK(!Unpack(bad.data(), bad.size(), untouched));
	bad = blob; bad[43] = 5; // an alert condition that does not exist
	CHECK(!Unpack(bad.data(), bad.size(), untouched));
	bad = blob; bad[45] = 0x7f; // a health that is not a fraction
	CHECK(!Unpack(bad.data(), bad.size(), untouched));
	bad = blob; bad[16] = 9; // a play mode that does not exist
	CHECK(!Unpack(bad.data(), bad.size(), untouched));
	CHECK(Describe(untouched) == before);
}

// S6. Nothing repairs itself: it takes engineers, time and parts.
static void TestDamageControl()
{
	g_test = "damage control";
	Ship s = NewShip();
	DamageSystem(s, SYS_SENSORS, 0.5f);
	Tick(s, 1.0f);
	CHECK(s.systems[SYS_SENSORS].repairing == REPAIR_TEAM_MAX); // a party goes to it, and no more than a party
	CHECK(s.systems[SYS_WARP_DRIVE].repairing == 0);            // nobody is sent to mend what is whole
	const float parts = s.stores.spareParts;
	Tick(s, Hours(s, 0.5f));
	CHECK(s.systems[SYS_SENSORS].health > 0.7f && s.systems[SYS_SENSORS].health < 0.8f); // 3 engineers, half an hour: a quarter
	CHECK(std::fabs((parts - s.stores.spareParts) - 0.25f * PARTS_PER_SYSTEM) < 0.2f);     // and paid for
	Tick(s, Hours(s, 1.0f));
	CHECK(s.systems[SYS_SENSORS].health == 1.0f);
	const float after = s.stores.spareParts;
	Tick(s, Hours(s, 1.0f));
	CHECK(s.stores.spareParts == after && s.systems[SYS_SENSORS].repairing == 0); // done: the party stands down

	// Two systems down: the critical one gets hands first, and both are worked.
	DamageSystem(s, SYS_HOLODECKS, 0.9f);
	DamageSystem(s, SYS_LIFE_SUPPORT, 0.9f);
	Tick(s, Hours(s, 0.25f));
	CHECK(s.systems[SYS_LIFE_SUPPORT].repairing == REPAIR_TEAM_MAX && s.systems[SYS_HOLODECKS].repairing > 0);
	CHECK(s.systems[SYS_LIFE_SUPPORT].health >= s.systems[SYS_HOLODECKS].health);

	// No parts, no repair -- however many engineers stand over it.
	Ship poor = NewShip();
	poor.stores.spareParts = 1.2f; // a tenth of a system's worth
	DamageSystem(poor, SYS_SHIELDS, 1.0f);
	Tick(poor, Hours(poor, 12.0f));
	CHECK(poor.stores.spareParts == 0.0f);
	CHECK(std::fabs(poor.systems[SYS_SHIELDS].health - 0.1f) < 0.01f);

	// No engineers to spare, no repair: with the off-station engineers of every watch dead, damage stays.
	Ship thin = NewShip();
	for (CrewMember &c : thin.crew)
		if (c.dept == DEPT_ENGINEERING && c.post == SYS_COUNT) c.status = CREW_DEAD;
	DamageSystem(thin, SYS_SENSORS, 0.5f);
	Tick(thin, Hours(thin, 6.0f));
	CHECK(thin.systems[SYS_SENSORS].health == 0.5f && thin.stores.spareParts == 100.0f);
}

static void TestCasualties()
{
	g_test = "casualties";
	Ship s = NewShip();
	Tick(s, 1.0f);
	const int onEleven = static_cast<int>(CrewOnDeck(s, 11).size());
	CHECK(onEleven >= 5);
	BreachDeck(s, 11, 1.0f); // Main Engineering opens to space
	Tick(s, Hours(s, 0.08f)); // five minutes: the air goes
	Tick(s, Hours(s, 0.03f)); // and two more without it
	int injured = 0, dead = 0;
	for (const CrewMember &c : s.crew) {
		if (c.status == CREW_INJURED) ++injured;
		if (c.status == CREW_DEAD) ++dead;
	}
	CHECK(injured >= onEleven && dead == 0); // hurt within a minute, not yet lost
	CHECK(s.systems[SYS_WARP_DRIVE].manned == 0); // the injured stand no station
	for (const CrewMember &c : s.crew)
		if (c.status == CREW_FIT) CHECK(c.exposure == 0.0f || c.deck == 11);

	// Sickbay takes three standard cases and one surgical at a time; the rest wait and worsen.
	int inSickbay = 0;
	for (const CrewMember &c : s.crew)
		if (c.status == CREW_INJURED && c.underCare) ++inSickbay;
	CHECK(inSickbay == (injured < SICKBAY_BEDS ? injured : SICKBAY_BEDS));

	// While the deck is open, mending people only sends them back to be hurt again.
	Tick(s, Hours(s, 13.0f));
	int stillHurt = 0;
	for (const CrewMember &c : s.crew)
		if (c.status == CREW_INJURED) ++stillHurt;
	CHECK(stillHurt > 0);
	// Seal the hull, and half a day later sickbay has returned its first patients to duty for good.
	RepairDeck(s, 11, 1.0f);
	Tick(s, Hours(s, 14.0f));
	int hurtAfter = 0;
	for (const CrewMember &c : s.crew)
		if (c.status == CREW_INJURED) ++hurtAfter;
	CHECK(hurtAfter < stillHurt);
	CHECK(s.decks[10].atmosphere == 1.0f);

	// Someone hurt, with no bed free and quarters on a deck without air, has nowhere to go: they die.
	Ship held = NewShip();
	held.decks[0].atmosphere = 0.0f;
	held.decks[0].hull = 0.0f;
	for (int i = 0; i < SICKBAY_BEDS; ++i) held.crew[i].status = CREW_INJURED; // the beds are taken
	CrewMember &last = held.crew[COMPLEMENT - 1];
	last.status = CREW_INJURED;
	last.quartersDeck = 1;
	Tick(held, Hours(held, 0.05f)); // three minutes
	CHECK(last.status == CREW_INJURED && last.deck == 1);
	Tick(held, Hours(held, 0.05f)); // six
	CHECK(last.status == CREW_DEAD && last.deck == 0);
	CHECK(CrewOnDeck(held, 1).size() < CrewOnDeck(NewShip(), 1).size() + 1);
}

// The triage gap: more casualties than beds is a decision, not a queue that clears itself.
static void TestTriage()
{
	g_test = "more casualties than beds is a decision, not a queue";
	Ship s = NewShip();
	int made = 0;
	for (CrewMember &c : s.crew) {
		if (made >= 6) break;
		if (c.status != CREW_FIT) continue;
		c.status = CREW_INJURED;
		c.severity = 0.2f + 0.1f * made;   // 0.2 .. 0.7
		++made;
	}
	CHECK(made == 6);
	Tick(s, 0.0f);
	int care = 0;
	for (const CrewMember &c : s.crew) if (c.underCare) ++care;
	CHECK(care == SICKBAY_BEDS);           // exactly the beds, no more
	// Default triage is worst first: the beds hold the graver cases, the rest wait.
	float careMin = 1.0f, waitMax = 0.0f;
	for (const CrewMember &c : s.crew) {
		if (c.status != CREW_INJURED) continue;
		if (c.underCare) careMin = std::min(careMin, c.severity);
		else waitMax = std::max(waitMax, c.severity);
	}
	CHECK(careMin >= waitMax);

	// Rank first is the other choice: the senior cases get the beds, however hurt.
	s.orderTriage = 1;
	Tick(s, 0.0f);
	float careRank = 0.0f, waitRank = 999.0f;
	for (const CrewMember &c : s.crew) {
		if (c.status != CREW_INJURED) continue;
		if (c.underCare) careRank = std::max(careRank, static_cast<float>(c.rank));
		else waitRank = std::min(waitRank, static_cast<float>(c.rank));
	}
	CHECK(careRank >= waitRank);

	// Waiting without treatment kills: no supplies means the ward only holds, so the untreated die.
	Ship t = NewShip();
	t.stores.medicalSupplies = 0.0f;
	for (CrewMember &c : t.crew) { if (c.status != CREW_FIT) continue; c.status = CREW_INJURED; c.severity = 0.4f; }
	Tick(t, Hours(t, 30.0f));
	int dead = 0;
	for (const CrewMember &c : t.crew) if (c.status == CREW_DEAD) ++dead;
	CHECK(dead > 0);
	CHECK(t.stores.medicalSupplies == 0.0f);
}

// The air-and-endurance gap: a breach is a number, and the number moves when the crew act.
static void TestAirAndEndurance()
{
	g_test = "a breach has a number; seal it or hold the field and the number goes";
	Ship s = NewShip();
	CHECK(MinutesOfAir(s, 9) < 0.0f);            // intact and holding: no countdown
	BreachDeck(s, 9, 1.0f);
	const float toVacuum = MinutesOfAir(s, 9);
	CHECK(toVacuum > 0.0f && toVacuum < 60.0f);  // an open deck is minutes, not weeks
	// A force field over the breach holds the air: the countdown goes.
	SetForceField(s, 9, true);
	CHECK(MinutesOfAir(s, 9) < 0.0f);
	SetForceField(s, 9, false);
	// Sealing it is the other answer.
	RepairDeck(s, 9, 1.0f);
	CHECK(MinutesOfAir(s, 9) < 0.0f);
	CHECK(s.decks[8].hull == 1.0f);

	// The power clock: with the reactors off, the batteries are the ship's endurance.
	Ship t = NewShip();
	SetSourceOnline(t, SRC_WARP_CORE, false);
	SetSourceOnline(t, SRC_IMPULSE_REACTORS, false);
	SetSourceOnline(t, SRC_AUXILIARY, false);
	Tick(t, 1.0f);
	const float battery = MinutesToDark(t);
	CHECK(battery > 0.0f && battery <= 3.0f * 60.0f + 1.0f); // at most the cells' three hours
	SetSourceOnline(t, SRC_WARP_CORE, true);
	Tick(t, 1.0f);
	CHECK(MinutesToDark(t) > battery);           // a reactor back on stretches endurance
}

// The log gap: a run can be reconstructed from the log alone.
static void TestLog()
{
	g_test = "the log says what happened, when, and by whom";
	Ship s = NewShip();
	SetRole(s, ROLE_IN_COMMAND); // so orders may be given
	SetAlert(s, ALERT_RED);
	BreachDeck(s, 9, 1.0f);
	DamageSystem(s, SYS_SENSORS, 0.4f);
	OrderTriage(s, 1);
	Board(s, 9, 3);
	Tick(s, Hours(s, 0.1f));

	CHECK(!s.log.empty());
	double last = -1.0;
	bool red = false, breach = false, dmg = false, triage = false, boarders = false;
	for (const LogEntry &e : s.log) {
		CHECK(e.time >= last);                 // written in time order
		last = e.time;
		CHECK(!e.who.empty() && !e.scope.empty() && !e.what.empty());
		if (e.what.find("red") != std::string::npos) red = true;
		if (e.what.find("breached") != std::string::npos) breach = true;
		if (e.what.find("damaged") != std::string::npos) dmg = true;
		if (e.what.find("triage") != std::string::npos) triage = true;
		if (e.what.find("boarders") != std::string::npos) boarders = true;
	}
	CHECK(red && breach && dmg && triage && boarders);

	// The scope is a filter: the hull entries are the damage-control ones.
	int hull = 0;
	for (const LogEntry &e : s.log) if (e.scope == "hull") ++hull;
	CHECK(hull >= 2); // the breach and the boarding

	// The log survives a save and a load, in order.
	std::vector<uint8_t> blob = Pack(s);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(back.log.size() == s.log.size());
	CHECK(!back.log.empty() && back.log.back().what == s.log.back().what);
}

// S7. Boarders take systems; the crew take them back.
static void TestBoarding()
{
	g_test = "boarding and control";
	// Unopposed: no security left aboard. Two boarders on deck 8 work at the sensors.
	Ship s = NewShip();
	for (CrewMember &c : s.crew)
		if (c.dept == DEPT_SECURITY && c.post == SYS_COUNT) c.status = CREW_DEAD;
	Board(s, 8, 2);
	CHECK(Intruders(s) == 2);
	Tick(s, Hours(s, 2.0f / 60.0f)); // two minutes
	CHECK(s.systems[SYS_SENSORS].control < 1.0f && !Hijacked(s, SYS_SENSORS));
	CHECK(s.systems[SYS_WARP_DRIVE].control == 1.0f); // they are not on deck 11
	Tick(s, Hours(s, 3.0f / 60.0f));
	CHECK(Hijacked(s, SYS_SENSORS));
	CHECK(s.systems[SYS_SENSORS].output == 0.0f && s.systems[SYS_SENSORS].allocated > 0); // it runs, but not for us

	// A hijacked system refuses its console.
	SetEnabled(s, SYS_SENSORS, false);
	SetPriority(s, SYS_SENSORS, -9);
	CHECK(s.systems[SYS_SENSORS].enabled && s.systems[SYS_SENSORS].priority == Spec(SYS_SENSORS).priority);

	// A counter-hack takes it back for a while -- and with the boarders still there, they take it again.
	CounterHack(s, SYS_SENSORS, 1.0f);
	CHECK(!Hijacked(s, SYS_SENSORS));
	Tick(s, Hours(s, 5.0f / 60.0f));
	CHECK(Hijacked(s, SYS_SENSORS));

	// Cutting its power denies it to both sides: control stops moving.
	CounterHack(s, SYS_SENSORS, 1.0f);
	SetEnabled(s, SYS_SENSORS, false);
	const float frozen = s.systems[SYS_SENSORS].control;
	Tick(s, Hours(s, 10.0f / 60.0f));
	CHECK(s.systems[SYS_SENSORS].control == frozen);

	// Security answers a boarding, wears it down, and is hurt doing it. Then the crew win the system back.
	Ship d = NewShip();
	Board(d, 8, 2);
	Tick(d, 1.0f);
	CHECK(d.decks[7].defenders >= 3); // enough to outnumber them
	int secOnEight = 0;
	for (const CrewMember &c : d.crew)
		if (c.dept == DEPT_SECURITY && c.deck == 8) ++secOnEight;
	CHECK(secOnEight >= d.decks[7].defenders); // they went there (others may have quarters on the deck)
	Tick(d, Hours(d, 6.0f / 60.0f));
	CHECK(Intruders(d) == 0);
	CHECK(!Hijacked(d, SYS_SENSORS)); // pinned by the defenders, the boarders never got to work
	int hurt = 0;
	float wounds = 0.0f;
	for (const CrewMember &c : d.crew) {
		if (c.status == CREW_INJURED) ++hurt;
		wounds += c.wounds;
	}
	CHECK(wounds > 0.3f && hurt <= 3); // it cost something: wounds taken, though four against two put nobody out of it
	d.systems[SYS_SENSORS].control = 0.2f; // as if they had taken it before being cleared
	Tick(d, Hours(d, 30.0f / 60.0f));
	CHECK(d.systems[SYS_SENSORS].control == 1.0f); // the station's own crew, uncontested, restore it
	Tick(d, 1.0f);
	CHECK(d.decks[7].defenders == 0); // and security stands down

	// Boarders on a deck with nothing to take move on -- toward the bridge from deck 3.
	Ship m = NewShip();
	for (CrewMember &c : m.crew)
		if (c.dept == DEPT_SECURITY) c.status = CREW_DEAD;
	Board(m, 3, 4);
	Tick(m, Hours(m, 1.5f));
	CHECK(m.decks[2].intruders < 0.5f && m.decks[0].intruders > 3.0f);
	CHECK(Intruders(m) == 4); // nobody was lost on the way
	Tick(m, Hours(m, 0.5f));
	CHECK(Hijacked(m, SYS_SHIELDS) && Hijacked(m, SYS_COMMUNICATIONS)); // the bridge is theirs

	// Venting the deck kills boarders too.
	Ship v = NewShip();
	for (CrewMember &c : v.crew)
		if (c.dept == DEPT_SECURITY) c.status = CREW_DEAD;
	Board(v, 9, 3);
	BreachDeck(v, 9, 1.0f);
	Tick(v, Hours(v, 0.5f));
	CHECK(v.decks[8].intruders == 0.0f); // none left alive on the vented deck
	CHECK(Intruders(v) < 3);             // (some had taken the torpedo bay and moved on before the air went)

	// And it is all in the save.
	Ship w = NewShip();
	Board(w, 8, 3);
	Tick(w, Hours(w, 1.0f / 60.0f));
	w.systems[SYS_SENSORS].control = 0.3f;
	const std::vector<uint8_t> blob = Pack(w);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(Hijacked(back, SYS_SENSORS) && Intruders(back) == Intruders(w));
	CHECK(Describe(back) == Describe(w));
}

static void TestBreachPuzzle()
{
	g_test = "the breach puzzle";
	for (uint32_t seed = 1; seed <= 40; ++seed) {
		const Breach b = MakeBreach(seed);
		CHECK(static_cast<int>(b.grid.size()) == b.size * b.size && b.targets.size() == 3);
		CHECK(b.targets[0].size() == 2 && b.targets[1].size() == 3 && b.targets[2].size() == 4);

		// Every puzzle can be solved in full: search the legal paths for one that scores 1.
		float best = 0.0f;
		std::vector<int> path;
		struct Search {
			const Breach &b; float &best; std::vector<int> &path;
			void Go(std::vector<bool> &used) {
				if (best >= 1.0f) return;
				if (!path.empty()) { const float sc = BreachScore(b, path); if (sc > best) best = sc; }
				if (static_cast<int>(path.size()) >= b.buffer) return;
				for (int k = 0; k < b.size; ++k) {
					int cell;
					if (path.empty()) cell = k;
					else if (path.size() % 2 == 1) cell = k * b.size + path.back() % b.size;
					else cell = (path.back() / b.size) * b.size + k;
					if (used[cell]) continue;
					used[cell] = true; path.push_back(cell);
					Go(used);
					path.pop_back(); used[cell] = false;
				}
			}
		} search{b, best, path};
		std::vector<bool> used(b.grid.size(), false);
		search.Go(used);
		CHECK(best == 1.0f);

		// The same seed is the same puzzle.
		const Breach again = MakeBreach(seed);
		CHECK(again.grid == b.grid && again.targets == b.targets);
	}
	CHECK(MakeBreach(1).grid != MakeBreach(2).grid);

	// The rules of the path.
	const Breach b = MakeBreach(7);
	CHECK(BreachScore(b, {}) == 0.0f);
	CHECK(BreachScore(b, {5}) == 0.0f);                 // must start in the top row
	CHECK(BreachScore(b, {0, 1}) == 0.0f);              // the second pick goes down the column, not along the row
	CHECK(BreachScore(b, {0, 5, 0}) == 0.0f);           // no cell twice
	CHECK(BreachScore(b, {0, 5, 6, 1, 2, 7, 8, 3}) == 0.0f); // longer than the buffer
	CHECK(BreachScore(b, {0, 99}) == 0.0f);
	const float partial = BreachScore(b, {0, 5});       // a legal path scores what it contains, perhaps nothing
	CHECK(partial >= 0.0f && partial <= 1.0f);
}

// S8. The Borg do not leave things as they found them.
static void TestBorg()
{
	g_test = "the Borg";
	Ship s = NewShip();
	for (CrewMember &c : s.crew)
		if (c.dept == DEPT_SECURITY) c.status = CREW_DEAD; // nobody to stop them
	Tick(s, 1.0f);
	const int crewOnEight = static_cast<int>(CrewOnDeck(s, 8).size());
	CHECK(crewOnEight >= 3);
	BoardBorg(s, 8, 2);
	Tick(s, Hours(s, 0.25f));
	CHECK(s.decks[7].assimilated > 0.3f && s.decks[7].assimilated < 1.0f); // under way; faster than two drones alone, as their number grows
	int taken = 0;
	for (const CrewMember &c : s.crew)
		if (c.status == CREW_ASSIMILATED) ++taken;
	CHECK(taken >= 1);                         // they take the crew they find
	CHECK(Intruders(s) >= 2 + taken);          // and each one taken is another drone
	CHECK(s.CrewFit() < COMPLEMENT - 25);

	Tick(s, Hours(s, 0.5f));
	CHECK(DeckAssimilated(s, 8));
	CHECK(s.systems[SYS_SENSORS].control == 0.0f && Hijacked(s, SYS_SENSORS));
	CounterHack(s, SYS_SENSORS, 1.0f);         // no console left to hack from
	CHECK(s.systems[SYS_SENSORS].control == 0.0f);

	// Drive them off (vent the deck): the drones die, and the deck is still Borg.
	BreachDeck(s, 8, 1.0f);
	Tick(s, Hours(s, 1.0f));
	CHECK(s.decks[7].intruders == 0.0f && !s.decks[7].borg);
	CHECK(DeckAssimilated(s, 8) && Hijacked(s, SYS_SENSORS));

	// Seal it, and the engineers strip it: hours and parts. Only then do its systems answer.
	RepairDeck(s, 8, 1.0f);
	for (int d = 0; d < DECKS; ++d) { s.decks[d].intruders = 0.0f; s.decks[d].borg = false; } // (any that had moved on)
	const float parts = s.stores.spareParts;
	const float before = s.decks[7].assimilated;
	Tick(s, Hours(s, 1.0f));
	CHECK(s.decks[7].stripping > 0 && s.decks[7].stripping <= STRIP_TEAM_MAX);
	CHECK(s.decks[7].assimilated < before && s.stores.spareParts < parts);
	Tick(s, Hours(s, 24.0f));
	CHECK(s.decks[7].assimilated == 0.0f);
	CHECK(s.systems[SYS_SENSORS].control == 1.0f && s.systems[SYS_SENSORS].output > 0.0f);
	CHECK(std::fabs((parts - s.stores.spareParts) - before * PARTS_PER_DECK) < 1.0f); // what it cost
	for (const CrewMember &c : s.crew)
		if (c.status == CREW_ASSIMILATED) CHECK(c.deck == 0); // those taken do not come back

	// Without parts a deck stays as the Borg left it.
	Ship poor = NewShip();
	poor.decks[5].assimilated = 0.8f;
	poor.stores.spareParts = 0.0f;
	Tick(poor, Hours(poor, 24.0f));
	CHECK(poor.decks[5].assimilated == 0.8f);

	// Security holds them: pinned drones convert nothing.
	Ship held = NewShip();
	BoardBorg(held, 8, 2);
	Tick(held, Hours(held, 0.2f));
	CHECK(held.decks[7].assimilated < 0.02f && Intruders(held) == 0);

	// And it is in the save.
	Ship w = NewShip();
	for (CrewMember &c : w.crew)
		if (c.dept == DEPT_SECURITY) c.status = CREW_DEAD;
	BoardBorg(w, 8, 3);
	Tick(w, Hours(w, 0.3f));
	const std::vector<uint8_t> blob = Pack(w);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(back.decks[7].borg && std::fabs(back.decks[7].assimilated - w.decks[7].assimilated) < 1e-6f);
	CHECK(Describe(back) == Describe(w));
}

// S9. The outside: a sector to cross, and something in it that shoots back.
static int FirstOfKind(const Ship &s, BeaconKind k)
{
	for (size_t i = 0; i < s.sector.size(); ++i)
		if (s.sector[i].kind == k) return static_cast<int>(i);
	return -1;
}

// Walk the chain to a beacon, resolving nothing on the way (enemies met are simply removed).
static void GoTo(Ship &s, int beacon)
{
	while (s.beacon != beacon) {
		s.enemy = Enemy();
		const bool ok = Jump(s, s.beacon + (beacon > s.beacon ? 1 : -1));
		if (!ok) return;
	}
}

static void TestSector()
{
	g_test = "the sector";
	const Ship s = NewShip();
	CHECK(static_cast<int>(s.sector.size()) == SECTOR_BEACONS && s.beacon == 0 && !InCombat(s));
	for (int i = 0; i + 1 < SECTOR_BEACONS; ++i) { // it can always be crossed
		bool linked = false;
		for (int l : s.sector[i].links) linked = linked || l == i + 1;
		CHECK(linked);
	}
	for (size_t i = 0; i < s.sector.size(); ++i)
		for (int l : s.sector[i].links) { // links run both ways
			bool back = false;
			for (int m : s.sector[l].links) back = back || m == static_cast<int>(i);
			CHECK(back);
		}
	const Ship again = NewShip();
	Config other; other.seed = 7;
	const Ship different = NewShip(other);
	bool differs = false;
	for (int i = 0; i < SECTOR_BEACONS; ++i) {
		CHECK(again.sector[i].kind == s.sector[i].kind);
		differs = differs || different.sector[i].kind != s.sector[i].kind || different.sector[i].links != s.sector[i].links;
	}
	CHECK(differs);

	// Jumping: one link at a time, with a working drive, and it costs fuel.
	Ship j = NewShip();
	Tick(j, 1.0f);
	CHECK(!Jump(j, 5) || j.sector[0].links.size() > 2); // not adjacent (unless the seed linked it)
	CHECK(!Jump(j, -1) && !Jump(j, 99));
	const float fuel = j.stores.deuterium;
	CHECK(Jump(j, 1) && j.beacon == 1 && j.sector[1].visited);
	CHECK(j.stores.deuterium < fuel);
	j.enemy = Enemy();
	DamageSystem(j, SYS_WARP_DRIVE, 0.8f);
	j.stores.spareParts = 0.0f;
	Tick(j, 1.0f);
	CHECK(!Jump(j, 2)); // a crippled drive goes nowhere -- including away
	CHECK(j.beacon == 1);

	// A derelict yields parts, once.
	Ship d = NewShip();
	Tick(d, 1.0f);
	const int derelict = FirstOfKind(d, BEACON_DERELICT);
	if (derelict > 0) {
		GoTo(d, derelict - 1);
		d.enemy = Enemy();
		const float parts = d.stores.spareParts;
		CHECK(Jump(d, derelict));
		CHECK(d.stores.spareParts == parts + SALVAGE_PARTS);
		CHECK(Jump(d, derelict - 1));
		d.enemy = Enemy();
		CHECK(Jump(d, derelict));
		CHECK(d.stores.spareParts == parts + SALVAGE_PARTS); // picked clean
	}
}

static void TestCombat()
{
	g_test = "ship to ship";
	Ship s = NewShip();
	Tick(s, 1.0f);
	const int hostile = FirstOfKind(s, BEACON_HOSTILE);
	CHECK(hostile > 0);
	GoTo(s, hostile - 1);
	s.enemy = Enemy();
	CHECK(Jump(s, hostile));
	CHECK(InCombat(s) && s.enemy.hull == 1.0f && s.enemy.shields == 1.0f);

	// At condition green the weapons are stood down: we do them no harm and our shields are not up.
	Tick(s, Hours(s, 2.0f / 60.0f));
	CHECK(s.enemy.shields == 1.0f);
	CHECK(s.shieldStrength == 0.0f);
	bool hurt = false;
	for (int i = 0; i < SYS_COUNT; ++i) hurt = hurt || s.systems[i].health < 1.0f;
	for (const Deck &d : s.decks) hurt = hurt || d.hull < 1.0f;
	CHECK(hurt);                      // their fire lands
	CHECK(Intruders(s) >= 1 && Intruders(s) <= 3); // and with our shields down they have come aboard, in Engineering
	                                  // (three came; security is already among them)
	CHECK(s.decks[ENGINEERING_DECK - 1].intruders > 0.0f);

	// Red alert: shields and phasers. The fight is now ours to win.
	SetAlert(s, ALERT_RED);
	Tick(s, Hours(s, 4.0f / 60.0f));
	CHECK(s.enemy.shields < 1.0f);
	CHECK(s.shieldStrength > 0.0f);
	const int torpedoes = s.stores.torpedoes;
	CHECK(FireTorpedo(s) && s.stores.torpedoes == torpedoes - 1);
	Tick(s, Hours(s, 20.0f / 60.0f));
	CHECK(!InCombat(s) && s.enemy.hull == 0.0f);
	CHECK(!FireTorpedo(s) && s.stores.torpedoes == torpedoes - 1); // nothing left to shoot at
	const float hullAfter = s.decks[0].hull + s.decks[7].hull + s.decks[14].hull;
	Tick(s, Hours(s, 10.0f / 60.0f));
	CHECK(s.decks[0].hull + s.decks[7].hull + s.decks[14].hull == hullAfter); // the firing has stopped

	// The fight is decided by the ship's systems: with the phasers wrecked, time alone does not win it.
	Ship weak = NewShip();
	Tick(weak, 1.0f);
	GoTo(weak, hostile - 1);
	weak.enemy = Enemy();
	Jump(weak, hostile);
	SetAlert(weak, ALERT_RED);
	DamageSystem(weak, SYS_PHASERS, 1.0f);
	weak.stores.spareParts = 0.0f;
	Tick(weak, Hours(weak, 0.5f));
	CHECK(InCombat(weak) && weak.enemy.hull == 1.0f);
	// Torpedoes still work, and three on an unshielded hull would finish it -- but its shields are up.
	CHECK(FireTorpedo(weak));
	CHECK(weak.enemy.hull == 1.0f && weak.enemy.shields < 1.0f);
	// No launchers, no torpedoes.
	DamageSystem(weak, SYS_TORPEDO_LAUNCHERS, 1.0f);
	Tick(weak, 1.0f);
	CHECK(!FireTorpedo(weak));

	// The Borg send drones, not boarders.
	Ship b = NewShip();
	Tick(b, 1.0f);
	const int cube = FirstOfKind(b, BEACON_BORG);
	if (cube > 0) {
		GoTo(b, cube - 1);
		b.enemy = Enemy();
		CHECK(Jump(b, cube) && b.enemy.borg);
		Tick(b, Hours(b, 3.0f / 60.0f));
		CHECK(b.decks[ENGINEERING_DECK - 1].borg);
	}

	// Mid-fight, it is all in the save, and carries on the same.
	Ship w = NewShip();
	Tick(w, 1.0f);
	GoTo(w, hostile - 1);
	w.enemy = Enemy();
	Jump(w, hostile);
	SetAlert(w, ALERT_RED);
	Tick(w, Hours(w, 3.0f / 60.0f));
	const std::vector<uint8_t> blob = Pack(w);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(back.beacon == w.beacon && InCombat(back) && back.hits == w.hits);
	CHECK(Describe(back) == Describe(w));
	Tick(w, Hours(w, 5.0f / 60.0f));
	Tick(back, Hours(back, 5.0f / 60.0f));
	CHECK(Describe(back) == Describe(w));
}

// S10. How it is played, and who may do what.
static void TestModesAndClocks()
{
	g_test = "modes and clocks";
	Config cfg;
	CHECK(cfg.mode == MODE_IRONMAN && !SavesAllowed(cfg)); // ironman is the game
	cfg.mode = MODE_HOLODECK;
	CHECK(SavesAllowed(cfg));

	// Accelerated: a ship's day in twenty-four minutes. Real time and wall clock: a second is a second.
	Config acc, real, wall;
	real.clockMode = CLOCK_REAL_TIME;
	wall.clockMode = CLOCK_WALL;
	CHECK(ClockRate(acc) == 60.0f && ClockRate(real) == 1.0f && ClockRate(wall) == 1.0f);
	Ship a = NewShip(acc), r = NewShip(real), w = NewShip(wall);
	Tick(a, 60.0f); Tick(r, 60.0f); Tick(w, 60.0f);
	CHECK(a.SecondOfDay() == 9 * 3600 && r.SecondOfDay() == 8 * 3600 + 60 && w.SecondOfDay() == 8 * 3600 + 60);

	// Away for two days: only the wall-clock ship lived through them.
	const float fuelA = a.stores.deuterium, fuelW = w.stores.deuterium;
	CatchUp(a, 2.0 * SECONDS_PER_DAY); CatchUp(r, 2.0 * SECONDS_PER_DAY); CatchUp(w, 2.0 * SECONDS_PER_DAY);
	CHECK(a.Day() == 0 && r.Day() == 0 && w.Day() == 2);
	CHECK(a.stores.deuterium == fuelA && w.stores.deuterium < fuelW);
	// A ship left hurt is found worse, or mended, by what her crew could do meanwhile.
	Ship hurt = NewShip(wall);
	DamageSystem(hurt, SYS_SENSORS, 0.6f);
	CatchUp(hurt, 6.0 * 3600);
	CHECK(hurt.systems[SYS_SENSORS].health == 1.0f && hurt.stores.spareParts < 100.0f);
	// A year away is not a year simulated.
	Ship longAway = NewShip(wall);
	CatchUp(longAway, 365.0 * SECONDS_PER_DAY);
	CHECK(longAway.Day() == static_cast<int>(MAX_CATCH_UP_DAYS));
	CatchUp(longAway, -5.0);
	CHECK(longAway.Day() == static_cast<int>(MAX_CATCH_UP_DAYS));
}

static void TestRankAndRoles()
{
	g_test = "rank, clearance and the player";
	Ship s = NewShip();
	const CrewMember &janeway = s.crew[0], &tuvok = s.crew[2], &torres = s.crew[5], &kim = s.crew[4], &doctor = s.crew[6];
	CHECK(MayCommand(janeway) && MayCommand(s.crew[1]) && !MayCommand(tuvok) && !MayCommand(torres));
	CHECK(MayOperate(torres, STN_ENGINEERING) && !MayOperate(torres, STN_TACTICAL));
	CHECK(MayOperate(tuvok, STN_TACTICAL) && MayOperate(tuvok, STN_ENGINEERING)); // a lieutenant commander may take any station
	CHECK(MayOperate(kim, STN_OPS) && MayOperate(kim, STN_CONN) && !MayOperate(kim, STN_SICKBAY));
	CHECK(MayOperate(doctor, STN_SICKBAY) && !MayOperate(doctor, STN_CONN));
	CHECK(MayCallAlert(torres, STN_ENGINEERING) && MayCallAlert(tuvok, STN_TACTICAL));
	CHECK(!MayCallAlert(kim, STN_OPS));           // not from Operations
	CHECK(!MayCallAlert(s.crew[9], STN_ENGINEERING)); // Vorik is an ensign
	s.crew[5].status = CREW_INJURED;
	CHECK(!MayOperate(s.crew[5], STN_ENGINEERING)); // the injured operate nothing

	// The player, three ways.
	Ship p = NewShip();
	CHECK(!PlayerMayOperate(p, STN_ENGINEERING) && !PlayerMayCommand(p)); // nobody yet
	CHECK(CreateCharacter(p, "", DEPT_ENGINEERING, 1) == -1);
	CHECK(CreateCharacter(p, "Too Senior", DEPT_ENGINEERING, 5) == -1);     // nobody is created a commander
	const int me = CreateCharacter(p, "Ensign Reyes", DEPT_ENGINEERING, 1);
	CHECK(me >= 19 && p.player == me && p.crew[me].name == "Ensign Reyes" && p.crew[me].rank == 1);
	CHECK(static_cast<int>(p.crew.size()) == COMPLEMENT);                  // a place taken, not a berth added
	CHECK(p.crew[me].post == SYS_COUNT);                                   // and nobody's station taken from them
	CHECK(PlayerMayOperate(p, STN_ENGINEERING) && !PlayerMayOperate(p, STN_TACTICAL) && !PlayerMayCommand(p));
	SetRole(p, ROLE_IN_COMMAND);
	CHECK(PlayerMayOperate(p, STN_TACTICAL) && PlayerMayCommand(p));
	SetRole(p, ROLE_MUNRO);
	CHECK(p.crew[p.player].name == "Alexander Munro");
	CHECK(PlayerMayOperate(p, STN_TACTICAL) && !PlayerMayOperate(p, STN_ENGINEERING) && !PlayerMayCommand(p));

	// The mode, the clock, the role and the created character are in the save.
	Config cfg;
	cfg.mode = MODE_HOLODECK;
	cfg.clockMode = CLOCK_WALL;
	Ship w = NewShip(cfg);
	const int who = CreateCharacter(w, "Lt. Okoro", DEPT_SCIENCES, 3);
	w.wallSeconds = 1791234567ull;
	const std::vector<uint8_t> blob = Pack(w);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(back.cfg.mode == MODE_HOLODECK && back.cfg.clockMode == CLOCK_WALL && back.cfg.role == ROLE_ANY_POST);
	CHECK(back.player == who && back.crew[who].name == "Lt. Okoro" && back.crew[who].rank == 3);
	CHECK(back.wallSeconds == 1791234567ull);
	CHECK(Pack(back) == blob);
}

// Orders: what being in command is for.
static void TestOrders()
{
	g_test = "orders";
	Ship s = NewShip();
	Tick(s, 1.0f);
	// Nobody in particular may not give them.
	CHECK(!OrderRepairFirst(s, SYS_HOLODECKS) && !OrderSecurityTo(s, 4) && !OrderEvacuate(s, 9));
	CHECK(s.orderRepairFirst == -1 && s.orderSecurityTo == 0 && s.orderEvacuate == 0);
	CreateCharacter(s, "Ensign Reyes", DEPT_ENGINEERING, 1);
	CHECK(!OrderEvacuate(s, 9)); // nor an ensign
	SetRole(s, ROLE_IN_COMMAND);

	// Repair first: with life support and the holodecks both down, the party would see to life
	// support. Ordered to the holodecks, it goes there first.
	DamageSystem(s, SYS_LIFE_SUPPORT, 0.5f);
	DamageSystem(s, SYS_HOLODECKS, 0.5f);
	CHECK(OrderRepairFirst(s, SYS_HOLODECKS));
	Ship unordered = s;
	unordered.orderRepairFirst = -1;
	Tick(s, Hours(s, 0.5f));
	Tick(unordered, Hours(unordered, 0.5f));
	CHECK(s.systems[SYS_HOLODECKS].health >= unordered.systems[SYS_HOLODECKS].health);
	CHECK(s.systems[SYS_HOLODECKS].repairing == REPAIR_TEAM_MAX);
	CHECK(OrderRepairFirst(s, -1) && s.orderRepairFirst == -1);

	// Security to a deck: a guard goes there though nobody has boarded.
	Ship g = NewShip();
	SetRole(g, ROLE_IN_COMMAND);
	Tick(g, 1.0f);
	int before = 0, after = 0;
	for (const CrewMember &c : g.crew)
		if (c.dept == DEPT_SECURITY && c.deck == 9) ++before;
	CHECK(OrderSecurityTo(g, 9));
	Tick(g, 1.0f);
	for (const CrewMember &c : g.crew)
		if (c.dept == DEPT_SECURITY && c.deck == 9) ++after;
	CHECK(after >= before + 3 && after <= before + 4);
	// And when boarders come to that deck, the guard is already fighting them.
	Board(g, 9, 2);
	Tick(g, 1.0f);
	CHECK(g.decks[8].defenders >= 4);

	// Evacuate: nobody stays, and the stations there go unmanned.
	Ship e = NewShip();
	SetRole(e, ROLE_IN_COMMAND);
	Tick(e, 1.0f);
	CHECK(!CrewOnDeck(e, 11).empty() && e.systems[SYS_WARP_DRIVE].manned > 0);
	CHECK(OrderEvacuate(e, 11));
	Tick(e, 1.0f);
	CHECK(CrewOnDeck(e, 11).empty());
	CHECK(e.systems[SYS_WARP_DRIVE].manned == 0 && e.systems[SYS_WARP_DRIVE].output == 0.5f); // on automation
	// So a deck can be cleared before it is vented: boarders die, the crew do not.
	Board(e, 11, 3);
	BreachDeck(e, 11, 1.0f);
	Tick(e, Hours(e, 0.5f));
	int lost = 0;
	for (const CrewMember &c : e.crew)
		if (c.status != CREW_FIT) ++lost;
	CHECK(e.decks[10].intruders == 0.0f && lost == 0);
	// Seal it, let the air come back, and only then let them return -- sending them back at once
	// would put them on a deck that is sealed but not yet breathable.
	RepairDeck(e, 11, 1.0f);
	Tick(e, Hours(e, 2.0f));
	CHECK(e.decks[10].atmosphere == 1.0f && CrewOnDeck(e, 11).empty());
	CHECK(OrderEvacuate(e, 0));
	Tick(e, 1.0f);
	CHECK(e.systems[SYS_WARP_DRIVE].manned > 0); // and they go back

	// Orders are the ship's, so they are in the save.
	Ship w = NewShip();
	SetRole(w, ROLE_IN_COMMAND);
	OrderRepairFirst(w, SYS_SENSORS); OrderSecurityTo(w, 4); OrderEvacuate(w, 9);
	const std::vector<uint8_t> blob = Pack(w);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(back.orderRepairFirst == SYS_SENSORS && back.orderSecurityTo == 4 && back.orderEvacuate == 9);
}

static void TestStations()
{
	g_test = "stations";
	int owned[STN_COUNT] = {0, 0, 0, 0, 0};
	for (int i = 0; i < SYS_COUNT; ++i) {
		const SystemId id = static_cast<SystemId>(i);
		const Station st = StationOf(id);
		CHECK(st != STN_ENGINEERING && st < STN_COUNT); // every system has one operating station
		++owned[st];
		CHECK(OperatedFrom(id, STN_ENGINEERING));        // engineering sees everything
		int seenBy = 0;
		for (int s = 1; s < STN_COUNT; ++s)
			if (OperatedFrom(id, static_cast<Station>(s))) ++seenBy;
		CHECK(seenBy == 1);                              // and exactly one other station does
	}
	for (int s = 1; s < STN_COUNT; ++s) CHECK(owned[s] > 0); // no station is a console with nothing on it
	CHECK(StationOf(SYS_PHASERS) == STN_TACTICAL && StationOf(SYS_SHIELDS) == STN_TACTICAL);
	CHECK(StationOf(SYS_WARP_DRIVE) == STN_CONN && StationOf(SYS_SICKBAY) == STN_SICKBAY);
	CHECK(StationOf(SYS_TRANSPORTERS) == STN_OPS);
	CHECK(std::string(StationName(STN_TACTICAL)) == "TACTICAL");
}

static void TestCrewOnDeck()
{
	g_test = "who is on a deck";
	Ship s = NewShip();
	s.crew[30].status = CREW_DEAD;
	Tick(s, 1.0f);
	int total = 0;
	for (int d = 1; d <= DECKS; ++d) {
		for (int i : CrewOnDeck(s, d)) {
			CHECK(s.crew[i].deck == d);
			++total;
		}
	}
	CHECK(total == COMPLEMENT - 1);          // everyone alive is on exactly one deck
	CHECK(CrewOnDeck(s, 0).empty());         // the dead are on none
	CHECK(CrewOnDeck(s, 99).empty());
	// 0800: alpha at stations. The captain is on the bridge and the chief engineer in Engineering.
	bool janeway = false, torres = false;
	for (int i : CrewOnDeck(s, 1)) if (s.crew[i].name == "Kathryn Janeway") janeway = true;
	for (int i : CrewOnDeck(s, 11)) if (s.crew[i].name == "B'Elanna Torres") torres = true;
	CHECK(janeway && torres);
	// 1600: alpha comes off watch and goes to eat. The mess hall fills; the bridge keeps a watch.
	Tick(s, Hours(s, 8.0f));
	CHECK(CrewOnDeck(s, 2).size() >= 50);
	CHECK(!CrewOnDeck(s, 1).empty());
	janeway = false;
	for (int i : CrewOnDeck(s, 2)) if (s.crew[i].name == "Kathryn Janeway") janeway = true;
	CHECK(janeway);
}

static int PrintDay()
{
	Ship s = NewShip();
	std::printf("%s\n", Describe(s).c_str());
	static const char *const ACT[] = {"duty", "meal", "rec", "personal", "sleep"};
	std::printf("hour  watch  on-duty  mess  recreation  quarters   Janeway        Torres          Crewman 100\n");
	for (int h = 0; h < 24; ++h) {
		int n[ACT_COUNT] = {0, 0, 0, 0, 0};
		for (const CrewMember &c : s.crew) ++n[c.activity];
		const CrewMember &j = s.crew[0], &t = s.crew[5], &x = s.crew[99];
		std::printf("%02d:00  %d      %3d    %3d     %3d       %3d      %-8s d%-2d   %-8s d%-2d    %-8s d%-2d\n", s.SecondOfDay() / 3600, s.Watch(),
			n[ACT_ON_DUTY], n[ACT_MEAL], n[ACT_RECREATION], n[ACT_PERSONAL] + n[ACT_SLEEP], ACT[j.activity], j.deck, ACT[t.activity], t.deck,
			ACT[x.activity], x.deck);
		Tick(s, Hours(s, 1.0f));
	}
	return 0;
}

int main(int argc, char **argv)
{
	if (argc > 1 && !std::strcmp(argv[1], "--day")) return PrintDay();

	TestNominalShip();
	TestPowerIsConserved();
	TestSheddingFollowsPriority();
	TestBatteriesKeepTheCrewAlive();
	TestHullBreach();
	TestFuel();
	TestRoster();
	TestSchedule();
	TestADayAboard();
	TestRedAlertAndCasualties();
	TestDamageAndRepair();
	TestMoraleAndFatigue();
	TestDeterminismAndStepSize();
	TestSave();
	TestStations();
	TestCrewOnDeck();
	TestDamageControl();
	TestCasualties();
	TestTriage();
	TestAirAndEndurance();
	TestLog();
	TestBoarding();
	TestBreachPuzzle();
	TestBorg();
	TestSector();
	TestCombat();
	TestModesAndClocks();
	TestRankAndRoles();
	TestOrders();

	if (g_failures) {
		std::printf("%d check(s) failed\n", g_failures);
		return 1;
	}
	std::printf("ship_core: all checks passed\n");
	return 0;
}
