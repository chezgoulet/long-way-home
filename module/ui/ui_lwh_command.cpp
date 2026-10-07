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
	if ( line[0] ) UI_DrawProportionalString( 44, 88, line, UI_TINYFONT, colorTable[CT_LTBLUE2] );

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
			UI_DrawProportionalString( 460, y + 7, va( "%s - %s", what, who ), UI_TINYFONT, colorTable[CT_LTBLUE2] );
		}
		if ( !rows ) UI_DrawProportionalString( 460, 224, "NOTHING YET", UI_TINYFONT, colorTable[CT_LTPURPLE1] );
	}

	// Access, in command's own room: the emergency override in progress, the lock-outs a senior
	// officer has imposed, and the grants for a shift (docs/access-and-authority.md, owner decision
	// 2026-10-07). The lock-out names both hands, so the console can say who did it to whom.
	{
		char ov[160], locks[512], grants[512];
		ui.Cvar_VariableStringBuffer( "lwh_ship_override", ov, sizeof( ov ) );
		ui.Cvar_VariableStringBuffer( "lwh_ship_lockouts", locks, sizeof( locks ) );
		ui.Cvar_VariableStringBuffer( "lwh_ship_delegations", grants, sizeof( grants ) );
		int y = 300;
		if ( ov[0] ) { UI_DrawProportionalString( 44, y, ov, UI_TINYFONT, colorTable[strstr( ov, "ACTIVE" ) ? CT_RED : CT_LTORANGE] ); y += 14; }
		if ( locks[0] ) { UI_DrawProportionalString( 44, y, va( "LOCKED OUT: %s", locks ), UI_TINYFONT, colorTable[CT_RED] ); y += 14; }
		if ( grants[0] ) { UI_DrawProportionalString( 44, y, va( "DELEGATED: %s", grants ), UI_TINYFONT, colorTable[CT_LTBLUE2] ); }
	}

	// The player's character and their career: command can confirm a field promotion here.
	ui.Cvar_VariableStringBuffer( "lwh_ship_player", line, sizeof( line ) );
	UI_DrawProportionalString( 44, 320, va( "YOUR CHARACTER: %s", line[0] ? line : "NONE" ), UI_SMALLFONT, colorTable[CT_WHITE] );
	ui.Cvar_VariableStringBuffer( "lwh_ship_player_next", line, sizeof( line ) );
	if ( line[0] ) UI_DrawProportionalString( 44, 336, va( "P  field promotion to %s", line ), UI_TINYFONT, colorTable[CT_LTBLUE2] );
	ui.Cvar_VariableStringBuffer( "lwh_ship_promote", line, sizeof( line ) );
	if ( line[0] ) UI_DrawProportionalString( 44, 350, line, UI_TINYFONT, strncmp( line, "REFUSED", 7 ) == 0 ? colorTable[CT_RED] : colorTable[CT_LTGOLD1] );

	// Command sees the forecasts: the estimate under each available course (docs/navigation-counter.md).
	ui.Cvar_VariableStringBuffer( "lwh_ship_forecast", line, sizeof( line ) );
	if ( line[0] )
	{
		UI_DrawProportionalString( 44, 362, "FORECASTS - the estimate under each course", UI_TINYFONT, colorTable[CT_LTORANGE] );
		int row = 0;
		for ( char *tok = strtok( line, ";" ); tok && row < 3; tok = strtok( NULL, ";" ), ++row )
			UI_DrawProportionalString( 44, 376 + row * 12, tok, UI_TINYFONT, colorTable[CT_LTBLUE2] );
	}

	UI_DrawProportionalString( 44, 412, "UP/DOWN system   LEFT/RIGHT deck   R see to it first   G guard that deck   V evacuate that deck   W write it off",
		UI_TINYFONT, colorTable[CT_LTPURPLE1] );
	UI_DrawProportionalString( 44, 426, "T triage order   O month report   J job queue   A chart   C clear orders   P promote   ESC leave", UI_TINYFONT, colorTable[CT_LTPURPLE1] );
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
	case 'o': case 'O': ui.Cmd_ExecuteText( EXEC_APPEND, "ui_lwh_report\n" ); return true;
	case 'j': case 'J': ui.Cmd_ExecuteText( EXEC_APPEND, "ui_lwh_jobs\n" ); return true;
	case 'a': case 'A': ui.Cmd_ExecuteText( EXEC_APPEND, "ui_lwh_chart\n" ); return true;
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
	UI_FillRect( 20, 16, 600, 22, colorTable[CT_LTBLUE2] );
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
		UI_SMALLFONT, colorTable[CT_LTBLUE2] );

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

