// g_ship.cpp -- hosts the ship simulation in the game module: ticks it with the game clock, keeps
// it in the save, carries it across the turbolift, and exposes it to the console.
//
// The simulation itself is ship_core, which knows nothing of the engine. This file is the only
// place the two meet. Consoles (S2 onward) and embodied crew (S5) reach the ship through Ship_Get().

#include "g_local.h"
#include "g_functions.h"

#include "ship_core.h"
#include "g_ship.h"
#include "g_scope.h"
#include "lwh_panel.h"
#include "g_crew.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

extern qboolean g_qbLoadTransition;

namespace {

const unsigned long SAVE_CHUNK = 0x53484950; // 'SHIP'
const char *const CARRY_FILE = "ship/carry.ship";

cvar_t *g_ship;         // 1 = the ship simulation runs
cvar_t *g_shipDayScale; // ship seconds per game second (60 = a day in 24 minutes; 1 = real time)
cvar_t *g_shipMode;     // 0 ironman (the game), 1 holodeck (saves allowed)
cvar_t *g_shipClock;    // 0 accelerated by g_shipDayScale, 1 real time, 2 wall clock (the ship lives on while away)
cvar_t *g_shipRole;     // 0 any post, 1 in command, 2 Munro
cvar_t *g_shipTest;     // harness: 1 = act, report, save, quit; 2 = report what a load restored, quit;
                        //          3 = operate the Engineering console; 4 = go to g_shipTestPos and photograph;
                        //          5 = ride the turbolift to every deck and report each arrival
cvar_t *g_shipTestPos;
cvar_t *g_shipTestPitch;
cvar_t *g_shipTestWatch; // "x y z": report whether the trigger centred there has fired

// A breach puzzle in progress at a console: which system it is for, and the puzzle itself.
int breachSystem = -1;
ship::Breach breach;

bool active = false;
bool tested = false;
ship::Ship vessel;
std::vector<uint8_t> pendingSave;
vec3_t glanceAngles = { 0, 0, 0 }; // where the glance test points the player, re-applied each frame
bool haveGlanceAim = false;
bool bodyApplied = false;          // S10: the player's model set from the crew record this map

void WriteFile( const char *path, const void *data, int len )
{
	fileHandle_t f = 0;
	gi.FS_FOpenFile( path, &f, FS_WRITE );
	if ( !f )
	{
		gi.Printf( S_COLOR_YELLOW"SHIP: could not write %s\n", path );
		return;
	}
	gi.FS_Write( data, len, f );
	gi.FS_FCloseFile( f );
}

// A system by number, or by the start of its name ("warp", "life").
int FindSystem( const char *arg )
{
	if ( !arg || !arg[0] ) return -1;
	char *end = NULL;
	const long n = strtol( arg, &end, 10 );
	if ( *end == '\0' ) return n >= 0 && n < ship::SYS_COUNT ? static_cast<int>( n ) : -1;
	for ( int i = 0; i < ship::SYS_COUNT; ++i )
		if ( !Q_stricmpn( ship::Spec( static_cast<ship::SystemId>( i ) ).name, arg, strlen( arg ) ) ) return i;
	return -1;
}

int FindSource( const char *arg )
{
	static const char *const names[ship::SRC_COUNT] = { "core", "impulse", "auxiliary", "batteries" };
	for ( int i = 0; i < ship::SRC_COUNT; ++i )
		if ( arg && arg[0] && !Q_stricmpn( names[i], arg, strlen( arg ) ) ) return i;
	return -1;
}

// A loss kind by the start of its name ("seal", "strip", "uninhab", "written"). -1 if not one of
// the four, so the caller can keep a default. (The names carry a space; the arg is one token.)
int FindLossKind( const char *arg )
{
	static const char *const names[ship::LOSS_KIND_COUNT] = { "sealed", "stripped", "uninhabitable", "written" };
	if ( !arg || !arg[0] ) return -1;
	for ( int i = 0; i < ship::LOSS_KIND_COUNT; ++i )
		if ( !Q_stricmpn( names[i], arg, strlen( arg ) ) ) return i;
	return -1;
}

void PrintStatus( void )
{
	const std::string text = ship::Describe( vessel );
	size_t at = 0;
	while ( at < text.size() )
	{//the engine's print buffer is small: a line at a time
		const size_t nl = text.find( '\n', at );
		gi.Printf( "SHIP: %s\n", text.substr( at, nl - at ).c_str() );
		if ( nl == std::string::npos ) break;
		at = nl + 1;
	}
}

void WriteReport( const char *name )
{
	const std::string text = ship::Describe( vessel );
	WriteFile( name, text.c_str(), static_cast<int>( text.size() ) );
	gi.Printf( "SHIP: wrote %s\n", name );
}

std::string Fmt( const char *fmt, ... ) __attribute__(( format( printf, 1, 2 ) ));
std::string Fmt( const char *fmt, ... )
{
	char buf[256];
	va_list ap;
	va_start( ap, fmt );
	vsnprintf( buf, sizeof( buf ), fmt, ap );
	va_end( ap );
	return buf;
}

// The UI is a separate module and cannot see the ship, so the ship's state is published as cvars
// for its screens to read (module/ui). Systems go out in the order power is given to them.
void Publish( void )
{
	static const char *const ALERTS[] = { "GREEN", "YELLOW", "RED" };
	static const char *const WATCH[] = { "ALPHA", "BETA", "GAMMA" };
	static const char *const SOURCES[ship::SRC_COUNT] = { "WARP CORE", "IMPULSE REACTORS", "AUXILIARY FUSION", "BATTERIES" };
	static const int CAPACITY[ship::SRC_COUNT] = { 1400, 250, 90, 60 };
	const int sod = vessel.SecondOfDay();

	gi.cvar_set( "lwh_ship_alert", Fmt( "%d", vessel.alert ).c_str() );
	// The power budget twice (owner ruling, 2026-10-07): what the plant delivers fresh, and what it
	// delivers now -- the power half of the navigation counter, and where the player watches it shrink
	// as the dilithium ages. Costs a string.
	// Committed against available, and the shortfall as a number: the FTL line the player works from
	// (docs/power-assignment.md, Task B). Automatic mode says so, and names its author.
	{
		const int shortfall = ship::PowerShortfall( vessel );
		// The FTL line (docs/power-assignment.md, Task B), kept short enough for one line at SMALLFONT:
		// committed against available, the shortfall as a number, the budget, and who is deciding.
		gi.cvar_set( "lwh_ship_header", Fmt( "DAY %d  %02d:%02d  %s  %s  COMMITTED %d OF %d%s  BUDGET %d/%d  %s",
			vessel.Day(), sod / 3600, sod % 3600 / 60, WATCH[vessel.Watch()], ALERTS[vessel.alert],
			ship::PowerCommitted( vessel ), vessel.PowerAvailable(),
			shortfall > 0 ? Fmt( "  SHORT %d", shortfall ).c_str() : "",
			vessel.PowerCapacityFresh(), vessel.PowerCapacityNow(),
			ship::PowerAuto( vessel ) ? "AUTOMATIC MODE" : "MANUAL" ).c_str() );
		gi.cvar_set( "lwh_ship_shortfall", Fmt( "%d", shortfall ).c_str() );
	}
	// The chief engineer's recommendation, with his reasoning, and the band delegations by name.
	{
		const ship::Recommendation rec = ship::RecommendAllocation( vessel );
		gi.cvar_set( "lwh_ship_recommend", rec.by >= 0 ? rec.reasoning.c_str() : "" );
		std::string grants;
		for ( int b = 0; b < ship::BAND_COUNT; ++b ) {
			const ship::BandGrant *g = ship::BandHolder( vessel, static_cast<ship::BudgetBand>( b ) );
			if ( !g ) continue;
			if ( !grants.empty() ) grants += "; ";
			grants += std::string( ship::BandName( static_cast<ship::BudgetBand>( g->band ) ) ) + " held by "
				+ vessel.crew[g->grantee].name;
		}
		gi.cvar_set( "lwh_ship_grants", grants.c_str() );
	}
	gi.cvar_set( "lwh_ship_stores", Fmt( "DEUTERIUM %.1f%%   ANTIMATTER %.1f%%   BATTERIES %.0f%%   TORPEDOES %d   CREW FIT %d OF %d",
		vessel.stores.deuterium * 100, vessel.stores.antimatter * 100, vessel.stores.batteries * 100, vessel.stores.torpedoes,
		vessel.CrewFit(), static_cast<int>( vessel.crew.size() ) ).c_str() );
	gi.cvar_set( "lwh_ship_dilithium", Fmt( "DILITHIUM %.0f%%   RANGE %d LY   QUALITY %.2f", vessel.dilithium * 100,
		ship::DilithiumRange( vessel ), vessel.crystalQuality ).c_str() );
	// The navigation counter (docs/navigation-counter.md): the crew's shared fact, on every console,
	// and the forecasts command alone sees. Read from the ship, never from a snapshot.
	{
		const ship::Navigation nav = ship::NavigationCounter( vessel );
		if ( !nav.warp )
			gi.cvar_set( "lwh_ship_nav", Fmt( "NAV  %d LY OUT   NO WARP: HOME STOPS GETTING CLOSER",
				static_cast<int>( nav.distanceLy + 0.5f ) ).c_str() );
		else
			gi.cvar_set( "lwh_ship_nav", Fmt( "NAV  %d LY OUT   %d YR NOMINAL   %d YR NOW   %+.1f SINCE LAST",
				static_cast<int>( nav.distanceLy + 0.5f ), static_cast<int>( nav.nominalYears + 0.5f ),
				static_cast<int>( nav.currentYears + 0.5f ), static_cast<double>( nav.changeYears ) ).c_str() );
		std::string forecast;
		if ( ship::PlayerMayCommand( vessel ) )
		{
			const std::vector<ship::NavCourse> routes = ship::NavigationForecasts( vessel );
			for ( size_t i = 0; i < routes.size(); ++i )
			{
				const ship::NavCourse &c = routes[i];
				forecast += Fmt( "%sBEACON %d %s: %s", i ? ";" : "", c.beacon,
					c.charted ? ship::BeaconKindName( static_cast<ship::BeaconKind>( c.kind ) ) : "UNCHARTED",
					c.years >= 0.0f ? Fmt( "%d YR", static_cast<int>( c.years + 0.5f ) ).c_str() : "NO WARP" );
			}
		}
		gi.cvar_set( "lwh_ship_forecast", forecast.c_str() );
	}
	gi.cvar_set( "lwh_ship_kit", Fmt( "AWAY KIT  TRICORDERS %d (%d%%)   PHASERS %d   EV SUITS %d",
		vessel.stores.tricorders, static_cast<int>( vessel.stores.tricorderCharge * 100 + 0.5f ),
		vessel.stores.phasers, vessel.stores.evSuits ).c_str() );
	for ( int i = 0; i < ship::SRC_COUNT; ++i )
	{
		const ship::Source &src = vessel.sources[i];
		gi.cvar_set( Fmt( "lwh_ship_src%d", i ).c_str(), Fmt( "%s|%d %d %d %d", SOURCES[i], src.output, CAPACITY[i],
			static_cast<int>( src.health * 100 + 0.5f ), src.online ? 1 : 0 ).c_str() );
	}
	// The outside, for Tactical and the Conn.
	gi.cvar_set( "lwh_ship_enemy", !vessel.enemy.present ? "" : Fmt( "%s   HULL %d%%   SHIELDS %d%%   WEAPONS %d%%   ENGINES %d%%   TARGETING %s%s",
		vessel.enemy.kind == ship::ENEMY_BORG_VESSEL ? "BORG VESSEL" : vessel.enemy.kind == ship::ENEMY_WARSHIP ? "WARSHIP" : "RAIDER",
		static_cast<int>( vessel.enemy.hull * 100 + 0.5f ), static_cast<int>( vessel.enemy.shields * 100 + 0.5f ),
		static_cast<int>( vessel.enemy.weapons * 100 + 0.5f ), static_cast<int>( vessel.enemy.engines * 100 + 0.5f ),
		ship::EnemySubsystemName( vessel.target ), vessel.enemy.hull <= 0.0f ? "   DESTROYED" : "" ).c_str() );
	gi.cvar_set( "lwh_ship_pursued", vessel.pursued ? Fmt( "%d", vessel.pursuitJumps ).c_str() : "" );
	gi.cvar_set( "lwh_ship_sector", Fmt( "SECTOR %d OF %d%s", vessel.sectorNumber + 1, ship::SECTORS_TO_CROSS, vessel.won ? "   HOME" : "" ).c_str() );
	gi.cvar_set( "lwh_ship_shields", Fmt( "%d", static_cast<int>( vessel.shieldStrength * 100 + 0.5f ) ).c_str() );
	// The phaser bank's setting (Tactical's standing decision, and the one that exists with no
	// contact): the setting, and what it asks of the power budget over its nominal demand.
	{
		const int base = ship::Spec( ship::SYS_PHASERS ).demand;
		const int pct = base ? ship::EffectiveDemand( vessel, ship::SYS_PHASERS ) * 100 / base : 100;
		gi.cvar_set( "lwh_ship_yield", Fmt( "PHASER BANK %s   POWER %d%%",
			ship::PhaserYieldName( ship::PhaserYieldOf( vessel ) ), pct ).c_str() );
		gi.cvar_set( "lwh_ship_yield_idx", Fmt( "%d", static_cast<int>( ship::PhaserYieldOf( vessel ) ) ).c_str() );
	}

	// Each station's information portfolio: what the post is expected to see, which is wider than the
	// systems the station operates (docs/scenario-atlas.md, "what you can see"; owner ruling,
	// 2026-10-07). Multi-content is EARNED by the job: only Tactical (the sensor picture and the comms
	// traffic) and Sickbay (the medical record and what the Doctor is running) carry a portfolio. The
	// other consoles carry one content type, well, and publish none. The names are the retail Virtual
	// Voyager station menus the game ships (docs/program-proposal-v2.md section 4, docs/gates.md G2);
	// the contents are this ship's own state. Sourced per portfolio in docs/lore-ledger.md.
	{
		const int charted = [&] { int n = 0; for ( const ship::Beacon &b : vessel.sector ) if ( b.visited || b.surveyed ) ++n; return n; }();
		int fires = 0, medlog = 0;
		for ( int d = 0; d < ship::DECKS; ++d ) if ( vessel.decks[d].fire > 0.0f ) ++fires;
		for ( const ship::LogEntry &e : vessel.log ) if ( e.scope == "sickbay" ) ++medlog;
		int dueVisit = 0;
		for ( const ship::CrewMember &c : vessel.crew ) if ( c.status == ship::CREW_FIT && c.fatigue > 0.7f ) ++dueVisit;
		gi.cvar_set( "lwh_ship_port1", Fmt( "SENSORS EXTERNAL: %d of %d beacons charted   INTERNAL: %d intruder(s), %d deck(s) afire   COMMS: hail / trade / distress",
			charted, static_cast<int>( vessel.sector.size() ), ship::Intruders( vessel ), fires ).c_str() );
		gi.cvar_set( "lwh_ship_port4", Fmt( "MEDICAL LOG: %d entry(ies)   DISEASE LIBRARY: the Doctor's references   VISIT ROSTER: %d due a check   RESEARCH: %s",
			medlog, dueVisit, ship::EMHActive( vessel ) ? "the Doctor's research running" : "none" ).c_str() );
		gi.cvar_set( "lwh_ship_port0", "" ); gi.cvar_set( "lwh_ship_port2", "" ); gi.cvar_set( "lwh_ship_port3", "" );
		// What each station may READ but not operate (owner ruling, 2026-10-07): the systems another
		// station operates that this one is cleared to see. The console draws them as readouts, and
		// its own as controls, so a read is never mistaken for a control. Empty where the post has no
		// read layer, which is most of them.
		for ( int st = 0; st < ship::STN_COUNT; ++st )
		{
			std::string reads;
			for ( int i = 0; i < ship::SYS_COUNT; ++i )
				if ( ship::StationReads( static_cast<ship::Station>( st ), static_cast<ship::SystemId>( i ) )
					&& ship::StationOf( static_cast<ship::SystemId>( i ) ) != st )
					reads += Fmt( "%s%s", reads.empty() ? "" : "|", ship::Spec( static_cast<ship::SystemId>( i ) ).name );
			gi.cvar_set( Fmt( "lwh_ship_reads%d", st ).c_str(), reads.c_str() );
		}
	}
	{
		static const char *const KINDS[] = { "EMPTY SPACE", "HOSTILE", "DERELICT", "BORG", "TRADER", "DISTRESS", "RESOURCE BELT", "PRE-WARP", "THE END" };
		std::string chart = Fmt( "SECTOR %d   AT BEACON %d OF %d.   JUMPS:", vessel.sectorNumber + 1, vessel.beacon, static_cast<int>( vessel.sector.size() ) - 1 );
		std::string links;
		for ( int l : vessel.sector[vessel.beacon].links )
		{
			const ship::Beacon &b = vessel.sector[l];
			chart += Fmt( "   [%d] %s%s", l, ( b.visited || b.surveyed ) ? KINDS[b.kind] : "UNCHARTED",
				b.surveyed && !b.visited ? "?" : "" );
			links += Fmt( "%d ", l );
		}
		if ( vessel.course >= 0 ) chart += Fmt( "   COURSE: BEACON %d", vessel.course );
		gi.cvar_set( "lwh_ship_chart", chart.c_str() );
		gi.cvar_set( "lwh_ship_links", links.c_str() );
	}
	{
		std::string aboard;
		for ( int d = 0; d < ship::DECKS; ++d )
		{
			if ( vessel.decks[d].intruders > 0.0f )
				aboard += Fmt( "%s DECK %d: %d   ", vessel.decks[d].borg ? "BORG" : "INTRUDERS", d + 1, static_cast<int>( std::ceil( vessel.decks[d].intruders ) ) );
			if ( vessel.decks[d].assimilated > 0.0f )
				aboard += Fmt( "DECK %d %d%% ASSIMILATED   ", d + 1, static_cast<int>( vessel.decks[d].assimilated * 100 + 0.5f ) );
			if ( vessel.decks[d].fire > 0.0f )
				aboard += Fmt( "FIRE DECK %d: %d%%%s   ", d + 1, static_cast<int>( vessel.decks[d].fire * 100 + 0.5f ),
					vessel.decks[d].firefighting ? " BEING FOUGHT" : "" );
		}
		gi.cvar_set( "lwh_ship_aboard", aboard.c_str() );
	}

	// The medical state and the crew's condition, for the Sickbay console (S6 and the morale gap).
	{
		int injured = 0, beds = 0, lost = 0, assimilated = 0, fit = 0;
		float morale = 0.0f, fatigue = 0.0f;
		for ( const ship::CrewMember &c : vessel.crew )
		{
			if ( c.status == ship::CREW_INJURED ) { ++injured; if ( c.underCare ) ++beds; }
			else if ( c.status == ship::CREW_ASSIMILATED ) ++assimilated;
			else if ( c.status == ship::CREW_DEAD ) ++lost;
			else { ++fit; morale += c.morale; fatigue += c.fatigue; }
		}
		if ( fit ) { morale /= fit; fatigue /= fit; }
		gi.cvar_set( "lwh_ship_medical", Fmt( "INJURED %d   BEDS %d   WAITING %d   LOST %d   ASSIMILATED %d   MORALE %d%%   FATIGUE %d%%   TRIAGE %s",
			injured, beds, injured - beds, lost, assimilated,
			static_cast<int>( morale * 100 + 0.5f ), static_cast<int>( fatigue * 100 + 0.5f ),
			vessel.orderTriage == 1 ? "RANK FIRST" : "WORST FIRST" ).c_str() );
		// The ward itself, one row per casualty in the order triage treats them, for the triage screen.
		std::string ward;
		const std::vector<int> rows = ship::Patients( vessel );
		for ( int i : rows )
		{
			const ship::CrewMember &c = vessel.crew[i];
			ward += Fmt( "%s|%d|%d|%d;", c.name.c_str(), static_cast<int>( c.severity * 100 + 0.5f ), c.underCare ? 1 : 0, i );
		}
		gi.cvar_set( "lwh_ship_ward", ward.c_str() );
		// The recovery window (docs/borg-incursion.md): the crew the Borg have begun to take, and
		// who can still be brought back. Sickbay's board acts on these by name.
		std::string captives;
		for ( int i = 0; i < static_cast<int>( vessel.crew.size() ); ++i )
		{
			const ship::CrewMember &c = vessel.crew[i];
			if ( c.status == ship::CREW_DEAD || c.status == ship::CREW_ASSIMILATED ) continue;
			if ( c.wounds > 0.0f && c.wounds < ship::RECOVERY_LIMIT )
				captives += Fmt( "%s|%d|%d;", c.name.c_str(), i, static_cast<int>( c.wounds * 100 + 0.5f ) );
		}
		gi.cvar_set( "lwh_ship_captives", captives.c_str() );
		gi.cvar_set( "lwh_ship_emh", vessel.emhActive ? "1" : "0" );
		gi.cvar_set( "lwh_ship_surgical", vessel.surgicalForceField ? "1" : "0" );
	}

	// The away mission and the course, for the Operations and Conn consoles.
	gi.cvar_set( "lwh_ship_away", Fmt( "%d", ship::AwayTeam( vessel ) ).c_str() );
	// The instrument: the transporter's condition, stated before the act (the condition gap). Read
	// by the Operations panel; it is a read of state and does not lie.
	gi.cvar_set( "lwh_ship_transporter", ship::TransporterConditionLine( vessel ).c_str() );
	gi.cvar_set( "lwh_ship_course", Fmt( "%d", vessel.course ).c_str() );
	gi.cvar_set( "lwh_ship_goal", Fmt( "%d", static_cast<int>( vessel.sector.size() ) - 1 ).c_str() );

	// The log, newest first, for the browsable screen: when|who|scope|what, one per entry.
	{
		std::string entries;
		int n = 0;
		for ( int i = static_cast<int>( vessel.log.size() ) - 1; i >= 0 && n < 24; --i, ++n )
		{
			const ship::LogEntry &e = vessel.log[i];
			const int day = static_cast<int>( e.time / ship::SECONDS_PER_DAY );
			const int sod = static_cast<int>( e.time ) % ship::SECONDS_PER_DAY;
			entries += Fmt( "D%d %02d:%02d|%s|%s|%s;", day, sod / 3600, sod % 3600 / 60, e.who.c_str(), e.scope.c_str(), e.what.c_str() );
		}
		gi.cvar_set( "lwh_ship_log", entries.c_str() );
	}

	// The personal log (docs/the-record-and-the-log.md): the player's own private store, and only
	// the player's. The model returns one person's entries and nobody else's (PersonalLog), so a
	// screen reading this cvar can only ever show what its owner wrote. Newest first, when|what.
	{
		std::string entries;
		if ( vessel.player >= 0 )
		{
			const std::vector<ship::PersonalLogEntry> own = ship::PersonalLog( vessel, vessel.player );
			int n = 0;
			for ( int i = static_cast<int>( own.size() ) - 1; i >= 0 && n < 24; --i, ++n )
			{
				const ship::PersonalLogEntry &e = own[i];
				const int day = static_cast<int>( e.time / ship::SECONDS_PER_DAY );
				const int sod = static_cast<int>( e.time ) % ship::SECONDS_PER_DAY;
				entries += Fmt( "D%d %02d:%02d|%s;", day, sod / 3600, sod % 3600 / 60, e.what.c_str() );
			}
		}
		gi.cvar_set( "lwh_ship_personal", entries.c_str() );
		gi.cvar_set( "lwh_ship_personal_who", vessel.player >= 0 ? vessel.crew[vessel.player].name.c_str() : "" );
	}

	// What the ship has given up (docs/damage-and-budgets.md), newest first, for the command console
	// and the panel: when|kind|what|who, one per entry. The count is separate so a screen can show it.
	{
		std::string entries;
		int n = 0;
		const std::vector<ship::LossEntry> &losses = ship::WriteOffs( vessel );
		for ( int i = static_cast<int>( losses.size() ) - 1; i >= 0 && n < 24; --i, ++n )
		{
			const ship::LossEntry &e = losses[i];
			const int day = static_cast<int>( e.time / ship::SECONDS_PER_DAY );
			const int sod = static_cast<int>( e.time ) % ship::SECONDS_PER_DAY;
			entries += Fmt( "D%d %02d:%02d|%s|%s|%s;", day, sod / 3600, sod % 3600 / 60,
				ship::LossKindName( e.kind ), e.what.c_str(), e.who.c_str() );
		}
		gi.cvar_set( "lwh_ship_losses", entries.c_str() );
		gi.cvar_set( "lwh_ship_loss_count", Fmt( "%d", static_cast<int>( losses.size() ) ).c_str() );
	}

	// Grief, for the personnel screen and the HUD: the quarters still sealed (name|deck) and the
	// wall of names -- the dead the ship carries with it (docs/morale.md).
	{
		std::string sealedTxt, wallTxt;
		const std::vector<ship::SealedQuarter> sealed = ship::SealedQuarters( vessel );
		for ( size_t i = 0; i < sealed.size(); ++i )
			sealedTxt += Fmt( "%s|deck%d;", vessel.crew[sealed[i].crew].name.c_str(), sealed[i].deck );
		const std::vector<std::string> wall = ship::WallOfNames( vessel );
		for ( size_t i = 0; i < wall.size(); ++i ) wallTxt += Fmt( "%s;", wall[i].c_str() );
		gi.cvar_set( "lwh_ship_sealed", sealedTxt.c_str() );
		gi.cvar_set( "lwh_ship_wall", wallTxt.c_str() );
	}

	// The endurance clocks, for Engineering (the air-and-endurance gap): a countdown wherever the air
	// is going, and the ship's time to dark on the stores it has.
	{
		std::string clocks;
		int going = 0, breachDeck = 0;
		float worstAir = 1.0f;
		for ( int d = 0; d < ship::DECKS; ++d )
		{
			const float air = ship::MinutesOfAir( vessel, d + 1 );
			if ( air >= 0.0f ) { clocks += Fmt( "DECK %d AIR %d MIN   ", d + 1, static_cast<int>( air + 0.5f ) ); ++going; }
			if ( vessel.decks[d].atmosphere < worstAir ) { worstAir = vessel.decks[d].atmosphere; breachDeck = d + 1; }
		}
		const float dark = ship::MinutesToDark( vessel );
		if ( dark >= 0.0f && dark < 1440.0f ) clocks += Fmt( "ENDURANCE %dH %02dM   ", static_cast<int>( dark ) / 60, static_cast<int>( dark ) % 60 );
		// Battery and auxiliary endurance as their own numbers (the air-and-endurance gap).
		const float battery = ship::EnduranceOf( vessel, ship::SRC_BATTERIES );
		const float aux = ship::EnduranceOf( vessel, ship::SRC_AUXILIARY );
		if ( battery >= 0.0f && battery < 1440.0f ) clocks += Fmt( "BATTERY %dH %02dM   ", static_cast<int>( battery ) / 60, static_cast<int>( battery ) % 60 );
		if ( aux >= 0.0f && aux < 1440.0f ) clocks += Fmt( "AUXILIARY %dH %02dM", static_cast<int>( aux ) / 60, static_cast<int>( aux ) % 60 );
		gi.cvar_set( "lwh_ship_clocks", clocks.c_str() );
		gi.cvar_set( "lwh_ship_clocks_alarm", going || ( dark >= 0.0f && dark < 60.0f ) ? "1" : "0" );
		// The compartment losing air and whether a field covers it, for the environmental-control control.
		gi.cvar_set( "lwh_ship_breach_deck", breachDeck && worstAir < 1.0f ? Fmt( "%d", breachDeck ).c_str() : "" );
		gi.cvar_set( "lwh_ship_breach_field", breachDeck && worstAir < 1.0f && vessel.decks[breachDeck - 1].forceField ? "1" : "0" );
	}

	// Standing orders and who the player is, for the command console and the personnel screen.
	{
		std::string orders;
		if ( vessel.orderRepairFirst >= 0 ) orders += Fmt( "SEE FIRST TO %s.   ", ship::Spec( static_cast<ship::SystemId>( vessel.orderRepairFirst ) ).name );
		if ( vessel.orderSecurityTo ) orders += Fmt( "GUARD ON DECK %d.   ", vessel.orderSecurityTo );
		if ( vessel.orderEvacuate ) orders += Fmt( "DECK %d EVACUATED.   ", vessel.orderEvacuate );
		orders += Fmt( "SICKBAY: %s.", vessel.orderTriage == 1 ? "RANK FIRST" : "WORST FIRST" );
		gi.cvar_set( "lwh_ship_orders", orders.c_str() );
		gi.cvar_set( "lwh_ship_triage", Fmt( "%d", vessel.orderTriage ).c_str() );
		static const char *const RANKS[] = { "Crewman", "Ensign", "Lt. j.g.", "Lieutenant", "Lt. Commander", "Commander", "Captain" };
		const bool chosen = vessel.player >= 0 && vessel.player < static_cast<int>( vessel.crew.size() );
		gi.cvar_set( "lwh_ship_player", chosen ? Fmt( "%s %s", RANKS[vessel.crew[vessel.player].rank], vessel.crew[vessel.player].name.c_str() ).c_str() : "" );
		gi.cvar_set( "lwh_ship_player_index", chosen ? Fmt( "%d", vessel.player ).c_str() : "" );
		gi.cvar_set( "lwh_ship_player_next", chosen && vessel.crew[vessel.player].rank < 6 ? RANKS[vessel.crew[vessel.player].rank + 1] : "" );
	}

	// Access (docs/access-and-authority.md): the override in progress, the lock-outs a senior officer
	// has imposed, and the grants for a shift. The lock-out names both hands, so the console that has
	// stopped answering can tell the person locked out by name.
	{
		const ship::Override &ov = ship::OverrideState( vessel );
		if ( ov.station < 0 )
			gi.cvar_set( "lwh_ship_override", "" );
		else if ( ov.active )
			gi.cvar_set( "lwh_ship_override", Fmt( "EMERGENCY OVERRIDE ACTIVE AT %s FOR %d MINUTES%s",
				ship::StationName( static_cast<ship::Station>( ov.station ) ),
				static_cast<int>( ( ov.expires - vessel.clock ) / 60.0 + 0.999 ),
				ov.solo ? " (ONE HAND)" : "" ).c_str() );
		else
			gi.cvar_set( "lwh_ship_override", Fmt( "OVERRIDE PENDING AT %s - TAKES IN %d MIN",
				ship::StationName( static_cast<ship::Station>( ov.station ) ),
				static_cast<int>( ( ov.readyAt - vessel.clock ) / 60.0 + 0.999 ) ).c_str() );
		std::string lockouts, delegations;
		for ( const ship::Lockout &l : ship::Lockouts( vessel ) )
			lockouts += Fmt( "%s|%s|%s;", ship::StationName( static_cast<ship::Station>( l.station ) ),
				vessel.crew[l.lockedBy].name.c_str(), vessel.crew[l.locked].name.c_str() );
		for ( const ship::Delegation &d : ship::Delegations( vessel ) )
			delegations += Fmt( "%s|%s|%s;", ship::StationName( static_cast<ship::Station>( d.station ) ),
				vessel.crew[d.grantor].name.c_str(), vessel.crew[d.grantee].name.c_str() );
		gi.cvar_set( "lwh_ship_lockouts", lockouts.c_str() );
		gi.cvar_set( "lwh_ship_delegations", delegations.c_str() );
	}
	// Whose acts these are (the person axis of the two-lock model): command's reports, orders and
	// priorities are refused to everyone else, and the screens say so by name.
	gi.cvar_set( "lwh_ship_may_command", ship::PlayerMayCommand( vessel ) ? "1" : "0" );

	// The month report editor (row 20): the open draft's lines with their edit state, the headline,
	// and the diff the record keeps. The screen sends its edits as `ship report ...` under the
	// person's clearance, so the editor is the same path a hand on the console drives.
	{
		const ship::MonthReport &r = ship::OpenReport( vessel );
		std::string lines;
		for ( size_t i = 0; i < r.lines.size(); ++i )
		{
			const ship::ReportLine &l = r.lines[i];
			const char status = l.struck ? 'S' : l.added ? '+' : ( l.text != l.draft ? 'E' : ' ' );
			// rows are separated by the unit separator, not ';': a report line's own text carries
			// semicolons (the headline is "home 75000 light years; 75 years nominal, ...").
			lines += Fmt( "%d|%c|%s|%s\x1f", static_cast<int>( i ), status, l.scope.c_str(), l.text.c_str() );
		}
		gi.cvar_set( "lwh_ship_report", lines.c_str() );
		gi.cvar_set( "lwh_ship_report_open", r.open ? "1" : "0" );
		gi.cvar_set( "lwh_ship_report_number", Fmt( "%d", r.number ).c_str() );
		// The diff, one line per change, newlines flattened for a cvar. The player's honesty instrument.
		std::string diff = ship::ReportDiff( r );
		for ( size_t i = 0; i < diff.size(); ++i ) if ( diff[i] == '\n' ) diff[i] = '\x1f';
		gi.cvar_set( "lwh_ship_report_diff", diff.c_str() );
	}

	// The job-queue board (row 19): the outstanding work as its own face, one row per job with its
	// kind, its target, how far and its place. Command sets the place, and the board is where it does.
	{
		const std::vector<ship::Job> &jobs = ship::Jobs( vessel );
		std::string rows;
		for ( size_t i = 0; i < jobs.size(); ++i )
		{
			const ship::Job &j = jobs[i];
			std::string what;
			if ( j.kind == ship::JOB_REPAIR ) what = ship::Spec( static_cast<ship::SystemId>( j.target ) ).name;
			else if ( j.kind == ship::JOB_BUILD ) what = Fmt( "%d spare parts", j.target );
			else what = Fmt( "deck %d", j.target );
			rows += Fmt( "%d|%s|%s|%d|%d;", static_cast<int>( i ), ship::JobKindName( j.kind ), what.c_str(),
				static_cast<int>( j.progress * 100.0f + 0.5f ), j.priority );
		}
		gi.cvar_set( "lwh_ship_jobs", rows.c_str() );
		gi.cvar_set( "lwh_ship_job_count", Fmt( "%d", static_cast<int>( jobs.size() ) ).c_str() );
	}

	// The beacon-choice block (row 21): what the outside offers here, so Operations (which speaks and
	// hails) can offer it by key rather than leaving it to a typed command. Empty in empty space with
	// nothing to do but go on or run.
	{
		const ship::Beacon &here = vessel.sector[vessel.beacon];
		std::string affords;
		switch ( here.kind )
		{
		case ship::BEACON_TRADER: affords = "AT A TRADER   L hail   M trade"; break;
		case ship::BEACON_DISTRESS: affords = "AT A DISTRESS CALL   A answer   L hail"; break;
		case ship::BEACON_DERELICT: affords = "AT A DERELICT   L hail   M trade"; break;
		case ship::BEACON_BELT: affords = "AT A RESOURCE BELT   L hail"; break;
		case ship::BEACON_PREWARP: affords = "AT A PRE-WARP WORLD   L hail"; break;
		case ship::BEACON_HOSTILE: affords = "A HOSTILE SHIP   L hail   (the Conn may run)"; break;
		default: affords = "EMPTY SPACE   L hail   (the Conn may run)"; break;
		}
		gi.cvar_set( "lwh_ship_beacon", affords.c_str() );
	}

	// The survey (row 22): what a tricorder can be spent on here. The site the ship is at, and the
	// ship's own compartments -- one shared charge, so choosing is the decision. The reading comes back
	// in lwh_ship_survey_reading, set by the scan commands.
	{
		const ship::Beacon &here = vessel.sector[vessel.beacon];
		std::string targets = Fmt( "SITE|THE SITE AT BEACON %d|%s;", vessel.beacon,
			( here.visited || here.surveyed ) ? "already charted" : "unscanned" );
		for ( int d = 0; d < ship::DECKS; ++d )
		{
			const ship::Deck &dk = vessel.decks[d];
			const char *state = dk.atmosphere < ship::AIRLESS ? "NO AIR" : dk.fire > 0.0f ? "AFIRE"
				: dk.hull < 1.0f ? "HULL BREACHED" : "nominal";
			targets += Fmt( "DECK|DECK %d|%s;", d + 1, state );
		}
		gi.cvar_set( "lwh_ship_survey", targets.c_str() );
	}

	// The workable chart (row 23): every beacon in the sector, its kind if charted, and its links --
	// the map command works from, with the forecast (lwh_ship_forecast) keyed by beacon.
	{
		std::string map;
		for ( size_t i = 0; i < vessel.sector.size(); ++i )
		{
			const ship::Beacon &b = vessel.sector[i];
			const char *kind = ( b.visited || b.surveyed ) ? ship::BeaconKindName( b.kind ) : "UNCHARTED";
			map += Fmt( "%d|%d|%s|%s;", static_cast<int>( i ), static_cast<int>( i ) == vessel.beacon ? 1 : 0, kind,
				( b.visited || b.surveyed ) ? "known" : "unknown" );
		}
		gi.cvar_set( "lwh_ship_sector_map", map.c_str() );
	}

	int order[ship::SYS_COUNT];
	for ( int i = 0; i < ship::SYS_COUNT; ++i ) order[i] = i;
	std::stable_sort( order, order + ship::SYS_COUNT, []( int a, int b ) { return vessel.systems[a].priority < vessel.systems[b].priority; } );
	for ( int k = 0; k < ship::SYS_COUNT; ++k )
	{
		const ship::System &sys = vessel.systems[order[k]];
		const ship::SystemSpec &spec = ship::Spec( static_cast<ship::SystemId>( order[k] ) );
		// The row carries, after the ten an older console reads: the share a person set (11) and the
		// provenance of the commitment (12), so the console can show the FTL number beside the control
		// and say who decided (docs/power-assignment.md, Task B).
		gi.cvar_set( Fmt( "lwh_ship_sys%d", k ).c_str(), Fmt( "%s|%d %d %d %d %d %d %d %d %d %d %d %d %d", spec.name, sys.allocated, spec.demand,
			static_cast<int>( sys.health * 100 + 0.5f ), static_cast<int>( sys.output * 100 + 0.5f ), sys.manned, spec.crewNeeded,
			sys.enabled ? 1 : 0, sys.priority, ship::StationOf( static_cast<ship::SystemId>( order[k] ) ),
			static_cast<int>( sys.control * 100 + 0.5f ),
			ship::AllocationPercent( vessel, static_cast<ship::SystemId>( order[k] ) ),
			static_cast<int>( ship::AllocationSource( vessel, static_cast<ship::SystemId>( order[k] ) ) ),
			static_cast<int>( ship::BandOf( static_cast<ship::SystemId>( order[k] ) ) ) ).c_str() );
	}
	// Who the console would delegate a band to: the department head a recommendation comes from.
	gi.cvar_set( "lwh_ship_chief", Fmt( "%d", ship::DepartmentHead( vessel, ship::DEPT_ENGINEERING ) ).c_str() );
}

// Reports the trigger whose centre is at g_shipTestWatch: a trigger that has fired is waiting
// (nextthink set) before it can fire again.
void ReportWatchedTrigger( const char *when )
{
	vec3_t at;
	if ( sscanf( g_shipTestWatch->string, "%f %f %f", &at[0], &at[1], &at[2] ) != 3 ) return;
	for ( int i = 1; i < globals.num_entities; ++i )
	{
		const gentity_t *e = &g_entities[i];
		if ( !e->inuse || !e->classname || Q_stricmpn( e->classname, "trigger", 7 ) ) continue;
		if ( DistanceSquared( e->currentOrigin, at ) > 4.0f ) continue;
		gi.Printf( "SHIP: trigger %d (%s, %s) %s: %s; bounds %s .. %s\n", i, e->classname, e->model && e->model[0] == '*' ? "brush model" : "box",
			when, e->nextthink > level.time ? "FIRED, waiting to re-arm" : "armed, not fired", vtos( e->absmin ), vtos( e->absmax ) );
		return;
	}
	gi.Printf( "SHIP: no trigger found at %s\n", g_shipTestWatch->string );
}

// Find a usable panel the player can stand in front of and stand there, aiming at it. A panel's
// facing is its thinnest axis; stand a pace out along it, on the floor, and look back at it. Used
// by the glance test (13) and the viewscreen test (30). Returns the panel, or NULL; sets
// glanceAngles/haveGlanceAim so the frame loop can hold the aim.
gentity_t *StandAtPanel( void )
{
	gentity_t *chosen = NULL;
	vec3_t stand = { 0, 0, 0 }, dir = { 0, 0, 0 };
	for ( int i = 1; i < globals.num_entities && !chosen; ++i )
	{
		gentity_t *e = &g_entities[i];
		if ( !e->inuse || !e->classname || Q_stricmp( e->classname, "func_usable" ) ) continue;
		if ( !e->model || e->model[0] != '*' ) continue;
		vec3_t span;
		for ( int a = 0; a < 3; ++a ) span[a] = e->absmax[a] - e->absmin[a];
		if ( span[0] <= 0.0f || span[1] <= 0.0f || span[2] <= 0.0f ) continue;
		int axis = span[0] <= span[1] ? 0 : 1;
		if ( span[2] < span[axis] ) axis = 2;      // a floor panel: not a standing console
		if ( axis == 2 ) continue;
		vec3_t center;
		for ( int a = 0; a < 3; ++a ) center[a] = ( e->absmin[a] + e->absmax[a] ) * 0.5f;
		for ( int sign = 1; sign >= -1 && !chosen; sign -= 2 )
		{
			vec3_t at = { center[0], center[1], center[2] }, eye;
			at[axis] = center[axis] + sign * ( span[axis] * 0.5f + 40.0f );
			eye[0] = at[0]; eye[1] = at[1]; eye[2] = center[2];
			trace_t tr;
			gi.trace( &tr, eye, vec3_origin, vec3_origin, center, 0, MASK_OPAQUE | CONTENTS_BODY | CONTENTS_ITEM | CONTENTS_CORPSE );
			if ( tr.entityNum != i ) continue;
			vec3_t from = { at[0], at[1], e->absmax[2] + 64.0f }, to = { at[0], at[1], e->absmin[2] - 128.0f };
			gi.trace( &tr, from, vec3_origin, vec3_origin, to, 0, MASK_PLAYERSOLID );
			if ( tr.fraction == 1.0f ) continue; // no floor: cannot stand here
			VectorCopy( at, stand );
			stand[2] = tr.endpos[2] + 1.0f;
			VectorSubtract( center, stand, dir );
			chosen = e;
		}
	}
	if ( !chosen ) return NULL;
	vec3_t angles;
	vectoangles( dir, angles );
	TeleportPlayer( &g_entities[0], stand, angles, 0 );
	VectorCopy( angles, glanceAngles );
	haveGlanceAim = true;
	return chosen;
}

void ApplyPlayerBody( void ); // S10: defined below, after the harness section

// Stand the player at a named marker, a little above it. Used by the environment tests (54, 55).
// Returns false, and does nothing, if the map has no such marker.
bool TeleportPlayerTo( const char *name, const char *label )
{
	gentity_t *at = NULL;
	for ( int i = 1; i < globals.num_entities && !at; ++i )
	{
		gentity_t *e = &g_entities[i];
		if ( e->inuse && e->targetname && !Q_stricmp( e->targetname, name ) ) at = e;
	}
	if ( !at ) return false;
	vec3_t origin, angles = { 0, 0, 0 };
	VectorCopy( at->currentOrigin, origin );
	origin[2] += 24.0f;
	TeleportPlayer( &g_entities[0], origin, angles, 0 );
	VectorCopy( angles, glanceAngles );
	haveGlanceAim = true;
	if ( label ) gi.Printf( "SHIP: %s: standing at %s\n", label, vtos( origin ) );
	return true;
}

// The harness (scripts/s2-check.sh): do to the ship what a console would, then prove the result
// is what the save holds.
void RunTest( void )
{
	if ( g_shipTest->integer == 3 )
	{//operate the Engineering console the way a hand would, a step at a time, then photograph it
		static const struct { int ms; const char *command; } STEPS[] = {
			{ 3000, "ui_lwh_engineering\n" },
			{ 3500, "lwh_eng_key 3\n" },                                        // condition red
			{ 4000, "lwh_eng_key down\n" }, { 4200, "lwh_eng_key down\n" },     // to the third system
			{ 4400, "lwh_eng_key enter\n" },                                    // switch it off
			{ 5000, "lwh_eng_key up\n" }, { 5200, "lwh_eng_key up\n" },         // back to the first
			{ 5400, "lwh_eng_key right\n" },                                    // and demote it one place
			{ 6500, "screenshot lwh_engineering\n" },
		};
		static size_t step = 0;
		if ( level.time < 1000 ) step = 0;
		while ( step < sizeof( STEPS ) / sizeof( STEPS[0] ) && level.time >= STEPS[step].ms )
		{
			gi.Printf( "SHIP: console test t=%d: %s", level.time, STEPS[step].command );
			gi.SendConsoleCommand( STEPS[step++].command );
		}
		if ( tested || level.time < 8000 ) return;
		tested = true;
		gi.Printf( "SHIP: life support priority %d, structural integrity priority %d\n",
			vessel.systems[ship::SYS_LIFE_SUPPORT].priority, vessel.systems[ship::SYS_STRUCTURAL_INTEGRITY].priority );
		WriteReport( "ship/operated.txt" );
		gi.SendConsoleCommand( "quit\n" );
		return;
	}
	if ( g_shipTest->integer == 71 )
	{//the allocation console (docs/power-assignment.md, Task B): set a share by key, turn automatic
	 //mode on and off, and photograph the FTL surface. Everything is a key a hand would press.
		static const struct { int ms; const char *command; } STEPS[] = {
			{ 3000, "ui_lwh_engineering\n" },
			{ 3600, "lwh_eng_key -\n" },   // the selected system (life support, first in the order) to 90%
			{ 3900, "lwh_eng_key -\n" },   // ... and to 80%
			{ 4200, "lwh_eng_key e\n" },   // automatic mode on: the ladder is the policy
			{ 4600, "screenshot lwh_power\n" },
			{ 5600, "lwh_eng_key e\n" },   // ... and off again: what the player set stands
		};
		static size_t step = 0;
		if ( level.time < 1000 ) step = 0;
		while ( step < sizeof( STEPS ) / sizeof( STEPS[0] ) && level.time >= STEPS[step].ms )
		{
			gi.Printf( "SHIP: power console test t=%d: %s", level.time, STEPS[step].command );
			gi.SendConsoleCommand( STEPS[step++].command );
		}
		if ( tested || level.time < 7000 ) return;
		tested = true;
		gi.Printf( "SHIP: allocation test: life support %d%%, automatic mode %d, committed %d of %d, short %d\n",
			ship::AllocationPercent( vessel, ship::SYS_LIFE_SUPPORT ), ship::PowerAuto( vessel ) ? 1 : 0,
			ship::PowerCommitted( vessel ), vessel.PowerAvailable(), ship::PowerShortfall( vessel ) );
		WriteReport( "ship/power.txt" );
		gi.SendConsoleCommand( "quit\n" );
		return;
	}
	if ( g_shipTest->integer == 80 )
	{//the meeting (docs/staff-meetings.md): a brief emitted in the normal case at the watch change,
	 //and an allocation decision the simulation applies end to end. No model is referenced anywhere.
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 2500 )
		{//advance the clock to the next watch change: the normal-case trigger, no drama required
			vessel.clock = ( std::floor( vessel.clock / ship::SECONDS_PER_WATCH ) + 1.0 ) * ship::SECONDS_PER_WATCH;
			ship::Tick( vessel, 1.0f );
			const std::vector<ship::MeetingBrief> &q = ship::PendingMeetings( vessel );
			gi.Printf( "SHIP: meeting test: %d brief(s) queued at the watch change\n", static_cast<int>( q.size() ) );
			if ( !q.empty() )
			{
				const ship::MeetingBrief &mb = q.front();
				gi.Printf( "SHIP: meeting test: %s: %s\n", ship::MeetingKindName( mb.kind ), mb.decision.c_str() );
				gi.Printf( "SHIP: meeting test: trigger: %s; present %d, options %d\n", mb.trigger.c_str(), mb.presentCount, mb.optionCount );
				if ( mb.optionCount > 0 )
					gi.Printf( "SHIP: meeting test: first option: %s [cost: %s]\n", mb.options[0].label.c_str(), mb.options[0].cost.c_str() );
				const ship::MeetingSkeleton &sk = ship::AuthoredSkeleton( mb.kind );
				if ( sk.outcomeCount > 0 && sk.outcomes[0].lineCount > 0 )
				{
					ship::SynthesisRequest req;
					const bool ok = ship::LineToSynthesis( sk.outcomes[0].lines[0], req );
					gi.Printf( "SHIP: meeting test: line -> synthesis: delivery %s, known %d, exaggeration %.2f\n",
						ship::DeliveryName( req.delivery ), ok ? 1 : 0, req.exaggeration );
				}
			}
			step = 1;
		}
		if ( step == 1 && level.time >= 3200 )
		{//the allocation seam, end to end: a person in the room decides the holodecks/shields option
			ship::SetAlert( vessel, ship::ALERT_YELLOW ); // the shields are not suppressed
			const ship::MeetingBrief mb = ship::BuildBrief( vessel, ship::MEET_ALLOCATION );
			const ship::MeetingSkeleton &sk = ship::AuthoredSkeleton( ship::MEET_ALLOCATION );
			int outcome = -1;
			for ( int i = 0; i < sk.outcomeCount; ++i )
				if ( sk.outcomes[i].option.effect == ship::EFFECT_SET_ALLOCATION ) { outcome = i; break; }
			const int decider = vessel.player >= 0 ? vessel.player : 0;
			const bool applied = outcome >= 0 && ship::ApplyMeetingOutcome( vessel, mb, outcome, decider, false );
			gi.Printf( "SHIP: meeting test: allocation outcome applied=%d; holodecks %d%%, shields %d%% (provenance %s)\n",
				applied ? 1 : 0, ship::AllocationPercent( vessel, ship::SYS_HOLODECKS ),
				ship::AllocationPercent( vessel, ship::SYS_SHIELDS ), ship::AllocationProvenance( vessel, ship::SYS_HOLODECKS ).c_str() );
			const bool asShip = ship::ApplyMeetingOutcome( vessel, mb, outcome, -1, true );
			gi.Printf( "SHIP: meeting test: set as the ship (automatic) applied=%d (0 = refused, as designed)\n", asShip ? 1 : 0 );
			WriteReport( "ship/meeting.txt" );
			gi.SendConsoleCommand( "quit\n" );
			step = 2;
		}
		return;
	}
	if ( g_shipTest->integer == 11 )
	{//report for duty through the personnel screen, be refused an order, then command and give three
		static const struct { int ms; const char *command; } STEPS[] = {
			{ 3000, "ui_lwh_character\n" },
			{ 3300, "lwh_new_key n\nlwh_new_key right\nlwh_new_key right\nlwh_new_key up\n" }, // Okoro, security, ensign
			{ 3800, "lwh_new_key enter\n" },
			{ 4300, "ui_lwh_command\n" },
			{ 4600, "lwh_cmd_key g\n" },                    // an ensign posts no guards
			{ 5600, "screenshot lwh_personnel\n" },
			{ 6000, "set g_shipRole 1\nship role\n" },
			{ 6500, "lwh_cmd_key down\nlwh_cmd_key down\nlwh_cmd_key r\n" },   // third system in the power order first
			{ 6800, "lwh_cmd_key right\nlwh_cmd_key right\nlwh_cmd_key right\nlwh_cmd_key g\n" }, // guard to deck 4
			{ 7100, "lwh_cmd_key right\nlwh_cmd_key v\n" },                     // evacuate deck 5
			{ 8200, "screenshot lwh_command\n" },
		};
		static size_t step = 0;
		if ( level.time < 1000 ) step = 0;
		while ( step < sizeof( STEPS ) / sizeof( STEPS[0] ) && level.time >= STEPS[step].ms )
			gi.SendConsoleCommand( STEPS[step++].command );
		if ( tested || level.time < 9000 ) return;
		tested = true;
		gi.Printf( "SHIP: command test: player %s, orders repair %d guard %d evacuate %d\n",
			vessel.player >= 0 ? vessel.crew[vessel.player].name.c_str() : "nobody", vessel.orderRepairFirst, vessel.orderSecurityTo, vessel.orderEvacuate );
		gi.SendConsoleCommand( "quit\n" );
		return;
	}
	if ( g_shipTest->integer == 10 )
	{//ironman: a save by hand, a load by hand, and the one save the game is allowed
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 3000 ) { gi.SendConsoleCommand( "save byhand\n" ); step = 1; }
		if ( step == 1 && level.time >= 4000 ) { gi.SendConsoleCommand( "load auto\n" ); step = 2; }
		if ( step == 2 && level.time >= 5000 ) { gi.SendConsoleCommand( "save ironman\n" ); step = 3; }
		if ( step == 3 && level.time >= 7000 ) { gi.Printf( "SHIP: ironman test done at t=%d\n", level.time ); gi.SendConsoleCommand( "quit\n" ); step = 4; }
		return;
	}
	if ( g_shipTest->integer == 9 )
	{//the consoles of S7 and S9: win a hijacked system back through the breach screen, jump, and fire
		static int step = 0, nextKeyMs = 0;
		static std::vector<std::string> keys;
		if ( level.time < 1000 ) { step = 0; keys.clear(); }
		auto at = [&]( int n, int ms ) { return step == n && level.time >= ms; };
		if ( at( 0, 3000 ) ) { vessel.systems[ship::SYS_SENSORS].control = 0.2f; gi.SendConsoleCommand( "ui_ops\n" ); step = 1; }
		if ( at( 1, 3400 ) ) { gi.SendConsoleCommand( "lwh_eng_key down\nlwh_eng_key down\nlwh_eng_key down\n" ); step = 2; }
		if ( at( 2, 4000 ) ) { gi.SendConsoleCommand( "lwh_eng_key h\n" ); step = 3; }
		if ( at( 3, 4600 ) )
		{//solve the puzzle the ship published, by search, and turn the path into the operator's key presses
			gi.SendConsoleCommand( "screenshot lwh_breach\n" );
			std::vector<int> best, path;
			float bestScore = -1.0f;
			std::vector<bool> used( breach.grid.size(), false );
			struct Search {
				const ship::Breach &b; std::vector<int> &best, &path; float &bestScore; std::vector<bool> &used;
				void Go() {
					if ( bestScore >= 1.0f ) return;
					if ( !path.empty() ) { const float sc = ship::BreachScore( b, path ); if ( sc > bestScore ) { bestScore = sc; best = path; } }
					if ( static_cast<int>( path.size() ) >= b.buffer ) return;
					for ( int k = 0; k < b.size; ++k ) {
						const int cell = path.empty() ? k : path.size() % 2 == 1 ? k * b.size + path.back() % b.size : ( path.back() / b.size ) * b.size + k;
						if ( used[cell] ) continue;
						used[cell] = true; path.push_back( cell ); Go(); path.pop_back(); used[cell] = false;
					}
				}
			} search{ breach, best, path, bestScore, used };
			if ( breachSystem >= 0 ) search.Go();
			for ( size_t i = 0; i < best.size(); ++i )
			{
				const int line = i == 0 ? best[0] : i % 2 == 1 ? best[i] / breach.size : best[i] % breach.size;
				for ( int k = 0; k < line; ++k ) keys.push_back( "right" );
				keys.push_back( "enter" );
			}
			if ( static_cast<int>( best.size() ) < breach.buffer ) keys.push_back( "s" );
			gi.Printf( "SHIP: console test: breach puzzle solvable to %d%% in %d picks\n", static_cast<int>( bestScore * 100 + 0.5f ), static_cast<int>( best.size() ) );
			nextKeyMs = level.time;
			step = 4;
		}
		if ( step == 4 && level.time >= nextKeyMs )
		{
			if ( keys.empty() ) step = 5;
			else { gi.SendConsoleCommand( Fmt( "lwh_eng_key %s\n", keys.front().c_str() ).c_str() ); keys.erase( keys.begin() ); nextKeyMs = level.time + 100; }
		}
		if ( at( 5, nextKeyMs + 800 ) )
		{
			gi.Printf( "SHIP: console test: sensors control %d%%, %s\n", static_cast<int>( vessel.systems[ship::SYS_SENSORS].control * 100 + 0.5f ),
				ship::Hijacked( vessel, ship::SYS_SENSORS ) ? "still hijacked" : "ours again" );
			// the first beacon the Conn lists is a contact, so the check exercises the fight deterministically
			if ( vessel.sector.size() > 1 ) { vessel.sector[1].kind = ship::BEACON_HOSTILE; vessel.sector[1].visited = false; }
			gi.SendConsoleCommand( "ui_navigation\n" );
			nextKeyMs = level.time;
			step = 6;
		}
		if ( at( 6, nextKeyMs + 500 ) ) { gi.SendConsoleCommand( "lwh_eng_key j\n" ); step = 7; }
		if ( at( 7, nextKeyMs + 1200 ) ) { gi.SendConsoleCommand( "ui_tactical\n" ); step = 8; }
		if ( at( 8, nextKeyMs + 1600 ) ) { gi.SendConsoleCommand( "lwh_eng_key 3\n" ); step = 9; }
		if ( at( 9, nextKeyMs + 3000 ) ) { gi.SendConsoleCommand( "lwh_eng_key f\n" ); step = 10; }
		if ( at( 10, nextKeyMs + 4000 ) ) { gi.SendConsoleCommand( "screenshot lwh_combat\n" ); step = 11; }
		if ( at( 11, nextKeyMs + 5000 ) )
		{
			gi.Printf( "SHIP: console test: at beacon %d, %s, condition %d, torpedoes %d, enemy shields %d%%\n", vessel.beacon,
				ship::InCombat( vessel ) ? "in combat" : "no contact", vessel.alert, vessel.stores.torpedoes,
				static_cast<int>( vessel.enemy.shields * 100 + 0.5f ) );
			gi.SendConsoleCommand( "quit\n" );
			step = 12;
		}
		return;
	}
	if ( g_shipTest->integer == 8 )
	{//board the player's deck, then kill one of the bodies and see the ship's count follow
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		const int deck = static_cast<int>( gi.cvar( "g_crewDeck", "0", 0 )->value );
		if ( step == 0 && level.time >= 3000 )
		{
			if ( g_shipTestPos->string[0] == 'b' ) ship::BoardBorg( vessel, deck, 3 ); else ship::Board( vessel, deck, 3 );
			step = 1;
		}
		if ( step == 1 && level.time >= 9000 )
		{
			int bodies = 0;
			gentity_t *victim = NULL;
			for ( int i = 1; i < globals.num_entities; ++i )
			{
				gentity_t *e = &g_entities[i];
				if ( !e->inuse || !e->client || !e->script_targetname || Q_stricmpn( e->script_targetname, "lwh_boarder_", 12 ) ) continue;
				++bodies;
				victim = e;
				gi.Printf( "SHIP: boarder body %s: type %s, team %d, hostile to team %d\n", e->script_targetname,
					e->NPC_type ? e->NPC_type : "?", e->client->playerTeam, e->client->enemyTeam );
			}
			gi.Printf( "SHIP: boarding test: %d bodies for %d boarders the ship counts\n", bodies, ship::Intruders( vessel ) );
			if ( victim ) G_Damage( victim, &g_entities[0], &g_entities[0], NULL, victim->currentOrigin, 10000, 0, MOD_UNKNOWN );
			step = 2;
		}
		if ( step == 2 && level.time >= 13000 )
		{
			gi.Printf( "SHIP: boarding test: after one was killed the ship counts %d\n", ship::Intruders( vessel ) );
			gi.SendConsoleCommand( "quit\n" );
			step = 3;
		}
		return;
	}
	if ( g_shipTest->integer == 7 )
	{//the player as a security ensign: what each console lets them do
		static const struct { int ms; const char *command; } STEPS[] = {
			{ 3000, "ship character Reyes 2 1\n" },   // department 2 security, rank 1 ensign
			{ 3500, "ship as 0 off sensors\n" },      // Engineering: not their station
			{ 4000, "ship as 1 off phasers\n" },      // Tactical: theirs
			{ 4500, "ship as 1 alert red\n" },        // but an ensign does not call the alert
			{ 5000, "ship as 1 off sensors\n" },      // and sensors are not Tactical's
		};
		static size_t step = 0;
		if ( level.time < 1000 ) step = 0;
		while ( step < sizeof( STEPS ) / sizeof( STEPS[0] ) && level.time >= STEPS[step].ms )
			gi.SendConsoleCommand( STEPS[step++].command );
		if ( tested || level.time < 6500 ) return;
		tested = true;
		gi.Printf( "SHIP: clearance test: sensors %s, phasers %s, condition %d\n", vessel.systems[ship::SYS_SENSORS].enabled ? "on" : "off",
			vessel.systems[ship::SYS_PHASERS].enabled ? "on" : "off", vessel.alert );
		gi.SendConsoleCommand( "quit\n" );
		return;
	}
	if ( g_shipTest->integer == 6 )
	{//walk up to the Tactical panel as the retail game would have it opened, and operate it
		static const struct { int ms; const char *command; } STEPS[] = {
			{ 3000, "ui_tactical\n" },            // the retail panel's own command
			{ 3500, "lwh_eng_key 3\n" },          // condition red
			{ 4500, "lwh_eng_key down\n" },       // second of Tactical's systems
			{ 4700, "lwh_eng_key enter\n" },      // switch it off
			{ 5500, "lwh_eng_key right\n" },      // priority is Engineering's to set: must do nothing here
			{ 6500, "screenshot lwh_tactical\n" },
		};
		static size_t step = 0;
		if ( level.time < 1000 ) step = 0;
		while ( step < sizeof( STEPS ) / sizeof( STEPS[0] ) && level.time >= STEPS[step].ms )
			gi.SendConsoleCommand( STEPS[step++].command );
		if ( tested || level.time < 8000 ) return;
		tested = true;
		gi.Printf( "SHIP: phasers priority %d\n", vessel.systems[ship::SYS_PHASERS].priority );
		WriteReport( "ship/tactical.txt" );
		gi.SendConsoleCommand( "quit\n" );
		return;
	}
	if ( g_shipTest->integer == 13 )
	{//stand at a panel and photograph the ship's live state drawn at it (S4's "glance")
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 3000 )
		{
			gentity_t *chosen = StandAtPanel();
			if ( chosen )
				gi.Printf( "SHIP: glance test: standing at %s looking at %s (%s)\n", vtos( g_entities[0].client->ps.origin ),
					chosen->targetname ? chosen->targetname : "?", chosen->classname );
			else
				gi.Printf( "SHIP: glance test: no usable panel with a standable side found\n" );
			step = 1;
		}
		// Pmove overwrites the view from the client's input each frame; hold it on the panel
		if ( haveGlanceAim && g_entities[0].client )
			VectorCopy( glanceAngles, g_entities[0].client->ps.viewangles );
		if ( step == 1 && level.time >= 5000 ) { gi.SendConsoleCommand( "screenshot lwh_glance\n" ); step = 2; }
		if ( step == 2 && level.time >= 6500 ) { gi.SendConsoleCommand( "quit\n" ); step = 3; }
		return;
	}
	if ( g_shipTest->integer == 30 )
	{//stand at a panel with a contact and photograph the live viewscreen beside it (S9's "see")
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 3000 )
		{
			gentity_t *chosen = StandAtPanel();
			// a contact of a known, damaged state, so the image drawn is deterministic
			vessel.enemy.present = true;
			vessel.enemy.kind = ship::ENEMY_WARSHIP;
			vessel.enemy.hull = 0.62f;
			vessel.enemy.shields = 0.35f;
			vessel.target = ship::TARGET_WEAPONS;
			if ( chosen )
				gi.Printf( "SHIP: viewscreen test: standing at %s with a contact hull 62%% shields 35%%\n",
					chosen->targetname ? chosen->targetname : "?" );
			else
				gi.Printf( "SHIP: viewscreen test: no usable panel with a standable side found\n" );
			step = 1;
		}
		if ( haveGlanceAim && g_entities[0].client )
			VectorCopy( glanceAngles, g_entities[0].client->ps.viewangles );
		if ( step == 1 && level.time >= 5500 ) { gi.SendConsoleCommand( "screenshot lwh_viewscreen\n" ); step = 2; }
		if ( step == 2 && level.time >= 7000 ) { gi.SendConsoleCommand( "quit\n" ); step = 3; }
		return;
	}
	if ( g_shipTest->integer == 32 )
	{//the adopted RPG-X target_shaderremap: it spawned, and firing it toggles the shader (Harvest B)
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 2500 )
		{
			gentity_t *e = G_Find( NULL, FOFS( classname ), (char *)"target_shaderremap" );
			if ( e )
			{
				gi.Printf( "SHIP: shaderremap test: found target_shaderremap at %s\n", vtos( e->s.origin ) );
				GEntity_UseFunc( e, e, e );   // swap falsename -> truename
				GEntity_UseFunc( e, e, e );   // and back again
			}
			else
				gi.Printf( "SHIP: shaderremap test: no target_shaderremap in the map\n" );
			step = 1;
		}
		if ( step == 1 && level.time >= 4000 ) { gi.SendConsoleCommand( "quit\n" ); step = 2; }
		return;
	}
	if ( g_shipTest->integer == 33 )
	{//S10: the player's character is the body they walk in -- set a named character and apply it
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 2500 )
		{
			const int who = ship::CreateCharacter( vessel, "Tuvok Test", ship::DEPT_SECURITY, 2 );
			if ( who >= 0 ) { vessel.crew[who].type = "tuvok"; ApplyPlayerBody(); }
			else gi.Printf( "SHIP: body test: no character could be created\n" );
			step = 1;
		}
		if ( step == 1 && level.time >= 4500 ) { gi.SendConsoleCommand( "screenshot lwh_body\n" ); step = 2; }
		if ( step == 2 && level.time >= 6000 ) { gi.SendConsoleCommand( "quit\n" ); step = 3; }
		return;
	}
	if ( g_shipTest->integer == 34 )
	{//S6: damage is visible -- a damaged system sparks where it is worked on the player's deck
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 2500 )
		{
			gentity_t *arrival = NULL;
			for ( int i = 1; i < globals.num_entities && !arrival; ++i )
			{
				gentity_t *e = &g_entities[i];
				if ( e->inuse && e->targetname && !Q_stricmp( e->targetname, "d12_arrival" ) ) arrival = e;
			}
			if ( arrival )
			{
				vec3_t at, angles = { 0, 0, 0 };
				VectorCopy( arrival->currentOrigin, at );
				at[2] += 24.0f;
				TeleportPlayer( &g_entities[0], at, angles, 0 );
				VectorCopy( angles, glanceAngles );
				haveGlanceAim = true;
				gi.Printf( "SHIP: damage test: standing on deck 12 at %s\n", vtos( at ) );
			}
			else gi.Printf( "SHIP: damage test: no d12_arrival on this map\n" );
			ship::DamageSystem( vessel, ship::SYS_LIFE_SUPPORT, 0.4f );
			gi.Printf( "SHIP: damage test: life support at %d%% health\n", static_cast<int>( vessel.systems[ship::SYS_LIFE_SUPPORT].health * 100 + 0.5f ) );
			step = 1;
		}
		if ( haveGlanceAim && g_entities[0].client )
			VectorCopy( glanceAngles, g_entities[0].client->ps.viewangles );
		if ( step == 1 && level.time >= 5000 ) { gi.SendConsoleCommand( "screenshot lwh_damage\n" ); step = 2; }
		if ( step == 2 && level.time >= 6500 ) { gi.SendConsoleCommand( "quit\n" ); step = 3; }
		return;
	}
	if ( g_shipTest->integer == 35 )
	{//S6: a burning deck is seen to burn -- smoke and flame across the deck the player is on
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 2500 )
		{
			gentity_t *arrival = NULL;
			for ( int i = 1; i < globals.num_entities && !arrival; ++i )
			{
				gentity_t *e = &g_entities[i];
				if ( e->inuse && e->targetname && !Q_stricmp( e->targetname, "d12_arrival" ) ) arrival = e;
			}
			if ( arrival )
			{
				vec3_t at, angles = { 0, 0, 0 };
				VectorCopy( arrival->currentOrigin, at );
				at[2] += 24.0f;
				TeleportPlayer( &g_entities[0], at, angles, 0 );
				VectorCopy( angles, glanceAngles );
				haveGlanceAim = true;
				gi.Printf( "SHIP: fire test: standing on deck 12 at %s\n", vtos( at ) );
			}
			else gi.Printf( "SHIP: fire test: no d12_arrival on this map\n" );
			ship::IgniteDeck( vessel, 12, 0.6f );
			gi.Printf( "SHIP: fire test: deck 12 alight at %d%%\n", static_cast<int>( vessel.decks[11].fire * 100 + 0.5f ) );
			step = 1;
		}
		if ( haveGlanceAim && g_entities[0].client )
			VectorCopy( glanceAngles, g_entities[0].client->ps.viewangles );
		if ( step == 1 && level.time >= 5000 ) { gi.SendConsoleCommand( "screenshot lwh_fire\n" ); step = 2; }
		if ( step == 2 && level.time >= 6500 ) { gi.SendConsoleCommand( "quit\n" ); step = 3; }
		return;
	}
	if ( g_shipTest->integer == 36 )
	{//S10: the command console confirms a field promotion (the ship's answer reaches the UI cvar)
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 2500 )
		{
			ship::SetRole( vessel, ship::ROLE_IN_COMMAND );
			const int who = ship::CreateCharacter( vessel, "Reyes", ship::DEPT_COMMAND, 0 );
			if ( who < 0 ) gi.Printf( "SHIP: promote test: no character could be created\n" );
			else { gi.Printf( "SHIP: promote test: Reyes is crew number %d at rank %d\n", who, vessel.crew[who].rank ); gi.SendConsoleCommand( Fmt( "ship promote %d\n", who ).c_str() ); }
			step = 1;
		}
		if ( step == 1 && level.time >= 4500 )
		{
			gi.Printf( "SHIP: promote test: the ship says \"%s\"\n", gi.cvar( "lwh_ship_promote", "", 0 )->string );
			step = 2;
		}
		if ( step == 2 && level.time >= 6000 ) { gi.SendConsoleCommand( "quit\n" ); step = 3; }
		return;
	}
	if ( g_shipTest->integer == 37 )
	{//memory: a crew member speaks a line drawn from what they remember, when addressed
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 3500 )
		{
			ship::Ship *s = Ship_Get();
			gentity_t *who = NULL;
			int idx = -1;
			if ( s )
				for ( int n = 1; n < globals.num_entities && !who; ++n )
				{
					gentity_t *e = &g_entities[n];
					if ( !e->inuse || !e->fullName ) continue;
					for ( size_t k = 0; k < s->crew.size(); ++k )
						if ( s->crew[k].name == e->fullName ) { idx = static_cast<int>( k ); who = e; break; }
				}
			if ( s && who && idx >= 0 )
			{
				ship::Remember( *s, idx, ship::MEM_RESCUE, s->player, ship::MEM_SAW, 0.9f );
				gi.Printf( "SHIP: speech test: %s carries a rescue; addressing them\n", s->crew[idx].name.c_str() );
				GEntity_UseFunc( who, &g_entities[0], &g_entities[0] );
			}
			else gi.Printf( "SHIP: speech test: no embodied crew found\n" );
			step = 1;
		}
		if ( step == 1 && level.time >= 5500 ) { gi.SendConsoleCommand( "quit\n" ); step = 2; }
		return;
	}
	if ( g_shipTest->integer == 38 )
	{//dilithium: the crystal, recomposition, and the journey's scoreboard, driven at the console
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 2500 ) { gi.SendConsoleCommand( "ship dilithium\n" ); step = 1; }
		if ( step == 1 && level.time >= 3200 ) { vessel.dilithium = 0.5f; gi.SendConsoleCommand( "ship recomposite\n" ); step = 2; }
		if ( step == 2 && level.time >= 4500 ) { gi.SendConsoleCommand( "ship dilithium\n" ); step = 3; }
		if ( step == 3 && level.time >= 5200 ) { gi.SendConsoleCommand( "ship finddilithium\n" ); step = 4; }
		if ( step == 4 && level.time >= 6500 ) { gi.SendConsoleCommand( "quit\n" ); step = 5; }
		return;
	}
	if ( g_shipTest->integer == 39 )
	{//shuttles: the bay, a launch with a manifest, and a recall, at the console
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 2500 ) { gi.SendConsoleCommand( "ship shuttle\n" ); step = 1; }
		if ( step == 1 && level.time >= 3200 ) { gi.SendConsoleCommand( "ship launch 1 3 5 6\n" ); step = 2; }
		if ( step == 2 && level.time >= 4200 ) { gi.SendConsoleCommand( "ship shuttle\n" ); step = 3; }
		if ( step == 3 && level.time >= 5200 ) { gi.SendConsoleCommand( "ship shuttledock 1\n" ); step = 4; }
		if ( step == 4 && level.time >= 6000 ) { gi.SendConsoleCommand( "quit\n" ); step = 5; }
		return;
	}
	if ( g_shipTest->integer == 40 )
	{//the incursion's field: board a deck, let security answer, and report who holds it
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 2500 ) { gi.SendConsoleCommand( "ship board 4 2 raider\n" ); step = 1; }
		if ( step == 1 && level.time >= 4000 ) { gi.SendConsoleCommand( "ship controller\n" ); step = 2; }
		if ( step == 2 && level.time >= 9000 ) { gi.SendConsoleCommand( "ship controller\n" ); step = 3; }
		if ( step == 3 && level.time >= 10500 ) { gi.SendConsoleCommand( "quit\n" ); step = 4; }
		return;
	}
	if ( g_shipTest->integer == 41 )
	{//the counter-play kit: a Borg whose adaptation is broken by rotating the phaser modulation
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 2500 )
		{
			vessel.enemy.present = true; vessel.enemy.kind = ship::ENEMY_BORG_VESSEL; vessel.enemy.borg = true;
			vessel.enemy.hull = 1.0f; vessel.enemy.shields = 0.0f; vessel.enemy.adaptation = 0.6f;
			gi.Printf( "SHIP: counterplay test: adaptation %d%% before remodulation\n", static_cast<int>( vessel.enemy.adaptation * 100 + 0.5f ) );
			gi.SendConsoleCommand( "ship remodulate\n" );
			step = 1;
		}
		if ( step == 1 && level.time >= 4000 ) { gi.SendConsoleCommand( "quit\n" ); step = 2; }
		return;
	}
	if ( g_shipTest->integer == 42 )
	{//de-assimilation: a crew member in the window is recovered, with lasting residue
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 2500 )
		{
			if ( vessel.crew.size() > 10 )
			{
				vessel.crew[10].wounds = 0.5f;
				vessel.crew[10].status = ship::CREW_INJURED;
				gi.Printf( "SHIP: recover test: %s is 50%% assimilated\n", vessel.crew[10].name.c_str() );
				gi.SendConsoleCommand( "ship recover 10\n" );
			}
			step = 1;
		}
		if ( step == 1 && level.time >= 4000 ) { gi.SendConsoleCommand( "quit\n" ); step = 2; }
		return;
	}
	if ( g_shipTest->integer == 43 )
	{//force fields rated: a level-10 field holds boarders and drains under their pressure
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 2000 ) { gi.SendConsoleCommand( "ship order evacuate 4\n" ); step = 1; }
		if ( step == 1 && level.time >= 3000 ) { gi.SendConsoleCommand( "ship board 4 2 raider\n" ); step = 2; }
		if ( step == 2 && level.time >= 3800 ) { gi.SendConsoleCommand( "ship field 4 10\n" ); step = 3; }
		if ( step == 3 && level.time >= 5500 ) { gi.SendConsoleCommand( "ship controller\n" ); step = 4; }
		if ( step == 4 && level.time >= 7000 ) { gi.SendConsoleCommand( "quit\n" ); step = 5; }
		return;
	}
	if ( g_shipTest->integer == 44 )
	{//probes: the safe way to look at something hostile
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 2500 ) { gi.SendConsoleCommand( "ship alert red\n" ); step = 1; }
		if ( step == 1 && level.time >= 3200 ) { gi.SendConsoleCommand( "ship probe 2\n" ); step = 2; }
		if ( step == 2 && level.time >= 4500 ) { gi.SendConsoleCommand( "quit\n" ); step = 3; }
		return;
	}
	if ( g_shipTest->integer == 45 )
	{//a phenomenon: scan its hidden attributes one at a time, then answer it correctly
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 2000 ) { gi.SendConsoleCommand( "ship alert red\n" ); step = 1; }
		if ( step == 1 && level.time >= 3000 )
		{
			int ph = -1;
			for ( int i = 0; i < static_cast<int>( vessel.sector.size() ); ++i ) if ( vessel.sector[i].phenomenon ) ph = i;
			if ( ph >= 0 )
			{
				vessel.beacon = ph;
				vessel.enemy = ship::Enemy();
				gi.Printf( "SHIP: phenomenon test: at beacon %d, the correct response is %d\n", ph, vessel.sector[ph].phenomTruth );
				gi.SendConsoleCommand( "ship scan\n" );
				gi.SendConsoleCommand( "ship scan\n" );
				gi.SendConsoleCommand( "ship scan\n" );
				gi.SendConsoleCommand( Fmt( "ship study %d\n", vessel.sector[ph].phenomTruth ).c_str() );
			}
			else gi.Printf( "SHIP: phenomenon test: none in this sector\n" );
			step = 2;
		}
		if ( step == 2 && level.time >= 5000 ) { gi.SendConsoleCommand( "quit\n" ); step = 3; }
		return;
	}
	if ( g_shipTest->integer == 46 )
	{//the security squad: sent to retake a deck the boarders hold
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 2000 ) { gi.SendConsoleCommand( "ship order evacuate 4\n" ); step = 1; }
		if ( step == 1 && level.time >= 3000 ) { gi.SendConsoleCommand( "ship board 4 2 raider\n" ); step = 2; }
		if ( step == 2 && level.time >= 3800 ) { gi.SendConsoleCommand( "ship advance 4\n" ); step = 3; }
		if ( step == 3 && level.time >= 22000 ) { gi.SendConsoleCommand( "ship controller\n" ); step = 4; }
		if ( step == 4 && level.time >= 24000 ) { gi.SendConsoleCommand( "quit\n" ); step = 5; }
		return;
	}
	if ( g_shipTest->integer == 47 )
	{//the warp core cascade: a damaged core overheats, then command shuts it down to stop the breach
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 2000 ) { ship::DamageSystem( vessel, ship::SYS_WARP_DRIVE, 0.8f ); step = 1; }
		if ( step == 1 && level.time >= 4000 ) { gi.SendConsoleCommand( "ship core\n" ); step = 2; }
		if ( step == 2 && level.time >= 5500 ) { gi.SendConsoleCommand( "ship core shutdown\n" ); step = 3; }
		if ( step == 3 && level.time >= 7000 ) { gi.SendConsoleCommand( "ship core\n" ); step = 4; }
		if ( step == 4 && level.time >= 8500 ) { gi.SendConsoleCommand( "quit\n" ); step = 5; }
		return;
	}
	if ( g_shipTest->integer == 50 )
	{//condition sets the odds, and stress sets the severity: the transporter, worked end to end. The
	 //instrument states the condition before the act, a nominal beam never mangles, a degraded one
	 //under load does, and the log carries the chain.
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 2000 )
		{
			vessel.shieldStrength = 0.0f;
			vessel.sector[vessel.beacon].phenomenon = false;
			vessel.sector[vessel.beacon].kind = ship::BEACON_EMPTY;
			ship::SetAlert( vessel, ship::ALERT_GREEN );
			gi.Printf( "SHIP: RISK instrument nominal: %s\n", ship::TransporterConditionLine( vessel ).c_str() );
			int anomalies = 0, beams = 0;
			for ( int i = 0; i < 50; ++i )
			{
				std::string note;
				if ( !ship::TransportAway( vessel, 3, &note ) ) break;
				++beams; if ( !note.empty() ) ++anomalies;
				if ( !ship::TransportBack( vessel, &note ) ) break;
				if ( !note.empty() ) ++anomalies;
			}
			gi.Printf( "SHIP: RISK nominal: %d beams, %d anomalies (top tenth)\n", beams, anomalies );
			step = 1;
		}
		if ( step == 1 && level.time >= 3200 )
		{//the console states the condition before the act: the command a hand at the panel sends
			gi.SendConsoleCommand( "ship transport 3\n" );
			step = 2;
		}
		if ( step == 2 && level.time >= 3800 ) { gi.SendConsoleCommand( "ship recall\n" ); step = 3; }
		if ( step == 3 && level.time >= 4400 )
		{
			ship::DamageSystem( vessel, ship::SYS_TRANSPORTERS, 0.6f );
			ship::SetAlert( vessel, ship::ALERT_RED );
			gi.Printf( "SHIP: RISK instrument degraded: %s\n", ship::TransporterConditionLine( vessel ).c_str() );
			std::string last;
			int anomalies = 0;
			for ( int i = 0; i < 200 && anomalies == 0; ++i )
			{
				vessel.systems[ship::SYS_TRANSPORTERS].health = 0.4f;
				vessel.systems[ship::SYS_TRANSPORTERS].output = 0.4f;
				std::string note;
				if ( !ship::TransportAway( vessel, 3, &note ) ) break;
				if ( !note.empty() ) { ++anomalies; last = note; }
				ship::TransportBack( vessel, &note );
				if ( !note.empty() ) { ++anomalies; last = note; }
			}
			gi.Printf( "SHIP: RISK degraded under red alert: %d anomaly, last: %s\n", anomalies, last.c_str() );
			step = 4;
		}
		if ( step == 4 && level.time >= 6000 )
		{
			gi.SendConsoleCommand( "ship log 12\n" );
			step = 5;
		}
		if ( step == 5 && level.time >= 7500 ) { gi.SendConsoleCommand( "quit\n" ); step = 6; }
		return;
	}
	if ( g_shipTest->integer == 51 )
	{//the three clocks and the two exits (docs/ship-model.md): ironman refuses to suspend; a sleep
	 //converges with a played interval; a standing order fires across a sleep; holodeck may suspend
	 static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 2000 )
		{
			gi.Printf( "SHIP: clock test: ironman may suspend %d (must be 0)\n", ship::MaySuspend( vessel.cfg ) ? 1 : 0 );
			gi.SendConsoleCommand( "ship suspend\n" ); // refused in ironman
			// Sleeping in one jump, in steps, and a played interval must all arrive together.
			ship::Ship one = vessel, steps = vessel, played = vessel;
			const double interval = 6.0 * 3600.0;
			played.cfg.clockMode = ship::CLOCK_REAL_TIME; // a played second is a second
			ship::Sleep( one, interval );
			for ( int m = 0; m < 12; ++m ) ship::Sleep( steps, interval / 12.0 );
			ship::Tick( played, static_cast<float>( interval ) );
			const int same = ( std::fabs( one.clock - steps.clock ) < 1e-6 && std::fabs( one.clock - played.clock ) < 1.0 ) ? 1 : 0;
			gi.Printf( "SHIP: clock test: sleep 6h -> day %d; twelve steps -> day %d; played -> day %d; identical %d\n",
				one.Day(), steps.Day(), played.Day(), same );
			step = 1;
		}
		if ( step == 1 && level.time >= 3200 )
		{//a standing order while the player sleeps, and something for the night's work to show
			ship::SetRole( vessel, ship::ROLE_IN_COMMAND );
			ship::DamageSystem( vessel, ship::SYS_SENSORS, 0.6f );
			ship::OrderEvacuate( vessel, 11 );
			gi.SendConsoleCommand( "ship sleep 6\n" );
			step = 2;
		}
		if ( step == 2 && level.time >= 6200 )
		{
			gi.Printf( "SHIP: clock test: after a 6h sleep, deck 11 crew %d (must be 0), sensors %.2f, log entries %d\n",
				static_cast<int>( ship::CrewOnDeck( vessel, 11 ).size() ), vessel.systems[ship::SYS_SENSORS].health,
				static_cast<int>( vessel.log.size() ) );
			vessel.cfg.mode = ship::MODE_HOLODECK; // the guard is tied to the play mode
			gi.SendConsoleCommand( "ship suspend\n" );
			step = 3;
		}
		if ( step == 3 && level.time >= 7200 )
		{
			gi.Printf( "SHIP: clock test: holodeck suspend -> left standing %d (must be 1)\n", ship::LeftStanding( vessel ) ? 1 : 0 );
			gi.SendConsoleCommand( "quit\n" );
			step = 4;
		}
		return;
	}
	if ( g_shipTest->integer == 52 )
	{//the navigation counter (docs/navigation-counter.md): how far home, how long, and the arrow
	 //moving. The console query is the same read on every deck; wrecking the crystal and jumping
	 //closer must both show on the spot, and command sees the forecasts.
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 2000 )
		{
			ship::SetRole( vessel, ship::ROLE_IN_COMMAND ); // so the console answers command's forecasts
			gi.Printf( "SHIP: nav test: the counter before anything\n" );
			gi.SendConsoleCommand( "ship nav\n" );
			step = 1;
		}
		if ( step == 1 && level.time >= 3200 )
		{
			vessel.dilithium = 0.2f; // wreck the crystal: the estimate must worsen on the spot
			gi.Printf( "SHIP: nav test: the crystal wrecked\n" );
			gi.SendConsoleCommand( "ship nav\n" );
			step = 2;
		}
		if ( step == 2 && level.time >= 4400 ) { gi.SendConsoleCommand( "ship jump 1\n" ); step = 3; }
		if ( step == 3 && level.time >= 5400 )
		{
			gi.Printf( "SHIP: nav test: after a jump toward home\n" );
			gi.SendConsoleCommand( "ship nav\n" );
			step = 4;
		}
		if ( step == 4 && level.time >= 6800 ) { gi.SendConsoleCommand( "quit\n" ); step = 5; }
		return;
	}
	if ( g_shipTest->integer == 14 )
	{//the other station panels open the working console, and Sickbay shows the medical state
		static const struct { int ms; const char *command; } STEPS[] = {
			{ 3000, "genericmenu transporter\n" },   // the transporter panel: worked from Operations
			{ 4000, "screenshot lwh_ops\n" },
			{ 5200, "ui_lwh_station 4\n" },          // the Sickbay console
			{ 6400, "screenshot lwh_sickbay\n" },
		};
		static size_t step = 0;
		if ( level.time < 1000 ) step = 0;
		while ( step < sizeof( STEPS ) / sizeof( STEPS[0] ) && level.time >= STEPS[step].ms )
			gi.SendConsoleCommand( STEPS[step++].command );
		if ( tested || level.time < 8000 ) return;
		tested = true;
		gi.Printf( "SHIP: medical state: %s\n", gi.cvar( "lwh_ship_medical", "", 0 )->string );
		gi.SendConsoleCommand( "quit\n" );
		return;
	}
	if ( g_shipTest->integer == 28 )
	{//a generic generated-deck blockout photograph: the deck number in g_shipTestPos, the arrival
		//point dNN_arrival on the merged ship, the screenshot lwh_deckNN (used by scripts/deck-check.sh)
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 3000 )
		{
			const int deck = atoi( g_shipTestPos->string );
			char name[32];
			Com_sprintf( name, sizeof( name ), "d%02d_arrival", deck );
			gentity_t *arrival = NULL;
			for ( int i = 1; i < globals.num_entities && !arrival; ++i )
			{
				gentity_t *e = &g_entities[i];
				if ( e->inuse && e->targetname && !Q_stricmp( e->targetname, name ) ) arrival = e;
			}
			if ( arrival )
			{
				vec3_t at, angles = { 0, 0, 0 };
				VectorCopy( arrival->currentOrigin, at );
				at[2] += 24.0f;
				TeleportPlayer( &g_entities[0], at, angles, 0 );
				VectorCopy( angles, glanceAngles );
				haveGlanceAim = true;
				gi.Printf( "SHIP: deck %d blockout: standing at %s\n", deck, vtos( at ) );
			}
			else gi.Printf( "SHIP: deck %d blockout: no %s on this map\n", deck, name );
			step = 1;
		}
		if ( haveGlanceAim && g_entities[0].client )
			VectorCopy( glanceAngles, g_entities[0].client->ps.viewangles );
		if ( step == 1 && level.time >= 5000 )
		{
			gi.SendConsoleCommand( Fmt( "screenshot lwh_deck%02d\n", atoi( g_shipTestPos->string ) ).c_str() );
			step = 2;
		}
		if ( step == 2 && level.time >= 6500 ) { gi.SendConsoleCommand( "quit\n" ); step = 3; }
		return;
	}
	if ( g_shipTest->integer == 27 )
	{//the deck 13 re-dress: stand on the life-support plant at the turbolift arrival, then up on
		//the catwalk at the plant panel, and photograph it (docs/locations/deck13-life-support.brief.md)
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 3000 )
		{
			gentity_t *arrival = NULL;
			for ( int i = 1; i < globals.num_entities && !arrival; ++i )
			{
				gentity_t *e = &g_entities[i];
				if ( e->inuse && e->targetname && !Q_stricmp( e->targetname, "d13_arrival" ) ) arrival = e;
			}
			if ( arrival )
			{
				vec3_t at, angles = { 0, 180, 0 };  // face into the room, toward the plant
				VectorCopy( arrival->currentOrigin, at );
				at[2] += 24.0f;
				TeleportPlayer( &g_entities[0], at, angles, 0 );
				VectorCopy( angles, glanceAngles );
				haveGlanceAim = true;
				gi.Printf( "SHIP: deck 13 room: standing at %s\n", vtos( at ) );
			}
			else gi.Printf( "SHIP: deck 13 room: no d13_arrival on this map\n" );
			step = 1;
		}
		if ( haveGlanceAim && g_entities[0].client )
			VectorCopy( glanceAngles, g_entities[0].client->ps.viewangles );
		if ( step == 1 && level.time >= 4000 ) { gi.SendConsoleCommand( "screenshot lwh_deck13\n" ); step = 2; }
		if ( step == 2 && level.time >= 5000 )
		{//the catwalk: the post the brief puts on it, reached and stood on, looking down the island
			if ( TeleportPlayerTo( "lwh_plant_post", "deck 13 room: at the catwalk" ) )
			{
				vec3_t look = { 0, 270, 0 };
				VectorCopy( look, glanceAngles );
				haveGlanceAim = true;
			}
			step = 3;
		}
		if ( step == 3 && level.time >= 6000 ) { gi.SendConsoleCommand( "screenshot lwh_deck13_catwalk\n" ); step = 4; }
		if ( step == 4 && level.time >= 7400 ) { gi.SendConsoleCommand( "quit\n" ); step = 5; }
		return;
	}
	if ( g_shipTest->integer == 26 )
	{//the deck 12 blockout: stand on the generated environmental-control deck and photograph it, for
		//the owner to approve before detail (docs/locations/deck12-environmental-control.brief.md)
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 3000 )
		{
			gentity_t *arrival = NULL;
			for ( int i = 1; i < globals.num_entities && !arrival; ++i )
			{
				gentity_t *e = &g_entities[i];
				if ( e->inuse && e->targetname && !Q_stricmp( e->targetname, "d12_arrival" ) ) arrival = e;
			}
			if ( arrival )
			{
				vec3_t at, angles = { 0, 0, 0 };
				VectorCopy( arrival->currentOrigin, at );
				at[2] += 24.0f;
				TeleportPlayer( &g_entities[0], at, angles, 0 );
				VectorCopy( angles, glanceAngles );
				haveGlanceAim = true;
				gi.Printf( "SHIP: deck 12 room: standing at %s\n", vtos( at ) );
				angles[1] = 180;                 // face into the re-dressed room, toward the plant
				VectorCopy( angles, glanceAngles );
			}
			else gi.Printf( "SHIP: deck 12 room: no d12_arrival on this map\n" );
			step = 1;
		}
		if ( haveGlanceAim && g_entities[0].client )
			VectorCopy( glanceAngles, g_entities[0].client->ps.viewangles );
		if ( step == 1 && level.time >= 5000 ) { gi.SendConsoleCommand( "screenshot lwh_deck12\n" ); step = 2; }
		if ( step == 2 && level.time >= 6500 ) { gi.SendConsoleCommand( "quit\n" ); step = 3; }
		return;
	}
	if ( g_shipTest->integer == 53 )
	{//grief in the game (docs/morale.md): a death seals the quarters the player can read, puts the
		//name on the wall, and the funeral opens the quarters and leaves a positive mark. Needs
		//`map voyager`. The rules are unit-tested; this shows the player-facing path.
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 3000 )
		{
			ship::KillCrew( vessel, 10, "a hull breach on deck 9" );
			gi.Printf( "SHIP: grief: %s is dead, %d quarters sealed, %d on the wall\n",
				vessel.crew[10].name.c_str(), static_cast<int>( ship::SealedQuarters( vessel ).size() ),
				static_cast<int>( ship::WallOfNames( vessel ).size() ) );
			gi.SendConsoleCommand( "ship wall\n" );
			step = 1;
		}
		if ( step == 1 && level.time >= 4000 ) { gi.SendConsoleCommand( "set g_shipRole 1\nship role\n" ); step = 2; }
		if ( step == 2 && level.time >= 5000 ) { gi.SendConsoleCommand( "ship funeral\n" ); step = 3; }
		if ( step == 3 && level.time >= 6000 )
		{
			gi.Printf( "SHIP: grief: after the funeral %d quarters sealed, %d on the wall, crew 11 funeral mark %d\n",
				static_cast<int>( ship::SealedQuarters( vessel ).size() ),
				static_cast<int>( ship::WallOfNames( vessel ).size() ),
				ship::Recall( vessel.crew[11], ship::MEM_FUNERAL ) ? 1 : 0 );
			gi.SendConsoleCommand( "ship wall\n" );
			step = 4;
		}
		if ( step == 4 && level.time >= 7000 ) { gi.SendConsoleCommand( "quit\n" ); step = 5; }
		return;
	}
	if ( g_shipTest->integer == 25 )
	{//runtime asset replacement on the merged ship: stand on a deck (g_shipTestPos, default 6),
		//assimilate it, and photograph the surfaces before and after the Borg swap (BorgAssets). Needs
		//`map voyager`. Works for a generated deck and, with lwh_decks.txt, a published one.
		static int step = 0;
		static int deck = 6;
		if ( level.time < 1000 ) { step = 0; deck = g_shipTestPos->string[0] ? atoi( g_shipTestPos->string ) : 6; }
		if ( step == 0 && level.time >= 3000 )
		{
			char name[32];
			Com_sprintf( name, sizeof( name ), "d%02d_arrival", deck );
			gentity_t *arrival = NULL;
			for ( int i = 1; i < globals.num_entities && !arrival; ++i )
			{
				gentity_t *e = &g_entities[i];
				if ( e->inuse && e->targetname && !Q_stricmp( e->targetname, name ) ) arrival = e;
			}
			if ( arrival )
			{
				vec3_t at, angles = { 0, 0, 0 };
				VectorCopy( arrival->currentOrigin, at );
				at[2] += 24.0f;
				TeleportPlayer( &g_entities[0], at, angles, 0 );
				VectorCopy( angles, glanceAngles );
				haveGlanceAim = true;
				gi.Printf( "SHIP: borg deck test: standing on deck %d at %s\n", deck, vtos( at ) );
			}
			else gi.Printf( "SHIP: borg deck test: no %s on this map\n", name );
			// Borg aboard: the ship's own damage control leaves the deck alone, so the assimilation
			// we set stands for the photograph (otherwise the crew strip it back as fast as it grows).
			vessel.decks[deck - 1].intruders = 1.0f;
			step = 1;
		}
		if ( haveGlanceAim && g_entities[0].client )
			VectorCopy( glanceAngles, g_entities[0].client->ps.viewangles );
		// Before; half assimilated (some sections Borg); wholly assimilated (all sections Borg).
		if ( step == 1 && level.time >= 4500 ) { gi.SendConsoleCommand( "screenshot lwh_borg_deck_before\n" ); vessel.decks[deck - 1].assimilated = ship::ASSIMILATED * 0.5f; step = 2; }
		if ( step == 2 && level.time >= 6000 ) { gi.SendConsoleCommand( "screenshot lwh_borg_deck_partial\n" ); vessel.decks[deck - 1].assimilated = 1.0f; step = 3; }
		if ( step == 3 && level.time >= 7500 ) { gi.SendConsoleCommand( "screenshot lwh_borg_deck_after\n" ); step = 4; }
		if ( step == 4 && level.time >= 9000 )
		{
			gi.Printf( "SHIP: borg deck test: deck %d assimilated %d%%\n", deck, static_cast<int>( vessel.decks[deck - 1].assimilated * 100 + 0.5f ) );
			gi.SendConsoleCommand( "quit\n" );
			step = 5;
		}
		return;
	}
	if ( g_shipTest->integer == 15 )
	{//stand at the generated deck's status screen and photograph the live surface the module draws
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 3000 )
		{
			gentity_t *arrival = NULL;
			for ( int i = 1; i < globals.num_entities && !arrival; ++i )
			{
				gentity_t *e = &g_entities[i];
				if ( e->inuse && e->targetname && !Q_stricmp( e->targetname, "d06_arrival" ) ) arrival = e;
			}
			if ( arrival )
			{
				vec3_t at, angles = { 0, 0, 0 };
				VectorCopy( arrival->currentOrigin, at );
				at[2] += 24.0f; // the arrival sits on the floor; the eye stands above it
				TeleportPlayer( &g_entities[0], at, angles, 0 );
				VectorCopy( angles, glanceAngles );
				haveGlanceAim = true;
				gi.Printf( "SHIP: panel test: standing on deck 6 at %s facing the screen\n", vtos( at ) );
			}
			else
				gi.Printf( "SHIP: panel test: no d06_arrival on this map\n" );
			step = 1;
		}
		if ( haveGlanceAim && g_entities[0].client )
			VectorCopy( glanceAngles, g_entities[0].client->ps.viewangles );
		if ( step == 1 && level.time >= 5000 ) { gi.SendConsoleCommand( "screenshot lwh_panel\n" ); step = 2; }
		if ( step == 2 && level.time >= 6500 ) { gi.SendConsoleCommand( "quit\n" ); step = 3; }
		return;
	}
	if ( g_shipTest->integer == 16 )
	{//the log: act, then read it back the way a player would
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 3000 )
		{
			ship::SetRole( vessel, ship::ROLE_IN_COMMAND );
			ship::SetAlert( vessel, ship::ALERT_RED );
			ship::BreachDeck( vessel, 9, 1.0f );
			ship::DamageSystem( vessel, ship::SYS_SENSORS, 0.4f );
			ship::OrderTriage( vessel, 1 );
			step = 1;
		}
		if ( step == 1 && level.time >= 4500 )
		{
			gi.SendConsoleCommand( "ship jobs\n" );
			gi.Printf( "SHIP: --- the log ---\n" );
			for ( const ship::LogEntry &e : vessel.log )
			{
				const int day = static_cast<int>( e.time / ship::SECONDS_PER_DAY );
				const int sod = static_cast<int>( e.time ) % ship::SECONDS_PER_DAY;
				gi.Printf( "SHIP: day %d %02d:%02d [%s] %s: %s\n", day, sod / 3600, sod % 3600 / 60,
					e.scope.c_str(), e.who.c_str(), e.what.c_str() );
			}
			step = 2;
		}
		if ( step == 2 && level.time >= 5500 ) { gi.SendConsoleCommand( "quit\n" ); step = 3; }
		return;
	}
	if ( g_shipTest->integer == 48 )
	{//the written-off list: give things up, then read it back the way a player would
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 3000 )
		{
			ship::SetRole( vessel, ship::ROLE_IN_COMMAND );
			ship::WriteOff( vessel, false, 9, ship::LOSS_SEALED );
			ship::WriteOff( vessel, true, ship::SYS_PHASERS, ship::LOSS_STRIPPED );
			ship::WriteOff( vessel, false, 5, ship::LOSS_UNINHABITABLE );
			step = 1;
		}
		if ( step == 1 && level.time >= 4500 )
		{
			gi.SendConsoleCommand( "ship losses\n" );
			gi.Printf( "SHIP: --- what the ship has given up ---\n" );
			for ( const ship::LossEntry &e : ship::WriteOffs( vessel ) )
			{
				const int day = static_cast<int>( e.time / ship::SECONDS_PER_DAY );
				const int sod = static_cast<int>( e.time ) % ship::SECONDS_PER_DAY;
				gi.Printf( "SHIP: day %d %02d:%02d [%s] %s: decided by %s\n", day, sod / 3600, sod % 3600 / 60,
					ship::LossKindName( e.kind ), e.what.c_str(), e.who.c_str() );
			}
			step = 2;
		}
		if ( step == 2 && level.time >= 5500 ) { gi.SendConsoleCommand( "quit\n" ); step = 3; }
		return;
	}
	if ( g_shipTest->integer == 24 )
	{//runtime asset replacement (S8): swap a surface's shader for the Borg one live, then back
		static const struct { int ms; const char *command; } STEPS[] = {
			{ 3000, "ship borgfx on\n" },
			{ 4500, "screenshot lwh_borgfx\n" },
			{ 5500, "ship borgfx off\n" },
		};
		static size_t step = 0;
		if ( level.time < 1000 ) step = 0;
		while ( step < sizeof( STEPS ) / sizeof( STEPS[0] ) && level.time >= STEPS[step].ms )
			gi.SendConsoleCommand( STEPS[step++].command );
		if ( tested || level.time < 7000 ) return;
		tested = true;
		gi.Printf( "SHIP: borgfx test done\n" );
		gi.SendConsoleCommand( "quit\n" );
		return;
	}
	if ( g_shipTest->integer == 23 )
	{//the panels that are not stations: the log terminal, the ready room and the personnel padd open
		//the ship's own screens, driven by the command a map's interface fires
		static const struct { int ms; const char *command; } STEPS[] = {
			{ 3000, "genericmenu log7\n" },
			{ 4200, "genericmenu readyroom\n" },
			{ 5400, "genericmenu personnel\n" },
			{ 6200, "genericmenu replicator\n" },
			{ 7200, "screenshot lwh_nonstation\n" },
		};
		static size_t step = 0;
		if ( level.time < 1000 ) step = 0;
		while ( step < sizeof( STEPS ) / sizeof( STEPS[0] ) && level.time >= STEPS[step].ms )
			gi.SendConsoleCommand( STEPS[step++].command );
		if ( tested || level.time < 8000 ) return;
		tested = true;
		gi.Printf( "SHIP: non-station panel test done\n" );
		gi.SendConsoleCommand( "quit\n" );
		return;
	}
	if ( g_shipTest->integer == 21 )
	{//the backlog batch: the tractor and salvage, fabrication, the EMH, and the crew systems
		static int step = 0;
		if ( level.time < 1000 ) { step = 0; }
		if ( step == 0 && level.time >= 3000 )
		{
			ship::SetRole( vessel, ship::ROLE_IN_COMMAND );
			vessel.sector[0].kind = ship::BEACON_DERELICT;
			vessel.sector[0].looted = false;
			gi.SendConsoleCommand( "ship as 1 tractor\n" );     // strip the wreck
			gi.SendConsoleCommand( "ship as 0 fabricate 5\n" ); // material into parts
			gi.SendConsoleCommand( "ship as 4 emh on\n" );      // the hologram
			gi.SendConsoleCommand( "ship train 20 1\n" );       // a cross-qualification
			gi.SendConsoleCommand( "ship promote 20\n" );       // a promotion
			gi.SendConsoleCommand( "ship brig 40 on\n" );       // the brig
			gi.SendConsoleCommand( "ship funeral\n" );          // a funeral
			step = 1;
		}
		if ( step == 1 && level.time >= 4800 )
		{
			gi.Printf( "SHIP: backlog test: material %.0f, parts %.0f, EMH %d\n", vessel.stores.materials, vessel.stores.spareParts,
				ship::EMHActive( vessel ) ? 1 : 0 );
			gi.Printf( "SHIP: backlog test: crew 20 qualified at Tactical %d, rank %d; crew 40 brigged %d\n",
				ship::Qualified( vessel.crew[20], ship::STN_TACTICAL ) ? 1 : 0, vessel.crew[20].rank, ship::Brigged( vessel, 40 ) ? 1 : 0 );
			gi.SendConsoleCommand( "quit\n" );
			step = 2;
		}
		return;
	}
	if ( g_shipTest->integer == 20 )
	{//the wall clock (S10): the ship lives on while the game is closed. Two days away is two days
		//aboard, and a year away is capped at thirty.
		static int step = 0;
		if ( level.time < 1000 ) { step = 0; }
		if ( step == 0 && level.time >= 3000 )
		{
			vessel.cfg.clockMode = ship::CLOCK_WALL; // the clock that lives on while the game is closed
			gi.Printf( "SHIP: wall-clock test: before, day %d, deuterium %d%%\n", vessel.Day(), static_cast<int>( vessel.stores.deuterium * 100 + 0.5f ) );
			ship::CatchUp( vessel, 2.0 * ship::SECONDS_PER_DAY );
			gi.Printf( "SHIP: wall-clock test: after two days away, day %d, deuterium %d%%\n", vessel.Day(), static_cast<int>( vessel.stores.deuterium * 100 + 0.5f ) );
			ship::CatchUp( vessel, 365.0 * ship::SECONDS_PER_DAY );
			gi.Printf( "SHIP: wall-clock test: a year away advanced her to day %d (capped at %d)\n",
				vessel.Day(), static_cast<int>( ship::MAX_CATCH_UP_DAYS ) );
			step = 1;
		}
		if ( step == 1 && level.time >= 4200 ) { gi.SendConsoleCommand( "quit\n" ); step = 2; }
		return;
	}
	if ( g_shipTest->integer == 19 )
	{//the outside, full (S9): target an enemy subsystem, hail, run, read the sector and the chart
		static const struct { int ms; const char *command; } STEPS[] = {
			{ 3000, "ship as 1 target weapons\n" },
			{ 3600, "ship as 2 hail\n" },
			{ 4200, "ship as 3 run\n" },
			{ 5000, "ship chart\n" },
			{ 5600, "screenshot lwh_outside\n" },
		};
		static size_t step = 0;
		if ( level.time < 1000 ) { step = 0; ship::SetRole( vessel, ship::ROLE_IN_COMMAND ); }
		while ( step < sizeof( STEPS ) / sizeof( STEPS[0] ) && level.time >= STEPS[step].ms )
			gi.SendConsoleCommand( STEPS[step++].command );
		if ( tested || level.time < 6500 ) return;
		tested = true;
		gi.Printf( "SHIP: outside test: %s, at beacon %d, target %s, in combat %d, pursued %d\n",
			gi.cvar( "lwh_ship_sector", "", 0 )->string, vessel.beacon, ship::EnemySubsystemName( ship::Target( vessel ) ),
			ship::InCombat( vessel ) ? 1 : 0, ship::Pursued( vessel ) ? 1 : 0 );
		gi.SendConsoleCommand( "quit\n" );
		return;
	}
	if ( g_shipTest->integer == 18 )
	{//the five gaps' completions: replication, the surgical field, endurance per source, the tricorder
		//reading a compartment, and the captain's log -- by console, as the panels drive them
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 3000 )
		{
			ship::SetRole( vessel, ship::ROLE_IN_COMMAND );
			ship::BreachDeck( vessel, 9, 1.0f );
			vessel.stores.medicalSupplies = 0.0f;
			ship::SetEnabled( vessel, ship::SYS_REPLICATORS, false ); // so the ward does not restock during the demo
			for ( ship::CrewMember &c : vessel.crew )
				if ( c.status == ship::CREW_FIT ) { c.status = ship::CREW_INJURED; c.severity = 0.9f; break; }
			gi.SendConsoleCommand( "ship scancomp 9\n" );
			gi.SendConsoleCommand( "ship surgical on\n" );
			step = 1;
		}
		if ( step == 1 && level.time >= 4600 )
		{
			gi.SendConsoleCommand( "ship as 4 patients\n" );
			gi.SendConsoleCommand( "ship captain\n" );
			step = 2;
		}
		if ( step == 2 && level.time >= 6000 )
		{
			gi.Printf( "SHIP: gap test: clocks: %s\n", gi.cvar( "lwh_ship_clocks", "", 0 )->string );
			gi.Printf( "SHIP: gap test: surgical field %d, kit condition %d%%\n",
				ship::SurgicalField( vessel ) ? 1 : 0, static_cast<int>( vessel.stores.kitCondition * 100 + 0.5f ) );
			gi.SendConsoleCommand( "screenshot lwh_gaps\n" );
			gi.SendConsoleCommand( "ui_lwh_log\n" );
			step = 3;
		}
		if ( step == 3 && level.time >= 7200 ) { gi.SendConsoleCommand( "screenshot lwh_log\n" ); step = 4; }
		if ( step == 4 && level.time >= 8000 ) { gi.SendConsoleCommand( "quit\n" ); step = 5; }
		return;
	}
	if ( g_shipTest->integer == 17 )
	{//the stations' purposes (S4): the transporter beams a party, astrometrics surveys, the Conn lays
		//in a course, sickbay reads its ward -- each through the console a station's panel opens
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 3000 )
		{
			ship::SetRole( vessel, ship::ROLE_IN_COMMAND );
			// a few casualties, so the ward has something to read
			int made = 0;
			for ( ship::CrewMember &c : vessel.crew ) {
				if ( made >= 5 ) break;
				if ( c.status != ship::CREW_FIT ) continue;
				c.status = ship::CREW_INJURED;
				c.severity = 0.2f + 0.15f * made;
				++made;
			}
			gi.SendConsoleCommand( "ship as 2 survey\n" );
			gi.SendConsoleCommand( "ship as 2 transport 3\n" );
			step = 1;
		}
		if ( step == 1 && level.time >= 4200 )
		{
			gi.SendConsoleCommand( "ship as 3 course 5\n" );
			gi.SendConsoleCommand( "ship as 4 patients\n" );
			step = 2;
		}
		if ( step == 2 && level.time >= 5600 )
		{
			int surveyed = 0;
			for ( const ship::Beacon &b : vessel.sector ) if ( b.surveyed ) ++surveyed;
			gi.Printf( "SHIP: station purposes: away team %d on beacon %d, course %d, %d beacons surveyed, %d in the ward\n",
				ship::AwayTeam( vessel ), vessel.awayBeacon, vessel.course, surveyed,
				static_cast<int>( ship::Patients( vessel ).size() ) );
			gi.SendConsoleCommand( "ship as 2 recall\n" );
			step = 3;
		}
		if ( step == 3 && level.time >= 6200 )
		{
			gi.Printf( "SHIP: station purposes: away team back, %d away now\n", ship::AwayTeam( vessel ) );
			gi.SendConsoleCommand( "screenshot lwh_stations\n" );
			step = 4;
		}
		if ( step == 4 && level.time >= 7400 ) { gi.SendConsoleCommand( "quit\n" ); step = 5; }
		return;
	}
	if ( g_shipTest->integer == 12 )
	{//the turbolift's own menu: report the deck list it reads, then open it exactly as a panel does
		// Opening the menu pauses the game, which stops Ship_Frame, so the screenshot and the quit
		// must ride the engine's own command buffer behind it; `wait` lets frames pass first.
		static const struct { int ms; const char *command; } STEPS[] = {
			{ 3000, "lwh_ui_turbolift\n" },        // what the UI's FS loads for the menu
			{ 4000, "genericmenu turbolift\nwait 60\nscreenshot lwh_turbolift\nwait 20\nquit\n" },
		};
		static size_t step = 0;
		if ( level.time < 1000 ) step = 0;
		while ( step < sizeof( STEPS ) / sizeof( STEPS[0] ) && level.time >= STEPS[step].ms )
			gi.SendConsoleCommand( STEPS[step++].command );
		if ( tested || level.time < 8000 ) return;
		tested = true;
		gi.Printf( "SHIP: turbolift menu test done\n" );
		gi.SendConsoleCommand( "quit\n" );
		return;
	}
	if ( g_shipTest->integer == 5 )
	{//ride the turbolift to every deck in turn, and say how the player arrived on each
		static int deck = 0, from = 1, nextMs = 0, good = 0;
		if ( level.time < 1000 ) { deck = 0; from = 1; nextMs = 3000; good = 0; }
		if ( level.time < nextMs || deck > ship::DECKS ) return;
		gentity_t *player = &g_entities[0];
		if ( deck >= 1 )
		{//report the arrival made a second ago
			trace_t tr;
			gi.trace( &tr, player->currentOrigin, player->mins, player->maxs, player->currentOrigin, 0, MASK_PLAYERSOLID );
			const bool onFloor = player->client->ps.groundEntityNum != ENTITYNUM_NONE;
			const bool clear = !tr.startsolid && !tr.allsolid;
			const float depth = -player->currentOrigin[2];
			const int arrived = static_cast<int>( ( depth - 1636.0f ) / g_shipTestPitch->value ) + 1;
			const bool ok = onFloor && clear && arrived == deck;
			if ( ok ) ++good;
			gi.Printf( "SHIP: deck %2d: at %s  %s, %s, on deck %d  %s\n", deck, vtos( player->currentOrigin ),
				onFloor ? "standing" : "NOT ON A FLOOR", clear ? "clear" : "IN SOLID", arrived, ok ? "ok" : "FAILED" );
			from = arrived;
		}
		if ( ++deck > ship::DECKS )
		{
			gi.Printf( "SHIP: turbolift tour: %d of %d decks reached standing and clear\n", good, ship::DECKS );
			gi.SendConsoleCommand( "quit\n" );
			return;
		}
		// asked for exactly as the retail turbolift menu asks (sp_turbolift.dat: "use tour_turbo_04"):
		// the ship resolves the name on the deck the player is standing on
		if ( deck != from ) gi.SendConsoleCommand( Fmt( "use tour_turbo_%02d\n", deck ).c_str() );
		nextMs = level.time + 1500;
		return;
	}
	if ( g_shipTest->integer == 4 )
	{//stand somewhere else in the ship (g_shipTestPos "x y z") and photograph what is there
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 3000 )
		{
			ReportWatchedTrigger( "before the player arrives" );
			vec3_t to = { 0, 0, 0 }, angles = { 0, 0, 0 };
			if ( sscanf( g_shipTestPos->string, "%f %f %f", &to[0], &to[1], &to[2] ) == 3 )
			{
				TeleportPlayer( &g_entities[0], to, angles, 0 );
			}
			else
			{//not a position: the name of something to use, as the turbolift's menu would
				gi.SendConsoleCommand( Fmt( "use %s\n", g_shipTestPos->string ).c_str() );
			}
			step = 1;
		}
		if ( step == 1 && level.time >= 4500 ) { ReportWatchedTrigger( "with the player inside it" ); step = 10; }
		if ( step == 10 && level.time >= 7000 ) { gi.SendConsoleCommand( "screenshot lwh_ship\n" ); step = 2; }
		if ( step == 2 && level.time >= 8000 )
		{
			gentity_t *player = &g_entities[0];
			gi.Printf( "SHIP: standing at %s\n", vtos( player->currentOrigin ) );
			int lookups = 0, resolved = 0;
			LWH_ScopeStats( &lookups, &resolved );
			gi.Printf( "SHIP: script name lookups %d, resolved to the script's own deck %d\n", lookups, resolved );
			gi.SendConsoleCommand( "quit\n" );
			step = 3;
		}
		return;
	}
	if ( g_shipTest->integer == 54 )
	{//the environment in the world: a deck whose plating has failed. The player does not fall; the
	 //engine's own FIXME (no way back) is closed by the magnetic boots and by the deck recovering.
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 2500 )
		{
			if ( !TeleportPlayerTo( "lwh_breach", "gravity test" ) )
				TeleportPlayerTo( "d12_arrival", "gravity test" );
			vessel.decks[11].gravity = 0.0f; // the plating on deck 12 loses hold
			gi.Printf( "SHIP: gravity test: deck 12 plating at 0%%\n" );
			step = 1;
		}
		if ( step == 1 && level.time >= 4000 )
		{
			gentity_t *p = &g_entities[0];
			gi.Printf( "SHIP: gravity test: floating: ps.gravity %d, custom %d, z %.0f, on floor %d, crew floating %d\n",
				p->client->ps.gravity, ( p->svFlags & SVF_CUSTOM_GRAVITY ) ? 1 : 0,
				p->currentOrigin[2], p->client->ps.groundEntityNum != ENTITYNUM_NONE ? 1 : 0, Crew_Floating() );
			step = 2;
		}
		if ( step == 2 && level.time >= 5000 ) { gi.SendConsoleCommand( "ship boots on\n" ); step = 3; }
		if ( step == 3 && level.time >= 6200 )
		{
			gentity_t *p = &g_entities[0];
			gi.Printf( "SHIP: gravity test: boots on: ps.gravity %d, custom %d\n",
				p->client->ps.gravity, ( p->svFlags & SVF_CUSTOM_GRAVITY ) ? 1 : 0 );
			gi.SendConsoleCommand( "ship boots off\n" );
			step = 4;
		}
		if ( step == 4 && level.time >= 7400 )
		{
			gentity_t *p = &g_entities[0];
			gi.Printf( "SHIP: gravity test: boots off: ps.gravity %d, custom %d; deck recovers\n",
				p->client->ps.gravity, ( p->svFlags & SVF_CUSTOM_GRAVITY ) ? 1 : 0 );
			vessel.decks[11].gravity = 1.0f; // the deck has hold again: the engine's FIXME case
			step = 5;
		}
		if ( step == 5 && level.time >= 8600 )
		{
			gentity_t *p = &g_entities[0];
			gi.Printf( "SHIP: gravity test: recovered: ps.gravity %d, custom %d, crew floating %d\n",
				p->client->ps.gravity, ( p->svFlags & SVF_CUSTOM_GRAVITY ) ? 1 : 0, Crew_Floating() );
			gi.SendConsoleCommand( "screenshot lwh_gravity\n" );
			step = 6;
		}
		if ( step == 6 && level.time >= 10000 ) { gi.SendConsoleCommand( "quit\n" ); step = 7; }
		return;
	}
	if ( g_shipTest->integer == 55 )
	{//a breach you can feel and a field you can see: with the field off the authored push and hurt
	 //are live and the air is going; raise the field and they stop, the brush is solid, air holds.
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 2500 )
		{
			if ( !TeleportPlayerTo( "lwh_breach", "breach test" ) )
				TeleportPlayerTo( "d12_arrival", "breach test" );
			vessel.decks[11].gravity = 1.0f; // gravity is not the subject of this test
			ship::SetForceFieldLevel( vessel, 12, 0 );
			ship::BreachDeck( vessel, 12, 1.0f );
			gi.Printf( "SHIP: breach test: deck 12 hull %.2f, air %.2f, minutes of air %.1f\n",
				vessel.decks[11].hull, vessel.decks[11].atmosphere, ship::MinutesOfAir( vessel, 12 ) );
			step = 1;
		}
		if ( step == 1 && level.time >= 2900 )
		{//a beat after the breach: the push has just thrown the player toward the hole
			gentity_t *p = &g_entities[0];
			gi.Printf( "SHIP: breach test: field off, thrown: health %d, velocity %s, at %s\n",
				p->health, vtos( p->client->ps.velocity ), vtos( p->currentOrigin ) );
			step = 2;
		}
		if ( step == 2 && level.time >= 4200 )
		{
			gentity_t *p = &g_entities[0];
			gi.Printf( "SHIP: breach test: field off: health %d, velocity %s, air %.2f, minutes of air %.1f\n",
				p->health, vtos( p->client->ps.velocity ), vessel.decks[11].atmosphere, ship::MinutesOfAir( vessel, 12 ) );
			step = 3;
		}
		if ( step == 3 && level.time >= 4800 ) { gi.SendConsoleCommand( "ship field 12 on\n" ); step = 4; }
		if ( step == 4 && level.time >= 6300 )
		{
			gentity_t *p = &g_entities[0];
			gi.Printf( "SHIP: breach test: field on: health %d, velocity %s, minutes of air %.1f, published field %s\n",
				p->health, vtos( p->client->ps.velocity ), ship::MinutesOfAir( vessel, 12 ),
				gi.cvar( "lwh_ship_breach_field", "", 0 )->string );
			step = 5;
		}
		if ( step == 5 && level.time >= 6800 ) { gi.SendConsoleCommand( "screenshot lwh_breach\n" ); step = 6; }
		if ( step == 6 && level.time >= 8300 ) { gi.SendConsoleCommand( "quit\n" ); step = 7; }
		return;
	}
	if ( g_shipTest->integer == 56 )
	{//Stage B -- the player in the world. A console that lets go at the operator; being carried and
	 //treated; the air of the room the player is actually in; and death, which is not a reload.
	 //Run with g_player 1 and again with g_player 0 (the gate): off, none of it happens.
		static int step = 0, me = -1;
		if ( level.time < 1000 ) { step = 0; me = -1; }
		if ( step == 0 && level.time >= 2500 )
		{
			me = ship::CreateCharacter( vessel, "Test Player", ship::DEPT_COMMAND, 2 );
			if ( me >= 0 ) ApplyPlayerBody();
			if ( !TeleportPlayerTo( "lwh_station_0", "player test" ) )
				TeleportPlayerTo( "d12_arrival", "player test" );
			// the degraded grid: life support damaged, and the ship at battle stations
			ship::DamageSystem( vessel, ship::SYS_LIFE_SUPPORT, 0.6f );
			ship::SetAlert( vessel, ship::ALERT_RED );
			gi.Printf( "SHIP: player test: %s at the life support console, %d%% health, %d%% condition, red alert\n",
				me >= 0 ? vessel.crew[me].name.c_str() : "no character",
				static_cast<int>( vessel.systems[ship::SYS_LIFE_SUPPORT].health * 100.0f + 0.5f ),
				static_cast<int>( ship::SystemCondition( vessel.systems[ship::SYS_LIFE_SUPPORT] ) * 100.0f + 0.5f ) );
			gi.SendConsoleCommand( "ship operate life\n" ); // the path a hand at the panel sends
			step = 1;
		}
		if ( step == 1 && level.time >= 3400 && me >= 0 )
		{// keep working the degraded console until it lets go at the operator
			int draws = 0;
			while ( draws < 400 && vessel.crew[me].status == ship::CREW_FIT )
			{
				// the gated path the console command uses: with g_player off this refuses at once,
				// so nothing here reaches the record (the gate holds)
				if ( !Crew_PlayerUseSystem( static_cast<int>( ship::SYS_LIFE_SUPPORT ) ) ) break;
				++draws;
			}
			gi.Printf( "SHIP: player test: the console let go after %d draws: status %d, severity %.2f\n",
				draws, vessel.crew[me].status, vessel.crew[me].severity );
			step = 2;
		}
		if ( step == 2 && level.time >= 4200 && me >= 0 )
		{// the body follows the record (the record -> body direction), and the ship acts on it
			bool letGo = false, attends = false;
			for ( const ship::LogEntry &e : vessel.log )
			{
				if ( e.what.find( "console let go" ) != std::string::npos ) letGo = true;
				if ( e.what.find( "attends" ) != std::string::npos ) attends = true;
			}
			for ( const ship::LogEntry &e : vessel.log )
				if ( e.what.find( "console let go" ) != std::string::npos || e.what.find( "attends" ) != std::string::npos )
					gi.Printf( "SHIP: player test: [%s] %s: %s\n", e.scope.c_str(), e.who.c_str(), e.what.c_str() );
			gi.Printf( "SHIP: player test: hurt by the console %d, attended %d; body health %d of %d, incapacitated %d\n",
				letGo ? 1 : 0, attends ? 1 : 0, g_entities[0].health, g_entities[0].max_health,
				ship::PlayerIncapacitated( vessel ) ? 1 : 0 );
			// treatment is the ordinary casualty path: supplies, a bed, and time
			vessel.systems[ship::SYS_SICKBAY].health = 1.0f;
			vessel.systems[ship::SYS_SICKBAY].output = 1.0f;
			vessel.stores.medicalSupplies = 100.0f;
			ship::Sleep( vessel, 24.0 * 3600.0 );
			gi.Printf( "SHIP: player test: after a day in the ward: status %d\n", vessel.crew[me].status );
			step = 3;
		}
		if ( step == 3 && level.time >= 5200 && me >= 0 )
		{// recovered: the ward returned the record to fit, and the body is whole again
			gi.Printf( "SHIP: player test: recovered: status %d, body health %d of %d\n",
				vessel.crew[me].status, g_entities[0].health, g_entities[0].max_health );
			// a breached, airless compartment: the person in it is affected. The ward is held back
			// for a beat so the player is not carried before we can show they can still leave.
			vessel.systems[ship::SYS_SICKBAY].output = 0.0f;
			vessel.stores.medicalSupplies = 0.0f;
			vessel.emhActive = false;
			vessel.decks[11].hull = 0.0f;
			vessel.decks[11].atmosphere = 0.0f;
			ship::Sleep( vessel, ship::EXPOSURE_INJURES + 25.0f );
			gi.Printf( "SHIP: player test: in the airless compartment: status %d, severity %.2f, deck %d, moving %d\n",
				vessel.crew[me].status, vessel.crew[me].severity, vessel.crew[me].deck,
				g_entities[0].client->ps.pm_type != PM_DEAD ? 1 : 0 );
			gi.SendConsoleCommand( "use tour_turbo_04\n" ); // leave the deck: the way out
			step = 4;
		}
		if ( step == 4 && level.time >= 7200 && me >= 0 )
		{// the player can leave: the body has gone to another deck, and the air there is not the hazard
			gi.Printf( "SHIP: player test: left the deck: at %s, record deck %d, still moving %d, status %d\n",
				vtos( g_entities[0].currentOrigin ), vessel.crew[me].deck,
				g_entities[0].client->ps.pm_type != PM_DEAD ? 1 : 0, vessel.crew[me].status );
			gi.SendConsoleCommand( "use tour_turbo_12\n" ); // back into the airless compartment, to die
			step = 5;
		}
		if ( step == 5 && level.time >= 9200 && me >= 0 )
		{// death is reachable by a cause the model tracks, and it is not a reload: the record closes,
		 // the body falls, the engine's respawn is refused, and command passes to the senior officer.
			ship::Sleep( vessel, ship::EXPOSURE_KILLS + 40.0f );
			gi.Printf( "SHIP: player test: in the airless compartment past the limit: status %d\n", vessel.crew[me].status );
			step = 6;
		}
		if ( step == 6 && level.time >= 10500 && me >= 0 )
		{
			const bool dead = ship::PlayerDead( vessel );
			for ( const ship::LogEntry &e : vessel.log )
				if ( e.what.find( "dead, no air" ) != std::string::npos || e.what.find( "is dead" ) != std::string::npos )
					gi.Printf( "SHIP: player test: [%s] %s: %s\n", e.scope.c_str(), e.who.c_str(), e.what.c_str() );
			gi.Printf( "SHIP: player test: dead %d, body health %d, respawn blocked %d, command \"%s\"\n",
				dead ? 1 : 0, g_entities[0].health, LWH_BlockRespawn( &g_entities[0] ) ? 1 : 0,
				ship::CommandingOfficer( vessel ).c_str() );
			gi.SendConsoleCommand( "quit\n" );
			step = 7;
		}
		return;
	}
	if ( g_shipTest->integer == 58 )
	{//the emergency lighting state (docs/locations/deck12-environmental-control.brief.md): the room
	 // whose failure darkens other decks goes red first. Stand at the life-support watch station,
	 // put the ship at battle stations, photograph the red state, then stand down and photograph it
	 // off. The strips are authored func_usable brushes; the module (g_env) swaps them.
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 2500 )
		{
			if ( !TeleportPlayerTo( "lwh_station_0", "emergency test" ) )
				TeleportPlayerTo( "d12_arrival", "emergency test" );
			vec3_t look = { -25, 90, 0 };  // look up along the room, at the light strips
			VectorCopy( look, glanceAngles );
			haveGlanceAim = true;
			gi.Printf( "SHIP: emergency test: alert %d, life support %d%%\n",
				static_cast<int>( vessel.alert ),
				static_cast<int>( ship::SystemCondition( vessel.systems[ship::SYS_LIFE_SUPPORT] ) * 100.0f + 0.5f ) );
			step = 1;
		}
		if ( haveGlanceAim && g_entities[0].client )
			VectorCopy( glanceAngles, g_entities[0].client->ps.viewangles );
		if ( step == 1 && level.time >= 3200 ) { ship::SetAlert( vessel, ship::ALERT_RED ); step = 2; }
		if ( step == 2 && level.time >= 4300 ) { gi.SendConsoleCommand( "screenshot lwh_emergency\n" ); step = 3; }
		if ( step == 3 && level.time >= 5600 ) { ship::SetAlert( vessel, ship::ALERT_GREEN ); step = 4; }
		if ( step == 4 && level.time >= 6200 ) { gi.SendConsoleCommand( "screenshot lwh_emergency_off\n" ); step = 5; }
		if ( step == 5 && level.time >= 7600 ) { gi.SendConsoleCommand( "quit\n" ); step = 6; }
		return;
	}
	if ( g_shipTest->integer == 59 )
	{//the deck 13 emergency lighting state (docs/locations/deck13-life-support.brief.md): the plant
	 // whose failure darkens other decks. Stand at the local plant panel on the catwalk, go to battle
	 // stations, photograph the red strips, then stand down and photograph them off. Same mechanism as
	 // the deck 12 test, the second half of the life-support pair.
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 2500 )
		{
			if ( !TeleportPlayerTo( "lwh_plant_post", "emergency test 13" ) )
				TeleportPlayerTo( "d13_arrival", "emergency test 13" );
			vec3_t look = { 0, 270, 0 };  // look south from the catwalk, at the machinery hall
			VectorCopy( look, glanceAngles );
			haveGlanceAim = true;
			gi.Printf( "SHIP: emergency test 13: alert %d, life support %d%%\n",
				static_cast<int>( vessel.alert ),
				static_cast<int>( ship::SystemCondition( vessel.systems[ship::SYS_LIFE_SUPPORT] ) * 100.0f + 0.5f ) );
			step = 1;
		}
		if ( haveGlanceAim && g_entities[0].client )
			VectorCopy( glanceAngles, g_entities[0].client->ps.viewangles );
		if ( step == 1 && level.time >= 3200 ) { ship::SetAlert( vessel, ship::ALERT_RED ); step = 2; }
		if ( step == 2 && level.time >= 4300 ) { gi.SendConsoleCommand( "screenshot lwh_emergency_13\n" ); step = 3; }
		if ( step == 3 && level.time >= 5600 ) { ship::SetAlert( vessel, ship::ALERT_GREEN ); step = 4; }
		if ( step == 4 && level.time >= 6200 ) { gi.SendConsoleCommand( "screenshot lwh_emergency_13_off\n" ); step = 5; }
		if ( step == 5 && level.time >= 7600 ) { gi.SendConsoleCommand( "quit\n" ); step = 6; }
		return;
	}
	if ( g_shipTest->integer == 60 )
	{//the deck 14 re-dress (docs/locations/deck14-stasis.brief.md): stand at the turbolift arrival
	 // facing in past the stasis pods, then at the holodeck end by the arch, and photograph both --
	 // the cold pods and the warm holodeck are the brief's point, so both ends are seen.
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 3000 )
		{
			gentity_t *arrival = NULL;
			for ( int i = 1; i < globals.num_entities && !arrival; ++i )
			{
				gentity_t *e = &g_entities[i];
				if ( e->inuse && e->targetname && !Q_stricmp( e->targetname, "d14_arrival" ) ) arrival = e;
			}
			if ( arrival )
			{
				vec3_t at, angles = { 0, 135, 0 };  // face in and north, along the stasis pod wall
				VectorCopy( arrival->currentOrigin, at );
				at[2] += 24.0f;
				TeleportPlayer( &g_entities[0], at, angles, 0 );
				VectorCopy( angles, glanceAngles );
				haveGlanceAim = true;
				gi.Printf( "SHIP: deck 14 room: standing at %s\n", vtos( at ) );
			}
			else gi.Printf( "SHIP: deck 14 room: no d14_arrival on this map\n" );
			step = 1;
		}
		if ( haveGlanceAim && g_entities[0].client )
			VectorCopy( glanceAngles, g_entities[0].client->ps.viewangles );
		if ( step == 1 && level.time >= 4000 ) { gi.SendConsoleCommand( "screenshot lwh_deck14\n" ); step = 2; }
		if ( step == 2 && level.time >= 5000 )
		{//the holodeck end: the stasis/holodeck maintenance post, looking at the arch
			if ( TeleportPlayerTo( "lwh_stasis_post", "deck 14 room: at the holodeck" ) )
			{
				vec3_t look = { 0, 90, 0 };  // face the holodeck arch on the north wall
				VectorCopy( look, glanceAngles );
				haveGlanceAim = true;
			}
			step = 3;
		}
		if ( step == 3 && level.time >= 6000 ) { gi.SendConsoleCommand( "screenshot lwh_deck14_holodeck\n" ); step = 4; }
		if ( step == 4 && level.time >= 7400 ) { gi.SendConsoleCommand( "quit\n" ); step = 5; }
		return;
	}
	if ( g_shipTest->integer == 61 )
	{//the deck 14 emergency lighting state (docs/locations/deck14-stasis.brief.md): the stasis deck
	 // goes red with the ship. Stand at the holodeck end, go to battle stations, photograph the red
	 // strips, then stand down and photograph them off. Same mechanism as the deck 12 and 13 tests,
	 // the third deck to carry the state.
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 2500 )
		{
			if ( !TeleportPlayerTo( "lwh_stasis_post", "emergency test 14" ) )
				TeleportPlayerTo( "d14_arrival", "emergency test 14" );
			vec3_t look = { 0, 270, 0 };  // look south into the room, at the ceiling strips
			VectorCopy( look, glanceAngles );
			haveGlanceAim = true;
			gi.Printf( "SHIP: emergency test 14: alert %d, life support %d%%\n",
				static_cast<int>( vessel.alert ),
				static_cast<int>( ship::SystemCondition( vessel.systems[ship::SYS_LIFE_SUPPORT] ) * 100.0f + 0.5f ) );
			step = 1;
		}
		if ( haveGlanceAim && g_entities[0].client )
			VectorCopy( glanceAngles, g_entities[0].client->ps.viewangles );
		if ( step == 1 && level.time >= 3200 ) { ship::SetAlert( vessel, ship::ALERT_RED ); step = 2; }
		if ( step == 2 && level.time >= 4300 ) { gi.SendConsoleCommand( "screenshot lwh_emergency_14\n" ); step = 3; }
		if ( step == 3 && level.time >= 5600 ) { ship::SetAlert( vessel, ship::ALERT_GREEN ); step = 4; }
		if ( step == 4 && level.time >= 6200 ) { gi.SendConsoleCommand( "screenshot lwh_emergency_14_off\n" ); step = 5; }
		if ( step == 5 && level.time >= 7600 ) { gi.SendConsoleCommand( "quit\n" ); step = 6; }
		return;
	}
	if ( g_shipTest->integer == 29 )
	{//the deck 7 re-dress (docs/locations/deck07-auxcore.brief.md): stand at the turbolift arrival
	 // facing in past the core column, then at the core-watch post by the panel, and photograph both.
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 3000 )
		{
			gentity_t *arrival = NULL;
			for ( int i = 1; i < globals.num_entities && !arrival; ++i )
			{
				gentity_t *e = &g_entities[i];
				if ( e->inuse && e->targetname && !Q_stricmp( e->targetname, "d07_arrival" ) ) arrival = e;
			}
			if ( arrival )
			{
				vec3_t at, angles = { 0, 180, 0 };  // face into the room, toward the core column
				VectorCopy( arrival->currentOrigin, at );
				at[2] += 24.0f;
				TeleportPlayer( &g_entities[0], at, angles, 0 );
				VectorCopy( angles, glanceAngles );
				haveGlanceAim = true;
				gi.Printf( "SHIP: deck 7 room: standing at %s\n", vtos( at ) );
			}
			else gi.Printf( "SHIP: deck 7 room: no d07_arrival on this map\n" );
			step = 1;
		}
		if ( haveGlanceAim && g_entities[0].client )
			VectorCopy( glanceAngles, g_entities[0].client->ps.viewangles );
		if ( step == 1 && level.time >= 4000 ) { gi.SendConsoleCommand( "screenshot lwh_deck07\n" ); step = 2; }
		if ( step == 2 && level.time >= 5000 )
		{//the core-watch post, looking down the room at the core column and its panel
			if ( TeleportPlayerTo( "lwh_core_post", "deck 7 room: at the core" ) )
			{
				vec3_t look = { 0, 210, 0 };
				VectorCopy( look, glanceAngles );
				haveGlanceAim = true;
			}
			step = 3;
		}
		if ( step == 3 && level.time >= 6000 ) { gi.SendConsoleCommand( "screenshot lwh_deck07_core\n" ); step = 4; }
		if ( step == 4 && level.time >= 7400 ) { gi.SendConsoleCommand( "quit\n" ); step = 5; }
		return;
	}
	if ( g_shipTest->integer == 62 )
	{//the deck 7 emergency lighting state (docs/locations/deck07-auxcore.brief.md): the auxiliary
	 // core deck goes red with the ship. Stand at the core post, go to battle stations, photograph
	 // the red strips, then stand down and photograph them off. The fourth deck to carry the state.
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 2500 )
		{
			if ( !TeleportPlayerTo( "lwh_core_post", "emergency test 7" ) )
				TeleportPlayerTo( "d07_arrival", "emergency test 7" );
			vec3_t look = { 0, 270, 0 };  // look into the room, at the ceiling strips
			VectorCopy( look, glanceAngles );
			haveGlanceAim = true;
			gi.Printf( "SHIP: emergency test 7: alert %d, life support %d%%\n",
				static_cast<int>( vessel.alert ),
				static_cast<int>( ship::SystemCondition( vessel.systems[ship::SYS_LIFE_SUPPORT] ) * 100.0f + 0.5f ) );
			step = 1;
		}
		if ( haveGlanceAim && g_entities[0].client )
			VectorCopy( glanceAngles, g_entities[0].client->ps.viewangles );
		if ( step == 1 && level.time >= 3200 ) { ship::SetAlert( vessel, ship::ALERT_RED ); step = 2; }
		if ( step == 2 && level.time >= 4300 ) { gi.SendConsoleCommand( "screenshot lwh_emergency_07\n" ); step = 3; }
		if ( step == 3 && level.time >= 5600 ) { ship::SetAlert( vessel, ship::ALERT_GREEN ); step = 4; }
		if ( step == 4 && level.time >= 6200 ) { gi.SendConsoleCommand( "screenshot lwh_emergency_07_off\n" ); step = 5; }
		if ( step == 5 && level.time >= 7600 ) { gi.SendConsoleCommand( "quit\n" ); step = 6; }
		return;
	}
	if ( g_shipTest->integer == 63 )
	{//the deck 6 composition (docs/locations/deck06-holodecks.brief.md): the deck carries three
	 // functions on one corridor -- a holodeck with two arches, an armory with a security post, and
	 // the crew quarters. Stand at the turbolift arrival facing in, then at the holodeck arch, then
	 // at the armory, and photograph all three; the walk between them is the brief's point.
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 3000 )
		{
			gentity_t *arrival = NULL;
			for ( int i = 1; i < globals.num_entities && !arrival; ++i )
			{
				gentity_t *e = &g_entities[i];
				if ( e->inuse && e->targetname && !Q_stricmp( e->targetname, "d06_arrival" ) ) arrival = e;
			}
			if ( arrival )
			{
				vec3_t at, angles = { 0, 180, 0 };  // face west, down the corridor
				VectorCopy( arrival->currentOrigin, at );
				at[2] += 24.0f;
				TeleportPlayer( &g_entities[0], at, angles, 0 );
				VectorCopy( angles, glanceAngles );
				haveGlanceAim = true;
				gi.Printf( "SHIP: deck 6 room: standing at %s\n", vtos( at ) );
			}
			else gi.Printf( "SHIP: deck 6 room: no d06_arrival on this map\n" );
			step = 1;
		}
		if ( haveGlanceAim && g_entities[0].client )
			VectorCopy( glanceAngles, g_entities[0].client->ps.viewangles );
		if ( step == 1 && level.time >= 4000 ) { gi.SendConsoleCommand( "screenshot lwh_deck06\n" ); step = 2; }
		if ( step == 2 && level.time >= 5000 )
		{//the holodeck end: the arch, facing it across the corridor
			if ( TeleportPlayerTo( "lwh_holo_post", "deck 6 room: at the holodeck" ) )
			{
				vec3_t look = { 0, 180, 0 };  // face south, at the two holodeck arches
				VectorCopy( look, glanceAngles );
				haveGlanceAim = true;
			}
			step = 3;
		}
		if ( step == 3 && level.time >= 6000 ) { gi.SendConsoleCommand( "screenshot lwh_deck06_holodeck\n" ); step = 4; }
		if ( step == 4 && level.time >= 7000 )
		{//the armory: the security post by the lockers
			if ( TeleportPlayerTo( "lwh_armory_post", "deck 6 room: at the armory" ) )
			{
				vec3_t look = { 0, 90, 0 };  // face north, out of the armory alcove
				VectorCopy( look, glanceAngles );
				haveGlanceAim = true;
			}
			step = 5;
		}
		if ( step == 5 && level.time >= 8000 ) { gi.SendConsoleCommand( "screenshot lwh_deck06_armory\n" ); step = 6; }
		if ( step == 6 && level.time >= 9400 ) { gi.SendConsoleCommand( "quit\n" ); step = 7; }
		return;
	}
	if ( g_shipTest->integer == 64 )
	{//access and authority at the console (docs/access-and-authority.md, owner decision 2026-10-07):
	 //the named refusal, a delegation for a shift and its revocation, the lock-out that names both
	 //hands, and the emergency override begun. What a hand presses is what the test sends.
		static int step = 0;
		static char playerIdx[16] = "";
		if ( level.time < 1000 ) { step = 0; playerIdx[0] = 0; }
		if ( step == 0 && level.time >= 2000 ) { gi.SendConsoleCommand( "ship character Reyes 2 1\n" ); step = 1; }
		if ( step == 1 && level.time >= 2800 ) { gi.SendConsoleCommand( "ship as 0 off sensors\n" ); step = 2; }
		if ( step == 2 && level.time >= 3300 )
		{
			gi.Printf( "SHIP: clearance 1 (not cleared): \"%s\"\n", gi.cvar( "lwh_ship_refusal", "", 0 )->string );
			gi.Cvar_VariableStringBuffer( "lwh_ship_player_index", playerIdx, sizeof( playerIdx ) );
			if ( playerIdx[0] ) gi.SendConsoleCommand( va( "ship delegate 5 %s 0\n", playerIdx ) );
			step = 3;
		}
		if ( step == 3 && level.time >= 3800 ) { gi.SendConsoleCommand( "ship as 0 off sensors\n" ); step = 4; }
		if ( step == 4 && level.time >= 4300 )
		{
			gi.Printf( "SHIP: clearance 2 (delegated for the shift): sensors %s\n", vessel.systems[ship::SYS_SENSORS].enabled ? "on" : "off" );
			if ( playerIdx[0] ) gi.SendConsoleCommand( va( "ship revoke 5 %s 0\n", playerIdx ) );
			step = 5;
		}
		if ( step == 5 && level.time >= 4800 ) { gi.SendConsoleCommand( "ship as 0 off communications\n" ); step = 6; }
		if ( step == 6 && level.time >= 5300 )
		{
			gi.Printf( "SHIP: clearance 3 (revoked): \"%s\"\n", gi.cvar( "lwh_ship_refusal", "", 0 )->string );
			if ( playerIdx[0] ) gi.SendConsoleCommand( va( "ship lockout 5 %s 0\n", playerIdx ) );
			step = 7;
		}
		if ( step == 7 && level.time >= 5800 ) { gi.SendConsoleCommand( "ship as 0 off sensors\n" ); step = 8; }
		if ( step == 8 && level.time >= 6300 )
		{
			gi.Printf( "SHIP: clearance 4 (locked out): \"%s\"\n", gi.cvar( "lwh_ship_refusal", "", 0 )->string );
			gi.Printf( "SHIP: clearance 4: the lock-out is \"%s\"\n", gi.cvar( "lwh_ship_lockouts", "", 0 )->string );
			if ( playerIdx[0] ) gi.SendConsoleCommand( va( "ship unlock 5 %s 0\n", playerIdx ) );
			step = 9;
		}
		if ( step == 9 && level.time >= 6800 ) { gi.SendConsoleCommand( "ship override begin 0\n" ); step = 10; }
		if ( step == 10 && level.time >= 7300 ) { gi.SendConsoleCommand( "ship override confirm 0\n" ); step = 11; }
		if ( step == 11 && level.time >= 7600 ) { gi.SendConsoleCommand( "ui_lwh_command\n" ); step = 12; }
		if ( step == 12 && level.time >= 8100 )
		{
			gi.Printf( "SHIP: clearance 5 (override): \"%s\"\n", gi.cvar( "lwh_ship_override", "", 0 )->string );
			gi.SendConsoleCommand( "screenshot lwh_access\n" );
			step = 13;
		}
		if ( step == 13 && level.time >= 8900 ) { gi.SendConsoleCommand( "quit\n" ); step = 14; }
		return;
	}
	if ( g_shipTest->integer == 65 )
	{//the personal log (docs/the-record-and-the-log.md, Task B): the player writes a private entry,
	 //the screen reads it, the official read of every scope does not, and another person's read is
	 //empty. The store and the rule are the model's; this proves the screen reaches the owner's only.
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 2000 ) { gi.SendConsoleCommand( "ship character Reyes 0 4\n" ); step = 1; }
		if ( step == 1 && level.time >= 2600 ) { gi.SendConsoleCommand( "ship personal write I did not put this in the report\n" ); step = 2; }
		if ( step == 2 && level.time >= 3200 ) { gi.SendConsoleCommand( "ui_lwh_log\n" ); step = 3; }
		if ( step == 3 && level.time >= 3800 ) { gi.SendConsoleCommand( "screenshot lwh_log\n" ); gi.SendConsoleCommand( "ui_lwh_personal\n" ); step = 4; }
		if ( step == 4 && level.time >= 4600 )
		{
			const int me = vessel.player;
			const int mine = me >= 0 ? static_cast<int>( ship::PersonalLog( vessel, me ).size() ) : 0;
			const int other = me >= 0 ? ( me + 1 ) % static_cast<int>( vessel.crew.size() ) : -1;
			const int theirs = other >= 0 ? static_cast<int>( ship::PersonalLog( vessel, other ).size() ) : 0;
			bool inOfficial = false;
			for ( const ship::LogEntry &e : ship::ReadOfficialLog( vessel, 200, "" ) )
				if ( e.what.find( "did not put this in the report" ) != std::string::npos ) inOfficial = true;
			gi.Printf( "SHIP: personal test: %s wrote a private entry; the owner's read has %d\n", me >= 0 ? vessel.crew[me].name.c_str() : "nobody", mine );
			gi.Printf( "SHIP: personal test: official read (all scopes) contains it: %s\n", inOfficial ? "yes" : "no" );
			gi.Printf( "SHIP: personal test: another person's read has %d\n", theirs );
			gi.Printf( "SHIP: personal test: the private screen reads \"%s\" for %s\n",
				gi.cvar( "lwh_ship_personal", "", 0 )->string, gi.cvar( "lwh_ship_personal_who", "", 0 )->string );
			step = 5;
		}
		if ( step == 5 && level.time >= 5200 ) { gi.SendConsoleCommand( "screenshot lwh_personal\n" ); step = 6; }
		if ( step == 6 && level.time >= 6000 ) { gi.SendConsoleCommand( "quit\n" ); step = 7; }
		return;
	}
	if ( g_shipTest->integer == 66 )
	{//Tactical's phaser setting (Task C): the decision the console carries when there is no contact.
	 //Open the Tactical console, cycle the setting by key, and show it changes what the bank asks of
	 //the power budget. Photographed before and after.
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 2000 ) { gi.SendConsoleCommand( "ui_lwh_station 1\n" ); step = 1; }
		if ( step == 1 && level.time >= 3200 ) { gi.SendConsoleCommand( "screenshot lwh_tactical_before\n" ); step = 2; }
		if ( step == 2 && level.time >= 3600 ) { gi.SendConsoleCommand( "lwh_eng_key v\n" ); step = 3; }
		if ( step == 3 && level.time >= 4400 )
		{
			const int base = ship::Spec( ship::SYS_PHASERS ).demand;
			const int now = ship::EffectiveDemand( vessel, ship::SYS_PHASERS );
			gi.Printf( "SHIP: tactical test: phaser bank %s, demand %d of %d (%d%%)\n",
				ship::PhaserYieldName( ship::PhaserYieldOf( vessel ) ), now, base, base ? now * 100 / base : 100 );
			ship::SetPhaserYield( vessel, ship::YIELD_STUN );
			const int low = ship::EffectiveDemand( vessel, ship::SYS_PHASERS );
			ship::SetPhaserYield( vessel, ship::YIELD_VAPORIZE );
			const int high = ship::EffectiveDemand( vessel, ship::SYS_PHASERS );
			gi.Printf( "SHIP: tactical test: stun asks %d, vaporize asks %d; vaporize asks %d more\n", low, high, high - low );
			Publish();
			step = 4;
		}
		if ( step == 4 && level.time >= 5200 ) { gi.SendConsoleCommand( "screenshot lwh_tactical_after\n" ); step = 5; }
		if ( step == 5 && level.time >= 6000 ) { gi.SendConsoleCommand( "quit\n" ); step = 6; }
		return;
	}
	if ( g_shipTest->integer == 67 )
	{//the triage board acts (Task C): it reported and could not act. Now it orders the treatment --
	 //the surgical field, the EMH, a captive's recovery -- and offers the triage order, which is
	 //command's: the board offers and the ship judges. Photographed before and after.
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 1800 ) { gi.SendConsoleCommand( "ship character Reyes 0 4\n" ); step = 1; }
		if ( step == 1 && level.time >= 2200 )
		{
			vessel.crew[3].status = ship::CREW_INJURED; vessel.crew[3].severity = 0.8f;
			vessel.crew[4].status = ship::CREW_INJURED; vessel.crew[4].severity = 0.4f;
			vessel.crew[5].status = ship::CREW_INJURED; vessel.crew[5].severity = 0.2f;
			vessel.crew[6].wounds = 0.5f; // in the Borg recovery window, not yet injured
			gi.SendConsoleCommand( "ui_lwh_triage\n" );
			step = 2;
		}
		if ( step == 2 && level.time >= 3200 ) { gi.SendConsoleCommand( "screenshot lwh_triage_before\n" ); step = 3; }
		if ( step == 3 && level.time >= 3600 ) { gi.SendConsoleCommand( "lwh_triage_key b\n" ); step = 4; }
		if ( step == 4 && level.time >= 4000 ) { gi.SendConsoleCommand( "lwh_triage_key e\n" ); step = 5; }
		if ( step == 5 && level.time >= 4400 ) { gi.SendConsoleCommand( "lwh_triage_key r\n" ); step = 6; }
		if ( step == 6 && level.time >= 4800 ) { gi.SendConsoleCommand( "set g_shipRole 1\nship role\n" ); step = 7; }
		if ( step == 7 && level.time >= 5100 ) { gi.SendConsoleCommand( "lwh_triage_key t\n" ); step = 8; }
		if ( step == 8 && level.time >= 5400 )
		{
			int cap = 0;
			for ( const ship::CrewMember &c : vessel.crew )
				if ( c.status != ship::CREW_DEAD && c.status != ship::CREW_ASSIMILATED && c.wounds > 0.0f && c.wounds < ship::RECOVERY_LIMIT ) ++cap;
			gi.Printf( "SHIP: triage test: surgical field %d, EMH %d, triage order %d, ward %d, recovery window %d\n",
				ship::SurgicalField( vessel ) ? 1 : 0, ship::EMHActive( vessel ) ? 1 : 0, vessel.orderTriage,
				static_cast<int>( ship::Patients( vessel ).size() ), cap );
			step = 9;
		}
		if ( step == 9 && level.time >= 5800 ) { gi.SendConsoleCommand( "screenshot lwh_triage_after\n" ); step = 10; }
		if ( step == 10 && level.time >= 6600 ) { gi.SendConsoleCommand( "quit\n" ); step = 11; }
		return;
	}
	if ( g_shipTest->integer == 68 )
	{//two axes (owner rulings, 2026-10-07): the STATION decides how many content types a console
	 //carries (earned by the job), and the PERSON decides what they can reach (the permission
	 //boundary, here the remote call-up). Tactical carries a read layer; Operations does not. From
	 //the single-type Conn console a commander still reaches a system stationed elsewhere; an
	 //uncleared ensign does not.
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 2000 ) { gi.SendConsoleCommand( "set g_shipRole 1\nship role\n" ); step = 1; }
		if ( step == 1 && level.time >= 2800 )
		{
			gi.Printf( "SHIP: axes test: reads for Tactical \"%s\", Ops \"%s\", Sickbay \"%s\"; Operations' portfolio \"%s\"\n",
				gi.cvar( "lwh_ship_reads1", "", 0 )->string, gi.cvar( "lwh_ship_reads2", "", 0 )->string,
				gi.cvar( "lwh_ship_reads4", "", 0 )->string, gi.cvar( "lwh_ship_port2", "", 0 )->string );
			gi.SendConsoleCommand( "ship as 3 on sensors\n" ); // Conn (single-type) reaches sensors, Operations'
			step = 2;
		}
		if ( step == 2 && level.time >= 3400 )
		{
			gi.Printf( "SHIP: axes test: Conn reaches sensors: %s (refusal \"%s\")\n",
				vessel.systems[ship::SYS_SENSORS].enabled ? "yes, operated" : "no",
				gi.cvar( "lwh_ship_refusal", "", 0 )->string );
			gi.SendConsoleCommand( "set g_shipRole 0\nship role\nship character Okoro 2 1\n" );
			step = 3;
		}
		if ( step == 3 && level.time >= 4000 ) { gi.SendConsoleCommand( "ship as 3 on sensors\n" ); step = 4; }
		if ( step == 4 && level.time >= 4600 )
		{
			gi.Printf( "SHIP: axes test: an uncleared ensign at Conn: \"%s\"\n", gi.cvar( "lwh_ship_refusal", "", 0 )->string );
			gi.SendConsoleCommand( "ui_lwh_station 3\n" );
			step = 5;
		}
		if ( step == 5 && level.time >= 5400 ) { gi.SendConsoleCommand( "screenshot lwh_conn_single\n" ); step = 6; }
		if ( step == 6 && level.time >= 6200 ) { gi.SendConsoleCommand( "quit\n" ); step = 7; }
		return;
	}
	if ( g_shipTest->integer == 69 )
	{//the month report editor and the job-queue board (rows 20 and 19, the two the inventory marked
	 //missing), and the two-lock on both: a post officer -- a created character never commands -- is
	 //refused, the refusal names whose the act is, and the state is unchanged; whoever commands
	 //edits the report (the lie and its direction) and sets the queue's order. Photographed locked
	 //and after. The screen's own cvars are read, not a model printf.
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 1800 )
		{
			ship::DamageSystem( vessel, ship::SYS_SHIELDS, 0.5f ); // so the queue has a repair job
			ship::BreachDeck( vessel, 9, 0.3f );                   // and a seal job
			gi.SendConsoleCommand( "ship character Reyes 2 4\n" ); // a security lieutenant commander: a post, not command
			step = 1;
		}
		if ( step == 1 && level.time >= 2400 ) { gi.SendConsoleCommand( "ship report read\nui_lwh_report\n" ); step = 2; }
		if ( step == 2 && level.time >= 3000 )
		{
			gi.Printf( "SHIP: report test: a post officer, may command %s\n", gi.cvar( "lwh_ship_may_command", "", 0 )->string );
			gi.SendConsoleCommand( "lwh_report_key s\n" ); // strike a line: refused
			step = 3;
		}
		if ( step == 3 && level.time >= 3600 )
		{
			int struck = 0;
			for ( const ship::ReportLine &l : ship::OpenReport( vessel ).lines ) if ( l.struck ) ++struck;
			gi.Printf( "SHIP: report test: the post officer's strike was refused: \"%s\"; struck lines %d\n",
				gi.cvar( "lwh_ship_report_refused", "", 0 )->string, struck );
			gi.SendConsoleCommand( "screenshot lwh_report_locked\n" ); // the locked report, still open
			step = 4;
		}
		if ( step == 4 && level.time >= 4200 ) { gi.SendConsoleCommand( "ui_lwh_jobs\nlwh_jobs_key right\n" ); step = 5; } // reorder: refused
		if ( step == 5 && level.time >= 4800 )
		{
			gi.Printf( "SHIP: jobs test: the post officer's reorder was refused: \"%s\"\n", gi.cvar( "lwh_ship_job_refused", "", 0 )->string );
			gi.SendConsoleCommand( "screenshot lwh_jobs_locked\n" );
			step = 6;
		}
		if ( step == 6 && level.time >= 5400 ) { gi.SendConsoleCommand( "set g_shipRole 1\nship role\nship report read\nui_lwh_report\n" ); step = 7; }
		if ( step == 7 && level.time >= 6000 )
		{
			gi.Printf( "SHIP: report test: in command, may command %s\n", gi.cvar( "lwh_ship_may_command", "", 0 )->string );
			gi.SendConsoleCommand( "lwh_report_key s\n" ); // strike the headline
			step = 8;
		}
		if ( step == 8 && level.time >= 6600 ) { gi.SendConsoleCommand( "lwh_report_key down\nlwh_report_key f\n" ); step = 9; }
		if ( step == 9 && level.time >= 7200 )
		{
			int struck = 0, softened = 0;
			for ( const ship::ReportLine &l : ship::OpenReport( vessel ).lines ) { if ( l.struck ) ++struck; else if ( l.text != l.draft ) ++softened; }
			gi.Printf( "SHIP: report test: in command, struck %d, softened %d; the diff is \"%s\"\n",
				struck, softened, gi.cvar( "lwh_ship_report_diff", "", 0 )->string );
			gi.SendConsoleCommand( "screenshot lwh_report\n" );
			step = 10;
		}
		if ( step == 10 && level.time >= 7800 ) { gi.SendConsoleCommand( "lwh_report_key enter\n" ); step = 11; } // sign to the crew
		if ( step == 11 && level.time >= 8400 )
		{
			const bool signedNow = !ship::Reports( vessel ).empty() && ship::Reports( vessel ).back().audience == ship::REPORT_TO_CREW;
			gi.Printf( "SHIP: report test: signed to the crew %s; a fresh draft is open %s\n",
				signedNow ? "yes" : "no", ship::OpenReport( vessel ).open ? "yes" : "no" );
			gi.SendConsoleCommand( "ui_lwh_jobs\nlwh_jobs_key right\n" );
			step = 12;
		}
		if ( step == 12 && level.time >= 9000 ) { gi.SendConsoleCommand( "lwh_jobs_key b\n" ); step = 13; } // order a build
		if ( step == 13 && level.time >= 9600 )
		{
			int build = 0;
			for ( const ship::Job &j : ship::Jobs( vessel ) ) if ( j.kind == ship::JOB_BUILD ) ++build;
			gi.Printf( "SHIP: jobs test: in command, %d job(s), %d build; the queue reads \"%s\"\n",
				static_cast<int>( ship::Jobs( vessel ).size() ), build, gi.cvar( "lwh_ship_jobs", "", 0 )->string );
			gi.SendConsoleCommand( "screenshot lwh_jobs\n" );
			step = 14;
		}
		if ( step == 14 && level.time >= 10200 ) { gi.SendConsoleCommand( "quit\n" ); step = 15; }
		return;
	}
	if ( g_shipTest->integer == 70 )
	{//the chart you can work (row 23), the tricorder survey (row 22) and the beacon's choices as
	 //keys on Operations (row 21). The chart sets a course; the survey spends the shared tricorder
	 //charge on the site or a compartment; the beacon block offers hail / trade / distress at
	 //Operations, where the ship holds the hailing channels. Photographed from the screens' own state.
		static int step = 0;
		if ( level.time < 1000 ) step = 0;
		if ( step == 0 && level.time >= 1800 ) { gi.SendConsoleCommand( "set g_shipRole 1\nship role\n" ); step = 1; }
		if ( step == 1 && level.time >= 2400 ) { gi.SendConsoleCommand( "ui_lwh_chart\n" ); step = 2; }
		if ( step == 2 && level.time >= 3000 )
		{
			gi.Printf( "SHIP: chart test: the chart reads \"%s\"\n", gi.cvar( "lwh_ship_sector_map", "", 0 )->string );
			gi.Printf( "SHIP: chart test: the forecast reads \"%s\"\n", gi.cvar( "lwh_ship_forecast", "", 0 )->string );
			gi.SendConsoleCommand( "screenshot lwh_chart\n" );
			step = 3;
		}
		if ( step == 3 && level.time >= 3600 ) { gi.SendConsoleCommand( "lwh_chart_key down\n" ); step = 4; }
		if ( step == 4 && level.time >= 4200 ) { gi.SendConsoleCommand( "lwh_chart_key enter\n" ); step = 5; }
		if ( step == 5 && level.time >= 4800 )
		{
			gi.Printf( "SHIP: chart test: setting the course by key left it at %s\n", gi.cvar( "lwh_ship_course", "", 0 )->string );
			gi.SendConsoleCommand( "ui_lwh_survey\n" );
			step = 6;
		}
		if ( step == 6 && level.time >= 5400 )
		{
			gi.Printf( "SHIP: survey test: the survey reads \"%s\"\n", gi.cvar( "lwh_ship_survey", "", 0 )->string );
			gi.SendConsoleCommand( "lwh_survey_key enter\n" ); // scan the site
			step = 7;
		}
		if ( step == 7 && level.time >= 6000 ) { gi.SendConsoleCommand( "lwh_survey_key down\nlwh_survey_key enter\n" ); step = 8; } // scan deck 1
		if ( step == 8 && level.time >= 6600 )
		{
			gi.Printf( "SHIP: survey test: the last reading is \"%s\"; the charge is %d%%\n",
				gi.cvar( "lwh_ship_survey_reading", "", 0 )->string,
				static_cast<int>( vessel.stores.tricorderCharge * 100 + 0.5f ) );
			gi.SendConsoleCommand( "screenshot lwh_survey\n" );
			step = 9;
		}
		if ( step == 9 && level.time >= 7200 ) { gi.SendConsoleCommand( "ui_lwh_station 2\n" ); step = 10; }
		if ( step == 10 && level.time >= 7800 )
		{
			gi.Printf( "SHIP: beacon test: at beacon %d the choices read \"%s\"\n", vessel.beacon, gi.cvar( "lwh_ship_beacon", "", 0 )->string );
			gi.SendConsoleCommand( "screenshot lwh_ops_beacon\n" );
			gi.SendConsoleCommand( "lwh_eng_key l\n" ); // hail, the first choice, by key at Operations
			step = 11;
		}
		if ( step == 11 && level.time >= 8400 )
		{
			gi.Printf( "SHIP: beacon test: hail by key wrote to the log: %s\n",
				gi.cvar( "lwh_ship_log", "", 0 )->string[0] ? "yes" : "no" );
			gi.SendConsoleCommand( "quit\n" );
			step = 12;
		}
		return;
	}
	if ( tested || level.time < 3000 ) return;
	tested = true;
	if ( g_shipTest->integer == 1 )
	{
		ship::SetAlert( vessel, ship::ALERT_RED );
		ship::DamageSystem( vessel, ship::SYS_SHIELDS, 0.4f );
		ship::DamageSource( vessel, ship::SRC_WARP_CORE, 0.5f );
		ship::SetPriority( vessel, ship::SYS_WARP_DRIVE, -1 );
		ship::BreachDeck( vessel, 9, 0.3f );
		ship::Tick( vessel, 0.0f );
		WriteReport( "ship/acted.txt" );
		gi.SendConsoleCommand( "save shiprun\n" );
	}
	else
	{
		WriteReport( "ship/restored.txt" );
	}
	gi.SendConsoleCommand( "wait 20\n" );
	gi.SendConsoleCommand( "quit\n" );
}

