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
	int allocated, demand, health, output, manned, need, enabled, priority, station, control;
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

	// the breach puzzle, while a counter-hack is in progress at this console
	bool breaching;
	char codes[25][4];
	char targets[160];
	int picks[8], npicks;
	int line;           // the cell the cursor is on, within the row or column the rules allow
	int deadline;       // when the intruders' trace completes and whatever has been entered is sent
} screen;

const int BREACH_SIZE = 5, BREACH_BUFFER = 7;
const int BREACH_MS = 30000;    // the operator has this long

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
		int v[10];
		if ( !ReadRow( va( "lwh_ship_sys%d", i ), r.name, sizeof( r.name ), v, 10 ) ) break;
		r.allocated = v[0]; r.demand = v[1]; r.health = v[2]; r.output = v[3];
		r.manned = v[4]; r.need = v[5]; r.enabled = v[6]; r.priority = v[7]; r.station = v[8]; r.control = v[9];
		// a station shows the systems it operates; Engineering distributes power to all of them
		if ( screen.station == 0 || r.station == screen.station ) ++screen.systems;
	}
	ui.Cvar_VariableStringBuffer( "lwh_ship_header", screen.header, sizeof( screen.header ) );
	ui.Cvar_VariableStringBuffer( "lwh_ship_stores", screen.stores, sizeof( screen.stores ) );
	screen.alert = static_cast<int>( ui.Cvar_VariableValue( "lwh_ship_alert" ) );
	if ( screen.cursor >= screen.systems ) screen.cursor = screen.systems ? screen.systems - 1 : 0;
}