// ---- the month report editor (row 20: the screen that was missing) ----------------------------
//
// The model is complete (DraftReport / StrikeReportLine / SoftenReportLine / SignReport) and there
// was no screen to edit it. The report is signed by the officer who commands, so writing it is
// command's alone: the screen sends `ship report ...`, and the ship refuses anyone who does not
// command, naming the reason -- the person axis of the two-lock model, not a clearance model of the
// screen's own. The decision is the lie and its direction: strike a line, soften a number, sign it to
// the crew (where a contradiction is read by those below and the toll is paid, docs/the-record-and-
// the-log.md) or file it upward, where nobody below reads it. The diff the record keeps is drawn, so
// the player always sees what they actually did.

struct {
	menuframework_s menu;
	int cursor;
} report;

void ReportDraw( void )
{
	char rows[2048], line[512];
	UI_FillRect( 0, 0, 640, 480, colorTable[CT_BLACK] );
	UI_FillRect( 20, 16, 600, 22, colorTable[CT_LTGOLD1] );
	UI_FillRect( 20, 42, 14, 396, colorTable[CT_DKPURPLE1] );
	UI_FillRect( 20, 442, 600, 10, colorTable[CT_DKPURPLE1] );
	UI_DrawProportionalString( 44, 19, "COMMAND  -  THE MONTH REPORT", UI_SMALLFONT, colorTable[CT_BLACK] );
	ui.Cvar_VariableStringBuffer( "lwh_ship_report_number", line, sizeof( line ) );
	const bool open = ui.Cvar_VariableValue( "lwh_ship_report_open" ) > 0.5f;
	UI_DrawProportionalString( 44, 48, open ? va( "ENTRY %s  -  DRAFT: the record keeps what you change", line )
		: "SIGNED AND PUBLISHED  (a fresh draft is written when you ask for the report again)",
		UI_TINYFONT, colorTable[CT_LTGOLD1] );
	UI_DrawProportionalString( 44, 68, "LINE  SCOPE   AS IT READS", UI_TINYFONT, colorTable[CT_LTORANGE] );

	ui.Cvar_VariableStringBuffer( "lwh_ship_report", rows, sizeof( rows ) );
	int n = 0;
	for ( char *tok = strtok( rows, "\x1f" ); tok; tok = strtok( NULL, "\x1f" ), ++n )
	{
		char *st = strchr( tok, '|' ); if ( !st ) break; *st++ = 0;
		char *sc = strchr( st, '|' ); if ( !sc ) break; *sc++ = 0;
		char *text = strchr( sc, '|' ); if ( !text ) break; *text++ = 0;
		const int y = 84 + n * 15;
		if ( n == report.cursor ) UI_FillRect( 38, y - 1, 582, 14, colorTable[CT_DKPURPLE2] );
		// status: ' ' drafted, 'S' struck, 'E' edited, '+' added -- the lie's own tags, in words
		const char s = st[0];
		const int col = s == 'S' ? CT_RED : s == '+' ? CT_LTORANGE : s == 'E' ? CT_LTGOLD1 : CT_WHITE;
		UI_DrawProportionalString( 44, y, va( "%2d %c", n, s == ' ' ? '.' : s ), UI_TINYFONT, colorTable[CT_LTBLUE2] );
		UI_DrawProportionalString( 90, y, va( "%s]", sc ), UI_TINYFONT, colorTable[CT_LTBLUE2] );
		UI_DrawProportionalString( 190, y, text, UI_TINYFONT, colorTable[col] );
	}
	if ( !n ) UI_DrawProportionalString( 44, 84, "THE REPORT IS EMPTY", UI_SMALLFONT, colorTable[CT_LTBLUE2] );
	if ( n && report.cursor >= n ) report.cursor = n - 1;

	UI_DrawProportionalString( 44, 350, "WHAT YOU CHANGED  (the record keeps it)", UI_TINYFONT, colorTable[CT_LTORANGE] );
	ui.Cvar_VariableStringBuffer( "lwh_ship_report_diff", line, sizeof( line ) );
	if ( !line[0] ) UI_DrawProportionalString( 44, 364, "nothing yet - the report is as it was drafted", UI_TINYFONT, colorTable[CT_LTBLUE2] );
	else
	{//the diff is drawn one change per line and clipped to the frame; the record holds all of it
		int row = 0;
		for ( char *tok = strtok( line, "\x1f" ); tok && row < 3; tok = strtok( NULL, "\x1f" ), ++row )
			UI_DrawProportionalString( 44, 364 + row * 13, tok, UI_TINYFONT, colorTable[tok[0] == '+' ? CT_LTGOLD1 : CT_LTBLUE2] );
	}

	ui.Cvar_VariableStringBuffer( "lwh_ship_report_refused", line, sizeof( line ) );
	if ( line[0] ) UI_DrawProportionalString( 44, 412, va( "REFUSED: %s", line ), UI_TINYFONT, colorTable[CT_RED] );

	UI_DrawProportionalString( 44, 426, "UP/DOWN line   S strike   F soften the number   ENTER sign to the crew   U file it upward   ESC leave",
		UI_TINYFONT, colorTable[CT_LTPURPLE1] );
}

