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
	int share, allocBy; // the allocation a person set (per cent) and who set it (docs/power-assignment.md)
	int band; // the budget band it belongs to, for the band delegation (docs/power-assignment.md, Task C)
	int read; // 1 = the station reads it but does not operate it: a readout, not a control
};

// The provenance the ship reports: 0 unset, 1 the player, 2 an officer under standing orders, 3 auto.
const char *ProvenanceName( int by )
{
	switch ( by ) { case 1: return "PLAYER"; case 2: return "DELEGATE"; case 3: return "AUTO"; default: return "UNSET"; }
}

struct SourceRow {
	char name[32];
	int output, capacity, health, online;
};

struct Screen {
	menuframework_s menu;
	int cursor;
	int station;        // which console this is: 0 Engineering (sees all), 1 Tactical, 2 Ops, 3 Conn, 4 Sickbay
	char focus[32];     // the system this console's location is for: the cursor opens on it (location appropriateness)
	int systems, sources;
	SystemRow sys[MAX_SYSTEMS];
	SourceRow src[MAX_SOURCES];
	char header[128];
	char stores[128];
	char transporter[160]; // the instrument: the transporter's condition, stated before the act
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
	// The systems this station may READ but not operate (owner ruling, 2026-10-07). They are shown
	// below its own systems, and drawn as readouts; the operating station is still the only one that
	// may change them (the ship refuses the command, as s4-check proves). Controls first, so the
	// operating station's list is exactly what it was; the reads are appended after it.
	{
		char reads[512];
		ui.Cvar_VariableStringBuffer( va( "lwh_ship_reads%d", screen.station ), reads, sizeof( reads ) );
		char readNames[16][32];
		int nReads = 0;
		for ( char *tok = strtok( reads, "|" ); tok && nReads < 16; tok = strtok( NULL, "|" ) )
			Q_strncpyz( readNames[nReads++], tok, sizeof( readNames[0] ) );
		auto isRead = [&]( const char *n ) { for ( int k = 0; k < nReads; ++k ) if ( !Q_stricmp( readNames[k], n ) ) return true; return false; };

		SystemRow all[MAX_SYSTEMS];
		int nAll = 0;
		for ( int i = 0; i < MAX_SYSTEMS; ++i )
		{
			SystemRow &r = all[nAll];
			int v[13];
			if ( !ReadRow( va( "lwh_ship_sys%d", i ), r.name, sizeof( r.name ), v, 13 ) ) break;
			r.allocated = v[0]; r.demand = v[1]; r.health = v[2]; r.output = v[3];
			r.manned = v[4]; r.need = v[5]; r.enabled = v[6]; r.priority = v[7]; r.station = v[8]; r.control = v[9];
			r.share = v[10]; r.allocBy = v[11]; r.band = v[12];
			r.read = 0;
			++nAll;
		}
		for ( int pass = 0; pass < 2; ++pass )
			for ( int i = 0; i < nAll; ++i )
			{
				// A station's own systems are controls; Engineering distributes power to all of them;
				// the rest of its portfolio are reads. Anything else is not this console's business.
				const bool control = screen.station == 0 || all[i].station == screen.station;
				if ( pass == 0 ? !control : ( control || !isRead( all[i].name ) ) ) continue;
				all[i].read = pass == 1 ? 1 : 0;
				screen.sys[screen.systems++] = all[i];
			}
	}
	ui.Cvar_VariableStringBuffer( "lwh_ship_header", screen.header, sizeof( screen.header ) );
	ui.Cvar_VariableStringBuffer( "lwh_ship_stores", screen.stores, sizeof( screen.stores ) );
	ui.Cvar_VariableStringBuffer( "lwh_ship_transporter", screen.transporter, sizeof( screen.transporter ) );
	screen.alert = static_cast<int>( ui.Cvar_VariableValue( "lwh_ship_alert" ) );
	if ( screen.cursor >= screen.systems ) screen.cursor = screen.systems ? screen.systems - 1 : 0;
	// Location appropriateness: a console opens on the system its location is for (a transporter
	// console on the transporters, the environmental console on life support). Applied once, so the
	// hand still moves the cursor afterwards.
	if ( screen.focus[0] )
	{
		for ( int i = 0; i < screen.systems; ++i )
			if ( !Q_stricmp( screen.sys[i].name, screen.focus ) ) { screen.cursor = i; break; }
		screen.focus[0] = 0;
	}
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
	return percent >= 75 ? CT_LTBLUE2 : percent >= 35 ? CT_LTORANGE : CT_RED;
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
			UI_DrawProportionalString( 380, 84 + row * 28, seq, UI_SMALLFONT, colorTable[CT_LTBLUE2] );
	}
	UI_DrawProportionalString( 44, 330, va( "BUFFER  %d / %d", screen.npicks, BREACH_BUFFER ), UI_TINYFONT, colorTable[CT_LTORANGE] );
	UI_DrawProportionalString( 380, 330, va( "TRACE COMPLETES IN %d", ( left + 999 ) / 1000 ), UI_SMALLFONT, colorTable[left < 10000 ? CT_RED : CT_LTGOLD1] );
	UI_FillRect( 380, 352, 200, 6, colorTable[CT_DKPURPLE3] );
	UI_FillRect( 380, 352, 200 * left / BREACH_MS, 6, colorTable[left < 10000 ? CT_RED : CT_LTBLUE2] );
	for ( int i = 0; i < screen.npicks; ++i )
		UI_DrawProportionalString( 60 + i * 56, 348, screen.codes[screen.picks[i]], UI_SMALLFONT, colorTable[CT_WHITE] );
	UI_DrawProportionalString( 44, 426, "ARROWS move along the lit line   ENTER take the code   S send what you have   ESC abandon",
		UI_TINYFONT, colorTable[CT_LTPURPLE1] );
}

