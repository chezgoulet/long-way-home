// g_ship.cpp -- hosts the ship simulation in the game module: ticks it with the game clock, keeps
// it in the save, carries it across the turbolift, and exposes it to the console.
//
// The simulation itself is ship_core, which knows nothing of the engine. This file is the only
// place the two meet. Consoles (S2 onward) and embodied crew (S5) reach the ship through Ship_Get().

#include "g_local.h"

#include "ship_core.h"
#include "g_ship.h"
#include "g_scope.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
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
	static const int CAPACITY[ship::SRC_COUNT] = { 1000, 300, 120, 80 };
	const int sod = vessel.SecondOfDay();

	gi.cvar_set( "lwh_ship_alert", Fmt( "%d", vessel.alert ).c_str() );
	gi.cvar_set( "lwh_ship_header", Fmt( "DAY %d  %02d:%02d  %s WATCH   CONDITION %s   POWER %d SUPPLIED  %d ALLOCATED", vessel.Day(),
		sod / 3600, sod % 3600 / 60, WATCH[vessel.Watch()], ALERTS[vessel.alert], vessel.PowerAvailable(), vessel.PowerAllocated() ).c_str() );
	gi.cvar_set( "lwh_ship_stores", Fmt( "DEUTERIUM %.1f%%   ANTIMATTER %.1f%%   BATTERIES %.0f%%   TORPEDOES %d   CREW FIT %d OF %d",
		vessel.stores.deuterium * 100, vessel.stores.antimatter * 100, vessel.stores.batteries * 100, vessel.stores.torpedoes,
		vessel.CrewFit(), static_cast<int>( vessel.crew.size() ) ).c_str() );
	for ( int i = 0; i < ship::SRC_COUNT; ++i )
	{
		const ship::Source &src = vessel.sources[i];
		gi.cvar_set( Fmt( "lwh_ship_src%d", i ).c_str(), Fmt( "%s|%d %d %d %d", SOURCES[i], src.output, CAPACITY[i],
			static_cast<int>( src.health * 100 + 0.5f ), src.online ? 1 : 0 ).c_str() );
	}
	// The outside, for Tactical and the Conn.
	gi.cvar_set( "lwh_ship_enemy", !vessel.enemy.present ? "" : Fmt( "%s   HULL %d%%   SHIELDS %d%%%s", vessel.enemy.borg ? "BORG VESSEL" : "HOSTILE VESSEL",
		static_cast<int>( vessel.enemy.hull * 100 + 0.5f ), static_cast<int>( vessel.enemy.shields * 100 + 0.5f ),
		vessel.enemy.hull <= 0.0f ? "   DESTROYED" : "" ).c_str() );
	gi.cvar_set( "lwh_ship_shields", Fmt( "%d", static_cast<int>( vessel.shieldStrength * 100 + 0.5f ) ).c_str() );
	{
		static const char *const KINDS[] = { "EMPTY SPACE", "HOSTILE", "DERELICT", "BORG" };
		std::string chart = Fmt( "AT BEACON %d OF %d.   JUMPS:", vessel.beacon, static_cast<int>( vessel.sector.size() ) - 1 );
		std::string links;
		for ( int l : vessel.sector[vessel.beacon].links )
		{
			chart += Fmt( "   [%d] %s", l, vessel.sector[l].visited ? KINDS[vessel.sector[l].kind] : "UNCHARTED" );
			links += Fmt( "%d ", l );
		}
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
		}
		gi.cvar_set( "lwh_ship_aboard", aboard.c_str() );
	}

	// Standing orders and who the player is, for the command console and the personnel screen.
	{
		std::string orders;
		if ( vessel.orderRepairFirst >= 0 ) orders += Fmt( "SEE FIRST TO %s.   ", ship::Spec( static_cast<ship::SystemId>( vessel.orderRepairFirst ) ).name );
		if ( vessel.orderSecurityTo ) orders += Fmt( "GUARD ON DECK %d.   ", vessel.orderSecurityTo );
		if ( vessel.orderEvacuate ) orders += Fmt( "DECK %d EVACUATED.", vessel.orderEvacuate );
		gi.cvar_set( "lwh_ship_orders", orders.c_str() );
		static const char *const RANKS[] = { "Crewman", "Ensign", "Lt. j.g.", "Lieutenant", "Lt. Commander", "Commander", "Captain" };
		const bool chosen = vessel.player >= 0 && vessel.player < static_cast<int>( vessel.crew.size() );
		gi.cvar_set( "lwh_ship_player", chosen ? Fmt( "%s %s", RANKS[vessel.crew[vessel.player].rank], vessel.crew[vessel.player].name.c_str() ).c_str() : "" );
	}

	int order[ship::SYS_COUNT];
	for ( int i = 0; i < ship::SYS_COUNT; ++i ) order[i] = i;
	std::stable_sort( order, order + ship::SYS_COUNT, []( int a, int b ) { return vessel.systems[a].priority < vessel.systems[b].priority; } );
	for ( int k = 0; k < ship::SYS_COUNT; ++k )
	{
		const ship::System &sys = vessel.systems[order[k]];
		const ship::SystemSpec &spec = ship::Spec( static_cast<ship::SystemId>( order[k] ) );
		gi.cvar_set( Fmt( "lwh_ship_sys%d", k ).c_str(), Fmt( "%s|%d %d %d %d %d %d %d %d %d %d", spec.name, sys.allocated, spec.demand,
			static_cast<int>( sys.health * 100 + 0.5f ), static_cast<int>( sys.output * 100 + 0.5f ), sys.manned, spec.crewNeeded,
			sys.enabled ? 1 : 0, sys.priority, ship::StationOf( static_cast<ship::SystemId>( order[k] ) ),
			static_cast<int>( sys.control * 100 + 0.5f ) ).c_str() );
	}
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
		if ( deck != from ) gi.SendConsoleCommand( Fmt( "use d%02d_tour_turbo_%02d\n", from, deck ).c_str() );
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

} // namespace