// Runtime asset replacement (S8's first hard problem): a deck the Borg hold turns Borg, and is
// stripped back. gi.RemapShader addresses a shader by name, so this works on the generated decks,
// whose wall and floor shaders are deck-unique (tools/shipmap/data/lwh_borg.shader). The published
// decks share shader names and cannot be addressed this way without a stitcher pass.
static void BorgAssets( ship::Ship *s )
{
	if ( !s ) return;
	static int turned[ship::DECKS] = {0}; // how many of a deck's sections are Borg now
	static std::vector<std::string> deckShaders[ship::DECKS];
	static bool loaded = false;
	if ( !loaded )
	{
		loaded = true;
		// The generated decks' shaders are known here (gendeck splits each into four sections); the
		// published decks' are in lwh_decks.txt, written by the stitcher and shipped in the pak. The
		// order is the order the sections turn Borg as the deck's assimilation rises.
		static const char *const DECK_SECTIONS[ship::DECKS][4] = {
			{ NULL, NULL, NULL, NULL },
			{ NULL, NULL, NULL, NULL },
			{ NULL, NULL, NULL, NULL },
			{ NULL, NULL, NULL, NULL },
			{ NULL, NULL, NULL, NULL },
			{ "textures/lwh/deck06wall0", "textures/lwh/deck06wall1", "textures/lwh/deck06wall2", "textures/lwh/deck06floor" },
			{ "textures/lwh/deck07wall0", "textures/lwh/deck07wall1", "textures/lwh/deck07wall2", "textures/lwh/deck07floor" },
			{ NULL, NULL, NULL, NULL },
			{ NULL, NULL, NULL, NULL },
			{ NULL, NULL, NULL, NULL },
			{ "textures/lwh/deck12wall0", "textures/lwh/deck12wall1", "textures/lwh/deck12wall2", "textures/lwh/deck12floor" },
			{ "textures/lwh/deck13wall0", "textures/lwh/deck13wall1", "textures/lwh/deck13wall2", "textures/lwh/deck13floor" },
			{ "textures/lwh/deck14wall0", "textures/lwh/deck14wall1", "textures/lwh/deck14wall2", "textures/lwh/deck14floor" },
			{ NULL, NULL, NULL, NULL },
		};
		for ( int d = 0; d < ship::DECKS; ++d )
			for ( int k = 0; k < 4; ++k )
				if ( DECK_SECTIONS[d][k] ) deckShaders[d].push_back( DECK_SECTIONS[d][k] );
		void *buf = NULL;
		const int len = gi.FS_ReadFile( "lwh_decks.txt", &buf );
		if ( len > 0 && buf )
		{
			const std::string text( static_cast<const char *>( buf ), len );
			size_t at = 0;
			while ( at < text.size() )
			{
				const size_t nl = text.find( '\n', at );
				const std::string line = text.substr( at, nl == std::string::npos ? std::string::npos : nl - at );
				at = nl == std::string::npos ? text.size() : nl + 1;
				const size_t colon = line.find( ':' );
				if ( line.empty() || line[0] == '#' || colon == std::string::npos ) continue;
				const int deck = atoi( line.substr( 0, colon ).c_str() );
				if ( deck < 1 || deck > ship::DECKS ) continue;
				std::string rest = line.substr( colon + 1 );
				size_t i = 0;
				while ( i < rest.size() )
				{
					while ( i < rest.size() && ( rest[i] == ' ' || rest[i] == '\t' || rest[i] == '\r' ) ) ++i;
					size_t j = i;
					while ( j < rest.size() && rest[j] != ' ' && rest[j] != '\t' && rest[j] != '\r' ) ++j;
					if ( j > i ) deckShaders[deck - 1].push_back( "textures/" + rest.substr( i, j - i ) );
					i = j;
				}
			}
			gi.FS_FreeFile( buf );
		}
	}
	for ( int d = 0; d < ship::DECKS; ++d )
	{
		if ( deckShaders[d].empty() ) continue;
		const int n = static_cast<int>( deckShaders[d].size() );
		// Sections turn in order as assimilation rises to ASSIMILATED, so the deck is taken part by
		// part, not all at once: "parts of the ship".
		int want = static_cast<int>( std::floor( s->decks[d].assimilated / ship::ASSIMILATED * n + 1e-4f ) );
		want = want < 0 ? 0 : ( want > n ? n : want );
		if ( want == turned[d] ) continue;
		for ( int k = 0; k < n; ++k )
		{
			const std::string &name = deckShaders[d][k];
			const bool borg = k < want;
			const bool was = k < turned[d];
			if ( borg && !was ) gi.RemapShader( name.c_str(), "textures/lwh/borg", "0" );
			// RE_RemapShader(name, name) clears the remap: the section is ours again.
			else if ( !borg && was ) gi.RemapShader( name.c_str(), name.c_str(), "0" );
		}
		if ( want == n ) gi.Printf( "SHIP: deck %d has turned Borg (%d of %d sections)\n", d + 1, want, n );
		else if ( want > 0 ) gi.Printf( "SHIP: deck %d is turning Borg (%d of %d sections)\n", d + 1, want, n );
		else gi.Printf( "SHIP: deck %d is ours again\n", d + 1 );
		turned[d] = want;
	}
}