void Draw( void )
{
	Refresh();
	if ( screen.breaching ) { BreachDraw(); return; }

	static const int ALERT_COLOUR[3] = { CT_LTBLUE2, CT_YELLOW, CT_RED };
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
	if ( screen.station == 0 )
	{// the meeting's seam, made visible: the chief engineer's recommendation, and who holds a band
		ui.Cvar_VariableStringBuffer( "lwh_ship_recommend", line, sizeof( line ) );
		if ( line[0] ) UI_DrawProportionalString( 44, 468, va( "RECOMMENDATION: %s", line ), UI_TINYFONT, colorTable[CT_LTGOLD1] );
		ui.Cvar_VariableStringBuffer( "lwh_ship_grants", line, sizeof( line ) );
		if ( line[0] ) UI_DrawProportionalString( 44, 128, va( "DELEGATED: %s", line ), UI_TINYFONT, colorTable[CT_LTBLUE2] );
	}
	if ( screen.station == 1 )
	{
		// The phaser bank's setting: Tactical's standing decision, and the one the console carries
		// when there is no contact to shoot at (the Task C finding). V cycles it.
		ui.Cvar_VariableStringBuffer( "lwh_ship_yield", line, sizeof( line ) );
		if ( line[0] ) UI_DrawProportionalString( 44, 366, line, UI_TINYFONT, colorTable[CT_LTGOLD1] );
		ui.Cvar_VariableStringBuffer( "lwh_ship_enemy", line, sizeof( line ) );
		UI_DrawProportionalString( 44, 384, line[0] ? line : "NO CONTACTS", UI_SMALLFONT, colorTable[line[0] ? CT_RED : CT_LTBLUE2] );
		UI_DrawProportionalString( 44, 398, va( "OUR SHIELDS %d%%", static_cast<int>( ui.Cvar_VariableValue( "lwh_ship_shields" ) ) ),
			UI_TINYFONT, colorTable[CT_LTGOLD1] );
	}
	if ( screen.station == 3 )
	{
		ui.Cvar_VariableStringBuffer( "lwh_ship_chart", line, sizeof( line ) );
		UI_DrawProportionalString( 44, 384, line, UI_TINYFONT, colorTable[CT_LTGOLD1] );
	}
	if ( screen.station == 4 )
	{
		ui.Cvar_VariableStringBuffer( "lwh_ship_medical", line, sizeof( line ) );
		// TINYFONT, not SMALLFONT: the ward line carries seven fields and overflowed the 640px face
		// in the large font (see docs/evidence/access-and-authority.md, the usability finding).
		UI_DrawProportionalString( 44, 384, line[0] ? line : "NO MEDICAL DATA", UI_TINYFONT, colorTable[CT_LTBLUE2] );
	}
	if ( screen.station == 2 )
	{// the endurance clocks, the away kit and what the beacon here affords (Operations)
		ui.Cvar_VariableStringBuffer( "lwh_ship_clocks", line, sizeof( line ) );
		if ( line[0] ) UI_DrawProportionalString( 44, 384, line, UI_TINYFONT,
			colorTable[ui.Cvar_VariableValue( "lwh_ship_clocks_alarm" ) > 0.5f ? CT_RED : CT_LTBLUE2] );
		ui.Cvar_VariableStringBuffer( "lwh_ship_kit", line, sizeof( line ) );
		if ( line[0] ) UI_DrawProportionalString( 44, 398, line, UI_TINYFONT, colorTable[CT_LTGOLD1] );
		// The beacon-choice block (row 21): what the outside offers here, as keys rather than typed
		// commands. Operations speaks and hails; the Conn runs (X at the helm). Decision, not readout.
		ui.Cvar_VariableStringBuffer( "lwh_ship_beacon", line, sizeof( line ) );
		if ( line[0] ) UI_DrawProportionalString( 44, 352, line, UI_TINYFONT, colorTable[CT_LTGOLD1] );
	}
	char result[16];
	ui.Cvar_VariableStringBuffer( "lwh_breach_result", result, sizeof( result ) );
	if ( result[0] ) UI_DrawProportionalString( 320, 398, va( "LAST COUNTERMEASURE: %s%% EFFECTIVE", result ), UI_TINYFONT, colorTable[CT_LTBLUE2] );

	// Access, at this station: the emergency override in progress and any lock-out that has shut
	// this station's console. Both name who did it, because the record is the point
	// (docs/access-and-authority.md, owner decision 2026-10-07).
	{
		char ov[128], locks[512];
		ui.Cvar_VariableStringBuffer( "lwh_ship_override", ov, sizeof( ov ) );
		ui.Cvar_VariableStringBuffer( "lwh_ship_lockouts", locks, sizeof( locks ) );
		if ( ov[0] ) UI_DrawProportionalString( 44, 408, ov, UI_TINYFONT,
			colorTable[strstr( ov, "ACTIVE" ) ? CT_RED : CT_LTORANGE] );
		else if ( locks[0] )
			UI_DrawProportionalString( 44, 408, va( "ACCESS LOCKED OUT AT THIS STATION: %s", locks ), UI_TINYFONT, colorTable[CT_RED] );
	}

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
		UI_DrawProportionalString( x, 94, r.name, UI_TINYFONT, colorTable[r.online ? CT_LTBLUE2 : CT_DKGREY] );
		Bar( x, 107, 130, 8, r.capacity ? r.output * 100 / r.capacity : 0, HealthColour( r.health ) );
		UI_DrawProportionalString( x, 117, va( "%d / %d   %d%%%s", r.output, r.capacity, r.health, r.online ? "" : "  OFFLINE" ),
			UI_TINYFONT, colorTable[CT_LTPURPLE1] );
	}

	// The post's information portfolio (docs/scenario-atlas.md, "what you can see"; owner addition
	// 2026-10-07): the console shows what the JOB is expected to see, which is wider than the systems
	// the station operates. The module publishes it in lwh_ship_port<N>, one line per station; the
	// names are the retail Virtual Voyager station menus, sourced per portfolio in docs/lore-ledger.md.
	{
		char portfolio[256];
		ui.Cvar_VariableStringBuffer( va( "lwh_ship_port%d", screen.station ), portfolio, sizeof( portfolio ) );
		if ( portfolio[0] ) UI_DrawProportionalString( 44, 128, va( "READS  %s", portfolio ), UI_TINYFONT, colorTable[CT_LTBLUE2] );
	}

	// Distribution: one line per system, in the order power is given out. Each carries the FTL
	// numbers the brief requires (docs/power-assignment.md, Task B): what a person set (SET), the
	// power it is getting (POWER bar and a/b), what that power buys (BUYS), and who decided (WHO).
	UI_DrawProportionalString( 44, 138, "PRI SYSTEM", UI_TINYFONT, colorTable[CT_LTORANGE] );
	UI_DrawProportionalString( 236, 138, "SET", UI_TINYFONT, colorTable[CT_LTORANGE] );
	UI_DrawProportionalString( 268, 138, "POWER GETTING", UI_TINYFONT, colorTable[CT_LTORANGE] );
	UI_DrawProportionalString( 420, 138, "BUYS", UI_TINYFONT, colorTable[CT_LTORANGE] );
	UI_DrawProportionalString( 520, 138, "WHO", UI_TINYFONT, colorTable[CT_LTORANGE] );
	UI_DrawProportionalString( 584, 138, "COND CREW", UI_TINYFONT, colorTable[CT_LTORANGE] );
	for ( int i = 0; i < screen.systems; ++i )
	{
		const SystemRow &r = screen.sys[i];
		const int y = 150 + i * 13;
		const bool selected = i == screen.cursor;
		if ( selected ) UI_FillRect( 38, y - 1, 582, 12, colorTable[CT_DKPURPLE2] );
		// A read is not a control: a row this station may read but not operate is tagged RD and drawn
		// in the read colour, so the difference is visible (owner ruling, 2026-10-07).
		const int text = r.read ? CT_LTBLUE2 : ( !r.enabled ? CT_DKGREY : selected ? CT_WHITE : CT_LTGOLD1 );
		UI_DrawProportionalString( 44, y, r.read ? "RD" : va( "%2d", r.priority ), UI_TINYFONT, colorTable[text] );
		UI_DrawProportionalString( 76, y, r.read ? va( "%s (read)", r.name ) : r.name, UI_TINYFONT, colorTable[text] );
		// The allocation a person set, and whether the system is on: the two things a person decides.
		UI_DrawProportionalString( 236, y, r.enabled ? va( "%3d%%", r.share ) : " OFF", UI_TINYFONT,
			colorTable[!r.enabled ? CT_DKGREY : r.allocBy ? CT_WHITE : CT_LTPURPLE1] );
		Bar( 268, y + 2, 84, 8, r.demand ? r.allocated * 100 / r.demand : 0, CT_LTBLUE2 );
		UI_DrawProportionalString( 356, y, r.enabled ? va( "%d/%d", r.allocated, r.demand ) : "OFF", UI_TINYFONT, colorTable[text] );
		Bar( 420, y + 2, 64, 8, r.output, HealthColour( r.health ) );
		UI_DrawProportionalString( 490, y, va( "%3d%%", r.output ), UI_TINYFONT, colorTable[text] );
		UI_DrawProportionalString( 520, y, ProvenanceName( r.allocBy ), UI_TINYFONT,
			colorTable[r.allocBy == 3 ? CT_LTORANGE : r.allocBy ? CT_LTBLUE2 : CT_DKGREY] );
		if ( r.control < 50 ) UI_DrawProportionalString( 584, y, "HIJACK", UI_TINYFONT, colorTable[CT_RED] );
		else UI_DrawProportionalString( 584, y, va( "%3d %d/%d", r.health, r.manned, r.need ), UI_TINYFONT,
			colorTable[r.manned >= r.need ? CT_LTBLUE2 : CT_RED] );
	}

	// The navigation counter: the crew's shared fact, on every console (docs/navigation-counter.md).
	// It reads from the ship and cannot lie. Drawn below the system list where the list leaves room;
	// where it does not (Main Engineering shows all eighteen), the panel and `ship nav` carry it.
	{
		const int navY = 150 + screen.systems * 13 + 3;
		if ( navY <= 372 )
		{
			ui.Cvar_VariableStringBuffer( "lwh_ship_nav", line, sizeof( line ) );
			if ( line[0] ) UI_DrawProportionalString( 44, navY, line, UI_TINYFONT, colorTable[CT_LTBLUE2] );
		}
	}

	// The instrument (the condition gap): the Operations console states the transporter's condition
	// before the beam, so the operator has the risk before them. A read of state; it cannot lie.
	if ( screen.station == 2 && screen.transporter[0] )
		UI_DrawProportionalString( 44, 410, screen.transporter, UI_TINYFONT,
			colorTable[screen.alert == 2 ? CT_RED : screen.alert == 1 ? CT_LTORANGE : CT_LTBLUE2] );

	// Engineering shows every system, so its list runs to the foot of the frame: its controls go
	// below the frame's bottom bar, where the rest have room above it.
	const int footerY = screen.station == 0 ? 456 : 426;
	UI_DrawProportionalString( 44, footerY, screen.station == 0
		? "UP/DOWN  ENTER on/off  -/+ ALLOC  E auto  G accept  D decline  [ ] band  L/R priority  Y override  ESC"
		: screen.station == 1 ? "UP/DOWN  ENTER on/off  V phaser  A target  F fire  M remodulate  N vinculum  H counter  Y override  1 2 3  ESC leave"
		: screen.station == 2 ? "UP/DOWN  ENTER on/off  T beam  R recall  U survey  O field  L hail  M trade  A distress  H counter  Y override  ESC"
		: screen.station == 3 ? "UP/DOWN   ENTER on/off   J K L jump   C course   X run   H counter   Y override   ESC leave"
		: screen.station == 4 ? "UP/DOWN select   ENTER on/off   B surgical field   H countermeasures   Y override   ESC leave"
		: "UP/DOWN select   ENTER on/off   H countermeasures   Y override   ESC leave", UI_TINYFONT, colorTable[CT_LTPURPLE1] );
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
		if ( r.read ) return true; // a read is not a control (owner ruling, 2026-10-07)
		Send( va( "ship %s \"%s\"", r.enabled ? "off" : "on", r.name ) );
		return true;
	case K_LEFTARROW: // earlier in the list: fed sooner. Just ahead of the system above it.
		if ( r.read ) return true;
		if ( screen.cursor > 0 )
		{
			Send( va( "ship priority \"%s\" %d", r.name, screen.sys[screen.cursor - 1].priority - 1 ) );
			--screen.cursor;
		}
		return true;
	case K_RIGHTARROW:
		if ( r.read ) return true;
		if ( screen.cursor + 1 < screen.systems )
		{
			Send( va( "ship priority \"%s\" %d", r.name, screen.sys[screen.cursor + 1].priority + 1 ) );
			++screen.cursor;
		}
		return true;
	case 'f': case 'F': Send( "ship fire" ); return true;
	case 't': case 'T': // the transporter beams a party (Operations)
		if ( screen.station != 2 ) return false;
		Send( "ship transport 3" );
		return true;
	case 'r': case 'R': // bring the away team back
		if ( screen.station != 2 ) return false;
		Send( "ship recall" );
		return true;
	case 'u': case 'U': // astrometrics: open the survey board, where the target is the decision
		if ( screen.station != 2 ) return false;
		ui.Cmd_ExecuteText( EXEC_APPEND, "ui_lwh_survey\n" );
		return true;
	case 'm': case 'M': // trade with a trader (Operations); remodulate the phasers (Tactical)
	{
		if ( screen.station == 2 ) { Send( "ship trade" ); return true; }
		// The counter-play kit (docs/borg-incursion.md, the reachability pass): the phaser adapter's
		// rotating modulation, Tactical's act, reached only by `ship remodulate` before.
		if ( screen.station == 1 ) { Send( "ship remodulate" ); return true; }
		return false;
	}
	case 'a': case 'A': // answer a distress call (Operations); pick what to aim at (Tactical)
	{
		if ( screen.station == 2 ) { Send( "ship distress" ); return true; }
		// What the enemy's shields are down over: hull, weapons, engines or shield generator. The
		// standing combat decision, reached only by `ship target` before (docs/ship-systems.md).
		if ( screen.station == 1 )
		{
			static const char *const AIM[] = { "hull", "weapons", "engines", "shields" };
			const int next = ( static_cast<int>( ui.Cvar_VariableValue( "lwh_ship_target_idx" ) ) + 1 ) % 4;
			Send( va( "ship target %s", AIM[next] ) );
			return true;
		}
		return false;
	}
	case 'n': case 'N': // a vinculum raid: sever the Collective's local coordination (Tactical)
		if ( screen.station != 1 ) return false;
		Send( "ship vinculum" );
		return true;
	case 'x': case 'X': // the Conn runs from a fight it cannot win (the fourth choice, at the helm)
		if ( screen.station != 3 ) return false;
		Send( "ship run" );
		return true;
	case 'c': case 'C': // the Conn lays in a course for the far end of the sector
		if ( screen.station != 3 ) return false;
		Send( va( "ship course %d", static_cast<int>( ui.Cvar_VariableValue( "lwh_ship_goal" ) ) ) );
		return true;
	case 'o': case 'O': // raise or lower the field over the deck that is losing air (environmental control)
	{
		if ( screen.station != 2 ) return false;
		char deck[16];
		ui.Cvar_VariableStringBuffer( "lwh_ship_breach_deck", deck, sizeof( deck ) );
		if ( deck[0] ) Send( va( "ship field %s %s", deck, ui.Cvar_VariableValue( "lwh_ship_breach_field" ) > 0.5f ? "off" : "on" ) );
		return true;
	}
	case 'b': case 'B': // the surgical bay's force field (Sickbay)
		if ( screen.station != 4 ) return false;
		Send( "ship surgical" );
		return true;
	case 'v': case 'V': // the phaser bank's setting: Tactical's standing decision, there with no contact
		if ( screen.station != 1 ) return false;
		Send( va( "ship yield %d", ( static_cast<int>( ui.Cvar_VariableValue( "lwh_ship_yield_idx" ) ) + 1 ) % 4 ) );
		return true;
	// The FTL controls (docs/power-assignment.md, Task B and C): the allocation a person sets, the
	// automatic mode, the chief's recommendation, and a band delegation. Engineering's to work.
	case '-': case '=': case '+':
	{//the allocation itself: not a tier, a number. A decrease is always allowed; an increase the
	 //ship refuses is reported back in lwh_ship_refusal.
		if ( r.read || !r.enabled ) return true;
		int want = r.share + ( key == '-' ? -10 : 10 );
		if ( want < 0 ) want = 0;
		if ( want > 100 ) want = 100;
		Send( va( "ship alloc \"%s\" %d", r.name, want ) );
		return true;
	}
	case 'e': case 'E': // automatic mode: the ladder as policy, off by default
		Send( "ship auto toggle" );
		return true;
	case 'g': case 'G': // accept the chief engineer's recommendation
		Send( "ship accept" );
		return true;
	case 'd': case 'D': // decline it: recorded, and the officer remembers
		Send( "ship refuse" );
		return true;
	case '[': // grant the selected system's band to the chief engineer
	case ']': // ... and take it back, at once
	{
		char chief[16];
		ui.Cvar_VariableStringBuffer( "lwh_ship_chief", chief, sizeof( chief ) );
		Send( va( "ship band %s %d %s", key == '[' ? "grant" : "revoke", r.band, chief ) );
		return true;
	}
	case 'h': case 'H': // countermeasures on the selected system: ask the ship for a puzzle, then present it
		Send( va( "ship breach \"%s\"", r.name ) );
		ui.Cmd_ExecuteText( EXEC_APPEND, "lwh_eng_key breachopen\n" ); //after the ship has published it
		return true;
	case 'l': case 'L': // Operations hails; the Conn, where L is the second jump, does not (see below)
		if ( screen.station == 2 ) { Send( "ship hail" ); return true; }
		if ( screen.station != 3 ) return false;
		// fall through: at the Conn, L is the second of the jumps
	case 'j': case 'J': case 'k': case 'K':
	{
		if ( screen.station != 3 ) return false; // the ship is flown from the Conn (the station's own act)
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
	case 'y': case 'Y':
	{// the emergency override at this station: slow, logged, and usually two hands. Sent bare, not
	 // as the station, because the override is the act of reaching around the chain of command.
		char ov[128];
		ui.Cvar_VariableStringBuffer( "lwh_ship_override", ov, sizeof( ov ) );
		if ( ov[0] && strstr( ov, "PENDING" ) ) ui.Cmd_ExecuteText( EXEC_APPEND, va( "ship override confirm %d\n", screen.station ) );
		else if ( !ov[0] ) ui.Cmd_ExecuteText( EXEC_APPEND, va( "ship override begin %d\n", screen.station ) );
		return true;
	}
	}
	return false;
}