bool ReportAct( int key )
{
	switch ( key )
	{
	case K_UPARROW: if ( report.cursor > 0 ) --report.cursor; return true;
	case K_DOWNARROW: ++report.cursor; return true;
	case 's': case 'S': ui.Cmd_ExecuteText( EXEC_APPEND, va( "ship report strike %d\n", report.cursor ) ); return true;
	case 'f': case 'F': ui.Cmd_ExecuteText( EXEC_APPEND, va( "ship report soften %d 0.5\n", report.cursor ) ); return true;
	case 'u': case 'U': ui.Cmd_ExecuteText( EXEC_APPEND, "ship report file\n" ); return true;
	case K_ENTER: case K_KP_ENTER:
	{
		char idx[16];
		ui.Cvar_VariableStringBuffer( "lwh_ship_player_index", idx, sizeof( idx ) );
		ui.Cmd_ExecuteText( EXEC_APPEND, va( "ship report sign %s\n", idx[0] ? idx : "0" ) );
		return true;
	}
	}
	return false;
}

sfxHandle_t ReportKey( int key )
{
	if ( ReportAct( key ) ) return menu_null_sound;
	return Menu_DefaultKey( &report.menu, key );
}

// ---- the job-queue board (row 19: the queue as its own face) ----------------------------------
//
// docs/crew-work.md: "the queue needs a board", and "priority is where rank lives." The queue existed
// and showed only as the Engineering list and the command console's GIVEN UP. Here it is itself: one
// row per job, what it is, how far, its place -- and the one command-side act, setting the order,
// which the ship refuses to anyone who does not command. Build is the net-new job command can order.

struct {
	menuframework_s menu;
	int cursor;
} jobs;