// S10: the player's character is the body they walk in. The single-player player is Munro by
// default; when a character is chosen (or the Munro role taken) we set the player's head, torso and
// legs from the crew record -- the character's own face, and the uniform their department wears.
// The model change rides the game's own headModel/torsoModel/legsModel path (the one it uses for a
// disguise), and the Starfleet humanoid models share one animation set, so no anim reset is needed.
// Off unless the simulation is on and a character is chosen.
bool IsNamedType( const std::string &type )
{
	static const char *const NAMED_TYPES[] = { "janeway", "chakotay", "tuvok", "paris", "kim",
		"torres", "doctor", "seven", "neelix", "vorik", "munro" };
	for ( const char *n : NAMED_TYPES ) if ( type == n ) return true;
	return false;
}

void ApplyPlayerBody( void )
{
	if ( !active ) return;
	if ( vessel.player < 0 || vessel.player >= static_cast<int>( vessel.crew.size() ) ) return;
	bodyApplied = true;
	const ship::CrewMember &c = vessel.crew[vessel.player];

	const bool female = c.type == "janeway" || c.type == "torres" || c.type == "seven"
		|| ( c.type.size() >= 2 && c.type[1] == 'F' && ( c.type[0] == 'R' || c.type[0] == 'G' || c.type[0] == 'B' ) );
	const bool isCommand = c.dept == ship::DEPT_COMMAND;
	const bool isScience = c.dept == ship::DEPT_SCIENCES || c.dept == ship::DEPT_MEDICAL;
	// the female torso has no red skin; a woman of command wears the neutral cut
	const char *colour = isCommand ? ( female ? "default" : "red" ) : isScience ? "blue" : "gold";

	char head[64], torso[64], legs[64];
	if ( IsNamedType( c.type ) ) std::snprintf( head, sizeof( head ), "%s/default", c.type.c_str() );
	else std::snprintf( head, sizeof( head ), "%s", female ? "torres/default" : "munro/default" );
	std::snprintf( torso, sizeof( torso ), "%s/%s", female ? "crewfemale" : "crewthin", colour );
	std::snprintf( legs, sizeof( legs ), "%s/default", female ? "crewfemale" : "crewthin" );

	gi.SendConsoleCommand( Fmt( "headModel %s; torsoModel %s; legsModel %s\n", head, torso, legs ).c_str() );
	gi.Printf( "SHIP: %s walks as head %s, torso %s, legs %s\n", c.name.c_str(), head, torso, legs );
}

} // namespace

