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
	s.stores.spareParts = 0.0f; // no parts: the crew cannot seal it, so the deck stays open
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
	CHECK(blob.size() < 65536); // the whole ship, its 141 crew, and their bounded memories
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
	s.stores.spareParts = 0.0f; // no parts: the hull cannot be sealed while we watch the attrition
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

// S6's second injury cause: radiation from a failing reactor hurts the engineering watch.
static void TestRadiation()
{
	g_test = "radiation from a failing core";
	Ship rad = NewShip();
	rad.stores.spareParts = 0.0f;              // the core cannot be mended out from under the check
	DamageSource(rad, SRC_WARP_CORE, 0.8f);    // the reactor is at a fifth
	Ship ok = NewShip();
	Tick(rad, Hours(rad, 0.1f));               // six minutes on deck 11
	Tick(ok, Hours(ok, 0.1f));
	int hurt = 0, clean = 0;
	for (const CrewMember &c : rad.crew) if (c.status == CREW_INJURED) ++hurt;
	for (const CrewMember &c : ok.crew) if (c.status == CREW_INJURED) ++clean;
	CHECK(hurt > 0);       // the engineering watch was irradiated
	CHECK(clean == 0);     // a whole core is clean
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
	// (The replicators restock supplies -- the triage gap's control -- so they are stood down first.)
	Ship t = NewShip();
	SetEnabled(t, SYS_REPLICATORS, false);
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

// The written-off list (docs/damage-and-budgets.md): the ship records what she has given up -- when,
// what, the kind of loss and who decided. It is bounded, evicting the oldest, and survives a save.
static void TestWrittenOffList()
{
	g_test = "the ship carries a written list of what she has given up";
	Ship s = NewShip();
	const std::string decider = CommandingOfficer(s);

	CHECK(WriteOff(s, false, 9, LOSS_SEALED));            // a compartment sealed and left
	CHECK(WriteOff(s, true, SYS_SENSORS, LOSS_STRIPPED)); // a system stripped for parts
	CHECK(WriteOff(s, false, 5, LOSS_UNINHABITABLE));     // a compartment marked uninhabitable
	CHECK(!WriteOff(s, false, 9, LOSS_SEALED));           // a thing is on the list once
	CHECK(!WriteOff(s, false, 99, LOSS_SEALED));          // not a deck
	CHECK(!WriteOff(s, true, SYS_COUNT, LOSS_SEALED));    // not a system
	CHECK(!WriteOff(s, false, 3, 99));                    // not a kind

	const std::vector<LossEntry> &l = WriteOffs(s);
	CHECK(l.size() == 3);
	CHECK(!l[0].system && l[0].target == 9 && l[0].kind == LOSS_SEALED);
	CHECK(l[1].system && l[1].target == SYS_SENSORS && l[1].kind == LOSS_STRIPPED);
	CHECK(!l[2].system && l[2].target == 5 && l[2].kind == LOSS_UNINHABITABLE);
	for (const LossEntry &e : l) {
		CHECK(e.time == s.clock);
		CHECK(!e.what.empty() && !e.who.empty());
		CHECK(e.who == decider);
	}

	// The list survives a save and a load, in order.
	std::vector<uint8_t> blob = Pack(s);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(WriteOffs(back).size() == l.size());
	CHECK(!WriteOffs(back).empty() && WriteOffs(back).back().what == l.back().what && WriteOffs(back).back().who == decider);

	// The bound: every deck and every system is written off, and the oldest fall off the end.
	Ship b = NewShip();
	int written = 0;
	for (int d = 1; d <= DECKS; ++d) { CHECK(WriteOff(b, false, d, LOSS_SEALED)); ++written; }
	for (int i = 0; i < SYS_COUNT; ++i) { CHECK(WriteOff(b, true, i, LOSS_STRIPPED)); ++written; }
	CHECK(written == DECKS + SYS_COUNT);
	const std::vector<LossEntry> &bl = WriteOffs(b);
	const int evicted = DECKS + SYS_COUNT - LOSS_MAX;
	CHECK(evicted > 0);
	CHECK(static_cast<int>(bl.size()) == LOSS_MAX);
	CHECK(!bl.front().system && bl.front().target == evicted + 1);   // the oldest decks fall off first
	CHECK(bl.back().system && bl.back().target == SYS_COUNT - 1);
	for (int d = 1; d <= evicted; ++d) {
		bool found = false;
		for (const LossEntry &e : bl) if (!e.system && e.target == d) found = true;
		CHECK(!found);
	}
}

// The tricorder gap: a scan reveals; a weak charge misreads; a dead tricorder does nothing.
static void TestAwayKit()
{
	g_test = "the tricorder reveals, misreads on a weak charge, and dies";
	Ship s = NewShip();
	s.sector[s.beacon].kind = BEACON_HOSTILE;   // something real to find, and to get wrong
	s.sector[s.beacon].visited = false;
	LoadAwayKit(s, 4, 6, 4, 1.0f);
	CHECK(s.stores.tricorders == 4 && s.stores.phasers == 6 && s.stores.evSuits == 4);
	CHECK(Scan(s, s.beacon) == 1);
	CHECK(s.sector[s.beacon].visited);
	CHECK(!s.log.empty() && s.log.back().what.find("a hostile ship") != std::string::npos);

	// A weak charge reads the next thing along: the site is a hostile, the reading is not.
	s.stores.tricorderCharge = 0.15f;
	CHECK(Scan(s, s.beacon) == 2);
	CHECK(s.log.back().what.find("a hostile ship") == std::string::npos);
	CHECK(s.log.back().what.find("weak") != std::string::npos);

	// A dead tricorder scans nothing, and says so.
	s.stores.tricorderCharge = 0.0f;
	const size_t before = s.log.size();
	CHECK(Scan(s, s.beacon) == 0);
	CHECK(s.log.size() == before + 1 && s.log.back().what.find("dead") != std::string::npos);

	// The kit survives a save and a load.
	std::vector<uint8_t> blob = Pack(s);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(back.stores.tricorders == s.stores.tricorders && std::fabs(back.stores.tricorderCharge - s.stores.tricorderCharge) < 1e-4f);
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
	s.stores.spareParts = 0.0f; // no parts: the crew cannot seal the breach and spoil the vent
	BreachDeck(s, 8, 1.0f);
	Tick(s, Hours(s, 1.0f));
	CHECK(s.decks[7].intruders == 0.0f && !s.decks[7].borg);
	CHECK(DeckAssimilated(s, 8) && Hijacked(s, SYS_SENSORS));

	// Seal it, and the engineers strip it: hours and parts. Only then do its systems answer.
	s.stores.spareParts = 100.0f; // the parts the strip will need
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
	s.enemy.kind = ENEMY_RAIDER; s.enemy.firepower = 0.25f; // the standard raider, for a stable fight
	s.contact2 = Enemy();                                    // and no wingman, so the fight is the one fight
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
	// The firing has stopped: no system takes new damage. (Hulls and fires may still be settling.)
	int systemsHurt = 0;
	for (int i = 0; i < SYS_COUNT; ++i) if (s.systems[i].health < 1.0f) ++systemsHurt;
	Tick(s, Hours(s, 10.0f / 60.0f));
	int systemsHurtAfter = 0;
	for (int i = 0; i < SYS_COUNT; ++i) if (s.systems[i].health < 1.0f) ++systemsHurtAfter;
	CHECK(systemsHurtAfter <= systemsHurt);

	// The fight is decided by the ship's systems: with the phasers wrecked, time alone does not win it.
	Ship weak = NewShip();
	Tick(weak, 1.0f);
	GoTo(weak, hostile - 1);
	weak.enemy = Enemy();
	Jump(weak, hostile);
	weak.enemy.kind = ENEMY_RAIDER; weak.enemy.firepower = 0.25f;
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
	w.enemy.kind = ENEMY_RAIDER; w.enemy.firepower = 0.25f; w.contact2 = Enemy();
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

static void StartFightTest(Ship &s);

// S9: more than one contact at a time.
static void TestMultipleContacts()
{
	g_test = "more than one contact";
	auto setFight = [](Ship &x) {
		Tick(x, 1.0f);
		x.enemy = Enemy();
		x.enemy.present = true; x.enemy.kind = ENEMY_RAIDER;
		x.enemy.hull = 1.0f; x.enemy.shields = 1.0f; x.enemy.weapons = 1.0f; x.enemy.firepower = 0.25f;
	};
	Ship a = NewShip(); setFight(a);
	Ship b = NewShip(); setFight(b); b.contact2 = b.enemy; // a wingman joins
	Tick(a, Hours(a, 0.4f));
	Tick(b, Hours(b, 0.4f));
	float ha = 0.0f, hb = 0.0f;
	for (int i = 0; i < SYS_COUNT; ++i) { ha += a.systems[i].health; hb += b.systems[i].health; }
	CHECK(hb < ha); // two contacts firing do more damage than one

	// Destroy the primary and the wingman becomes the primary.
	Ship c = NewShip(); setFight(c); c.contact2 = c.enemy;
	c.enemy.hull = 0.0f;
	Tick(c, 0.01f);
	CHECK(!c.contact2.present && c.enemy.present && c.enemy.hull > 0.0f);

	// A wingman survives a save.
	Tick(b, 0.0f); // recompute the derived counts from the final fires, as the game does after a change
	std::vector<uint8_t> blob = Pack(b);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(back.contact2.present == b.contact2.present);
	CHECK(std::fabs(back.contact2.hull - b.contact2.hull) < 1e-4f && Describe(back) == Describe(b));
}

// S9, full: an opponent with systems of its own, choices at a beacon, pursuit, and an end to reach.
static void TestEnemySystemsAndOutsideChoices()
{
	g_test = "the outside: enemy systems, choices, pursuit, and an end";

	// An enemy has systems: Tactical targets one, and breaking it changes what the enemy can do.
	Ship s = NewShip();
	Tick(s, 1.0f);
	const int hostile = FirstOfKind(s, BEACON_HOSTILE);
	CHECK(hostile > 0);
	GoTo(s, hostile - 1);
	s.enemy = Enemy();
	Jump(s, hostile);
	s.enemy.kind = ENEMY_RAIDER; s.enemy.firepower = 0.25f; s.enemy.shieldGen = 0.0f; s.enemy.shields = 0.0f;
	s.contact2 = Enemy();
	SetTarget(s, TARGET_WEAPONS);
	CHECK(Target(s) == TARGET_WEAPONS && std::string(EnemySubsystemName(TARGET_WEAPONS)) == "its weapons");
	SetAlert(s, ALERT_RED);
	const float hullBefore = s.enemy.hull;
	Tick(s, Hours(s, 6.0f));
	CHECK(s.enemy.weapons < 1.0f);          // its weapons are broken down
	CHECK(std::fabs(s.enemy.hull - hullBefore) < 0.05f); // and its hull, untargeted, is barely touched
	Ship other = NewShip();                 // a warship answers harder than a raider
	CHECK(std::string(EnemyKindName(ENEMY_WARSHIP)) == "a warship" && std::string(EnemyKindName(ENEMY_BORG_VESSEL)) == "a Borg vessel");

	// Choices at a beacon: trade, answer a distress call, hail, run.
	Ship t = NewShip();
	CHECK(!Trade(t)); // nothing to trade with in empty space
	t.sector[t.beacon].kind = BEACON_TRADER;
	t.sector[t.beacon].visited = false;
	t.stores.rations = 10.0f;
	const float partsBefore = t.stores.spareParts, ratBefore = t.stores.rations;
	CHECK(Trade(t) && t.stores.spareParts < partsBefore && t.stores.rations > ratBefore);
	Ship d = NewShip();
	d.beacon = 4;                            // even: a wreck to help, not a trap
	d.sector[4].kind = BEACON_DISTRESS; d.sector[4].visited = false;
	d.stores.medicalSupplies = 50.0f;
	const float medBefore = d.stores.medicalSupplies;
	CHECK(AnswerDistress(d) && d.stores.medicalSupplies > medBefore);
	Ship trap = NewShip();
	trap.beacon = 5;                         // odd: a trap
	trap.sector[5].kind = BEACON_DISTRESS; trap.sector[5].visited = false;
	CHECK(AnswerDistress(trap) && InCombat(trap));
	Ship h = NewShip();
	h.sector[h.beacon].kind = BEACON_HOSTILE; h.sector[h.beacon].visited = false;
	CHECK(Hail(h) && InCombat(h));           // a hostile answers with its weapons

	// Pursuit: a raider whose engines survived follows us, and repairs suffer while it does.
	Ship p = NewShip();
	p.beacon = 1;
	StartFightTest(p);
	Tick(p, Hours(p, 0.1f));
	CHECK(Pursued(p) && PursuitJumps(p) > 0);
	const int behind = PursuitJumps(p);
	CHECK(Jump(p, p.sector[p.beacon].links.front()) && PursuitJumps(p) == behind - 1);
	// A damaged system mends more slowly with a raider behind us.
	Ship fast = NewShip(); DamageSystem(fast, SYS_SENSORS, 0.5f); Tick(fast, Hours(fast, 1.0f));
	Ship slow = NewShip(); slow.pursued = true; DamageSystem(slow, SYS_SENSORS, 0.5f); Tick(slow, Hours(slow, 1.0f));
	CHECK(slow.systems[SYS_SENSORS].health < fast.systems[SYS_SENSORS].health);

	// An end to reach: the sector's last beacon ends it; crossing three sectors wins.
	Ship e = NewShip();
	GoTo(e, SECTOR_BEACONS - 1);
	CHECK(AtEnd(e));
	CHECK(AdvanceSector(e) && SectorNumber(e) == 1 && e.beacon == 0 && !AtEnd(e));
	e.beacon = SECTOR_BEACONS - 1; e.reachedEnd = true;
	CHECK(AdvanceSector(e) && SectorNumber(e) == 2);
	e.beacon = SECTOR_BEACONS - 1; e.reachedEnd = true;
	CHECK(AdvanceSector(e) && Won(e));
	CHECK(!AdvanceSector(e)); // nothing left to cross

	// It is all in the save.
	Ship w = NewShip();
	w.target = TARGET_ENGINES; w.pursued = true; w.pursuitJumps = 2; w.sectorNumber = 1; w.reachedEnd = true;
	std::vector<uint8_t> blob = Pack(w);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(back.target == TARGET_ENGINES && back.pursued && back.pursuitJumps == 2);
	CHECK(back.sectorNumber == 1 && back.reachedEnd && Describe(back) == Describe(w));
}

// A smallest fight at the ship's beacon: a raider already breaking up, engines intact, so it flees.
static void StartFightTest(Ship &s)
{
	s.enemy.present = true;
	s.enemy.kind = ENEMY_RAIDER;
	s.enemy.hull = 0.1f;
	s.enemy.shields = 1.0f;
	s.enemy.engines = 1.0f;
	s.enemy.weapons = 1.0f;
	s.enemy.firepower = 0.25f;
}

// The backlog's first items: the tractor beam and salvage as the door to a materials economy, and the
// three travel systems each gating or risking a jump; and the EMH.
static void TestMaterialsAndTravel()
{
	g_test = "the materials economy, travel systems and the EMH";

	// A derelict is stripped with the tractor: parts and raw material, once.
	Ship s = NewShip();
	Tick(s, 1.0f);
	const int derelict = FirstOfKind(s, BEACON_DERELICT);
	if (derelict > 0) {
		GoTo(s, derelict - 1);
		s.enemy = Enemy();
		Jump(s, derelict);                 // the quick look yields parts (S9)
		const float matBefore = s.stores.materials, partsBefore = s.stores.spareParts;
		CHECK(TractorWreck(s));            // the hold strips it fully
		CHECK(s.stores.materials == matBefore + SALVAGE_MATERIALS && s.stores.spareParts == partsBefore + SALVAGE_PARTS);
		CHECK(!TractorWreck(s));           // once
		// With the tractor down, nothing to strip.
		Ship t = NewShip();
		t.sector[t.beacon].kind = BEACON_DERELICT;
		SetEnabled(t, SYS_TRACTOR_BEAM, false);
		Tick(t, 1.0f);
		CHECK(!TractorWreck(t));
	}

	// Fabrication: with the replicators running, material becomes spare parts.
	Ship f = NewShip();
	Tick(f, 1.0f);
	f.stores.materials = 30.0f;
	const float parts = f.stores.spareParts;
	CHECK(FabricateParts(f, 12) && f.stores.materials == 18.0f && f.stores.spareParts == parts + 12.0f);
	CHECK(!FabricateParts(f, 100));        // not enough material
	SetEnabled(f, SYS_REPLICATORS, false);
	SetAlert(f, ALERT_RED);
	Tick(f, 1.0f);
	CHECK(!FabricateParts(f, 5));          // no replicators, no fabrication

	// The tractor beam holds a contact so it cannot break off.
	Ship h = NewShip();
	h.beacon = 1;
	StartFightTest(h);                     // a raider with hull going, engines intact
	Tick(h, Hours(h, 0.1f));
	CHECK(Held(h) == false && Pursued(h)); // it ran
	Ship lock = NewShip();
	lock.beacon = 1;
	StartFightTest(lock);
	CHECK(TractorHold(lock) && Held(lock));
	Tick(lock, Hours(lock, 0.1f));
	CHECK(InCombat(lock) && !Pursued(lock)); // held: it cannot run
	CHECK(!TractorHold(lock));             // release
	Tick(lock, Hours(lock, 0.1f));
	CHECK(Pursued(lock));                  // now it runs

	// The travel systems: safe with all three, and a weak drive costs on the jump.
	CHECK(TravelSafe(NewShip()));
	Ship d = NewShip();
	DamageSystem(d, SYS_NAV_DEFLECTOR, 1.0f);
	d.stores.spareParts = 0.0f;
	Tick(d, 1.0f); Tick(d, 1.0f);
	CHECK(!TravelSafe(d));
	const int deck = static_cast<int>((d.hits + 3u) % DECKS) + 1;
	CHECK(Jump(d, d.sector[d.beacon].links.front()));
	CHECK(d.decks[deck - 1].hull < 1.0f);  // dust through the weak deflector
	Ship m = NewShip();
	DamageSystem(m, SYS_INERTIAL_DAMPERS, 1.0f);
	m.stores.spareParts = 0.0f;
	Tick(m, 1.0f); Tick(m, 1.0f);
	int before = 0;
	for (const CrewMember &c : m.crew) if (c.status == CREW_INJURED) ++before;
	CHECK(Jump(m, m.sector[m.beacon].links.front()));
	int after = 0;
	for (const CrewMember &c : m.crew) if (c.status == CREW_INJURED) ++after;
	CHECK(after > before);                 // the jump shook the crew

	// The EMH: with the medical staff dead and no sickbay, the program keeps the ward going.
	Ship e = NewShip();
	for (CrewMember &c : e.crew) if (c.dept == DEPT_MEDICAL) c.status = CREW_DEAD;
	SetEnabled(e, SYS_SICKBAY, false);
	Tick(e, 1.0f);                         // let the sickbay's output fall to zero first
	e.crew[3].status = CREW_INJURED;
	e.crew[3].severity = 0.5f;
	Tick(e, Hours(e, 2.0f));
	CHECK(e.crew[3].recovery == 0.0f);     // nobody to treat them
	CHECK(ActivateEMH(e, true) && EMHActive(e));
	Tick(e, Hours(e, 2.0f));
	CHECK(e.crew[3].recovery > 0.0f);      // the hologram does
	e.stores.spareParts = 0.0f;            // so the core cannot be mended out from under the check
	DamageSystem(e, SYS_COMPUTER_CORE, 1.0f);
	Tick(e, 1.0f);
	CHECK(!EMHActive(e));                  // a hologram needs the computer

	// The galley: material becomes food, so a crew out of rations can eat again.
	Ship galley = NewShip();
	galley.stores.materials = 20.0f;
	galley.stores.rations = 10.0f;
	CHECK(FabricateRations(galley, 10) && galley.stores.rations == 20.0f && galley.stores.materials == 10.0f);
	CHECK(!FabricateRations(galley, 100));
	// The Maquis split is a drag: half a crew at odds with the other half is worse than a united one.
	Ship united = NewShip(), split = NewShip();
	for (int i = 0; i < static_cast<int>(split.crew.size()); ++i) split.crew[i].faction = static_cast<uint8_t>(i % 2);
	Tick(united, Hours(united, 24.0f));
	Tick(split, Hours(split, 24.0f));
	float um = 0.0f, sm = 0.0f;
	for (const CrewMember &c : united.crew) if (c.status == CREW_FIT) um += c.morale;
	for (const CrewMember &c : split.crew) if (c.status == CREW_FIT) sm += c.morale;
	CHECK(sm < um);

	// It is all in the save.
	Ship w = NewShip();
	w.stores.materials = 33.0f;
	w.emhActive = true;
	w.enemyHeld = true;
	w.sector[w.beacon].looted = true;
	std::vector<uint8_t> blob = Pack(w);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(back.stores.materials == 33.0f && back.emhActive && back.enemyHeld);
	CHECK(back.sector[w.beacon].looted && Describe(back) == Describe(w));
}

// The crew and ship tied together: training and credentials, the brig, a funeral, the player's career
// and body, and the Borg's adaptation.
static void TestCrewJusticeAndBorg()
{
	g_test = "credentials, the brig, grief, the career and the Borg's adaptation";

	// A credential lets a crew member operate a station their department does not own.
	Ship s = NewShip();
	int scientist = -1;
	for (int i = 0; i < static_cast<int>(s.crew.size()); ++i)
		if (s.crew[i].dept == DEPT_SCIENCES && s.crew[i].rank < 4 && s.crew[i].status == CREW_FIT && s.crew[i].post == SYS_COUNT) { scientist = i; break; }
	CHECK(scientist >= 0);
	CHECK(!MayOperate(s.crew[scientist], STN_TACTICAL));
	CHECK(Train(s, scientist, STN_TACTICAL) && Qualified(s.crew[scientist], STN_TACTICAL));
	CHECK(MayOperate(s.crew[scientist], STN_TACTICAL));       // trained: may now work Tactical
	CHECK(!Train(s, scientist, STN_TACTICAL));                // already qualified

	// The brig: confined, a crew member operates nothing and stands no watch.
	CHECK(Brig(s, scientist, true) && Brigged(s, scientist));
	CHECK(!MayOperate(s.crew[scientist], STN_TACTICAL));
	Tick(s, 1.0f);
	CHECK(s.crew[scientist].activity != ACT_ON_DUTY);
	CHECK(Brig(s, scientist, false) && !Brigged(s, scientist));

	// A funeral lifts the crew, and only whoever commands holds one.
	Ship f = NewShip();
	for (CrewMember &c : f.crew) c.morale = 0.3f;
	CHECK(!HoldFuneral(f));                                   // nobody in particular
	SetRole(f, ROLE_IN_COMMAND);
	const float before = f.crew[0].morale;
	CHECK(HoldFuneral(f) && f.crew[0].morale > before);

	// Grief: a death is notified and the quarters sealed; the funeral opens them again. A crew member
	// asleep on a deck that loses its air dies (their quarters are there, so the injured stay in it).
	Ship d = NewShip();
	d.decks[8].atmosphere = 0.0f;
	d.decks[8].hull = 0.0f;
	d.crew[50].watch = 1; d.crew[50].post = SYS_COUNT; d.crew[50].quartersDeck = 9; // off duty, asleep there
	Tick(d, Hours(d, 0.12f)); // past EXPOSURE_KILLS
	CHECK(d.crew[50].status == CREW_DEAD && d.crew[50].quartersSealed);
	int sealed = 0;
	for (const CrewMember &c : d.crew) if (c.quartersSealed) ++sealed;
	CHECK(sealed >= 1);
	SetRole(d, ROLE_IN_COMMAND);
	CHECK(HoldFuneral(d));
	for (const CrewMember &c : d.crew) CHECK(!c.quartersSealed);

	// The career: a promotion within the complement, by whoever commands.
	Ship p = NewShip();
	const int who = CreateCharacter(p, "Ensign Reyes", DEPT_ENGINEERING, 1);
	CHECK(who >= 0);
	const uint8_t was = p.crew[who].rank;
	SetRole(p, ROLE_IN_COMMAND);
	CHECK(Promote(p, who) && p.crew[who].rank == was + 1);
	CHECK(!PlayerIncapacitated(p));
	p.crew[who].status = CREW_INJURED;
	CHECK(PlayerIncapacitated(p));                            // the player's body is a crew record

	// The Borg adapt: the same fire does less to a cube than to a raider.
	auto HurtAfter = [](Ship &sh, EnemyKind kind) {
		Tick(sh, 1.0f);
		const int hostile = FirstOfKind(sh, BEACON_HOSTILE);
		GoTo(sh, hostile - 1);
		sh.enemy = Enemy();
		Jump(sh, hostile);
		sh.enemy.kind = kind; sh.enemy.borg = kind == ENEMY_BORG_VESSEL;
		sh.enemy.shields = 0.0f; sh.enemy.shieldGen = 0.0f; sh.enemy.firepower = 0.0f;
		SetAlert(sh, ALERT_RED);
		Tick(sh, Hours(sh, 10.0f));
		return sh.enemy.hull;
	};
	Ship raider = NewShip(), cube = NewShip();
	const float raiderHull = HurtAfter(raider, ENEMY_RAIDER);
	const float cubeHull = HurtAfter(cube, ENEMY_BORG_VESSEL);
	CHECK(cubeHull > raiderHull);                             // the Borg take less after adapting
	CHECK(cube.enemy.adaptation > 0.0f);

	// It is all in the save.
	Tick(f, 0.0f); // derive manning with the funeral's morale, as the game does after an order
	std::vector<uint8_t> blob = Pack(f);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(back.crew[0].faction == f.crew[0].faction && back.crew[0].credentials == f.crew[0].credentials);
	CHECK(Describe(back) == Describe(f));
}

// The memory model (docs/memory-and-consequence.md): bounded marks with provenance, decay, bonds.
static void TestMemoryAndConsequence()
{
	g_test = "memory and consequence";

	Ship s = NewShip();
	// A mark is held with its provenance; a character who was not told does not hold it.
	Remember(s, 0, MEM_DEATH, 5, MEM_SAW, -0.8f);
	CHECK(Recall(s.crew[0], MEM_DEATH) && RecallSource(s.crew[0], MEM_DEATH) == MEM_SAW);
	CHECK(!Recall(s.crew[1], MEM_DEATH));           // not present, not told: does not know
	Brief(s, MEM_DEATH, -0.4f);                      // command tells the crew
	CHECK(Recall(s.crew[1], MEM_DEATH) && RecallSource(s.crew[1], MEM_DEATH) == MEM_TOLD);

	// Telling the same thing again reinforces the mark rather than duplicating it.
	const int before = MemoryCount(s.crew[0]);
	Remember(s, 0, MEM_DEATH, 5, MEM_SAW, -0.8f);
	CHECK(MemoryCount(s.crew[0]) == before);

	// Bonds: remembered valence toward a person, positive and negative.
	Remember(s, 2, MEM_RESCUE, 3, MEM_SAW, 0.9f);
	Remember(s, 4, MEM_LIE, 3, MEM_TOLD, -0.9f);
	CHECK(Bond(s, 2, 3) > 0.5f);                     // a rescue remembered
	CHECK(Bond(s, 4, 3) < -0.5f);                    // a lie remembered
	CHECK(Bond(s, 0, 3) == 0.0f);                    // nothing between these two

	// The store is bounded: over the cap, the least salient (oldest on a tie) falls off.
	Ship b = NewShip();
	for (uint16_t e = 100; e < 110; ++e) { Remember(b, 0, e, -1, MEM_SAW, -0.1f); Tick(b, 1.0f); }
	CHECK(MemoryCount(b.crew[0]) == MEMORY_MAX);
	CHECK(!Recall(b.crew[0], 100));                  // the first, least salient, was evicted

	// Salience decays with time.
	float sal = 0.0f;
	for (const Memory &m : b.crew[0].memories) sal = std::max(sal, m.salience);
	Tick(b, Hours(b, 10.0f));
	float sal2 = 0.0f;
	for (const Memory &m : b.crew[0].memories) sal2 = std::max(sal2, m.salience);
	CHECK(sal2 < sal);

	// The director reads the marks: one person's story reaches the log.
	Ship dr = NewShip();
	Remember(dr, 7, MEM_DEATH, 5, MEM_SAW, -1.0f);
	Tick(dr, Hours(dr, 6.0f));
	bool told = false;
	for (const LogEntry &e : dr.log) if (e.what.find(dr.crew[7].name + " is still carrying what happened") != std::string::npos) told = true;
	CHECK(told);

	// And it is all in the save.
	std::vector<uint8_t> blob = Pack(s);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(Recall(back.crew[0], MEM_DEATH) && Recall(back.crew[1], MEM_DEATH));
	CHECK(RecallSource(back.crew[1], MEM_DEATH) == MEM_TOLD);
	CHECK(std::fabs(Bond(back, 2, 3) - Bond(s, 2, 3)) < 1e-4f);
	CHECK(Pack(back) == blob);
}

// The backlog, continued: resource acquisition (mining a belt), population pressure (refugees), and
// justice (a hearing).
static void TestResourcesAndPressure()
{
	g_test = "resource belts, refugees and the hearing";

	// A belt yields material and fuel once, with a tractor or sensors to reach it.
	Ship s = NewShip();
	Tick(s, 1.0f);
	const int belt = FirstOfKind(s, BEACON_BELT);
	if (belt > 0) {
		GoTo(s, belt - 1);
		s.enemy = Enemy();
		Jump(s, belt);
		const float mat = s.stores.materials, deut = s.stores.deuterium;
		CHECK(MineBelt(s) && s.stores.materials == mat + BELT_MATERIALS && s.stores.deuterium > deut);
		CHECK(!MineBelt(s)); // worked out
	}
	Ship none = NewShip();
	none.sector[none.beacon].kind = BEACON_BELT;
	SetEnabled(none, SYS_TRACTOR_BEAM, false);
	SetEnabled(none, SYS_SENSORS, false);
	Tick(none, 1.0f);
	CHECK(!MineBelt(none));

	// Refugees eat and crowd the ship: fewer rations and lower morale than a ship without them.
	Ship empty = NewShip(), full = NewShip();
	empty.stores.rations = 60.0f; full.stores.rations = 60.0f;
	CHECK(TakeSurvivors(full, 40) && Refugees(full) == 40);
	Tick(empty, Hours(empty, 24.0f));
	Tick(full, Hours(full, 24.0f));
	CHECK(full.stores.rations < empty.stores.rations);
	float em = 0.0f, fm = 0.0f;
	for (const CrewMember &c : empty.crew) if (c.status == CREW_FIT) em += c.morale;
	for (const CrewMember &c : full.crew) if (c.status == CREW_FIT) fm += c.morale;
	CHECK(fm < em);

	// A hearing releases the innocent, and a conviction keeps them confined.
	Ship h = NewShip();
	CHECK(Brig(h, 12, true));
	CHECK(!Hearing(h, 12, true) || Brigged(h, 12)); // convicted: still in the brig
	CHECK(Brig(h, 13, true));
	CHECK(Hearing(h, 13, false) && !Brigged(h, 13)); // acquitted: released
	CHECK(!Hearing(h, 5, false));                    // nobody by that number is confined

	// It is all in the save.
	std::vector<uint8_t> blob = Pack(full);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(back.refugees == 40 && Describe(back) == Describe(full));
}

// Phenomena: an anomaly with hidden attributes, revealed one scan at a time, with a correct response
// that depends on the set revealed (docs/exploration-and-science.md).
static void TestPhenomenon()
{
	g_test = "phenomena: hidden attributes, one scan at a time, and the right response";
	Ship s = NewShip();
	SetAlert(s, ALERT_RED);
	Tick(s, 1.0f);
	int ph = -1;
	for (int i = 0; i < static_cast<int>(s.sector.size()); ++i) if (s.sector[i].phenomenon) ph = i;
	CHECK(ph >= 0);
	GoTo(s, ph);
	CHECK(RevealPhenomenon(s) == 1);
	CHECK(RevealPhenomenon(s) == 2);
	CHECK(RevealPhenomenon(s) == 3);
	CHECK((s.sector[ph].phenomAttrs & 0x7) == 0x7); // all three known
	CHECK(RevealPhenomenon(s) == 3);                 // scanning a resolved phenomenon is a no-op
	// The correct response is a function of the three attribute values: it can only be worked out
	// once they are all resolved (docs/exploration-and-science.md).
	const int vsum = s.sector[ph].phenomAttrVal[0] + s.sector[ph].phenomAttrVal[1] + s.sector[ph].phenomAttrVal[2];
	CHECK(s.sector[ph].phenomTruth == vsum % PHENOM_RESPONSE_COUNT);

	// The sensors are the instrument: dark, no attribute is resolved.
	s.sector[ph].phenomAttrs = 1;
	SetEnabled(s, SYS_SENSORS, false);
	Tick(s, 1.0f);
	CHECK(RevealPhenomenon(s) == -1);
	SetEnabled(s, SYS_SENSORS, true);
	Tick(s, 1.0f);

	// The correct response is the science reward; a wrong one is damage the crew can watch.
	const int truth = s.sector[ph].phenomTruth;
	const float mat = s.stores.materials;
	CHECK(RespondPhenomenon(s, truth) && s.stores.materials > mat);
	const int wrong = (truth + 1) % PHENOM_RESPONSE_COUNT;
	const float mat2 = s.stores.materials;
	int hullBefore = 0; for (const Deck &d : s.decks) hullBefore += static_cast<int>(d.hull * 1000);
	CHECK(!RespondPhenomenon(s, wrong));
	int hullAfter = 0; for (const Deck &d : s.decks) hullAfter += static_cast<int>(d.hull * 1000);
	CHECK(hullAfter < hullBefore && s.stores.materials == mat2);

	std::vector<uint8_t> blob = Pack(s);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(back.sector[ph].phenomAttrs == s.sector[ph].phenomAttrs);
}

// Probes: consumable exploration and the safe way to look at something hostile
// (docs/exploration-and-science.md).
static void TestProbes()
{
	g_test = "probes: the safe way to look at something hostile";
	Ship s = NewShip();
	SetAlert(s, ALERT_RED);          // red alert brings the launchers up
	Tick(s, 1.0f);
	CHECK(s.stores.probes == 6);
	const int target = s.sector[s.beacon].links.empty() ? 1 : s.sector[s.beacon].links[0];
	s.sector[target].visited = false;
	s.sector[target].surveyed = false;
	CHECK(LaunchProbe(s, target));
	CHECK(s.stores.probes == 5);
	CHECK(s.sector[target].surveyed); // its telemetry charts the target without the ship going
	CHECK(!LaunchProbe(s, s.beacon)); // not the beacon we are at

	SetEnabled(s, SYS_TORPEDO_LAUNCHERS, false);
	Tick(s, 1.0f);                    // the outputs settle: the launchers are down
	CHECK(!LaunchProbe(s, target));   // no launcher, no probe
	SetEnabled(s, SYS_TORPEDO_LAUNCHERS, true);
	Tick(s, 1.0f);
	s.stores.probes = 0;
	CHECK(!LaunchProbe(s, target));   // none left

	std::vector<uint8_t> blob = Pack(s);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(back.stores.probes == s.stores.probes);
}

// A dedicated survey to locate a dilithium source: the design's "survey, chart, detour".
static void TestLocateDilithium()
{
	g_test = "a survey can locate a dilithium source";
	Ship s = NewShip();
	SetAlert(s, ALERT_RED);
	Tick(s, 1.0f);
	const int at = LocateDilithium(s);
	CHECK(at >= 0);
	CHECK(s.sector[at].surveyed); // it is charted, so the crew can detour to it
	CHECK(s.sector[at].kind == BEACON_BELT || s.sector[at].kind == BEACON_TRADER || s.sector[at].kind == BEACON_DERELICT);

	// The sensors are the instrument: dark, no survey.
	Ship d = NewShip();
	SetAlert(d, ALERT_RED);
	Tick(d, 1.0f);
	SetEnabled(d, SYS_SENSORS, false);
	Tick(d, 1.0f);
	CHECK(LocateDilithium(d) == -1);
}

// The dilithium constraint: warp spends the crystal, recomposition buys some back until it cannot,
// and only a new crystal -- found by exploring -- resets it. No crystal, no warp.
static void TestDilithium()
{
	g_test = "dilithium: the constraint that forces exploration";

	Ship s = NewShip();
	Tick(s, 1.0f);
	CHECK(s.dilithium == 1.0f && s.crystalQuality == 1.0f);
	CHECK(DilithiumRange(s) == static_cast<int>(DILITHIUM_LIGHT_YEARS + 0.5f));
	CHECK(WarpPossible(s));

	// Warp use spends the crystal: every jump leaves less.
	GoTo(s, 0);
	float prev = s.dilithium;
	for (int i = 0; i < 3; ++i) {
		s.enemy = Enemy(); s.contact2 = Enemy();
		CHECK(Jump(s, s.beacon + 1));
		CHECK(s.dilithium < prev);
		prev = s.dilithium;
	}

	// Recomposition buys back life in the frame, but the ceiling falls each time; eventually it
	// cannot reach the crystal any more.
	Ship r = NewShip();
	Tick(r, 1.0f);
	r.dilithium = 0.2f;
	const float ceil0 = r.crystalCeiling;
	CHECK(Recomposite(r) && r.dilithium > 0.2f);
	CHECK(r.crystalCeiling < ceil0);
	for (int i = 0; i < 20; ++i) Recomposite(r);
	r.dilithium = r.crystalCeiling;
	CHECK(!Recomposite(r)); // the crystal is spent: only replacement will do

	// A new crystal, found by exploring. Mined from a belt; a bad trade is refused; research makes a
	// better crystal that shortens the journey.
	Ship m = NewShip();
	Tick(m, 1.0f);
	m.sector[m.beacon].kind = BEACON_BELT;
	m.dilithium = 0.1f;
	CHECK(AcquireDilithium(m, DIL_MINE) && m.dilithium == 1.0f && m.crystalReplacements == 1);
	CHECK(!AcquireDilithium(m, DIL_TRADE)); // no trader here
	Ship q = NewShip();
	Tick(q, 1.0f);
	CHECK(AcquireDilithium(q, DIL_RESEARCH) && q.crystalQuality > 1.0f);
	CHECK(DilithiumRange(q) > DilithiumRange(NewShip()));

	// No crystal, no warp: home stops getting closer, but the ship still runs.
	Ship none = NewShip();
	Tick(none, 1.0f);
	none.dilithium = 0.0f;
	none.enemy = Enemy();
	CHECK(!WarpPossible(none) && !Jump(none, none.beacon + 1));

	// It is all in the save (q's sector is unmodified, so the whole state round-trips).
	std::vector<uint8_t> blob = Pack(q);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(back.crystalQuality == q.crystalQuality);
	CHECK(Describe(back) == Describe(q));
}

// Every system has three named failure states, not just one (docs/failure-is-content.md).
static void TestSystemStates()
{
	g_test = "every system has three failure states, each change named";
	Ship s = NewShip();
	s.cfg.dayScale = 1.0f;
	Tick(s, 1.0f);
	s.stores.spareParts = 0.0f; // no parts, so damage control cannot quietly repair the state away
	const int id = SYS_SENSORS;
	CHECK(s.systems[id].fault == SYS_NOMINAL);
	DamageSystem(s, SYS_SENSORS, 0.4f);
	Tick(s, 1.0f);
	CHECK(s.systems[id].fault == SYS_DEGRADED);
	DamageSystem(s, SYS_SENSORS, 0.3f); // health 0.3: below a third, offline
	Tick(s, 1.0f);
	CHECK(s.systems[id].fault == SYS_OFFLINE);
	DamageSystem(s, SYS_SENSORS, 1.0f); // health 0: destroyed
	Tick(s, 1.0f);
	CHECK(s.systems[id].fault == SYS_DESTROYED);
	bool named = false;
	for (const LogEntry &e : s.log) { if (e.what.find("degraded") != std::string::npos) named = true; }
	CHECK(named);

	// A disabled system is offline even at full health, and returns to nominal when switched back.
	Ship d = NewShip();
	d.cfg.dayScale = 1.0f;
	Tick(d, 1.0f);
	SetEnabled(d, SYS_SENSORS, false);
	Tick(d, 1.0f);
	CHECK(d.systems[SYS_SENSORS].fault == SYS_OFFLINE);
	SetEnabled(d, SYS_SENSORS, true);
	Tick(d, 1.0f);
	CHECK(d.systems[SYS_SENSORS].fault == SYS_NOMINAL);

	std::vector<uint8_t> blob = Pack(s);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(back.systems[id].fault == s.systems[id].fault);
}

// Injury causes beyond air, fire, fighting wounds and radiation: a poisoned site, and an exploding
// console (docs/failure-is-content.md, S6's "more injury causes").
static void TestHazardInjuries()
{
	g_test = "injury causes beyond air, fire, wounds and radiation";

	// A poisoned site: beaming to a phenomenon hazards the away team.
	Ship s = NewShip();
	SetAlert(s, ALERT_RED);
	Tick(s, 1.0f);
	int ph = -1;
	for (int i = 0; i < static_cast<int>(s.sector.size()); ++i) if (s.sector[i].phenomenon) ph = i;
	CHECK(ph >= 0);
	GoTo(s, ph);
	s.shieldStrength = 0.0f;
	CHECK(TransportAway(s, 3));
	bool siteHurt = false;
	for (const LogEntry &e : s.log) if (e.what.find("was hurt by the site") != std::string::npos) siteHurt = true;
	CHECK(siteHurt);

	// An exploding console: a hit on a manned station's deck hurts whoever was there.
	Ship c = NewShip();
	c.cfg.dayScale = 1.0f; // so a short tick is one exchange of fire, not sixty
	SetAlert(c, ALERT_RED);
	Tick(c, 1.0f);
	const int hostile = FirstOfKind(c, BEACON_HOSTILE);
	GoTo(c, hostile - 1);
	c.enemy = Enemy();
	Jump(c, hostile);
	SetEnabled(c, SYS_SHIELDS, false); // our shields stay down: the hits land
	Tick(c, 1.0f);
	c.enemy.kind = ENEMY_WARSHIP; c.enemy.hull = 1.0f; c.enemy.shields = 0.0f; c.enemy.shieldGen = 0.0f;
	c.enemy.firepower = 0.8f;
	// Read the log each step, before it is bounded by later events.
	bool consoleHurt = false;
	for (int i = 0; i < 240 && !consoleHurt; ++i) {
		Tick(c, 10.0f);
		for (const LogEntry &e : c.log) if (e.what.find("exploding console") != std::string::npos) consoleHurt = true;
	}
	CHECK(consoleHurt);
}

// The warp core cascade: coolant loss -> overheat -> falling containment -> a breach countdown, with
// several interrupts, and a breach as the one unwinnable end (docs/failure-is-content.md).
static void TestCoreCascade()
{
	g_test = "the warp core cascade: a chain, and a wall";
	Ship s = NewShip();
	s.cfg.dayScale = 1.0f;
	Tick(s, 1.0f);
	DamageSystem(s, SYS_WARP_DRIVE, 0.8f); // a badly hurt core loses coolant and heats
	Tick(s, 1.0f);
	const float coolant0 = s.coolant;
	for (int i = 0; i < 24; ++i) Tick(s, 60.0f); // a few hours
	CHECK(s.coolant < coolant0 && s.coreTemp > 0.0f);

	// If nothing is done, containment falls and a breach countdown begins.
	Ship doomed = NewShip();
	doomed.cfg.dayScale = 1.0f;
	Tick(doomed, 1.0f);
	for (int i = 0; i < 5000 && doomed.breachCountdown < 0.0f && !CoreBreached(doomed); ++i) { DamageSystem(doomed, SYS_WARP_DRIVE, 0.4f); Tick(doomed, 60.0f); }
	CHECK(doomed.breachCountdown >= 0.0f || CoreBreached(doomed));

	// Interrupt: shut the core down and it stabilises (but warp is gone until restarted).
	Ship k = NewShip();
	k.cfg.dayScale = 1.0f;
	Tick(k, 1.0f);
	DamageSystem(k, SYS_WARP_DRIVE, 0.8f);
	for (int i = 0; i < 500 && k.containment > CONTAINMENT_CRITICAL; ++i) Tick(k, 60.0f);
	Tick(k, 1.0f);
	CHECK(k.breachCountdown >= 0.0f);
	CHECK(ShutDownCore(k) && !WarpPossible(k));
	for (int i = 0; i < 500 && k.containment <= CONTAINMENT_CRITICAL; ++i) Tick(k, 60.0f);
	CHECK(k.containment > CONTAINMENT_CRITICAL && k.breachCountdown < 0.0f);
	for (int i = 0; i < 500 && k.containment < 0.6f; ++i) Tick(k, 60.0f);
	CHECK(RestartCore(k));

	// Eject is the last resort: no warp until a new core is found.
	Ship e = NewShip();
	e.cfg.dayScale = 1.0f;
	Tick(e, 1.0f);
	CHECK(EjectCore(e) && e.coreEjected && !WarpPossible(e));

	// Abandoned past the countdown: the one unwinnable end.
	Ship l = NewShip();
	l.cfg.dayScale = 1.0f;
	Tick(l, 1.0f);
	for (int i = 0; i < 8000 && !CoreBreached(l); ++i) { DamageSystem(l, SYS_WARP_DRIVE, 0.4f); Tick(l, 60.0f); }
	CHECK(CoreBreached(l));

	// The cascade is in the save.
	DamageSystem(k, SYS_WARP_DRIVE, 0.2f);
	Tick(k, 1.0f);
	std::vector<uint8_t> blob = Pack(k);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(back.coreShutdown == k.coreShutdown);
}

// The general mechanism (docs/failure-is-content.md): condition sets the odds, stress sets the
// severity, and the top tenth of capability is nominal. Measured, not asserted.
static void TestConditionOdds()
{
	g_test = "condition sets the odds, and stress sets the severity";
	// The top tenth is nominal: no anomaly of any kind, on any draw.
	CHECK(AnomalyOdds(1.0f) == 0.0f);
	CHECK(AnomalyOdds(NOMINAL_CONDITION) == 0.0f);
	CHECK(AnomalyOdds(0.95f) == 0.0f);
	CHECK(AnomalyOdds(0.85f) > 0.0f);
	CHECK(AnomalyOdds(0.0f) == ANOMALY_ODDS_MAX);
	float prev = -1.0f;
	for (float c = 1.0f; c >= 0.0f; c -= 0.05f) { const float o = AnomalyOdds(c); CHECK(o >= prev - 1e-6f); prev = o; }

	// The measurement: twenty thousand draws at each condition. Nominal never rolls; the odds rise as
	// the condition falls, and the measured frequency tracks the function.
	auto measure = [](float condition) {
		int hits = 0;
		for (uint32_t i = 0; i < 20000; ++i)
			if (RollAnomaly(condition, 1.0f, i * 2654435761u + 12345u) != ANOMALY_NONE) ++hits;
		return hits;
	};
	CHECK(measure(0.95f) == 0);
	const int at62 = measure(0.62f), at40 = measure(0.40f), at00 = measure(0.0f);
	CHECK(at62 > 0);
	CHECK(at40 > at62);
	CHECK(at00 > at40);
	CHECK(at00 > 15000); // toward the maximum
	CHECK(std::fabs(at40 / 20000.0f - AnomalyOdds(0.40f)) < 0.03f);

	// The ladder: the same degraded system is worse under load than it is light.
	CHECK(AnomalySeverityFor(0.40f, 0.15f) == ANOMALY_DEGRADED);   // 40% run light: a misaligned beam
	CHECK(AnomalySeverityFor(0.40f, 1.0f) == ANOMALY_CATASTROPHIC); // ... at battle stations: the worst
	CHECK(AnomalySeverityFor(0.40f, 1.0f) > AnomalySeverityFor(0.40f, 0.15f));
	for (float st = 0.1f; st <= 1.0f; st += 0.1f)
		CHECK(AnomalySeverityFor(0.3f, st) >= AnomalySeverityFor(0.3f, st - 0.1f)); // monotonic in load
	for (float c = 0.1f; c <= 1.0f; c += 0.1f)
		CHECK(AnomalySeverityFor(c, 1.0f) <= AnomalySeverityFor(c - 0.1f, 1.0f));    // ... and in condition
	CHECK(RollAnomaly(0.95f, 1.0f, 0u) == ANOMALY_NONE); // the top tenth, whatever the load

	// UseSystem advances the deterministic draw counter, writes the chain, and lets the system go.
	Ship u = NewShip();
	u.cfg.dayScale = 1.0f;
	SetAlert(u, ALERT_RED);
	Tick(u, 1.0f);
	const uint32_t before = u.riskRolls;
	int anomalies = 0;
	for (int i = 0; i < 200; ++i) {
		u.systems[SYS_TRANSPORTERS].health = 0.4f;
		u.systems[SYS_TRANSPORTERS].output = 0.4f;
		if (UseSystem(u, SYS_TRANSPORTERS, 1.0f, "the operator") != ANOMALY_NONE) ++anomalies;
	}
	CHECK(u.riskRolls == before + 200);
	CHECK(anomalies > 0);
	bool chain = false;
	for (const LogEntry &e : u.log)
		if (e.who == "the operator" && e.what.find("condition under") != std::string::npos) chain = true;
	CHECK(chain);
}

// The transporter, worked end to end: the instrument states the condition before the act; a nominal
// beam mangles no one over a long run; a degraded one under load does, and writes to the records.
static void TestTransporterAnomaly()
{
	g_test = "the transporter: the ruling end to end";
	Ship s = NewShip();
	s.cfg.dayScale = 1.0f;
	s.shieldStrength = 0.0f;
	s.sector[s.beacon].phenomenon = false;
	s.sector[s.beacon].kind = BEACON_EMPTY;
	SetAlert(s, ALERT_GREEN);
	Tick(s, 1.0f);
	CHECK(s.systems[SYS_TRANSPORTERS].health == 1.0f);

	// The instrument, read before the act: this system's condition, and the threshold.
	const float nominalCond = SystemCondition(s.systems[SYS_TRANSPORTERS]);
	CHECK(nominalCond >= NOMINAL_CONDITION);
	CHECK(TransporterConditionLine(s).find("pattern buffer") != std::string::npos);
	CHECK(TransporterConditionLine(s).find("nominal") != std::string::npos);
	s.systems[SYS_TRANSPORTERS].health = 0.62f; // the document's own example
	s.systems[SYS_TRANSPORTERS].output = 0.62f;
	const std::string degradedLine = TransporterConditionLine(s);
	CHECK(degradedLine.find("62%") != std::string::npos);
	CHECK(degradedLine.find("below 40") != std::string::npos);
	s.systems[SYS_TRANSPORTERS].health = 1.0f;
	s.systems[SYS_TRANSPORTERS].output = 1.0f;

	// Nominal: a hundred beams out and back, and no anomaly of any kind; the crew unchanged.
	const size_t crew0 = s.crew.size();
	int anomalies = 0;
	for (int i = 0; i < 100; ++i) {
		std::string note;
		CHECK(TransportAway(s, 3, &note));
		if (!note.empty()) ++anomalies;
		CHECK(TransportBack(s, &note));
		if (!note.empty()) ++anomalies;
	}
	CHECK(anomalies == 0);
	CHECK(s.crew.size() == crew0);
	for (const CrewMember &c : s.crew) CHECK(c.status == CREW_FIT);

	// Degraded and under load: the same system now gets it wrong, and the outcome family appears.
	// The condition is held at 40% so the test measures the ladder, not the decay it causes.
	SetAlert(s, ALERT_RED);
	bool degraded = false, catastrophic = false;
	for (int i = 0; i < 400 && !(degraded && catastrophic); ++i) {
		s.systems[SYS_TRANSPORTERS].health = 0.4f;
		s.systems[SYS_TRANSPORTERS].output = 0.4f;
		std::string note;
		CHECK(TransportAway(s, 3, &note));
		if (note.find("misaligned") != std::string::npos || note.find("mangled") != std::string::npos) degraded = true;
		if (note.find("copy") != std::string::npos || note.find("merged") != std::string::npos) catastrophic = true;
		CHECK(TransportBack(s, &note));
		if (note.find("misaligned") != std::string::npos || note.find("mangled") != std::string::npos) degraded = true;
		if (note.find("copy") != std::string::npos || note.find("merged") != std::string::npos) catastrophic = true;
	}
	CHECK(degraded && catastrophic); // both ends of the ladder were reached
	bool recorded = false;
	for (const LogEntry &e : s.log) if (e.what.find("condition under") != std::string::npos) recorded = true;
	CHECK(recorded); // the chain -- condition, load, what happened -- is in the record

	// A copy or a merge changes the records, and the save round-trips them. (The un-ticked state
	// above is not byte-comparable: a zero tick runs triage, which can move a casualty's wounds, so
	// the round trip is checked for stability instead.)
	std::vector<uint8_t> blob = Pack(s);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(back.crew.size() == s.crew.size());
	std::vector<uint8_t> blob2 = Pack(back);
	Ship back2;
	CHECK(Unpack(blob2.data(), blob2.size(), back2));
	CHECK(Pack(back2) == blob2);
	CHECK(back2.crew.size() == s.crew.size());
	if (s.crew.size() > static_cast<size_t>(COMPLEMENT))
		CHECK(back2.crew[COMPLEMENT].name == s.crew[COMPLEMENT].name); // the copy carries its name
}

// The security squad: a fireteam command sends to retake a deck, advancing a deck at a time
// (docs/borg-incursion.md).
static void TestSquad()
{
	g_test = "the security squad: advancing to retake a deck";
	Ship s = NewShip();
	s.cfg.dayScale = 1.0f;
	SetRole(s, ROLE_IN_COMMAND);
	SetAlert(s, ALERT_RED);
	Tick(s, 1.0f);
	const int d = 11; // engineering, two decks from the muster (9)
	s.decks[d - 1].intruders = 3.0f;
	s.decks[d - 1].boarderKind = BOARDER_RAIDER;
	CHECK(OrderEvacuate(s, d));       // nobody else fights
	CHECK(OrderAdvance(s, d));
	CHECK(s.advanceDeck == d && s.advanceAt == 9);
	Tick(s, SQUAD_TRAVEL_MINUTES * 60.0f * 0.5f);
	CHECK(s.advanceAt != d);          // still en route
	for (int i = 0; i < 400 && s.advanceDeck != 0; ++i) Tick(s, 60.0f);
	CHECK(s.advanceDeck == 0);        // stood down
	CHECK(s.decks[d - 1].intruders == 0.0f); // deck retaken

	// It needs whoever commands.
	Ship n = NewShip();
	Tick(n, 1.0f);
	CHECK(!OrderAdvance(n, 5));

	// In the save.
	Ship b = NewShip();
	SetRole(b, ROLE_IN_COMMAND);
	Tick(b, 1.0f);
	CHECK(OrderAdvance(b, 5));
	std::vector<uint8_t> blob = Pack(b);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(back.advanceDeck == b.advanceDeck && back.advanceAt == b.advanceAt);
}

// Force fields rated 1-10 (docs/borg-incursion.md): a field holds intruders and drains under
// pressure; a level-10 field cuts a drone from the Collective.
static void TestForceFieldKit()
{
	g_test = "force fields rated: a field holds, drains, and a level-10 field cuts the Collective";
	Ship s = NewShip();
	s.cfg.dayScale = 1.0f;
	SetRole(s, ROLE_IN_COMMAND);
	Tick(s, 1.0f);
	const int d = 4; // the transporter deck
	s.decks[d - 1].intruders = 2.0f;
	s.decks[d - 1].boarderKind = BOARDER_RAIDER;
	CHECK(OrderEvacuate(s, d));
	SetForceFieldLevel(s, d, FIELD_MAX);
	Tick(s, 60.0f);                                     // a minute held
	CHECK(s.systems[SYS_TRANSPORTERS].control == 1.0f); // no hack: the field holds them
	CHECK(!s.decks[d - 1].compromised && s.decks[d - 1].dwell == 0.0f);
	CHECK(s.decks[d - 1].forceFieldLevel < static_cast<float>(FIELD_MAX)); // and it drained
	for (int i = 0; i < 200 && s.decks[d - 1].forceField; ++i) Tick(s, 60.0f);
	CHECK(!s.decks[d - 1].forceField);                  // under pressure it fails

	// A level-10 field on a Borg deck cuts them from the Collective: adaptation is suppressed.
	Ship b = NewShip();
	b.cfg.dayScale = 1.0f;
	Tick(b, 1.0f);
	b.decks[d - 1].intruders = 2.0f; b.decks[d - 1].borg = true; b.decks[d - 1].boarderKind = BOARDER_BORG;
	SetForceFieldLevel(b, d, FIELD_MAX);
	Tick(b, 1.0f);
	CHECK(b.adaptationSuppressed > 0.0f);

	SetForceFieldLevel(b, d, 99);
	CHECK(b.decks[d - 1].forceFieldLevel == static_cast<float>(FIELD_MAX)); // clamped
	SetForceFieldLevel(b, d, 0);
	CHECK(!b.decks[d - 1].forceField);

	std::vector<uint8_t> blob = Pack(s);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(back.decks[d - 1].forceFieldLevel == s.decks[d - 1].forceFieldLevel);
}

// De-assimilation: reversible only in a narrow window, and never whole (docs/borg-incursion.md).
static void TestDeassimilation()
{
	g_test = "de-assimilation: a narrow window, and never whole";
	Ship s = NewShip();
	Tick(s, 1.0f);
	const int who = 10;
	s.crew[who].wounds = 0.5f;
	s.crew[who].status = CREW_INJURED;
	const float sup = s.stores.medicalSupplies;
	CHECK(RecoverCaptive(s, who));
	CHECK(s.crew[who].wounds == 0.0f && s.crew[who].status == CREW_INJURED);
	CHECK(s.crew[who].assimScar >= 0.5f);
	CHECK(s.stores.medicalSupplies < sup);

	// Past the window it is too late, and without supplies nothing can be done.
	Ship late = NewShip();
	Tick(late, 1.0f);
	late.crew[who].wounds = 0.9f; late.crew[who].status = CREW_INJURED;
	CHECK(!RecoverCaptive(late, who));
	Ship poor = NewShip();
	Tick(poor, 1.0f);
	poor.crew[who].wounds = 0.5f; poor.crew[who].status = CREW_INJURED;
	poor.stores.medicalSupplies = 0.0f;
	CHECK(!RecoverCaptive(poor, who));

	// The residue never fully fades: a scarred ship's crew are worse off than a clean one's.
	Ship scarred = NewShip(), clean = NewShip();
	Tick(scarred, 1.0f); Tick(clean, 1.0f);
	scarred.crew[who].assimScar = 1.0f;
	Tick(scarred, Hours(scarred, 24.0f)); Tick(clean, Hours(clean, 24.0f));
	CHECK(scarred.crew[who].morale < clean.crew[who].morale);

	// It is all in the save.
	std::vector<uint8_t> blob = Pack(s);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(back.crew[who].assimScar == s.crew[who].assimScar);
}

// The counter-play kit: the phaser adapter's rotating modulation and a vinculum raid
// (docs/borg-incursion.md). Adaptation cannot be absolute, and the crew can buy their weapons back.
static void TestCounterPlay()
{
	g_test = "the counter-play kit: remodulation and a vinculum raid";

	auto DamageOne = [](Ship &sh) { const float h = sh.enemy.hull; FireTorpedo(sh); return h - sh.enemy.hull; };

	Ship s = NewShip();
	s.cfg.dayScale = 1.0f;
	Tick(s, 1.0f);
	const int hostile = FirstOfKind(s, BEACON_HOSTILE);
	GoTo(s, hostile - 1);
	s.enemy = Enemy();
	Jump(s, hostile);
	s.enemy.kind = ENEMY_BORG_VESSEL; s.enemy.borg = true;
	s.enemy.hull = 1.0f; s.enemy.shields = 0.0f; s.enemy.shieldGen = 0.0f; s.enemy.firepower = 0.0f;
	s.enemy.adaptation = 0.6f;
	SetAlert(s, ALERT_RED);
	Tick(s, 1.0f);                             // red alert brings the weapons up

	const float withLock = DamageOne(s);       // they have adapted: little gets through
	s.enemy.adaptation = 0.6f; s.enemy.hull = 1.0f; s.stores.torpedoes = 38;
	CHECK(Remodulate(s));                      // rotate the modulation
	CHECK(s.enemy.adaptation < 0.2f);
	CHECK(DamageOne(s) > withLock);            // and now the shots land again
	CHECK(!Remodulate(s));                     // but not every second: it costs attention
	Tick(s, REMODULATE_COOLDOWN + 1.0f);
	CHECK(Remodulate(s));

	// A vinculum raid needs Borg here and spends a security party; while it is down they do not adapt.
	Ship v = NewShip();
	v.cfg.dayScale = 1.0f;
	Tick(v, 1.0f);
	const int h2 = FirstOfKind(v, BEACON_HOSTILE);
	GoTo(v, h2 - 1); v.enemy = Enemy(); Jump(v, h2);
	v.enemy.kind = ENEMY_BORG_VESSEL; v.enemy.borg = true; v.enemy.hull = 1.0f;
	v.enemy.adaptation = 0.5f; v.enemy.shields = 0.0f; v.enemy.shieldGen = 0.0f; v.enemy.firepower = 0.0f;
	SetAlert(v, ALERT_RED);
	Tick(v, 1.0f);                             // red alert brings the weapons up
	CHECK(RaidVinculum(v));
	CHECK(v.enemy.adaptation == 0.0f && v.adaptationSuppressed > 0.0f);
	bool hurt = false;
	for (const CrewMember &c : v.crew) if (c.status == CREW_INJURED && c.dept == DEPT_SECURITY) hurt = true;
	CHECK(hurt);
	DamageOne(v);
	CHECK(v.enemy.adaptation == 0.0f);         // severed: no adaptation while it is down
	CHECK(!RaidVinculum(v));                    // already suppressed
}

// Shuttles: supported, not pilotable. The bay's contents, an away shuttle and its manifest, losing
// one permanently as a build job, and a hit on the bay (docs/shuttles.md).
static void TestShuttles()
{
	g_test = "shuttles: the bay's contents and what reads them";

	Ship s = NewShip();
	Tick(s, 1.0f);
	CHECK(ShuttlesInBay(s) == 4 && ShuttlesAway(s) == 0);
	CHECK(ShuttleByClass(s, SHUTTLE_CLASS2) && ShuttleByClass(s, SHUTTLE_CLASS2)->location == SHUTTLE_IN_BAY);

	std::vector<int> manifest;
	for (size_t i = 0; i < s.crew.size() && manifest.size() < 3; ++i) if (s.crew[i].status == CREW_FIT) manifest.push_back(static_cast<int>(i));
	const float mat = s.stores.materials;
	CHECK(LaunchShuttle(s, SHUTTLE_TYPE6, 3, manifest));
	CHECK(ShuttlesAway(s) == 1 && ShuttlesInBay(s) == 3);
	CHECK(ShuttleByClass(s, SHUTTLE_TYPE6)->manifest.size() == manifest.size());
	for (int i : manifest) CHECK(s.crew[i].away);
	CHECK(s.stores.materials < mat);
	CHECK(!LaunchShuttle(s, SHUTTLE_TYPE6, 4, manifest)); // already away, not in the bay

	// Save and load keeps the bay correct, an away shuttle and its manifest included.
	std::vector<uint8_t> blob = Pack(s);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(ShuttlesInBay(back) == 3 && ShuttlesAway(back) == 1);
	CHECK(ShuttleByClass(back, SHUTTLE_TYPE6)->location == SHUTTLE_AWAY && ShuttleByClass(back, SHUTTLE_TYPE6)->awayBeacon == 3);
	CHECK(ShuttleByClass(back, SHUTTLE_TYPE6)->manifest.size() == manifest.size());

	// Recall: home, and the crew are aboard again.
	CHECK(RecallShuttle(s, SHUTTLE_TYPE6) && ShuttlesInBay(s) == 4);
	for (int i : manifest) CHECK(!s.crew[i].away);

	// Losing one is permanent: the crew are lost with it and a replacement becomes a build job.
	Ship l = NewShip();
	Tick(l, 1.0f);
	std::vector<int> m2;
	for (size_t i = 0; i < l.crew.size() && m2.size() < 2; ++i) if (l.crew[i].status == CREW_FIT) m2.push_back(static_cast<int>(i));
	CHECK(LaunchShuttle(l, SHUTTLE_TYPE8, 5, m2));
	CHECK(LoseShuttle(l, SHUTTLE_TYPE8));
	CHECK(ShuttleByClass(l, SHUTTLE_TYPE8)->location == SHUTTLE_LOST);
	for (int i : m2) CHECK(l.crew[i].status == CREW_DEAD);
	bool buildJob = false;
	for (const Job &j : Jobs(l)) if (j.kind == JOB_BUILD && j.target == -(SHUTTLE_TYPE8 + 1)) buildJob = true;
	CHECK(buildJob);

	// Stranded: the crew beam back and the shuttle is left behind -- a loss with a location.
	Ship st = NewShip();
	Tick(st, 1.0f);
	CHECK(LaunchShuttle(st, SHUTTLE_CLASS2, 2, std::vector<int>()));
	CHECK(StrandShuttle(st, SHUTTLE_CLASS2));
	CHECK(ShuttleByClass(st, SHUTTLE_CLASS2)->location == SHUTTLE_LOST && ShuttleByClass(st, SHUTTLE_CLASS2)->awayBeacon == 2);

	// A hit on the bay damages what is parked in it.
	Ship h = NewShip();
	Tick(h, 1.0f);
	CHECK(ShuttleBayHit(h, 0.6f));
	CHECK(ShuttleByClass(h, SHUTTLE_CLASS2)->condition < 1.0f);

	// The replacement: work the build job with material, and the second bay builds a new one.
	Ship b = NewShip();
	Tick(b, 1.0f);
	CHECK(LaunchShuttle(b, SHUTTLE_CLASS2, 1, std::vector<int>()));
	CHECK(LoseShuttle(b, SHUTTLE_CLASS2));
	b.stores.materials = 200.0f;
	for (int i = 0; i < 40 && ShuttleByClass(b, SHUTTLE_CLASS2)->location == SHUTTLE_LOST; ++i) Tick(b, Hours(b, 1.0f));
	CHECK(ShuttleByClass(b, SHUTTLE_CLASS2)->location == SHUTTLE_IN_BAY);
}

// The Borg incursion's keystone: who holds a deck, whether its systems are compromised, and the
// clean intercept recorded as a win (docs/borg-incursion.md).
static void TestIncursion()
{
	g_test = "the incursion: controller, compromise and the clean intercept";

	// A guard beats a single boarder before any system is touched: a clean intercept, recorded.
	Ship s = NewShip();
	SetRole(s, ROLE_IN_COMMAND);
	Tick(s, 1.0f);
	const int deck = 4;
	s.decks[deck - 1].intruders = 1.0f;
	s.decks[deck - 1].boarderKind = BOARDER_RAIDER;
	CHECK(OrderSecurityTo(s, deck));
	const int before = s.cleanIntercepts;
	for (int i = 0; i < 300 && s.decks[deck - 1].intruders > 0.0f; ++i) Tick(s, 5.0f);
	CHECK(s.decks[deck - 1].intruders == 0.0f);
	CHECK(s.cleanIntercepts == before + 1);           // the win is recorded
	CHECK(!s.decks[deck - 1].compromised);
	CHECK(s.decks[deck - 1].controller == CTRL_CREW);

	// No defenders: the boarders write to the systems, and the deck is compromised and contested.
	Ship b = NewShip();
	SetRole(b, ROLE_IN_COMMAND);
	Tick(b, 1.0f);
	const int d2 = 1;                                  // the bridge: a deck with workable systems
	b.decks[d2 - 1].intruders = 4.0f;
	b.decks[d2 - 1].boarderKind = BOARDER_RAIDER;
	CHECK(OrderEvacuate(b, d2));                       // nobody stays to fight
	for (int i = 0; i < 60; ++i) Tick(b, 60.0f);        // a ship-hour of hacking
	CHECK(b.decks[d2 - 1].compromised);
	CHECK(b.decks[d2 - 1].controller == CTRL_CONTESTED);
	CHECK(b.decks[d2 - 1].dwell > 0.0f);
	CHECK(b.systems[SYS_COMMUNICATIONS].control < 1.0f); // the bridge's comms, under their hands

	// No dwell, no write: an unopposed boarder below the threshold writes nothing; past it, writes.
	Ship g = NewShip();
	g.cfg.dayScale = 1.0f;                              // so Tick seconds are ship seconds
	SetRole(g, ROLE_IN_COMMAND);
	Tick(g, 1.0f);
	const int dg = 1; // the bridge: workable systems
	g.decks[dg - 1].intruders = 4.0f;
	CHECK(OrderEvacuate(g, dg));                        // unopposed
	Tick(g, DWELL_COMPROMISE * 0.5f);                   // half the threshold
	CHECK(g.systems[SYS_COMMUNICATIONS].control == 1.0f && !g.decks[dg - 1].compromised);
	Tick(g, DWELL_COMPROMISE);                          // cross it
	CHECK(g.systems[SYS_COMMUNICATIONS].control < 1.0f && g.decks[dg - 1].compromised);

	// The three thresholds: compromise begins, then they hold ground, then seizure outright.
	Ship t = NewShip();
	t.cfg.dayScale = 1.0f;
	SetRole(t, ROLE_IN_COMMAND);
	Tick(t, 1.0f);
	const int dt = 4; // the transporter deck
	t.decks[dt - 1].intruders = 4.0f;
	t.decks[dt - 1].boarderKind = BOARDER_RAIDER;
	CHECK(OrderEvacuate(t, dt));

	t.decks[dt - 1].dwell = DWELL_COMPROMISE - 0.5f;
	t.systems[SYS_TRANSPORTERS].control = 1.0f;
	Tick(t, 1.0f);                                     // crosses the first threshold
	CHECK(t.decks[dt - 1].compromised);
	CHECK(t.decks[dt - 1].controller == CTRL_CREW);    // compromised, but the crew still hold

	t.decks[dt - 1].dwell = DWELL_HOLD - 0.5f;
	Tick(t, 1.0f);                                     // crosses the second
	CHECK(t.decks[dt - 1].controller == CTRL_CONTESTED);

	t.decks[dt - 1].dwell = DWELL_SEIZE - 0.5f;
	t.systems[SYS_TRANSPORTERS].control = 1.0f;
	Tick(t, 1.0f);                                     // crosses the third
	CHECK(t.systems[SYS_TRANSPORTERS].control == 0.0f); // seized outright

	// The fields are in the save.
	Ship back;
	std::vector<uint8_t> blob = Pack(b);
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(back.decks[d2 - 1].compromised && back.cleanIntercepts == b.cleanIntercepts);
}

// The holodeck's uses, trauma read by the simulation, and living conditions.
static void TestHolodeckAndQuarters()
{
	g_test = "the holodeck, trauma and quarters";

	// The holodeck does nothing with the system down.
	Ship down = NewShip();
	SetEnabled(down, SYS_HOLODECKS, false);
	Tick(down, 1.0f);
	CHECK(!RunHolodeck(down, HOLO_RECREATION, 0));

	Ship s = NewShip();
	Tick(s, 1.0f);
	// Recreation lifts the mood.
	s.crew[0].morale = 0.5f;
	CHECK(RunHolodeck(s, HOLO_RECREATION, 0) && s.crew[0].morale > 0.5f);
	// Training grants the credential for the crew member's own department station.
	CHECK(!Qualified(s.crew[0], STN_CONN));  // Janeway, command: the Conn is the department's station
	CHECK(RunHolodeck(s, HOLO_TRAINING, 0) && Qualified(s.crew[0], STN_CONN));
	CHECK(!RunHolodeck(s, HOLO_TRAINING, 0)); // already qualified
	// Therapy fades what a character carries.
	Remember(s, 3, MEM_DEATH, 5, MEM_SAW, -0.9f);
	const float before = Trauma(s.crew[3]);
	CHECK(before > 0.5f);
	CHECK(RunHolodeck(s, HOLO_THERAPY, 3));
	CHECK(Trauma(s.crew[3]) < before);
	// Forensic reconstruction reports what they hold.
	CHECK(RunHolodeck(s, HOLO_FORENSIC, 3));

	// Trauma drags on the mood: a crew carrying a death is worse off than one that is not.
	Ship calm = NewShip(), hurt = NewShip();
	for (CrewMember &c : hurt.crew) Remember(hurt, static_cast<int>(&c - hurt.crew.data()), MEM_DEATH, 5, MEM_SAW, -0.9f);
	Tick(calm, Hours(calm, 24.0f));
	Tick(hurt, Hours(hurt, 24.0f));
	float cm = 0.0f, hm = 0.0f;
	for (const CrewMember &c : calm.crew) if (c.status == CREW_FIT) cm += c.morale;
	for (const CrewMember &c : hurt.crew) if (c.status == CREW_FIT) hm += c.morale;
	CHECK(hm < cm);

	// Living conditions: better quarters, at a cost in material.
	Ship q = NewShip();
	q.stores.materials = 50.0f;
	const float quality = q.crew[0].quartersQuality;
	CHECK(ImproveQuarters(q) && q.crew[0].quartersQuality > quality && q.stores.materials == 40.0f);
	Ship poor = NewShip();
	poor.stores.materials = 0.0f;
	CHECK(!ImproveQuarters(poor));

	// The nacelle pylons: a hit can take them, and a ship without them cannot go to warp.
	Ship py = NewShip();
	CHECK(PylonsIntact(py));
	DamagePylon(py, 0.6f);
	CHECK(!PylonsIntact(py));
	CHECK(!Jump(py, py.sector[py.beacon].links.front()));

	// The mobile emitter lets the EMH work harder away from sickbay.
	auto prep = [](Ship &x) {
		for (CrewMember &c : x.crew) if (c.dept == DEPT_MEDICAL) c.status = CREW_DEAD;
		SetEnabled(x, SYS_SICKBAY, false);
		Tick(x, 1.0f);
		x.crew[3].status = CREW_INJURED; x.crew[3].severity = 0.5f;
		ActivateEMH(x, true);
	};
	Ship noEmitter = NewShip(); prep(noEmitter);
	Ship emitter = NewShip(); prep(emitter); SetMobileEmitter(emitter, true);
	Tick(noEmitter, Hours(noEmitter, 2.0f));
	Tick(emitter, Hours(emitter, 2.0f));
	CHECK(emitter.crew[3].recovery > noEmitter.crew[3].recovery);

	// The program that will not end: too much time in it and a crew member stops standing watch;
	// command pulls them out.
	Ship h = NewShip();
	for (int i = 0; i < 4; ++i) CHECK(RunHolodeck(h, HOLO_RECREATION, 7));
	CHECK(h.crew[7].holoCompulsion >= 1.0f);
	Tick(h, 1.0f);
	CHECK(h.crew[7].activity == ACT_RECREATION);
	CHECK(EndHolodeckProgram(h, 7) && h.crew[7].holoCompulsion == 0.0f);

	// It is all in the save.
	std::vector<uint8_t> blob = Pack(s);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(std::fabs(back.crew[0].quartersQuality - s.crew[0].quartersQuality) < 1e-4f);
	CHECK(Qualified(back.crew[0], STN_CONN) && Describe(back) == Describe(s));
}

// EVA, Prime Directive contact, and the Maquis arc.
static void TestEVAAndContact()
{
	g_test = "EVA, first contact and the Maquis arc";

	// EVA: a suited party already out reaches a belt the systems cannot, once.
	Ship s = NewShip();
	s.sector[s.beacon].kind = BEACON_BELT;
	s.sector[s.beacon].looted = false;
	CHECK(TransportAway(s, 3)); // a party on the site
	Tick(s, 0.0f);
	const float mat = s.stores.materials;
	CHECK(EVA(s) && s.stores.materials == mat + BELT_MATERIALS);
	CHECK(!EVA(s));             // worked out
	Ship none = NewShip();
	none.sector[none.beacon].kind = BEACON_BELT;
	CHECK(!EVA(none));          // no party out

	// First contact: observing is free and good; interfering takes, and costs.
	Ship obs = NewShip();
	obs.sector[obs.beacon].kind = BEACON_PREWARP;
	const float m0 = obs.stores.materials;
	CHECK(ObservePreWarp(obs) && obs.stores.materials > m0);
	CHECK(!Recall(obs.crew[0], MEM_VIOLATION));
	Ship vio = NewShip();
	vio.sector[vio.beacon].kind = BEACON_PREWARP;
	CHECK(InterferePreWarp(vio));
	CHECK(Recall(vio.crew[0], MEM_VIOLATION));   // the whole crew carry it
	CHECK(Resentment(vio) > Resentment(obs));    // and the crew are divided

	// The Maquis arc: resentment can be worked through, and when it is gone the split is too.
	Ship r = NewShip();
	r.resentment = 0.5f;
	Ship nobody = NewShip();
	CHECK(!ReconcileFactions(nobody));           // nobody in particular commands
	SetRole(r, ROLE_IN_COMMAND);
	CHECK(ReconcileFactions(r) && Resentment(r) < 0.5f);
	for (int k = 0; k < 10; ++k) ReconcileFactions(r);
	CHECK(Resentment(r) == 0.0f);
	bool oneCrew = true;
	for (const CrewMember &c : r.crew) if (c.faction != 0) oneCrew = false;
	CHECK(oneCrew);

	// The airponics bay grows food without the replicators.
	Ship withA = NewShip(), noA = NewShip();
	withA.stores.rations = 20.0f; noA.stores.rations = 20.0f;
	SetEnabled(withA, SYS_REPLICATORS, false); SetEnabled(noA, SYS_REPLICATORS, false);
	CHECK(SetAirponics(withA, true) && Airponics(withA));
	Tick(withA, Hours(withA, 24.0f));
	Tick(noA, Hours(noA, 24.0f));
	CHECK(withA.stores.rations > noA.stores.rations);

	// It is all in the save.
	std::vector<uint8_t> blob = Pack(vio);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(std::fabs(back.resentment - vio.resentment) < 1e-4f && Recall(back.crew[0], MEM_VIOLATION));
}

// S7, the rest: more than one kind of boarder, and objectives.
static void TestBoarderKinds()
{
	g_test = "boarder kinds and objectives";

	auto noSecurity = [](Ship &x) { for (CrewMember &c : x.crew) if (c.dept == DEPT_SECURITY) c.status = CREW_DEAD; };

	// An objective sends a party the way it was told, not the nearest of the bridge and Engineering.
	Ship o = NewShip();
	noSecurity(o);
	BoardAs(o, 3, 3, BOARDER_RAIDER, 9);   // making for the computer core
	Tick(o, Hours(o, 2.0f / 60.0f));
	CHECK(o.decks[3].intruders > 0.0f);    // deck 4: one step toward 9
	CHECK(o.decks[1].intruders == 0.0f);   // not toward the bridge
	Ship a = NewShip();
	noSecurity(a);
	Board(a, 3, 3);                        // no objective: the auto target, the bridge from deck 3
	Tick(a, Hours(a, 2.0f / 60.0f));
	CHECK(a.decks[1].intruders > 0.0f);    // deck 2: toward the bridge

	// Raiders loot when they hold a deck with nothing to take.
	Ship l = NewShip();
	noSecurity(l);
	BoardAs(l, 3, 3, BOARDER_RAIDER, 3);   // hold deck 3
	const float parts = l.stores.spareParts;
	Tick(l, Hours(l, 1.0f));
	CHECK(l.stores.spareParts < parts);

	// Hunters come for the crew.
	Ship h = NewShip();
	noSecurity(h);
	BoardAs(h, 9, 4, BOARDER_HUNTER, 9);   // there are crew on deck 9
	const int hurtBefore = [&] { int n = 0; for (const CrewMember &c : h.crew) if (c.status == CREW_INJURED || c.wounds > 0.0f) ++n; return n; }();
	Tick(h, Hours(h, 0.5f));
	const int hurtAfter = [&] { int n = 0; for (const CrewMember &c : h.crew) if (c.status == CREW_INJURED || c.wounds > 0.0f) ++n; return n; }();
	CHECK(hurtAfter > hurtBefore);
	CHECK(std::string(BoarderKindName(BOARDER_HUNTER)) == "a hunter");

	// Kind and objective survive a save.
	std::vector<uint8_t> blob = Pack(o);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(back.decks[2].boarderKind == BOARDER_RAIDER && back.decks[2].objective == 9);
}

// Borg strategic awareness, and memory (a grudge) read by the simulation.
static void TestAwarenessAndLoyalty()
{
	g_test = "Borg awareness and a grudge read by the simulation";

	// A Borg contact raises the Collective's awareness of the ship.
	Ship s = NewShip();
	Tick(s, 1.0f);
	const int cube = FirstOfKind(s, BEACON_BORG);
	if (cube > 0) {
		GoTo(s, cube - 1);
		s.enemy = Enemy();
		CHECK(Jump(s, cube) && s.enemy.borg && BorgAwareness(s) > 0.0f);
	}

	// The more the Collective knows, the faster it adapts: the same fire does less to a cube it has
	// already met.
	auto cubeHull = [](float aware) {
		Ship x = NewShip();
		Tick(x, 1.0f);
		const int h = FirstOfKind(x, BEACON_HOSTILE);
		GoTo(x, h - 1);
		x.enemy = Enemy();
		Jump(x, h);
		x.enemy.kind = ENEMY_BORG_VESSEL; x.enemy.borg = true;
		x.enemy.shields = 0.0f; x.enemy.shieldGen = 0.0f; x.enemy.firepower = 0.0f;
		x.borgAwareness = aware;
		SetAlert(x, ALERT_RED);
		Tick(x, Hours(x, 10.0f));
		return x.enemy.hull;
	};
	CHECK(cubeHull(0.9f) > cubeHull(0.0f));

	// A grudge toward whoever commands is going through the motions: the post delivers less.
	Ship g = NewShip();
	int holder = -1;
	for (int i = 0; i < static_cast<int>(g.crew.size()); ++i)
		if (g.crew[i].post == SYS_SENSORS && g.crew[i].rank < 5 && g.crew[i].status == CREW_FIT) { holder = i; break; }
	CHECK(holder >= 0);
	Remember(g, holder, MEM_LIE, 0, MEM_SAW, -0.9f); // a grudge against Janeway, who commands
	CHECK(Loyalty(g, holder) < -0.3f);
	Tick(g, 1.0f);
	Ship c = NewShip();
	Tick(c, 1.0f);
	CHECK(g.systems[SYS_SENSORS].output < c.systems[SYS_SENSORS].output);

	// It is all in the save.
	std::vector<uint8_t> blob = Pack(s);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(std::fabs(back.borgAwareness - s.borgAwareness) < 1e-4f);
}

// S6's exit evidence: a multi-day soak with no stuck state. A deterministic sequence of damage,
// fire, breaches, boarders, Borg, fights and orders over a fortnight, with the invariants checked
// every day, and the whole thing saved and restored at the end.
static void TestSoak()
{
	g_test = "a multi-day soak leaves no stuck state";
	Ship s = NewShip();
	for (int day = 0; day < 14; ++day) {
		const int deck = 1 + (day * 5) % DECKS;
		BreachDeck(s, deck, 0.3f);
		IgniteDeck(s, deck, 0.2f);
		Board(s, deck, 2);
		if (day % 4 == 0) DamageSystem(s, static_cast<SystemId>(day % SYS_COUNT), 0.3f);
		if (day % 7 == 3) BoardBorg(s, 1 + (day * 3) % DECKS, 2);
		if (day % 5 == 2) { // a fight, then stand down
			SetAlert(s, ALERT_RED);
			s.enemy = Enemy();
			s.enemy.present = true; s.enemy.kind = ENEMY_RAIDER;
			s.enemy.hull = 1.0f; s.enemy.shields = 1.0f; s.enemy.weapons = 1.0f; s.enemy.firepower = 0.2f;
		}
		Tick(s, Hours(s, 24.0f));
		if (s.alert == ALERT_RED) SetAlert(s, ALERT_GREEN);
		CHECK(!std::isnan(s.clock) && !std::isnan(s.stores.deuterium) && !std::isnan(s.stores.batteries));
		for (int i = 0; i < SYS_COUNT; ++i) {
			CHECK(s.systems[i].health >= 0.0f && s.systems[i].health <= 1.0f);
			CHECK(s.systems[i].output >= 0.0f && s.systems[i].output <= 1.0f);
			CHECK(s.systems[i].control >= 0.0f && s.systems[i].control <= 1.0f);
			CHECK(!std::isnan(s.systems[i].output));
		}
		for (int d = 0; d < DECKS; ++d) {
			CHECK(s.decks[d].atmosphere >= 0.0f && s.decks[d].atmosphere <= 1.0f);
			CHECK(s.decks[d].hull >= 0.0f && s.decks[d].hull <= 1.0f);
			CHECK(s.decks[d].fire >= 0.0f && s.decks[d].fire <= 1.0f);
			CHECK(s.decks[d].assimilated >= 0.0f && s.decks[d].assimilated <= 1.0f);
		}
		for (const CrewMember &c : s.crew) {
			CHECK(c.morale >= 0.0f && c.morale <= 1.0f && !std::isnan(c.morale));
			CHECK(c.fatigue >= 0.0f && c.fatigue <= 1.0f);
			CHECK(c.severity >= 0.0f && c.severity <= 1.0f && c.wounds >= 0.0f && c.wounds <= 1.0f);
			CHECK(c.deck <= DECKS);
		}
		CHECK(s.stores.spareParts >= 0.0f && s.stores.deuterium >= 0.0f && s.stores.deuterium <= 1.0f);
	}
	// And it still saves and restores identically after the fortnight.
	std::vector<uint8_t> blob = Pack(s);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(Describe(back) == Describe(s));
}

// Deferred maintenance (docs/crew-work.md): a station left undermanned fails, and the failure is
// traceable to the work that was due.
static void TestMaintenance()
{
	g_test = "deferred maintenance";
	Ship w = NewShip();
	for (CrewMember &c : w.crew) if (c.post == SYS_SENSORS) c.status = CREW_DEAD; // the whole watch, every watch
	w.stores.spareParts = 0.0f;                                                    // and no parts to mend it
	Tick(w, Hours(w, 15.0f * 24.0f)); // fifteen days
	CHECK(w.systems[SYS_SENSORS].health < 1.0f);
	bool traced = false;
	for (const LogEntry &e : w.log) if (e.what.find("sensors failed for want of maintenance") != std::string::npos) traced = true;
	CHECK(traced);
	// A system its own watch keeps up never wears.
	Ship m = NewShip();
	Tick(m, Hours(m, 15.0f * 24.0f));
	CHECK(m.systems[SYS_SENSORS].health == 1.0f);
}

// The job queue (docs/crew-work.md): the outstanding work, in the order it is worked.
static void TestJobQueue()
{
	g_test = "the job queue";
	Ship s = NewShip();
	Tick(s, 1.0f);
	CHECK(Jobs(s).empty());
	DamageSystem(s, SYS_HOLODECKS, 0.5f);
	DamageSystem(s, SYS_LIFE_SUPPORT, 0.5f);
	BreachDeck(s, 9, 0.5f);
	Tick(s, 0.0f);
	const std::vector<Job> &jobs = Jobs(s);
	CHECK(jobs.size() == 3);
	// life support (priority 0) is worked before the holodecks (17) and the seal (20)
	CHECK(jobs[0].kind == JOB_REPAIR && jobs[0].target == SYS_LIFE_SUPPORT && jobs[0].priority == 0);
	CHECK(jobs[1].kind == JOB_REPAIR && jobs[1].target == SYS_HOLODECKS);
	CHECK(jobs[2].kind == JOB_SEAL && jobs[2].target == 9);
	// The work that is due is written down, so the log can trace it.
	bool ordered = false;
	for (const LogEntry &e : s.log) if (e.what.find("work ordered: repair life support") != std::string::npos) ordered = true;
	CHECK(ordered);

	// Command: see first to the holodecks, and that repair goes to the head.
	SetRole(s, ROLE_IN_COMMAND);
	CHECK(OrderRepairFirst(s, SYS_HOLODECKS));
	Tick(s, 0.0f);
	CHECK(Jobs(s).front().kind == JOB_REPAIR && Jobs(s).front().target == SYS_HOLODECKS);
	CHECK(Jobs(s).front().priority == -1);

	// Repair it and the job is gone; a Borg deck queues a reclaim.
	Repair(s, SYS_HOLODECKS, 1.0f);
	CHECK(OrderRepairFirst(s, -1));
	s.decks[7].assimilated = 0.8f;
	Tick(s, 0.0f);
	for (const Job &j : Jobs(s)) CHECK(!(j.kind == JOB_REPAIR && j.target == SYS_HOLODECKS));
	bool reclaim = false;
	for (const Job &j : Jobs(s)) if (j.kind == JOB_RECLAIM && j.target == 8) reclaim = true;
	CHECK(reclaim);

	// The queue survives a save and a load, unchanged.
	Tick(s, 0.0f);
	const std::vector<uint8_t> blob = Pack(s);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(Pack(back) == blob && Jobs(back).size() == Jobs(s).size());

	// A build job: command orders spare parts built; the crew fabricate them from material.
	Ship b = NewShip();
	b.stores.materials = 200.0f;
	SetRole(b, ROLE_IN_COMMAND);
	const float parts0 = b.stores.spareParts;
	CHECK(OrderBuild(b, 5));
	Tick(b, 1.0f);
	bool building = false;
	for (const Job &j : Jobs(b)) if (j.kind == JOB_BUILD && j.target == 5) building = true;
	CHECK(building);
	Tick(b, Hours(b, 12.0f));
	CHECK(b.stores.spareParts >= parts0 + 5.0f && b.stores.materials < 200.0f);
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

// S4. Each station does more than switch a system: the transporter beams, astrometrics surveys, the
// Conn lays in a course, sickbay reads its ward.
static void TestStationPurposes()
{
	g_test = "the stations' purposes";
	Ship s = NewShip();
	Tick(s, 1.0f);

	// The transporter: a party to the site, and back. The shields must be down, the transporters up.
	CHECK(AwayTeam(s) == 0);
	CHECK(TransportAway(s, 3));
	CHECK(AwayTeam(s) == 3);
	int awayOnDeck = 0;
	for (const CrewMember &c : s.crew)
		if (c.away) { CHECK(c.deck == 0); ++awayOnDeck; }
	CHECK(awayOnDeck == 3);
	Tick(s, Hours(s, 1.0f));
	for (const CrewMember &c : s.crew) CHECK(!c.away || (c.activity == ACT_PERSONAL && c.deck == 0));
	CHECK(TransportBack(s) && AwayTeam(s) == 0);

	// With our shields up the beam cannot reach the site.
	Ship red = NewShip();
	SetAlert(red, ALERT_RED);
	Tick(red, Hours(red, 0.2f));
	CHECK(red.shieldStrength > 0.0f);
	CHECK(!TransportAway(red, 2) && AwayTeam(red) == 0);
	// ... and with the transporters down, there is no beam at all.
	Ship down = NewShip();
	SetEnabled(down, SYS_TRANSPORTERS, false);
	Tick(down, 1.0f);
	CHECK(!TransportAway(down, 2));
	CHECK(!TransportBack(down)); // nobody is away to bring back

	// Astrometrics: the survey reads the beacons one jump away and marks them known.
	Ship a = NewShip();
	const int near = a.sector[a.beacon].links.front();
	CHECK(!a.sector[near].surveyed);
	SetEnabled(a, SYS_SENSORS, false);
	Tick(a, 1.0f);
	CHECK(Survey(a) == 0); // no sensors, no survey
	SetEnabled(a, SYS_SENSORS, true);
	Tick(a, 1.0f);
	CHECK(Survey(a) > 0 && a.sector[near].surveyed);

	// The Conn's course: the shortest route, and setting it.
	Ship c = NewShip();
	const int goal = SECTOR_BEACONS - 1;
	const std::vector<int> route = PlotCourse(c, goal);
	CHECK(!route.empty() && route.front() == c.beacon && route.back() == goal);
	for (size_t i = 1; i < route.size(); ++i) {
		const std::vector<int> &links = c.sector[route[i - 1]].links;
		CHECK(std::find(links.begin(), links.end(), route[i]) != links.end()); // every leg is a link
	}
	CHECK(SetCourse(c, goal) && Course(c) == goal);
	CHECK(!SetCourse(c, 99)); // nowhere to lay a course to

	// Sickbay's ward: the patients, in the order triage treats them.
	Ship w = NewShip();
	int made = 0;
	for (CrewMember &m : w.crew) {
		if (made >= 5) break;
		if (m.status != CREW_FIT) continue;
		m.status = CREW_INJURED;
		m.severity = 0.1f + 0.15f * made; // 0.1 .. 0.7
		++made;
	}
	const std::vector<int> ward = Patients(w);
	CHECK(static_cast<int>(ward.size()) == 5);
	float lastSeverity = 2.0f;
	for (int i : ward) { CHECK(w.crew[i].status == CREW_INJURED); CHECK(w.crew[i].severity <= lastSeverity); lastSeverity = w.crew[i].severity; }
	// Rank first reverses the order where rank decides.
	w.orderTriage = 1;
	for (int i : Patients(w)) CHECK(w.crew[i].status == CREW_INJURED);

	// The away mission, the surveys and the course are in the save.
	Ship p = NewShip();
	p.sector[p.beacon].surveyed = true;
	CHECK(SetCourse(p, goal));
	CHECK(TransportAway(p, 2));
	Tick(p, 0.0f); // derive manning and locations with the party away, as the console does after a command
	const std::vector<uint8_t> blob = Pack(p);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(back.awayBeacon == p.awayBeacon && AwayTeam(back) == 2);
	CHECK(back.course == p.course);
	CHECK(back.sector[p.beacon].surveyed);
	CHECK(back.awayBeacon == 0 && p.awayBeacon == 0);
	CHECK(Describe(back) == Describe(p));
	CHECK(Pack(back) == blob);
}

// The five gaps, to their criteria: replication restocks the ward, the surgical field holds the
// gravest case, endurance is a number per source, a tricorder reads the ship's own compartments, and
// the log signs events with people.
static void TestGapCompletions()
{
	g_test = "the gaps: replication, the surgical field, endurance, the tricorder, named authors";

	// The replicators restock medical supplies while they run.
	Ship r = NewShip();
	r.stores.medicalSupplies = 10.0f;
	Tick(r, Hours(r, 4.0f));
	CHECK(r.stores.medicalSupplies > 10.0f && r.stores.medicalSupplies <= 100.0f);
	Ship dark = NewShip();
	dark.stores.medicalSupplies = 10.0f;
	SetEnabled(dark, SYS_REPLICATORS, false);
	SetAlert(dark, ALERT_RED); // red alert stands the replicators down
	Tick(dark, Hours(dark, 4.0f));
	CHECK(dark.stores.medicalSupplies == 10.0f);

	// The surgical bay's force field holds the gravest case steady when there are no supplies; without
	// it, the same case dies.
	Ship hold = NewShip();
	SetEnabled(hold, SYS_REPLICATORS, false); // no restock: the field is the only thing holding them
	hold.stores.medicalSupplies = 0.0f;
	hold.crew[3].status = CREW_INJURED;
	hold.crew[3].severity = 0.9f;
	SetSurgicalField(hold, true);
	CHECK(SurgicalField(hold));
	Tick(hold, Hours(hold, 4.0f));
	CHECK(hold.crew[3].status == CREW_INJURED && hold.crew[3].severity <= 0.9f + 1e-4f);
	Ship let = NewShip();
	SetEnabled(let, SYS_REPLICATORS, false);
	let.stores.medicalSupplies = 0.0f;
	let.crew[3].status = CREW_INJURED;
	let.crew[3].severity = 0.9f;
	Tick(let, Hours(let, 4.0f));
	CHECK(let.crew[3].status == CREW_DEAD && !SurgicalField(let));

	// Endurance is its own number per source: the batteries are the ship's endurance with the reactors
	// off, and each reactor reports its own.
	Ship e = NewShip();
	for (int i = 0; i < SRC_BATTERIES; ++i) SetSourceOnline(e, static_cast<SourceId>(i), false);
	Tick(e, 1.0f);
	const float battery = EnduranceOf(e, SRC_BATTERIES);
	CHECK(battery > 0.0f && battery <= 3.0f * 60.0f + 1.0f); // the cells' three hours at most
	CHECK(EnduranceOf(e, SRC_WARP_CORE) < 0.0f); // not supplying now
	SetSourceOnline(e, SRC_WARP_CORE, true);
	Tick(e, 1.0f);
	CHECK(EnduranceOf(e, SRC_WARP_CORE) > battery);
	CHECK(EnduranceOf(e, static_cast<SourceId>(SRC_COUNT)) < 0.0f);

	// A tricorder reads the ship's own compartments, wears the kit, and a dead one reads nothing.
	Ship t = NewShip();
	LoadAwayKit(t, 2, 0, 0, 1.0f);
	CHECK(t.stores.kitCondition == 1.0f);
	BreachDeck(t, 9, 1.0f);
	const std::string reading = ScanCompartment(t, 9);
	CHECK(reading.find("deck 9") != std::string::npos && reading.find("air") != std::string::npos);
	CHECK(t.stores.kitCondition < 1.0f && !t.log.empty() && t.log.back().what.find("compartment scan") != std::string::npos);
	t.stores.tricorderCharge = 0.0f;
	CHECK(ScanCompartment(t, 9) == "the tricorder is dead");
	CHECK(ScanCompartment(t, 99) == "no such compartment");

	// The log signs events with people, not subsystems.
	Ship who = NewShip();
	DamageSystem(who, SYS_SENSORS, 0.4f);
	bool named = false;
	for (const LogEntry &le : who.log)
		if (le.what.find("damaged") != std::string::npos)
			named = !le.who.empty() && le.who != "damage control"; // a person signs it, not the subsystem
	CHECK(named);

	// And all of it survives a save and a load.
	std::vector<uint8_t> blob = Pack(t);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(std::fabs(back.stores.kitCondition - t.stores.kitCondition) < 1e-4f);
	CHECK(back.surgicalForceField == t.surgicalForceField);
}

// S6, full: hull repair is crewed work that costs parts, fire is a second way to be hurt, and the
// galley's rations feed the crew.
static void TestHullSealFireRations()
{
	g_test = "hull repair costs crew and parts; fire injures and spreads; rations feed the crew";

	// Sealing a hull breach is crewed work, paid for in parts; without parts it stays open.
	Ship s = NewShip();
	BreachDeck(s, 9, 1.0f);
	Tick(s, 1.0f);
	CHECK(s.decks[8].sealing > 0 && s.decks[8].sealing <= REPAIR_TEAM_MAX);
	const float parts = s.stores.spareParts;
	Tick(s, Hours(s, 1.0f));
	CHECK(s.decks[8].hull > 0.4f && s.decks[8].hull < 1.0f);
	CHECK(s.stores.spareParts < parts);
	Tick(s, Hours(s, 3.0f));
	CHECK(s.decks[8].hull == 1.0f);
	Ship poor = NewShip();
	poor.stores.spareParts = 0.0f;
	BreachDeck(poor, 9, 1.0f);
	Tick(poor, Hours(poor, 6.0f));
	CHECK(poor.decks[8].hull == 0.0f);

	// Fire injures the crew, is fought down while there are hands, and spreads when there are none.
	Ship f = NewShip();
	IgniteDeck(f, 9, 1.0f);
	CHECK(f.decks[8].fire == 1.0f);
	Tick(f, Hours(f, 0.05f)); // three minutes: the fire burns and the crew turn out
	CHECK(f.decks[8].fire < 1.0f);
	int burned = 0;
	for (const CrewMember &c : f.crew) if (c.burn > 0.0f || c.status == CREW_INJURED) ++burned;
	CHECK(burned > 0);                                    // somebody was caught by it
	Ship g = NewShip();
	for (CrewMember &c : g.crew) c.status = CREW_DEAD;    // nobody to fight it
	IgniteDeck(g, 9, 1.0f);
	Tick(g, Hours(g, 2.0f));
	CHECK(g.decks[8].fire > 0.5f);                        // still burning
	CHECK(g.decks[7].fire > 0.0f || g.decks[9].fire > 0.0f); // and it has spread

	// Rations are eaten, and a crew with none is in a worse mood than one that is fed.
	Ship r = NewShip();
	Tick(r, Hours(r, 12.0f));
	CHECK(r.stores.rations < 100.0f);
	Ship hungry = NewShip();
	hungry.stores.rations = 0.0f;
	SetEnabled(hungry, SYS_REPLICATORS, false);
	Ship fed = NewShip();
	Tick(hungry, Hours(hungry, 24.0f));
	Tick(fed, Hours(fed, 24.0f));
	float hungryMorale = 0.0f, fedMorale = 0.0f;
	int hn = 0, fn = 0;
	for (const CrewMember &c : hungry.crew) if (c.status == CREW_FIT) { hungryMorale += c.morale; ++hn; }
	for (const CrewMember &c : fed.crew) if (c.status == CREW_FIT) { fedMorale += c.morale; ++fn; }
	CHECK(hn > 0 && fn > 0 && hungryMorale / hn < fedMorale / fn);

	// Fire and rations are in the save.
	std::vector<uint8_t> blob = Pack(f);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(std::fabs(back.decks[8].fire - f.decks[8].fire) < 1e-4f);
	CHECK(std::fabs(back.stores.rations - f.stores.rations) < 1e-4f);
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

// `test_ship_core --losses` prints the written-off list as evidence: write a few things off, read
// the list back, and show that it survives a save and a reload (docs/evidence/abandonment-list.md).
static int PrintLosses()
{
	Ship s = NewShip();
	SetRole(s, ROLE_IN_COMMAND);
	WriteOff(s, false, 9, LOSS_SEALED);
	WriteOff(s, true, SYS_PHASERS, LOSS_STRIPPED);
	WriteOff(s, false, 5, LOSS_UNINHABITABLE);
	WriteOff(s, false, 12, LOSS_WRITTEN_OFF);
	for (const LossEntry &e : WriteOffs(s)) {
		const int day = static_cast<int>(e.time / SECONDS_PER_DAY);
		const int sod = static_cast<int>(e.time) % SECONDS_PER_DAY;
		std::printf("PASS  day %d %02d:%02d  [%-13s] %-12s  decided by %s\n",
			day, sod / 3600, sod % 3600 / 60, LossKindName(e.kind), e.what.c_str(), e.who.c_str());
	}
	std::vector<uint8_t> blob = Pack(s);
	Ship back;
	const bool restored = Unpack(blob.data(), blob.size(), back);
	std::printf("PASS  %s: %d entries survive save and reload, newest %s (%s)\n",
		restored ? "reloaded" : "RELOAD FAILED", static_cast<int>(WriteOffs(back).size()),
		WriteOffs(back).empty() ? "-" : WriteOffs(back).back().what.c_str(),
		WriteOffs(back).empty() ? "-" : WriteOffs(back).back().who.c_str());
	return restored && WriteOffs(back).size() == 4 ? 0 : 1;
}

// The measurement the ruling asks to be shown: the odds as a function of condition, over twenty
// thousand draws each, and the severity the same condition takes light and at battle stations.
static int PrintRisk()
{
	std::printf("condition  AnomalyOdds   measured/20000   severity light   severity battle\n");
	for (float c = 1.0f; c >= -0.001f; c -= 0.1f) {
		int hits = 0;
		for (uint32_t i = 0; i < 20000; ++i)
			if (RollAnomaly(c, 1.0f, i * 2654435761u + 12345u) != ANOMALY_NONE) ++hits;
		std::printf("  %5.2f      %8.4f        %6d          %-13s   %s\n",
			c, AnomalyOdds(c), hits, AnomalyName(AnomalySeverityFor(c, 0.15f)),
			AnomalyName(AnomalySeverityFor(c, 1.0f)));
	}
	return 0;
}

int main(int argc, char **argv)
{
	if (argc > 1 && !std::strcmp(argv[1], "--day")) return PrintDay();
	if (argc > 1 && !std::strcmp(argv[1], "--losses")) return PrintLosses();
	if (argc > 1 && !std::strcmp(argv[1], "--risk")) return PrintRisk();

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
	TestStationPurposes();
	TestGapCompletions();
	TestHullSealFireRations();
	TestCrewOnDeck();
	TestDamageControl();
	TestCasualties();
	TestRadiation();
	TestTriage();
	TestAirAndEndurance();
	TestLog();
	TestWrittenOffList();
	TestAwayKit();
	TestBoarding();
	TestBreachPuzzle();
	TestBorg();
	TestSector();
	TestCombat();
	TestMultipleContacts();
	TestEnemySystemsAndOutsideChoices();
	TestMaterialsAndTravel();
	TestCrewJusticeAndBorg();
	TestMemoryAndConsequence();
	TestResourcesAndPressure();
	TestPhenomenon();
	TestProbes();
	TestDilithium();
	TestLocateDilithium();
	TestShuttles();
	TestIncursion();
	TestSystemStates();
	TestHazardInjuries();
	TestCoreCascade();
	TestConditionOdds();
	TestTransporterAnomaly();
	TestSquad();
	TestForceFieldKit();
	TestDeassimilation();
	TestCounterPlay();
	TestHolodeckAndQuarters();
	TestEVAAndContact();
	TestBoarderKinds();
	TestAwarenessAndLoyalty();
	TestSoak();
	TestJobQueue();
	TestMaintenance();
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
