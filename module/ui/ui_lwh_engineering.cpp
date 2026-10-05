// ui_lwh_engineering.cpp -- Main Engineering's console: EPS power distribution (gate S2).
//
// Compiled into the UI module. The ship lives in the game module, a different library, so the two
// talk the way this engine's modules always have: the game publishes the ship's state into cvars
// (lwh_ship_*, written by g_ship.cpp), and this screen sends the game its `ship` commands.
// Nothing here simulates anything; it shows what the ship says and asks for what the operator wants.
//
// Operation: up/down choose a system, ENTER switches it on or off, left/right move it up or down
// the power priority list, 1/2/3 set condition green/yellow/red, ESC leaves.

#include "ui_local.h"

#include "lwh_ui.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

const int MAX_SYSTEMS = 32;
const int MAX_SOURCES = 8;

struct SystemRow {
	char name[32];
	int allocated, demand, health, output, manned, need, enabled, priority, station;
};

struct SourceRow {
	char name[32];
	int output, capacity, health, online;
};

struct Screen {
	menuframework_s menu;
	int cursor;
	int station;        // which console this is: 0 Engineering (sees all), 1 Tactical, 2 Ops, 3 Conn, 4 Sickbay
	int systems, sources;
	SystemRow sys[MAX_SYSTEMS];
	SourceRow src[MAX_SOURCES];
	char header[128];
	char stores[128];
	int alert;
} screen;

// "name|a b c ..." -- the name may contain spaces, the numbers follow the bar.
bool ReadRow( const char *cvar, char *name, int nameSize, int *out, int count )
{
	char buf[256];
	ui.Cvar_VariableStringBuffer( cvar, buf, sizeof( buf ) );
	char *bar = strchr( buf, '|' );
	if ( !bar ) return false;
	*bar = 0;
	Q_strncpyz( name, buf, nameSize );
	char *p = bar + 1;
	for ( int i = 0; i < count; ++i )
	{
		char *end = p;
		out[i] = static_cast<int>( strtol( p, &end, 10 ) );
		if ( end == p ) return false;
		p = end;
	}
	return true;
}

void Refresh( void )
{
	screen.systems = screen.sources = 0;
	for ( int i = 0; i < MAX_SOURCES; ++i )
	{
		SourceRow &r = screen.src[screen.sources];
		int v[4];
		if ( !ReadRow( va( "lwh_ship_src%d", i ), r.name, sizeof( r.name ), v, 4 ) ) break;
		r.output = v[0]; r.capacity = v[1]; r.health = v[2]; r.online = v[3];
		++screen.sources;
	}
	for ( int i = 0; i < MAX_SYSTEMS; ++i )
	{
		SystemRow &r = screen.sys[screen.systems];
		int v[9];
		if ( !ReadRow( va( "lwh_ship_sys%d", i ), r.name, sizeof( r.name ), v, 9 ) ) break;
		r.allocated = v[0]; r.demand = v[1]; r.health = v[2]; r.output = v[3];
		r.manned = v[4]; r.need = v[5]; r.enabled = v[6]; r.priority = v[7]; r.station = v[8];
		// a station shows the systems it operates; Engineering distributes power to all of them
		if ( screen.station == 0 || r.station == screen.station ) ++screen.systems;
	}
	ui.Cvar_VariableStringBuffer( "lwh_ship_header", screen.header, sizeof( screen.header ) );
	ui.Cvar_VariableStringBuffer( "lwh_ship_stores", screen.stores, sizeof( screen.stores ) );
	screen.alert = static_cast<int>( ui.Cvar_VariableValue( "lwh_ship_alert" ) );
	if ( screen.cursor >= screen.systems ) screen.cursor = screen.systems ? screen.systems - 1 : 0;
}

void Send( const char *command )
{
	ui.Cmd_ExecuteText( EXEC_APPEND, va( "%s\n", command ) );
}

void Bar( int x, int y, int w, int h, int percent, int colour )
{
	if ( percent < 0 ) percent = 0;
	if ( percent > 100 ) percent = 100;
	UI_FillRect( x, y, w, h, colorTable[CT_DKPURPLE3] );
	if ( percent ) UI_FillRect( x, y, w * percent / 100, h, colorTable[colour] );
}

int HealthColour( int percent )
{
	return percent >= 75 ? CT_LTBLUE1 : percent >= 35 ? CT_LTORANGE : CT_RED;
}