ship::Ship *Ship_Get( void ) { return active ? &vessel : NULL; }

// The player-in-the-world layer reapplies the body when command passes to a successor (Stage B).
void Ship_ApplyPlayerBody( void ) { ApplyPlayerBody(); }

void Ship_RegisterCvars( void )
{
	g_ship = gi.cvar( "g_ship", "0", 0 );
	g_shipDayScale = gi.cvar( "g_shipDayScale", "60", 0 );
	g_shipMode = gi.cvar( "g_shipMode", "0", 0 );
	g_shipClock = gi.cvar( "g_shipClock", "0", 0 );
	g_shipRole = gi.cvar( "g_shipRole", "0", 0 );
	g_shipTest = gi.cvar( "g_shipTest", "0", 0 );
	g_shipTestPos = gi.cvar( "g_shipTestPos", "0 0 0", 0 );
	g_shipTestPitch = gi.cvar( "g_shipDeckPitch", "0", 0 );
	g_shipTestWatch = gi.cvar( "g_shipTestWatch", "", 0 );
}

const int IRONMAN_SAVE_MS = 60000; // how often the one save is written forward
int nextIronmanSaveMs = 0;

void Ship_Init( void )
{
	active = g_ship && g_ship->integer;
	// The engine holds saves and loads to ironman while this is set (patches/0012).
	gi.cvar_set( "g_ironman", active && g_shipMode->integer != 1 ? "1" : "0" );
	nextIronmanSaveMs = IRONMAN_SAVE_MS;
	tested = false;
	bodyApplied = false;
	pendingSave.clear();
	if ( !active ) return;

	// Taking the turbolift is a level change, and the ship must come through it: the state written
	// at shutdown is read back here. A new game, or a load, starts from its own source instead.
	if ( g_qbLoadTransition )
	{
		void *buf = NULL;
		const int len = gi.FS_ReadFile( CARRY_FILE, &buf );
		const bool ok = len > 0 && buf && ship::Unpack( static_cast<const uint8_t *>( buf ), len, vessel );
		if ( buf ) gi.FS_FreeFile( buf );
		if ( ok )
		{
			gi.Printf( "SHIP: carried through the level change, day %d\n", vessel.Day() );
			return;
		}
		gi.Printf( S_COLOR_YELLOW"SHIP: no ship state carried through the level change; starting a new ship\n" );
	}
	ship::Config cfg;
	if ( g_shipDayScale->value > 0.0f ) cfg.dayScale = g_shipDayScale->value;
	cfg.mode = g_shipMode->integer == 1 ? ship::MODE_HOLODECK : ship::MODE_IRONMAN;
	cfg.clockMode = g_shipClock->integer == 1 ? ship::CLOCK_REAL_TIME : g_shipClock->integer == 2 ? ship::CLOCK_WALL : ship::CLOCK_ACCELERATED;
	vessel = ship::NewShip( cfg );
	ship::SetRole( vessel, g_shipRole->integer == 1 ? ship::ROLE_IN_COMMAND : g_shipRole->integer == 2 ? ship::ROLE_MUNRO : ship::ROLE_ANY_POST );
	gi.Printf( "SHIP: simulation active, %d crew, a day every %.0f minutes\n",
		static_cast<int>( vessel.crew.size() ), ship::SECONDS_PER_DAY / cfg.dayScale / 60.0f );
}