int JobPriorityAt( int index )
{//read the selected job's priority back from the queue the ship published
	char rows[2048];
	ui.Cvar_VariableStringBuffer( "lwh_ship_jobs", rows, sizeof( rows ) );
	int n = 0;
	for ( char *tok = strtok( rows, ";" ); tok; tok = strtok( NULL, ";" ), ++n )
	{
		char *k = strchr( tok, '|' ); if ( !k ) continue; *k++ = 0;
		char *w = strchr( k, '|' ); if ( !w ) continue; *w++ = 0;
		char *pr = strchr( w, '|' ); if ( !pr ) continue; *pr++ = 0;
		char *prio = strchr( pr, '|' ); if ( !prio ) continue; *prio++ = 0;
		if ( n == index ) return atoi( prio );
	}
	return 0;
}

void JobsDraw( void )
{
	char rows[2048], line[256];
	UI_FillRect( 0, 0, 640, 480, colorTable[CT_BLACK] );
	UI_FillRect( 20, 16, 600, 22, colorTable[CT_LTGOLD1] );
	UI_FillRect( 20, 42, 14, 396, colorTable[CT_DKPURPLE1] );
	UI_FillRect( 20, 442, 600, 10, colorTable[CT_DKPURPLE1] );
	UI_DrawProportionalString( 44, 19, "COMMAND  -  THE JOB QUEUE", UI_SMALLFONT, colorTable[CT_BLACK] );
	ui.Cvar_VariableStringBuffer( "lwh_ship_job_count", line, sizeof( line ) );
	UI_DrawProportionalString( 44, 48, va( "%s JOB(S) OUTSTANDING  -  the damage-control party works the queue; command sets the order",
		line ), UI_TINYFONT, colorTable[CT_LTGOLD1] );
	UI_DrawProportionalString( 44, 68, "PRI", UI_TINYFONT, colorTable[CT_LTORANGE] );
	UI_DrawProportionalString( 110, 68, "KIND", UI_TINYFONT, colorTable[CT_LTORANGE] );
	UI_DrawProportionalString( 220, 68, "TARGET", UI_TINYFONT, colorTable[CT_LTORANGE] );
	UI_DrawProportionalString( 470, 68, "PROGRESS", UI_TINYFONT, colorTable[CT_LTORANGE] );

	ui.Cvar_VariableStringBuffer( "lwh_ship_jobs", rows, sizeof( rows ) );
	int n = 0;
	for ( char *tok = strtok( rows, ";" ); tok; tok = strtok( NULL, ";" ), ++n )
	{
		char *kind = strchr( tok, '|' ); if ( !kind ) break; *kind++ = 0;
		char *what = strchr( kind, '|' ); if ( !what ) break; *what++ = 0;
		char *prog = strchr( what, '|' ); if ( !prog ) break; *prog++ = 0;
		char *prio = strchr( prog, '|' ); if ( !prio ) break; *prio++ = 0;
		const int y = 88 + n * 16;
		const bool selected = n == jobs.cursor;
		if ( selected ) UI_FillRect( 38, y - 1, 582, 14, colorTable[CT_DKPURPLE2] );
		UI_DrawProportionalString( 44, y, prio, UI_TINYFONT, colorTable[selected ? CT_WHITE : CT_LTGOLD1] );
		UI_DrawProportionalString( 110, y, kind, UI_TINYFONT, colorTable[selected ? CT_WHITE : CT_LTBLUE2] );
		UI_DrawProportionalString( 220, y, what, UI_TINYFONT, colorTable[selected ? CT_WHITE : CT_LTGOLD1] );
		UI_FillRect( 470, y + 2, 120, 8, colorTable[CT_DKPURPLE3] );
		UI_FillRect( 470, y + 2, 120 * atoi( prog ) / 100, 8, colorTable[CT_LTBLUE2] );
	}
	if ( !n ) UI_DrawProportionalString( 44, 88, "THE QUEUE IS EMPTY  -  nothing needs doing", UI_SMALLFONT, colorTable[CT_LTBLUE2] );
	if ( n && jobs.cursor >= n ) jobs.cursor = n - 1;

	ui.Cvar_VariableStringBuffer( "lwh_ship_job_refused", line, sizeof( line ) );
	if ( line[0] ) UI_DrawProportionalString( 44, 412, va( "REFUSED: %s", line ), UI_TINYFONT, colorTable[CT_RED] );

	UI_DrawProportionalString( 44, 426, "UP/DOWN job   LEFT/RIGHT its place (left is sooner)   B build spare parts   ESC leave",
		UI_TINYFONT, colorTable[CT_LTPURPLE1] );
}