void Draw( void )
{
	Refresh();

	static const int ALERT_COLOUR[3] = { CT_LTBLUE1, CT_YELLOW, CT_RED };
	const int alertColour = ALERT_COLOUR[screen.alert >= 0 && screen.alert < 3 ? screen.alert : 0];

	// The frame: a header bar, a left spine, a footer -- the LCARS shape, in flat colour.
	UI_FillRect( 0, 0, 640, 480, colorTable[CT_BLACK] );
	UI_FillRect( 20, 16, 600, 22, colorTable[alertColour] );
	UI_FillRect( 20, 42, 14, 396, colorTable[CT_DKPURPLE1] );
	UI_FillRect( 20, 442, 600, 10, colorTable[CT_DKPURPLE1] );
	static const char *const TITLES[] = { "MAIN ENGINEERING  -  EPS POWER DISTRIBUTION", "TACTICAL  -  WEAPONS AND DEFENCE",
		"OPERATIONS  -  SHIP'S SERVICES", "CONN  -  FLIGHT CONTROL", "SICKBAY  -  MEDICAL SYSTEMS" };
	UI_DrawProportionalString( 44, 19, TITLES[screen.station], UI_SMALLFONT, colorTable[CT_BLACK] );
	UI_DrawProportionalString( 44, 46, screen.header, UI_SMALLFONT, colorTable[CT_LTGOLD1] );
	UI_DrawProportionalString( 44, 62, screen.stores, UI_TINYFONT, colorTable[CT_LTPURPLE1] );

	if ( !screen.systems )
	{
		UI_DrawProportionalString( 44, 120, "NO SHIP DATA  -  the ship simulation is not running (g_ship 1)", UI_SMALLFONT, colorTable[CT_RED] );
		return;
	}

	// Generation
	UI_DrawProportionalString( 44, 82, "GENERATION", UI_TINYFONT, colorTable[CT_LTORANGE] );
	for ( int i = 0; i < screen.sources; ++i )
	{
		const SourceRow &r = screen.src[i];
		const int x = 44 + i * 146;
		UI_DrawProportionalString( x, 94, r.name, UI_TINYFONT, colorTable[r.online ? CT_LTBLUE1 : CT_DKGREY] );
		Bar( x, 107, 130, 8, r.capacity ? r.output * 100 / r.capacity : 0, HealthColour( r.health ) );
		UI_DrawProportionalString( x, 117, va( "%d / %d   %d%%%s", r.output, r.capacity, r.health, r.online ? "" : "  OFFLINE" ),
			UI_TINYFONT, colorTable[CT_LTPURPLE1] );
	}

	// Distribution: one line per system, in the order power is given out.
	UI_DrawProportionalString( 44, 136, "PRI  SYSTEM", UI_TINYFONT, colorTable[CT_LTORANGE] );
	UI_DrawProportionalString( 252, 136, "POWER", UI_TINYFONT, colorTable[CT_LTORANGE] );
	UI_DrawProportionalString( 400, 136, "OUTPUT", UI_TINYFONT, colorTable[CT_LTORANGE] );
	UI_DrawProportionalString( 520, 136, "CONDITION  CREW", UI_TINYFONT, colorTable[CT_LTORANGE] );
	for ( int i = 0; i < screen.systems; ++i )
	{
		const SystemRow &r = screen.sys[i];
		const int y = 149 + i * 15;
		const bool selected = i == screen.cursor;
		if ( selected ) UI_FillRect( 38, y - 1, 582, 14, colorTable[CT_DKPURPLE2] );
		const int text = !r.enabled ? CT_DKGREY : selected ? CT_WHITE : CT_LTGOLD1;
		UI_DrawProportionalString( 44, y, va( "%3d", r.priority ), UI_TINYFONT, colorTable[text] );
		UI_DrawProportionalString( 76, y, r.name, UI_TINYFONT, colorTable[text] );
		Bar( 252, y + 2, 100, 8, r.demand ? r.allocated * 100 / r.demand : 0, CT_LTBLUE1 );
		UI_DrawProportionalString( 358, y, r.enabled ? va( "%d/%d", r.allocated, r.demand ) : "OFF", UI_TINYFONT, colorTable[text] );
		Bar( 400, y + 2, 100, 8, r.output, HealthColour( r.health ) );
		UI_DrawProportionalString( 520, y, va( "%3d%%", r.health ), UI_TINYFONT, colorTable[HealthColour( r.health )] );
		UI_DrawProportionalString( 584, y, va( "%d/%d", r.manned, r.need ), UI_TINYFONT,
			colorTable[r.manned >= r.need ? CT_LTBLUE1 : CT_RED] );
	}

	UI_DrawProportionalString( 44, 426, screen.station == 0
		? "UP/DOWN select   ENTER on/off   LEFT/RIGHT priority   1 2 3 condition green/yellow/red   ESC leave"
		: screen.station == 1 ? "UP/DOWN select   ENTER on/off   1 2 3 condition green/yellow/red   ESC leave"
		: "UP/DOWN select   ENTER on/off   ESC leave", UI_TINYFONT, colorTable[CT_LTPURPLE1] );
}

