// Tests for module/ship/ship_core: gate S1's exit criteria, one function per property.
// `test_ship_core --day` instead prints a simulated day, hour by hour, as evidence.

#include "ship_core.h"

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
	Tick(s, Hours(s, 5.5f));

	const std::vector<uint8_t> blob = Pack(s);
	CHECK(blob.size() < 2048); // the whole ship and its 141 crew
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
	bad = blob; bad[4] = 9; // a newer version
	CHECK(!Unpack(bad.data(), bad.size(), untouched));
	bad = blob; bad[6] = 7; // a different complement
	CHECK(!Unpack(bad.data(), bad.size(), untouched));
	bad = blob; bad[24] = 5; // an alert condition that does not exist
	CHECK(!Unpack(bad.data(), bad.size(), untouched));
	bad = blob; bad[28] = 0x7f; // a health that is not a fraction
	CHECK(!Unpack(bad.data(), bad.size(), untouched));
	CHECK(Describe(untouched) == before);
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
	TestDeterminismAndStepSize();
	TestSave();
	TestStations();
	TestCrewOnDeck();

	if (g_failures) {
		std::printf("%d check(s) failed\n", g_failures);
		return 1;
	}
	std::printf("ship_core: all checks passed\n");
	return 0;
}