sfxHandle_t Key( int key )
{
	if ( screen.breaching ) { BreachAct( key ); return menu_null_sound; } //ESC abandons the puzzle, not the console
	if ( Act( key ) ) return menu_null_sound;
	return Menu_DefaultKey( &screen.menu, key ); // ESC and the rest
}

// A station console, with the system its location is for as the opening cursor (or none).
void Open( int station, const char *focus = NULL )
{
	screen.station = station;
	screen.cursor = 0;
	if ( focus ) Q_strncpyz( screen.focus, focus, sizeof( screen.focus ) );
	else screen.focus[0] = 0;
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

// ---- the triage screen (S4 / the triage gap) ---------------------------------------------------
//
// The ward, one row per casualty, in the order the triage standing order treats them -- read straight
// from the ship (lwh_ship_ward), as the station consoles read their cvars. It began as a read that
// could not act (the Task C finding). It now acts, and the actions are Sickbay's own: raise the
// surgical field, call the EMH, recover a crew member the Borg have begun to take. The triage ORDER
// is offered too, but it is command's -- the board sends it and the ship refuses anyone who does not
// command, which is the two-lock model rather than a special case (docs/access-and-authority.md).

struct {
	menuframework_s menu;
	int cursor;      // the casualty the cursor is on
	int captives;    // how many are in the Borg recovery window (the rows after the ward proper)
} triage;

bool TriageAct( int key )
{
	char ward[1024];
	ui.Cvar_VariableStringBuffer( "lwh_ship_ward", ward, sizeof( ward ) );
	int count = 0;
	for ( char *tok = strtok( ward, ";" ); tok; tok = strtok( NULL, ";" ), ++count ) {}
	switch ( key )
	{
	case K_UPARROW: if ( count ) triage.cursor = ( triage.cursor + count - 1 ) % count; return true;
	case K_DOWNARROW: if ( count ) triage.cursor = ( triage.cursor + 1 ) % count; return true;
	case 'b': case 'B': ui.Cmd_ExecuteText( EXEC_APPEND, "ship surgical\n" ); return true;
	case 'e': case 'E': ui.Cmd_ExecuteText( EXEC_APPEND, va( "ship emh %s\n", ui.Cvar_VariableValue( "lwh_ship_emh" ) > 0.5f ? "off" : "on" ) ); return true;
	case 't': case 'T': ui.Cmd_ExecuteText( EXEC_APPEND, va( "ship order triage %s\n", ui.Cvar_VariableValue( "lwh_ship_triage" ) > 0.5f ? "worst" : "rank" ) ); return true;
	case 'r': case 'R':
	{//de-assimilation: the first crew member still in the window is brought back, at a cost in
	 //supplies. Sickbay's most loaded decision, and the board was where it could not be made.
		char caps[512];
		ui.Cvar_VariableStringBuffer( "lwh_ship_captives", caps, sizeof( caps ) );
		char *tok = strtok( caps, ";" );
		if ( tok ) { char *bar = strchr( tok, '|' ); if ( bar ) ui.Cmd_ExecuteText( EXEC_APPEND, va( "ship recover %s\n", bar + 1 ) ); }
		return true;
	}
	}
	return false;
}

void TriageDraw( void )
{
	char medical[256], ward[1024], caps[512];
	ui.Cvar_VariableStringBuffer( "lwh_ship_medical", medical, sizeof( medical ) );
	ui.Cvar_VariableStringBuffer( "lwh_ship_captives", caps, sizeof( caps ) );
	UI_FillRect( 0, 0, 640, 480, colorTable[CT_BLACK] );
	UI_FillRect( 20, 16, 600, 22, colorTable[CT_LTBLUE2] );
	UI_FillRect( 20, 42, 14, 396, colorTable[CT_DKPURPLE1] );
	UI_FillRect( 20, 442, 600, 10, colorTable[CT_DKPURPLE1] );
	UI_DrawProportionalString( 44, 19, "SICKBAY  -  TRIAGE", UI_SMALLFONT, colorTable[CT_BLACK] );
	UI_DrawProportionalString( 44, 46, medical, UI_TINYFONT, colorTable[CT_LTGOLD1] );
	UI_DrawProportionalString( 44, 70, "NAME", UI_TINYFONT, colorTable[CT_LTORANGE] );
	UI_DrawProportionalString( 300, 70, "SEVERITY", UI_TINYFONT, colorTable[CT_LTORANGE] );
	UI_DrawProportionalString( 440, 70, "STATUS", UI_TINYFONT, colorTable[CT_LTORANGE] );
	ui.Cvar_VariableStringBuffer( "lwh_ship_ward", ward, sizeof( ward ) );
	int row = 0;
	for ( char *tok = strtok( ward, ";" ); tok && row < 16; tok = strtok( NULL, ";" ), ++row )
	{
		char *bar1 = strchr( tok, '|' );
		if ( !bar1 ) continue;
		*bar1 = 0;
		char *bar2 = strchr( bar1 + 1, '|' );
		if ( !bar2 ) continue;
		const int sev = atoi( bar1 + 1 ), care = atoi( bar2 + 1 );
		const int y = 88 + row * 16;
		if ( row == triage.cursor ) UI_FillRect( 38, y - 1, 582, 14, colorTable[CT_DKPURPLE2] );
		UI_DrawProportionalString( 44, y, tok, UI_SMALLFONT, colorTable[CT_WHITE] );
		Bar( 300, y + 2, 120, 10, sev, sev >= 70 ? CT_RED : sev >= 40 ? CT_LTORANGE : CT_LTBLUE2 );
		UI_DrawProportionalString( 300, y + 14, va( "%d%%", sev ), UI_TINYFONT, colorTable[CT_LTPURPLE1] );
		UI_DrawProportionalString( 440, y, care ? "ON A BED" : "WAITING", UI_SMALLFONT, colorTable[care ? CT_LTBLUE2 : CT_RED] );
	}
	if ( !row ) UI_DrawProportionalString( 44, 88, "THE WARD IS EMPTY", UI_SMALLFONT, colorTable[CT_LTBLUE2] );

	// The recovery window (docs/borg-incursion.md): the crew the Borg have begun to take and who can
	// still be brought back. The board acts on these by name -- the one decision a ward read cannot.
	int capRow = 0;
	{
		char copy[512];
		Q_strncpyz( copy, caps, sizeof( copy ) );
		const int capY = 88 + ( row > 0 ? row : 1 ) * 16 + 16;
		UI_DrawProportionalString( 44, capY, "RECOVERY WINDOW", UI_TINYFONT, colorTable[CT_LTORANGE] );
		for ( char *tok = strtok( copy, ";" ); tok; tok = strtok( NULL, ";" ), ++capRow )
		{
			char *bar = strchr( tok, '|' );
			if ( !bar ) continue;
			*bar = 0;
			UI_DrawProportionalString( 44, capY + 14 + capRow * 14, va( "%s  (taken %s%%)", tok, bar + 1 ),
				UI_TINYFONT, colorTable[CT_RED] );
		}
		if ( !capRow ) UI_DrawProportionalString( 44, capY + 14, "nobody in the window", UI_TINYFONT, colorTable[CT_LTPURPLE1] );
	}

	UI_DrawProportionalString( 44, 412, ui.Cvar_VariableValue( "lwh_ship_emh" ) > 0.5f ? "THE DOCTOR IS ON" : "THE DOCTOR IS OFF",
		UI_TINYFONT, colorTable[ui.Cvar_VariableValue( "lwh_ship_emh" ) > 0.5f ? CT_LTBLUE2 : CT_LTPURPLE1] );
	UI_DrawProportionalString( 44, 426, "UP/DOWN the ward   B surgical field   E the Doctor   R recover a captive   T triage order (command's)   ESC leave",
		UI_TINYFONT, colorTable[CT_LTPURPLE1] );
}

sfxHandle_t TriageKey( int key )
{
	if ( TriageAct( key ) ) return menu_null_sound;
	return Menu_DefaultKey( &triage.menu, key );
}

// ---- the log, as a browsable artifact (the log gap) -------------------------------------------
//
// The ship's record, newest first, from lwh_ship_log. This is the ready-room terminal and the
// captain's log's raw feed; the summary itself is written at the command console.

struct {
	menuframework_s menu;
	int scroll;
} logscreen;

void LogDraw( void )
{
	char raw[2048];
	ui.Cvar_VariableStringBuffer( "lwh_ship_log", raw, sizeof( raw ) );
	UI_FillRect( 0, 0, 640, 480, colorTable[CT_BLACK] );
	UI_FillRect( 20, 16, 600, 22, colorTable[CT_LTGOLD1] );
	UI_FillRect( 20, 42, 14, 396, colorTable[CT_DKPURPLE1] );
	UI_FillRect( 20, 442, 600, 10, colorTable[CT_DKPURPLE1] );
	UI_DrawProportionalString( 44, 19, "SHIP'S LOG  -  READY ROOM TERMINAL", UI_SMALLFONT, colorTable[CT_BLACK] );
	UI_DrawProportionalString( 44, 48, "WHEN", UI_TINYFONT, colorTable[CT_LTORANGE] );
	UI_DrawProportionalString( 150, 48, "WHO", UI_TINYFONT, colorTable[CT_LTORANGE] );
	UI_DrawProportionalString( 290, 48, "SCOPE", UI_TINYFONT, colorTable[CT_LTORANGE] );
	UI_DrawProportionalString( 360, 48, "WHAT", UI_TINYFONT, colorTable[CT_LTORANGE] );

	// Walk the entries to the scroll offset, then draw the window.
	char *tok = strtok( raw, ";" );
	for ( int i = 0; tok && i < logscreen.scroll; ++i ) tok = strtok( NULL, ";" );
	int row = 0;
	for ( ; tok && row < 22; tok = strtok( NULL, ";" ), ++row )
	{
		char *when = tok;
		char *who = strchr( when, '|' ); if ( !who ) continue; *who++ = 0;
		char *scope = strchr( who, '|' ); if ( !scope ) continue; *scope++ = 0;
		char *what = strchr( scope, '|' ); if ( !what ) continue; *what++ = 0;
		const int y = 66 + row * 17;
		UI_DrawProportionalString( 44, y, when, UI_TINYFONT, colorTable[CT_LTPURPLE1] );
		UI_DrawProportionalString( 150, y, who, UI_TINYFONT, colorTable[CT_LTBLUE2] );
		UI_DrawProportionalString( 290, y, scope, UI_TINYFONT, colorTable[CT_LTGOLD1] );
		UI_DrawProportionalString( 360, y, what, UI_TINYFONT, colorTable[CT_WHITE] );
	}
	if ( !row ) UI_DrawProportionalString( 44, 66, "THE LOG IS EMPTY", UI_SMALLFONT, colorTable[CT_LTBLUE2] );
	UI_DrawProportionalString( 44, 426, "UP/DOWN scroll   the captain's log is written at the command console   ESC leave",
		UI_TINYFONT, colorTable[CT_LTPURPLE1] );
}

sfxHandle_t LogKey( int key )
{
	if ( key == K_UPARROW ) { if ( logscreen.scroll > 0 ) --logscreen.scroll; return menu_null_sound; }
	if ( key == K_DOWNARROW ) { ++logscreen.scroll; return menu_null_sound; }
	if ( key == 'p' || key == 'P' ) { ui.Cmd_ExecuteText( EXEC_APPEND, "ui_lwh_personal\n" ); return menu_null_sound; }
	return Menu_DefaultKey( &logscreen.menu, key );
}

// ---- the personal log (docs/the-record-and-the-log.md, Task B) ----------------------------------
//
// The private half of the record, and the screen that was the one outright absence in the set: the
// store exists (WritePersonalLog), the rule is implemented and tested, and there was nowhere to read
// it. It is the place the truth goes when it cannot go in the report. What it shows is what the
// model published for the player, and the model returns one person's entries and nobody else's
// (PersonalLog), so the privacy is in the store, not a claim this screen makes. Reached from the
// ship's log terminal (P), where a console reaches the others.

struct {
	menuframework_s menu;
	int scroll;
} personal;

void PersonalDraw( void )
{
	char raw[4096], who[64];
	ui.Cvar_VariableStringBuffer( "lwh_ship_personal", raw, sizeof( raw ) );
	ui.Cvar_VariableStringBuffer( "lwh_ship_personal_who", who, sizeof( who ) );
	UI_FillRect( 0, 0, 640, 480, colorTable[CT_BLACK] );
	UI_FillRect( 20, 16, 600, 22, colorTable[CT_LTBLUE2] );
	UI_FillRect( 20, 42, 14, 396, colorTable[CT_DKPURPLE1] );
	UI_FillRect( 20, 442, 600, 10, colorTable[CT_DKPURPLE1] );
	UI_DrawProportionalString( 44, 19, "PERSONAL LOG  -  PRIVATE", UI_SMALLFONT, colorTable[CT_BLACK] );
	UI_DrawProportionalString( 44, 48, who[0] ? va( "%s's private log. Nobody else reads it.", who ) : "No character: nobody's private log is open.",
		UI_TINYFONT, colorTable[CT_LTGOLD1] );
	UI_DrawProportionalString( 44, 62, "WHEN", UI_TINYFONT, colorTable[CT_LTORANGE] );
	UI_DrawProportionalString( 150, 62, "WHAT", UI_TINYFONT, colorTable[CT_LTORANGE] );

	char *tok = strtok( raw, ";" );
	for ( int i = 0; tok && i < personal.scroll; ++i ) tok = strtok( NULL, ";" );
	int row = 0;
	for ( ; tok && row < 21; tok = strtok( NULL, ";" ), ++row )
	{
		char *when = tok;
		char *what = strchr( when, '|' );
		if ( !what ) continue;
		*what++ = 0;
		const int y = 80 + row * 17;
		UI_DrawProportionalString( 44, y, when, UI_TINYFONT, colorTable[CT_LTPURPLE1] );
		UI_DrawProportionalString( 150, y, what, UI_TINYFONT, colorTable[CT_WHITE] );
	}
	if ( !row ) UI_DrawProportionalString( 44, 80, "The personal log is empty. The official log is L.", UI_SMALLFONT, colorTable[CT_LTBLUE2] );
	UI_DrawProportionalString( 44, 426, "UP/DOWN scroll   L the ship's log (published)   only its owner reads this   ESC leave",
		UI_TINYFONT, colorTable[CT_LTPURPLE1] );
}

sfxHandle_t PersonalKey( int key )
{
	if ( key == K_UPARROW ) { if ( personal.scroll > 0 ) --personal.scroll; return menu_null_sound; }
	if ( key == K_DOWNARROW ) { ++personal.scroll; return menu_null_sound; }
	if ( key == 'l' || key == 'L' ) { ui.Cmd_ExecuteText( EXEC_APPEND, "ui_lwh_log\n" ); return menu_null_sound; }
	return Menu_DefaultKey( &personal.menu, key );
}

// ---- the survey screen (row 22: the tricorder / away-kit readout made a decision) --------------
//
// Operations' console showed the kit line and nothing to do with it. The instruments are the ship's
// (lwh_ship_survey lists what a tricorder can be spent on here: the site the ship is at, and the
// ship's own compartments). The tricorder charge is shared, so *what to scan* is the decision, and a
// weak charge reads wrong -- the reading comes back in lwh_ship_survey_reading and the tricorder's
// own honesty is the consequence. One content type, Operations' (the sensors), as the ruling requires.

struct SurveyRow { char type[8]; char name[48]; char state[24]; };

struct {
	menuframework_s menu;
	int cursor;
} survey;

int ReadSurvey( SurveyRow *out, int max )
{
	char buf[2048];
	ui.Cvar_VariableStringBuffer( "lwh_ship_survey", buf, sizeof( buf ) );
	int n = 0;
	for ( char *tok = strtok( buf, ";" ); tok && n < max; tok = strtok( NULL, ";" ) )
	{
		char *bar1 = strchr( tok, '|' ); if ( !bar1 ) continue; *bar1++ = 0;
		char *bar2 = strchr( bar1, '|' ); if ( !bar2 ) continue; *bar2++ = 0;
		Q_strncpyz( out[n].type, tok, sizeof( out[n].type ) );
		Q_strncpyz( out[n].name, bar1, sizeof( out[n].name ) );
		Q_strncpyz( out[n].state, bar2, sizeof( out[n].state ) );
		++n;
	}
	return n;
}

bool SurveyAct( int key )
{
	SurveyRow rows[20];
	const int n = ReadSurvey( rows, 20 );
	switch ( key )
	{
	case K_UPARROW: if ( n ) survey.cursor = ( survey.cursor + n - 1 ) % n; return true;
	case K_DOWNARROW: if ( n ) survey.cursor = ( survey.cursor + 1 ) % n; return true;
	case K_ENTER: case K_KP_ENTER:
		if ( n )
		{//the site is the beacon the ship is at; a deck is scanned in place. Both come out of the
		 //Operations console, so the ship holds them to that station's authority (the two axes).
			if ( !Q_stricmp( rows[survey.cursor].type, "SITE" ) )
				ui.Cmd_ExecuteText( EXEC_APPEND, "ship as 2 scan\n" );
			else
				ui.Cmd_ExecuteText( EXEC_APPEND, va( "ship as 2 scancomp %d\n", survey.cursor ) );
		}
		return true;
	}
	return false;
}

void SurveyDraw( void )
{
	char line[512];
	SurveyRow rows[20];
	const int n = ReadSurvey( rows, 20 );
	if ( survey.cursor >= n ) survey.cursor = n ? n - 1 : 0;
	UI_FillRect( 0, 0, 640, 480, colorTable[CT_BLACK] );
	UI_FillRect( 20, 16, 600, 22, colorTable[CT_LTBLUE2] );
	UI_FillRect( 20, 42, 14, 396, colorTable[CT_DKPURPLE1] );
	UI_FillRect( 20, 442, 600, 10, colorTable[CT_DKPURPLE1] );
	UI_DrawProportionalString( 44, 19, "OPERATIONS  -  SURVEY", UI_SMALLFONT, colorTable[CT_BLACK] );
	ui.Cvar_VariableStringBuffer( "lwh_ship_header", line, sizeof( line ) );
	UI_DrawProportionalString( 44, 46, line, UI_SMALLFONT, colorTable[CT_LTGOLD1] );
	ui.Cvar_VariableStringBuffer( "lwh_ship_kit", line, sizeof( line ) );
	UI_DrawProportionalString( 44, 64, line, UI_TINYFONT, colorTable[CT_LTPURPLE1] );
	UI_DrawProportionalString( 44, 84, "WHAT A SCAN MAY BE SPENT ON  (the charge is shared)", UI_TINYFONT, colorTable[CT_LTORANGE] );
	for ( int i = 0; i < n && i < 17; ++i )
	{
		const int y = 100 + i * 16;
		const bool selected = i == survey.cursor;
		if ( selected ) UI_FillRect( 38, y - 1, 582, 14, colorTable[CT_DKPURPLE2] );
		UI_DrawProportionalString( 44, y, rows[i].name, UI_TINYFONT,
			colorTable[selected ? CT_WHITE : CT_LTGOLD1] );
		const bool trouble = Q_stricmp( rows[i].state, "nominal" ) && Q_stricmp( rows[i].state, "already charted" )
			&& Q_stricmp( rows[i].state, "unscanned" );
		UI_DrawProportionalString( 460, y, rows[i].state, UI_TINYFONT,
			colorTable[trouble ? CT_RED : CT_LTBLUE2] );
	}
	if ( !n ) UI_DrawProportionalString( 44, 100, "NO SURVEY DATA  -  the ship simulation is not running", UI_SMALLFONT, colorTable[CT_RED] );

	ui.Cvar_VariableStringBuffer( "lwh_ship_survey_reading", line, sizeof( line ) );
	UI_DrawProportionalString( 44, 396, line[0] ? va( "LAST READING: %s", line ) : "LAST READING: none",
		UI_TINYFONT, colorTable[CT_LTGOLD1] );

	ui.Cvar_VariableStringBuffer( "lwh_ship_refusal", line, sizeof( line ) );
	if ( line[0] ) UI_DrawProportionalString( 44, 412, va( "REFUSED: %s", line ), UI_TINYFONT, colorTable[CT_RED] );

	UI_DrawProportionalString( 44, 426, "UP/DOWN choose   ENTER scan   a weak charge reads wrong   ESC leave",
		UI_TINYFONT, colorTable[CT_LTPURPLE1] );
}

sfxHandle_t SurveyKey( int key )
{
	if ( SurveyAct( key ) ) return menu_null_sound;
	return Menu_DefaultKey( &survey.menu, key );
}

} // namespace