void Ship_Frame( void )
{
	if ( !active ) return;
	// S10: once a character is the player, the body follows. Applied after the world exists (a
	// console command needs a live client), once per map, and again whenever the character changes.
	if ( !bodyApplied && vessel.player >= 0 )
	{
		bodyApplied = true;
		ApplyPlayerBody();
	}
	if ( !pendingSave.empty() )
	{
		if ( ship::Unpack( pendingSave.data(), pendingSave.size(), vessel ) )
		{
			gi.Printf( "SHIP: restored from the save, day %d\n", vessel.Day() );
			// Under the wall clock the ship lived on while the game was closed.
			const uint64_t now = static_cast<uint64_t>( time( NULL ) );
			if ( vessel.cfg.clockMode == ship::CLOCK_WALL && vessel.wallSeconds && now > vessel.wallSeconds )
			{
				ship::CatchUp( vessel, static_cast<double>( now - vessel.wallSeconds ) );
				gi.Printf( "SHIP: %.1f hours passed aboard while you were away; it is now day %d\n",
					( now - vessel.wallSeconds ) / 3600.0, vessel.Day() );
			}
		}
		else
			gi.Printf( S_COLOR_YELLOW"SHIP: the save's ship state could not be read; keeping a new ship\n" );
		pendingSave.clear();
	}
	const int ms = level.time - level.previousTime;
	if ( ms > 0 && ms < 1000 ) ship::Tick( vessel, ms / 1000.0f );
	// The alert klaxon (S9's "hear"): the game ships a red-alert and an alarm sound; play one when the
	// condition changes, around the player.
	static int lastAlert = -1;
	if ( vessel.alert != lastAlert ) {
		const int was = lastAlert;
		lastAlert = vessel.alert;
		if ( was >= 0 ) {
			if ( vessel.alert == ship::ALERT_RED ) G_Sound( &g_entities[0], G_SoundIndex( "sound/ambience/voyager/redalert.mp3" ) );
			else if ( vessel.alert == ship::ALERT_YELLOW ) G_Sound( &g_entities[0], G_SoundIndex( "sound/ambience/voyager/alarm1.mp3" ) );
		}
	}
	if ( level.time / 250 != level.previousTime / 250 ) Publish();
	LWH_Panel_Frame( &vessel ); // the status panel's live surface (S4's glance, second half)
	BorgAssets( &vessel );      // the Borg swap the ship's state onto the generated decks (S8)
	if ( !ship::SavesAllowed( vessel.cfg ) && level.time >= nextIronmanSaveMs && !g_shipTest->integer )
	{//ironman: the ship is saved for you, forward only
		nextIronmanSaveMs = level.time + IRONMAN_SAVE_MS;
		gi.SendConsoleCommand( "save ironman\n" );
	}
	if ( g_shipTest->integer ) RunTest();
}