ship::Ship *Ship_Get( void ) { return active ? &vessel : NULL; }

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
	if ( level.time / 250 != level.previousTime / 250 ) Publish();
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
		const char *why = NULL;
		const bool isAlert = !Q_stricmp( cmd, "alert" );
		const bool isSwitch = !Q_stricmp( cmd, "on" ) || !Q_stricmp( cmd, "off" );
		const bool isPriority = !Q_stricmp( cmd, "priority" );
		const bool isFire = !Q_stricmp( cmd, "fire" );
		const bool isJump = !Q_stricmp( cmd, "jump" );
		const bool isBreach = !Q_stricmp( cmd, "breach" ) || !Q_stricmp( cmd, "solve" );
		// until a character is chosen the player is nobody in particular, and is not held to a rank
		const bool anyone = vessel.player < 0 && vessel.cfg.role != ship::ROLE_IN_COMMAND;
		if ( !isAlert && !isSwitch && !isPriority && !isFire && !isJump && !isBreach ) why = "that is not a console's to do";
		else if ( !anyone && !ship::PlayerMayOperate( vessel, st ) ) why = "you are not cleared for this station";
		else if ( isAlert && !( st == ship::STN_ENGINEERING || st == ship::STN_TACTICAL ) ) why = "the alert is not called from this station";
		else if ( isAlert && !anyone && vessel.cfg.role != ship::ROLE_IN_COMMAND && !ship::MayCallAlert( vessel.crew[vessel.player], st ) )
			why = "calling the alert needs a lieutenant or above";
		else if ( isPriority && st != ship::STN_ENGINEERING ) why = "the power order is Engineering's to set";
		else if ( isFire && st != ship::STN_TACTICAL ) why = "weapons are fired from Tactical";
		else if ( isJump && st != ship::STN_CONN ) why = "the ship is flown from the Conn";
		else if ( !Q_stricmp( cmd, "breach" ) && ( sys < 0 || !ship::OperatedFrom( static_cast<ship::SystemId>( sys ), st ) ) )
			why = "that system is not operated from this station";
		else if ( ( isSwitch || isPriority ) && ( sys < 0 || !ship::OperatedFrom( static_cast<ship::SystemId>( sys ), st ) ) )
			why = "that system is not operated from this station";
		else if ( ( isSwitch || isPriority ) && ship::Hijacked( vessel, static_cast<ship::SystemId>( sys ) ) )
			why = "the system does not answer: it is not ours";
		if ( why )
		{
			gi.Printf( "SHIP: %s refused at %s: %s\n", cmd, ship::StationName( st ), why );
			gi.cvar_set( "lwh_ship_refusal", why );
			return;
		}
		gi.cvar_set( "lwh_ship_refusal", "" );
	}

	if ( !Q_stricmp( cmd, "status" ) ) { PrintStatus(); return; }
	if ( !Q_stricmp( cmd, "role" ) )
	{//take up the role g_shipRole names (0 any post, 1 in command, 2 Munro)
		ship::SetRole( vessel, g_shipRole->integer == 1 ? ship::ROLE_IN_COMMAND : g_shipRole->integer == 2 ? ship::ROLE_MUNRO : ship::ROLE_ANY_POST );
		Publish();
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
	else if ( !Q_stricmp( cmd, "damage" ) && sys >= 0 && b[0] ) ship::DamageSystem( vessel, static_cast<ship::SystemId>( sys ), atof( b ) );
	else if ( !Q_stricmp( cmd, "repair" ) && sys >= 0 && b[0] ) ship::Repair( vessel, static_cast<ship::SystemId>( sys ), atof( b ) );
	else if ( !Q_stricmp( cmd, "breach" ) && a[0] && b[0] ) ship::BreachDeck( vessel, atoi( a ), atof( b ) );
	else if ( !Q_stricmp( cmd, "seal" ) && a[0] ) ship::RepairDeck( vessel, atoi( a ), 1.0f );
	else if ( !Q_stricmp( cmd, "board" ) && a[0] && b[0] ) ship::Board( vessel, atoi( a ), atoi( b ) );
	else if ( !Q_stricmp( cmd, "borg" ) && a[0] && b[0] ) ship::BoardBorg( vessel, atoi( a ), atoi( b ) );
	else if ( !Q_stricmp( cmd, "order" ) && a[0] )
	{//ship order repair <system>|security <deck>|evacuate <deck>   (0 or "none" clears it)
		bool ok = false;
		if ( !Q_stricmp( a, "repair" ) ) ok = ship::OrderRepairFirst( vessel, FindSystem( b ) );
		else if ( !Q_stricmp( a, "security" ) ) ok = ship::OrderSecurityTo( vessel, atoi( b ) );
		else if ( !Q_stricmp( a, "evacuate" ) ) ok = ship::OrderEvacuate( vessel, atoi( b ) );
		gi.cvar_set( "lwh_ship_order_refused", ok ? "" : "only whoever commands the ship gives orders" );
		if ( !ok ) { gi.Printf( "SHIP: order refused: only whoever commands the ship gives orders\n" ); return; }
		gi.Printf( "SHIP: standing orders: repair first %s, security to deck %d, evacuate deck %d\n",
			vessel.orderRepairFirst >= 0 ? ship::Spec( static_cast<ship::SystemId>( vessel.orderRepairFirst ) ).name : "nothing in particular",
			vessel.orderSecurityTo, vessel.orderEvacuate );
		Publish();
		return;
	}
	else if ( !Q_stricmp( cmd, "character" ) && a[0] && b[0] && gi.argc() > 4 )
	{//ship character <name> <department 0-4> <rank 0-4>
		const int who = ship::CreateCharacter( vessel, a, static_cast<ship::Department>( atoi( b ) ), atoi( gi.argv( 4 ) ) );
		if ( who < 0 ) gi.Printf( "SHIP: no such character can be created (department 0-4: command, engineering, security, sciences, medical; rank 0-4)\n" );
		else gi.Printf( "SHIP: you are %s, crew number %d\n", vessel.crew[who].name.c_str(), who );
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
	else if ( !Q_stricmp( cmd, "chart" ) )
	{
		static const char *const KINDS[] = { "empty", "hostile", "derelict", "Borg" };
		for ( size_t i = 0; i < vessel.sector.size(); ++i )
		{
			std::string links;
			for ( int l : vessel.sector[i].links ) links += Fmt( " %d", l );
			gi.Printf( "SHIP: %sbeacon %2d  %-8s  jumps to%s\n", static_cast<int>( i ) == vessel.beacon ? "> " : "  ", static_cast<int>( i ),
				vessel.sector[i].visited ? KINDS[vessel.sector[i].kind] : "unknown", links.c_str() );
		}
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
		gi.Printf( "       ship order repair <system> | order security <deck> | order evacuate <deck>\n" );
		return;
	}
	ship::Tick( vessel, 0.0f );
	Publish();
	if ( !g_shipTest->integer ) PrintStatus();
}
