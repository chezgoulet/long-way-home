// g_ship.cpp -- hosts the ship simulation in the game module: ticks it with the game clock, keeps
// it in the save, carries it across the turbolift, and exposes it to the console.
//
// The simulation itself is ship_core, which knows nothing of the engine. This file is the only
// place the two meet. Consoles (S2 onward) and embodied crew (S5) reach the ship through Ship_Get().

#include "g_local.h"

#include "ship_core.h"
#include "g_ship.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

extern qboolean g_qbLoadTransition;

namespace {

const unsigned long SAVE_CHUNK = 0x53484950; // 'SHIP'
const char *const CARRY_FILE = "ship/carry.ship";

cvar_t *g_ship;         // 1 = the ship simulation runs
cvar_t *g_shipDayScale; // ship seconds per game second (60 = a day in 24 minutes; 1 = real time)
cvar_t *g_shipTest;     // harness: 1 = act, report, save, quit; 2 = report what a load restored, quit

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
	int order[ship::SYS_COUNT];
	for ( int i = 0; i < ship::SYS_COUNT; ++i ) order[i] = i;
	std::stable_sort( order, order + ship::SYS_COUNT, []( int a, int b ) { return vessel.systems[a].priority < vessel.systems[b].priority; } );
	for ( int k = 0; k < ship::SYS_COUNT; ++k )
	{
		const ship::System &sys = vessel.systems[order[k]];
		const ship::SystemSpec &spec = ship::Spec( static_cast<ship::SystemId>( order[k] ) );
		gi.cvar_set( Fmt( "lwh_ship_sys%d", k ).c_str(), Fmt( "%s|%d %d %d %d %d %d %d %d", spec.name, sys.allocated, spec.demand,
			static_cast<int>( sys.health * 100 + 0.5f ), static_cast<int>( sys.output * 100 + 0.5f ), sys.manned, spec.crewNeeded,
			sys.enabled ? 1 : 0, sys.priority ).c_str() );
	}
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
	g_shipTest = gi.cvar( "g_shipTest", "0", 0 );
}

void Ship_Init( void )
{
	active = g_ship && g_ship->integer;
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
	vessel = ship::NewShip( cfg );
	gi.Printf( "SHIP: simulation active, %d crew, a day every %.0f minutes\n",
		static_cast<int>( vessel.crew.size() ), ship::SECONDS_PER_DAY / cfg.dayScale / 60.0f );
}

void Ship_Frame( void )
{
	if ( !active ) return;
	if ( !pendingSave.empty() )
	{
		if ( ship::Unpack( pendingSave.data(), pendingSave.size(), vessel ) )
			gi.Printf( "SHIP: restored from the save, day %d\n", vessel.Day() );
		else
			gi.Printf( S_COLOR_YELLOW"SHIP: the save's ship state could not be read; keeping a new ship\n" );
		pendingSave.clear();
	}
	const int ms = level.time - level.previousTime;
	if ( ms > 0 && ms < 1000 ) ship::Tick( vessel, ms / 1000.0f );
	if ( level.time / 250 != level.previousTime / 250 ) Publish();
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
	const char *cmd = gi.argc() > 1 ? gi.argv( 1 ) : "status";
	const char *a = gi.argc() > 2 ? gi.argv( 2 ) : "";
	const char *b = gi.argc() > 3 ? gi.argv( 3 ) : "";
	const int sys = FindSystem( a );

	if ( !Q_stricmp( cmd, "status" ) ) { PrintStatus(); return; }
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
	else if ( !Q_stricmp( cmd, "source" ) && FindSource( a ) >= 0 && b[0] )
		ship::SetSourceOnline( vessel, static_cast<ship::SourceId>( FindSource( a ) ), !Q_stricmp( b, "on" ) );
	else
	{
		gi.Printf( "usage: ship status | alert green|yellow|red | on|off <system> | priority <system> <n>\n" );
		gi.Printf( "       ship damage|repair <system> <0..1> | breach <deck> <0..1> | source core|impulse|auxiliary|batteries on|off\n" );
		return;
	}
	ship::Tick( vessel, 0.0f );
	Publish();
	if ( !g_shipTest->integer ) PrintStatus();
}