// One operator action. Shared by the keyboard and by the `lwh_eng_key` command, so that what a
// test drives is exactly what a hand on the console drives.
bool Act( int key )
{
	Refresh();
	if ( !screen.systems ) return false;
	const SystemRow &r = screen.sys[screen.cursor];
	switch ( key )
	{
	case K_UPARROW:
		screen.cursor = ( screen.cursor + screen.systems - 1 ) % screen.systems;
		return true;
	case K_DOWNARROW:
		screen.cursor = ( screen.cursor + 1 ) % screen.systems;
		return true;
	case K_ENTER:
	case K_KP_ENTER:
		Send( va( "ship %s \"%s\"", r.enabled ? "off" : "on", r.name ) );
		return true;
	case K_LEFTARROW: // earlier in the list: fed sooner. Just ahead of the system above it.
		if ( screen.station != 0 ) return true; // the power order is Engineering's to set
		if ( screen.cursor > 0 )
		{
			Send( va( "ship priority \"%s\" %d", r.name, screen.sys[screen.cursor - 1].priority - 1 ) );
			--screen.cursor;
		}
		return true;
	case K_RIGHTARROW:
		if ( screen.station != 0 ) return true;
		if ( screen.cursor + 1 < screen.systems )
		{
			Send( va( "ship priority \"%s\" %d", r.name, screen.sys[screen.cursor + 1].priority + 1 ) );
			++screen.cursor;
		}
		return true;
	// the alert condition is called from Engineering or Tactical, not from the transporter room
	case '1': if ( screen.station <= 1 ) Send( "ship alert green" ); return true;
	case '2': if ( screen.station <= 1 ) Send( "ship alert yellow" ); return true;
	case '3': if ( screen.station <= 1 ) Send( "ship alert red" ); return true;
	}
	return false;
}

sfxHandle_t Key( int key )
{
	if ( Act( key ) ) return menu_null_sound;
	return Menu_DefaultKey( &screen.menu, key ); // ESC and the rest
}

void Open( int station )
{
	screen.station = station;
	screen.cursor = 0;
	memset( &screen.menu, 0, sizeof( screen.menu ) );
	screen.menu.draw = Draw;
	screen.menu.key = Key;
	// Not "fullscreen" in the engine's sense: a fullscreen menu stops the game being drawn, and in
	// this engine the game's frame is driven from its draw. The screen covers the view all the same.
	screen.menu.fullscreen = qfalse;
	screen.menu.wrapAround = qtrue;
	screen.menu.initialized = qtrue;
	UI_PushMenu( &screen.menu );
	// A console is operated while the ship carries on: tell the engine not to pause for this menu.
	// The engine clears the flag itself once no menu is open (patches/0006).
	ui.Cvar_Set( "ui_liveMenu", "1" );
}

int KeyByName( const char *name )
{
	if ( !Q_stricmp( name, "up" ) ) return K_UPARROW;
	if ( !Q_stricmp( name, "down" ) ) return K_DOWNARROW;
	if ( !Q_stricmp( name, "left" ) ) return K_LEFTARROW;
	if ( !Q_stricmp( name, "right" ) ) return K_RIGHTARROW;
	if ( !Q_stricmp( name, "enter" ) ) return K_ENTER;
	return name[0] && !name[1] ? name[0] : 0;
}

} // namespace

qboolean LWH_UI_ConsoleCommand( const char *cmd )
{
	if ( !Q_stricmp( cmd, "ui_lwh_engineering" ) )
	{
		Open( 0 );
		return qtrue;
	}
	if ( !Q_stricmp( cmd, "ui_lwh_station" ) )
	{
		char arg[32];
		ui.Argv( 1, arg, sizeof( arg ) );
		const int n = atoi( arg );
		Open( n >= 0 && n <= 4 ? n : 0 );
		return qtrue;
	}
	// The panels already in the ship call the retail game's station screens by these commands. With
	// the ship simulation running, each opens the working console for that station instead; without
	// it, the retail screen opens as it always did.
	if ( ui.Cvar_VariableValue( "g_ship" ) )
	{
		static const struct { const char *retail; int station; } PANELS[] = {
			{ "ui_engineeringstatus", 0 }, { "ui_tactical", 1 }, { "ui_ops", 2 }, { "ui_navigation", 3 },
		};
		for ( size_t i = 0; i < sizeof( PANELS ) / sizeof( PANELS[0] ); ++i )
		{
			if ( !Q_stricmp( cmd, PANELS[i].retail ) )
			{
				Open( PANELS[i].station );
				return qtrue;
			}
		}
	}
	if ( !Q_stricmp( cmd, "lwh_eng_key" ) )
	{
		char arg[32];
		ui.Argv( 1, arg, sizeof( arg ) );
		Act( KeyByName( arg ) );
		return qtrue;
	}
	return qfalse;
}
