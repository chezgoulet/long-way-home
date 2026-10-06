// ui_lwh_command.cpp -- the command console (standing orders) and character creation (gate S10).
//
// Like the station consoles these decide nothing: they send the ship her commands and show what
// she says. Whether the operator may give an order is the ship's to judge (ship_core:
// PlayerMayCommand), and a refusal comes back from her.

#include "ui_local.h"

#include "lwh_ui.h"

#include <cstdlib>
#include <cstring>

namespace {

// ---- the command console ----------------------------------------------------------------------

struct {
	menuframework_s menu;
	int system;     // cursor in the list of systems, for "see to this first"
	int deck;       // 1..15, for the guard and the evacuation
} command;

const int DECKS = 15;

int SystemCount( void )
{
	char buf[256];
	int n = 0;
	for ( ; n < 32; ++n )
	{
		ui.Cvar_VariableStringBuffer( va( "lwh_ship_sys%d", n ), buf, sizeof( buf ) );
		if ( !strchr( buf, '|' ) ) break;
	}
	return n;
}

void SystemName( int i, char *out, int size )
{
	char buf[256];
	ui.Cvar_VariableStringBuffer( va( "lwh_ship_sys%d", i ), buf, sizeof( buf ) );
	char *bar = strchr( buf, '|' );
	if ( bar ) *bar = 0;
	Q_strncpyz( out, buf, size );
}

void Order( const char *text )
{
	ui.Cmd_ExecuteText( EXEC_APPEND, va( "ship order %s\n", text ) );
}

void CommandDraw( void )
{
	char line[256];
	UI_FillRect( 0, 0, 640, 480, colorTable[CT_BLACK] );
	UI_FillRect( 20, 16, 600, 22, colorTable[CT_LTGOLD1] );
	UI_FillRect( 20, 42, 14, 396, colorTable[CT_DKPURPLE1] );
	UI_FillRect( 20, 442, 600, 10, colorTable[CT_DKPURPLE1] );
	UI_DrawProportionalString( 44, 19, "COMMAND  -  STANDING ORDERS", UI_SMALLFONT, colorTable[CT_BLACK] );
	ui.Cvar_VariableStringBuffer( "lwh_ship_header", line, sizeof( line ) );
	UI_DrawProportionalString( 44, 46, line, UI_SMALLFONT, colorTable[CT_LTGOLD1] );
	ui.Cvar_VariableStringBuffer( "lwh_ship_stores", line, sizeof( line ) );
	UI_DrawProportionalString( 44, 62, line, UI_TINYFONT, colorTable[CT_LTPURPLE1] );
	ui.Cvar_VariableStringBuffer( "lwh_ship_aboard", line, sizeof( line ) );
	if ( line[0] ) UI_DrawProportionalString( 44, 76, line, UI_TINYFONT, colorTable[CT_RED] );

	// The navigation counter: everyone's fact (docs/navigation-counter.md). Here in the ready room it
	// sits above the forecasts, which are command's alone.
	ui.Cvar_VariableStringBuffer( "lwh_ship_nav", line, sizeof( line ) );
	if ( line[0] ) UI_DrawProportionalString( 44, 88, line, UI_TINYFONT, colorTable[CT_LTBLUE1] );

	UI_DrawProportionalString( 44, 100, "ORDERS IN FORCE", UI_TINYFONT, colorTable[CT_LTORANGE] );
	ui.Cvar_VariableStringBuffer( "lwh_ship_orders", line, sizeof( line ) );
	UI_DrawProportionalString( 44, 114, line[0] ? line : "NONE", UI_SMALLFONT, colorTable[CT_WHITE] );
	ui.Cvar_VariableStringBuffer( "lwh_ship_order_refused", line, sizeof( line ) );
	if ( line[0] ) UI_DrawProportionalString( 44, 134, va( "REFUSED: %s", line ), UI_TINYFONT, colorTable[CT_RED] );

	const int n = SystemCount();
	if ( command.system >= n ) command.system = n ? n - 1 : 0;
	UI_DrawProportionalString( 44, 160, "DAMAGE CONTROL IS TO SEE FIRST TO", UI_TINYFONT, colorTable[CT_LTORANGE] );
	for ( int i = 0; i < n; ++i )
	{
		char name[32];
		SystemName( i, name, sizeof( name ) );
		const int x = 44 + ( i / 9 ) * 200, y = 174 + ( i % 9 ) * 14;
		if ( i == command.system ) UI_FillRect( x - 4, y - 1, 190, 13, colorTable[CT_DKPURPLE2] );
		UI_DrawProportionalString( x, y, name, UI_TINYFONT, colorTable[i == command.system ? CT_WHITE : CT_LTGOLD1] );
	}
	UI_DrawProportionalString( 460, 160, "DECK", UI_TINYFONT, colorTable[CT_LTORANGE] );
	UI_DrawProportionalString( 460, 176, va( "%d", command.deck ), UI_BIGFONT, colorTable[CT_WHITE] );

	// What the ship has given up (docs/damage-and-budgets.md): the abandonment list, the one place
	// the player's own hand is in the record. Newest first, from lwh_ship_losses; W writes off the
	// deck the cursor is on.
	UI_DrawProportionalString( 460, 210, "GIVEN UP", UI_TINYFONT, colorTable[CT_LTORANGE] );
	{
		char losses[1024];
		ui.Cvar_VariableStringBuffer( "lwh_ship_losses", losses, sizeof( losses ) );
		int rows = 0;
		for ( char *tok = strtok( losses, ";" ); tok && rows < 12; tok = strtok( NULL, ";" ), ++rows )
		{
			char *when = tok;
			char *kind = strchr( when, '|' ); if ( !kind ) break; *kind++ = 0;
			char *what = strchr( kind, '|' ); if ( !what ) break; *what++ = 0;
			char *who = strchr( what, '|' ); if ( !who ) break; *who++ = 0;
			const int y = 224 + rows * 14;
			UI_DrawProportionalString( 460, y, va( "%s  %s", when, kind ), UI_TINYFONT, colorTable[CT_LTGOLD1] );
			UI_DrawProportionalString( 460, y + 7, va( "%s - %s", what, who ), UI_TINYFONT, colorTable[CT_LTBLUE1] );
		}
		if ( !rows ) UI_DrawProportionalString( 460, 224, "NOTHING YET", UI_TINYFONT, colorTable[CT_LTPURPLE1] );
	}

	// The player's character and their career: command can confirm a field promotion here.
	ui.Cvar_VariableStringBuffer( "lwh_ship_player", line, sizeof( line ) );
	UI_DrawProportionalString( 44, 320, va( "YOUR CHARACTER: %s", line[0] ? line : "NONE" ), UI_SMALLFONT, colorTable[CT_WHITE] );
	ui.Cvar_VariableStringBuffer( "lwh_ship_player_next", line, sizeof( line ) );
	if ( line[0] ) UI_DrawProportionalString( 44, 336, va( "P  field promotion to %s", line ), UI_TINYFONT, colorTable[CT_LTBLUE1] );
	ui.Cvar_VariableStringBuffer( "lwh_ship_promote", line, sizeof( line ) );
	if ( line[0] ) UI_DrawProportionalString( 44, 350, line, UI_TINYFONT, strncmp( line, "REFUSED", 7 ) == 0 ? colorTable[CT_RED] : colorTable[CT_LTGOLD1] );

	// Command sees the forecasts: the estimate under each available course (docs/navigation-counter.md).
	ui.Cvar_VariableStringBuffer( "lwh_ship_forecast", line, sizeof( line ) );
	if ( line[0] )
	{
		UI_DrawProportionalString( 44, 362, "FORECASTS - the estimate under each course", UI_TINYFONT, colorTable[CT_LTORANGE] );
		int row = 0;
		for ( char *tok = strtok( line, ";" ); tok && row < 3; tok = strtok( NULL, ";" ), ++row )
			UI_DrawProportionalString( 44, 376 + row * 12, tok, UI_TINYFONT, colorTable[CT_LTBLUE1] );
	}

	UI_DrawProportionalString( 44, 412, "UP/DOWN system   LEFT/RIGHT deck   R repair that system first   G guard to that deck   V evacuate that deck",
		UI_TINYFONT, colorTable[CT_LTPURPLE1] );
	UI_DrawProportionalString( 44, 426, "T sickbay triage worst/rank first   W write off that deck   C clear all orders   ESC leave", UI_TINYFONT, colorTable[CT_LTPURPLE1] );
}

bool CommandAct( int key )
{
	const int n = SystemCount();
	char name[32];
	switch ( key )
	{
	case K_UPARROW: if ( n ) command.system = ( command.system + n - 1 ) % n; return true;
	case K_DOWNARROW: if ( n ) command.system = ( command.system + 1 ) % n; return true;
	case K_LEFTARROW: command.deck = command.deck > 1 ? command.deck - 1 : DECKS; return true;
	case K_RIGHTARROW: command.deck = command.deck < DECKS ? command.deck + 1 : 1; return true;
	case 'r': case 'R':
		if ( !n ) return true;
		SystemName( command.system, name, sizeof( name ) );
		Order( va( "repair \"%s\"", name ) );
		return true;
	case 'g': case 'G': Order( va( "security %d", command.deck ) ); return true;
	case 'v': case 'V': Order( va( "evacuate %d", command.deck ) ); return true;
	case 'w': case 'W':
		ui.Cmd_ExecuteText( EXEC_APPEND, va( "ship writeoff %d\n", command.deck ) );
		return true;
	case 't': case 'T': Order( ui.Cvar_VariableValue( "lwh_ship_triage" ) > 0.5f ? "triage worst" : "triage rank" ); return true;
	case 'c': case 'C': Order( "repair none" ); Order( "security 0" ); Order( "evacuate 0" ); return true;
	case 'p': case 'P':
	{
		char idx[16];
		ui.Cvar_VariableStringBuffer( "lwh_ship_player_index", idx, sizeof( idx ) );
		if ( idx[0] ) ui.Cmd_ExecuteText( EXEC_APPEND, va( "ship promote %s\n", idx ) );
		return true;
	}
	}
	return false;
}

sfxHandle_t CommandKey( int key )
{
	if ( CommandAct( key ) ) return menu_null_sound;
	return Menu_DefaultKey( &command.menu, key );
}

// ---- character creation -----------------------------------------------------------------------

const char *const DEPARTMENTS[] = { "COMMAND", "ENGINEERING", "SECURITY", "SCIENCES", "MEDICAL" };
const char *const RANKS[] = { "CREWMAN", "ENSIGN", "LIEUTENANT J.G.", "LIEUTENANT", "LIEUTENANT COMMANDER" };
// Surnames to choose from until there is somewhere to type one. Invented; none is a canon character.
const char *const NAMES[] = { "Reyes", "Okoro", "Lindqvist", "Tanaka", "Ferreira", "Mbeki", "Novak", "Castellanos", "Rahimi", "Whitlock" };
const int NUM_NAMES = sizeof( NAMES ) / sizeof( NAMES[0] );

struct {
	menuframework_s menu;
	int dept, rank, name;
} creation;

void CreationDraw( void )
{
	char line[128];
	UI_FillRect( 0, 0, 640, 480, colorTable[CT_BLACK] );
	UI_FillRect( 20, 16, 600, 22, colorTable[CT_LTBLUE1] );
	UI_FillRect( 20, 42, 14, 396, colorTable[CT_DKPURPLE1] );
	UI_FillRect( 20, 442, 600, 10, colorTable[CT_DKPURPLE1] );
	UI_DrawProportionalString( 44, 19, "PERSONNEL  -  NEW CREW MEMBER", UI_SMALLFONT, colorTable[CT_BLACK] );
	UI_DrawProportionalString( 44, 70, "You take the place of a member of the crew: the ship's complement does not grow.", UI_TINYFONT, colorTable[CT_LTPURPLE1] );

	UI_DrawProportionalString( 44, 110, "NAME", UI_TINYFONT, colorTable[CT_LTORANGE] );
	UI_DrawProportionalString( 200, 106, NAMES[creation.name], UI_BIGFONT, colorTable[CT_WHITE] );
	UI_DrawProportionalString( 44, 160, "DEPARTMENT", UI_TINYFONT, colorTable[CT_LTORANGE] );
	UI_DrawProportionalString( 200, 156, DEPARTMENTS[creation.dept], UI_BIGFONT, colorTable[CT_LTGOLD1] );
	UI_DrawProportionalString( 44, 210, "RANK", UI_TINYFONT, colorTable[CT_LTORANGE] );
	UI_DrawProportionalString( 200, 206, RANKS[creation.rank], UI_BIGFONT, colorTable[CT_LTGOLD1] );

	static const char *const WORKS[] = { "Operations and the Conn", "Main Engineering", "Tactical", "Operations and the Conn", "Sickbay" };
	UI_DrawProportionalString( 44, 270, va( "Station: %s%s", creation.rank >= 4 ? "any" : WORKS[creation.dept],
		creation.rank >= 3 && ( creation.dept == 1 || creation.dept == 2 || creation.rank >= 4 ) ? ", and may call the alert there" : "" ),
		UI_SMALLFONT, colorTable[CT_LTBLUE1] );

	ui.Cvar_VariableStringBuffer( "lwh_ship_player", line, sizeof( line ) );
	if ( line[0] ) UI_DrawProportionalString( 44, 320, va( "SERVING AS: %s", line ), UI_SMALLFONT, colorTable[CT_WHITE] );

	UI_DrawProportionalString( 44, 426, "N next name   LEFT/RIGHT department   UP/DOWN rank   ENTER report for duty   ESC leave",
		UI_TINYFONT, colorTable[CT_LTPURPLE1] );
}

bool CreationAct( int key )
{
	switch ( key )
	{
	case 'n': case 'N': creation.name = ( creation.name + 1 ) % NUM_NAMES; return true;
	case K_LEFTARROW: creation.dept = ( creation.dept + 4 ) % 5; return true;
	case K_RIGHTARROW: creation.dept = ( creation.dept + 1 ) % 5; return true;
	case K_UPARROW: if ( creation.rank < 4 ) ++creation.rank; return true;
	case K_DOWNARROW: if ( creation.rank > 0 ) --creation.rank; return true;
	case K_ENTER: case K_KP_ENTER:
		ui.Cmd_ExecuteText( EXEC_APPEND, va( "ship character %s %d %d\n", NAMES[creation.name], creation.dept, creation.rank ) );
		return true;
	}
	return false;
}

sfxHandle_t CreationKey( int key )
{
	if ( CreationAct( key ) ) return menu_null_sound;
	return Menu_DefaultKey( &creation.menu, key );
}

void Push( menuframework_s *menu, void ( *draw )( void ), sfxHandle_t ( *key )( int ) )
{
	memset( menu, 0, sizeof( *menu ) );
	menu->draw = draw;
	menu->key = key;
	menu->fullscreen = qfalse; //see ui_lwh_engineering.cpp: a fullscreen menu stops the game
	menu->wrapAround = qtrue;
	menu->initialized = qtrue;
	UI_PushMenu( menu );
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

// Called from LWH_UI_ConsoleCommand. `lwh_cmd_key` and `lwh_new_key` drive the two screens by name,
// as `lwh_eng_key` drives the station consoles: what a test presses is what a hand presses.
qboolean LWH_UI_CommandScreens( const char *cmd )
{
	char arg[32];
	if ( !Q_stricmp( cmd, "ui_lwh_command" ) )
	{
		if ( command.deck < 1 ) command.deck = 1;
		Push( &command.menu, CommandDraw, CommandKey );
		return qtrue;
	}
	if ( !Q_stricmp( cmd, "ui_lwh_character" ) )
	{
		Push( &creation.menu, CreationDraw, CreationKey );
		return qtrue;
	}
	if ( !Q_stricmp( cmd, "lwh_cmd_key" ) )
	{
		ui.Argv( 1, arg, sizeof( arg ) );
		CommandAct( KeyByName( arg ) );
		return qtrue;
	}
	if ( !Q_stricmp( cmd, "lwh_new_key" ) )
	{
		ui.Argv( 1, arg, sizeof( arg ) );
		CreationAct( KeyByName( arg ) );
		return qtrue;
	}
	return qfalse;
}