qboolean LWH_UI_ConsoleCommand( const char *cmd )
{
	if ( LWH_UI_StartScreens( cmd ) ) return qtrue;
	if ( LWH_UI_CommandScreens( cmd ) ) return qtrue;
	if ( !Q_stricmp( cmd, "ui_lwh_triage" ) )
	{
		memset( &triage.menu, 0, sizeof( triage.menu ) );
		triage.menu.draw = TriageDraw;
		triage.menu.key = TriageKey;
		triage.menu.fullscreen = qfalse;
		triage.menu.wrapAround = qtrue;
		triage.menu.initialized = qtrue;
		UI_PushMenu( &triage.menu );
		ui.Cvar_Set( "ui_liveMenu", "1" );
		return qtrue;
	}
	if ( !Q_stricmp( cmd, "ui_lwh_log" ) )
	{
		logscreen.scroll = 0;
		memset( &logscreen.menu, 0, sizeof( logscreen.menu ) );
		logscreen.menu.draw = LogDraw;
		logscreen.menu.key = LogKey;
		logscreen.menu.fullscreen = qfalse;
		logscreen.menu.wrapAround = qtrue;
		logscreen.menu.initialized = qtrue;
		UI_PushMenu( &logscreen.menu );
		ui.Cvar_Set( "ui_liveMenu", "1" );
		return qtrue;
	}
	if ( !Q_stricmp( cmd, "ui_lwh_personal" ) )
	{//the private half of the log (docs/the-record-and-the-log.md, Task B). Only the owner's entries
	 //are ever published for it, so the screen reads what the model returns and nothing else.
		personal.scroll = 0;
		memset( &personal.menu, 0, sizeof( personal.menu ) );
		personal.menu.draw = PersonalDraw;
		personal.menu.key = PersonalKey;
		personal.menu.fullscreen = qfalse;
		personal.menu.wrapAround = qtrue;
		personal.menu.initialized = qtrue;
		UI_PushMenu( &personal.menu );
		ui.Cvar_Set( "ui_liveMenu", "1" );
		return qtrue;
	}
	if ( !Q_stricmp( cmd, "lwh_triage_key" ) )
	{//what a test presses is what a hand presses: drive the triage board by name
		char arg[32];
		ui.Argv( 1, arg, sizeof( arg ) );
		TriageAct( KeyByName( arg ) );
		return qtrue;
	}
	if ( !Q_stricmp( cmd, "ui_lwh_survey" ) )
	{//the tricorder survey (row 22): what a charge is spent on, from Operations
		survey.cursor = 0;
		memset( &survey.menu, 0, sizeof( survey.menu ) );
		survey.menu.draw = SurveyDraw;
		survey.menu.key = SurveyKey;
		survey.menu.fullscreen = qfalse;
		survey.menu.wrapAround = qtrue;
		survey.menu.initialized = qtrue;
		UI_PushMenu( &survey.menu );
		ui.Cvar_Set( "ui_liveMenu", "1" );
		return qtrue;
	}
	if ( !Q_stricmp( cmd, "lwh_survey_key" ) )
	{//drive the survey board by name, as a test's hand
		char arg[32];
		ui.Argv( 1, arg, sizeof( arg ) );
		SurveyAct( KeyByName( arg ) );
		return qtrue;
	}
	if ( !Q_stricmp( cmd, "lwh_ui_turbolift" ) ) { ReportTurboliftDecks(); return qtrue; }
	if ( !Q_stricmp( cmd, "ui_lwh_engineering" ) )
	{
		Open( 0 );
		return qtrue;
	}
	if ( !Q_stricmp( cmd, "ui_lwh_station" ) )
	{
		char arg[32], focus[32];
		ui.Argv( 1, arg, sizeof( arg ) );
		const int n = atoi( arg );
		ui.Argv( 2, focus, sizeof( focus ) );  // an optional system name: the cursor opens on it
		Open( n >= 0 && n <= 4 ? n : 0, focus[0] ? focus : NULL );
		return qtrue;
	}
	// The panels already in the ship call the retail game's station screens by these commands. With
	// the ship simulation running, each opens the working console for that station instead; without
	// it, the retail screen opens as it always did.
	if ( ui.Cvar_VariableValue( "g_ship" ) )
	{
		static const struct { const char *retail; int station; const char *focus; } PANELS[] = {
			{ "ui_engineeringstatus", 0, "warp drive" }, { "ui_tactical", 1, "shields" },
			{ "ui_ops", 2, NULL }, { "ui_navigation", 3, "navigational deflector" },
		};
		for ( size_t i = 0; i < sizeof( PANELS ) / sizeof( PANELS[0] ); ++i )
		{
			if ( !Q_stricmp( cmd, PANELS[i].retail ) )
			{
				Open( PANELS[i].station, PANELS[i].focus );
				return qtrue;
			}
		}
		// The ship's own panels fire "genericmenu <screen>". A station's panel opens the working
		// console instead of the retail screen; the turbolift and the logs are not stations and fall
		// through to the retail handler and keep their own menus.
		if ( !Q_stricmp( cmd, "genericmenu" ) )
		{
			char id[32];
			ui.Argv( 1, id, sizeof( id ) );
			// Each panel opens the console its location is for, focused on that system: the transporter
			// room's console on the transporters, astrometrics on the sensors, environmental control on
			// life support (docs/access-and-authority.md, owner decision 2026-10-07).
			static const struct { const char *panel; int station; const char *focus; } STATION_PANELS[] = {
				{ "tactical", 1, "shields" }, { "engineeringStatus", 0, "warp drive" },
				{ "navigation", 3, "navigational deflector" },
				{ "transporter", 2, "transporters" }, { "astrometrics", 2, "sensors" }, // both worked from Operations
				{ "environmental", 2, "life support" }, { "lifesupport", 2, "life support" },
				{ "sickbay", 4, "sickbay" }, { "medical", 4, "sickbay" },
			};
			static const char *const NAMES[] = { "MAIN ENGINEERING", "TACTICAL", "OPERATIONS", "CONN", "SICKBAY" };
			for ( size_t i = 0; i < sizeof( STATION_PANELS ) / sizeof( STATION_PANELS[0] ); ++i )
			{
				if ( !Q_stricmp( id, STATION_PANELS[i].panel ) )
				{
					ui.Printf( "LWH: the %s panel opens the %s console\n", id, NAMES[STATION_PANELS[i].station] );
					Open( STATION_PANELS[i].station, STATION_PANELS[i].focus );
					return qtrue;
				}
			}
			// The replicator and the mess hall: worked from Operations (the galley is an Ops service).
			if ( !Q_stricmpn( id, "replicat", 8 ) || !Q_stricmpn( id, "mess", 4 ) )
			{
				ui.Printf( "LWH: the %s panel opens the OPERATIONS console\n", id );
				Open( 2, "replicators" );
				return qtrue;
			}
			// The panels that are not stations: the log terminal, the ready room, the personnel padd.
			// The map's own interface names decide which; the ship's screens open in their place.
			if ( !Q_stricmpn( id, "log", 3 ) || !Q_stricmpn( id, "padd", 4 ) )
			{
				ui.Printf( "LWH: the %s terminal opens the ship's log\n", id );
				ui.Cmd_ExecuteText( EXEC_APPEND, "ui_lwh_log\n" );
				return qtrue;
			}
			if ( !Q_stricmpn( id, "ready", 5 ) || !Q_stricmpn( id, "command", 7 ) )
			{
				ui.Printf( "LWH: the %s panel opens the command console\n", id );
				ui.Cmd_ExecuteText( EXEC_APPEND, "ui_lwh_command\n" );
				return qtrue;
			}
			if ( !Q_stricmpn( id, "personnel", 9 ) || !Q_stricmpn( id, "crew", 4 ) )
			{
				ui.Printf( "LWH: the %s panel opens the personnel screen\n", id );
				ui.Cmd_ExecuteText( EXEC_APPEND, "ui_lwh_character\n" );
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