bool JobsAct( int key )
{
	switch ( key )
	{
	case K_UPARROW: if ( jobs.cursor > 0 ) --jobs.cursor; return true;
	case K_DOWNARROW: ++jobs.cursor; return true;
	case K_LEFTARROW: case K_RIGHTARROW:
	{
		char rows[64];
		ui.Cvar_VariableStringBuffer( "lwh_ship_jobs", rows, sizeof( rows ) );
		if ( !rows[0] ) return true; // an empty queue has no place to move
		const int delta = key == K_LEFTARROW ? -1 : 1;
		ui.Cmd_ExecuteText( EXEC_APPEND, va( "ship job %d %d\n", jobs.cursor, JobPriorityAt( jobs.cursor ) + delta ) );
		return true;
	}
	case 'b': case 'B': ui.Cmd_ExecuteText( EXEC_APPEND, "ship build 10\n" ); return true;
	}
	return false;
}

sfxHandle_t JobsKey( int key )
{
	if ( JobsAct( key ) ) return menu_null_sound;
	return Menu_DefaultKey( &jobs.menu, key );
}

// ---- the chart you can work (row 23) ----------------------------------------------------------
//
// The chart existed as a line on the Conn and as forecasts on the command console; there was no chart
// to work. Here it is: the sector's beacons, where the ship is, which are charted, and -- the
// decision -- where to make for. Command decides the course (docs/navigation-counter.md), and the
// Conn lays it in; the forecast for each beacon is command's to see. The horizon is the sector graph,
// not a map: a course is a sequence of jumps, and the estimates are the counter's.

struct {
	menuframework_s menu;
	int cursor;
} chart;

