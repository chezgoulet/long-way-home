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

struct System {
	float health = 1.0f;    // 0 destroyed .. 1 intact; output can never exceed it
	bool enabled = true;    // switched on at its console
	int priority = 0;       // current shedding order (consoles may change it)
	int allocated = 0;      // EPS units granted this tick
	int manned = 0;         // on-duty crew at the station this tick
	int repairing = 0;      // damage-control crew working on it this tick
	float control = 1.0f;   // 1 = the crew's, 0 = the intruders'; below HIJACKED it answers to them, not to us
	float output = 0.0f;    // 0..1: what the system is actually delivering
};

// ---- power ------------------------------------------------------------------------------------

enum SourceId : uint8_t { SRC_WARP_CORE = 0, SRC_IMPULSE_REACTORS, SRC_AUXILIARY, SRC_BATTERIES, SRC_COUNT };

struct Source {
	float health = 1.0f;
	bool online = true;
	int output = 0;         // EPS units supplied this tick
};

// ---- compartments and consumables ---------------------------------------------------------------

struct Deck {
	float atmosphere = 1.0f; // 0 vacuum .. 1 breathable
	float hull = 1.0f;       // 0 open to space .. 1 intact
	float intruders = 0.0f;  // hostile boarders on this deck (fractional while a fight wears them down)
	bool borg = false;       // the boarders here are Borg: they assimilate what they hold
	float assimilated = 0.0f; // 0 ours .. 1 wholly Borg; at ASSIMILATED the deck's systems are theirs outright
	int stripping = 0;       // engineers cutting Borg technology out of this deck this tick
	int defenders = 0;       // security crew fighting here this tick
};

struct Stores {
	float deuterium = 1.0f;      // fraction of tankage; the fusion reactors and the warp core burn it
	float antimatter = 1.0f;     // fraction of pods; the warp core burns it
	float batteries = 1.0f;      // fraction of emergency cell charge
	int torpedoes = 38;          // not replaceable
	float spareParts = 100.0f;   // what repairs are made of; a system rebuilt from nothing costs PARTS_PER_SYSTEM
};

// ---- crew -------------------------------------------------------------------------------------

enum Activity : uint8_t { ACT_ON_DUTY = 0, ACT_MEAL, ACT_RECREATION, ACT_PERSONAL, ACT_SLEEP, ACT_COUNT };
enum CrewStatus : uint8_t { CREW_FIT = 0, CREW_INJURED, CREW_DEAD, CREW_ASSIMILATED };

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
	float exposure = 0.0f;   // seconds spent on a deck without air; injures, then kills
	float wounds = 0.0f;     // 0..1 taken fighting boarders; at 1 they are out of the fight, injured
	float recovery = 0.0f;   // 0..1 progress of an injured crew member's treatment

	// derived each tick
	uint8_t activity = ACT_SLEEP;
	uint8_t deck = 0;        // where they are now
};

// Where a crew member is and what they are doing at a time of day, by their watch alone. The day
// is eight hours on duty, then a meal, recreation, personal time and eight hours' sleep.
Activity ScheduledActivity(int watch, int secondOfDay);

// ---- the outside (S9) ---------------------------------------------------------------------------
//
// The ship crosses a sector of beacons, FTL-fashion: a jump needs the warp drive and burns fuel, and
// what waits at the far end is fixed by the sector's seed. A hostile ship is fought with the ship's
// own systems -- what the phasers and shields deliver is what the simulation says they deliver, so a
// fight is won or lost in Engineering as much as at Tactical -- and what gets through lands on a
// deck and a system. With our shields down, the enemy sends boarders across.

enum BeaconKind : uint8_t { BEACON_EMPTY = 0, BEACON_HOSTILE, BEACON_DERELICT, BEACON_BORG, BEACON_KIND_COUNT };

struct Beacon {
	BeaconKind kind = BEACON_EMPTY;
	bool visited = false;
	std::vector<int> links;      // beacons one jump away
};

struct Enemy {
	bool present = false;
	bool borg = false;
	float hull = 0.0f;           // 0..1
	float shields = 0.0f;        // 0..1
	float firepower = 0.0f;      // shield strength it strips from us per minute, unopposed
	int boarders = 0;            // it will send these across once, when our shields are down
};