// Every command goes out under the station's name, so the ship can hold it to that station's
// authority and the operator's clearance; the screen itself decides nothing.
void Send( const char *command )
{
	if ( !Q_stricmpn( command, "ship ", 5 ) ) ui.Cmd_ExecuteText( EXEC_APPEND, va( "ship as %d %s\n", screen.station, command + 5 ) );
	else ui.Cmd_ExecuteText( EXEC_APPEND, va( "%s\n", command ) );
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

// ---- the breach puzzle ----------------------------------------------------------------------
//
// The rules are the ship's (ship_core: MakeBreach, BreachScore). This presents the grid she
// published, lets the operator walk it the way the rules allow -- along the top row first, then
// down a column, then along a row, and so on -- and sends her the picks to score.

int BreachCell( void )
{//the cell under the cursor: the first pick moves along the top row, odd picks down the last pick's column, even along its row
	if ( screen.npicks == 0 ) return screen.line;
	const int last = screen.picks[screen.npicks - 1];
	return screen.npicks % 2 == 1 ? screen.line * BREACH_SIZE + last % BREACH_SIZE : ( last / BREACH_SIZE ) * BREACH_SIZE + screen.line;
}

bool BreachPicked( int cell )
{
	for ( int i = 0; i < screen.npicks; ++i )
		if ( screen.picks[i] == cell ) return true;
	return false;
}

void BreachBegin( void )
{
	char grid[256];
	ui.Cvar_VariableStringBuffer( "lwh_breach_grid", grid, sizeof( grid ) );
	ui.Cvar_VariableStringBuffer( "lwh_breach_targets", screen.targets, sizeof( screen.targets ) );
	int n = 0;
	for ( char *tok = strtok( grid, " " ); tok && n < 25; tok = strtok( NULL, " " ) ) Q_strncpyz( screen.codes[n++], tok, sizeof( screen.codes[0] ) );
	if ( n != 25 ) return; //the ship has published nothing: stay on the station
	screen.breaching = true;
	screen.npicks = 0;
	screen.line = 0;
	screen.deadline = ui.Milliseconds() + BREACH_MS;
}

void BreachSubmit( void )
{
	char cmd[128] = "ship solve";
	for ( int i = 0; i < screen.npicks; ++i ) Q_strcat( cmd, sizeof( cmd ), va( " %d", screen.picks[i] ) );
	screen.breaching = false;
	ui.Cmd_ExecuteText( EXEC_APPEND, va( "ship as %d %s\n", screen.station, cmd + 5 ) );
}

bool BreachAct( int key )
{
	switch ( key )
	{
	case K_LEFTARROW: case K_UPARROW:
		screen.line = ( screen.line + BREACH_SIZE - 1 ) % BREACH_SIZE;
		return true;
	case K_RIGHTARROW: case K_DOWNARROW:
		screen.line = ( screen.line + 1 ) % BREACH_SIZE;
		return true;
	case K_ENTER: case K_KP_ENTER:
		if ( BreachPicked( BreachCell() ) ) return true; //a cell is used once
		screen.picks[screen.npicks++] = BreachCell();
		screen.line = 0;
		if ( screen.npicks >= BREACH_BUFFER ) BreachSubmit();
		return true;
	case 's': case 'S':
		if ( screen.npicks ) BreachSubmit();
		return true;
	case K_ESCAPE:
		screen.breaching = false; //abandoned: nothing is sent
		return true;
	}
	return true;
}

void BreachSubmit( void );

void BreachDraw( void )
{
	// Time is part of the puzzle: when it runs out, what has been entered is what is sent.
	const int left = screen.deadline - ui.Milliseconds();
	if ( left <= 0 )
	{
		if ( screen.npicks ) BreachSubmit(); else screen.breaching = false;
		return;
	}
	char system[64];
	ui.Cvar_VariableStringBuffer( "lwh_breach_system", system, sizeof( system ) );
	UI_FillRect( 0, 0, 640, 480, colorTable[CT_BLACK] );
	UI_FillRect( 20, 16, 600, 22, colorTable[CT_RED] );
	UI_DrawProportionalString( 44, 19, va( "INTRUSION COUNTERMEASURES  -  %s", system ), UI_SMALLFONT, colorTable[CT_BLACK] );
	UI_DrawProportionalString( 44, 60, "CODE MATRIX", UI_TINYFONT, colorTable[CT_LTORANGE] );
	const int cursor = BreachCell();
	for ( int c = 0; c < 25; ++c )
	{
		const int x = 60 + ( c % BREACH_SIZE ) * 56, y = 84 + ( c / BREACH_SIZE ) * 44;
		const bool picked = BreachPicked( c );
		// the line the rules allow the next pick from
		bool allowed;
		if ( screen.npicks == 0 ) allowed = c / BREACH_SIZE == 0;
		else if ( screen.npicks % 2 == 1 ) allowed = c % BREACH_SIZE == screen.picks[screen.npicks - 1] % BREACH_SIZE;
		else allowed = c / BREACH_SIZE == screen.picks[screen.npicks - 1] / BREACH_SIZE;
		if ( c == cursor ) UI_FillRect( x - 8, y - 6, 48, 34, colorTable[CT_DKPURPLE2] );
		UI_DrawProportionalString( x, y, screen.codes[c], UI_SMALLFONT,
			colorTable[picked ? CT_DKGREY : c == cursor ? CT_WHITE : allowed ? CT_LTGOLD1 : CT_LTPURPLE1] );
	}
	UI_DrawProportionalString( 380, 60, "SEQUENCES REQUIRED", UI_TINYFONT, colorTable[CT_LTORANGE] );
	{
		char copy[160];
		Q_strncpyz( copy, screen.targets, sizeof( copy ) );
		int row = 0;
		for ( char *seq = strtok( copy, "|" ); seq; seq = strtok( NULL, "|" ), ++row )
			UI_DrawProportionalString( 380, 84 + row * 28, seq, UI_SMALLFONT, colorTable[CT_LTBLUE1] );
	}
	UI_DrawProportionalString( 44, 330, va( "BUFFER  %d / %d", screen.npicks, BREACH_BUFFER ), UI_TINYFONT, colorTable[CT_LTORANGE] );
	UI_DrawProportionalString( 380, 330, va( "TRACE COMPLETES IN %d", ( left + 999 ) / 1000 ), UI_SMALLFONT, colorTable[left < 10000 ? CT_RED : CT_LTGOLD1] );
	UI_FillRect( 380, 352, 200, 6, colorTable[CT_DKPURPLE3] );
	UI_FillRect( 380, 352, 200 * left / BREACH_MS, 6, colorTable[left < 10000 ? CT_RED : CT_LTBLUE1] );
	for ( int i = 0; i < screen.npicks; ++i )
		UI_DrawProportionalString( 60 + i * 56, 348, screen.codes[screen.picks[i]], UI_SMALLFONT, colorTable[CT_WHITE] );
	UI_DrawProportionalString( 44, 426, "ARROWS move along the lit line   ENTER take the code   S send what you have   ESC abandon",
		UI_TINYFONT, colorTable[CT_LTPURPLE1] );
}

void Draw( void )
{
	Refresh();
	if ( screen.breaching ) { BreachDraw(); return; }

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

	char refusal[128];
	ui.Cvar_VariableStringBuffer( "lwh_ship_refusal", refusal, sizeof( refusal ) );
	if ( refusal[0] ) UI_DrawProportionalString( 44, 412, va( "REFUSED: %s", refusal ), UI_TINYFONT, colorTable[CT_RED] );

	// What is aboard that should not be, on every console; the outside, where it is worked from.
	char line[256];
	ui.Cvar_VariableStringBuffer( "lwh_ship_aboard", line, sizeof( line ) );
	if ( line[0] ) UI_DrawProportionalString( 44, 72, line, UI_TINYFONT, colorTable[CT_RED] );
	if ( screen.station == 1 )
	{
		ui.Cvar_VariableStringBuffer( "lwh_ship_enemy", line, sizeof( line ) );
		UI_DrawProportionalString( 44, 384, line[0] ? line : "NO CONTACTS", UI_SMALLFONT, colorTable[line[0] ? CT_RED : CT_LTBLUE1] );
		UI_DrawProportionalString( 44, 398, va( "OUR SHIELDS %d%%", static_cast<int>( ui.Cvar_VariableValue( "lwh_ship_shields" ) ) ),
			UI_TINYFONT, colorTable[CT_LTGOLD1] );
	}
	if ( screen.station == 3 )
	{
		ui.Cvar_VariableStringBuffer( "lwh_ship_chart", line, sizeof( line ) );
		UI_DrawProportionalString( 44, 384, line, UI_TINYFONT, colorTable[CT_LTGOLD1] );
	}
	char result[16];
	ui.Cvar_VariableStringBuffer( "lwh_breach_result", result, sizeof( result ) );
	if ( result[0] ) UI_DrawProportionalString( 320, 398, va( "LAST COUNTERMEASURE: %s%% EFFECTIVE", result ), UI_TINYFONT, colorTable[CT_LTBLUE1] );

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
		if ( r.control < 50 ) UI_DrawProportionalString( 520, y, "HIJACKED", UI_TINYFONT, colorTable[CT_RED] );
		else UI_DrawProportionalString( 520, y, va( "%3d%%", r.health ), UI_TINYFONT, colorTable[HealthColour( r.health )] );
		UI_DrawProportionalString( 584, y, va( "%d/%d", r.manned, r.need ), UI_TINYFONT,
			colorTable[r.manned >= r.need ? CT_LTBLUE1 : CT_RED] );
	}

	UI_DrawProportionalString( 44, 426, screen.station == 0
		? "UP/DOWN select   ENTER on/off   LEFT/RIGHT priority   1 2 3 condition green/yellow/red   ESC leave"
		: screen.station == 1 ? "UP/DOWN select   ENTER on/off   1 2 3 condition   F fire torpedo   H countermeasures   ESC leave"
		: screen.station == 3 ? "UP/DOWN select   ENTER on/off   J K L jump to the first, second, third beacon listed   H countermeasures   ESC leave"
		: "UP/DOWN select   ENTER on/off   H countermeasures   ESC leave", UI_TINYFONT, colorTable[CT_LTPURPLE1] );
}

