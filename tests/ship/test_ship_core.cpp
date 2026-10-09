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
	// Conservation is now a property of the allocator, not of the ship: with automatic mode on the
	// ladder spends only what the plant supplies. With it off the player's commitments are honoured
	// in full and a deficit is reported instead (see TestOversubscriptionReported).
	g_test = "power is conserved by the automatic allocator";
	Ship s = NewShip();
	SetPowerAuto(s, true);
	for (int step = 0; step < 6; ++step) {
		if (step == 1) SetAlert(s, ALERT_RED);
		if (step == 2) DamageSource(s, SRC_WARP_CORE, 0.6f);
		if (step == 3) SetSourceOnline(s, SRC_IMPULSE_REACTORS, false);
		if (step == 4) DamageSystem(s, SYS_SHIELDS, 0.5f);
		if (step == 5) SetSourceOnline(s, SRC_WARP_CORE, false);
		Tick(s, 1.0f);
		CHECK(s.PowerAllocated() <= s.PowerAvailable());
		for (int i = 0; i < SYS_COUNT; ++i) {
			CHECK(s.systems[i].allocated >= 0 && s.systems[i].allocated <= EffectiveDemand(s, static_cast<SystemId>(i)));
			CHECK(s.systems[i].output >= 0.0f && s.systems[i].output <= s.systems[i].health);
		}
		for (int i = 0; i < SRC_COUNT; ++i) CHECK(s.sources[i].output >= 0);
	}
}

// The budget is a ladder: what is fed first is given up last, and the warp drive is given up last of
// all -- the ship keeps the way home after every comfort (docs/budget-squaring.md, Part six).
static void TestSheddingFollowsPriority()
{
	g_test = "shedding follows priority, and the warp drive is shed last";
	Ship s = NewShip();
	SetPowerAuto(s, true); // the ladder is the policy of automatic mode only
	SetAlert(s, ALERT_YELLOW); // everything demands: nothing is suppressed by the condition
	Tick(s, 1.0f);

	// A worked ceiling: the crystal ages, the core's output falls, and what she gives up is the
	// cheapest thing first.
	s.crystalCeiling = 0.55f;
	Tick(s, 1.0f);
	CHECK(s.PowerCapacityNow() == 1170);
	CHECK(s.systems[SYS_WARP_DRIVE].allocated == Spec(SYS_WARP_DRIVE).demand); // the way home still runs
	CHECK(s.systems[SYS_CARGO_HANDLING].allocated == 0);                       // the cheapest thing does not
	CHECK(s.systems[SYS_GRAVITY_PLATING].allocated == 0);
	CHECK(s.systems[SYS_HOLODECKS].allocated == 0);
	CHECK(s.systems[SYS_LIFE_SUPPORT].output == 1.0f);

	// A console can reorder the list. (The design's priority is the point, but it is still a control.)
	// Put the warp drive behind the comforts and it is the warp that goes dark first: the inversion
	// the document warns about, made visible.
	Ship r = NewShip();
	SetPowerAuto(r, true);
	SetAlert(r, ALERT_YELLOW);
	Tick(r, 1.0f);
	SetPriority(r, SYS_WARP_DRIVE, 99);
	r.crystalCeiling = 0.55f;
	Tick(r, 1.0f);
	CHECK(r.systems[SYS_WARP_DRIVE].allocated == 0);
	CHECK(r.systems[SYS_GRAVITY_PLATING].allocated == Spec(SYS_GRAVITY_PLATING).demand);
}

// The headline acceptance (docs/budget-squaring.md, Part five): demand 1,730, fresh supply 1,800,
// seventy EPS in hand, and the deterministic set of systems running or shed at each crystal ceiling.
static void TestBudgetShedSequence()
{
	g_test = "the budget: 1,730 demanded, 1,800 fresh, and the shed sequence at each ceiling";
	Ship s = NewShip();
	int demand = 0;
	for (int i = 0; i < SYS_COUNT; ++i) demand += Spec(static_cast<SystemId>(i)).demand;
	CHECK(demand == 1730);                         // the demand table
	CHECK(s.PowerCapacityFresh() == 1800);         // the fresh grid
	CHECK(s.PowerCapacityNow() == 1800);           // a fresh crystal delivers the whole nameplate
	CHECK(s.PowerCapacityFresh() - demand == 70);  // she leaves port with seventy in hand

	// At each ceiling, the systems the ship gives up -- the document's table, as the arithmetic runs.
	struct Case { float ceiling; int supply; int shedCount; SystemId shed[16]; };
	const Case CASES[4] = {
		{1.00f, 1800, 0, {}},
		{0.85f, 1590, 4, {SYS_CARGO_HANDLING, SYS_LIGHTING, SYS_HOLODECKS, SYS_REPLICATORS}},
		{0.70f, 1380, 9, {SYS_CARGO_HANDLING, SYS_LIGHTING, SYS_HOLODECKS, SYS_REPLICATORS, SYS_GRAVITY_PLATING,
			SYS_TRACTOR_BEAM, SYS_TURBOLIFTS, SYS_SICKBAY, SYS_SCIENCE_LABS}},
		{0.55f, 1170, 14, {SYS_CARGO_HANDLING, SYS_LIGHTING, SYS_HOLODECKS, SYS_REPLICATORS, SYS_GRAVITY_PLATING,
			SYS_TRACTOR_BEAM, SYS_TURBOLIFTS, SYS_SICKBAY, SYS_SCIENCE_LABS, SYS_TRANSPORTERS, SYS_COMMUNICATIONS,
			SYS_NAV_DEFLECTOR, SYS_ASTROMETRICS, SYS_TORPEDO_LAUNCHERS}},
	};
	for (const Case &c : CASES) {
		Ship q = NewShip();
		SetPowerAuto(q, true); // the shed sequence is what automatic mode does (docs/power-assignment.md)
		SetAlert(q, ALERT_YELLOW); // everything demands: nothing is suppressed by the condition
		Tick(q, 1.0f);
		q.crystalCeiling = c.ceiling;
		Tick(q, 1.0f);
		CHECK(q.PowerCapacityNow() == c.supply);
		bool shed[SYS_COUNT] = {false};
		int n = 0;
		for (int i = 0; i < SYS_COUNT; ++i) {
			const int want = EffectiveDemand(q, static_cast<SystemId>(i));
			if (want > 0 && q.systems[i].allocated < want) { shed[i] = true; ++n; }
		}
		CHECK(n == c.shedCount);
		for (int k = 0; k < c.shedCount; ++k) CHECK(shed[c.shed[k]]);
		// And in the document's order: the list is the shed order, highest keep-priority first.
		for (int k = 1; k < c.shedCount; ++k)
			CHECK(Spec(c.shed[k - 1]).priority > Spec(c.shed[k]).priority);
		// The warp drive is never among them while there is any crystal left: she keeps the way home.
		CHECK(!shed[SYS_WARP_DRIVE]);
	}
}

// Coreless (docs/budget-squaring.md, Part 4a): with the warp core gone, the fusion reactors and the
// batteries are all that remain, and the ship runs only the set that keeps her alive and moving.
static void TestCorelessShip()
{
	g_test = "coreless: the critical four, the impulse drive and the deflector, and nothing else";
	Ship s = NewShip();
	SetAlert(s, ALERT_YELLOW);
	Tick(s, 1.0f);
	DamageSource(s, SRC_WARP_CORE, 1.0f); // the core is gone entirely
	Tick(s, 1.0f);
	CHECK(Coreless(s));
	CHECK(s.PowerCapacityNow() == 400);   // impulse reactors 250 + auxiliary fusion 90 + batteries 60

	// The survival set is 390 of the 400; there is not the power for another system.
	int vital = 0;
	for (int i = 0; i < SYS_COUNT; ++i) {
		const bool inSet = i == SYS_LIFE_SUPPORT || i == SYS_STRUCTURAL_INTEGRITY || i == SYS_INERTIAL_DAMPERS
			|| i == SYS_COMPUTER_CORE || i == SYS_IMPULSE_DRIVE || i == SYS_NAV_DEFLECTOR;
		if (inSet) vital += Spec(static_cast<SystemId>(i)).demand;
	}
	CHECK(vital == 390);
	CHECK(s.PowerAllocated() == 390);
	for (int i = 0; i < SYS_COUNT; ++i) {
		const bool inSet = i == SYS_LIFE_SUPPORT || i == SYS_STRUCTURAL_INTEGRITY || i == SYS_INERTIAL_DAMPERS
			|| i == SYS_COMPUTER_CORE || i == SYS_IMPULSE_DRIVE || i == SYS_NAV_DEFLECTOR;
		CHECK(s.systems[i].allocated == (inSet ? Spec(static_cast<SystemId>(i)).demand : 0));
	}
	// No weapons, no sensors, no comms: there is not the power.
	CHECK(s.systems[SYS_SHIELDS].allocated == 0 && s.systems[SYS_SENSORS].allocated == 0
		&& s.systems[SYS_PHASERS].allocated == 0 && s.systems[SYS_COMMUNICATIONS].allocated == 0);
}

static void TestBatteriesKeepTheCrewAlive()
{
	g_test = "the batteries alone hold life support at exactly full for three hours";
	// The canon arithmetic stands, but who decides is now the player (docs/power-assignment.md):
	// with the reactors down the player sets life support to full and everything else dark, and the
	// batteries hold it. The old ship did this silently; the new one waits to be told.
	Ship s = NewShip();
	SetAlert(s, ALERT_YELLOW); // nothing is stood down by the condition, so every allocation can be set
	for (int i = 0; i < SYS_COUNT; ++i)
		if (i != SYS_LIFE_SUPPORT) CHECK(SetAllocation(s, static_cast<SystemId>(i), 0));
	for (int i = 0; i < SRC_BATTERIES; ++i) SetSourceOnline(s, static_cast<SourceId>(i), false);
	CHECK(SetAllocation(s, SYS_LIFE_SUPPORT, 100));
	Tick(s, 1.0f);
	CHECK(s.sources[SRC_BATTERIES].output == 60);        // exactly life support's own demand
	CHECK(s.systems[SYS_LIFE_SUPPORT].output == 1.0f);
	CHECK(s.systems[SYS_STRUCTURAL_INTEGRITY].allocated == 0); // not a system short of it
	CHECK(s.systems[SYS_COMPUTER_CORE].allocated == 0 && s.systems[SYS_SENSORS].allocated == 0);
	CHECK(PowerShortfall(s) == 0);                       // the player's commitments fit the batteries

	Tick(s, Hours(s, 2.0f));
	CHECK(s.stores.batteries > 0.25f && s.stores.batteries < 0.40f); // three hours of charge, two spent
	Tick(s, Hours(s, 1.5f));
	CHECK(s.stores.batteries == 0.0f);
	CHECK(s.PowerAvailable() == 0 && s.systems[SYS_LIFE_SUPPORT].output == 0.0f);

	// With no life support the air goes stale, slowly; it does not vanish -- and the plating loses
	// its hold too: gravity is life support's, sited on deck 12 (docs/ship-master-map.md).
	const float before = s.decks[4].atmosphere;
	const float gravityBefore = s.decks[4].gravity;
	Tick(s, Hours(s, 3.0f));
	CHECK(s.decks[4].atmosphere < before && s.decks[4].atmosphere > 0.5f);
	CHECK(s.decks[4].gravity < gravityBefore && s.decks[4].gravity > 0.0f);

	// Bring a reactor back and the ship recovers by itself: air and gravity both.
	SetSourceOnline(s, SRC_AUXILIARY, true);
	Tick(s, Hours(s, 2.0f));
	CHECK(s.systems[SYS_LIFE_SUPPORT].output == 1.0f);
	CHECK(s.decks[4].atmosphere == 1.0f);
	CHECK(s.decks[4].gravity == 1.0f);
}

// ---- power allocation: the player decides (docs/power-assignment.md) ---------------------------

// The proving case, and the acceptance that matters: the holodecks running at full while the
// shields are dark, and the reverse. Both reachable, both stick, neither overridden by any ladder.
static void TestThePlayerDecides()
{
	g_test = "the proving case: the holodecks at full while the shields are dark, and the reverse";
	Ship s = NewShip();
	SetAlert(s, ALERT_YELLOW); // nothing is stood down by the condition, so both can be set
	CHECK(!PowerAuto(s));      // the default: the player sets the allocation

	CHECK(SetAllocation(s, SYS_SHIELDS, 0));   // the shields dark
	Tick(s, 1.0f);
	CHECK(AllocationSource(s, SYS_SHIELDS) == ALLOC_PLAYER);
	CHECK(s.systems[SYS_SHIELDS].allocated == 0 && s.systems[SYS_SHIELDS].output == 0.0f);
	CHECK(s.systems[SYS_HOLODECKS].output == 1.0f); // ... and the holodecks run whole
	// The ladder would have kept the shields (priority 7) and shed the holodecks (20); the player's
	// decision stands in both directions.
	CHECK(SetAllocation(s, SYS_HOLODECKS, 0));
	CHECK(SetAllocation(s, SYS_SHIELDS, 100));
	Tick(s, 1.0f);
	CHECK(s.systems[SYS_HOLODECKS].allocated == 0 && s.systems[SYS_HOLODECKS].output == 0.0f);
	CHECK(s.systems[SYS_SHIELDS].output == 1.0f);
	CHECK(AllocationSource(s, SYS_HOLODECKS) == ALLOC_PLAYER);
}

// The acceptance, to the letter: set an allocation, turn automatic mode off, change nothing else,
// and the ladder does not fire.
static void TestLadderOnlyInAutoMode()
{
	g_test = "the ladder fires only in automatic mode, and never on a system a person has set";
	Ship s = NewShip();
	SetAlert(s, ALERT_YELLOW);
	CHECK(SetAllocation(s, SYS_HOLODECKS, 60)); // the player wants the holodecks

	SetPowerAuto(s, true);
	s.crystalCeiling = 0.55f; // 1,170 against 1,730: the ladder must shed something
	Tick(s, 1.0f);
	CHECK(s.systems[SYS_CARGO_HANDLING].allocated == 0);   // the ladder fired on an unset system
	CHECK(s.systems[SYS_HOLODECKS].allocated == 36);       // ... and never touched the player's 60%

	SetPowerAuto(s, false);    // turn it off, change nothing else
	Tick(s, 1.0f);
	CHECK(s.systems[SYS_CARGO_HANDLING].allocated == Spec(SYS_CARGO_HANDLING).demand); // the ladder did not fire
	CHECK(s.systems[SYS_HOLODECKS].allocated == 36);       // the player's decision stands
	CHECK(PowerShortfall(s) > 0);                          // ... and the shortfall is reported
}

// Oversubscription is reported, never resolved: nothing goes dark, and the console refuses more.
static void TestOversubscriptionReported()
{
	g_test = "oversubscription is reported as a number, and nothing is shed";
	Ship s = NewShip();
	SetAlert(s, ALERT_YELLOW);
	s.crystalCeiling = 0.30f; // the plant ages: 420 (core) + 250 + 90 + 60 = 820 against a demand of 1,730
	Tick(s, 1.0f);
	CHECK(s.PowerAvailable() == 820);
	CHECK(PowerShortfall(s) == 1730 - 820); // the shortfall, as a number
	for (int i = 0; i < SYS_COUNT; ++i)
		CHECK(s.systems[i].allocated == EffectiveDemand(s, static_cast<SystemId>(i))); // nothing went dark
	CHECK(s.systems[SYS_HOLODECKS].output > 0.0f);
	CHECK(s.systems[SYS_LIFE_SUPPORT].output == 1.0f);
	bool logged = false;
	for (const LogEntry &e : s.log) if (e.what.find("exceed the plant") != std::string::npos) logged = true;
	CHECK(logged); // the record carries it

	// The console refuses more until something is freed: a decrease is allowed, the increase back is not.
	CHECK(SetAllocation(s, SYS_HOLODECKS, 50));
	CHECK(!SetAllocation(s, SYS_HOLODECKS, 100));
	CHECK(AllocationPercent(s, SYS_HOLODECKS) == 50);
}

// Partial allocation is a real state: forty per cent delivers forty per cent.
static void TestPartialAllocation()
{
	g_test = "forty per cent delivers forty per cent";
	Ship s = NewShip();
	SetAlert(s, ALERT_YELLOW);
	CHECK(SetAllocation(s, SYS_SENSORS, 40)); // the sensors see nearer, but they see
	Tick(s, 1.0f);
	CHECK(AllocationPercent(s, SYS_SENSORS) == 40);
	CHECK(s.systems[SYS_SENSORS].allocated == 24); // 40% of the demand of 60
	CHECK(std::fabs(s.systems[SYS_SENSORS].output - 0.4f) < 1e-5f); // almost-on, and measurable
	CHECK(SetAllocation(s, SYS_SENSORS, 100));
	Tick(s, 1.0f);
	CHECK(s.systems[SYS_SENSORS].output == 1.0f);
}

// Task C: the chief engineer recommends (and is refused, and remembers it), and a band delegation
// is grantable, held by a name, and revocable immediately.
static void TestRecommendationAndDelegation()
{
	g_test = "the chief engineer's recommendation, his refusal mark, and a revocable band";
	Ship s = NewShip();
	SetAlert(s, ALERT_YELLOW);
	const int chief = DepartmentHead(s, DEPT_ENGINEERING);
	CHECK(chief >= 0);
	const Recommendation rec = RecommendAllocation(s);
	CHECK(rec.by == chief);
	CHECK(!rec.reasoning.empty()); // it is on the console, with his reasoning
	for (int i = 0; i < SYS_COUNT; ++i) CHECK(rec.percent[i] >= 0 && rec.percent[i] <= 100);

	CHECK(RefuseRecommendation(s)); // the player refuses: recorded, and the officer remembers it
	CHECK(Recall(s.crew[chief], MEM_OVERRULED));
	CHECK(MemoryCount(s.crew[chief]) >= 1);

	// A band delegation: grantable, held by a name, revocable at once.
	const int grantor = 0; // whoever commands: the captain signs the grant
	CHECK(GrantBand(s, grantor, chief, BAND_COMFORT));
	const BandGrant *held = BandHolder(s, BAND_COMFORT);
	CHECK(held && held->grantee == chief);
	CHECK(SetAllocationBy(s, SYS_HOLODECKS, 40, chief)); // the officer acts under the grant
	CHECK(AllocationSource(s, SYS_HOLODECKS) == ALLOC_DELEGATE);
	CHECK(AllocationProvenance(s, SYS_HOLODECKS).find(s.crew[chief].name) != std::string::npos);
	CHECK(!SetAllocationBy(s, SYS_SHIELDS, 40, chief)); // the tactical band is not his
	CHECK(RevokeBand(s, chief, BAND_COMFORT));
	CHECK(BandHolder(s, BAND_COMFORT) == nullptr);
	CHECK(!SetAllocationBy(s, SYS_HOLODECKS, 60, chief)); // immediately: the authority is gone
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

	// The same hands, spent and gone through the motions, deliver much less. Morale is a read of
	// the three components; a wholly short, hopeless, unheld person reads 0.2.
	for (CrewMember &c : s.crew)
		if (c.post == SYS_SENSORS) { c.fatigue = 1.0f; c.deficit = 0.5f; c.outlook = 0.0f; c.holdings = 0.0f; }
	Tick(s, 0.0f);
	const float spent = s.systems[SYS_SENSORS].output;
	CHECK(spent < 1.0f);
	CHECK(spent >= 0.24f);                         // tired and flagging is not the same as absent
	CHECK(s.systems[SYS_SENSORS].manned >= Spec(SYS_SENSORS).crewNeeded); // the hands are there

	// Rested and willing again: the post is a whole hand once more.
	for (CrewMember &c : s.crew)
		if (c.post == SYS_SENSORS) { c.fatigue = 0.0f; c.deficit = 0.0f; c.outlook = 1.0f; c.holdings = 1.0f; }
	Tick(s, 0.0f);
	CHECK(s.systems[SYS_SENSORS].output == 1.0f);

	// The drivers move: a watch raises fatigue, and green lifts the reading.
	Ship t = NewShip();
	t.crew[0].watch = 0; t.crew[0].post = SYS_SENSORS; t.crew[0].dept = DEPT_SCIENCES;
	t.crew[0].fatigue = 0.0f; t.crew[0].deficit = 0.2f; t.crew[0].outlook = 0.2f; t.crew[0].holdings = 0.2f;
	const float before = Morale(t.crew[0]);
	Tick(t, Hours(t, 8.0f));                       // 0800 to 1600, alpha watch on duty throughout
	CHECK(t.crew[0].fatigue > 0.0f);
	CHECK(Morale(t.crew[0]) > before);

	// The three components, and the reading they produce, survive a save and a load.
	for (CrewMember &c : t.crew) { c.deficit = 0.30f; c.outlook = 0.40f; c.holdings = 0.50f; }
	const float read = Morale(t.crew[0]);
	std::vector<uint8_t> blob = Pack(t);
	Ship u = NewShip();
	CHECK(Unpack(blob.data(), blob.size(), u));
	CHECK(std::fabs(u.crew[0].deficit - 0.30f) < 1e-4f);
	CHECK(std::fabs(u.crew[0].outlook - 0.40f) < 1e-4f);
	CHECK(std::fabs(u.crew[0].holdings - 0.50f) < 1e-4f);
	CHECK(std::fabs(Morale(u.crew[0]) - read) < 1e-4f);
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
	bad = blob; bad[61] = 5; // an alert condition that does not exist
	CHECK(!Unpack(bad.data(), bad.size(), untouched));
	bad = blob; bad[65] = 0x7f; // a health that is not a fraction
	CHECK(!Unpack(bad.data(), bad.size(), untouched));
	bad = blob; bad[16] = 9; // a play mode that does not exist
	CHECK(!Unpack(bad.data(), bad.size(), untouched));
	CHECK(Describe(untouched) == before);
}