void Ship_Shutdown( void )
{
	if ( !active ) return;
	std::vector<uint8_t> blob = ship::Pack( vessel );
	WriteFile( CARRY_FILE, blob.data(), static_cast<int>( blob.size() ) );
}

void Ship_WriteSave( void )
{
	if ( !active ) return;
	vessel.wallSeconds = static_cast<uint64_t>( time( NULL ) );
	std::vector<uint8_t> blob = ship::Pack( vessel );
	gi.AppendToSaveGame( SAVE_CHUNK, blob.data(), static_cast<int>( blob.size() ) );
}

// Always attempted: a save made with the ship on must still load with it off.
void Ship_ReadSave( void )
{
	void *data = NULL;
	const int len = gi.ReadFromSaveGameOptional( SAVE_CHUNK, NULL, 0, &data );
	if ( len > 0 && data && active )
	{
		const uint8_t *bytes = static_cast<const uint8_t *>( data );
		pendingSave.assign( bytes, bytes + len );
	}
	if ( data ) gi.Free( data );
}
void Svcmd_Ship_f( void )
{
	if ( !active )
	{
		gi.Printf( "ship: the simulation is off (set g_ship 1 and load a map)\n" );
		return;
	}
	// A console says which station it is: "ship as <station> <command...>". What follows is then
	// held to that station's authority and to the player's clearance. Typed bare at the game's own
	// console the commands are a developer's, and unrestricted.
	int first = 1, station = -1;
	if ( gi.argc() > 2 && !Q_stricmp( gi.argv( 1 ), "as" ) )
	{
		station = atoi( gi.argv( 2 ) );
		first = 3;
		if ( station < 0 || station >= ship::STN_COUNT ) return;
	}
	const char *cmd = gi.argc() > first ? gi.argv( first ) : "status";
	const char *a = gi.argc() > first + 1 ? gi.argv( first + 1 ) : "";
	const char *b = gi.argc() > first + 2 ? gi.argv( first + 2 ) : "";
	const int sys = FindSystem( a );

	if ( station >= 0 )
	{
		const ship::Station st = static_cast<ship::Station>( station );
		std::string why;
		const bool isAlert = !Q_stricmp( cmd, "alert" );
		const bool isSwitch = !Q_stricmp( cmd, "on" ) || !Q_stricmp( cmd, "off" );
		const bool isPriority = !Q_stricmp( cmd, "priority" );
		// The player's own allocation of power (docs/power-assignment.md): the FTL surface, and
		// Engineering's to work, alongside the ladder the automatic mode uses.
		const bool isAlloc = !Q_stricmp( cmd, "alloc" ) || !Q_stricmp( cmd, "auto" ) || !Q_stricmp( cmd, "recommend" )
			|| !Q_stricmp( cmd, "accept" ) || !Q_stricmp( cmd, "refuse" ) || !Q_stricmp( cmd, "band" );
		const bool isFire = !Q_stricmp( cmd, "fire" );
		const bool isJump = !Q_stricmp( cmd, "jump" );
		const bool isBreach = !Q_stricmp( cmd, "breach" ) || !Q_stricmp( cmd, "solve" );
		const bool isTransport = !Q_stricmp( cmd, "transport" ) || !Q_stricmp( cmd, "recall" );
		const bool isSurvey = !Q_stricmp( cmd, "survey" );
		const bool isCourse = !Q_stricmp( cmd, "course" );
		const bool isPatients = !Q_stricmp( cmd, "patients" );
		const bool isField = !Q_stricmp( cmd, "field" );
		const bool isSurgical = !Q_stricmp( cmd, "surgical" );
		const bool isScanComp = !Q_stricmp( cmd, "scancomp" ) || !Q_stricmp( cmd, "scan" );
		const bool isTarget = !Q_stricmp( cmd, "target" );
		const bool isYield = !Q_stricmp( cmd, "yield" );
		const bool isChoice = !Q_stricmp( cmd, "hail" ) || !Q_stricmp( cmd, "trade" ) || !Q_stricmp( cmd, "distress" )
			|| !Q_stricmp( cmd, "mine" ) || !Q_stricmp( cmd, "survivors" ) || !Q_stricmp( cmd, "holo" )
			|| !Q_stricmp( cmd, "eva" ) || !Q_stricmp( cmd, "observe" ) || !Q_stricmp( cmd, "interfere" );
		const bool isRun = !Q_stricmp( cmd, "run" ) || !Q_stricmp( cmd, "cross" );
		const bool isTractor = !Q_stricmp( cmd, "tractor" );
		const bool isFabricate = !Q_stricmp( cmd, "fabricate" );
		const bool isEMH = !Q_stricmp( cmd, "emh" );
		// until a character is chosen the player is nobody in particular, and is not held to a rank
		const bool anyone = vessel.player < 0 && vessel.cfg.role != ship::ROLE_IN_COMMAND;
		if ( !isAlert && !isSwitch && !isPriority && !isAlloc && !isFire && !isJump && !isBreach && !isTransport && !isSurvey && !isCourse && !isPatients && !isField && !isSurgical && !isScanComp && !isTarget && !isYield && !isChoice && !isRun && !isTractor && !isFabricate && !isEMH )
			why = "that is not a console's to do";
		else if ( !anyone && !ship::PlayerMayOperate( vessel, st ) )
		{
			// The console that has stopped answering names the person locked out, and a locked control
			// names who can open it (docs/access-and-authority.md).
			const std::string lock = ship::LockoutNotice( vessel, vessel.player, st );
			why = lock.empty() ? ship::AccessRefusal( vessel, st ) : std::string( "LOCKED OUT: " ) + lock;
		}
		else if ( isAlert && !( st == ship::STN_ENGINEERING || st == ship::STN_TACTICAL ) ) why = "the alert is not called from this station";
		else if ( isAlert && !anyone && vessel.cfg.role != ship::ROLE_IN_COMMAND && !ship::MayCallAlert( vessel, vessel.player, st ) )
			why = "calling the alert needs a lieutenant or above";
		else if ( isPriority && st != ship::STN_ENGINEERING ) why = "the power order is Engineering's to set";
		else if ( isAlloc && st != ship::STN_ENGINEERING ) why = "power allocation is Engineering's to set";
		else if ( isFire && st != ship::STN_TACTICAL ) why = "weapons are fired from Tactical";
		else if ( isJump && st != ship::STN_CONN ) why = "the ship is flown from the Conn";
		else if ( isCourse && st != ship::STN_CONN ) why = "the course is laid in from the Conn";
		else if ( isTransport && st != ship::STN_OPS ) why = "the transporter is worked from Operations";
		else if ( isSurvey && st != ship::STN_OPS ) why = "the sensors are read from Operations";
		else if ( isPatients && st != ship::STN_SICKBAY ) why = "the ward is read from Sickbay";
		else if ( isField && st != ship::STN_OPS ) why = "environmental control is worked from Operations";
		else if ( isSurgical && st != ship::STN_SICKBAY ) why = "the surgical bay is Sickbay's";
		else if ( isScanComp && st != ship::STN_OPS ) why = "the tricorder is read from Operations";
		else if ( isTarget && st != ship::STN_TACTICAL ) why = "targets are picked at Tactical";
		else if ( isYield && st != ship::STN_TACTICAL ) why = "the phaser setting is Tactical's";
		else if ( isChoice && st != ship::STN_OPS ) why = "the hailing and trade channels are Operations'";
		else if ( isRun && st != ship::STN_CONN ) why = "the ship is flown from the Conn";
		else if ( isTractor && st != ship::STN_TACTICAL ) why = "the tractor beam is worked from Tactical";
		else if ( isFabricate && st != ship::STN_ENGINEERING ) why = "fabrication is Engineering's";
		else if ( isEMH && st != ship::STN_SICKBAY ) why = "the EMH is Sickbay's";
		else if ( ( !Q_stricmp( cmd, "breach" ) || isSwitch || isPriority || !Q_stricmp( cmd, "alloc" ) ) && sys < 0 )
			why = "no such system";
		else if ( ( !Q_stricmp( cmd, "breach" ) || isSwitch || isPriority || !Q_stricmp( cmd, "alloc" ) )
			&& !ship::MayCallUp( vessel, vessel.player, st, static_cast<ship::SystemId>( sys ) ) )
			why = ship::OperatedFromRefusal( static_cast<ship::SystemId>( sys ) );
		else if ( ( isSwitch || isPriority || !Q_stricmp( cmd, "alloc" ) ) && ship::Hijacked( vessel, static_cast<ship::SystemId>( sys ) ) )
			why = std::string( ship::Spec( static_cast<ship::SystemId>( sys ) ).name ) + " does not answer: it is not ours";
		if ( !why.empty() )
		{
			gi.Printf( "SHIP: %s refused at %s: %s\n", cmd, ship::StationName( st ), why.c_str() );
			gi.cvar_set( "lwh_ship_refusal", why.c_str() );
			return;
		}
		// A remote call-up: command travels to a console away from the system, and the record says so.
		// The work itself does not travel: a repair, a seal or a valve still needs a hand there.
		if ( ( !Q_stricmp( cmd, "breach" ) || isSwitch || isPriority ) && sys >= 0 && !anyone
			&& !ship::OperatedFrom( static_cast<ship::SystemId>( sys ), st ) )
			gi.Printf( "SHIP: remote call-up: %s operated from %s\n",
				ship::Spec( static_cast<ship::SystemId>( sys ) ).name, ship::StationName( st ) );
		gi.cvar_set( "lwh_ship_refusal", "" );
	}

	if ( !Q_stricmp( cmd, "status" ) ) { PrintStatus(); return; }
	if ( !Q_stricmp( cmd, "role" ) )
	{//take up the role g_shipRole names (0 any post, 1 in command, 2 Munro)
		ship::SetRole( vessel, g_shipRole->integer == 1 ? ship::ROLE_IN_COMMAND : g_shipRole->integer == 2 ? ship::ROLE_MUNRO : ship::ROLE_ANY_POST );
		ApplyPlayerBody();
		Publish();
		return;
	}
	if ( !Q_stricmp( cmd, "body" ) )
	{//put the player's character in the body their crew record and department say (S10)
		ApplyPlayerBody();
		return;
	}
	if ( !Q_stricmp( cmd, "crew" ) )
	{//who the ship says is on a deck now -- the people S5 will embody there
		static const char *const DOING[] = { "on duty", "at a meal", "at recreation", "personal time", "asleep" };
		const int deck = atoi( a );
		const std::vector<int> aboard = ship::CrewOnDeck( vessel, deck );
		gi.Printf( "SHIP: deck %d: %d crew\n", deck, static_cast<int>( aboard.size() ) );
		for ( int i : aboard )
			gi.Printf( "SHIP:   %-18s %-10s %s\n", vessel.crew[i].name.c_str(), vessel.crew[i].type.c_str(), DOING[vessel.crew[i].activity] );
		return;
	}
	if ( !Q_stricmp( cmd, "console" ) ) { gi.SendConsoleCommand( "ui_lwh_engineering\n" ); return; }
	if ( !Q_stricmp( cmd, "alert" ) )
	{
		const ship::Alert al = !Q_stricmp( a, "red" ) ? ship::ALERT_RED : !Q_stricmp( a, "yellow" ) ? ship::ALERT_YELLOW : ship::ALERT_GREEN;
		ship::SetAlert( vessel, al );
	}
	else if ( !Q_stricmp( cmd, "on" ) && sys >= 0 ) ship::SetEnabled( vessel, static_cast<ship::SystemId>( sys ), true );
	else if ( !Q_stricmp( cmd, "off" ) && sys >= 0 ) ship::SetEnabled( vessel, static_cast<ship::SystemId>( sys ), false );
	else if ( !Q_stricmp( cmd, "priority" ) && sys >= 0 && b[0] ) ship::SetPriority( vessel, static_cast<ship::SystemId>( sys ), atoi( b ) );
	else if ( !Q_stricmp( cmd, "alloc" ) && sys >= 0 && b[0] )
	{//the player sets the system's share of its demand: the mechanism (docs/power-assignment.md)
		if ( !ship::SetAllocation( vessel, static_cast<ship::SystemId>( sys ), atoi( b ) ) )
		{
			gi.cvar_set( "lwh_ship_refusal", "refused: that would commit more power than the plant supplies" );
			gi.Printf( "SHIP: allocation refused: it would oversubscribe the plant\n" );
		}
		Publish();
	}
	else if ( !Q_stricmp( cmd, "auto" ) )
	{//automatic mode: the ladder is the policy, off by default
		const bool on = !Q_stricmp( a, "on" ) ? true : !Q_stricmp( a, "off" ) ? false : !ship::PowerAuto( vessel );
		ship::SetPowerAuto( vessel, on );
		gi.Printf( "SHIP: automatic power allocation is %s\n", ship::PowerAuto( vessel ) ? "on" : "off" );
		Publish();
	}
	else if ( !Q_stricmp( cmd, "recommend" ) )
	{//the chief engineer's recommendation, with his reasoning
		const ship::Recommendation rec = ship::RecommendAllocation( vessel );
		gi.Printf( "SHIP: %s\n", rec.reasoning.c_str() );
		Publish();
	}
	else if ( !Q_stricmp( cmd, "accept" ) ) { if ( ship::AcceptRecommendation( vessel ) ) gi.Printf( "SHIP: the chief's allocation is adopted\n" ); Publish(); }
	else if ( !Q_stricmp( cmd, "refuse" ) ) { if ( ship::RefuseRecommendation( vessel ) ) gi.Printf( "SHIP: the chief's allocation is refused, and he notes it\n" ); Publish(); }
	else if ( !Q_stricmp( cmd, "band" ) )
	{//band grant|revoke <band> <officer> (docs/power-assignment.md, Task C)
		const int band = atoi( b );
		const int officer = gi.argc() > first + 3 ? atoi( gi.argv( first + 3 ) ) : -1;
		const int grantor = vessel.player >= 0 ? vessel.player : 0;
		if ( !Q_stricmp( a, "revoke" ) ) ship::RevokeBand( vessel, officer, static_cast<ship::BudgetBand>( band ) );
		else ship::GrantBand( vessel, grantor, officer, static_cast<ship::BudgetBand>( band ) );
		Publish();
	}
	else if ( !Q_stricmp( cmd, "damage" ) && sys >= 0 && b[0] ) ship::DamageSystem( vessel, static_cast<ship::SystemId>( sys ), atof( b ) );
	else if ( !Q_stricmp( cmd, "repair" ) && sys >= 0 && b[0] ) ship::Repair( vessel, static_cast<ship::SystemId>( sys ), atof( b ) );
	else if ( !Q_stricmp( cmd, "operate" ) && sys >= 0 )
	{//the player works a console (Stage B): the odds are rolled with the player as the operator, so a
	 //degraded system lets go at the person holding the controls. Needs the player-in-the-world layer.
		if ( !Crew_PlayerUseSystem( sys ) )
			gi.Printf( "SHIP: nothing to operate it: g_player is off, or no character is the player\n" );
		else
			gi.Printf( "SHIP: %s worked by %s\n", ship::Spec( static_cast<ship::SystemId>( sys ) ).name,
				vessel.player >= 0 ? vessel.crew[vessel.player].name.c_str() : "the hand on duty" );
	}
	else if ( !Q_stricmp( cmd, "breach" ) && a[0] && b[0] ) ship::BreachDeck( vessel, atoi( a ), atof( b ) );
	else if ( !Q_stricmp( cmd, "seal" ) && a[0] ) ship::RepairDeck( vessel, atoi( a ), 1.0f );
	else if ( !Q_stricmp( cmd, "ignite" ) && a[0] && b[0] ) ship::IgniteDeck( vessel, atoi( a ), atof( b ) );
	else if ( !Q_stricmp( cmd, "field" ) && a[0] )
	{//ship field <deck> <level 1-10 | on | off>
		if ( b[0] >= '0' && b[0] <= '9' ) ship::SetForceFieldLevel( vessel, atoi( a ), atoi( b ) );
		else ship::SetForceField( vessel, atoi( a ), !Q_stricmp( b, "on" ) );
	}
	else if ( !Q_stricmp( cmd, "boots" ) )
	{//the way back out of freefall: standard gravity for the player until the plating holds again
		Crew_ToggleBoots();
		gi.Printf( "SHIP: magnetic boots are %s\n", Crew_BootsOn() ? "on; standard gravity for you" : "off" );
		return;
	}
	else if ( !Q_stricmp( cmd, "surgical" ) )
	{
		ship::SetSurgicalField( vessel, !Q_stricmp( b, "on" ) || ( !b[0] && !ship::SurgicalField( vessel ) ) );
		gi.Printf( "SHIP: the surgical bay's force field is %s\n", ship::SurgicalField( vessel ) ? "up" : "down" );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "scancomp" ) && a[0] )
	{
		const std::string reading = ship::ScanCompartment( vessel, atoi( a ) );
		gi.Printf( "SHIP: %s\n", reading.c_str() );
		gi.cvar_set( "lwh_ship_survey_reading", reading.c_str() ); // the survey screen draws the last reading
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "captain" ) )
	{//the captain's log: read the summary, or write an entry in the captain's name
		if ( a[0] )
		{
			if ( !ship::PlayerMayCommand( vessel ) ) { gi.Printf( "SHIP: the captain's log is the captain's\n" ); return; }
			std::string text;
			for ( int i = first + 1; i < gi.argc(); ++i ) { if ( i > first + 1 ) text += " "; text += gi.argv( i ); }
			ship::LogEvent( vessel, ship::CommandingOfficer( vessel ), "captain", text );
			gi.Printf( "SHIP: the captain's log records: %s\n", text.c_str() );
		}
		else
		{
			const std::string summary = ship::CaptainLog( vessel );
			gi.Printf( "SHIP: --- the captain's log ---\n" );
			gi.Printf( "SHIP: %s", summary.c_str() );
		}
		return;
	}
	else if ( !Q_stricmp( cmd, "kit" ) && gi.argc() > first + 4 )
		ship::LoadAwayKit( vessel, atoi( a ), atoi( b ), atoi( gi.argv( first + 3 ) ), atof( gi.argv( first + 4 ) ) );
	else if ( !Q_stricmp( cmd, "scan" ) )
	{
		const int r = ship::Scan( vessel, vessel.beacon );
		gi.Printf( "SHIP: scan of beacon %d: %s\n", vessel.beacon,
			r == 1 ? "clean reading" : r == 2 ? "suspect reading" : "none" );
		gi.cvar_set( "lwh_ship_survey_reading", Fmt( "the site at beacon %d: %s", vessel.beacon,
			r == 1 ? "clean reading" : r == 2 ? "suspect reading" : "the tricorder is dead" ).c_str() );
		const int ph = ship::RevealPhenomenon( vessel ); // a scan resolves one attribute of a phenomenon
		if ( ph > 0 ) gi.Printf( "SHIP: a phenomenon: %d of %d attributes resolved\n", ph, ship::PHENOM_ATTR_COUNT );
		Publish();
	}
	else if ( !Q_stricmp( cmd, "transport" ) )
	{//the transporter: beam a party to the site the ship is at. The instrument states the
	 //system's condition before the act, so the operator has the risk before them.
		gi.Printf( "SHIP: transmitter: %s\n", ship::TransporterConditionLine( vessel ).c_str() );
		std::string outcome;
		if ( !ship::TransportAway( vessel, a[0] ? atoi( a ) : 3, &outcome ) )
			gi.Printf( "SHIP: no beam (a party is away, the transporters are down, or the shields are up)\n" );
		else
		{
			gi.Printf( "SHIP: away team of %d beamed down\n", ship::AwayTeam( vessel ) );
			if ( !outcome.empty() ) gi.Printf( "SHIP: the transporter: %s\n", outcome.c_str() );
		}
	}
	else if ( !Q_stricmp( cmd, "recall" ) )
	{
		gi.Printf( "SHIP: transmitter: %s\n", ship::TransporterConditionLine( vessel ).c_str() );
		std::string outcome;
		if ( !ship::TransportBack( vessel, &outcome ) ) gi.Printf( "SHIP: nobody is away\n" );
		else
		{
			gi.Printf( "SHIP: the away team is back aboard\n" );
			if ( !outcome.empty() ) gi.Printf( "SHIP: the transporter: %s\n", outcome.c_str() );
		}
	}
	else if ( !Q_stricmp( cmd, "survey" ) )
	{
		const int n = ship::Survey( vessel );
		gi.Printf( "SHIP: astrometrics: %d new reading(s)\n", n );
	}
	else if ( !Q_stricmp( cmd, "study" ) && a[0] )
	{//respond to a phenomenon: 0 shield harmonics, 1 warp geometry, 2 distance, 3 do not touch
		if ( !ship::RespondPhenomenon( vessel, atoi( a ) ) )
			gi.Printf( "SHIP: the phenomenon did not answer to that (0 harmonics | 1 geometry | 2 distance | 3 do not touch)\n" );
		else gi.Printf( "SHIP: the phenomenon answers; material %.0f\n", vessel.stores.materials );
		Publish();
	}
	else if ( !Q_stricmp( cmd, "course" ) && a[0] )
	{
		if ( !ship::SetCourse( vessel, atoi( a ) ) ) gi.Printf( "SHIP: no course to beacon %s\n", a );
		else
		{
			const std::vector<int> route = ship::PlotCourse( vessel, vessel.course );
			std::string r;
			for ( int b : route ) r += Fmt( " %d", b );
			gi.Printf( "SHIP: course to beacon %d:%s\n", vessel.course, r.c_str() );
		}
	}
	else if ( !Q_stricmp( cmd, "patients" ) )
	{//sickbay's ward: one row per casualty, in the order triage treats them
		const std::vector<int> ward = ship::Patients( vessel );
		gi.Printf( "SHIP: %s  (%d in the ward, %d beds)\n", vessel.orderTriage == 1 ? "TRIAGE: RANK FIRST" : "TRIAGE: WORST FIRST",
			static_cast<int>( ward.size() ), ship::SICKBAY_BEDS );
		for ( int i : ward )
		{
			const ship::CrewMember &c = vessel.crew[i];
			gi.Printf( "SHIP:   %-18s severity %2d%%  %s\n", c.name.c_str(), static_cast<int>( c.severity * 100 + 0.5f ),
				c.underCare ? "ON A BED" : "WAITING" );
		}
		return;
	}
	else if ( !Q_stricmp( cmd, "board" ) && a[0] && b[0] )
	{//ship board <deck> <n> [raider|borg|hunter] [objective deck]
		ship::BoarderKind kind = ship::BOARDER_RAIDER;
		int objective = 0;
		if ( gi.argc() > first + 3 )
		{
			const char *k = gi.argv( first + 3 );
			if ( !Q_stricmpn( k, "borg", 4 ) ) kind = ship::BOARDER_BORG;
			else if ( !Q_stricmpn( k, "hunt", 4 ) ) kind = ship::BOARDER_HUNTER;
		}
		if ( gi.argc() > first + 4 ) objective = atoi( gi.argv( first + 4 ) );
		ship::BoardAs( vessel, atoi( a ), atoi( b ), kind, objective );
	}
	else if ( !Q_stricmp( cmd, "borg" ) && a[0] && b[0] ) ship::BoardBorg( vessel, atoi( a ), atoi( b ) );
	else if ( !Q_stricmp( cmd, "order" ) && a[0] )
	{//ship order repair <system>|security <deck>|evacuate <deck>   (0 or "none" clears it)
		bool ok = false;
		if ( !Q_stricmp( a, "repair" ) ) ok = ship::OrderRepairFirst( vessel, FindSystem( b ) );
		else if ( !Q_stricmp( a, "security" ) ) ok = ship::OrderSecurityTo( vessel, atoi( b ) );
		else if ( !Q_stricmp( a, "evacuate" ) ) ok = ship::OrderEvacuate( vessel, atoi( b ) );
		else if ( !Q_stricmp( a, "triage" ) ) ok = ship::OrderTriage( vessel, !Q_stricmpn( b, "rank", 4 ) ? 1 : 0 );
		gi.cvar_set( "lwh_ship_order_refused", ok ? "" : "only whoever commands the ship gives orders" );
		if ( !ok ) { gi.Printf( "SHIP: order refused: only whoever commands the ship gives orders\n" ); return; }
		gi.Printf( "SHIP: standing orders: repair first %s, security to deck %d, evacuate deck %d\n",
			vessel.orderRepairFirst >= 0 ? ship::Spec( static_cast<ship::SystemId>( vessel.orderRepairFirst ) ).name : "nothing in particular",
			vessel.orderSecurityTo, vessel.orderEvacuate );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "delegate" ) && a[0] && b[0] && gi.argc() > first + 3 )
	{//ship delegate <grantor> <grantee> <station>: a department head grants a shift's access
		const int grantor = atoi( a ), grantee = atoi( b ), stn = atoi( gi.argv( first + 3 ) );
		if ( !ship::Delegate( vessel, grantor, grantee, static_cast<ship::Station>( stn ) ) )
			gi.Printf( "SHIP: no delegation (bad hands, below a department head, or a bad station 0-4)\n" );
		else gi.Printf( "SHIP: delegation recorded\n" );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "revoke" ) && a[0] && b[0] && gi.argc() > first + 3 )
	{//ship revoke <revoker> <grantee> <station>
		const int revoker = atoi( a ), grantee = atoi( b ), stn = atoi( gi.argv( first + 3 ) );
		if ( !ship::RevokeDelegation( vessel, revoker, grantee, static_cast<ship::Station>( stn ) ) )
			gi.Printf( "SHIP: nothing to revoke (no live grant, or below a department head)\n" );
		else gi.Printf( "SHIP: delegation revoked\n" );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "credrevoke" ) && a[0] && b[0] && gi.argc() > first + 3 )
	{//ship credrevoke <revoker> <holder> <station>: the crew can take back a qualification
		const int revoker = atoi( a ), holder = atoi( b ), stn = atoi( gi.argv( first + 3 ) );
		if ( !ship::RevokeCredential( vessel, revoker, holder, static_cast<ship::Station>( stn ) ) )
			gi.Printf( "SHIP: nothing to revoke (no such qualification, or below a department head)\n" );
		else gi.Printf( "SHIP: qualification revoked\n" );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "lockout" ) && a[0] && b[0] && gi.argc() > first + 3 )
	{//ship lockout <officer> <locked> <station>: a senior officer shuts a post-holder out
		const int officer = atoi( a ), locked = atoi( b ), stn = atoi( gi.argv( first + 3 ) );
		if ( !ship::LockOut( vessel, officer, static_cast<ship::Station>( stn ), locked ) )
			gi.Printf( "SHIP: no lock-out (must be above the post-holder, at their station)\n" );
		else gi.Printf( "SHIP: lock-out recorded\n" );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "unlock" ) && a[0] && b[0] && gi.argc() > first + 3 )
	{//ship unlock <officer> <locked> <station>
		const int officer = atoi( a ), locked = atoi( b ), stn = atoi( gi.argv( first + 3 ) );
		if ( !ship::ClearLockout( vessel, officer, static_cast<ship::Station>( stn ), locked ) )
			gi.Printf( "SHIP: no such lock-out\n" );
		else gi.Printf( "SHIP: lock-out cleared\n" );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "override" ) && a[0] && b[0] )
	{//ship override begin|confirm <station>: the emergency override, slow and logged
		const ship::Station stn = static_cast<ship::Station>( atoi( b ) );
		const bool chosen = vessel.player >= 0 && vessel.player < static_cast<int>( vessel.crew.size() );
		if ( !Q_stricmp( a, "begin" ) )
		{
			if ( !chosen ) { gi.Printf( "SHIP: no character is the player\n" ); return; }
			if ( !ship::BeginOverride( vessel, vessel.player, stn ) ) gi.Printf( "SHIP: the override cannot begin\n" );
			else gi.Printf( "SHIP: override begun; a second officer must agree, or one hand may force it\n" );
		}
		else if ( !Q_stricmp( a, "confirm" ) )
		{
			// The second hand: the senior fit officer who is not the one who began it. [inv]
			int second = -1;
			for ( int i = 0; i < static_cast<int>( vessel.crew.size() ); ++i )
				if ( vessel.crew[i].status == ship::CREW_FIT && !vessel.crew[i].brigged && i != ship::OverrideState( vessel ).first
					&& ( second < 0 || vessel.crew[i].rank > vessel.crew[second].rank ) ) second = i;
			if ( second < 0 || !ship::ConfirmOverride( vessel, second, stn ) ) gi.Printf( "SHIP: no second officer can agree\n" );
			else gi.Printf( "SHIP: the override is agreed; it will take shortly\n" );
		}
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "character" ) && a[0] && b[0] && gi.argc() > 4 )
	{//ship character <name> <department 0-4> <rank 0-4>
		const int who = ship::CreateCharacter( vessel, a, static_cast<ship::Department>( atoi( b ) ), atoi( gi.argv( 4 ) ) );
		if ( who < 0 ) gi.Printf( "SHIP: no such character can be created (department 0-4: command, engineering, security, sciences, medical; rank 0-4)\n" );
		else
		{
			gi.Printf( "SHIP: you are %s, crew number %d\n", vessel.crew[who].name.c_str(), who );
			ApplyPlayerBody();
		}
		return;
	}
	else if ( !Q_stricmp( cmd, "breach" ) && sys >= 0 )
	{//start a counter-hack at a console: publish a puzzle for the screen to present
		breachSystem = sys;
		breach = ship::MakeBreach( static_cast<uint32_t>( level.time ) * 2654435761u + sys );
		std::string grid, targets;
		for ( const std::string &code : breach.grid ) grid += code + " ";
		for ( size_t t = 0; t < breach.targets.size(); ++t )
		{
			if ( t ) targets += "| ";
			for ( const std::string &code : breach.targets[t] ) targets += code + " ";
		}
		gi.cvar_set( "lwh_breach_grid", grid.c_str() );
		gi.cvar_set( "lwh_breach_targets", targets.c_str() );
		gi.cvar_set( "lwh_breach_system", ship::Spec( static_cast<ship::SystemId>( sys ) ).name );
		gi.cvar_set( "lwh_breach_result", "" );
		return;
	}
	else if ( !Q_stricmp( cmd, "solve" ) )
	{//the operator's picks: score them, and that is the strength of the counter-hack
		if ( breachSystem < 0 ) return;
		std::vector<int> picks;
		for ( int i = first + 1; i < gi.argc(); ++i ) picks.push_back( atoi( gi.argv( i ) ) );
		const float score = ship::BreachScore( breach, picks );
		ship::CounterHack( vessel, static_cast<ship::SystemId>( breachSystem ), score );
		gi.Printf( "SHIP: counter-hack on %s scored %d%%; control is now %d%%\n", ship::Spec( static_cast<ship::SystemId>( breachSystem ) ).name,
			static_cast<int>( score * 100 + 0.5f ), static_cast<int>( vessel.systems[breachSystem].control * 100 + 0.5f ) );
		gi.cvar_set( "lwh_breach_result", Fmt( "%d", static_cast<int>( score * 100 + 0.5f ) ).c_str() );
		breachSystem = -1;
	}
	else if ( !Q_stricmp( cmd, "jump" ) && a[0] )
	{
		if ( !ship::Jump( vessel, atoi( a ) ) ) gi.Printf( "SHIP: cannot jump to beacon %s (not one jump away, no warp drive, or no fuel)\n", a );
	}
	else if ( !Q_stricmp( cmd, "fire" ) )
	{
		if ( !ship::FireTorpedo( vessel ) ) gi.Printf( "SHIP: no torpedo fired (no target, none left, or the launchers are down)\n" );
	}
	else if ( !Q_stricmp( cmd, "target" ) )
	{//what Tactical aims at once the enemy's shields are down
		int t = -1;
		if ( !Q_stricmpn( a, "hull", 4 ) ) t = ship::TARGET_HULL;
		else if ( !Q_stricmpn( a, "weap", 4 ) ) t = ship::TARGET_WEAPONS;
		else if ( !Q_stricmpn( a, "eng", 3 ) ) t = ship::TARGET_ENGINES;
		else if ( !Q_stricmpn( a, "shie", 4 ) ) t = ship::TARGET_SHIELD_GEN;
		if ( t < 0 ) { gi.Printf( "SHIP: target hull | weapons | engines | shields\n" ); return; }
		ship::SetTarget( vessel, static_cast<ship::EnemySubsystem>( t ) );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "yield" ) )
	{//the phaser bank's setting: Tactical's standing decision, and the one that has an effect with
	 //no contact. 0/1/2/3 or the name. A higher setting asks the bank for more power.
		int y = -1;
		if ( a[0] >= '0' && a[0] <= '9' && !a[1] ) y = a[0] - '0';
		else if ( !Q_stricmpn( a, "stun", 4 ) ) y = ship::YIELD_STUN;
		else if ( !Q_stricmpn( a, "heavy", 5 ) ) y = ship::YIELD_HEAVY_STUN;
		else if ( !Q_stricmpn( a, "kill", 4 ) ) y = ship::YIELD_KILL;
		else if ( !Q_stricmpn( a, "vap", 3 ) ) y = ship::YIELD_VAPORIZE;
		if ( !ship::SetPhaserYield( vessel, y ) ) { gi.Printf( "SHIP: phaser yield stun | heavy | kill | vaporize\n" ); return; }
		gi.Printf( "SHIP: phaser bank set to %s\n", ship::PhaserYieldName( ship::PhaserYieldOf( vessel ) ) );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "hail" ) ) { ship::Hail( vessel ); Publish(); return; }
	else if ( !Q_stricmp( cmd, "trade" ) )
	{
		if ( !ship::Trade( vessel ) ) gi.Printf( "SHIP: no trade here (no trader, or not enough parts)\n" );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "distress" ) )
	{
		if ( !ship::AnswerDistress( vessel ) ) gi.Printf( "SHIP: no distress call here\n" );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "run" ) )
	{
		if ( !ship::Disengage( vessel ) ) gi.Printf( "SHIP: cannot run (no way out, no warp drive, or no fuel)\n" );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "cross" ) )
	{
		if ( !ship::AdvanceSector( vessel ) ) gi.Printf( "SHIP: the sector's end is not reached\n" );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "tractor" ) )
	{//the tractor beam: strip a derelict, or hold the contact
		if ( ship::TractorWreck( vessel ) )
		{
			gi.Printf( "SHIP: tractored a derelict and stripped it: %.0f parts, %.0f material\n", ship::SALVAGE_PARTS, ship::SALVAGE_MATERIALS );
			Publish();
			return;
		}
		if ( ship::InCombat( vessel ) ) { ship::TractorHold( vessel ); Publish(); return; }
		gi.Printf( "SHIP: nothing for the tractor beam to hold (the beam is down, or there is no wreck or contact)\n" );
		return;
	}
	else if ( !Q_stricmp( cmd, "fabricate" ) && a[0] )
	{
		if ( !ship::FabricateParts( vessel, atoi( a ) ) ) gi.Printf( "SHIP: cannot fabricate that (no replicators, or not enough material)\n" );
		else gi.Printf( "SHIP: material %.0f, parts %.0f\n", vessel.stores.materials, vessel.stores.spareParts );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "rations" ) && a[0] )
	{//the galley: material into food
		if ( !ship::FabricateRations( vessel, atoi( a ) ) ) gi.Printf( "SHIP: cannot prepare that (no replicators, or not enough material)\n" );
		else gi.Printf( "SHIP: rations %.0f, material %.0f\n", vessel.stores.rations, vessel.stores.materials );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "emh" ) )
	{
		const bool on = !Q_stricmp( b, "on" ) || ( !b[0] && !ship::EMHActive( vessel ) );
		if ( !ship::ActivateEMH( vessel, on ) ) gi.Printf( "SHIP: the EMH needs the computer core\n" );
		else gi.Printf( "SHIP: the EMH is %s\n", ship::EMHActive( vessel ) ? "active" : "off" );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "train" ) && a[0] && b[0] )
	{//ship train <crew> <station 0-4>: earn a cross-qualification
		if ( !ship::Train( vessel, atoi( a ), static_cast<ship::Station>( atoi( b ) ) ) )
			gi.Printf( "SHIP: no such training (bad crew number, station, or already qualified)\n" );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "brig" ) && a[0] )
	{//ship brig <crew> on|off
		if ( !ship::Brig( vessel, atoi( a ), Q_stricmp( b, "off" ) != 0 ) ) gi.Printf( "SHIP: the brig takes a fit crew member\n" );
		else gi.Printf( "SHIP: crew %s is %s\n", a, ship::Brigged( vessel, atoi( a ) ) ? "confined" : "released" );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "funeral" ) )
	{
		if ( !ship::HoldFuneral( vessel ) ) gi.Printf( "SHIP: only whoever commands holds a funeral\n" );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "promote" ) && a[0] )
	{
		static const char *const RANKS[] = { "Crewman", "Ensign", "Lt. j.g.", "Lieutenant", "Lt. Commander", "Commander", "Captain" };
		const int who = atoi( a );
		if ( !ship::Promote( vessel, who ) )
		{
			gi.Printf( "SHIP: only whoever commands promotes, and not beyond captain\n" );
			gi.cvar_set( "lwh_ship_promote", "REFUSED: only whoever commands promotes, and not beyond captain" );
		}
		else
		{
			gi.Printf( "SHIP: crew %s now holds rank %d\n", a, vessel.crew[who].rank );
			gi.cvar_set( "lwh_ship_promote", Fmt( "%s is promoted to %s", vessel.crew[who].name.c_str(), RANKS[vessel.crew[who].rank] ).c_str() );
		}
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "remember" ) && a[0] && b[0] )
	{//ship remember <crew> <event> [valence]
		const int who = atoi( a ), ev = atoi( b );
		const float val = gi.argc() > first + 2 ? atof( gi.argv( first + 2 ) ) : -0.5f;
		ship::Remember( vessel, who, static_cast<uint16_t>( ev ), -1, ship::MEM_SAW, val );
		gi.Printf( "SHIP: crew %d now remembers event %d\n", who, ev );
		return;
	}
	else if ( !Q_stricmp( cmd, "brief" ) && a[0] )
	{//command tells the crew
		ship::Brief( vessel, static_cast<uint16_t>( atoi( a ) ), b[0] ? atof( b ) : -0.4f );
		gi.Printf( "SHIP: the crew are told of event %s\n", a );
		return;
	}
	else if ( !Q_stricmp( cmd, "recall" ) && a[0] && b[0] )
	{
		const int who = atoi( a ); const uint16_t ev = static_cast<uint16_t>( atoi( b ) );
		if ( who < 0 || who >= static_cast<int>( vessel.crew.size() ) ) return;
		const int src = ship::RecallSource( vessel.crew[who], ev );
		static const char *const SRC[] = { "saw it", "was told", "heard it as rumour", "read it in the log" };
		gi.Printf( "SHIP: crew %d %s event %d%s\n", who, ship::Recall( vessel.crew[who], ev ) ? "knows" : "does not know", ev,
			src >= 0 ? Fmt( " (%s)", SRC[src] ).c_str() : "" );
		return;
	}
	else if ( !Q_stricmp( cmd, "bond" ) && a[0] && b[0] )
	{
		gi.Printf( "SHIP: bond %d -> %d: %.2f\n", atoi( a ), atoi( b ), ship::Bond( vessel, atoi( a ), atoi( b ) ) );
		return;
	}
	else if ( !Q_stricmp( cmd, "meeting" ) )
	{//the meeting (docs/staff-meetings.md): the brief, and the decision the simulation applies.
	 //A brief is a read; deciding applies exactly one enumerated outcome.
		static const char *const KINDS[] = { "watch", "departmental", "allocation", "dilithium", "casualties", "borg", "deferred" };
		auto kindOf = []( const char *name ) -> int {
			if ( name[0] >= '0' && name[0] <= '9' ) return atoi( name );
			for ( int i = 0; i < ship::MEET_KIND_COUNT; ++i )
				if ( !Q_stricmpn( name, KINDS[i], static_cast<int>( strlen( KINDS[i] ) ) ) ) return i;
			return -1;
		};
		if ( !a[0] || !Q_stricmp( a, "list" ) )
		{
			const std::vector<ship::MeetingBrief> &q = ship::PendingMeetings( vessel );
			gi.Printf( "SHIP: %d meeting brief(s) queued\n", static_cast<int>( q.size() ) );
			for ( const ship::MeetingBrief &mb : q )
				gi.Printf( "SHIP:   %s: %s (present %d, options %d)\n", ship::MeetingKindName( mb.kind ),
					mb.decision.c_str(), mb.presentCount, mb.optionCount );
			return;
		}
		if ( !Q_stricmp( a, "drain" ) ) { gi.Printf( "SHIP: %s\n", ship::TakeBrief( vessel ) ? "one brief taken" : "the queue is empty" ); return; }
		const int kind = kindOf( b );
		if ( kind < 0 || kind >= ship::MEET_KIND_COUNT )
		{
			gi.Printf( "SHIP: meeting [list|drain|brief <kind>|decide <kind> <outcome>]\n" );
			return;
		}
		if ( !Q_stricmp( a, "brief" ) )
		{
			const ship::MeetingBrief mb = ship::BuildBrief( vessel, static_cast<uint8_t>( kind ) );
			gi.Printf( "SHIP: meeting brief: %s: %s\n", ship::MeetingKindName( mb.kind ), mb.decision.c_str() );
			gi.Printf( "SHIP:   trigger: %s\n", mb.trigger.c_str() );
			for ( int i = 0; i < mb.presentCount; ++i )
				gi.Printf( "SHIP:   present: %s (%s, %s watch, mood %d%%)\n", mb.present[i].name.c_str(),
					mb.present[i].post < ship::SYS_COUNT ? ship::Spec( static_cast<ship::SystemId>( mb.present[i].post ) ).name : "department duties",
					mb.present[i].watch == 0 ? "alpha" : mb.present[i].watch == 1 ? "beta" : "gamma",
					static_cast<int>( mb.present[i].mood * 100.0f + 0.5f ) );
			for ( int i = 0; i < mb.optionCount; ++i )
				gi.Printf( "SHIP:   option %d: %s [cost: %s] (%s)\n", i + 1, mb.options[i].label.c_str(),
					mb.options[i].cost.c_str(), ship::MeetingEffectName( mb.options[i].effect ) );
			return;
		}
		if ( !Q_stricmp( a, "decide" ) )
		{
			const int outcome = gi.argc() > first + 3 ? atoi( gi.argv( first + 3 ) ) : 0;
			const ship::MeetingBrief mb = ship::BuildBrief( vessel, static_cast<uint8_t>( kind ) );
			const bool ok = ship::ApplyMeetingOutcome( vessel, mb, outcome, vessel.player, false );
			gi.Printf( "SHIP: meeting decided: %s outcome %d %s\n", ship::MeetingKindName( static_cast<uint8_t>( kind ) ),
				outcome, ok ? "applied" : "refused" );
			Publish();
			return;
		}
		gi.Printf( "SHIP: meeting [list|drain|brief <kind>|decide <kind> <outcome>]\n" );
		return;
	}
	else if ( !Q_stricmp( cmd, "mine" ) )
	{//resource acquisition: mine a belt and siphon its gas
		if ( !ship::MineBelt( vessel ) ) gi.Printf( "SHIP: nothing to mine here (no belt, already worked, or no tractor or sensors)\n" );
		else gi.Printf( "SHIP: material %.0f, deuterium %.1f%%\n", vessel.stores.materials, vessel.stores.deuterium * 100 );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "core" ) )
	{//the warp core cascade: coolant, overheat, containment, and the breach countdown
		if ( !Q_stricmp( a, "shutdown" ) ) { if ( !ship::ShutDownCore( vessel ) ) gi.Printf( "SHIP: the core cannot be shut down now\n" ); }
		else if ( !Q_stricmp( a, "restart" ) ) { if ( !ship::RestartCore( vessel ) ) gi.Printf( "SHIP: the core cannot be restarted yet\n" ); }
		else if ( !Q_stricmp( a, "eject" ) ) { if ( !ship::EjectCore( vessel ) ) gi.Printf( "SHIP: no core to eject\n" ); }
		else if ( !Q_stricmp( a, "coolant" ) ) { if ( !ship::RestoreCoolant( vessel ) ) gi.Printf( "SHIP: the coolant cannot be refilled now\n" ); }
		gi.Printf( "SHIP: core coolant %d%%  temperature %d%%  containment %d%%%s%s%s\n",
			static_cast<int>( vessel.coolant * 100 + 0.5f ), static_cast<int>( vessel.coreTemp * 100 + 0.5f ),
			static_cast<int>( vessel.containment * 100 + 0.5f ),
			vessel.breachCountdown >= 0.0f ? Fmt( "  BREACH IN %d s", static_cast<int>( vessel.breachCountdown ) ).c_str() : "",
			vessel.coreShutdown ? "  (shut down)" : "", vessel.coreEjected ? "  (ejected)" : "" );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "probe" ) && a[0] )
	{//a probe: the safe way to look at something hostile
		if ( !ship::LaunchProbe( vessel, atoi( a ) ) ) gi.Printf( "SHIP: no probe launched (none left, launchers down, or bad target)\n" );
		else gi.Printf( "SHIP: probe away; %d left\n", vessel.stores.probes );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "finddilithium" ) )
	{//a dedicated survey to locate a source: survey, chart, detour
		const int at = ship::LocateDilithium( vessel );
		if ( at < 0 ) gi.Printf( "SHIP: no dilithium source found (sensors down, or none in this sector)\n" );
		else gi.Printf( "SHIP: dilithium source charted at beacon %d\n", at );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "dilithium" ) )
	{//the constraint that forces exploration: the crystal, and how much of the journey it buys
		gi.Printf( "SHIP: dilithium %d%% (ceiling %d%%, quality %.2f, %d replaced), range %d ly, warp %s\n",
			static_cast<int>( vessel.dilithium * 100 + 0.5f ), static_cast<int>( vessel.crystalCeiling * 100 + 0.5f ),
			vessel.crystalQuality, vessel.crystalReplacements, ship::DilithiumRange( vessel ),
			ship::WarpPossible( vessel ) ? "possible" : "impossible" );
		return;
	}
	else if ( !Q_stricmp( cmd, "nav" ) || !Q_stricmp( cmd, "navigation" ) )
	{//the navigation counter: how far home, how long, and how that changed (docs/navigation-counter.md)
		const ship::Navigation nav = ship::NavigationCounter( vessel );
		gi.Printf( "SHIP: %d light years from home; %d years nominal, %d at current capability\n",
			static_cast<int>( nav.distanceLy + 0.5f ), static_cast<int>( nav.nominalYears + 0.5f ),
			static_cast<int>( nav.currentYears + 0.5f ) );
		if ( nav.warp )
			gi.Printf( "SHIP:   %+.1f years since the last entry; effective speed %d c\n",
				static_cast<double>( nav.changeYears ), static_cast<int>( nav.speedC + 0.5f ) );
		else
			gi.Printf( "SHIP:   no warp: home stops getting closer\n" );
		if ( ship::PlayerMayCommand( vessel ) )
		{//command sees the forecasts: the estimate under each available course
			const std::vector<ship::NavCourse> routes = ship::NavigationForecasts( vessel );
			if ( routes.empty() ) gi.Printf( "SHIP: forecasts: no course from here\n" );
			else for ( const ship::NavCourse &c : routes )
				gi.Printf( "SHIP:   forecast: beacon %d %s%s; %s from there\n", c.beacon,
					c.charted ? ship::BeaconKindName( static_cast<ship::BeaconKind>( c.kind ) ) : "uncharted",
					c.charted ? "" : " (not yet charted)",
					c.years >= 0.0f ? Fmt( "%d years", static_cast<int>( c.years + 0.5f ) ).c_str() : "no warp" );
		}
		return;
	}
	else if ( !Q_stricmp( cmd, "recomposite" ) )
	{//Engineering buys back life in the crystal's frame, until the crystal is too far gone
		if ( !ship::Recomposite( vessel ) )
			gi.Printf( "SHIP: recomposition cannot help (the warp core is down, no engineer is free, or the crystal is spent)\n" );
		else gi.Printf( "SHIP: dilithium now %d%% (ceiling %d%%)\n",
			static_cast<int>( vessel.dilithium * 100 + 0.5f ), static_cast<int>( vessel.crystalCeiling * 100 + 0.5f ) );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "acquire" ) && a[0] )
	{//a new crystal: mine a belt, trade, salvage, or research a better one
		const ship::DilithiumWay way = !Q_stricmp( a, "mine" ) ? ship::DIL_MINE : !Q_stricmp( a, "trade" ) ? ship::DIL_TRADE
			: !Q_stricmp( a, "salvage" ) ? ship::DIL_SALVAGE : !Q_stricmp( a, "research" ) ? ship::DIL_RESEARCH : ship::DIL_WAY_COUNT;
		if ( way >= ship::DIL_WAY_COUNT || !ship::AcquireDilithium( vessel, way ) )
			gi.Printf( "SHIP: no crystal to be had that way here (mine | trade | salvage | research)\n" );
		else gi.Printf( "SHIP: dilithium %d%%, quality %.2f, range %d ly\n",
			static_cast<int>( vessel.dilithium * 100 + 0.5f ), vessel.crystalQuality, ship::DilithiumRange( vessel ) );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "controller" ) )
	{//the Borg incursion's field: who holds each deck, and the clean intercepts recorded
		int shown = 0;
		for ( int d = 0; d < ship::DECKS; ++d )
		{
			const ship::Deck &deck = vessel.decks[d];
			if ( deck.intruders <= 0.0f && !deck.compromised && deck.controller == ship::CTRL_CREW && deck.forceFieldLevel <= 0.0f ) continue;
			gi.Printf( "SHIP: deck %d %s%s, intruders %.0f, dwell %.0fs, field %d\n", d + 1, ship::ControllerName( deck.controller ),
				deck.compromised ? " (compromised)" : "", std::ceil( deck.intruders - 1e-3f ), deck.dwell, static_cast<int>( deck.forceFieldLevel + 0.5f ) );
			++shown;
		}
		gi.Printf( "SHIP: %d deck(s) not wholly ours; %d clean intercept(s)\n", shown, vessel.cleanIntercepts );
		return;
	}
	else if ( !Q_stricmp( cmd, "recover" ) && a[0] )
	{//de-assimilation: sickbay against the nanoprobes, in the narrow window
		const int who = atoi( a );
		if ( !ship::RecoverCaptive( vessel, who ) )
			gi.Printf( "SHIP: nothing to recover (no one by that number is in the window, or sickbay cannot pay)\n" );
		else gi.Printf( "SHIP: crew %d is back, with lasting residue (scar %d%%)\n", who, static_cast<int>( vessel.crew[who].assimScar * 100 + 0.5f ) );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "remodulate" ) )
	{//the counter-play kit: rotate the phaser modulation and break the Borg's lock
		if ( !ship::Remodulate( vessel ) )
			gi.Printf( "SHIP: nothing to remodulate (no adaptation, or the adapter is still cooling)\n" );
		else gi.Printf( "SHIP: modulation rotated; enemy adaptation now %d%%\n", static_cast<int>( vessel.enemy.adaptation * 100 + 0.5f ) );
		return;
	}
	else if ( !Q_stricmp( cmd, "vinculum" ) )
	{//a vinculum raid: sever local coordination, at a cost
		if ( !ship::RaidVinculum( vessel ) )
			gi.Printf( "SHIP: no vinculum to raid here (no Borg, or one is already down)\n" );
		else gi.Printf( "SHIP: vinculum destroyed; adaptation suppressed for %d s\n", static_cast<int>( vessel.adaptationSuppressed + 0.5f ) );
		return;
	}
	else if ( !Q_stricmp( cmd, "advance" ) && a[0] )
	{//the security squad: send a fireteam to retake a deck (docs/borg-incursion.md)
		if ( !ship::OrderAdvance( vessel, atoi( a ) ) )
			gi.Printf( "SHIP: no squad to send (command only, a deck 1-15, and security crew fit)\n" );
		else gi.Printf( "SHIP: a squad is advancing to retake deck %s\n", a );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "shuttle" ) )
	{//shuttles: supported, not pilotable -- where each one is
		gi.Printf( "SHIP: %d in the bay, %d away\n", ship::ShuttlesInBay( vessel ), ship::ShuttlesAway( vessel ) );
		for ( const ship::Shuttle &sh : vessel.shuttles )
			gi.Printf( "SHIP:   %-12s %-10s condition %d%%%s\n", ship::ShuttleClassName( sh.cls ), ship::ShuttleLocationName( sh.location ),
				static_cast<int>( sh.condition * 100 + 0.5f ),
				sh.location == ship::SHUTTLE_AWAY ? Fmt( " (beacon %d, %d aboard)", sh.awayBeacon, static_cast<int>( sh.manifest.size() ) ).c_str()
				: ( sh.location == ship::SHUTTLE_LOST ? Fmt( " (left at beacon %d)", sh.awayBeacon ).c_str() : "" ) );
		return;
	}
	else if ( !Q_stricmp( cmd, "launch" ) && a[0] && b[0] )
	{//ship launch <class> <beacon> [crew...]: the load screen's commit
		const int cls = atoi( a );
		std::vector<int> manifest;
		for ( int i = first + 3; i < gi.argc(); ++i ) manifest.push_back( atoi( gi.argv( i ) ) );
		if ( cls < 0 || cls >= ship::SHUTTLE_CLASS_COUNT || !ship::LaunchShuttle( vessel, static_cast<ship::ShuttleClass>( cls ), atoi( b ), manifest ) )
			gi.Printf( "SHIP: cannot launch that shuttle (not in the bay, or a crew member is not fit)\n" );
		else gi.Printf( "SHIP: %s away to beacon %s, %d aboard\n", ship::ShuttleClassName( static_cast<ship::ShuttleClass>( cls ) ), b, static_cast<int>( manifest.size() ) );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "shuttledock" ) && a[0] )
	{//"recall" is the transporter's; a shuttle comes home by docking
		const int cls = atoi( a );
		if ( cls < 0 || cls >= ship::SHUTTLE_CLASS_COUNT || !ship::RecallShuttle( vessel, static_cast<ship::ShuttleClass>( cls ) ) )
			gi.Printf( "SHIP: no such shuttle is away\n" );
		else gi.Printf( "SHIP: %s is back in the bay\n", ship::ShuttleClassName( static_cast<ship::ShuttleClass>( cls ) ) );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "lose" ) && a[0] )
	{
		const int cls = atoi( a );
		if ( cls < 0 || cls >= ship::SHUTTLE_CLASS_COUNT || !ship::LoseShuttle( vessel, static_cast<ship::ShuttleClass>( cls ) ) )
			gi.Printf( "SHIP: no such shuttle to lose\n" );
		else gi.Printf( "SHIP: %s is lost; the second bay is to build a replacement\n", ship::ShuttleClassName( static_cast<ship::ShuttleClass>( cls ) ) );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "bayhit" ) && a[0] )
	{
		ship::ShuttleBayHit( vessel, static_cast<float>( atof( a ) ) );
		gi.Printf( "SHIP: %d shuttle(s) left in the bay\n", ship::ShuttlesInBay( vessel ) );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "rebuild" ) && a[0] )
	{
		const int cls = atoi( a );
		if ( cls < 0 || cls >= ship::SHUTTLE_CLASS_COUNT ) gi.Printf( "SHIP: no such shuttle class\n" );
		else { ship::RebuildShuttle( vessel, static_cast<ship::ShuttleClass>( cls ) ); gi.Printf( "SHIP: a build job for a %s is on the board\n", ship::ShuttleClassName( static_cast<ship::ShuttleClass>( cls ) ) ); }
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "survivors" ) && a[0] )
	{//population pressure: take survivors or refugees aboard
		ship::TakeSurvivors( vessel, atoi( a ) );
		gi.Printf( "SHIP: %d aboard\n", ship::Refugees( vessel ) );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "hearing" ) && a[0] )
	{//justice: a hearing releases or confirms a confinement
		const int who = atoi( a );
		const bool guilty = !Q_stricmp( b, "guilty" );
		if ( !ship::Hearing( vessel, who, guilty ) ) gi.Printf( "SHIP: there is no one by that number in the brig\n" );
		else gi.Printf( "SHIP: crew %d is %s\n", who, ship::Brigged( vessel, who ) ? "convicted" : "acquitted" );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "holo" ) && a[0] && b[0] )
	{//the holodeck: recreation, training, therapy or forensic reconstruction
		int use = -1;
		if ( !Q_stricmpn( a, "rec", 3 ) ) use = ship::HOLO_RECREATION;
		else if ( !Q_stricmpn( a, "train", 5 ) ) use = ship::HOLO_TRAINING;
		else if ( !Q_stricmpn( a, "thera", 5 ) ) use = ship::HOLO_THERAPY;
		else if ( !Q_stricmpn( a, "fore", 4 ) ) use = ship::HOLO_FORENSIC;
		if ( use < 0 ) { gi.Printf( "SHIP: holo recreation | training | therapy | forensic <crew>\n" ); return; }
		if ( !ship::RunHolodeck( vessel, static_cast<ship::HolodeckUse>( use ), atoi( b ) ) )
			gi.Printf( "SHIP: the holodeck could not run that (the system is down, or the crew member cannot)\n" );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "jumpstart" ) )
	{//the holodeck matrix is a trap, not a solution (docs/ship-systems.md; VOY "Parallax"): with
	 //the main grid down, tying in a holodeck reactor buys a charge and wrecks the ship's relays
		if ( !ship::JumpStartFromHolodeck( vessel ) )
			gi.Printf( "SHIP: the holodeck reactor cannot be tied in now (the main grid is up, or it is wrecked)\n" );
		else gi.Printf( "SHIP: jump-started from a holodeck reactor: the cross-tie blew relays; half the ship is wrecked\n" );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "quarters" ) )
	{
		if ( !ship::ImproveQuarters( vessel ) ) gi.Printf( "SHIP: not enough material to improve the quarters\n" );
		else gi.Printf( "SHIP: the crew's quarters are improved; material %.0f\n", vessel.stores.materials );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "wall" ) )
	{//the wall of names, and the quarters grief still keeps shut (docs/morale.md)
		const std::vector<std::string> names = ship::WallOfNames( vessel );
		gi.Printf( "SHIP: the wall of names, %d the ship has buried:\n", static_cast<int>( names.size() ) );
		for ( const std::string &name : names ) gi.Printf( "SHIP:   %s\n", name.c_str() );
		const std::vector<ship::SealedQuarter> sealed = ship::SealedQuarters( vessel );
		for ( const ship::SealedQuarter &q : sealed )
			gi.Printf( "SHIP:   %s's quarters, deck %d, are sealed\n", vessel.crew[q.crew].name.c_str(), q.deck );
		return;
	}
	else if ( !Q_stricmp( cmd, "pylon" ) && a[0] )
	{//damage the nacelle pylons (a hit does this in a fight); a ship without them cannot warp
		ship::DamagePylon( vessel, atof( a ) );
		gi.Printf( "SHIP: pylons at %.0f%%%s\n", vessel.pylonHealth * 100, ship::PylonsIntact( vessel ) ? "" : "  NO WARP" );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "emitter" ) )
	{//the mobile emitter: an artifact that lets the EMH work away from sickbay
		ship::SetMobileEmitter( vessel, Q_stricmp( b, "off" ) != 0 );
		gi.Printf( "SHIP: mobile emitter %s\n", ship::MobileEmitter( vessel ) ? "aboard" : "stowed" );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "airponics" ) )
	{//the airponics bay: food grown, not replicated
		ship::SetAirponics( vessel, Q_stricmp( b, "off" ) != 0 );
		gi.Printf( "SHIP: the airponics bay is %s\n", ship::Airponics( vessel ) ? "growing food" : "shut down" );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "borgfx" ) )
	{//runtime asset replacement (S8): a surface's shader swapped live, and swapped back. On the
		//generated decks the BorgAssets pass does this per deck by itself; this drives a known deck.
		const bool on = Q_stricmp( a, "off" ) != 0;
		if ( on ) gi.RemapShader( "textures/hall/hallfloor1", "textures/lwh/borg", "0" );
		else gi.RemapShader( "textures/hall/hallfloor1", "textures/hall/hallfloor1", "0" ); // clears the remap
		gi.Printf( "SHIP: Borg asset replacement %s\n", on ? "on" : "off" );
		return;
	}
	else if ( !Q_stricmp( cmd, "holoend" ) && a[0] )
	{
		if ( !ship::EndHolodeckProgram( vessel, atoi( a ) ) ) gi.Printf( "SHIP: that crew member is not lost in the program\n" );
		else gi.Printf( "SHIP: crew %s is back on duty\n", a );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "eva" ) )
	{//an away party works a belt or wreck by hand, in EV suits
		if ( !ship::EVA( vessel ) ) gi.Printf( "SHIP: no EVA possible (no party out, no EV suits, or nothing to reach)\n" );
		else gi.Printf( "SHIP: material %.0f, parts %.0f\n", vessel.stores.materials, vessel.stores.spareParts );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "observe" ) )
	{
		if ( !ship::ObservePreWarp( vessel ) ) gi.Printf( "SHIP: there is no pre-warp civilisation here\n" );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "interfere" ) )
	{
		if ( !ship::InterferePreWarp( vessel ) ) gi.Printf( "SHIP: there is no pre-warp civilisation here\n" );
		else gi.Printf( "SHIP: the Prime Directive was violated; resentment %.2f\n", ship::Resentment( vessel ) );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "reconcile" ) )
	{
		if ( !ship::ReconcileFactions( vessel ) ) gi.Printf( "SHIP: only whoever commands reconciles the crew\n" );
		else gi.Printf( "SHIP: resentment %.2f\n", ship::Resentment( vessel ) );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "awareness" ) )
	{
		gi.Printf( "SHIP: Borg strategic awareness %.0f%%\n", ship::BorgAwareness( vessel ) * 100.0f );
		return;
	}
	else if ( !Q_stricmp( cmd, "klaxon" ) )
	{//the alert klaxon, played around the player (S9's "hear"); the game's own alert sounds
		if ( !Q_stricmp( a, "red" ) ) G_Sound( &g_entities[0], G_SoundIndex( "sound/ambience/voyager/redalert.mp3" ) );
		else G_Sound( &g_entities[0], G_SoundIndex( "sound/ambience/voyager/alarm1.mp3" ) );
		return;
	}
	else if ( !Q_stricmp( cmd, "wingman" ) )
	{//a second contact joins the fight (S9: more than one at a time)
		if ( !ship::InCombat( vessel ) ) gi.Printf( "SHIP: no fight to join\n" );
		else { vessel.contact2 = vessel.enemy; vessel.contact2.firepower *= 0.6f; vessel.contact2.boarders = 0; gi.Printf( "SHIP: a second contact joins\n" ); }
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "grudge" ) && a[0] && b[0] )
	{//a remembered grievance: crew a holds one against crew b
		ship::Remember( vessel, atoi( a ), ship::MEM_LIE, atoi( b ), ship::MEM_SAW, -0.9f );
		gi.Printf( "SHIP: bond %d -> %d: %.2f\n", atoi( a ), atoi( b ), ship::Bond( vessel, atoi( a ), atoi( b ) ) );
		return;
	}
	else if ( !Q_stricmp( cmd, "chart" ) )
	{
		static const char *const KINDS[] = { "empty", "hostile", "derelict", "Borg", "trader", "distress", "belt", "pre-warp", "THE END" };
		for ( size_t i = 0; i < vessel.sector.size(); ++i )
		{
			std::string links;
			for ( int l : vessel.sector[i].links ) links += Fmt( " %d", l );
			gi.Printf( "SHIP: %sbeacon %2d  %-8s  jumps to%s\n", static_cast<int>( i ) == vessel.beacon ? "> " : "  ", static_cast<int>( i ),
				vessel.sector[i].visited ? KINDS[vessel.sector[i].kind] : "unknown", links.c_str() );
		}
		return;
	}
	else if ( !Q_stricmp( cmd, "log" ) )
	{//the official log: ship log [count] [scope] -- the signed, published record, newest first.
	 //The private log is a different store and is never returned here (ReadOfficialLog).
		const int n = a[0] ? atoi( a ) : 15;
		const std::vector<ship::LogEntry> entries = ship::ReadOfficialLog( vessel, n, b );
		for ( const ship::LogEntry &e : entries )
		{
			const int day = static_cast<int>( e.time / ship::SECONDS_PER_DAY );
			const int sod = static_cast<int>( e.time ) % ship::SECONDS_PER_DAY;
			gi.Printf( "SHIP: day %d %02d:%02d  [%s] %s: %s\n", day, sod / 3600, sod % 3600 / 60, e.scope.c_str(), e.who.c_str(), e.what.c_str() );
		}
		if ( entries.empty() ) gi.Printf( "SHIP: the log is empty\n" );
		return;
	}
	else if ( !Q_stricmp( cmd, "personal" ) )
	{//the private log (docs/the-record-and-the-log.md): per person, and nobody else's read. The
	 //truth goes here when it cannot go in the report. `ship personal write <text...>` writes it.
		if ( vessel.player < 0 )
		{ gi.Printf( "SHIP: no personal log: no character is the player\n" ); return; }
		if ( !Q_stricmp( a, "write" ) )
		{
			std::string text;
			for ( int i = first + 2; i < gi.argc(); ++i ) { if ( text.size() ) text += " "; text += gi.argv( i ); }
			if ( !ship::WritePersonalLog( vessel, vessel.player, text ) ) gi.Printf( "SHIP: nothing written\n" );
			else gi.Printf( "SHIP: %s's private log carries it; nobody else reads it\n", vessel.crew[vessel.player].name.c_str() );
			Publish();
			return;
		}
		const int n = a[0] ? atoi( a ) : 15;
		const std::vector<ship::PersonalLogEntry> entries = ship::PersonalLog( vessel, vessel.player );
		int printed = 0;
		for ( int i = static_cast<int>( entries.size() ) - 1; i >= 0 && printed < n; --i )
		{
			const ship::PersonalLogEntry &e = entries[i];
			const int day = static_cast<int>( e.time / ship::SECONDS_PER_DAY );
			const int sod = static_cast<int>( e.time ) % ship::SECONDS_PER_DAY;
			gi.Printf( "SHIP: day %d %02d:%02d  %s: %s\n", day, sod / 3600, sod % 3600 / 60, e.who.c_str(), e.what.c_str() );
			++printed;
		}
		if ( !printed ) gi.Printf( "SHIP: %s's personal log is empty\n", vessel.crew[vessel.player].name.c_str() );
		return;
	}
	else if ( !Q_stricmp( cmd, "sleep" ) && a[0] )
	{//the sleep state (docs/ship-model.md): skip time at the accelerated rate, in one jump or in steps
		const double hours = atof( a );
		if ( !( hours > 0.0 ) ) { gi.Printf( "SHIP: usage: ship sleep <hours>\n" ); return; }
		const double before = vessel.clock;
		ship::Sleep( vessel, hours * 3600.0 );
		gi.Printf( "SHIP: slept %.1f h: day %d %02d:%02d, %.1f hours of ship time passed\n", hours,
			vessel.Day(), vessel.SecondOfDay() / 3600, vessel.SecondOfDay() % 3600 / 60, ( vessel.clock - before ) / 3600.0 );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "suspend" ) )
	{//one of the two exits: leave the ship standing. Ironman refuses it, and keeps her own time.
		if ( ship::Suspend( vessel ) )
			gi.Printf( "SHIP: the ship is left standing; the run is marked\n" );
		else
			gi.Printf( "SHIP: ironman will not suspend the world; the ship keeps her own time\n" );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "leftstanding" ) )
	{//the record's mark: was this run ever left standing?
		gi.Printf( "SHIP: this run has %sbeen left standing\n", ship::LeftStanding( vessel ) ? "" : "not " );
		return;
	}
	else if ( !Q_stricmp( cmd, "losses" ) )
	{//the written-off list: what the ship has given up (docs/damage-and-budgets.md)
		const int filterKind = FindLossKind( a );
		const std::vector<ship::LossEntry> &losses = ship::WriteOffs( vessel );
		int printed = 0;
		for ( int i = static_cast<int>( losses.size() ) - 1; i >= 0; --i )
		{
			const ship::LossEntry &e = losses[i];
			if ( a[0] && filterKind >= 0 && e.kind != filterKind ) continue;
			const int day = static_cast<int>( e.time / ship::SECONDS_PER_DAY );
			const int sod = static_cast<int>( e.time ) % ship::SECONDS_PER_DAY;
			gi.Printf( "SHIP: day %d %02d:%02d  [%s] %s  (decided by %s)\n", day, sod / 3600, sod % 3600 / 60,
				ship::LossKindName( e.kind ), e.what.c_str(), e.who.c_str() );
			++printed;
		}
		if ( !printed ) gi.Printf( "SHIP: nothing has been given up\n" );
		return;
	}
	else if ( !Q_stricmp( cmd, "writeoff" ) )
	{//write off a compartment (a deck number) or a system (its name), and why it was given up
		const int kindArg = FindLossKind( b );
		const int kind = kindArg >= 0 ? kindArg : ship::LOSS_WRITTEN_OFF;
		const int deck = a[0] ? atoi( a ) : 0;
		bool ok;
		if ( deck >= 1 && deck <= ship::DECKS ) ok = ship::WriteOff( vessel, false, deck, kind );
		else if ( sys >= 0 ) ok = ship::WriteOff( vessel, true, sys, kind );
		else ok = false;
		if ( ok ) gi.Printf( "SHIP: written off: %s (%s)\n",
			deck >= 1 && deck <= ship::DECKS ? va( "deck %d", deck ) : ship::Spec( static_cast<ship::SystemId>( sys ) ).name,
			ship::LossKindName( kind ) );
		else gi.Printf( "SHIP: nothing written off: already on the list, or not a deck or a system\n" );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "jobs" ) )
	{//the outstanding work, in the order it is worked (docs/crew-work.md)
		const std::vector<ship::Job> &jobs = ship::Jobs( vessel );
		gi.Printf( "SHIP: %d job(s) outstanding\n", static_cast<int>( jobs.size() ) );
		for ( const ship::Job &j : jobs )
		{
			char what[48];
			if ( j.kind == ship::JOB_REPAIR ) Com_sprintf( what, sizeof( what ), "%s", ship::Spec( static_cast<ship::SystemId>( j.target ) ).name );
			else Com_sprintf( what, sizeof( what ), "deck %d", j.target );
			gi.Printf( "SHIP:   %-8s %-24s %3d%%  priority %d\n", ship::JobKindName( j.kind ), what,
				static_cast<int>( j.progress * 100 + 0.5f ), j.priority );
		}
		return;
	}
	else if ( !Q_stricmp( cmd, "job" ) && a[0] )
	{//the job-queue board's one command-side act (docs/crew-work.md): command sets the order
		const int index = atoi( a );
		const int priority = b[0] ? atoi( b ) : 0;
		if ( !ship::SetJobPriority( vessel, index, priority ) )
		{
			gi.cvar_set( "lwh_ship_job_refused", "priority is command's to set" );
			gi.Printf( "SHIP: job refused: priority is command's to set\n" );
			return;
		}
		gi.cvar_set( "lwh_ship_job_refused", "" );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "build" ) && a[0] )
	{//command orders spare parts built from the ship's material
		if ( !ship::OrderBuild( vessel, atoi( a ) ) ) gi.Printf( "SHIP: only whoever commands builds, and it takes material\n" );
		else gi.Printf( "SHIP: building spare parts; material %.0f, parts %.0f\n", vessel.stores.materials, vessel.stores.spareParts );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "report" ) )
	{//the month report: drafted from the record, edited by the player, signed, and purged. The report
	 //is signed by the officer who commands, so writing it is command's alone (the person axis of the
	 //two-lock model); reading the open draft and the diff is open to all. The editor screen drives
	 //these same commands, so what a check drives is what a hand on the console drives.
		const char *sub = a;
		const bool mutating = sub[0] && ( !Q_stricmp( sub, "edit" ) || !Q_stricmp( sub, "strike" )
			|| !Q_stricmp( sub, "soften" ) || !Q_stricmp( sub, "add" ) || !Q_stricmp( sub, "sign" )
			|| !Q_stricmp( sub, "file" ) || !Q_stricmp( sub, "purge" ) );
		if ( mutating && !ship::PlayerMayCommand( vessel ) )
		{
			gi.cvar_set( "lwh_ship_report_refused", "the report is the commanding officer's to write" );
			gi.Printf( "SHIP: report refused: the report is the commanding officer's to write\n" );
			return;
		}
		gi.cvar_set( "lwh_ship_report_refused", "" );
		if ( !sub[0] || !Q_stricmp( sub, "read" ) || !Q_stricmp( sub, "draft" ) )
		{
			if ( ship::OpenReport( vessel ).lines.empty() ) ship::DraftReport( vessel, ship::DEPT_COUNT );
			const ship::MonthReport &open = ship::OpenReport( vessel );
			gi.Printf( "SHIP: --- the month report, entry %d (%s) ---\n", open.number, open.open ? "draft" : "signed" );
			for ( size_t i = 0; i < open.lines.size(); ++i )
				gi.Printf( "SHIP: %2d %s[%s] %s\n", static_cast<int>( i ), open.lines[i].struck ? "(struck) " : "",
					open.lines[i].scope.c_str(), open.lines[i].text.c_str() );
			{
				const ship::Navigation nav = ship::NavigationCounter( vessel );
				if ( nav.warp )
					gi.Printf( "SHIP: headline: home %d light years; %d years nominal, %d now, %+.1f since the last entry\n",
						static_cast<int>( nav.distanceLy + 0.5f ), static_cast<int>( nav.nominalYears + 0.5f ),
						static_cast<int>( nav.currentYears + 0.5f ), static_cast<double>( open.counterChange ) );
				else
					gi.Printf( "SHIP: headline: no warp: %d light years out, and home stops getting closer\n",
						static_cast<int>( nav.distanceLy + 0.5f ) );
			}
			return;
		}
		if ( !Q_stricmp( sub, "edit" ) && b[0] )
		{//report edit <line> <text...>: soften a number, or say something else
			std::string text;
			for ( int i = first + 3; i < gi.argc(); ++i ) { if ( text.size() ) text += " "; text += gi.argv( i ); }
			if ( !ship::EditReportLine( vessel, atoi( b ), text ) ) gi.Printf( "SHIP: no such line\n" );
			return;
		}
		if ( !Q_stricmp( sub, "strike" ) && b[0] )
		{//strike a line from the published version
			if ( !ship::StrikeReportLine( vessel, atoi( b ) ) ) gi.Printf( "SHIP: no such line\n" );
			return;
		}
		if ( !Q_stricmp( sub, "soften" ) && b[0] )
		{//soften <line> [factor]
			const float factor = gi.argc() > first + 3 ? atof( gi.argv( first + 3 ) ) : 0.5f;
			if ( !ship::SoftenReportLine( vessel, atoi( b ), factor ) ) gi.Printf( "SHIP: that line has no number to soften\n" );
			return;
		}
		if ( !Q_stricmp( sub, "add" ) && b[0] )
		{//add a claim: report add <scope> <text...>
			std::string text;
			for ( int i = first + 3; i < gi.argc(); ++i ) { if ( text.size() ) text += " "; text += gi.argv( i ); }
			if ( !ship::AddReportLine( vessel, b, text ) ) gi.Printf( "SHIP: cannot add that claim\n" );
			return;
		}
		if ( !Q_stricmp( sub, "sign" ) || !Q_stricmp( sub, "file" ) )
		{//sign to the crew, or file upward where nobody below reads it
			int signer = b[0] ? atoi( b ) : 0; // the captain signs the whole
			const uint8_t aud = !Q_stricmp( sub, "file" ) ? ship::REPORT_UPWARD : ship::REPORT_TO_CREW;
			if ( signer < 0 || signer >= static_cast<int>( vessel.crew.size() ) || !ship::SignReport( vessel, signer, aud ) )
			{ gi.Printf( "SHIP: nothing to sign\n" ); return; }
			gi.Printf( "SHIP: the report is signed by %s and published to %s\n", vessel.crew[signer].name.c_str(), ship::ReportAudienceName( aud ) );
			Publish();
			return;
		}
		if ( !Q_stricmp( sub, "diff" ) )
		{//the record keeps the diff; the player can always see what they did
			const std::vector<ship::MonthReport> &reps = ship::Reports( vessel );
			if ( reps.empty() ) gi.Printf( "SHIP: no signed report\n" );
			else gi.Printf( "SHIP: --- what the record kept ---\n%s", ship::ReportDiff( reps.back() ).c_str() );
			return;
		}
		if ( !Q_stricmp( sub, "purge" ) )
		{//purge the published logs; the crew's read marks are orphaned, not erased
			if ( !ship::PurgeLogs( vessel ) ) gi.Printf( "SHIP: the logs are already empty\n" );
			else gi.Printf( "SHIP: the published logs are purged; the marks sourced read-it-in-the-log are orphaned\n" );
			Publish();
			return;
		}
		gi.Printf( "SHIP: report [draft] | strike <line> | soften <line> [factor] | edit <line> <text> | add <scope> <text> | sign [crew] | file [crew] | diff | purge\n" );
		return;
	}
	else if ( !Q_stricmp( cmd, "promise" ) && a[0] && b[0] && gi.argc() > first + 4 )
	{//ship promise <officer> <crew> <kind 0-3> <what...>
		std::string what;
		for ( int i = first + 5; i < gi.argc(); ++i ) { if ( what.size() ) what += " "; what += gi.argv( i ); }
		const int idx = ship::MakePromise( vessel, atoi( a ), atoi( b ), static_cast<ship::PromiseKind>( atoi( gi.argv( first + 4 ) ) ), what );
		if ( idx < 0 ) gi.Printf( "SHIP: no such promise (bad crew, or not fit)\n" );
		else gi.Printf( "SHIP: promise %d held: %s\n", idx, what.c_str() );
		return;
	}
	else if ( !Q_stricmp( cmd, "promises" ) )
	{//the claims held, so they can mature
		const std::vector<ship::Promise> &ps = ship::Promises( vessel );
		static const char *const STATE[] = { "open", "kept", "broken" };
		gi.Printf( "SHIP: %d promise(s) held\n", static_cast<int>( ps.size() ) );
		for ( size_t i = 0; i < ps.size(); ++i )
		{
			const ship::Promise &p = ps[i];
			gi.Printf( "SHIP:   %2d  %s -> %s  %s: %s\n", static_cast<int>( i ),
				( p.promiser >= 0 && p.promiser < static_cast<int>( vessel.crew.size() ) ) ? vessel.crew[p.promiser].name.c_str() : "command",
				( p.beneficiary >= 0 && p.beneficiary < static_cast<int>( vessel.crew.size() ) ) ? vessel.crew[p.beneficiary].name.c_str() : "?",
				STATE[p.state], p.what.c_str() );
		}
		return;
	}
	else if ( ( !Q_stricmp( cmd, "keep" ) || !Q_stricmp( cmd, "break" ) ) && a[0] )
	{//the thing is done, or it is not
		const bool kept = !Q_stricmp( cmd, "keep" );
		if ( !ship::ResolvePromise( vessel, atoi( a ), kept ) ) gi.Printf( "SHIP: no such open promise\n" );
		else gi.Printf( "SHIP: promise %s %s\n", a, kept ? "kept" : "broken" );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "counterhack" ) && sys >= 0 && b[0] ) ship::CounterHack( vessel, static_cast<ship::SystemId>( sys ), atof( b ) );
	else if ( !Q_stricmp( cmd, "source" ) && FindSource( a ) >= 0 && b[0] )
		ship::SetSourceOnline( vessel, static_cast<ship::SourceId>( FindSource( a ) ), !Q_stricmp( b, "on" ) );
	else
	{
		gi.Printf( "usage: ship status | crew <deck> | console | alert green|yellow|red | on|off <system> | priority <system> <n>\n" );
		gi.Printf( "       ship damage|repair <system> <0..1> | breach <deck> <0..1> | source core|impulse|auxiliary|batteries on|off\n" );
		gi.Printf( "       ship seal <deck> | board <deck> <boarders> | borg <deck> <drones> | counterhack <system> <0..1>\n" );
		gi.Printf( "       ship chart | jump <beacon> | fire | character <name> <department> <rank>\n" );
		gi.Printf( "       ship order repair <system> | order security <deck> | order evacuate <deck> | order triage worst|rank\n" );
		gi.Printf( "       ship log [count] [scope] | seal <deck> | field <deck> on|off\n" );
		gi.Printf( "       ship personal [count] | personal write <text...>   (the private log; nobody else reads it)\n" );
		gi.Printf( "       ship losses [sealed|stripped|uninhabitable|written] | writeoff <deck|system> [kind]\n" );
		gi.Printf( "       ship report [draft] | strike|edit <line> ... | soften <line> [f] | add <scope> <text> | sign|file [crew] | diff | purge\n" );
		gi.Printf( "       ship nav | navigation   (how far home, how long, and the change since the last entry)\n" );
		gi.Printf( "       ship promise <officer> <crew> <kind> <what> | promises | keep|break <n>\n" );
		gi.Printf( "       ship kit <tricorders> <phasers> <evsuits> <charge> | scan\n" );
		return;
	}
	ship::Tick( vessel, 0.0f );
	Publish();
	if ( !g_shipTest->integer ) PrintStatus();
}