// One operator action. Shared by the keyboard and by the `lwh_eng_key` command, so that what a
// test drives is exactly what a hand on the console drives.
bool Act( int key )
{
	Refresh();
	if ( screen.breaching ) return BreachAct( key );
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
		if ( screen.cursor > 0 )
		{
			Send( va( "ship priority \"%s\" %d", r.name, screen.sys[screen.cursor - 1].priority - 1 ) );
			--screen.cursor;
		}
		return true;
	case K_RIGHTARROW:
		if ( screen.cursor + 1 < screen.systems )
		{
			Send( va( "ship priority \"%s\" %d", r.name, screen.sys[screen.cursor + 1].priority + 1 ) );
			++screen.cursor;
		}
		return true;
	case 'f': case 'F': Send( "ship fire" ); return true;
	case 'h': case 'H': // countermeasures on the selected system: ask the ship for a puzzle, then present it
		Send( va( "ship breach \"%s\"", r.name ) );
		ui.Cmd_ExecuteText( EXEC_APPEND, "lwh_eng_key breachopen\n" ); //after the ship has published it
		return true;
	case 'j': case 'J': case 'k': case 'K': case 'l': case 'L':
	{
		char links[64];
		ui.Cvar_VariableStringBuffer( "lwh_ship_links", links, sizeof( links ) );
		int want = ( key | 32 ) - 'j', n = 0;
		for ( char *tok = strtok( links, " " ); tok; tok = strtok( NULL, " " ), ++n )
			if ( n == want ) { Send( va( "ship jump %s", tok ) ); break; }
		return true;
	}
	// whether this station, and this operator, may call the alert is the ship's to say
	case '1': Send( "ship alert green" ); return true;
	case '2': Send( "ship alert yellow" ); return true;
	case '3': Send( "ship alert red" ); return true;
	}
	return false;
}