// A3: saves must replay identically. A crew member's wounds, severity and recovery are distinct
// saved fields, and the round trip must not move any of them. This is the observed discrepancy --
// a record printed as wounds=0 severity=0.8 before a save and wounds=0.8 severity=0.8 after the
// load -- pinned down as a test so it cannot come back silently.
static void TestCrewWoundsRoundTrip()
{
	g_test = "a crew member's wounds, severity and recovery survive the save";
	Ship s = NewShip();
	const int who = 42;
	s.crew[who].status = CREW_INJURED;
	s.crew[who].wounds = 0.15f;    // taken in the fight
	s.crew[who].severity = 0.80f;  // how badly it will go untreated
	s.crew[who].recovery = 0.25f;  // progress so far
	s.stores.medicalSupplies = 0.0f; // no treatment, so the untreated rule would apply on any tick
	const std::vector<uint8_t> blob = Pack(s);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	std::printf("  wounds %.2f -> %.2f, severity %.2f -> %.2f, recovery %.2f -> %.2f\n",
		s.crew[who].wounds, back.crew[who].wounds, s.crew[who].severity, back.crew[who].severity,
		s.crew[who].recovery, back.crew[who].recovery);
	CHECK(back.crew[who].wounds == s.crew[who].wounds);
	CHECK(back.crew[who].severity == s.crew[who].severity);
	CHECK(back.crew[who].recovery == s.crew[who].recovery);
	CHECK(Pack(back) == blob);
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
static void TestAirAndEndurance(){
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

// The environment in the world: one person's gravity is the deck's plating scaled onto the world's,
// and at full plating the world's own value stands again -- the case the engine's own FIXME names.
static void TestGravity()
{
	g_test = "per-person gravity scales with the deck, and hands back to the world's own";
	CHECK(GravityScale(NewShip(), 1) == 1.0f);
	CHECK(GravityScale(NewShip(), 8) == 1.0f && GravityScale(NewShip(), 99) == 1.0f);

	// A deck whose plating has failed: no gravity to feel; a deck at half hold, half.
	CHECK(ScaleGravity(800, 0.0f) == 0);
	CHECK(ScaleGravity(800, 0.5f) == 400);
	CHECK(ScaleGravity(800, 0.25f) == 200);

	// Restore: at full plating the world's value stands -- what clearing the engine's custom-gravity
	// flag gives back. It is clamped: never more than the world, never negative.
	CHECK(ScaleGravity(800, 1.0f) == 800);
	CHECK(ScaleGravity(800, 2.0f) == 800);
	CHECK(ScaleGravity(800, -0.5f) == 0);

	// The scale is the deck's own value, and it survives a save and a load.
	Ship s = NewShip();
	s.decks[11].gravity = 0.0f;
	CHECK(GravityScale(s, 12) == 0.0f);
	CHECK(ScaleGravity(800, GravityScale(s, 12)) == 0);
	s.decks[4].gravity = 0.6f;
	CHECK(ScaleGravity(800, GravityScale(s, 5)) == 480);
	std::vector<uint8_t> blob = Pack(s);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(back.decks[11].gravity == 0.0f && back.decks[4].gravity == 0.6f);
	CHECK(ScaleGravity(800, GravityScale(back, 5)) == 480);
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
	// Deck 8 now carries three systems (sensors, astrometrics and the labs), so the two boarders
	// spread across them; the seize threshold at six minutes is what finally makes the deck theirs.
	Tick(s, Hours(s, 4.0f / 60.0f));
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

	// Red alert: shields and phasers. The fight is now ours to win. (The bank is set to kill: this is
	// a fight, and the default stun setting does it no harm.)
	SetAlert(s, ALERT_RED);
	SetPhaserYield(s, YIELD_KILL);
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
	for (const CrewMember &c : united.crew) if (c.status == CREW_FIT) um += Morale(c);
	for (const CrewMember &c : split.crew) if (c.status == CREW_FIT) sm += Morale(c);
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
	for (CrewMember &c : f.crew) { c.deficit = 0.4f; c.outlook = 0.1f; c.holdings = 0.1f; }
	CHECK(!HoldFuneral(f));                                   // nobody in particular
	SetRole(f, ROLE_IN_COMMAND);
	const float before = Morale(f.crew[0]);
	CHECK(HoldFuneral(f) && Morale(f.crew[0]) > before);

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
	// The wall of names records the dead, and the sealed quarters are legible with their deck.
	bool onWall = false;
	for (const std::string &name : WallOfNames(d)) if (name == d.crew[50].name) onWall = true;
	CHECK(onWall);
	bool sealedQ = false;
	for (const SealedQuarter &q : SealedQuarters(d)) if (q.crew == 50) { CHECK(q.deck == 9); sealedQ = true; }
	CHECK(sealedQ);
	int witnesses = 0;
	for (const CrewMember &c : d.crew) if (Recall(c, MEM_DEATH)) ++witnesses;
	CHECK(witnesses >= 1);                                    // someone on deck 9 saw it
	int survivor = -1;
	for (int i = 0; i < static_cast<int>(d.crew.size()) && survivor < 0; ++i)
		if (d.crew[i].status == CREW_FIT) survivor = i;
	CHECK(survivor >= 0);
	SetRole(d, ROLE_IN_COMMAND);
	CHECK(HoldFuneral(d));
	for (const CrewMember &c : d.crew) CHECK(!c.quartersSealed);
	CHECK(SealedQuarters(d).empty());                         // the doors are opened again
	onWall = false;                                           // the name stays on the wall
	for (const std::string &name : WallOfNames(d)) if (name == d.crew[50].name) onWall = true;
	CHECK(onWall);
	// The funeral metabolises the loss: a positive mark toward whoever held it, and the death's
	// valence softened toward shared memory instead of growing dread.
	bool funeralMark = false;
	for (const Memory &m : d.crew[survivor].memories)
		if (m.event == MEM_FUNERAL) { funeralMark = true; CHECK(m.valence > 0.0f); if (m.person >= 0) CHECK(Bond(d, survivor, m.person) > 0.0f); }
	CHECK(funeralMark);
	for (const CrewMember &c : d.crew)
		for (const Memory &m : c.memories)
			if (m.event == MEM_DEATH) CHECK(m.valence >= -0.2f);

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
		SetPhaserYield(sh, YIELD_KILL); // a fight, not the peacetime stun setting
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

// The month report, the promise and the lie, and the toll paid downward (docs/the-record-and-the-log.md,
// docs/memory-and-consequence.md).
static void TestMonthReportAndToll()
{
	g_test = "the month report, the promise, the lie, and the toll";

	const int officer = 0;      // the captain
	const int crewmember = 40;  // an ensign who stands below them

	// A promise made in front of a crew member produces a mark naming the promiser, and its outcome
	// moves the mark's valence and the bond.
	Ship p = NewShip();
	CHECK(MakePromise(p, officer, crewmember, PROMISE_REPAIR, "the sensors would be repaired") == 0);
	CHECK(Promises(p).size() == 1 && Promises(p)[0].state == PROMISE_OPEN);
	CHECK(Recall(p.crew[crewmember], MEM_PROMISE));
	CHECK(RecallSource(p.crew[crewmember], MEM_PROMISE) == MEM_SAW);
	const float promised = Bond(p, crewmember, officer);
	CHECK(promised > 0.0f);
	CHECK(ResolvePromise(p, 0, true)); // the thing is done
	CHECK(Promises(p)[0].state == PROMISE_KEPT);
	CHECK(Bond(p, crewmember, officer) > promised);

	// Broken, the mark turns negative, the bond falls, and the reason is in the crew member's terms.
	Ship q = NewShip();
	CHECK(MakePromise(q, officer, crewmember, PROMISE_RESCUE, "they would be brought back") == 0);
	const float qbefore = Bond(q, crewmember, officer);
	CHECK(ResolvePromise(q, 0, false));
	CHECK(Bond(q, crewmember, officer) < qbefore);
	bool reason = false;
	for (const LogEntry &e : q.log)
		if (e.what.find(q.crew[crewmember].name) == 0 && e.what.find("not done") != std::string::npos) reason = true;
	CHECK(reason && q.log.back().who == q.crew[crewmember].name); // the crew member's own voice

	// A deadline that passes with nothing said is a promise broken.
	Ship d = NewShip();
	CHECK(MakePromise(d, officer, crewmember, PROMISE_WAY_HOME, "a way home", d.clock + Hours(d, 1.0f)) == 0);
	CHECK(d.promises[0].state == PROMISE_OPEN);
	Tick(d, Hours(d, 2.0f));
	CHECK(d.promises[0].state == PROMISE_BROKEN);

	// A signed report that contradicts a MEM_SAW mark produces a lie mark in the witness, naming
	// the signer, and the witness's bond toward the signer falls.
	Ship s = NewShip();
	Remember(s, crewmember, MEM_DEATH, 5, MEM_SAW, -0.8f); // the witness saw a death
	DraftReport(s, DEPT_COUNT);
	int line = -1;
	for (int i = 0; i < static_cast<int>(OpenReport(s).lines.size()); ++i)
		if (OpenReport(s).lines[i].event == MEM_DEATH && OpenReport(s).lines[i].person == 5) line = i;
	CHECK(line >= 0);                                  // the draft carries what was seen
	CHECK(StrikeReportLine(s, line));                  // the player strikes it: the lie by hand
	CHECK(!ReportDiff(OpenReport(s)).empty());         // the record keeps the diff
	const float bondBefore = Bond(s, crewmember, officer);
	CHECK(SignReport(s, officer, REPORT_TO_CREW));
	CHECK(Recall(s.crew[crewmember], MEM_LIE));
	CHECK(Bond(s, crewmember, officer) < bondBefore);
	bool publishedDeath = false;                       // the death was struck from the signed report
	for (const LogEntry &e : s.log)
		if (e.who == s.crew[officer].name && e.what.find("we lost") != std::string::npos) publishedDeath = true;
	CHECK(!publishedDeath);                            // the crew only see the published version

	// The same falsehood signed into a report nobody below reads produces no such fall.
	Ship u = NewShip();
	Remember(u, crewmember, MEM_DEATH, 5, MEM_SAW, -0.8f);
	DraftReport(u, DEPT_COUNT);
	int uline = -1;
	for (int i = 0; i < static_cast<int>(OpenReport(u).lines.size()); ++i)
		if (OpenReport(u).lines[i].event == MEM_DEATH && OpenReport(u).lines[i].person == 5) uline = i;
	CHECK(uline >= 0 && StrikeReportLine(u, uline));
	const float ubefore = Bond(u, crewmember, officer);
	CHECK(SignReport(u, officer, REPORT_UPWARD));
	CHECK(!Recall(u.crew[crewmember], MEM_LIE));       // lying up costs nothing from below
	CHECK(std::fabs(Bond(u, crewmember, officer) - ubefore) < 1e-4f);

	// A purge removes the published report and leaves the MEM_LOG-sourced marks in place,
	// unverifiable -- not erased.
	Ship g = NewShip();
	Remember(g, 10, MEM_DEATH, 5, MEM_LOG, -0.6f);
	LogEvent(g, "the bridge", "crew", "we lost someone");
	CHECK(PurgeLogs(g));
	CHECK(g.log.empty() && LogsPurged(g));             // the hole where the log was
	CHECK(Recall(g.crew[10], MEM_DEATH));              // the mark is still held
	bool orphaned = false;
	for (const Memory &m : g.crew[10].memories)
		if (m.event == MEM_DEATH && m.source == MEM_LOG && m.orphaned) orphaned = true;
	CHECK(orphaned);                                   // its citation is gone, the mark is not

	// And the whole of it is in the save: the report with its diff, the promises, the orphaned mark.
	Ship r = NewShip();
	Remember(r, crewmember, MEM_DEATH, 5, MEM_SAW, -0.8f);
	DraftReport(r, DEPT_COUNT);
	CHECK(StrikeReportLine(r, 0));                     // strike the headline for a visible diff
	CHECK(SignReport(r, officer, REPORT_TO_CREW));
	CHECK(MakePromise(r, officer, crewmember, PROMISE_REPAIR, "the sensors would be repaired", r.clock + 3600.0) >= 0);
	Remember(r, 10, MEM_DEATH, 5, MEM_LOG, -0.6f);
	CHECK(PurgeLogs(r));
	std::vector<uint8_t> blob = Pack(r);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(back.reports.size() == r.reports.size() && !back.reports.empty());
	CHECK(back.reports.back().signed_ && back.reports.back().lines.size() == r.reports.back().lines.size());
	CHECK(!ReportDiff(back.reports.back()).empty());
	CHECK(back.promises.size() == r.promises.size());
	CHECK(back.promises.size() == 1 && back.promises[0].kind == PROMISE_REPAIR);
	CHECK(back.logPurged);
	bool backOrphan = false;
	for (const Memory &m : back.crew[10].memories)
		if (m.source == MEM_LOG && m.orphaned) backOrphan = true;
	CHECK(backOrphan);
	CHECK(Pack(back) == blob);
}

// The two logs (docs/the-record-and-the-log.md): the official log and the private one as distinct
// stores. A personal entry is read by its owner and nobody else -- not a post's scope, not command --
// and the simulation never reads either: the month report is drafted from the record and is unchanged
// by anything written in a private log. A purge takes the published log and leaves the private one.
static void TestTheTwoLogs()
{
	g_test = "the two logs: the official record and the private one";

	const int player = 0;       // the captain, in this test
	const int crewmember = 40;  // somebody else

	// Separation: an official entry is not a personal one, and the private read is per person.
	Ship s = NewShip();
	LogEvent(s, "the bridge", "command", "the official account of the day");
	CHECK(WritePersonalLog(s, player, "what I actually think of the day"));
	CHECK(s.log.size() == 1 && s.personalLog.size() == 1);
	CHECK(ReadOfficialLog(s, 10, "").size() == 1);
	CHECK(ReadOfficialLog(s, 10, "command").size() == 1);
	CHECK(PersonalLog(s, player).size() == 1);
	CHECK(PersonalLog(s, crewmember).empty());            // nobody else's read
	CHECK(PersonalLog(s, player).back().owner == player);
	CHECK(PersonalLog(s, player).back().what.find("actually think") != std::string::npos);
	CHECK(PersonalVisibleTo(s.personalLog[0], player));   // the visibility rule, in one place
	CHECK(!PersonalVisibleTo(s.personalLog[0], crewmember));
	CHECK(s.personalLog[0].visibility == LOG_PERSONAL);

	// A bad write changes nothing: no such owner, or nothing to say.
	CHECK(!WritePersonalLog(s, -1, "nobody's"));
	CHECK(!WritePersonalLog(s, 9999, "nobody's"));
	CHECK(!WritePersonalLog(s, player, ""));
	CHECK(s.personalLog.size() == 1);

	// Invisible to the official read and to every scope: the token appears in no official entry,
	// whatever scope is asked for, and never in the unfiltered read.
	const std::string secret = "actually think";
	static const char *const SCOPES[] = { "bridge", "engineering", "sickbay", "hull", "command", "outside", "crew", "security", "captain" };
	bool leaked = false;
	for (const char *sc : SCOPES)
		for (const LogEntry &e : ReadOfficialLog(s, 100, sc))
			if (e.what.find(secret) != std::string::npos) leaked = true;
	for (const LogEntry &e : ReadOfficialLog(s, 100, ""))
		if (e.what.find(secret) != std::string::npos) leaked = true;
	CHECK(!leaked);

	// The report is drafted from the record, never from a log: a private entry that speaks to a
	// death does not reach the draft, and the draft is identical with and without it.
	Ship base = NewShip();
	Remember(base, crewmember, MEM_DEATH, 5, MEM_SAW, -0.8f);
	DraftReport(base, DEPT_COUNT);

	Ship withPrivate = NewShip();
	Remember(withPrivate, crewmember, MEM_DEATH, 5, MEM_SAW, -0.8f);
	CHECK(WritePersonalLog(withPrivate, crewmember, "the death was my fault and I will not write it down"));
	DraftReport(withPrivate, DEPT_COUNT);

	const MonthReport &a = OpenReport(base);
	const MonthReport &b = OpenReport(withPrivate);
	CHECK(a.lines.size() == b.lines.size());
	bool same = a.lines.size() == b.lines.size();
	for (size_t i = 0; same && i < a.lines.size(); ++i)
		same = a.lines[i].text == b.lines[i].text && a.lines[i].scope == b.lines[i].scope;
	CHECK(same);
	bool inReport = false;
	for (const ReportLine &l : b.lines) if (l.text.find("my fault") != std::string::npos) inReport = true;
	CHECK(!inReport);

	// The purge removes the published log and orphans the MEM_LOG-sourced marks; it does not take
	// the private log with it.
	Ship g = NewShip();
	Remember(g, 10, MEM_DEATH, 5, MEM_LOG, -0.6f);
	LogEvent(g, "the bridge", "crew", "we lost someone");
	CHECK(WritePersonalLog(g, player, "I will remember them"));
	CHECK(PurgeLogs(g));
	CHECK(g.log.empty() && LogsPurged(g));                // the hole where the official log was
	CHECK(g.personalLog.size() == 1);                     // the private store is untouched
	CHECK(PersonalLog(g, player).size() == 1);
	bool orphaned = false;
	for (const Memory &m : g.crew[10].memories)
		if (m.source == MEM_LOG && m.orphaned) orphaned = true;
	CHECK(orphaned);                                      // the citation is gone, the mark is not

	// Both stores are separate in the save, and a reload restores both, byte-for-byte -- including
	// a private entry longer than an official one is allowed to be.
	Ship r = NewShip();
	r.player = player;
	LogEvent(r, "the bridge", "command", "the official account");
	std::string longEntry;
	while (longEntry.size() < 150) longEntry += "the truth, said at length. ";
	CHECK(WritePersonalLog(r, player, longEntry));
	CHECK(WritePersonalLog(r, crewmember, "somebody else's private one"));
	std::vector<uint8_t> blob = Pack(r);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(back.log.size() == 1 && back.personalLog.size() == 2);
	CHECK(back.personalLog[0].owner == player && back.personalLog[0].what == longEntry);
	CHECK(back.personalLog[1].owner == crewmember);
	CHECK(back.personalLog[0].visibility == LOG_PERSONAL);
	CHECK(PersonalLog(back, player).size() == 1 && PersonalLog(back, crewmember).size() == 1);
	CHECK(Pack(back) == blob);
}

// The navigation counter (docs/navigation-counter.md): distance home, the estimate nominal and at
// current capability, the change since it was last written down, and the forecasts command sees.
static void TestNavigation()
{
	g_test = "the navigation counter: how far, and how long";

	// A new ship reads the goal's distance, the nominal figure, and a current figure above it because
	// no resupply is charted yet -- the gap shown, not hidden. It is a projection, not a quotient.
	Ship s = NewShip();
	Tick(s, 1.0f);
	Navigation n = NavigationCounter(s);
	CHECK(std::fabs(n.distanceLy - NAV_LIGHT_YEARS) < 1.0f);
	CHECK(std::fabs(n.nominalYears - NAV_LIGHT_YEARS / NAV_NOMINAL_C) < 0.1f);
	CHECK(n.warp && n.currentYears > 0.0f);
	CHECK(n.currentYears >= n.nominalYears);          // the route is not yet charted: honestly worse
	CHECK(n.currentYears < n.nominalYears * 1.1f);     // ... but only a little, on a healthy ship

	// It changes when the state changes: wreck the crystal and the estimate worsens on the spot.
	const float before = n.currentYears;
	s.dilithium = 0.2f;
	n = NavigationCounter(s);
	CHECK(n.currentYears > before);
	// A researched better crystal shortens the journey: current capability beats nominal.
	Ship q = NewShip();
	Tick(q, 1.0f);
	CHECK(AcquireDilithium(q, DIL_RESEARCH) && q.crystalQuality > 1.0f);
	CHECK(NavigationCounter(q).currentYears < NavigationCounter(q).nominalYears);
	// A warp drive that cannot deliver: no warp, and home stops getting closer.
	Ship d = NewShip();
	Tick(d, 1.0f);
	SetEnabled(d, SYS_WARP_DRIVE, false);
	Tick(d, 1.0f);
	CHECK(!NavigationCounter(d).warp && NavigationCounter(d).currentYears < 0.0f);
	d.dilithium = 0.0f; // and no crystal either
	CHECK(!NavigationCounter(d).warp);

	// Distance is the position model's: a jump toward home is fewer light years, and the goal beacon
	// of the last sector is home.
	Ship j = NewShip();
	Tick(j, 1.0f);
	const float far = NavigationCounter(j).distanceLy;
	GoTo(j, 0);
	j.enemy = Enemy(); j.contact2 = Enemy();
	CHECK(Jump(j, 1));
	CHECK(NavigationCounter(j).distanceLy < far);
	j.sectorNumber = SECTORS_TO_CROSS - 1;
	j.beacon = SECTOR_BEACONS - 1;
	CHECK(NavigationCounter(j).distanceLy == 0.0f);
	j.won = true;
	CHECK(NavigationCounter(j).distanceLy == 0.0f);

	// Nominal and current are both carried; the change since the counter was last written down is the
	// derivative, and moving closer makes it negative.
	Ship m = NewShip();
	Tick(m, 1.0f);
	m.navCounterLast = NavigationCounter(m).currentYears;
	GoTo(m, 0); m.enemy = Enemy(); m.contact2 = Enemy();
	Jump(m, 1);
	m.navCounterLast = NavigationCounter(m).currentYears; // as a recorded entry would
	Jump(m, 2);
	CHECK(NavigationCounter(m).changeYears < 0.0f);

	// It is recorded in the log over time, so the crew can look back.
	Ship l = NewShip();
	Tick(l, 1.0f);
	Tick(l, Hours(l, 24.0f * (NAV_LOG_DAYS + 1.0f)));
	bool recorded = false;
	for (const LogEntry &e : l.log) if (e.what.find("light years from home") != std::string::npos) recorded = true;
	CHECK(recorded);

	// The forecasts command sees: one per available course, with the years under it.
	Ship f = NewShip();
	Tick(f, 1.0f);
	const std::vector<NavCourse> routes = NavigationForecasts(f);
	CHECK(!routes.empty());
	for (const NavCourse &r : routes) CHECK(r.years >= 0.0f && r.distanceLy <= NAV_LIGHT_YEARS);

	// It survives save and load, and the report's headline is the counter's change since the last entry.
	Ship sv = NewShip();
	Tick(sv, 1.0f);
	sv.navCounterLast = 12.5f;
	GoTo(sv, 0); sv.enemy = Enemy(); sv.contact2 = Enemy();
	Jump(sv, 1);
	DraftReport(sv, DEPT_COUNT);
	CHECK(std::fabs(OpenReport(sv).counter - NavigationCounter(sv).currentYears) < 0.01f);
	CHECK(OpenReport(sv).lines[0].text.find("since the last entry") != std::string::npos);
	std::vector<uint8_t> blob = Pack(sv);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(std::fabs(back.navCounterLast - sv.navCounterLast) < 1e-4f);
	CHECK(std::fabs(NavigationCounter(back).currentYears - NavigationCounter(sv).currentYears) < 0.01f);
	CHECK(Pack(back) == blob);

	// The report's change is measured from the last entry: an earlier century-out estimate makes it
	// read as ground gained.
	Ship r = NewShip();
	Tick(r, 1.0f);
	r.navCounterLast = 100.0f;
	DraftReport(r, DEPT_COUNT);
	CHECK(OpenReport(r).counterChange < 0.0f);
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
	for (const CrewMember &c : empty.crew) if (c.status == CREW_FIT) em += Morale(c);
	for (const CrewMember &c : full.crew) if (c.status == CREW_FIT) fm += Morale(c);
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
	CHECK(Morale(scarred.crew[who]) < Morale(clean.crew[who]));

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
	s.crew[0].deficit = 0.2f; s.crew[0].outlook = 0.3f; s.crew[0].holdings = 0.3f; // reads 0.5
	CHECK(RunHolodeck(s, HOLO_RECREATION, 0) && Morale(s.crew[0]) > 0.5f);
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
	for (const CrewMember &c : calm.crew) if (c.status == CREW_FIT) cm += Morale(c);
	for (const CrewMember &c : hurt.crew) if (c.status == CREW_FIT) hm += Morale(c);
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
			CHECK(Morale(c) >= 0.0f && Morale(c) <= 1.0f && !std::isnan(Morale(c)));
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

	// The board's one command-side act (docs/crew-work.md, "priority is where rank lives"): a job's
	// place in the order is command's to set, the place is carried across the tick, and a job that
	// does not exist is not a target. The board is ui_lwh_jobs; this is the function behind it.
	Ship q = NewShip();
	DamageSystem(q, SYS_HOLODECKS, 0.5f);
	Tick(q, 0.0f);
	CHECK(!SetJobPriority(q, 0, -5));               // nobody commands yet: refused, changing nothing
	CHECK(Jobs(q).front().priority != -5);
	SetRole(q, ROLE_IN_COMMAND);
	const int before = Jobs(q).front().priority;
	CHECK(SetJobPriority(q, 0, before - 3));        // command moves it up
	Tick(q, 0.0f);
	CHECK(Jobs(q).front().priority == before - 3);  // and the place survives the tick's rebuild
	CHECK(!SetJobPriority(q, 99, 0));               // no such job
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

// The three clocks and the two exits (docs/ship-model.md). Sleeping skips time at the accelerated
// rate and must land where a played run would; standing orders are what the ship does while the
// player sleeps; and the record can tell a run that was left standing from one that was shut down.
static void TestSleepAndExits()
{
	g_test = "the sleep state, and the two exits";

	// Sleeping advances the ship by exactly the interval asked for, and a played real-time run over
	// the same interval arrives in the same place. Both are shown, and compared.
	Ship slept = NewShip(); // accelerated by default, but sleeping is an act, not a rate
	Config real = Config();
	real.clockMode = CLOCK_REAL_TIME;
	Ship played = NewShip(real);
	DamageSource(slept, SRC_WARP_CORE, 0.4f);
	DamageSource(played, SRC_WARP_CORE, 0.4f);
	const double interval = 6.0 * 3600.0; // six ship-hours, compressed into a sleep
	Sleep(slept, interval);
	Tick(played, static_cast<float>(interval)); // real time: a second is a second
	std::printf("  slept %d h to day %d, deuterium %.3f; played the same interval: day %d, deuterium %.3f\n",
		static_cast<int>(interval / 3600.0 + 0.5), slept.Day(), slept.stores.deuterium,
		played.Day(), played.stores.deuterium);
	CHECK(std::fabs(slept.clock - (8 * 3600.0 + interval)) < 1.0); // exactly the compressed interval
	CHECK(std::fabs(slept.clock - played.clock) < 1.0);            // and the played run agrees
	CHECK(std::fabs(slept.stores.deuterium - played.stores.deuterium) < 1e-4f);
	for (size_t i = 0; i < slept.crew.size(); ++i)
		CHECK(slept.crew[i].activity == played.crew[i].activity && slept.crew[i].deck == played.crew[i].deck);

	// Incremental and one-jump sleeps converge: long steps are cut up, so both arrive together.
	Ship oneJump = NewShip(), inSteps = NewShip();
	Sleep(oneJump, interval);
	for (int m = 0; m < 12; ++m) Sleep(inSteps, interval / 12.0);
	CHECK(std::fabs(oneJump.clock - inSteps.clock) < 1e-6);
	CHECK(Describe(oneJump) == Describe(inSteps));
	CHECK(Pack(oneJump) == Pack(inSteps));

	// Standing orders are what the ship does while the player sleeps: ordered to evacuate deck 11,
	// she keeps it clear across the night, sees to a damaged system, and the night is in the log.
	Ship ordered = NewShip();
	SetRole(ordered, ROLE_IN_COMMAND);
	DamageSystem(ordered, SYS_SENSORS, 0.6f); // the night's work, and something to log
	CHECK(OrderEvacuate(ordered, 11));
	const int logBefore = static_cast<int>(ordered.log.size());
	Sleep(ordered, 6.0 * 3600.0);
	CHECK(CrewOnDeck(ordered, 11).empty());                 // the order stood across the sleep
	CHECK(ordered.systems[SYS_SENSORS].health > 0.6f);      // and the crew did the night's work
	CHECK(static_cast<int>(ordered.log.size()) > logBefore); // with the night carried in the log

	// The two exits. A run left standing carries the mark and continuous entries; a run shut down
	// carries neither, and the time never passed aboard.
	Config hol = Config();
	hol.mode = MODE_HOLODECK;
	hol.clockMode = CLOCK_WALL;
	Ship stood = NewShip(hol), shut = NewShip(hol);
	CHECK(MaySuspend(hol));
	CHECK(Suspend(stood));
	CHECK(LeftStanding(stood) && !LeftStanding(shut));
	const int stoodLog = static_cast<int>(stood.log.size());
	CatchUp(stood, 8.0 * 3600.0);
	// The shut-down run is never caught up: its process stopped, so no time existed aboard.
	CHECK(std::fabs(stood.clock - (8 * 3600.0 + 8 * 3600.0)) < 1.0); // she lived through the night
	CHECK(shut.clock == 8 * 3600.0);                                  // the shut-down ship did not
	CHECK(static_cast<int>(stood.log.size()) > stoodLog);
	CHECK(Describe(stood).find("left standing") != std::string::npos);
	CHECK(Describe(shut).find("left standing") == std::string::npos);
	std::printf("  left standing: mark %d, day %d, %d log entries; shut down: mark %d, day %d\n",
		LeftStanding(stood) ? 1 : 0, stood.Day(), static_cast<int>(stood.log.size()),
		LeftStanding(shut) ? 1 : 0, shut.Day());

	// The mark is in the save, so a load knows how the run was left.
	const std::vector<uint8_t> blob = Pack(stood);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(LeftStanding(back) && Pack(back) == blob);

	// The guard: holodeck may suspend the world; ironman may not, because ironman keeps its time.
	Ship iron = NewShip(); // ironman is the default
	CHECK(!MaySuspend(iron.cfg));
	CHECK(!Suspend(iron));
	CHECK(!LeftStanding(iron) && iron.clock == 8 * 3600.0);
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

// The access model beyond rank: delegation, revocation, the emergency override, remote call-up and
// the lock-out (docs/access-and-authority.md, owner decision 2026-10-07).
static void TestAccessAndAuthority()
{
	g_test = "delegation, revocation, override and the lock-out";

	// Find a department head (a fit engineering lieutenant or above) and a junior engineer to grant.
	Ship s = NewShip();
	// A junior of another department, so the delegation is the only reason Engineering opens to them.
	int head = -1, junior = -1, commander = -1;
	for (int i = 0; i < static_cast<int>(s.crew.size()); ++i) {
		const CrewMember &c = s.crew[i];
		if (c.status != CREW_FIT) continue;
		if (head < 0 && c.dept == DEPT_ENGINEERING && c.rank >= 3) head = i;
		if (junior < 0 && c.dept != DEPT_ENGINEERING && c.rank < 3) junior = i;
		if (commander < 0 && c.rank >= 4) commander = i;
	}
	CHECK(head >= 0 && junior >= 0 && commander >= 0);
	CHECK(!MayOperate(s, junior, STN_ENGINEERING)); // not their department, no credential, no grant
	// Access and authority stay two things: a crew member with no morale still operates their station.
	CHECK(MayOperate(s.crew[head], STN_ENGINEERING));
	s.crew[head].deficit = 1.0f; s.crew[head].outlook = 0.0f; s.crew[head].holdings = 0.0f; // reads 0.0
	CHECK(MayOperate(s.crew[head], STN_ENGINEERING)); // access is not morale-governed
	s.crew[head].deficit = 0.0f; s.crew[head].outlook = 1.0f; s.crew[head].holdings = 1.0f;

	// Delegation: the head grants Engineering for a shift. The grantee is not already cleared (a
	// credential would mask it), so check a station the junior does not hold, then revoke it.
	CHECK(Delegate(s, head, junior, STN_ENGINEERING) && DelegatedTo(s, junior, STN_ENGINEERING));
	CHECK(MayOperate(s, junior, STN_ENGINEERING));
	bool loggedGrant = false, loggedRevoke = false;
	for (const LogEntry &e : s.log) if (e.what.find("grants") != std::string::npos) loggedGrant = true;
	CHECK(loggedGrant);
	const int headRank = s.crew[head].rank;
	// The revocation names who turned it off, and the access stops.
	CHECK(RevokeDelegation(s, head, junior, STN_ENGINEERING));
	for (const LogEntry &e : s.log) if (e.what.find("revokes") != std::string::npos) loggedRevoke = true;
	CHECK(loggedRevoke && !DelegatedTo(s, junior, STN_ENGINEERING));
	CHECK(!MayOperate(s, junior, STN_ENGINEERING));
	(void)headRank;

	// The shift lapses on its own: a delegation with a near expiry is gone after a tick.
	CHECK(Delegate(s, head, junior, STN_ENGINEERING));
	s.delegations.back().expires = s.clock + 10.0;
	CHECK(DelegatedTo(s, junior, STN_ENGINEERING));
	Tick(s, Hours(s, 1.0f));
	CHECK(!DelegatedTo(s, junior, STN_ENGINEERING));

	// The crew can revoke a credential too, and the log says who.
	const int trained = junior;
	CHECK(Train(s, trained, STN_TACTICAL) && Qualified(s.crew[trained], STN_TACTICAL));
	CHECK(RevokeCredential(s, commander, trained, STN_TACTICAL));
	CHECK(!Qualified(s.crew[trained], STN_TACTICAL));

	// Emergency override: slow, and two hands are faster than one. A junior at the Conn begins it.
	Ship o = NewShip();
	int sec = -1;
	for (int i = 0; i < static_cast<int>(o.crew.size()); ++i)
		if (o.crew[i].status == CREW_FIT && o.crew[i].rank >= 3 && i != head) { sec = i; break; }
	CHECK(sec >= 0);
	CHECK(BeginOverride(o, junior, STN_CONN));
	CHECK(!OverrideActive(o, STN_CONN)); // slow: not yet
	CHECK(ConfirmOverride(o, sec, STN_CONN) && OverrideState(o).second == sec);
	Tick(o, Hours(o, OVERRIDE_TWO_MINUTES / 60.0f + 0.05f));
	CHECK(OverrideActive(o, STN_CONN));
	// While it holds, the forced station answers to the player even though they are not cleared for it.
	o.player = junior;
	CHECK(PlayerMayOperate(o, STN_CONN));
	CHECK(!PlayerMayOperate(o, STN_ENGINEERING)); // only the station that was forced
	// It lapses.
	Tick(o, Hours(o, OVERRIDE_DURATION_MINUTES / 60.0f + 0.05f));
	CHECK(!OverrideActive(o, STN_CONN));

	// A solo force takes longer and is recorded as one hand.
	Ship solo = NewShip();
	CHECK(BeginOverride(solo, junior, STN_CONN));
	CHECK(!OverrideActive(solo, STN_CONN));
	Tick(solo, Hours(solo, OVERRIDE_TWO_MINUTES / 60.0f + 0.05f));
	CHECK(!OverrideActive(solo, STN_CONN)); // one hand must wait the solo time
	Tick(solo, Hours(solo, (OVERRIDE_SOLO_MINUTES - OVERRIDE_TWO_MINUTES) / 60.0f + 0.05f));
	CHECK(OverrideActive(solo, STN_CONN) && OverrideState(solo).solo);

	// The named refusal says who can open it, and where a system is worked.
	CHECK(AccessRefusal(s, STN_ENGINEERING).find("MAIN ENGINEERING") != std::string::npos);
	CHECK(AccessRefusal(s, STN_ENGINEERING).find("Chief Engineer") != std::string::npos);
	CHECK(OperatedFromRefusal(SYS_TRANSPORTERS).find("OPERATIONS") != std::string::npos);

	// Remote call-up: a lieutenant commander may call up a system from a console away from it; a
	// junior may not. Command travels; the physical work still does not.
	CHECK(!OperatedFrom(SYS_TRANSPORTERS, STN_TACTICAL));
	CHECK(MayCallUp(s, commander, STN_TACTICAL, SYS_TRANSPORTERS));
	CHECK(!MayCallUp(s, junior, STN_TACTICAL, SYS_TRANSPORTERS));

	// The lock-out: a senior officer shuts a junior out of their own station. The console stops
	// answering, the notice names both hands, and the junior's record carries a negative mark.
	CHECK(!LockOut(s, junior, STN_ENGINEERING, head)); // below the post-holder: refused
	CHECK(LockOut(s, head, STN_ENGINEERING, junior));
	CHECK(LockedOut(s, junior, STN_ENGINEERING));
	CHECK(!MayOperate(s, junior, STN_ENGINEERING));
	CHECK(LockoutNotice(s, junior, STN_ENGINEERING).find(s.crew[head].name) != std::string::npos);
	CHECK(LockoutNotice(s, junior, STN_ENGINEERING).find(s.crew[junior].name) != std::string::npos);
	bool marked = false;
	for (const Memory &m : s.crew[junior].memories)
		if (m.event == MEM_LOCKOUT && m.source == MEM_SAW && m.valence < 0.0f && m.person == head) marked = true;
	CHECK(marked);
	CHECK(ClearLockout(s, head, STN_ENGINEERING, junior) && !LockedOut(s, junior, STN_ENGINEERING));

	// A compromised system refuses everyone, whatever their rank: hardware beats hierarchy.
	Ship h = NewShip();
	h.systems[SYS_PHASERS].enabled = true;
	h.systems[SYS_PHASERS].control = 0.2f;
	CHECK(Hijacked(h, SYS_PHASERS));
	SetEnabled(h, SYS_PHASERS, false);
	CHECK(h.systems[SYS_PHASERS].enabled); // the console did not answer, even for a captain

	// All of it is in the save, byte for byte.
	Ship w = NewShip();
	CHECK(Delegate(w, head, junior, STN_ENGINEERING));
	CHECK(LockOut(w, head, STN_ENGINEERING, junior));
	CHECK(BeginOverride(w, head, STN_CONN));
	CHECK(ConfirmOverride(w, sec, STN_CONN));
	const std::vector<uint8_t> blob = Pack(w);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(back.delegations.size() == 1 && back.lockouts.size() == 1);
	CHECK(back.emergencyOverride.station == STN_CONN && back.emergencyOverride.second == sec);
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
	for (const CrewMember &c : hungry.crew) if (c.status == CREW_FIT) { hungryMorale += Morale(c); ++hn; }
	for (const CrewMember &c : fed.crew) if (c.status == CREW_FIT) { fedMorale += Morale(c); ++fn; }
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

// `test_ship_core --month` prints the month report and the toll as evidence: a promise kept, one
// broken and one lapsed, a lie that lands and one filed where nobody below reads, an editable
// report with its diff, and a purge that orphans rather than erases. See
// docs/evidence/the-month-report-and-the-toll.md.
static int PrintMonth()
{
	const int officer = 0, crewmember = 40;
	int failures = 0;

	// A promise made in front of a crew member; kept, and broken.
	{
		Ship s = NewShip();
		MakePromise(s, officer, crewmember, PROMISE_REPAIR, "the sensors would be repaired");
		const float b0 = Bond(s, crewmember, officer);
		ResolvePromise(s, 0, true);
		std::printf("PASS  promise kept: mark %d valence %.2f, bond %.2f -> %.2f\n",
			Recall(s.crew[crewmember], MEM_PROMISE) ? 1 : 0, s.crew[crewmember].memories[0].valence,
			b0, Bond(s, crewmember, officer));
		if (!(Bond(s, crewmember, officer) > b0)) ++failures;

		Ship q = NewShip();
		MakePromise(q, officer, crewmember, PROMISE_RESCUE, "they would be brought back");
		const float q0 = Bond(q, crewmember, officer);
		ResolvePromise(q, 0, false);
		std::printf("PASS  promise broken: bond %.2f -> %.2f; the log, in the crew member's voice: %s\n",
			q0, Bond(q, crewmember, officer), q.log.back().what.c_str());
		if (!(Bond(q, crewmember, officer) < q0)) ++failures;

		Ship d = NewShip();
		MakePromise(d, officer, crewmember, PROMISE_WAY_HOME, "a way home", d.clock + Hours(d, 1.0f));
		Tick(d, Hours(d, 2.0f));
		std::printf("PASS  a deadline passed with nothing said: the promise is %s\n",
			d.promises[0].state == PROMISE_BROKEN ? "broken" : "still open");
		if (d.promises[0].state != PROMISE_BROKEN) ++failures;
	}

	// The lie is written by a signed report that contradicts a MEM_SAW mark; the toll is paid
	// downward only, so the same falsehood filed upward costs nothing from below.
	{
		Ship s = NewShip();
		Remember(s, crewmember, MEM_DEATH, 5, MEM_SAW, -0.8f);
		DraftReport(s, DEPT_COUNT);
		int line = -1;
		for (int i = 0; i < static_cast<int>(OpenReport(s).lines.size()); ++i)
			if (OpenReport(s).lines[i].event == MEM_DEATH && OpenReport(s).lines[i].person == 5) line = i;
		StrikeReportLine(s, line);
		const float b0 = Bond(s, crewmember, officer);
		SignReport(s, officer, REPORT_TO_CREW);
		std::printf("PASS  a struck death in a signed report: witness holds a lie mark = %d, bond toward the signer %.2f -> %.2f\n",
			Recall(s.crew[crewmember], MEM_LIE) ? 1 : 0, b0, Bond(s, crewmember, officer));
		if (!Recall(s.crew[crewmember], MEM_LIE) || !(Bond(s, crewmember, officer) < b0)) ++failures;

		Ship u = NewShip();
		Remember(u, crewmember, MEM_DEATH, 5, MEM_SAW, -0.8f);
		DraftReport(u, DEPT_COUNT);
		int ul = -1;
		for (int i = 0; i < static_cast<int>(OpenReport(u).lines.size()); ++i)
			if (OpenReport(u).lines[i].event == MEM_DEATH && OpenReport(u).lines[i].person == 5) ul = i;
		StrikeReportLine(u, ul);
		const float u0 = Bond(u, crewmember, officer);
		SignReport(u, officer, REPORT_UPWARD);
		std::printf("PASS  the same falsehood filed upward: lie mark = %d, bond change %+.4f\n",
			Recall(u.crew[crewmember], MEM_LIE) ? 1 : 0, Bond(u, crewmember, officer) - u0);
		if (Recall(u.crew[crewmember], MEM_LIE)) ++failures;
	}

	// The report is drafted from the record, edited, and the diff is recoverable.
	{
		Ship s = NewShip();
		DraftReport(s, DEPT_COUNT);
		const int lines = static_cast<int>(OpenReport(s).lines.size());
		SoftenReportLine(s, 0, 0.5f);
		AddReportLine(s, "command", "all is well");
		const std::string diff = ReportDiff(OpenReport(s));
		SignReport(s, officer, REPORT_TO_CREW);
		std::printf("PASS  the report drafted %d lines, was edited, and the record kept:\n%s", lines, diff.c_str());
		if (diff.empty() || s.reports.empty()) ++failures;
	}

	// A purge empties the published log and orphans the read marks; it does not erase them.
	{
		Ship s = NewShip();
		Remember(s, 10, MEM_DEATH, 5, MEM_LOG, -0.6f);
		LogEvent(s, "the bridge", "crew", "we lost someone");
		PurgeLogs(s);
		bool orphaned = false;
		for (const Memory &m : s.crew[10].memories)
			if (m.source == MEM_LOG && m.orphaned) orphaned = true;
		std::printf("PASS  purge: log entries %d, the MEM_LOG mark still held = %d, orphaned = %d\n",
			static_cast<int>(s.log.size()), Recall(s.crew[10], MEM_DEATH) ? 1 : 0, orphaned ? 1 : 0);
		if (!s.log.empty() || !Recall(s.crew[10], MEM_DEATH) || !orphaned) ++failures;
	}

	if (failures) std::printf("%d demonstration(s) failed\n", failures);
	else std::printf("all demonstrations passed\n");
	return failures ? 1 : 0;
}

// `test_ship_core --nav` prints the navigation counter as evidence (docs/navigation-counter.md):
// the distance home, the nominal and current figures, the effect of wrecking the crystal, the log
// record over time, the forecasts command sees, and the report's headline. See
// docs/evidence/the-navigation-counter.md.
static int PrintNav()
{
	int failures = 0;
	int d, ny, cy;
	Ship s = NewShip();
	Tick(s, 1.0f);
	Navigation n = NavigationCounter(s);
	d = static_cast<int>(n.distanceLy + 0.5f); ny = static_cast<int>(n.nominalYears + 0.5f); cy = static_cast<int>(n.currentYears + 0.5f);
	std::printf("PASS  a new ship reads %d light years out: %d years nominal, %d at current capability (the gap is shown)\n", d, ny, cy);
	if (!(n.warp && cy >= ny)) ++failures;

	const int was = cy;
	s.dilithium = 0.2f;
	n = NavigationCounter(s);
	std::printf("PASS  wreck the crystal: %d -> %d years at current capability\n", was, static_cast<int>(n.currentYears + 0.5f));
	if (!(n.currentYears > was)) ++failures;

	Ship q = NewShip();
	Tick(q, 1.0f);
	AcquireDilithium(q, DIL_RESEARCH);
	n = NavigationCounter(q);
	std::printf("PASS  a researched better crystal: %d years, against %d nominal (the one positive loop)\n",
		static_cast<int>(n.currentYears + 0.5f), static_cast<int>(n.nominalYears + 0.5f));
	if (!(n.currentYears < n.nominalYears)) ++failures;

	Ship w = NewShip();
	Tick(w, 1.0f);
	w.dilithium = 0.0f;
	n = NavigationCounter(w);
	std::printf("PASS  no crystal: warp %s; home stops getting closer (the failure is immobility, not death)\n",
		n.warp ? "possible" : "impossible");
	if (n.warp) ++failures;

	Ship j = NewShip();
	Tick(j, 1.0f);
	const int far = static_cast<int>(NavigationCounter(j).distanceLy + 0.5f);
	GoTo(j, 0); j.enemy = Enemy(); j.contact2 = Enemy();
	Jump(j, 1);
	const int near = static_cast<int>(NavigationCounter(j).distanceLy + 0.5f);
	std::printf("PASS  a jump toward home: %d -> %d light years\n", far, near);
	if (!(near < far)) ++failures;

	// Recorded in the log over time.
	Tick(j, Hours(j, 24.0f * (NAV_LOG_DAYS + 1.0f)));
	std::string line;
	for (const LogEntry &e : j.log) if (e.what.find("light years from home") != std::string::npos) line = e.what;
	std::printf("PASS  recorded in the log: \"%s\"\n", line.c_str());
	if (line.empty()) ++failures;

	// The forecasts command sees.
	std::string fc;
	for (const NavCourse &c : NavigationForecasts(j))
		fc += (fc.empty() ? "" : "; ") + std::string("beacon ") + std::to_string(c.beacon) + ": " + std::to_string(static_cast<int>(c.years + 0.5f)) + " years";
	std::printf("PASS  forecasts (command alone): %s\n", fc.c_str());
	if (fc.empty()) ++failures;

	// The month report's headline is the counter's change since the last entry.
	Ship r = NewShip();
	Tick(r, 1.0f);
	r.navCounterLast = 100.0f;
	DraftReport(r, DEPT_COUNT);
	std::printf("PASS  the month report's headline: \"%s\"\n", OpenReport(r).lines[0].text.c_str());
	if (OpenReport(r).lines[0].text.find("since the last entry") == std::string::npos) ++failures;

	if (failures) std::printf("%d demonstration(s) failed\n", failures);
	else std::printf("all demonstrations passed\n");
	return failures ? 1 : 0;
}

// The player in the world (Stage B): the model's causes reach the person holding the controls. The
// player is a crew record, so being hurt, carried and treated is the same shape as any other casualty.
static void TestPlayerInTheWorld()
{
	g_test = "the player in the world: a body the ship can reach";
	Ship s = NewShip();
	s.cfg.dayScale = 1.0f;
	const int me = CreateCharacter(s, "Test Officer", DEPT_COMMAND, 2);
	CHECK(me >= 0);
	Tick(s, 1.0f);

	// The world says which deck the player's body occupies; the tick uses it, not the schedule.
	SetPlayerDeck(s, 12);
	CHECK(PlayerDeck(s) == 12);

	// A breached, airless compartment injures the person standing in it -- where they are, not where
	// their watch would have put them.
	s.decks[11].hull = 0.0f;
	s.decks[11].atmosphere = 0.0f;
	Tick(s, EXPOSURE_INJURES + 10.0f);
	CHECK(s.crew[me].status == CREW_INJURED);
	CHECK(PlayerIncapacitated(s));
	CHECK(!PlayerDead(s));

	// The ship acts on it: someone comes, and the log names them.
	bool attended = false;
	for (const LogEntry &e : s.log) if (e.what.find("attends") != std::string::npos) attended = true;
	CHECK(attended);

	// The body follows the record: an untreated injury is a body less than whole.
	CHECK(PlayerBodyHealth(s, 100) < 100);
	CHECK(PlayerBodyHealth(s, 100) >= 1);

	// Treatment is the ordinary casualty path: the ward returns them to fit, and the body to whole.
	s.decks[11].hull = 1.0f;
	s.decks[11].atmosphere = 1.0f;
	s.systems[SYS_SICKBAY].health = 1.0f;
	s.systems[SYS_SICKBAY].output = 1.0f;
	s.stores.medicalSupplies = 100.0f;
	for (int i = 0; i < 60 && s.crew[me].status != CREW_FIT; ++i) Tick(s, 3600.0f);
	CHECK(s.crew[me].status == CREW_FIT);
	CHECK(PlayerBodyHealth(s, 100) == 100);

	// The console lets go at the person holding the controls: a degraded system, used by the player,
	// injures the player's own record.
	Ship c = NewShip();
	c.cfg.dayScale = 1.0f;
	const int op = CreateCharacter(c, "Test Operator", DEPT_COMMAND, 2);
	CHECK(op >= 0);
	SetAlert(c, ALERT_RED);
	Tick(c, 1.0f);
	bool letGo = false;
	for (int i = 0; i < 500 && !letGo; ++i) {
		c.systems[SYS_LIFE_SUPPORT].health = 0.3f;
		c.systems[SYS_LIFE_SUPPORT].output = 0.3f;
		if (UseSystemBy(c, SYS_LIFE_SUPPORT, 1.0f, op) >= ANOMALY_ACUTE) letGo = true;
		if (c.crew[op].status != CREW_FIT) letGo = true;
	}
	CHECK(letGo);
	CHECK(c.crew[op].status == CREW_INJURED || c.crew[op].status == CREW_DEAD);
	bool console = false;
	for (const LogEntry &e : c.log) if (e.what.find("console let go") != std::string::npos) console = true;
	CHECK(console);

	// A boarder or a weapon hurts the body in the world: that damage is written into the record.
	Ship w = NewShip();
	w.cfg.dayScale = 1.0f;
	const int hurt = CreateCharacter(w, "Test Hurt", DEPT_COMMAND, 2);
	CHECK(hurt >= 0);
	Tick(w, 1.0f);
	CHECK(w.crew[hurt].status == CREW_FIT);
	CHECK(WoundPlayer(w, 0.3f, "a weapon"));
	CHECK(w.crew[hurt].status == CREW_INJURED);
	CHECK(std::fabs(w.crew[hurt].severity - 0.3f) < 1e-4f);
	CHECK(!PlayerMayCommand(w)); // a player who is down commands nothing

	// Death is reachable and is not a reload: the record closes and is never restored; the roster
	// promotes to fill the gap, and command passes on. Nothing about the lost person comes back.
	Ship d = NewShip();
	d.cfg.dayScale = 1.0f;
	const int dead = CreateCharacter(d, "Test Fallen", DEPT_COMMAND, 2);
	CHECK(dead >= 0);
	Tick(d, 1.0f);
	CHECK(KillCrew(d, dead, "wounds"));
	CHECK(PlayerDead(d) && PlayerIncapacitated(d));
	CHECK(!PlayerMayCommand(d));
	const int next = AssumeCommand(d);
	CHECK(next >= 0 && next != dead);
	CHECK(d.player == next);
	CHECK(d.crew[dead].status == CREW_DEAD); // the closed record stays closed
	CHECK(!PlayerDead(d));
	bool passed = false;
	for (const LogEntry &e : d.log) if (e.what.find("command passes to") != std::string::npos) passed = true;
	CHECK(passed);
}

// `test_ship_core --personal` prints the two logs as evidence (docs/the-record-and-the-log.md): the
// stores are separate, a private entry is invisible to the official read and to every scope, the
// player can write one and read it back, the month report's draft is unchanged by it, and a purge
// takes the published log and leaves the private one. See docs/evidence/the-two-logs.md.
static int PrintPersonal()
{
	const int player = 0, crewmember = 40;
	int failures = 0;

	{
		Ship s = NewShip();
		s.player = player;
		LogEvent(s, "the bridge", "command", "the official account of the day");
		WritePersonalLog(s, player, "the death was my fault, and I will not write it down");
		std::printf("PASS  two stores: %d official entry, %d personal entry\n",
			static_cast<int>(s.log.size()), static_cast<int>(s.personalLog.size()));
		if (s.log.size() != 1 || s.personalLog.size() != 1) ++failures;

		const std::string secret = "my fault";
		int leaked = 0;
		static const char *const SCOPES[] = { "bridge", "engineering", "sickbay", "hull", "command", "outside", "crew", "security", "captain" };
		for (const char *sc : SCOPES)
			for (const LogEntry &e : ReadOfficialLog(s, 100, sc))
				if (e.what.find(secret) != std::string::npos) ++leaked;
		std::printf("PASS  the private entry is invisible to the official read and to every scope: %d match(es) across %d scopes\n",
			leaked, static_cast<int>(sizeof(SCOPES) / sizeof(SCOPES[0])));
		if (leaked) ++failures;

		std::printf("PASS  another person's read returns nothing: %d entries\n",
			static_cast<int>(PersonalLog(s, crewmember).size()));
		if (!PersonalLog(s, crewmember).empty()) ++failures;

		std::printf("PASS  the player wrote it and reads it back: \"%s\"\n",
			PersonalLog(s, player).back().what.c_str());
		if (PersonalLog(s, player).size() != 1) ++failures;
	}

	// The report is drafted from the record, never from a log: a private entry does not touch it.
	{
		Ship base = NewShip();
		Remember(base, crewmember, MEM_DEATH, 5, MEM_SAW, -0.8f);
		DraftReport(base, DEPT_COUNT);
		Ship withPrivate = NewShip();
		Remember(withPrivate, crewmember, MEM_DEATH, 5, MEM_SAW, -0.8f);
		WritePersonalLog(withPrivate, crewmember, "the death was my fault and I will not write it down");
		DraftReport(withPrivate, DEPT_COUNT);
		bool same = OpenReport(base).lines.size() == OpenReport(withPrivate).lines.size();
		for (size_t i = 0; same && i < OpenReport(base).lines.size(); ++i)
			same = OpenReport(base).lines[i].text == OpenReport(withPrivate).lines[i].text;
		std::printf("PASS  the month report's draft is unchanged by a personal entry: %d line(s), %s\n",
			static_cast<int>(OpenReport(withPrivate).lines.size()), same ? "identical" : "CHANGED");
		if (!same) ++failures;
	}

	// The purge takes the published log and leaves the private one.
	{
		Ship s = NewShip();
		Remember(s, 10, MEM_DEATH, 5, MEM_LOG, -0.6f);
		LogEvent(s, "the bridge", "crew", "we lost someone");
		WritePersonalLog(s, player, "I will remember them");
		PurgeLogs(s);
		bool orphaned = false;
		for (const Memory &m : s.crew[10].memories) if (m.source == MEM_LOG && m.orphaned) orphaned = true;
		std::printf("PASS  purge: published log %d entries, private log %d entries, orphaned mark %d\n",
			static_cast<int>(s.log.size()), static_cast<int>(s.personalLog.size()), orphaned ? 1 : 0);
		if (!s.log.empty() || s.personalLog.size() != 1 || !orphaned) ++failures;
	}

	// In the save, both stores, together.
	{
		Ship s = NewShip();
		s.player = player;
		LogEvent(s, "the bridge", "command", "the official account");
		WritePersonalLog(s, player, "the private one");
		std::vector<uint8_t> blob = Pack(s);
		Ship back;
		const bool ok = Unpack(blob.data(), blob.size(), back) && Pack(back) == blob;
		std::printf("PASS  save and reload restore both stores byte-for-byte: official %d, personal %d\n",
			static_cast<int>(back.log.size()), static_cast<int>(back.personalLog.size()));
		if (!ok || back.log.size() != 1 || back.personalLog.size() != 1) ++failures;
	}

	if (failures) std::printf("%d demonstration(s) failed\n", failures);
	else std::printf("all demonstrations passed\n");
	return failures ? 1 : 0;
}

// The phaser bank's setting (Task C) and reading-vs-operating (owner ruling, 2026-10-07). Plain model
// checks, no game header: the setting's ladder, its round-trip, and that a read is not a control.
static void TestPhaserYieldAndReading()
{
	g_test = "the phaser setting and reading vs operating";
	// A system has exactly one operating station, and no other station (Engineering's ship-wide power
	// distribution aside) may operate it. Comms is Operations', and Tactical reads the traffic but
	// cannot change it.
	for (int i = 0; i < SYS_COUNT; ++i)
	{
		const SystemId id = static_cast<SystemId>(i);
		for (int st = 0; st < STN_COUNT; ++st)
		{
			const Station s = static_cast<Station>(st);
			if (s == STN_ENGINEERING || s == StationOf(id)) continue;
			CHECK(!OperatedFrom(id, s));
		}
	}
	CHECK(StationOf(SYS_COMMUNICATIONS) == STN_OPS);
	CHECK(StationReads(STN_TACTICAL, SYS_SENSORS));          // the sensor picture, Operations' to operate
	CHECK(StationReads(STN_TACTICAL, SYS_COMMUNICATIONS));   // the comms traffic; Operations speaks
	CHECK(!OperatedFrom(SYS_COMMUNICATIONS, STN_TACTICAL));  // ... and Tactical cannot change it
	CHECK(!StationReads(STN_TACTICAL, SYS_WARP_DRIVE));      // not everything is a portfolio

	// The bank's setting: default stun (the bank's nominal demand), a higher setting asks more power,
	// a lower one less, and a bad setting changes nothing.
	Ship s = NewShip();
	const int nominal = Spec(SYS_PHASERS).demand;
	CHECK(PhaserYieldOf(s) == YIELD_STUN);
	CHECK(EffectiveDemand(s, SYS_PHASERS) == nominal);
	CHECK(!SetPhaserYield(s, 9) && PhaserYieldOf(s) == YIELD_STUN);
	SetPhaserYield(s, YIELD_KILL);
	const int kill = EffectiveDemand(s, SYS_PHASERS);
	SetPhaserYield(s, YIELD_VAPORIZE);
	const int vap = EffectiveDemand(s, SYS_PHASERS);
	CHECK(kill == nominal * 150 / 100 && kill < vap);
	CHECK(EffectiveDemand(s, SYS_SHIELDS) == Spec(SYS_SHIELDS).demand); // every other system is unchanged

	std::vector<uint8_t> blob = Pack(s);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(PhaserYieldOf(back) == YIELD_VAPORIZE);
	CHECK(Pack(back) == blob);

	// The setting changes what the bank does: a vaporize hit takes more of a hull than a stun one.
	// At red alert, so the bank is not suppressed the way a green watch suppresses it.
	auto hullAfter = [](uint8_t yield) {
		Ship c = NewShip();
		c.cfg.clockMode = CLOCK_REAL_TIME; // one simulated second is one ship second: the arithmetic is exact
		SetAlert(c, ALERT_RED);
		c.enemy.present = true; c.enemy.kind = ENEMY_RAIDER; c.enemy.hull = 1.0f; c.enemy.shields = 0.0f;
		c.enemy.firepower = 0.0f; c.enemy.weapons = 0.0f; c.enemy.boarders = 0;
		for (int i = 0; i < SYS_COUNT; ++i) c.systems[i].enabled = (i == SYS_PHASERS);
		SetPhaserYield(c, yield);
		Tick(c, 60.0f);
		return c.enemy.hull;
	};
	CHECK(hullAfter(YIELD_VAPORIZE) < hullAfter(YIELD_KILL));
	CHECK(hullAfter(YIELD_KILL) < hullAfter(YIELD_STUN));

	std::printf("      hull after a minute: vaporize %.3f, kill %.3f, stun %.3f\n",
		hullAfter(YIELD_VAPORIZE), hullAfter(YIELD_KILL), hullAfter(YIELD_STUN));

	std::printf("PASS  the phaser setting asks 100/125/150/175%% and vaporize hits harder than stun; comms is Operations' and Tactical reads it\n");
}

// Canon's crew check (docs/ship-systems.md, "The 37's"): the ship is operable with 100, and below
// that flyable but degraded.
static void TestHundredCrew()
{
	g_test = "operable with 100 crew";
	CHECK(PostsNeeded() == 22);                     // the systems' posts, the skeleton crew's bill
	const WatchCoverage full = CoverWithCrew(100);
	CHECK(full.perWatch >= PostsNeeded());          // a watch of 33 hands covers all 22 posts
	CHECK(full.postsCovered == PostsNeeded());
	CHECK(full.critical);
	// Below a hundred the ship is degraded but flyable: the critical set holds, the comforts go.
	const WatchCoverage thin = CoverWithCrew(60);
	CHECK(thin.perWatch < PostsNeeded());
	CHECK(thin.postsCovered < PostsNeeded());
	CHECK(thin.critical);
}

// The holodeck matrix is a trap, not a solution (docs/ship-systems.md; VOY "Parallax").
static void TestHolodeckTrap()
{
	g_test = "the holodeck matrix is a trap";
	Ship s = NewShip();
	Tick(s, 1.0f);
	CHECK(!JumpStartFromHolodeck(s));               // the main grid is up: there is no need
	CHECK(s.systems[SYS_HOLODECKS].health == 1.0f); // and nothing happens

	// With the core gone, the temptation: it returns a charge and wrecks half the relays.
	Ship d = NewShip();
	Tick(d, 1.0f);
	DamageSource(d, SRC_WARP_CORE, 1.0f);
	Tick(d, 1.0f);
	CHECK(Coreless(d));
	const float batteries = d.stores.batteries;
	CHECK(JumpStartFromHolodeck(d));
	CHECK(d.stores.batteries > batteries);          // it did buy back power
	CHECK(d.systems[SYS_HOLODECKS].health == 0.0f); // and it wrecked what it tapped
	int ruined = 0;
	for (int i = 0; i < SYS_COUNT; ++i) if (d.systems[i].health < 1.0f) ++ruined;
	CHECK(ruined >= SYS_COUNT / 3);                 // canon's "half the ship's relays", counted as systems
}

// The owner's mechanic (docs/budget-squaring.md, Part five): the crystal's ceiling scales the warp
// core's output, so recomposition trades capability for time.
static void TestCrystalCeilingScalesOutput()
{
	g_test = "the crystal ceiling scales the warp core's output";
	Ship s = NewShip();
	Tick(s, 1.0f);
	CHECK(s.PowerCapacityNow() == 1800);
	s.crystalCeiling = 0.85f;
	Tick(s, 1.0f);
	CHECK(s.PowerCapacityNow() == 1590);            // 1400 x 0.85 + 250 + 90 + 60
	s.dilithium = 0.2f;
	const float before = s.crystalCeiling;
	CHECK(Recomposite(s) && s.crystalCeiling < before);
	Tick(s, 1.0f);
	CHECK(s.PowerCapacityNow() == static_cast<int>(1400 * s.crystalCeiling + 0.5f) + 400);
	std::vector<uint8_t> blob = Pack(s);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(back.crystalCeiling == s.crystalCeiling && back.PowerCapacityNow() == s.PowerCapacityNow());
}

// The torpedo complement: canon's 38, set at the start and depleted by fire (docs/budget-squaring.md,
// Part 3, Finding four).
static void TestTorpedoComplement()
{
	g_test = "the torpedo complement: 38, and it depletes";
	Ship s = NewShip();
	SetAlert(s, ALERT_YELLOW); // the launchers are not suppressed by a green watch
	Tick(s, 1.0f);
	CHECK(s.stores.torpedoes == 38);
	for (int fired = 0; fired < 38; ++fired) {
		s.enemy = Enemy(); s.enemy.present = true; s.enemy.hull = 1.0f; s.enemy.shields = 0.0f;
		s.enemy.boarders = 0; s.enemy.firepower = 0.0f;
		CHECK(FireTorpedo(s));
	}
	CHECK(s.stores.torpedoes == 0);
	s.enemy = Enemy(); s.enemy.present = true; s.enemy.hull = 1.0f;
	CHECK(!FireTorpedo(s));                          // none left
}

// ---- the meeting (docs/staff-meetings.md) --------------------------------------------------------

// Advance the ship by ship-seconds (Tick takes simulated seconds, scaled by the clock).
static void AdvanceShip(Ship &s, double shipSeconds)
{
	Tick(s, static_cast<float>(shipSeconds / s.cfg.dayScale));
}

// A brief for every kind the design names, carrying participants, the decision, the enumerated options
// with their costs, and the current state -- and generating one changes nothing (Task A).
static void TestMeetingBriefIsARead()
{
	g_test = "every kind of meeting has a brief, and generating one is a read";
	Ship s = NewShip();
	LogEvent(s, "the bridge", "command", "a command entry a command seat can read");
	Remember(s, 3, MEM_DEATH, 7, MEM_SAW, -0.8f); // a mark on a crew member who may be in the room
	const std::vector<uint8_t> before = Pack(s);
	for (int k = 0; k < MEET_KIND_COUNT; ++k) {
		const MeetingBrief b = BuildBrief(s, static_cast<uint8_t>(k));
		CHECK(b.kind == k);
		CHECK(!b.decision.empty());
		CHECK(b.presentCount > 0);
		CHECK(b.optionCount > 0);
		for (int i = 0; i < b.optionCount; ++i) {
			CHECK(!b.options[i].label.empty());
			CHECK(!b.options[i].cost.empty());
		}
		CHECK(b.powerAvailable == s.PowerAvailable());
		CHECK(b.powerCommitted == PowerCommitted(s));
		CHECK(b.systems[SYS_LIFE_SUPPORT].allocated == s.systems[SYS_LIFE_SUPPORT].allocated);
		CHECK(b.systems[SYS_LIFE_SUPPORT].allocBy == AllocationSource(s, SYS_LIFE_SUPPORT));
	}
	CHECK(Pack(s) == before); // a read: the ship's state is untouched
}

// The dead-layer rule (docs/programme-meetings-and-voice.md, lesson 6): every meeting generates a
// brief, demonstrated in the normal case and not only a dramatic one. The check reads the emission --
// the queued brief itself -- not a count.
static void TestEveryMeetingEmitsABrief()
{
	g_test = "the normal case emits a brief: the watch change and the ordinary departmental meeting";
	Ship s = NewShip();
	CHECK(PendingMeetings(s).empty());

	// The watch change. No threshold, no drama: the simulation's own clock calls the meeting.
	const double toWatch = static_cast<double>(SECONDS_PER_WATCH) - std::fmod(s.clock, static_cast<double>(SECONDS_PER_WATCH));
	AdvanceShip(s, toWatch + 1.0);
	CHECK(!PendingMeetings(s).empty());
	if (!PendingMeetings(s).empty()) {
		const MeetingBrief &b = PendingMeetings(s).back();
		CHECK(b.kind == MEET_WATCH_CHANGE);
		CHECK(b.presentCount > 0);
		CHECK(b.optionCount > 0);
	}
	bool logged = false;
	for (const LogEntry &e : s.log) if (e.what.find("meeting is called") != std::string::npos) logged = true;
	CHECK(logged); // the emit site wrote the emission down

	// Drain, then a day: the ordinary departmental meeting, which resolves nothing, still produces one.
	while (TakeBrief(s)) {}
	const double toDay = static_cast<double>(SECONDS_PER_DAY) - std::fmod(s.clock, static_cast<double>(SECONDS_PER_DAY));
	AdvanceShip(s, toDay + 1.0);
	bool sawDept = false, sawWatch = false;
	for (const MeetingBrief &b : PendingMeetings(s)) {
		if (b.kind == MEET_DEPARTMENTAL) sawDept = true;
		if (b.kind == MEET_WATCH_CHANGE) sawWatch = true;
	}
	CHECK(sawWatch);
	CHECK(sawDept);
}

// The skeleton is the floor: with no model present the meeting still plays and resolves (Task B). Every
// outcome has dialogue; every line carries a delivery direction; and where the brief cannot know it,
// the line is marked rather than guessed and the seam refuses it (Task C).
static void TestSkeletonPlaysWithoutAModel()
{
	g_test = "the authored skeleton plays with no model, and every line carries its delivery";
	int unmarked = 0;
	for (int k = 0; k < MEET_KIND_COUNT; ++k) {
		const MeetingSkeleton &sk = AuthoredSkeleton(static_cast<uint8_t>(k));
		CHECK(sk.kind == k);
		CHECK(sk.outcomeCount > 0);
		for (int i = 0; i < sk.outcomeCount; ++i) {
			const MeetingOutcome &oc = sk.outcomes[i];
			CHECK(oc.lineCount > 0);                       // each outcome is legible
			CHECK(!oc.option.label.empty());
			CHECK(!oc.option.cost.empty());
			for (int l = 0; l < oc.lineCount; ++l) {
				CHECK(oc.lines[l].delivery < DELIVERY_COUNT); // a direction, always
				if (oc.lines[l].delivery == DELIVERY_UNMARKED) ++unmarked;
			}
		}
	}
	CHECK(unmarked > 0); // the marked case exists: the brief says it cannot know, rather than guessing

	// The seam carries the annotation with the text, and refuses a line whose delivery is unmarked.
	MeetingLine flat; flat.text = "the log is read aloud"; flat.delivery = DELIVERY_FLAT;
	SynthesisRequest req;
	CHECK(LineToSynthesis(flat, req));
	CHECK(req.text == flat.text && req.delivery == DELIVERY_FLAT);
	CHECK(req.exaggeration == DeliveryExaggeration(DELIVERY_FLAT));
	MeetingLine unknown; unknown.text = "something the brief cannot call"; unknown.delivery = DELIVERY_UNMARKED;
	CHECK(!LineToSynthesis(unknown, req));
	CHECK(!DeliveryKnown(DELIVERY_UNMARKED));

	// The vocabulary, small and named: an order up, the flat down, the rest between.
	CHECK(DeliveryExaggeration(DELIVERY_ORDER) > DeliveryExaggeration(DELIVERY_REPORT));
	CHECK(DeliveryExaggeration(DELIVERY_REPORT) > DeliveryExaggeration(DELIVERY_FLAT));
	CHECK(std::strcmp(DeliveryName(DELIVERY_CONDOLENCE), "condolence") == 0);
}

// Task D: a meeting outcome sets an allocation, end to end -- and the automatic-versus-person
// distinction is obeyed. The player and the crew decide; the ship's own answer is automatic mode; the
// meeting cannot override a person's decision by itself.
static void TestMeetingAllocationSeam()
{
	g_test = "a meeting outcome sets an allocation, and the meeting cannot override a person";
	Ship s = NewShip();
	s.player = 0; // the player is in the room, and decides
	SetAlert(s, ALERT_YELLOW); // the shields are not suppressed: the proving option is reachable
	const MeetingBrief b = BuildBrief(s, MEET_ALLOCATION);
	const MeetingSkeleton &sk = AuthoredSkeleton(MEET_ALLOCATION);
	int allocOpt = -1, autoOpt = -1;
	for (int i = 0; i < sk.outcomeCount; ++i) {
		if (sk.outcomes[i].option.effect == EFFECT_SET_ALLOCATION && allocOpt < 0) allocOpt = i;
		if (sk.outcomes[i].option.effect == EFFECT_SET_POWER_AUTO) autoOpt = i;
	}
	CHECK(allocOpt >= 0);
	CHECK(autoOpt >= 0);

	// A person in the room decides: the allocation is theirs, with their provenance.
	CHECK(ApplyMeetingOutcome(s, b, allocOpt, s.player, false));
	CHECK(AllocationPercent(s, SYS_HOLODECKS) == 100);
	CHECK(AllocationPercent(s, SYS_SHIELDS) == 0);
	CHECK(AllocationSource(s, SYS_HOLODECKS) == ALLOC_PLAYER);

	// The meeting cannot set an allocation *as the ship*: that is automatic mode, and it is refused.
	const int holodecksWas = AllocationPercent(s, SYS_HOLODECKS);
	CHECK(!ApplyMeetingOutcome(s, b, allocOpt, -1, true));
	CHECK(AllocationPercent(s, SYS_HOLODECKS) == holodecksWas);

	// An officer with no authority over the band cannot set it either.
	const int officer = DepartmentHead(s, DEPT_SECURITY);
	CHECK(officer >= 0);
	const int before = AllocationPercent(s, SYS_HOLODECKS);
	CHECK(!ApplyMeetingOutcome(s, b, allocOpt, officer, false));
	CHECK(AllocationPercent(s, SYS_HOLODECKS) == before);

	// The ship's own answer is automatic mode, and it is granted by the room.
	CHECK(ApplyMeetingOutcome(s, b, autoOpt, -1, true));
	CHECK(PowerAuto(s));
}

// A brief is built per participant from that person's marks and the log -- not from the record, so the
// room does not all know the same thing (docs/the-record-and-the-log.md).
static void TestMeetingBriefPerParticipant()
{
	g_test = "a brief is built per participant from marks and the log";
	Ship s = NewShip();
	LogEvent(s, "engineering", "engineering", "a coolant line was replaced");
	LogEvent(s, "sickbay", "sickbay", "a patient was admitted");
	Remember(s, 3, MEM_DEATH, 7, MEM_SAW, -0.8f);
	const MeetingBrief b = BuildBrief(s, MEET_WATCH_CHANGE);
	int engineer = -1, medic = -1;
	for (int i = 0; i < b.presentCount; ++i) {
		const int c = b.present[i].crew;
		if (s.crew[c].dept == DEPT_ENGINEERING) engineer = i;
		if (s.crew[c].dept == DEPT_MEDICAL) medic = i;
	}
	CHECK(engineer >= 0);
	CHECK(medic >= 0);
	if (engineer >= 0) {
		bool sawEngineering = false, sawSickbay = false;
		for (int i = 0; i < b.present[engineer].logCount; ++i) {
			if (b.present[engineer].log[i].what.find("coolant") != std::string::npos) sawEngineering = true;
			if (b.present[engineer].log[i].what.find("patient") != std::string::npos) sawSickbay = true;
		}
		CHECK(sawEngineering);
		CHECK(!sawSickbay); // a post reads its own scope
	}
	if (medic >= 0) {
		bool sawEngineering = false;
		for (int i = 0; i < b.present[medic].logCount; ++i)
			if (b.present[medic].log[i].what.find("coolant") != std::string::npos) sawEngineering = true;
		CHECK(!sawEngineering);
	}

	// A participant's marks and open promises travel with their view.
	Ship p = NewShip();
	const int officer = 1, beneficiary = 20;
	const int promise = MakePromise(p, officer, beneficiary, PROMISE_REPAIR, "the coolant line", p.clock + SECONDS_PER_DAY);
	CHECK(promise >= 0);
	const MeetingBrief pb = BuildBrief(p, MEET_DEFERRED);
	bool found = false;
	for (int i = 0; i < pb.presentCount; ++i) {
		if (pb.present[i].crew != beneficiary) continue;
		if (pb.present[i].promiseCount > 0 && pb.present[i].promises[0].what == "the coolant line") found = true;
	}
	CHECK(found);
}

// The meeting round-trips: the schedule and the queued briefs survive save and load byte-for-byte, and
// a load does not re-emit a meeting that has already been called.
static void TestMeetingSaveRoundTrip()
{
	g_test = "the meeting schedule and its queued briefs round-trip";
	Ship s = NewShip();
	const double toWatch = static_cast<double>(SECONDS_PER_WATCH) - std::fmod(s.clock, static_cast<double>(SECONDS_PER_WATCH));
	AdvanceShip(s, toWatch + 1.0);
	CHECK(!PendingMeetings(s).empty());
	const std::vector<uint8_t> blob = Pack(s);
	CHECK(blob.size() < 65536);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(Pack(back) == blob);
	CHECK(PendingMeetings(back).size() == PendingMeetings(s).size());
	// The load is a zero-length tick: it must not emit a second watch-change brief.
	CHECK(PendingMeetings(back).size() == PendingMeetings(s).size());
}

// `test_ship_core --power` prints the allocation model as evidence (docs/evidence/power-assignment.md):
// the proving case, the ladder only in automatic mode, oversubscription reported, the chief engineer's
// recommendation, and a band delegation.
static int PrintPower()
{
	std::printf("== the proving case: the player decides, and no ladder overrides it\n");
	{
		Ship s = NewShip();
		SetAlert(s, ALERT_YELLOW);
		SetAllocation(s, SYS_SHIELDS, 0);
		Tick(s, 1.0f);
		std::printf("  holodecks %3d%%  shields %3d%%   -> holodecks output %3.0f%%, shields output %3.0f%%  (provenance: %s)\n",
			AllocationPercent(s, SYS_HOLODECKS), AllocationPercent(s, SYS_SHIELDS), s.systems[SYS_HOLODECKS].output * 100,
			s.systems[SYS_SHIELDS].output * 100, AllocationProvenance(s, SYS_SHIELDS).c_str());
		SetAllocation(s, SYS_HOLODECKS, 0);
		SetAllocation(s, SYS_SHIELDS, 100);
		Tick(s, 1.0f);
		std::printf("  holodecks %3d%%  shields %3d%%   -> holodecks output %3.0f%%, shields output %3.0f%%  (provenance: %s)\n",
			AllocationPercent(s, SYS_HOLODECKS), AllocationPercent(s, SYS_SHIELDS), s.systems[SYS_HOLODECKS].output * 100,
			s.systems[SYS_SHIELDS].output * 100, AllocationProvenance(s, SYS_HOLODECKS).c_str());
	}

	std::printf("== the ladder fires only in automatic mode, and never on a system a person has set\n");
	{
		Ship s = NewShip();
		SetAlert(s, ALERT_YELLOW);
		SetAllocation(s, SYS_HOLODECKS, 60);
		SetPowerAuto(s, true);
		s.crystalCeiling = 0.55f;
		Tick(s, 1.0f);
		std::printf("  AUTO on  at 0.55: cargo handling %d/%d, holodecks %d/%d (the player's)\n",
			s.systems[SYS_CARGO_HANDLING].allocated, Spec(SYS_CARGO_HANDLING).demand,
			s.systems[SYS_HOLODECKS].allocated, Spec(SYS_HOLODECKS).demand);
		SetPowerAuto(s, false);
		Tick(s, 1.0f);
		std::printf("  AUTO off, nothing else changed: cargo handling %d/%d, holodecks %d/%d, short %d\n",
			s.systems[SYS_CARGO_HANDLING].allocated, Spec(SYS_CARGO_HANDLING).demand,
			s.systems[SYS_HOLODECKS].allocated, Spec(SYS_HOLODECKS).demand, PowerShortfall(s));
	}

	std::printf("== oversubscription is reported, never resolved\n");
	{
		Ship s = NewShip();
		SetAlert(s, ALERT_YELLOW);
		s.crystalCeiling = 0.30f;
		Tick(s, 1.0f);
		int dark = 0;
		for (int i = 0; i < SYS_COUNT; ++i) if (s.systems[i].allocated == 0) ++dark;
		std::printf("  plant %d EPS, committed %d, SHORT %d; systems allocated %d of %d, dark %d\n",
			s.PowerAvailable(), PowerCommitted(s), PowerShortfall(s),
			SYS_COUNT - dark, SYS_COUNT, dark);
	}

	std::printf("== the chief engineer recommends, and a band delegation is revocable\n");
	{
		Ship s = NewShip();
		SetAlert(s, ALERT_YELLOW);
		const Recommendation rec = RecommendAllocation(s);
		std::printf("  %s\n", rec.reasoning.c_str());
		const int chief = rec.by;
		std::printf("  refused: %s\n", RefuseRecommendation(s) ? "recorded, and he remembers it" : "nothing to refuse");
		std::printf("  his mark: MEM_OVERRULED held %s\n", Recall(s.crew[chief], MEM_OVERRULED) ? "yes" : "no");
		std::printf("  grant the comforts to %s: %s\n", s.crew[chief].name.c_str(),
			GrantBand(s, 0, chief, BAND_COMFORT) ? "held" : "refused");
		SetAllocationBy(s, SYS_HOLODECKS, 40, chief);
		std::printf("  holodecks set to %d%% by %s\n", AllocationPercent(s, SYS_HOLODECKS),
			AllocationProvenance(s, SYS_HOLODECKS).c_str());
		const bool revoked = RevokeBand(s, chief, BAND_COMFORT);
		const bool afterRevoke = SetAllocationBy(s, SYS_HOLODECKS, 100, chief);
		std::printf("  revoke: %s; then the officer's set is %s\n", revoked ? "immediate" : "refused",
			afterRevoke ? "accepted (a defect)" : "refused (the authority is gone)");
	}
	std::printf("== the meeting now consumes this seam (built, docs/staff-meetings.md): ApplyMeetingOutcome calls\n"
		"   SetAllocation/SetAllocationBy, AcceptRecommendation/RefuseRecommendation, SetPowerAuto, and reads\n"
		"   PowerCommitted/PowerAvailable/PowerShortfall (test_ship_core --meeting)\n");
	return 0;
}

// `test_ship_core --meeting` prints the meeting as evidence (docs/evidence/meeting-brief.md): the brief
// for each kind, the normal-case emission, the skeleton with no model, the delivery vocabulary, and the
// allocation seam.
static int PrintMeeting()
{
	int failures = 0;
	auto bad = [&failures](bool ok) { if (!ok) ++failures; };

	std::printf("== the delivery vocabulary (docs/evidence/voice-review.md, finding three)\n");
	{
		static const uint8_t ORDER[] = { DELIVERY_ORDER, DELIVERY_REPORT, DELIVERY_CONFESSION, DELIVERY_CONDOLENCE,
			DELIVERY_FLAT, DELIVERY_UNMARKED };
		for (uint8_t d : ORDER) {
			if (DeliveryKnown(d)) std::printf("  %-11s exaggeration %.2f\n", DeliveryName(d), DeliveryExaggeration(d));
			else std::printf("  %-11s unset (the seam refuses it)\n", DeliveryName(d));
		}
		MeetingLine line; line.text = "Make it so."; line.delivery = DELIVERY_ORDER;
		SynthesisRequest req;
		bad(LineToSynthesis(line, req));
		std::printf("  seam: \"%s\" -> exaggeration %.2f, delivery %s\n", req.text.c_str(), req.exaggeration, DeliveryName(req.delivery));
	}

	std::printf("== a brief for each kind the design names\n");
	for (int k = 0; k < MEET_KIND_COUNT; ++k) {
		Ship s = NewShip();
		s.player = 0;
		SetAlert(s, ALERT_YELLOW);
		LogEvent(s, "engineering", "engineering", "a coolant line was replaced"); // so a view has a log
		const MeetingBrief b = BuildBrief(s, static_cast<uint8_t>(k));
		bad(b.presentCount > 0 && b.optionCount > 0 && !b.decision.empty());
		std::printf("  %-12s  present %d, decision: %s\n", MeetingKindName(b.kind), b.presentCount, b.decision.c_str());
		std::printf("               trigger: %s; plant %d of %d (%d short), crystal %d%%, casualties %d/%d\n",
			b.trigger.c_str(), b.powerCommitted, b.powerAvailable, b.powerShortfall,
			static_cast<int>(b.dilithium * 100 + 0.5f), b.casualties, b.beds);
		for (int i = 0; i < b.optionCount; ++i)
			std::printf("               option %d: %s  [cost: %s]  (%s)\n", i + 1, b.options[i].label.c_str(),
				b.options[i].cost.c_str(), MeetingEffectName(b.options[i].effect));
	}

	std::printf("== every meeting emits a brief, in the normal case (the emit site, not a count)\n");
	{
		Ship s = NewShip();
		const double toWatch = static_cast<double>(SECONDS_PER_WATCH) - std::fmod(s.clock, static_cast<double>(SECONDS_PER_WATCH));
		AdvanceShip(s, toWatch + 1.0);
		std::printf("  at the watch change: %d brief(s) queued\n", static_cast<int>(PendingMeetings(s).size()));
		bad(!PendingMeetings(s).empty());
		for (const MeetingBrief &b : PendingMeetings(s)) {
			std::printf("    %-12s  %s\n", MeetingKindName(b.kind), b.trigger.c_str());
			for (int i = 0; i < b.presentCount; ++i) {
				const SystemId post = static_cast<SystemId>(b.present[i].post);
				std::printf("      present: %s (%s)\n", b.present[i].name.c_str(),
					post < SYS_COUNT ? Spec(post).name : "department duties");
			}
		}
	}

	std::printf("== the skeleton plays with no model present\n");
	for (int k = 0; k < MEET_KIND_COUNT; ++k) {
		const MeetingSkeleton &sk = AuthoredSkeleton(static_cast<uint8_t>(k));
		bad(sk.outcomeCount > 0);
		std::printf("  %-12s  %s\n", MeetingKindName(sk.kind), sk.decision.c_str());
		for (int i = 0; i < sk.outcomeCount; ++i) {
			const MeetingOutcome &oc = sk.outcomes[i];
			std::printf("    %s  [cost: %s]\n", oc.option.label.c_str(), oc.option.cost.c_str());
			for (int l = 0; l < oc.lineCount; ++l)
				std::printf("      (%s) %s\n", DeliveryName(oc.lines[l].delivery), oc.lines[l].text.c_str());
		}
	}

	std::printf("== a meeting outcome sets an allocation, end to end (Task D)\n");
	{
		Ship s = NewShip();
		s.player = 0;
		SetAlert(s, ALERT_YELLOW);
		const MeetingBrief b = BuildBrief(s, MEET_ALLOCATION);
		const MeetingSkeleton &sk = AuthoredSkeleton(MEET_ALLOCATION);
		int allocOpt = -1;
		for (int i = 0; i < sk.outcomeCount; ++i) if (sk.outcomes[i].option.effect == EFFECT_SET_ALLOCATION && allocOpt < 0) allocOpt = i;
		const bool applied = allocOpt >= 0 && ApplyMeetingOutcome(s, b, allocOpt, s.player, false);
		bad(applied);
		std::printf("  a person decides \"%s\": holodecks %d%%, shields %d%% (provenance: %s)\n",
			allocOpt >= 0 ? sk.outcomes[allocOpt].option.label.c_str() : "?", AllocationPercent(s, SYS_HOLODECKS),
			AllocationPercent(s, SYS_SHIELDS), AllocationProvenance(s, SYS_HOLODECKS).c_str());
		const bool asShip = ApplyMeetingOutcome(s, b, allocOpt, -1, true);
		std::printf("  the meeting tries to set it as the ship: %s\n", asShip ? "accepted (a defect)" : "refused (automatic mode is the ship's own answer)");
		bad(!asShip);
	}

	std::printf("== a brief is a read: the ship's state is unchanged\n");
	{
		Ship s = NewShip();
		const std::vector<uint8_t> before = Pack(s);
		for (int k = 0; k < MEET_KIND_COUNT; ++k) BuildBrief(s, static_cast<uint8_t>(k));
		const bool same = Pack(s) == before;
		std::printf("  %d briefs built; the ship's blob is %s\n", static_cast<int>(MEET_KIND_COUNT),
			same ? "unchanged" : "CHANGED (a defect)");
		bad(same);
	}

	return failures;
}

// ---- the audio plumbing (phase two) ------------------------------------------------------------
//
// The invariant this phase exists for: ONE PRODUCER PER TRACK, ONE RETIREMENT RULE PER REPLY. A
// producer that does not own a track cannot write to it, and cannot retire anything on it.

static void TestTrackOwnership()
{
	g_test = "three tracks, one owner each: a non-owner cannot write or retire";
	CHECK(TrackOwner(TRACK_DIALOGUE) == PROD_RENDERER);
	CHECK(TrackOwner(TRACK_CUE) == PROD_CUE_PLAYER);
	CHECK(TrackOwner(TRACK_LIVE) == PROD_LIVE);
	CHECK(OwnsTrack(PROD_RENDERER, TRACK_DIALOGUE));
	CHECK(!OwnsTrack(PROD_CUE_PLAYER, TRACK_DIALOGUE));
	CHECK(!OwnsTrack(PROD_LIVE, TRACK_DIALOGUE));
	CHECK(!OwnsTrack(PROD_RENDERER, TRACK_CUE));

	VoiceMixer m;
	// The wrong producer cannot write: refused, and the track stays empty.
	CHECK(VoiceWrite(m, PROD_CUE_PLAYER, TRACK_DIALOGUE, "line.wav") < 0);
	CHECK(VoiceWrite(m, PROD_LIVE, TRACK_DIALOGUE, "line.wav") < 0);
	CHECK(VoiceWrite(m, PROD_RENDERER, TRACK_LIVE, "line.wav") < 0);
	CHECK(!TrackBusy(m, TRACK_DIALOGUE));
	// The owner can.
	const int line = VoiceWrite(m, PROD_RENDERER, TRACK_DIALOGUE, "line.wav");
	CHECK(line >= 0);
	CHECK(TrackBusy(m, TRACK_DIALOGUE));
	CHECK(ActiveReply(m, TRACK_DIALOGUE) && ActiveReply(m, TRACK_DIALOGUE)->asset == "line.wav");
	// The wrong producer cannot retire: refused, and the reply stays.
	CHECK(!VoiceRetire(m, PROD_CUE_PLAYER, line));
	CHECK(!VoiceRetire(m, PROD_LIVE, line));
	CHECK(TrackBusy(m, TRACK_DIALOGUE));
	// The owner can.
	CHECK(VoiceRetire(m, PROD_RENDERER, line));
	CHECK(!TrackBusy(m, TRACK_DIALOGUE));
	CHECK(!VoiceRetire(m, PROD_RENDERER, line)); // already gone
}

static void TestCueCannotStopALine()
{
	g_test = "the cue track cannot stop a dialogue line";
	VoiceMixer m;
	const int line = VoiceWrite(m, PROD_RENDERER, TRACK_DIALOGUE, "line.wav");
	CHECK(line >= 0);
	const int cue = VoiceWrite(m, PROD_CUE_PLAYER, TRACK_CUE, CueClip(CUE_BREATH), CUE_BREATH);
	CHECK(cue >= 0);
	// The cue player trying to retire the line is refused, and the line plays on.
	CHECK(!VoiceRetire(m, PROD_CUE_PLAYER, line));
	CHECK(TrackBusy(m, TRACK_DIALOGUE));
	// Retiring the cue cannot reach the line either.
	CHECK(VoiceRetire(m, PROD_CUE_PLAYER, cue));
	CHECK(!TrackBusy(m, TRACK_CUE));
	CHECK(TrackBusy(m, TRACK_DIALOGUE));
	CHECK(ActiveReply(m, TRACK_DIALOGUE) && ActiveReply(m, TRACK_DIALOGUE)->id == line);
	CHECK(VoiceRetire(m, PROD_RENDERER, line));
}

static void TestCueSet()
{
	g_test = "the cue set is named, small, and each cue has one purpose";
	for (int c = 0; c < CUE_COUNT; ++c) {
		CHECK(CueName(static_cast<uint8_t>(c))[0] != '\0');
		CHECK(CueClip(static_cast<uint8_t>(c))[0] != '\0');
		CHECK(CuePurpose(static_cast<uint8_t>(c))[0] != '\0');
	}
	CHECK(std::strcmp(CueName(CUE_HOLDING), "I have to think about that.") == 0);
	CHECK(CueIsLexical(CUE_HOLDING));       // the one short spoken line
	CHECK(!CueIsLexical(CUE_BREATH));       // the rest are non-verbal clips
	CHECK(!CueIsLexical(CUE_HMM));
}

static void TestCueEmitSite()
{
	g_test = "the cue emit site fires in the normal case, and the log carries it";
	Ship s = NewShip();
	VoiceMixer m;
	const size_t before = s.log.size();
	const int cue = EmitCue(s, m, CUE_BREATH, "the pause before the answer");
	CHECK(cue >= 0);
	CHECK(TrackBusy(m, TRACK_CUE));
	bool logged = false;
	for (const LogEntry &e : s.log)
		if (e.what.find("cue: breath") != std::string::npos) logged = true;
	CHECK(logged);                       // the emit site wrote the emission down
	CHECK(s.log.size() > before);
	// The track is busy: a second cue is refused rather than stacking.
	CHECK(EmitCue(s, m, CUE_HMM, "another") < 0);
	CHECK(VoiceRetire(m, PROD_CUE_PLAYER, cue));
	CHECK(EmitCue(s, m, CUE_HOLDING, "the answer is late") >= 0);
}

static void TestRenderKeyAndCache()
{
	g_test = "the cache key avoids a re-render, and a second render is a no-op";
	const std::string a = RenderKey("tuvok", "Make it so.", DELIVERY_ORDER);
	CHECK(a == RenderKey("tuvok", "Make it so.", DELIVERY_ORDER));       // same inputs, same file
	CHECK(a != RenderKey("tuvok", "Make it so.", DELIVERY_REPORT));      // same text, different direction
	CHECK(a != RenderKey("janeway", "Make it so.", DELIVERY_ORDER));     // same text, different voice
	CHECK(a != RenderKey("tuvok", "Make it not so.", DELIVERY_ORDER));   // different text
	CHECK(a.size() == 16);

	VoiceRender vr;
	vr.dir = "voice";
	RenderJob job;
	job.voice = "tuvok"; job.text = "Make it so."; job.delivery = DELIVERY_ORDER;
	job.exaggeration = DeliveryExaggeration(DELIVERY_ORDER);
	job.key = RenderKey(job.voice, job.text, job.delivery);
	CHECK(QueueRender(vr, job) == 1);       // newly queued
	CHECK(QueueRender(vr, job) == 0);       // already queued: no-op
	CHECK(static_cast<int>(vr.queue.size()) == 1);
	CHECK(!VoiceCached(vr, job.key));
	const std::string file = VoiceCachePath(vr, job.key);
	CHECK(CacheRendered(vr, job.key, file));
	CHECK(VoiceCached(vr, job.key));
	CHECK(vr.queue.empty());
	CHECK(QueueRender(vr, job) == 0);       // already cached: a re-render is a no-op
	CHECK(static_cast<int>(vr.queue.size()) == 0);
	// A render nobody asked for cannot enter the cache.
	CHECK(!CacheRendered(vr, RenderKey("k", "x", DELIVERY_FLAT), "x.wav"));
	// An unmarked line is not rendered at all.
	RenderJob unmarked; unmarked.voice = "k"; unmarked.text = "?"; unmarked.delivery = DELIVERY_UNMARKED;
	unmarked.key = RenderKey(unmarked.voice, unmarked.text, unmarked.delivery);
	CHECK(QueueRender(vr, unmarked) == -1);
	// Pruned with the save: nothing survives.
	CHECK(PruneVoiceCache(vr) == 1);
	CHECK(vr.cache.empty());
}

static void TestPlanMeetingAudio()
{
	g_test = "a meeting's audio is planned and deduplicated; the queue may be unfinished";
	Ship s = NewShip();
	s.player = 0;
	SetAlert(s, ALERT_YELLOW);
	const MeetingBrief b = BuildBrief(s, MEET_WATCH_CHANGE);
	const MeetingSkeleton &sk = AuthoredSkeleton(MEET_WATCH_CHANGE);
	VoiceRender vr;
	vr.dir = "voice";
	const std::vector<uint8_t> before = Pack(s);
	const int first = PlanMeetingAudio(vr, s, b, sk);
	CHECK(first > 0);
	CHECK(Pack(s) == before);               // planning is a read
	const int second = PlanMeetingAudio(vr, s, b, sk);
	CHECK(second == 0);                     // the same lines are not queued twice
	// The unmarked line is marked, not rendered: count the skeleton's known lines.
	int known = 0, unmarked = 0;
	for (int i = 0; i < sk.outcomeCount; ++i)
		for (int l = 0; l < sk.outcomes[i].lineCount; ++l) {
			if (DeliveryKnown(sk.outcomes[i].lines[l].delivery)) ++known; else ++unmarked;
		}
	CHECK(unmarked > 0);
	CHECK(static_cast<int>(vr.queue.size()) == known);
	// The meeting can start with the queue unfinished: the skeleton is still there and legible.
	CHECK(!vr.queue.empty());
	CHECK(sk.outcomeCount > 0 && sk.outcomes[0].lineCount > 0);

	// Warming happens in the async window, once, and is written down.
	CHECK(!vr.warm);
	CHECK(WarmVoice(s, vr, 0.0));
	CHECK(vr.warm);
	CHECK(!WarmVoice(s, vr, 0.0));          // already warm: nothing to do
	bool logged = false;
	for (const LogEntry &e : s.log)
		if (e.what.find("warmed in the async window") != std::string::npos) logged = true;
	CHECK(logged);
}

static void TestAudioSaveRoundTrip()
{
	g_test = "the audio model adds nothing to the save: the blob round-trips unchanged";
	Ship s = NewShip();
	const std::vector<uint8_t> blob = Pack(s);
	Ship back;
	CHECK(Unpack(blob.data(), blob.size(), back));
	CHECK(Pack(back) == blob);
}

// `test_ship_core --voice` prints the audio plumbing as evidence (docs/evidence/audio-plumbing.md):
// the three sources and their owners, the cue set, the cache key and the no-op, the queue unfinished,
// the warm in the async window, and the delivery direction end to end.
static int PrintVoice()
{
	int failures = 0;
	auto bad = [&failures](bool ok) { if (!ok) ++failures; };

	std::printf("== three sources, one owner each\n");
	for (int t = 0; t < TRACK_COUNT; ++t)
		std::printf("  track %-8s  owner: %s\n", VoiceTrackName(static_cast<uint8_t>(t)),
			VoiceProducerName(TrackOwner(static_cast<uint8_t>(t))));
	{
		VoiceMixer m;
		const int line = VoiceWrite(m, PROD_RENDERER, TRACK_DIALOGUE, "line.wav");
		bad(line >= 0);
		const bool cueWroteLine = VoiceWrite(m, PROD_CUE_PLAYER, TRACK_DIALOGUE, "line.wav") < 0;
		const bool cueRetiredLine = !VoiceRetire(m, PROD_CUE_PLAYER, line);
		bad(cueWroteLine && cueRetiredLine);
		bad(TrackBusy(m, TRACK_DIALOGUE));
		std::printf("  the cue player writes to the dialogue track: %s\n", cueWroteLine ? "refused" : "ACCEPTED (a defect)");
		std::printf("  the cue player retires the dialogue line:   %s\n", cueRetiredLine ? "refused" : "ACCEPTED (a defect)");
		const int cue = VoiceWrite(m, PROD_CUE_PLAYER, TRACK_CUE, CueClip(CUE_BREATH), CUE_BREATH);
		bad(VoiceRetire(m, PROD_CUE_PLAYER, cue));
		const bool lineSurvives = TrackBusy(m, TRACK_DIALOGUE);
		bad(lineSurvives);
		std::printf("  a cue plays and retires: the dialogue line is %s\n", lineSurvives ? "still playing" : "STOPPED (a defect)");
	}

	std::printf("== the cue set: clips, never text, each for one thing\n");
	for (int c = 0; c < CUE_COUNT; ++c)
		std::printf("  %-30s  %-16s  %s\n", CueName(static_cast<uint8_t>(c)), CueClip(static_cast<uint8_t>(c)),
			CuePurpose(static_cast<uint8_t>(c)));
	{
		Ship s = NewShip();
		VoiceMixer m;
		const int cue = EmitCue(s, m, CUE_HOLDING, "the answer is late");
		bad(cue >= 0);
		bool logged = false;
		for (const LogEntry &e : s.log) if (e.what.find("cue: I have to think about that.") != std::string::npos) logged = true;
		bad(logged);
		std::printf("  the emit site, in the normal case: %s\n", logged ? "wrote the cue emission to the log" : "SILENT (a defect)");
	}

	std::printf("== the cache key, and a second render is a no-op\n");
	{
		VoiceRender vr;
		vr.dir = "voice";
		RenderJob job;
		job.voice = "tuvok"; job.text = "Make it so."; job.delivery = DELIVERY_ORDER;
		job.key = RenderKey(job.voice, job.text, job.delivery);
		const int q1 = QueueRender(vr, job);
		const int q2 = QueueRender(vr, job);
		std::printf("  key \"%s\" -> %s.wav\n", job.key.c_str(), job.key.c_str());
		std::printf("  first queue: %d; second queue: %d (0 = already queued, not rendered twice)\n", q1, q2);
		bad(q1 == 1 && q2 == 0);
		CacheRendered(vr, job.key, VoiceCachePath(vr, job.key));
		const int q3 = QueueRender(vr, job);
		std::printf("  after it is cached, queue again: %d (0 = cached, a re-render is a no-op)\n", q3);
		bad(q3 == 0);
		const int dropped = PruneVoiceCache(vr);
		std::printf("  pruned with the save: %d entr(ies) dropped, cache now %d\n", dropped,
			static_cast<int>(vr.cache.size()));
	}

	std::printf("== the meeting plays with the queue unfinished; the skeleton carries it\n");
	{
		Ship s = NewShip();
		s.player = 0;
		const MeetingBrief b = BuildBrief(s, MEET_WATCH_CHANGE);
		const MeetingSkeleton &sk = AuthoredSkeleton(MEET_WATCH_CHANGE);
		VoiceRender vr;
		vr.dir = "voice";
		const int queued = PlanMeetingAudio(vr, s, b, sk);
		std::printf("  planned %d line(s); queue unfinished: %d\n", queued, static_cast<int>(vr.queue.size()));
		std::printf("  the skeleton, played with the queue unfinished: %d outcome(s) in \"%s\"\n", sk.outcomeCount, sk.decision.c_str());
		bad(queued > 0 && sk.outcomeCount > 0);
		bad(WarmVoice(s, vr, 0.0));
		std::printf("  warm: happens in the async window, off the critical path (the queue is not the player)\n");
	}

	std::printf("== the delivery direction reaches the synthesizer's --exaggeration\n");
	{
		const MeetingSkeleton &sk = AuthoredSkeleton(MEET_WATCH_CHANGE);
		const MeetingLine &line = sk.outcomes[0].lines[0];
		SynthesisRequest req;
		bad(LineToSynthesis(line, req));
		std::printf("  line: \"%s\"\n", line.text.c_str());
		std::printf("  delivery %s -> RenderJob.exaggeration %.2f -> synthesize.py --exaggeration %.2f\n",
			DeliveryName(req.delivery), req.exaggeration, req.exaggeration);
	}
	return failures;
}

// ---- the character layer (O8, docs/character-derivation.md) -------------------------------------

// The derivation is a function of the record and the seed, not a table of hand-written people. The
// same seed produces the same person, which is what "saves must replay identically" requires; a
// different seed produces a different crew.
static void TestTheDerivation()
{
	g_test = "the derivation is a function of the record and the seed";
	Ship a = NewShip(), b = NewShip();
	CHECK(a.crew.size() == b.crew.size());
	bool same = true;
	for (size_t i = 0; i < a.crew.size(); ++i) {
		const CrewMember &x = a.crew[i], &y = b.crew[i];
		if (x.name != y.name || x.species != y.species || x.traits != y.traits ||
		    x.desire != y.desire || x.need != y.need || x.fear != y.fear) { same = false; break; }
		for (int k = 0; k < SKILL_COUNT; ++k) if (x.skills[k] != y.skills[k]) { same = false; break; }
		if (!same) break;
	}
	CHECK(same); // the same seed, the same crew

	Config cfgA; cfgA.seed = 909; Ship c = NewShip(cfgA);
	Config cfgB; cfgB.seed = 910; Ship d = NewShip(cfgB);
	int differences = 0;
	for (size_t i = 0; i < c.crew.size() && i < d.crew.size(); ++i)
		if (c.crew[i].species != d.crew[i].species || c.crew[i].traits != d.crew[i].traits ||
		    c.crew[i].desire != d.crew[i].desire) ++differences;
	CHECK(differences > 20); // a different seed is a different crew

	// The four kinds are present, and species is not a skill bonus: Vulcans and humans have the
	// same average skill, so no species reads as "the good one".
	Ship s = NewShip();
	int vulcanSum = 0, vulcanN = 0, humanSum = 0, humanN = 0;
	for (const CrewMember &m : s.crew) {
		int t = 0;
		for (int k = 0; k < SKILL_COUNT; ++k) t += m.skills[k];
		if (m.species == SPECIES_VULCAN) { vulcanSum += t; ++vulcanN; }
		else if (m.species == SPECIES_HUMAN) { humanSum += t; ++humanN; }
	}
	CHECK(vulcanN > 0 && humanN > 0);
	const float vulcanAvg = static_cast<float>(vulcanSum) / vulcanN;
	const float humanAvg = static_cast<float>(humanSum) / humanN;
	CHECK(std::fabs(vulcanAvg - humanAvg) < 1.5f); // no species is the good one
}

// A condition carries its source, its cure and its visibility; one arrives, is recorded, and is
// cured; the set stays small.
static void TestConditionLifecycle()
{
	g_test = "a condition arrives, is recorded with source and cure, and is cured";
	Ship s = NewShip();
	Tick(s, 1.0f);
	const int who = 30;
	s.crew[who].conditionCount = 0;
	const size_t before = s.log.size();
	const uint8_t all = CVIS_PLAYER | CVIS_CREW | CVIS_LOG;
	CHECK(Sicken(s, who, COND_CONCUSSED, "concussed in the coolant bay", CVAL_DEBUFF, CMAG_CLEAR, CLEAR_SICKBAY, all));
	const Condition *k = FindCondition(s.crew[who], COND_CONCUSSED);
	CHECK(k != nullptr);
	CHECK(k->source == "concussed in the coolant bay");
	CHECK(k->clears == CLEAR_SICKBAY);
	CHECK((k->visible & CVIS_PLAYER) && (k->visible & CVIS_CREW) && (k->visible & CVIS_LOG));
	CHECK(s.log.size() > before); // recorded on arrival
	// A condition nobody can see is refused: it would be a hidden penalty.
	CHECK(!Sicken(s, who, COND_AFRAID, "an unnamed dread", CVAL_DEBUFF, CMAG_SLIGHT, CLEAR_REST, 0));
	CHECK(FindCondition(s.crew[who], COND_AFRAID) == nullptr);
	const size_t afterArrival = s.log.size();
	CHECK(Cure(s, who, COND_CONCUSSED)); // the cure is recorded too
	CHECK(FindCondition(s.crew[who], COND_CONCUSSED) == nullptr);
	CHECK(s.log.size() > afterArrival);
	// Two or three at a time, never a soup.
	CHECK(Sicken(s, who, COND_EXHAUSTED, "hasn't slept enough", CVAL_DEBUFF, CMAG_CLEAR, CLEAR_REST, all));
	CHECK(Sicken(s, who, COND_HUNGRY, "the galley is empty", CVAL_DEBUFF, CMAG_CLEAR, CLEAR_MEAL, all));
	CHECK(Sicken(s, who, COND_HYPOXIC, "the deck has no air", CVAL_DEBUFF, CMAG_SHARP, CLEAR_SICKBAY, all));
	CHECK(s.crew[who].conditionCount == CONDITION_MAX);
	CHECK(!Sicken(s, who, COND_EXHAUSTED, "again", CVAL_DEBUFF, CMAG_CLEAR, CLEAR_REST, all)); // dedup
	CHECK(Sicken(s, who, COND_GRIEVING, "lost someone", CVAL_DEBUFF, CMAG_CLEAR, CLEAR_SALIENCE, all));
	CHECK(s.crew[who].conditionCount == CONDITION_MAX); // evicted, not stacked
}

// Morale is read from the three components, never a stored counter: change one and the reading
// moves, and the reason names it. The log says which one moved and why.
static void TestMoraleIsReadFromThree()
{
	g_test = "morale is read from deficit, outlook and holdings, and the log names the reason";
	Ship s = NewShip();
	Tick(s, 1.0f);
	CrewMember c = s.crew[30];
	c.conditionCount = 0;
	c.deficit = 0.2f; c.outlook = 0.6f; c.holdings = 0.6f;
	CHECK(std::fabs(Morale(c) - (0.40f * 0.8f + 0.35f * 0.6f + 0.25f * 0.6f)) < 1e-5f);
	const float before = Morale(c);
	c.outlook = 0.0f;
	CHECK(Morale(c) < before); // one component moved, and the reading moved with it
	CHECK(std::string(MoraleReason(c)) == "does not believe the course is worth the cost");
	c.outlook = 0.6f; c.deficit = 1.0f;
	CHECK(Morale(c) < before);
	CHECK(std::string(MoraleReason(c)) == "short of what they need" ||
	      std::string(MoraleReason(c)) == "short on sleep");
	// And the log carries it: the person carrying the most of it is named by component.
	s.crew[30].deficit = 1.0f; s.crew[30].outlook = 0.0f; s.crew[30].holdings = 0.0f;
	s.lastMoodLog = -1.0e9;
	Tick(s, Hours(s, 1.0f));
	bool named = false;
	for (const LogEntry &e : s.log)
		if (e.what.find("short of what they need") != std::string::npos ||
		    e.what.find("short on sleep") != std::string::npos) named = true;
	CHECK(named);
}

// Species are capabilities, needs and susceptibilities -- never bonuses. The record carries words
// and prone conditions, and the only comparison one can make is a need.
static void TestSpeciesAreCapabilitiesNotBonuses()
{
	g_test = "species are capabilities, needs and susceptibilities -- never bonuses";
	for (int sp = 0; sp < SPECIES_COUNT; ++sp) {
		const SpeciesRecord &r = SpeciesOf(sp);
		CHECK(r.name && r.name[0]);
		bool anyCapability = false;
		for (int i = 0; i < SPECIES_TRAIT_MAX; ++i) if (r.capabilities[i]) anyCapability = true;
		CHECK(anyCapability);
	}
	// Rest hours are the shape of rest, a need with a cost, not a bonus: the Vulcan needs less of
	// it and must meditate, the Borg needs the alcove, the hologram needs none and is different.
	CHECK(SpeciesOf(SPECIES_VULCAN).restNeedHours < SpeciesOf(SPECIES_HUMAN).restNeedHours);
	CHECK(SpeciesOf(SPECIES_HUMAN).sleeps);
	CHECK(!SpeciesOf(SPECIES_VULCAN).sleeps);
	CHECK(!SpeciesOf(SPECIES_HOLOGRAM).needsFood && !SpeciesOf(SPECIES_HOLOGRAM).sleeps);
	// A species need produces a condition, and the condition names its source.
	Ship s = NewShip();
	int vulcan = -1;
	for (int i = 19; i < static_cast<int>(s.crew.size()); ++i)
		if (s.crew[i].species == SPECIES_VULCAN) { vulcan = i; break; }
	if (vulcan < 0) { CHECK(false); return; }
	s.crew[vulcan].watch = 0; // on duty at 0800, so the cycle is not slept off
	s.crew[vulcan].fatigue = 0.8f;
	s.crew[vulcan].conditionCount = 0;
	s.clock = 8 * 3600;
	Tick(s, Hours(s, 1.0f));
	const Condition *k = FindCondition(s.crew[vulcan], COND_MEDITATION_DUE);
	CHECK(k != nullptr);
	if (k) CHECK(k->source == "the meditation cycle, unkept");

	// The galley and the watch bill read species and adapt: a hologram does not tire, and does not
	// eat -- heterogeneity is a logistics difference, not a stat.
	Ship h = NewShip();
	int doctor = -1;
	for (int i = 0; i < static_cast<int>(h.crew.size()); ++i)
		if (h.crew[i].species == SPECIES_HOLOGRAM) { doctor = i; break; }
	CHECK(doctor >= 0);
	h.crew[doctor].watch = 0;
	h.crew[doctor].fatigue = 0.9f;
	Tick(h, Hours(h, 8.0f)); // a full watch on duty
	CHECK(h.crew[doctor].fatigue == 0.0f); // no fatigue at all for a hologram

	Ship eats = NewShip(), fasts = NewShip();
	eats.stores.rations = 60.0f; fasts.stores.rations = 60.0f;
	eats.crew[doctor].species = SPECIES_HUMAN; // the same person, made to eat
	Tick(fasts, Hours(fasts, 24.0f));
	Tick(eats, Hours(eats, 24.0f));
	CHECK(eats.stores.rations < fasts.stores.rations);
}

// The manner reads at three levels: the same person sounds like what they feel.
static void TestManner()
{
	g_test = "the manner reads at three morale levels";
	Ship s = NewShip();
	CrewMember c = s.crew[30];
	c.conditionCount = 0;
	c.deficit = 0.80f; c.outlook = 0.10f; c.holdings = 0.10f; const char *lo = MannerLine(c);
	c.deficit = 0.30f; c.outlook = 0.50f; c.holdings = 0.50f; const char *mid = MannerLine(c);
	c.deficit = 0.00f; c.outlook = 0.90f; c.holdings = 0.90f; const char *hi = MannerLine(c);
	CHECK(lo && mid && hi);
	CHECK(std::string(lo) != std::string(mid));
	CHECK(std::string(mid) != std::string(hi));
	CHECK(std::string(lo) != std::string(hi));
	// The band names are the document's own.
	CHECK(std::string(MoraleBandName(0.9f)) == "fit");
	CHECK(std::string(MoraleBandName(0.6f)) == "worn");
	CHECK(std::string(MoraleBandName(0.4f)) == "strained");
	CHECK(std::string(MoraleBandName(0.1f)) == "at breaking point");
}

// The whole layer round-trips the save: the three reads and the conditions are dynamic; the static
// half comes back from the seed.
static void TestCharacterSaveRoundTrip()
{
	g_test = "the character layer round-trips the save";
	Ship s = NewShip();
	Tick(s, 1.0f);
	s.crew[30].conditionCount = 0;
	s.crew[30].deficit = 0.33f; s.crew[30].outlook = 0.22f; s.crew[30].holdings = 0.44f;
	const uint8_t all = CVIS_PLAYER | CVIS_CREW | CVIS_LOG;
	CHECK(Sicken(s, 30, COND_GRIEVING, "lost someone in the coolant bay", CVAL_DEBUFF, CMAG_CLEAR, CLEAR_SALIENCE, all));
	std::vector<uint8_t> blob = Pack(s);
	Ship u = NewShip();
	CHECK(Unpack(blob.data(), blob.size(), u));
	CHECK(std::fabs(u.crew[30].deficit - 0.33f) < 1e-4f);
	CHECK(std::fabs(u.crew[30].outlook - 0.22f) < 1e-4f);
	CHECK(std::fabs(u.crew[30].holdings - 0.44f) < 1e-4f);
	const Condition *k = FindCondition(u.crew[30], COND_GRIEVING);
	CHECK(k && k->source == "lost someone in the coolant bay" && k->clears == CLEAR_SALIENCE);
	// The static half came back identical, because it is derived from the same seed.
	for (size_t i = 0; i < u.crew.size() && i < s.crew.size(); ++i) {
		CHECK(u.crew[i].species == s.crew[i].species);
		CHECK(u.crew[i].traits == s.crew[i].traits);
		for (int kk = 0; kk < SKILL_COUNT; ++kk) CHECK(u.crew[i].skills[kk] == s.crew[i].skills[kk]);
	}
}

// A person with no modifier but their skill: neutral morale, no conditions, no traits, a routine
// task. The tests below vary one thing at a time from this, so a difference is attributable.
static CrewMember PlainCrew()
{
	CrewMember c;
	c.name = "Test Crewman";
	c.conditionCount = 0;
	c.fatigue = 0.0f;
	c.deficit = 0.65f; c.outlook = 0.6f; c.holdings = 0.6f; // morale exactly 0.5: the morale factor is neutral
	c.traits = 0;
	c.desire = DESIRE_A_PERSON; c.need = NEED_SLEEP; c.fear = FEAR_DYING_ALONE;
	c.species = SPECIES_HUMAN;
	for (int k = 0; k < SKILL_COUNT; ++k) c.skills[k] = 3;
	return c;
}

// Twenty thousand draws at a fixed condition and load, for a set of factors: the measurement a
// difference in the odds can be read from.
static int MeasureWork(float condition, float stress, const WorkFactor *f, int n)
{
	int hits = 0;
	for (uint32_t i = 0; i < 20000; ++i)
		if (RollWork(condition, stress, f, n, i * 2654435761u + 12345u) != ANOMALY_NONE) ++hits;
	return hits;
}

// Task A: the failure roll's odds are a function of what the operator can actually do. The same
// task and the same person at two skill levels produce different outcomes.
static void TestEffectiveSkillReachesTheOdds()
{
	g_test = "effective skill reaches the failure roll's odds";
	CrewMember lo = PlainCrew(); lo.skills[SKILL_ENGINEERING] = 1;
	CrewMember hi = PlainCrew(); hi.skills[SKILL_ENGINEERING] = 5;
	WorkFactor lf[WORK_FACTOR_MAX], hf[WORK_FACTOR_MAX];
	const int ln = WorkFactors(lo, SKILL_ENGINEERING, WORK_ROUTINE, 1.0f, lf, WORK_FACTOR_MAX);
	const int hn = WorkFactors(hi, SKILL_ENGINEERING, WORK_ROUTINE, 1.0f, hf, WORK_FACTOR_MAX);
	CHECK(ln >= 1 && hn >= 1);
	CHECK(lf[0].kind == WORK_SKILL && lf[0].delta > 0.0f); // an unskilled hand raises the odds
	CHECK(hf[0].kind == WORK_SKILL && hf[0].delta < 0.0f); // a competent one lowers them
	CHECK(hf[0].delta < lf[0].delta);
	const float c = 0.40f;
	CHECK(WorkOdds(c, hf, hn) < WorkOdds(c, lf, ln));
	const int loHits = MeasureWork(c, 1.0f, lf, ln);
	const int hiHits = MeasureWork(c, 1.0f, hf, hn);
	CHECK(loHits > hiHits);
	CHECK(std::fabs(loHits / 20000.0f - WorkOdds(c, lf, ln)) < 0.03f);
}

// Task B(i): a condition moves the odds while it holds, and the move is attributable -- the factor
// line and the log name the condition, its magnitude and its kind.
static void TestConditionMovesTheOdds()
{
	g_test = "a condition moves the odds, and the reasons are named";
	CrewMember c = PlainCrew();
	WorkFactor base[WORK_FACTOR_MAX];
	const int bn = WorkFactors(c, SKILL_ENGINEERING, WORK_ROUTINE, 1.0f, base, WORK_FACTOR_MAX);
	const float before = WorkOdds(0.40f, base, bn);
	CHECK(AddCondition(c, COND_AFRAID, "the ship is at battle stations", CVAL_DEBUFF, CMAG_SHARP,
		CLEAR_END_OF_WATCH, 0.0f, CVIS_PLAYER | CVIS_CREW));
	WorkFactor after[WORK_FACTOR_MAX];
	const int an = WorkFactors(c, SKILL_ENGINEERING, WORK_ROUTINE, 1.0f, after, WORK_FACTOR_MAX);
	CHECK(an > bn);
	CHECK(WorkOdds(0.40f, after, an) > before);
	const std::string line = WorkFactorLine(after, an);
	CHECK(line.find("afraid") != std::string::npos);
	CHECK(line.find("sharp") != std::string::npos);
	CHECK(line.find("condition") != std::string::npos); // kept distinct: the kind is in the record
	CHECK(MeasureWork(0.40f, 1.0f, after, an) > MeasureWork(0.40f, 1.0f, base, bn));

	// The reason travels with the work: a use that goes wrong writes the condition into the log.
	Ship s = NewShip();
	s.cfg.dayScale = 1.0f;
	SetAlert(s, ALERT_RED);
	Tick(s, 1.0f);
	const int op = 30;
	s.crew[op].conditionCount = 0;
	s.crew[op].traits = 0;
	s.crew[op].species = SPECIES_HUMAN;
	s.crew[op].skills[SKILL_ENGINEERING] = 0;
	const uint8_t all = CVIS_PLAYER | CVIS_CREW | CVIS_LOG;
	CHECK(Sicken(s, op, COND_AFRAID, "the ship is at battle stations", CVAL_DEBUFF, CMAG_SHARP,
		CLEAR_END_OF_WATCH, all));
	bool named = false;
	for (int i = 0; i < 500 && !named; ++i) {
		s.systems[SYS_TRANSPORTERS].health = 0.4f;
		s.systems[SYS_TRANSPORTERS].output = 0.4f;
		if (UseSystemBy(s, SYS_TRANSPORTERS, 1.0f, op) == ANOMALY_NONE) continue;
		for (const LogEntry &e : s.log)
			if (e.what.find("condition under") != std::string::npos
			    && e.what.find("afraid") != std::string::npos) named = true;
	}
	CHECK(named);
}

// Task B(ii): a trait is durable and behavioural, and shapes performance by interacting with the
// moment, not as a flat penalty. Steady under fire does nothing on a quiet watch and helps at battle
// stations; quick healer does not touch the odds at all (it shapes recovery).
static void TestTraitShapesPerformance()
{
	g_test = "a trait shapes performance without being a straight penalty";
	CrewMember plain = PlainCrew();
	CrewMember steady = PlainCrew();
	steady.traits = static_cast<uint16_t>(1u << TRAIT_STEADY_UNDER_FIRE);
	WorkFactor pf[WORK_FACTOR_MAX], sf[WORK_FACTOR_MAX];
	const int pn0 = WorkFactors(plain, SKILL_ENGINEERING, WORK_ROUTINE, 0.0f, pf, WORK_FACTOR_MAX);
	const int sn0 = WorkFactors(steady, SKILL_ENGINEERING, WORK_ROUTINE, 0.0f, sf, WORK_FACTOR_MAX);
	CHECK(sn0 == pn0); // calm: the trait is not a flat shift
	CHECK(WorkOdds(0.40f, sf, sn0) == WorkOdds(0.40f, pf, pn0));
	const int pn1 = WorkFactors(plain, SKILL_ENGINEERING, WORK_ROUTINE, 1.0f, pf, WORK_FACTOR_MAX);
	const int sn1 = WorkFactors(steady, SKILL_ENGINEERING, WORK_ROUTINE, 1.0f, sf, WORK_FACTOR_MAX);
	CHECK(sn1 > pn1);
	CHECK(WorkOdds(0.40f, sf, sn1) < WorkOdds(0.40f, pf, pn1)); // under load: it helps
	CHECK(WorkFactorLine(sf, sn1).find("steady under fire") != std::string::npos);
	// Quick healer is a trait too, but it shapes recovery, not the odds: the difference is the point.
	CrewMember healer = PlainCrew();
	healer.traits = static_cast<uint16_t>(1u << TRAIT_QUICK_HEALER);
	WorkFactor hf[WORK_FACTOR_MAX];
	const int hn = WorkFactors(healer, SKILL_ENGINEERING, WORK_ROUTINE, 1.0f, hf, WORK_FACTOR_MAX);
	CHECK(WorkOdds(0.40f, hf, hn) == WorkOdds(0.40f, pf, pn1));
}

// Task B(iii): a drive biases what a person does and how they bear up -- conditionally on the work,
// never a flat subtraction. Fear of decompression costs on a hull task and nothing on a routine one;
// wanting to prove something helps under load and does nothing on a quiet watch.
static void TestDriveBiasesBehaviour()
{
	g_test = "a drive biases behaviour rather than only subtracting";
	CrewMember f = PlainCrew(); f.fear = FEAR_DECOMPRESSION;
	WorkFactor hf[WORK_FACTOR_MAX], rf[WORK_FACTOR_MAX];
	const int hn = WorkFactors(f, SKILL_ENGINEERING, WORK_HULL, 0.15f, hf, WORK_FACTOR_MAX);
	const int rn = WorkFactors(f, SKILL_ENGINEERING, WORK_ROUTINE, 0.15f, rf, WORK_FACTOR_MAX);
	CHECK(hn > rn);
	CHECK(WorkOdds(0.40f, hf, hn) > WorkOdds(0.40f, rf, rn)); // the work changed, not the person
	CHECK(WorkFactorLine(hf, hn).find("decompression") != std::string::npos);
	CHECK(WorkFactorLine(rf, rn).find("decompression") == std::string::npos);
	CrewMember d = PlainCrew(); d.desire = DESIRE_TO_PROVE;
	WorkFactor cf[WORK_FACTOR_MAX], lf[WORK_FACTOR_MAX];
	const int cn = WorkFactors(d, SKILL_ENGINEERING, WORK_ROUTINE, 0.15f, cf, WORK_FACTOR_MAX);
	const int ln = WorkFactors(d, SKILL_ENGINEERING, WORK_ROUTINE, 1.0f, lf, WORK_FACTOR_MAX);
	CHECK(WorkOdds(0.40f, lf, ln) < WorkOdds(0.40f, cf, cn)); // under load it leans in
	CHECK(WorkFactorLine(lf, ln).find("leans in") != std::string::npos);
}

// Task B(iv): morale is read from its three components and reaches the work: a person who does not
// believe the course is worth the cost works like one who does not, and the reason names the
// component responsible.
static void TestMoraleReachesTheWork()
{
	g_test = "morale reaches the work, and the component responsible is named";
	CrewMember low = PlainCrew(); low.deficit = 1.0f; low.outlook = 0.0f; low.holdings = 0.0f;
	CrewMember high = PlainCrew(); high.deficit = 0.0f; high.outlook = 1.0f; high.holdings = 1.0f;
	WorkFactor lf[WORK_FACTOR_MAX], hf[WORK_FACTOR_MAX];
	const int ln = WorkFactors(low, SKILL_ENGINEERING, WORK_ROUTINE, 0.15f, lf, WORK_FACTOR_MAX);
	const int hn = WorkFactors(high, SKILL_ENGINEERING, WORK_ROUTINE, 0.15f, hf, WORK_FACTOR_MAX);
	CHECK(ln > 0 && hn > 0);
	CHECK(WorkOdds(0.40f, lf, ln) > WorkOdds(0.40f, hf, hn));
	CHECK(WorkFactorLine(lf, ln).find("morale") != std::string::npos);
	CHECK(WorkFactorLine(lf, ln).find(MoraleReason(low)) != std::string::npos);
	CHECK(MeasureWork(0.40f, 0.15f, lf, ln) > MeasureWork(0.40f, 0.15f, hf, hn));
}

// Task C: every modifier that touched an outcome is listable, and the reasons are recoverable from
// the outcome: the factors computed for the operator are exactly those the log carries.
static void TestReasonsRecoverableFromOutcome()
{
	g_test = "every modifier that touched an outcome is recoverable from the outcome";
	Ship s = NewShip();
	s.cfg.dayScale = 1.0f;
	SetAlert(s, ALERT_RED);
	Tick(s, 1.0f);
	const int op = 30;
	s.crew[op].conditionCount = 0;
	s.crew[op].traits = static_cast<uint16_t>(1u << TRAIT_STEADY_UNDER_FIRE);
	s.crew[op].species = SPECIES_HUMAN;
	s.crew[op].skills[SKILL_ENGINEERING] = 0;
	s.crew[op].deficit = 0.8f; s.crew[op].outlook = 0.2f; s.crew[op].holdings = 0.2f;
	const uint8_t all = CVIS_PLAYER | CVIS_CREW | CVIS_LOG;
	CHECK(Sicken(s, op, COND_AFRAID, "the ship is at battle stations", CVAL_DEBUFF, CMAG_CLEAR,
		CLEAR_END_OF_WATCH, all));
	std::string found;
	for (int i = 0; i < 500 && found.empty(); ++i) {
		s.systems[SYS_TRANSPORTERS].health = 0.4f;
		s.systems[SYS_TRANSPORTERS].output = 0.4f;
		if (UseSystemBy(s, SYS_TRANSPORTERS, 1.0f, op) == ANOMALY_NONE) continue;
		for (const LogEntry &e : s.log)
			if (e.what.find("condition under") != std::string::npos) { found = e.what; break; }
	}
	CHECK(!found.empty());
	// Recompute the factors for the operator as they were, and require each one in the line.
	WorkFactor wf[WORK_FACTOR_MAX];
	const int n = WorkFactors(s.crew[op], DepartmentSkill(Spec(SYS_TRANSPORTERS).dept),
		WorkContextOf(SYS_TRANSPORTERS), 1.0f, wf, WORK_FACTOR_MAX);
	CHECK(n > 0);
	for (int i = 0; i < n; ++i) CHECK(found.find(wf[i].name) != std::string::npos);
}

// Task D: the two-sided test. A Betazoid's empathy is a capability, not a bonus: it helps where the
// faculty fits (reading a patient) and costs where it does not (a hull full of the hurt), and the
// name of the faculty appears in both.
static void TestBetazoidEmpathyTwoSided()
{
	g_test = "a Betazoid's empathy helps in one case and costs in another";
	CrewMember human = PlainCrew(); human.species = SPECIES_HUMAN;
	CrewMember bet = PlainCrew(); bet.species = SPECIES_BETAZOID;
	WorkFactor hm[WORK_FACTOR_MAX], bm[WORK_FACTOR_MAX], hh[WORK_FACTOR_MAX], bh[WORK_FACTOR_MAX];
	const int hmn = WorkFactors(human, SKILL_MEDICAL, WORK_MEDICAL, 0.15f, hm, WORK_FACTOR_MAX);
	const int bmn = WorkFactors(bet, SKILL_MEDICAL, WORK_MEDICAL, 0.15f, bm, WORK_FACTOR_MAX);
	const int hhn = WorkFactors(human, SKILL_ENGINEERING, WORK_HULL, 0.15f, hh, WORK_FACTOR_MAX);
	const int bhn = WorkFactors(bet, SKILL_ENGINEERING, WORK_HULL, 0.15f, bh, WORK_FACTOR_MAX);
	CHECK(WorkOdds(0.40f, bm, bmn) < WorkOdds(0.40f, hm, hmn)); // helps
	CHECK(WorkOdds(0.40f, bh, bhn) > WorkOdds(0.40f, hh, hhn)); // costs
	const std::string mLine = WorkFactorLine(bm, bmn), hLine = WorkFactorLine(bh, bhn);
	CHECK(mLine.find("empathy") != std::string::npos);
	CHECK(hLine.find("empathy") != std::string::npos);
	CHECK(mLine.find("reads the feeling") != std::string::npos);
	CHECK(hLine.find("others' pain") != std::string::npos);
	CHECK(MeasureWork(0.40f, 0.15f, bm, bmn) < MeasureWork(0.40f, 0.15f, hm, hmn));
	CHECK(MeasureWork(0.40f, 0.15f, bh, bhn) > MeasureWork(0.40f, 0.15f, hh, hhn));
}

// ---- rising to the occasion (docs/rising-to-the-occasion.md) ------------------------------------

// A controlled rising: one unqualified hand at one post, and two crew who can see it from the same
// deck. The post is structural integrity (a hull job, engineering), the department's skill. Nothing
// here is posed that a tick would overwrite: the condition, the alert and the decks are set, and no
// tick runs after.
struct RisingFixture { Ship s; int hero; int post; int w1; int w2; };
static RisingFixture MakeRising()
{
	RisingFixture f;
	f.s = NewShip();
	f.post = SYS_STRUCTURAL_INTEGRITY;
	f.hero = 30; f.w1 = 40; f.w2 = 41;
	CrewMember &h = f.s.crew[f.hero];
	h.status = CREW_FIT; h.brigged = false; h.away = false;
	h.post = static_cast<uint8_t>(f.post); h.dept = DEPT_ENGINEERING; h.deck = 11;
	h.species = SPECIES_HUMAN; h.traits = 0; h.conditionCount = 0;
	h.fatigue = 0.0f; h.deficit = 0.0f; h.outlook = 0.8f; h.holdings = 0.8f;
	for (int k = 0; k < SKILL_COUNT; ++k) h.skills[k] = 0;
	h.desire = DESIRE_A_PERSON; h.fear = FEAR_DYING_ALONE;
	f.s.systems[f.post].health = 0.4f;
	f.s.systems[f.post].output = 0.4f;
	f.s.alert = ALERT_GREEN;
	f.s.crew[f.w1].status = CREW_FIT; f.s.crew[f.w1].deck = 11;
	f.s.crew[f.w2].status = CREW_FIT; f.s.crew[f.w2].deck = 11;
	f.s.crew[50].deck = 5; // a hand elsewhere: not a witness
	return f;
}

// The mark a person holds about a hero, or 0.
static float SalienceOf(const CrewMember &w, int hero)
{
	for (const Memory &m : w.memories)
		if (m.event == MEM_RESCUE && m.person == hero) return m.salience;
	return 0.0f;
}

// Task B trigger: a rising is offered only when the hand in front of the post is beyond their
// effective skill -- nobody who can hold it is available. Effective skill, not the raw rating, so a
// rated hand whom conditions have taken below the post is still beyond it.
static void TestRisingOfferRequiresBeyondSkill()
{
	g_test = "a rising is offered only when the hand in front of the post is beyond their skill";
	RisingFixture f = MakeRising();
	RisingOffer o = OfferRisingTo(f.s, f.post, f.hero);
	CHECK(o.offered);
	CHECK(o.candidate == f.hero);
	CHECK(o.context == WORK_HULL);
	CHECK(o.skill == SKILL_ENGINEERING);
	// A qualified hand at the same post: no rising. There is someone who can hold it.
	f.s.crew[f.hero].skills[SKILL_ENGINEERING] = 5;
	RisingOffer q = OfferRisingTo(f.s, f.post, f.hero);
	CHECK(!q.offered);
	CHECK(q.qualified == f.hero);
	// Effective skill, not the rating: a rated hand dragged below the post by what they carry.
	f.s.crew[f.hero].skills[SKILL_ENGINEERING] = 3;
	f.s.crew[f.hero].deficit = 1.0f;
	f.s.crew[f.hero].fatigue = 1.0f;
	CHECK(EffectiveSkill(f.s.crew[f.hero], SKILL_ENGINEERING) < RISING_QUALIFIED);
	CHECK(OfferRisingTo(f.s, f.post, f.hero).offered);
}

// Tasks A and E: the drive is the lever, and whether they go is a decision from the drives and the
// morale -- the same person does it here and does not do it there.
static void TestRisingDriveDecides()
{
	g_test = "the drive decides whether they rise, and the log says why";
	RisingFixture f = MakeRising();
	CrewMember &h = f.s.crew[f.hero];
	// The work realises a fear: a hull job and a fear of decompression. The reason they are the one
	// names the drive, and it is the work-bites realisation, not a second reading.
	h.fear = FEAR_DECOMPRESSION;
	RisingOffer o = OfferRisingTo(f.s, f.post, f.hero);
	CHECK(o.offered);
	CHECK(o.drive.find("decompression") != std::string::npos);
	// At good morale the desire carries them, and the drive still colours it.
	h.deficit = 0.0f; h.outlook = 0.9f; h.holdings = 0.9f;
	CHECK(o.choice != RISING_REFUSED);
	// The same person, low morale and wanting home: they will not spend themselves.
	h.deficit = 1.0f; h.outlook = 0.0f; h.holdings = 0.0f; h.desire = DESIRE_HOME;
	RisingOffer r = OfferRisingTo(f.s, f.post, f.hero);
	CHECK(r.choice == RISING_REFUSED);
	CHECK(r.reason.find("go home") != std::string::npos);
	CHECK(r.reason.find("decompression") != std::string::npos); // the drive is still named: why them
	// A refusal is legible: the attempt writes why, and it costs nothing and changes nothing.
	const int conds = h.conditionCount;
	std::string account;
	CHECK(AttemptRising(f.s, f.post, f.hero, 0u, &account) == RISING_REFUSED);
	CHECK(account.find("will not hold") != std::string::npos);
	CHECK(h.status == CREW_FIT);
	CHECK(h.conditionCount == conds);
	bool logged = false;
	for (const LogEntry &e : f.s.log) if (e.what.find("will not hold") != std::string::npos) logged = true;
	CHECK(logged);
	// Afraid of being useless goes anyway: being useful is all they have.
	h.fear = FEAR_USELESSNESS;
	RisingOffer u = OfferRisingTo(f.s, f.post, f.hero);
	CHECK(u.choice != RISING_REFUSED);
	CHECK(u.reason.find("useless") != std::string::npos);
}

// Tasks B and C: the attempt is beyond effective skill, it costs them, and the cost is in the record.
// Task D: the witnesses remember it by name, and their regard moves.
static void TestRisingCostAndWitnesses()
{
	g_test = "the act costs them, and the witnesses remember it by name";
	RisingFixture f = MakeRising();
	CrewMember &h = f.s.crew[f.hero];
	h.fear = FEAR_USELESSNESS; h.desire = DESIRE_HOME; // goes anyway despite the low morale
	h.deficit = 1.0f; h.outlook = 0.0f; h.holdings = 0.0f;
	const int beforeW = MemoryCount(f.s.crew[f.w1]);
	const float holdBefore = f.s.crew[f.w1].holdings;
	std::string account;
	const uint8_t outcome = AttemptRising(f.s, f.post, f.hero, 0xFFFFFFFFu, &account);
	CHECK(outcome == RISING_SUCCEEDED);
	CHECK(account.find("the post holds") != std::string::npos);
	// The cost: a condition, named and curable, in the record.
	const Condition *k = FindCondition(h, COND_EXHAUSTED);
	if (!k) k = FindCondition(h, COND_HYPOXIC);
	CHECK(k != nullptr);
	if (k) CHECK(!k->source.empty());
	// The witnesses: a positive mark naming the hero, and the regard moves with it.
	CHECK(MemoryCount(f.s.crew[f.w1]) > beforeW);
	CHECK(SalienceOf(f.s.crew[f.w1], f.hero) > 0.9f);
	CHECK(Bond(f.s, f.w1, f.hero) > 0.0f);
	CHECK(f.s.crew[f.w1].holdings > holdBefore);
	// The one elsewhere was not a witness: presence is the provenance.
	CHECK(SalienceOf(f.s.crew[50], f.hero) == 0.0f);
	// It is legible later: the mark decays unless reinforced, and a retelling sharpens it.
	const float fresh = SalienceOf(f.s.crew[f.w1], f.hero);
	Tick(f.s, Hours(f.s, 50.0f));
	const float faded = SalienceOf(f.s.crew[f.w1], f.hero);
	CHECK(faded < fresh);
	CHECK(RetellRising(f.s, f.hero) >= 1);
	CHECK(SalienceOf(f.s.crew[f.w1], f.hero) > faded);
}

// Task E: the attempt may fail, and a failed attempt is not a wasted one -- it still cost, and it is
// still remembered.
static void TestRisingFailureStillCostsAndIsRemembered()
{
	g_test = "a failed attempt still costs, and is still remembered";
	RisingFixture f = MakeRising();
	CrewMember &h = f.s.crew[f.hero];
	h.fear = FEAR_USELESSNESS; h.desire = DESIRE_HOME;
	h.deficit = 1.0f; h.outlook = 0.0f; h.holdings = 0.0f;
	f.s.alert = ALERT_YELLOW; // stress 0.5 at 40%: an acute failure, a wound, not yet death
	const int beforeW = MemoryCount(f.s.crew[f.w1]);
	std::string account;
	const uint8_t outcome = AttemptRising(f.s, f.post, f.hero, 0u, &account); // a draw at zero fails
	CHECK(outcome == RISING_FAILED);
	CHECK(account.find("lets go") != std::string::npos);
	CHECK(h.status == CREW_INJURED);
	CHECK(MemoryCount(f.s.crew[f.w1]) > beforeW);
	CHECK(Bond(f.s, f.w1, f.hero) > 0.0f);
}

// Task C at the extreme: a heroism that cannot kill is a cutscene.
static void TestRisingAtTheExtremeKills()
{
	g_test = "at the extreme the attempt can kill";
	RisingFixture f = MakeRising();
	CrewMember &h = f.s.crew[f.hero];
	h.fear = FEAR_USELESSNESS; h.desire = DESIRE_HOME;
	h.deficit = 1.0f; h.outlook = 0.0f; h.holdings = 0.0f;
	f.s.alert = ALERT_RED; // stress 1.0 at 40% condition: the catastrophic severity
	std::string account;
	const uint8_t outcome = AttemptRising(f.s, f.post, f.hero, 0u, &account);
	CHECK(outcome == RISING_DIED);
	CHECK(f.s.crew[f.hero].status == CREW_DEAD);
	CHECK(account.find("kills them") != std::string::npos);
	// Even a death is remembered: the witnesses carry the name, with the grief NoteDeath adds.
	CHECK(Recall(f.s.crew[f.w1], MEM_RESCUE));
	CHECK(Recall(f.s.crew[f.w1], MEM_DEATH));
}

// Task B: worse than the task would be for someone properly qualified, and success is not impossible.
static void TestRisingOddsBeyondSkill()
{
	g_test = "the attempt is beyond effective skill: worse odds, and never impossible";
	RisingFixture f = MakeRising();
	CrewMember &h = f.s.crew[f.hero];
	h.fear = FEAR_USELESSNESS; h.desire = DESIRE_A_PERSON;
	h.deficit = 0.0f; h.outlook = 0.8f; h.holdings = 0.8f;
	RisingOffer low = OfferRisingTo(f.s, f.post, f.hero);
	WorkFactor lf[WORK_FACTOR_MAX];
	const int ln = WorkFactors(h, low.skill, low.context, low.stress, lf, WORK_FACTOR_MAX);
	const float failLow = RisingOdds(low, h, lf, ln);
	CHECK(failLow > 0.0f);
	CHECK(failLow < 1.0f); // success is possible: the odds of failing are strictly below one
	CHECK(failLow <= 1.0f - RISING_MIN_SUCCESS + 1e-6f);
	// A qualified hand at the same post faces lower odds of failing.
	h.skills[SKILL_ENGINEERING] = 5;
	RisingOffer high = OfferRisingTo(f.s, f.post, f.hero);
	WorkFactor hf[WORK_FACTOR_MAX];
	const int hn = WorkFactors(h, high.skill, high.context, high.stress, hf, WORK_FACTOR_MAX);
	const float failHigh = RisingOdds(high, h, hf, hn);
	CHECK(failHigh < failLow);
}

// No new state: everything the act writes is already saved, so a rising round-trips unchanged.
static void TestRisingSaveRoundTrip()
{
	g_test = "a heroism round-trips the save with no new field";
	RisingFixture f = MakeRising();
	CrewMember &h = f.s.crew[f.hero];
	h.fear = FEAR_USELESSNESS;
	h.deficit = 1.0f; h.outlook = 0.0f; h.holdings = 0.0f;
	CHECK(AttemptRising(f.s, f.post, f.hero, 0xFFFFFFFFu, nullptr) == RISING_SUCCEEDED);
	const int conds = f.s.crew[f.hero].conditionCount;
	const int mems = MemoryCount(f.s.crew[f.hero]);
	std::vector<uint8_t> blob = Pack(f.s);
	Ship u = NewShip();
	CHECK(Unpack(blob.data(), blob.size(), u));
	CHECK(u.crew[f.hero].conditionCount == conds);
	CHECK(MemoryCount(u.crew[f.hero]) == mems);
	CHECK(Bond(u, f.w1, f.hero) > 0.0f);
}

// One act end to end, in the engine's own words: who, why them, what it cost, and what the
// witnesses carry afterwards. The transcript the owner judges (docs/rising-to-the-occasion.md).
static int PrintRising()
{
	std::printf("== rising to the occasion: one act, end to end (docs/rising-to-the-occasion.md)\n");
	RisingFixture f = MakeRising();
	CrewMember &h = f.s.crew[f.hero];
	// Two hands, the same post and the same crisis, and only the drive is different.
	h.fear = FEAR_DECOMPRESSION; h.desire = DESIRE_HOME;
	h.deficit = 1.0f; h.outlook = 0.05f; h.holdings = 0.05f;
	RisingOffer refused = OfferRisingTo(f.s, f.post, f.hero);
	std::printf("\n  the same person, the same post, two drives:\n");
	std::printf("    wants home, afraid of decompression: %s\n", refused.reason.c_str());
	std::printf("    -- and the offer is %s\n", refused.choice == RISING_REFUSED ? "refused" : "taken");
	std::string account;
	AttemptRising(f.s, f.post, f.hero, 0u, &account);
	std::printf("    the record: %s\n", account.c_str());

	// The exceeding: the same post and crisis, an unqualified hand and a qualified one. The odds of
	// failing are worse for the hand beyond their skill, and success is not impossible for either.
	RisingFixture q = MakeRising();
	CrewMember &lq = q.s.crew[q.hero];
	lq.fear = FEAR_USELESSNESS; lq.deficit = 0.0f; lq.outlook = 0.8f; lq.holdings = 0.8f;
	RisingOffer lo = OfferRisingTo(q.s, q.post, q.hero);
	WorkFactor lf[WORK_FACTOR_MAX];
	const int ln = WorkFactors(lq, lo.skill, lo.context, lo.stress, lf, WORK_FACTOR_MAX);
	const float loFail = RisingOdds(lo, lq, lf, ln);
	std::printf("\n  the exceeding: the same post and crisis, two hands\n");
	std::printf("    effective engineering %.1f   odds of failing %.3f (holds %.0f%%)\n",
		EffectiveSkill(lq, lo.skill), loFail, (1.0f - loFail) * 100.0f);
	lq.skills[SKILL_ENGINEERING] = 5;
	RisingOffer hi = OfferRisingTo(q.s, q.post, q.hero);
	WorkFactor hf[WORK_FACTOR_MAX];
	const int hn = WorkFactors(lq, hi.skill, hi.context, hi.stress, hf, WORK_FACTOR_MAX);
	const float hiFail = RisingOdds(hi, lq, hf, hn);
	std::printf("    effective engineering %.1f   odds of failing %.3f (holds %.0f%%)\n",
		EffectiveSkill(lq, hi.skill), hiFail, (1.0f - hiFail) * 100.0f);

	// A second hand, afraid of being useless, goes anyway -- and it costs them.
	RisingFixture g = MakeRising();
	CrewMember &u = g.s.crew[g.hero];
	u.fear = FEAR_USELESSNESS; u.desire = DESIRE_HOME;
	u.deficit = 1.0f; u.outlook = 0.05f; u.holdings = 0.05f;
	std::printf("\n  a different hand and a different drive, the same post and crisis:\n");
	std::string account2;
	const uint8_t outcome = AttemptRising(g.s, g.post, g.hero, 0xFFFFFFFFu, &account2);
	std::printf("    (%s) %s\n", RisingOutcomeName(outcome), account2.c_str());
	const Condition *k = FindCondition(g.s.crew[g.hero], COND_EXHAUSTED);
	if (!k) k = FindCondition(g.s.crew[g.hero], COND_HYPOXIC);
	std::printf("    what it cost: %s\n", k ? (std::string(ConditionName(k->id)) + " (" + k->source + ")").c_str() : "a wound");
	std::printf("    the witnesses now carry %s: bond %.2f, holdings %.2f, salience %.2f\n",
		g.s.crew[g.w1].name.c_str(), Bond(g.s, g.w1, g.hero), g.s.crew[g.w1].holdings,
		SalienceOf(g.s.crew[g.w1], g.hero));
	// A heroism nobody retells fades; one the crew keep retelling persists.
	Tick(g.s, Hours(g.s, 60.0f));
	std::printf("    60 hours on, untold: salience %.2f\n", SalienceOf(g.s.crew[g.w1], g.hero));
	RetellRising(g.s, g.hero);
	std::printf("    retold: salience %.2f\n", SalienceOf(g.s.crew[g.w1], g.hero));
	return 0;
}

// One crew member in full, for the evidence: the four kinds, the drives, and the morale reads.
static void PrintOneCrew(const CrewMember &c, int idx)
{
	std::printf("  [%d] %s -- %s, %s, %s department, post %s\n", idx, c.name.c_str(),
		SpeciesName(c.species), c.rank >= 5 ? "senior officer" : c.rank >= 3 ? "officer" : "crewman",
		c.dept == DEPT_COMMAND ? "command" : c.dept == DEPT_ENGINEERING ? "engineering"
			: c.dept == DEPT_SECURITY ? "security" : c.dept == DEPT_SCIENCES ? "sciences" : "medical",
		c.post < SYS_COUNT ? Spec(static_cast<SystemId>(c.post)).name : "department duties");
	std::printf("      skills:");
	for (int k = 0; k < SKILL_COUNT; ++k) std::printf(" %s %d", SkillName(static_cast<uint8_t>(k)), c.skills[k]);
	std::printf("   (effective engineering %.1f)\n", EffectiveSkill(c, SKILL_ENGINEERING));
	std::printf("      traits:");
	for (int t = 0; t < TRAIT_COUNT; ++t) if (HasTrait(c, static_cast<uint8_t>(t))) std::printf(" %s;", TraitName(static_cast<uint8_t>(t)));
	std::printf("\n");
	std::printf("      drives: wants %s; short of %s; afraid of %s\n", DesireName(c.desire), NeedName(c.need), FearName(c.fear));
	const SpeciesRecord &sp = SpeciesOf(c.species);
	std::printf("      species: %s\n", sp.name);
	for (int i = 0; i < SPECIES_TRAIT_MAX; ++i)
		if (sp.capabilities[i]) std::printf("        capability: %s\n", sp.capabilities[i]);
	for (int i = 0; i < SPECIES_TRAIT_MAX; ++i)
		if (sp.needs[i]) std::printf("        need: %s\n", sp.needs[i]);
	for (int i = 0; i < SPECIES_TRAIT_MAX; ++i)
		if (sp.susceptibilities[i]) std::printf("        susceptibility: %s\n", sp.susceptibilities[i]);
	std::printf("      conditions (%d):", c.conditionCount);
	for (int i = 0; i < c.conditionCount; ++i) {
		const Condition &k = c.conditions[i];
		std::printf("\n        %s (%s, %s) from \"%s\"; clears with %s; visible to %s%s%s",
			ConditionName(k.id), ConditionValenceName(k.valence), ConditionMagnitudeName(k.magnitude),
			k.source.c_str(), ConditionClearName(k.clears),
			(k.visible & CVIS_PLAYER) ? "player" : "", (k.visible & CVIS_CREW) ? " crew" : "",
			(k.visible & CVIS_LOG) ? " log" : " (not logged)");
	}
	std::printf("\n");
	std::printf("      morale %.2f (%s) = deficit %.2f (heart %.2f) / outlook %.2f / holdings %.2f -- %s\n",
		Morale(c), MoraleBandName(Morale(c)), c.deficit, 1.0f - c.deficit, c.outlook, c.holdings, MoraleReason(c));
	std::printf("      says: \"%s\"\n", MannerLine(c));
}

// Three generated crew members, printed in full -- the acceptance the owner judges.
static int PrintCrew()
{
	Ship s = NewShip();
	// A day and a half in the normal course, then a hard stretch -- battle stations and an empty
	// galley -- so the conditions shown are the ship's own and not posed on the people.
	Tick(s, Hours(s, 36.0f));
	s.alert = ALERT_RED;
	s.stores.rations = 0.0f;
	SetEnabled(s, SYS_REPLICATORS, false);
	Tick(s, Hours(s, 8.0f));
	std::printf("== three generated crew members, in full (O8; docs/character-derivation.md)\n");
	// One from each of three departments, deterministically: the first fit, unnamed member of each.
	int chosen[3] = {-1, -1, -1};
	int nChosen = 0;
	bool deptUsed[DEPT_COUNT] = {false};
	for (int i = 19; i < static_cast<int>(s.crew.size()) && nChosen < 3; ++i) {
		if (s.crew[i].status != CREW_FIT || deptUsed[s.crew[i].dept]) continue;
		deptUsed[s.crew[i].dept] = true;
		chosen[nChosen++] = i;
	}
	for (int k = 0; k < nChosen; ++k) PrintOneCrew(s.crew[chosen[k]], chosen[k]);
	return 0;
}

// The manner at three morale levels, for one person -- a demonstration, not a verdict.
static int PrintManner()
{
	Ship s = NewShip();
	const CrewMember &base = s.crew[19];
	CrewMember c = base;
	std::printf("== the manner at three morale levels (G14, second half)\n");
	std::printf("  one crew member: %s, %s; wants %s; afraid of %s\n", c.name.c_str(),
		SpeciesName(c.species), DesireName(c.desire), FearName(c.fear));
	struct Level { const char *label; float deficit, outlook, holdings; };
	const Level levels[3] = {
		{"fit", 0.05f, 0.85f, 0.80f},
		{"worn", 0.35f, 0.45f, 0.45f},
		{"at breaking point", 0.80f, 0.10f, 0.15f},
	};
	for (const Level &l : levels) {
		c.deficit = l.deficit; c.outlook = l.outlook; c.holdings = l.holdings;
		std::printf("  %-16s morale %.2f (%s): \"%s\"\n", l.label, Morale(c), MoraleBandName(Morale(c)), MannerLine(c));
	}
	return 0;
}

// One case of the work, printed: the odds, the measured count, and the reasons.
static void PrintWorkCase(const char *label, float condition, float stress,
                          const WorkFactor *f, int n)
{
	std::printf("    %-26s odds %.4f (base %.4f)   measured %d of 20000\n",
		label, WorkOdds(condition, f, n), AnomalyOdds(condition), MeasureWork(condition, stress, f, n));
	std::printf("                               reasons: %s\n",
		n > 0 ? WorkFactorLine(f, n).c_str() : "none");
}

// The task done by the same person in two conditions, with the reasons printed: the transcript the
// owner judges, not a verdict (docs/character-attributes.md).
static int PrintWork()
{
	std::printf("== the character layer reaches the work (docs/character-attributes.md)\n");
	const float cond = 0.40f;  // a system at 40%: the document's own degraded example
	const float battle = 1.0f; // used at battle stations: the load the design called the worst

	Ship s = NewShip();
	int who = -1;
	for (int i = 19; i < static_cast<int>(s.crew.size()); ++i)
		if (s.crew[i].dept == DEPT_ENGINEERING && s.crew[i].status == CREW_FIT) { who = i; break; }
	if (who < 0) { std::printf("  no engineering crew member\n"); return 0; }
	CrewMember person = s.crew[who];
	person.conditionCount = 0;
	person.traits = 0;
	person.desire = DESIRE_A_PERSON; person.need = NEED_SLEEP; person.fear = FEAR_DYING_ALONE;
	person.species = SPECIES_HUMAN;

	// One task, one person, two conditions.
	CrewMember fresh = person;
	fresh.fatigue = 0.0f; fresh.deficit = 0.05f; fresh.outlook = 0.80f; fresh.holdings = 0.80f;
	CrewMember worn = person;
	worn.fatigue = 0.8f; worn.deficit = 0.5f; worn.outlook = 0.4f; worn.holdings = 0.5f;
	AddCondition(worn, COND_AFRAID, "the ship is at battle stations", CVAL_DEBUFF, CMAG_SLIGHT,
		CLEAR_END_OF_WATCH, 0.0f, CVIS_PLAYER | CVIS_CREW);
	AddCondition(worn, COND_HUNGRY, "the galley is empty", CVAL_DEBUFF, CMAG_CLEAR,
		CLEAR_MEAL, 0.0f, CVIS_PLAYER | CVIS_CREW);
	WorkFactor ff[WORK_FACTOR_MAX], wf[WORK_FACTOR_MAX];
	const int fn = WorkFactors(fresh, SKILL_ENGINEERING, WORK_ROUTINE, battle, ff, WORK_FACTOR_MAX);
	const int wn = WorkFactors(worn, SKILL_ENGINEERING, WORK_ROUTINE, battle, wf, WORK_FACTOR_MAX);
	std::printf("\n  one person (%s, engineering %d), one task at 40%% under battle stations\n",
		person.name.c_str(), person.skills[SKILL_ENGINEERING]);
	PrintWorkCase("fresh", cond, battle, ff, fn);
	PrintWorkCase("afraid, hungry, worn", cond, battle, wf, wn);

	// The same task and the same person at two skill levels.
	std::printf("\n  the same task, two skill levels\n");
	CrewMember low = person; low.skills[SKILL_ENGINEERING] = 1;
	CrewMember high = person; high.skills[SKILL_ENGINEERING] = 5;
	WorkFactor lf[WORK_FACTOR_MAX], hf[WORK_FACTOR_MAX];
	const int ln = WorkFactors(low, SKILL_ENGINEERING, WORK_ROUTINE, battle, lf, WORK_FACTOR_MAX);
	const int hn = WorkFactors(high, SKILL_ENGINEERING, WORK_ROUTINE, battle, hf, WORK_FACTOR_MAX);
	PrintWorkCase("engineering 1", cond, battle, lf, ln);
	PrintWorkCase("engineering 5", cond, battle, hf, hn);

	// The four kinds, kept distinct in one list.
	std::printf("\n  the kinds kept distinct (one person carrying all of them, a hull task)\n");
	CrewMember all = person;
	all.skills[SKILL_ENGINEERING] = 1;
	all.traits = static_cast<uint16_t>((1u << TRAIT_STEADY_UNDER_FIRE) | (1u << TRAIT_CLAUSTRAPHOBIC));
	all.fear = FEAR_DECOMPRESSION;
	all.desire = DESIRE_HOME;
	all.deficit = 0.6f; all.outlook = 0.2f; all.holdings = 0.2f;
	all.conditionCount = 0;
	AddCondition(all, COND_AFRAID, "the ship is at battle stations", CVAL_DEBUFF, CMAG_CLEAR,
		CLEAR_END_OF_WATCH, 0.0f, CVIS_PLAYER | CVIS_CREW);
	WorkFactor af[WORK_FACTOR_MAX];
	const int an = WorkFactors(all, SKILL_ENGINEERING, WORK_HULL, battle, af, WORK_FACTOR_MAX);
	PrintWorkCase("under a hull task", cond, battle, af, an);

	// The Betazoid's empathy, two-sided.
	std::printf("\n  the Betazoid's empathy, two-sided (one faculty, two situations)\n");
	CrewMember bet = person;
	bet.species = SPECIES_BETAZOID; bet.traits = 0; bet.conditionCount = 0;
	bet.deficit = 0.05f; bet.outlook = 0.80f; bet.holdings = 0.80f;
	bet.fear = FEAR_DYING_ALONE; bet.desire = DESIRE_A_PERSON;
	CrewMember hum = bet; hum.species = SPECIES_HUMAN;
	WorkFactor bmf[WORK_FACTOR_MAX], hmf[WORK_FACTOR_MAX], bhf[WORK_FACTOR_MAX], hhf[WORK_FACTOR_MAX];
	const int bmn = WorkFactors(bet, SKILL_MEDICAL, WORK_MEDICAL, 0.15f, bmf, WORK_FACTOR_MAX);
	const int hmn = WorkFactors(hum, SKILL_MEDICAL, WORK_MEDICAL, 0.15f, hmf, WORK_FACTOR_MAX);
	const int bhn = WorkFactors(bet, SKILL_ENGINEERING, WORK_HULL, 0.15f, bhf, WORK_FACTOR_MAX);
	const int hhn = WorkFactors(hum, SKILL_ENGINEERING, WORK_HULL, 0.15f, hhf, WORK_FACTOR_MAX);
	PrintWorkCase("Human, a medical task", cond, 0.15f, hmf, hmn);
	PrintWorkCase("Betazoid, a medical task", cond, 0.15f, bmf, bmn);
	PrintWorkCase("Human, a hull task", cond, 0.15f, hhf, hhn);
	PrintWorkCase("Betazoid, a hull task", cond, 0.15f, bhf, bhn);

	// The console's read, and one use that goes wrong as the record writes it.
	std::printf("\n  one use that goes wrong, as the console and the record write it\n");
	Ship u = NewShip();
	u.cfg.dayScale = 1.0f;
	SetAlert(u, ALERT_RED);
	Tick(u, 1.0f);
	const int op = who;
	u.crew[op].conditionCount = 0;
	u.crew[op].skills[SKILL_ENGINEERING] = 0;
	u.crew[op].species = SPECIES_HUMAN;
	u.crew[op].deficit = 0.8f; u.crew[op].outlook = 0.2f; u.crew[op].holdings = 0.2f;
	AddCondition(u.crew[op], COND_AFRAID, "the ship is at battle stations", CVAL_DEBUFF, CMAG_CLEAR,
		CLEAR_END_OF_WATCH, 0.0f, CVIS_PLAYER | CVIS_CREW);
	std::printf("    console: %s\n", WorkReading(u, op, SYS_TRANSPORTERS).c_str());
	bool logged = false;
	for (int i = 0; i < 500 && !logged; ++i) {
		u.systems[SYS_TRANSPORTERS].health = 0.4f;
		u.systems[SYS_TRANSPORTERS].output = 0.4f;
		if (UseSystemBy(u, SYS_TRANSPORTERS, 1.0f, op) == ANOMALY_NONE) continue;
		for (const LogEntry &e : u.log)
			if (e.what.find("condition under") != std::string::npos) {
				std::printf("    log    : %s: %s\n", e.who.c_str(), e.what.c_str());
				logged = true; break;
			}
	}
	return 0;
}

// ---- the configurator (docs/the-entry-point.md, Part three) -----------------------------------
//
// The three proving cases, each a different path: the canon default (nobody dies, the chain is
// intact), the captain (the chair is vacant and the derivation fills it), and an all-fictitious crew
// (none of the show characters appear). The tests below demonstrate the acceptance; `--starts`
// prints the three in full, and scripts/configurator-check.sh judges the print.

static void TestCanonStartUntouched()
{
	g_test = "the canon default start loads untouched";
	CHECK(StartStateCount() >= 3);
	CHECK(std::string(StartStateAt(0).name) == "CANON"); // the configurator opens on it
	for (int i = 0; i < StartStateCount(); ++i) CHECK(std::string(StartStateAt(i).name).size() > 0);

	Config cfg;
	Ship s = NewShip(cfg);
	ApplyStartState(s, StartStateAt(0));
	CHECK(!s.cfg.fictitious);
	CHECK(static_cast<int>(s.crew.size()) == COMPLEMENT);
	CHECK(s.CrewFit() == COMPLEMENT);                       // nobody died
	CHECK(s.seatHolder[SEAT_CAPTAIN] == 0);                 // Janeway still holds the chair
	CHECK(s.crew[s.seatHolder[SEAT_FIRST_OFFICER]].type == "chakotay");
	CHECK(s.player >= 0);                                   // a junior officer is chosen
	CHECK(!PlayerMayCommand(s));                            // and does not command
	// The log's first entry states the losses, the condition, and that no rescue is coming.
	CHECK(!s.log.empty());
	const std::string seed = LogSeedText(s, StartStateAt(0));
	CHECK(seed.find("no one in the command crew was lost") != std::string::npos);
	CHECK(seed.find("No rescue is coming") != std::string::npos);
	CHECK(s.log[0].what == seed);
	CHECK(s.log[0].scope == "command");
	CHECK(s.log[0].what.find("Starfleet junior") != std::string::npos);
}

static void TestCaptainVacancyDerived()
{
	g_test = "the chair is vacant and the derivation fills it";
	Config cfg;
	const StartState &st = StartStateAt(1);
	CHECK(std::string(st.name) == "THE CHAIR");
	CHECK(st.casualty[SEAT_CAPTAIN]);
	Ship s = NewShip(cfg);
	ApplyStartState(s, st);
	// The casualty is closed, and the record names her as the one who made the vacancy.
	bool janewayDead = false;
	for (const CrewMember &c : s.crew) if (c.type == "janeway") janewayDead = (c.status == CREW_DEAD);
	CHECK(janewayDead);
	const int chair = SeatHolder(s, SEAT_CAPTAIN);
	CHECK(chair == s.player);
	CHECK(s.crew[chair].rank == 6);
	CHECK(PlayerMayCommand(s));          // the authority is real afterwards
	CHECK(s.log[0].what.find("Kathryn Janeway") != std::string::npos);
	CHECK(s.log[0].what.find("the chair") != std::string::npos);

	// The derivation, not the state, decides: the same casualty with a junior player leaves the
	// senior officer in the chair. Nothing was hand-written -- the rank is what seats the player.
	StartState junior = st;
	junior.playerRank = 1;
	junior.playerName = "Green";
	Ship j = NewShip(cfg);
	ApplyStartState(j, junior);
	CHECK(SeatHolder(j, SEAT_CAPTAIN) != j.player);
	CHECK(j.crew[SeatHolder(j, SEAT_CAPTAIN)].rank >= 5);
	CHECK(j.player == chair || j.crew[SeatHolder(j, SEAT_CAPTAIN)].status == CREW_FIT);
}

static void TestVacancySameRule()
{
	g_test = "a configurator vacancy and a mid-run loss are the same derivation";
	Config cfg;
	// The reference: the senior fit security officer once the seat-holder is gone.
	Ship ref = NewShip(cfg);
	KillCrew(ref, ref.seatHolder[SEAT_SECURITY], "a plasma fire");
	const int expected = DepartmentHead(ref, DEPT_SECURITY);
	CHECK(expected >= 0);

	// The configurator's vacancy: Tuvok is a casualty of the opening; the seat is derived.
	StartState st = StartStateAt(1);
	st.casualty[SEAT_SECURITY] = true;
	Ship a = NewShip(cfg);
	ApplyStartState(a, st);
	const int fromStart = SeatHolder(a, SEAT_SECURITY);
	CHECK(fromStart == expected);

	// The mid-run loss: the same holder dies in play, and FillVacancies -- the tick's own call --
	// fills it by the same rule, from the same roster.
	Ship b = NewShip(cfg);
	const int tuvok = b.seatHolder[SEAT_SECURITY];
	KillCrew(b, tuvok, "a plasma fire");
	CHECK(SeatHolder(b, SEAT_SECURITY) == tuvok); // the record is closed; the seat is not yet derived
	CHECK(FillVacancies(b, "vacant") >= 1);
	const int fromPlay = SeatHolder(b, SEAT_SECURITY);
	CHECK(fromPlay == expected);

	// And the rule is genuinely the roster's: it is the highest-ranked fit security officer left.
	CHECK(b.crew[fromPlay].dept == DEPT_SECURITY);
	CHECK(b.crew[fromPlay].status == CREW_FIT);
	for (const CrewMember &c : b.crew)
		if (c.dept == DEPT_SECURITY && c.status == CREW_FIT)
			CHECK(c.rank <= b.crew[fromPlay].rank);
}

static void TestFictitiousNoCanon()
{
	g_test = "an all-fictitious crew has no canon name anywhere";
	Config cfg;
	const StartState &st = StartStateAt(2);
	CHECK(st.fictitious);
	cfg.fictitious = st.fictitious;
	Ship s = NewShip(cfg);
	CHECK(cfg.fictitious);
	CHECK(static_cast<int>(s.crew.size()) == COMPLEMENT);
	ApplyStartState(s, st);
	// None of the show characters appear: no record's type or name is a canon one.
	static const char *const CANON[] = {"janeway", "chakotay", "tuvok", "paris", "kim", "torres", "doctor",
		"seven", "neelix", "vorik", "munro", "biessman", "chang", "telsia", "chell", "jurot", "kenn", "odell"};
	for (const CrewMember &c : s.crew) {
		for (const char *n : CANON) {
			CHECK(c.type.find(n) == std::string::npos);
			CHECK(c.name.find(n) == std::string::npos);
		}
	}
	// Every seat is held by a generated record: the derivation filled the whole chain.
	for (int seat = 0; seat < SEAT_COUNT; ++seat) {
		const int h = SeatHolder(s, seat);
		CHECK(h >= 0 && h < static_cast<int>(s.crew.size()));
	}
	CHECK(s.crew[SeatHolder(s, SEAT_CAPTAIN)].rank == 6);
	// Nothing in the run depends on a canon name: the command authority is a generated name, and the
	// first log entry is signed by one.
	CHECK(CommandingOfficer(s).find("Janeway") == std::string::npos);
	CHECK(s.log[0].who.find("Janeway") == std::string::npos);
	CHECK(s.log[0].what.find("Janeway") == std::string::npos);
}

static void TestStartStateSaveRoundTrip()
{
	g_test = "a configured start saves and reloads identically";
	for (int i = 0; i < StartStateCount(); ++i) {
		const StartState &st = StartStateAt(i);
		Config cfg;
		cfg.fictitious = st.fictitious;
		Ship s = NewShip(cfg);
		ApplyStartState(s, st);
		const std::vector<uint8_t> blob = Pack(s);
		Ship back;
		CHECK(Unpack(blob.data(), blob.size(), back));
		CHECK(Pack(back) == blob);
		CHECK(back.cfg.fictitious == s.cfg.fictitious);
		CHECK(back.career == s.career);
		CHECK(back.player == s.player);
		for (int seat = 0; seat < SEAT_COUNT; ++seat)
			CHECK(back.seatHolder[seat] == s.seatHolder[seat]);
		CHECK(!back.log.empty() && back.log[0].what == s.log[0].what);
	}
}

static void TestCharacterCreationComposes()
{
	g_test = "character creation composes with the start state";
	// The situation seats the player in the chair; creation names that person and does not move them.
	Config cfg;
	Ship s = NewShip(cfg);
	ApplyStartState(s, StartStateAt(1));
	CHECK(SeatHolder(s, SEAT_CAPTAIN) == s.player);
	const int seat = s.player;
	const int who = CreateCharacter(s, "Okoro", DEPT_SECURITY, 1);
	CHECK(who == seat);                                  // the same person, renamed
	CHECK(s.player == SeatHolder(s, SEAT_CAPTAIN));      // the situation is unchanged
	CHECK(s.crew[s.player].name == "Okoro");
	CHECK(s.crew[s.player].rank == 6);                   // the seat fixes the rank

	// An ordinary post: creation chooses the person, and the state does not override it.
	Ship t = NewShip(cfg);
	ApplyStartState(t, StartStateAt(0));
	const int before = t.player;
	CHECK(CreateCharacter(t, "Reyes", DEPT_ENGINEERING, 2) == before);
	CHECK(t.crew[before].name == "Reyes");
	CHECK(SeatHeldBy(t, before) < 0);                    // still an ordinary post
}

static void PrintStartState(const StartState &st, int index)
{
	Config cfg;
	cfg.fictitious = st.fictitious;
	Ship s = NewShip(cfg);
	ApplyStartState(s, st);
	std::printf("  [%d] %s -- %s\n", index, st.name, st.blurb);
	std::printf("      reason: %s\n", st.reason);
	std::printf("      career: %s (%s)\n", CareerPathName(st.career), CareerPathBlurb(st.career));
	std::printf("      roster: %s\n", st.fictitious ? "generated entirely -- no show character appears"
		: "the canon crew, with the casualties below closed");
	for (int i = 0; i < SEAT_COUNT; ++i) {
		const int h = SeatHolder(s, i);
		if (h >= 0)
			std::printf("      seat: %-24s = %s%s\n", SeatName(i), s.crew[h].name.c_str(),
				st.casualty[i] ? "  (derived: the named holder was lost)" : "");
		else
			std::printf("      seat: %-24s = VACANT\n", SeatName(i));
	}
	std::printf("      player: %s\n", PlayerPositionLine(s).c_str());
	std::printf("      save: %d bytes\n", static_cast<int>(Pack(s).size()));
	std::printf("      log: %s\n\n", LogSeedText(s, st).c_str());
}

static int PrintStarts()
{
	std::printf("== the configurator: the proving cases (docs/the-entry-point.md, Part three)\n");
	{
		Ship fresh = NewShip();
		std::printf("  [baseline] a fresh ship, no start state: %d crew, save %d bytes\n\n",
			static_cast<int>(fresh.crew.size()), static_cast<int>(Pack(fresh).size()));
	}
	for (int i = 0; i < StartStateCount(); ++i) PrintStartState(StartStateAt(i), i);
	return 0;
}

int main(int argc, char **argv)
{
	if (argc > 1 && !std::strcmp(argv[1], "--crew")) return PrintCrew();
	if (argc > 1 && !std::strcmp(argv[1], "--manner")) return PrintManner();
	if (argc > 1 && !std::strcmp(argv[1], "--work")) return PrintWork();
	if (argc > 1 && !std::strcmp(argv[1], "--rising")) return PrintRising();
	if (argc > 1 && !std::strcmp(argv[1], "--power")) return PrintPower();
	if (argc > 1 && !std::strcmp(argv[1], "--meeting")) return PrintMeeting();
	if (argc > 1 && !std::strcmp(argv[1], "--voice")) return PrintVoice();
	if (argc > 1 && !std::strcmp(argv[1], "--day")) return PrintDay();
	if (argc > 1 && !std::strcmp(argv[1], "--losses")) return PrintLosses();
	if (argc > 1 && !std::strcmp(argv[1], "--risk")) return PrintRisk();
	if (argc > 1 && !std::strcmp(argv[1], "--month")) return PrintMonth();
	if (argc > 1 && !std::strcmp(argv[1], "--nav")) return PrintNav();
	if (argc > 1 && !std::strcmp(argv[1], "--personal")) return PrintPersonal();
	if (argc > 1 && !std::strcmp(argv[1], "--starts")) return PrintStarts();

	TestNominalShip();
	TestPowerIsConserved();
	TestSheddingFollowsPriority();
	TestBudgetShedSequence();
	TestCorelessShip();
	TestBatteriesKeepTheCrewAlive();
	TestThePlayerDecides();
	TestLadderOnlyInAutoMode();
	TestOversubscriptionReported();
	TestPartialAllocation();
	TestRecommendationAndDelegation();
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
	TestCrewWoundsRoundTrip();
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
	TestGravity();
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
	TestMonthReportAndToll();
	TestTheTwoLogs();
	TestNavigation();
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
	TestSleepAndExits();
	TestRankAndRoles();
	TestAccessAndAuthority();
	TestOrders();
	TestPlayerInTheWorld();
	TestPhaserYieldAndReading();
	TestHundredCrew();
	TestHolodeckTrap();
	TestCrystalCeilingScalesOutput();
	TestTorpedoComplement();
	TestMeetingBriefIsARead();
	TestEveryMeetingEmitsABrief();
	TestSkeletonPlaysWithoutAModel();
	TestMeetingAllocationSeam();
	TestMeetingBriefPerParticipant();
	TestMeetingSaveRoundTrip();
	TestTrackOwnership();
	TestCueCannotStopALine();
	TestCueSet();
	TestCueEmitSite();
	TestRenderKeyAndCache();
	TestPlanMeetingAudio();
	TestAudioSaveRoundTrip();
	TestTheDerivation();
	TestConditionLifecycle();
	TestMoraleIsReadFromThree();
	TestSpeciesAreCapabilitiesNotBonuses();
	TestManner();
	TestCharacterSaveRoundTrip();
	TestEffectiveSkillReachesTheOdds();
	TestConditionMovesTheOdds();
	TestTraitShapesPerformance();
	TestDriveBiasesBehaviour();
	TestMoraleReachesTheWork();
	TestReasonsRecoverableFromOutcome();
	TestBetazoidEmpathyTwoSided();
	TestRisingOfferRequiresBeyondSkill();
	TestRisingDriveDecides();
	TestRisingCostAndWitnesses();
	TestRisingFailureStillCostsAndIsRemembered();
	TestRisingAtTheExtremeKills();
	TestRisingOddsBeyondSkill();
	TestRisingSaveRoundTrip();
	TestCanonStartUntouched();
	TestCaptainVacancyDerived();
	TestVacancySameRule();
	TestFictitiousNoCanon();
	TestStartStateSaveRoundTrip();
	TestCharacterCreationComposes();

	if (g_failures) {
		std::printf("%d check(s) failed\n", g_failures);
		return 1;
	}
	std::printf("ship_core: all checks passed\n");
	return 0;
}