void ChartDraw( void )
{
	char rows[2048], line[512];
	UI_FillRect( 0, 0, 640, 480, colorTable[CT_BLACK] );
	UI_FillRect( 20, 16, 600, 22, colorTable[CT_LTGOLD1] );
	UI_FillRect( 20, 42, 14, 396, colorTable[CT_DKPURPLE1] );
	UI_FillRect( 20, 442, 600, 10, colorTable[CT_DKPURPLE1] );
	UI_DrawProportionalString( 44, 19, "COMMAND  -  THE CHART", UI_SMALLFONT, colorTable[CT_BLACK] );
	ui.Cvar_VariableStringBuffer( "lwh_ship_nav", line, sizeof( line ) );
	UI_DrawProportionalString( 44, 48, line, UI_TINYFONT, colorTable[CT_LTBLUE2] );
	UI_DrawProportionalString( 44, 68, "BEACON   WHAT IS KNOWN", UI_TINYFONT, colorTable[CT_LTORANGE] );

	ui.Cvar_VariableStringBuffer( "lwh_ship_sector_map", rows, sizeof( rows ) );
	int n = 0, selectedBeacon = -1;
	for ( char *tok = strtok( rows, ";" ); tok; tok = strtok( NULL, ";" ), ++n )
	{
		char *here = strchr( tok, '|' ); if ( !here ) break; *here++ = 0;
		char *kind = strchr( here, '|' ); if ( !kind ) break; *kind++ = 0;
		char *known = strchr( kind, '|' ); if ( !known ) break; *known++ = 0;
		const int y = 84 + n * 16;
		const bool selected = n == chart.cursor;
		if ( selected ) { UI_FillRect( 38, y - 1, 582, 14, colorTable[CT_DKPURPLE2] ); selectedBeacon = atoi( tok ); }
		UI_DrawProportionalString( 44, y, va( "%s%2s", atoi( here ) ? "> " : "  ", tok ), UI_TINYFONT,
			colorTable[selected ? CT_WHITE : atoi( here ) ? CT_LTGOLD1 : CT_LTBLUE2] );
		UI_DrawProportionalString( 120, y, va( "%s%s", kind, Q_stricmp( known, "known" ) ? " (uncharted)" : "" ),
			UI_TINYFONT, colorTable[selected ? CT_WHITE : Q_stricmp( known, "known" ) ? CT_LTPURPLE1 : CT_LTBLUE2] );
	}

	if ( n && chart.cursor >= n ) chart.cursor = n - 1;

	ui.Cvar_VariableStringBuffer( "lwh_ship_forecast", line, sizeof( line ) );
	UI_DrawProportionalString( 44, 358, "FORECASTS  (command's): the estimate from each beacon one jump away",
		UI_TINYFONT, colorTable[CT_LTORANGE] );
	UI_DrawProportionalString( 44, 372, line[0] ? line : "no forecasts", UI_TINYFONT, colorTable[CT_LTBLUE2] );

	ui.Cvar_VariableStringBuffer( "lwh_ship_course", line, sizeof( line ) );
	UI_DrawProportionalString( 44, 396, atoi( line ) >= 0 ? va( "COURSE SET: BEACON %s", line ) : "COURSE: none set",
		UI_TINYFONT, colorTable[CT_LTGOLD1] );

	UI_DrawProportionalString( 44, 426, "UP/DOWN beacon   ENTER set the course (the Conn lays it in)   ESC leave",
		UI_TINYFONT, colorTable[CT_LTPURPLE1] );
}

bool ChartAct( int key )
{
	switch ( key )
	{
	case K_UPARROW: if ( chart.cursor > 0 ) --chart.cursor; return true;
	case K_DOWNARROW: ++chart.cursor; return true;
	case K_ENTER: case K_KP_ENTER: ui.Cmd_ExecuteText( EXEC_APPEND, va( "ship course %d\n", chart.cursor ) ); return true;
	}
	return false;
}

sfxHandle_t ChartKey( int key )
{
	if ( ChartAct( key ) ) return menu_null_sound;
	return Menu_DefaultKey( &chart.menu, key );
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
	// The three screens the inventory marked missing or thin that are command's: the month report
	// editor, the job-queue board and the chart you can work (docs/evidence/the-missing-screens.md,
	// rows 19, 20 and 23). Each is driven by name so what a test presses is what a hand presses.
	if ( !Q_stricmp( cmd, "ui_lwh_report" ) ) { report.cursor = 0; Push( &report.menu, ReportDraw, ReportKey ); return qtrue; }
	if ( !Q_stricmp( cmd, "ui_lwh_jobs" ) ) { jobs.cursor = 0; Push( &jobs.menu, JobsDraw, JobsKey ); return qtrue; }
	if ( !Q_stricmp( cmd, "ui_lwh_chart" ) ) { chart.cursor = 0; Push( &chart.menu, ChartDraw, ChartKey ); return qtrue; }
	if ( !Q_stricmp( cmd, "lwh_report_key" ) )
	{
		ui.Argv( 1, arg, sizeof( arg ) );
		ReportAct( KeyByName( arg ) );
		return qtrue;
	}
	if ( !Q_stricmp( cmd, "lwh_jobs_key" ) )
	{
		ui.Argv( 1, arg, sizeof( arg ) );
		JobsAct( KeyByName( arg ) );
		return qtrue;
	}
	if ( !Q_stricmp( cmd, "lwh_chart_key" ) )
	{
		ui.Argv( 1, arg, sizeof( arg ) );
		ChartAct( KeyByName( arg ) );
		return qtrue;
	}
	return qfalse;
}