sfxHandle_t Key( int key )
{
	if ( screen.breaching ) { BreachAct( key ); return menu_null_sound; } //ESC abandons the puzzle, not the console
	if ( Act( key ) ) return menu_null_sound;
	return Menu_DefaultKey( &screen.menu, key ); // ESC and the rest
}

void Open( int station )
{
	screen.station = station;
	screen.cursor = 0;
	screen.breaching = false;
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
	if ( !Q_stricmp( name, "escape" ) ) return K_ESCAPE;
	return name[0] && !name[1] ? name[0] : 0;
}

// Read the turbolift's deck list exactly as the menu reads it (UI_LanguageFilename + ui.FS_ReadFile,
// so the same pak search order), and report it. The merged ship's pak carries our fifteen-deck list
// to override the retail ten-deck one; a headless run needs to see which one wins, and the menu
// itself offers no way to ask. Each deck's command is the third quoted string on its line.
void ReportTurboliftDecks( void )
{
	char base[] = "ext_data/sp_turbolift", ext[] = "dat", filename[MAX_QPATH];
	UI_LanguageFilename( base, ext, filename );
	void *buf = NULL;
	const int len = ui.FS_ReadFile( filename, &buf );
	if ( len <= 0 || !buf )
	{
		ui.Printf( "LWH: turbolift deck list %s not found\n", filename );
		return;
	}
	const char *text = static_cast<const char *>( buf );
	int decks = 0, highest = 0;
	for ( const char *p = text; *p; )
	{
		if ( p[0] == 'D' && p[1] == 'E' && p[2] == 'C' && p[3] == 'K' && p[4] >= '0' && p[4] <= '9' )
		{
			const int n = atoi( p + 4 );
			char command[64] = "";
			const char *q = p;
			int quotes = 0;
			while ( *q && *q != '\n' && quotes < 6 )
			{
				if ( *q++ != '"' ) continue;
				if ( ++quotes != 5 ) continue;
				int i = 0;
				while ( *q && *q != '"' && i < static_cast<int>( sizeof( command ) ) - 1 ) command[i++] = *q++;
				command[i] = 0;
			}
			++decks;
			highest = n;
			ui.Printf( "LWH: turbolift deck %d: %s\n", n, command );
			p = q;
			continue;
		}
		++p;
	}
	ui.Printf( "LWH: turbolift deck list %s: %d decks, highest %d\n", filename, decks, highest );
	ui.FS_FreeFile( buf );
}

} // namespace

qboolean LWH_UI_ConsoleCommand( const char *cmd )
{
	if ( LWH_UI_CommandScreens( cmd ) ) return qtrue;
	if ( !Q_stricmp( cmd, "lwh_ui_turbolift" ) ) { ReportTurboliftDecks(); return qtrue; }
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
		if ( !Q_stricmp( arg, "breachopen" ) ) BreachBegin();
		else Act( KeyByName( arg ) );
		return qtrue;
	}
	return qfalse;
}