const int SECTOR_BEACONS = 12;
const float JUMP_DEUTERIUM = 0.01f;      // fraction of tankage per jump [inv]
const float JUMP_ANTIMATTER = 0.01f;
const float PHASER_MINUTES = 6.0f;       // full phasers strip an enemy's shields, or hole a bare hull, in this [inv]
const float TORPEDO_HULL = 0.34f;        // one torpedo on an unshielded hull [inv]
const float SHIELD_RECHARGE_MINUTES = 3.0f; // full shield output restores the shields from nothing in this: faster
                                            // than an ordinary raider strips them, slower than the Borg do [inv]
const float HIT_SYSTEM = 0.15f;          // what a minute's unopposed fire does to the system it lands on [inv]
const float HIT_HULL = 0.10f;            // ... and to that deck's hull
const float SALVAGE_PARTS = 25.0f;       // spare parts recovered from a derelict [inv]

// ---- the ship ---------------------------------------------------------------------------------

enum Alert : uint8_t { ALERT_GREEN = 0, ALERT_YELLOW, ALERT_RED };

struct Config {
	uint32_t seed = 2371;        // roster generation; the same seed is the same crew
	float dayScale = 60.0f;      // ship seconds per simulated second: 60 = a day in 24 minutes
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

	// the outside
	float shieldStrength = 1.0f; // what stands between enemy fire and the hull; 0 = hits land
	std::vector<Beacon> sector;
	int beacon = 0;              // where the ship is
	Enemy enemy;
	uint32_t hits = 0;           // how many hits have landed: decides, deterministically, where the next one does

	int Day() const { return static_cast<int>(clock / SECONDS_PER_DAY); }
	int SecondOfDay() const { return static_cast<int>(clock) % SECONDS_PER_DAY; }
	int Watch() const;           // the watch on duty now
	int PowerAvailable() const;
	int PowerAllocated() const;
	int CrewFit() const;
};

// A ship in the state Voyager is in with nothing wrong: every system intact, stores full, the
// roster generated from the seed.
Ship NewShip(const Config &cfg = Config());

// Advances the ship by `seconds` of simulated (not ship) time. Deterministic: the same ship and
// the same sequence of calls give the same result.
void Tick(Ship &s, float seconds);

// ---- what consoles and events do to it ------------------------------------------------------------

void SetAlert(Ship &s, Alert a);
void SetEnabled(Ship &s, SystemId id, bool on);
void SetPriority(Ship &s, SystemId id, int priority);
void SetSourceOnline(Ship &s, SourceId id, bool on);
void DamageSystem(Ship &s, SystemId id, float amount);
void DamageSource(Ship &s, SourceId id, float amount);
void BreachDeck(Ship &s, int deck, float amount);   // hull damage; atmosphere then vents by itself
void Repair(Ship &s, SystemId id, float amount);
void RepairDeck(Ship &s, int deck, float amount);   // seal the hull; life support then refills the deck

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
const float EXPOSURE_INJURES = 60.0f;        // seconds without air [inv]
const float EXPOSURE_KILLS = 300.0f;
const float AIRLESS = 0.25f;                 // a deck's atmosphere below this cannot be breathed [inv]
const int SICKBAY_BEDS = 6;                  // treated at once [inv]
const float TREATMENT_HOURS = 12.0f;         // per patient, with sickbay at full output [inv]
const int SICKBAY_DECK = 5;

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

void BoardBorg(Ship &s, int deck, int drones);
bool DeckAssimilated(const Ship &s, int deck);

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
bool Jump(Ship &s, int toBeacon);
// One torpedo at the enemy. False if there is no enemy, none left, or the launchers are not delivering.
bool FireTorpedo(Ship &s);
bool InCombat(const Ship &s);

// ---- persistence ------------------------------------------------------------------------------

const uint32_t SAVE_MAGIC = 0x50494853; // 'SHIP'
const uint16_t SAVE_VERSION = 5;   // 2: parts, exposure, recovery; 3: control, intruders; 4: the Borg; 5: the outside

std::vector<uint8_t> Pack(const Ship &s);
// False, leaving `s` untouched, on a truncated, foreign or newer record.
bool Unpack(const uint8_t *data, size_t len, Ship &s);

// Who is on a deck right now: indices into Ship::crew, in roster order. This is what decides which
// crew are embodied where the player is (S5); the dead and the assimilated are on no deck.
std::vector<int> CrewOnDeck(const Ship &s, int deck);

// A one-screen status report, for the console command and for tests' failure messages.
std::string Describe(const Ship &s);

} // namespace ship

#endif
