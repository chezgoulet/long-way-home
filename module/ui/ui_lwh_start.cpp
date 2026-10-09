// ui_lwh_start.cpp -- the way in: the "Long Way Home" line on the main menu, and the configurator it
// opens.
//
// This is G1 and G2 in docs/the-entry-point.md. The main menu is upstream's; the attach point that
// adds our line is one call (LWH_UI_MainMenuAdd), and everything that decides anything lives here.
//
// The selector is a **list of start states as data** (STARTS[]), and each entry opens the four
// dimensions the owner's ruling names: who you are, who died, who fills the gaps, and the career
// path. The list is deliberately a list -- a further state is a row, not a rewrite. The *mechanism*
// is not here: this screen writes the choice into the run's cvars, and the ship itself derives the
// casualties, the vacancies and the first log entry (ship_core: ApplyStartState, FillVacancies,
// LogSeedText). The UI module and the game module are different libraries, so this is the seam they
// have always used: the screen decides, the ship does.
//
// The values a run needs are not here either: they live once, in configs/lwh-start.cfg, which both
// this screen and scripts/run-lwh.sh execute. This screen sets only the start-state cvars.

#include "ui_local.h"

#include "lwh_ui.h"

namespace {

// ---------------------------------------------------------------------------------------------
// The start states. Data, not code: a new state is a new row. Each is the canon default, the
// captain, or an all-fictitious crew -- the three proving cases of docs/the-entry-point.md.
// ---------------------------------------------------------------------------------------------
struct StartChoice
{
	const char   *name;
	const char   *blurb;
	const char   *reason;
	unsigned      casualties; // bitmask over the command seats
	bool          fictitious;
	unsigned char career;
	const char   *playerName;
	unsigned char playerRank;
	unsigned char playerDept;
	int           playerSeat; // -1: the derivation decides
};

enum SeatId { SEAT_CAPTAIN = 0, SEAT_FIRST_OFFICER, SEAT_SECURITY, SEAT_ENGINEERING,
              SEAT_MEDICAL, SEAT_SCIENCES, SEAT_OPERATIONS, SEAT_CONN, SEAT_COUNT };

const StartChoice STARTS[] =
{
	{
		"CANON",
		"The senior staff survive, the chain of command is intact, and the player is a junior officer.",
		"the default: a whole ship and a chain that runs from the captain down past you.",
		0, false, 0, "Reyes", 1, 2 /*security*/, -1,
	},
	{
		"THE CHAIR",
		"The captain was lost with the array; the chair is vacant and the senior officer remaining inherits it.",
		"the captain was lost with the array, and command passes down the chain to whoever is left.",
		(1u << SEAT_CAPTAIN), false, 3 /*the chair*/, "Reyes", 6, 0 /*command*/, -1,
	},
	{
		"ALL-FICTITIOUS",
		"None of the show characters appear: the whole crew is generated, and the vacancy rule fills the chain from it.",
		"the Caretaker took the senior staff, and the crew that remains is one nobody has heard of.",
		0, true, 0, "Mara Reyes", 1, 2 /*security*/, -1,
	},
};
const int START_COUNT = (int)( sizeof( STARTS ) / sizeof( STARTS[0] ) );

const char *const SEAT_NAMES[SEAT_COUNT] =
{
	"the chair", "the first officer's seat", "the security seat", "the engineering seat",
	"the medical seat", "the sciences seat", "the operations seat", "the conn",
};
const char *const CAREERS[] = { "Starfleet junior", "lower decks", "Maquis", "the chair" };
const char *const DEPARTMENTS[] = { "COMMAND", "ENGINEERING", "SECURITY", "SCIENCES", "MEDICAL" };
const char *const RANKS[] = { "CREWMAN", "ENSIGN", "LIEUTENANT J.G.", "LIEUTENANT", "LIEUTENANT COMMANDER", "COMMANDER", "CAPTAIN" };
// Invented registers, none a canon character.
const char *const NAMES[] = { "Reyes", "Okoro", "Lindqvist", "Tanaka", "Ferreira", "Mbeki", "Novak", "Castellanos", "Rahimi", "Whitlock" };
const int NUM_NAMES = sizeof( NAMES ) / sizeof( NAMES[0] );

menuframework_s startMenu;
int startCursor = 0;

// The edit the screen is building: seeded from a list entry, then turned by the four dimensions.
struct Edit
{
	int   preset;
	unsigned casualties;
	bool  fictitious;
	int   career;
	int   name;
	int   dept;
	int   rank;
	int   seat;
};
Edit edit;

// The rows: the preset list and the dimensions, the seats, the player register, and begin.
enum { ROW_PRESET = 0, ROW_CAREER, ROW_ROSTER, ROW_SEAT0, ROW_PLAYER = ROW_SEAT0 + SEAT_COUNT, ROW_BEGIN, ROW_COUNT };

void SeedFromPreset( int i )
{
	if ( i < 0 || i >= START_COUNT ) return;
	edit.preset = i;
	edit.casualties = STARTS[i].casualties;
	edit.fictitious = STARTS[i].fictitious;
	edit.career = STARTS[i].career;
	edit.rank = STARTS[i].playerRank;
	edit.dept = STARTS[i].playerDept;
	edit.seat = STARTS[i].playerSeat;
	edit.name = 0;
	for ( int n = 0; n < NUM_NAMES; ++n )
		if ( !Q_stricmp( NAMES[n], STARTS[i].playerName ) ) { edit.name = n; break; }
}

// ---------------------------------------------------------------------------------------------
// Beginning the run. Close the menu first -- a menu pauses the SP simulation (patch 0007), and a
// map load needs frames -- then write the choice into the run's cvars and run the one config. This
// is the retail New Game path (ui_game.cpp: UI_ForceMenuOff(), then "map ...").
// ---------------------------------------------------------------------------------------------
void BeginRun( void )
{
	ui.Cvar_Set( "g_shipConfigured", "1" );
	ui.Cvar_Set( "g_shipFictitious", edit.fictitious ? "1" : "0" );
	ui.Cvar_Set( "g_shipCareer", va( "%d", edit.career ) );
	ui.Cvar_Set( "g_shipCasualties", va( "%u", edit.casualties ) );
	ui.Cvar_Set( "g_shipPlayerName", NAMES[edit.name] );
	ui.Cvar_Set( "g_shipPlayerRank", va( "%d", edit.rank ) );
	ui.Cvar_Set( "g_shipPlayerDept", va( "%d", edit.dept ) );
	ui.Cvar_Set( "g_shipPlayerSeat", va( "%d", edit.seat ) );
	ui.Printf( "LWH: beginning the run -- %s, career %s, %s\n", STARTS[edit.preset].name,
		CAREERS[edit.career], edit.fictitious ? "an all-fictitious crew" : "the canon crew" );
	UI_ForceMenuOff();
	ui.Cmd_ExecuteText( EXEC_APPEND, "exec lwh-start.cfg\n" );
}

// The descriptions and the reason are author-written sentences, and the column beside the list is
// narrower than the longer ones are. Drawn on one line they ran off the right edge of the frame --
// found in the owner's read of the rendered screen (2026-10-09), where THE CHAIR stopped at
// "…THE SENIOR OFFICER REMAINING I". So the text is broken at a space where the next word would pass
// the column, and the caller shifts whatever sits below by however many lines it took. Nothing is
// dropped, and the width is measured with the engine's own font metrics rather than guessed.
static int DrawWrappedText( int x, int y, int width, int lineHeight, int style, vec4_t color,
	const char *text )
{
	char line[1024] = "";
	const char *p = text ? text : "";
	int lines = 0;

	while ( *p )
	{
		char word[512];
		int n = 0;
		while ( *p == ' ' ) ++p;
		while ( *p && *p != ' ' && n < (int)sizeof( word ) - 1 ) word[n++] = *p++;
		word[n] = '\0';
		if ( !word[0] ) break;

		char candidate[1024];
		if ( line[0] ) Com_sprintf( candidate, sizeof( candidate ), "%s %s", line, word );
		else           Q_strncpyz( candidate, word, sizeof( candidate ) );

		if ( line[0] && UI_ProportionalStringWidth( candidate, style ) > width )
		{
			UI_DrawProportionalString( x, y, line, style, color );
			y += lineHeight;
			++lines;
			Q_strncpyz( line, word, sizeof( line ) );
		}
		else
		{
			Q_strncpyz( line, candidate, sizeof( line ) );
		}
	}
	if ( line[0] )
	{
		UI_DrawProportionalString( x, y, line, style, color );
		++lines;
	}
	return lines;
}

void StartDraw( void )
{
	UI_FillRect( 0, 0, 640, 480, colorTable[CT_BLACK] );
	UI_FillRect( 20, 16, 600, 24, colorTable[CT_LTGOLD1] );
	UI_FillRect( 20, 44, 600, 10, colorTable[CT_DKPURPLE1] );
	UI_DrawProportionalString( 44, 20, "LONG WAY HOME  -  WHERE YOU BEGIN", UI_SMALLFONT, colorTable[CT_BLACK] );
	UI_DrawProportionalString( 44, 58,
		"The menu grants the situation. The fiction supplies the reason. The simulation holds you to it.",
		UI_TINYFONT, colorTable[CT_LTPURPLE1] );

	// The list, as data: the three proving cases, and the player's cursor on one.
	UI_DrawProportionalString( 40, 80, "START STATE", UI_TINYFONT, colorTable[CT_LTORANGE] );
	for ( int i = 0; i < START_COUNT; ++i )
	{
		const int y = 94 + i * 16;
		if ( i == edit.preset ) UI_FillRect( 36, y - 2, 250, 15, colorTable[CT_DKPURPLE2] );
		UI_DrawProportionalString( 44, y, STARTS[i].name, UI_TINYFONT,
			colorTable[i == edit.preset ? CT_WHITE : CT_LTGOLD1] );
	}
	int blurbLines = 0;
	if ( edit.preset >= 0 && edit.preset < START_COUNT )
	{
		blurbLines = DrawWrappedText( 300, 94, 312, 16, UI_TINYFONT, colorTable[CT_LTBLUE2],
			STARTS[edit.preset].blurb );
	}
	const int extraY = blurbLines > 2 ? ( blurbLines - 2 ) * 16 : 0;

	// The four dimensions, as rows the player turns.
	const int x = 40, xv = 210;
	int y = 158 + extraY;
	auto row = [&]( int r, const char *label, const char *value ) {
		const bool sel = ( startCursor == r );
		if ( sel ) UI_FillRect( 36, y - 2, 566, 15, colorTable[CT_DKPURPLE2] );
		UI_DrawProportionalString( x, y, label, UI_TINYFONT, colorTable[sel ? CT_WHITE : CT_LTORANGE] );
		UI_DrawProportionalString( xv, y, value, UI_TINYFONT, colorTable[sel ? CT_WHITE : CT_LTGOLD1] );
		y += 16;
	};

	{ char why[256]; Q_strncpyz( why, STARTS[edit.preset].reason, sizeof( why ) );
	  UI_DrawProportionalString( x, 136 + extraY, va( "Reason the crew accept it: %s", why ), UI_TINYFONT, colorTable[CT_LTPURPLE1] ); }

	row( ROW_PRESET, "START STATE", STARTS[edit.preset].name );
	row( ROW_CAREER, "CAREER PATH", CAREERS[edit.career] );
	row( ROW_ROSTER, "WHO FILLS THE GAPS", edit.fictitious ? "a generated crew (no show character)" : "the derivation, from the roster" );
	for ( int s = 0; s < SEAT_COUNT; ++s )
	{
		const bool lost = ( edit.casualties & ( 1u << s ) ) != 0;
		row( ROW_SEAT0 + s, "WHO DIED:", va( "%s  %s", lost ? "CASUALTY" : "survivor", SEAT_NAMES[s] ) );
	}
	row( ROW_PLAYER, "WHO YOU ARE", va( "%s %s, %s", RANKS[edit.rank], NAMES[edit.name], DEPARTMENTS[edit.dept] ) );

	// Begin, and the player's position stated plainly (the opening's own fourth duty).
	{
		const bool sel = ( startCursor == ROW_BEGIN );
		if ( sel ) UI_FillRect( 36, y - 2, 566, 15, colorTable[CT_DKPURPLE2] );
		UI_DrawProportionalString( x, y, "BEGIN THE RUN", UI_SMALLFONT, colorTable[sel ? CT_WHITE : CT_LTGOLD1] );
	}

	UI_FillRect( 20, 442, 600, 10, colorTable[CT_DKPURPLE1] );
	UI_DrawProportionalString( 44, 410,
		"UP/DOWN choose   LEFT/RIGHT change   ENTER begin   ESC leave", UI_TINYFONT, colorTable[CT_LTPURPLE1] );
	UI_DrawProportionalString( 44, 426,
		"the run's values live once, in configs/lwh-start.cfg (executed here and by scripts/run-lwh.sh)",
		UI_TINYFONT, colorTable[CT_MDGREY] );
}

bool StartAct( int key )
{
	switch ( key )
	{
	case K_UPARROW:   if ( startCursor > 0 ) --startCursor; return true;
	case K_DOWNARROW: if ( startCursor < ROW_COUNT - 1 ) ++startCursor; return true;
	case K_LEFTARROW:
	case K_RIGHTARROW:
	{
		const int d = key == K_RIGHTARROW ? 1 : -1;
		if ( startCursor == ROW_PRESET ) SeedFromPreset( ( edit.preset + START_COUNT + d ) % START_COUNT );
		else if ( startCursor == ROW_CAREER ) edit.career = ( edit.career + 4 + d ) % 4;
		else if ( startCursor == ROW_ROSTER ) edit.fictitious = !edit.fictitious;
		else if ( startCursor >= ROW_SEAT0 && startCursor < ROW_SEAT0 + SEAT_COUNT )
			edit.casualties ^= ( 1u << ( startCursor - ROW_SEAT0 ) );
		else if ( startCursor == ROW_PLAYER )
		{
			// The register cycles with N; the arrows choose department and rank.
			if ( key == K_LEFTARROW ) edit.dept = ( edit.dept + 4 ) % 5;
			else edit.dept = ( edit.dept + 1 ) % 5;
		}
		return true;
	}
	case K_ENTER:
	case K_KP_ENTER:  BeginRun(); return true;
	case 'n': case 'N': edit.name = ( edit.name + 1 ) % NUM_NAMES; return true;
	case '[': if ( edit.rank > 0 ) --edit.rank; return true;
	case ']': if ( edit.rank < 6 ) ++edit.rank; return true;
	}
	return false;
}

sfxHandle_t StartKey( int key )
{
	if ( StartAct( key ) ) return menu_null_sound;
	return Menu_DefaultKey( &startMenu, key );
}

void OpenStart( void )
{
	startCursor = 0;
	SeedFromPreset( 0 ); // the configurator opens on the canon default
	memset( &startMenu, 0, sizeof( startMenu ) );
	startMenu.draw        = StartDraw;
	startMenu.key         = StartKey;
	// Fullscreen on purpose: this selector lives on the main menu, and the engine's CA_DISCONNECTED
	// path re-pushes the main menu whenever the active menu is *not* fullscreen (cl_scrn.c: the
	// "force menu up" case). A non-fullscreen selector is therefore replaced by the main menu the
	// frame after it opens.
	startMenu.fullscreen  = qtrue;
	startMenu.wrapAround  = qtrue;
	startMenu.initialized = qtrue;
	UI_PushMenu( &startMenu );
}

// ---------------------------------------------------------------------------------------------
// The main-menu line. A bitmap button in the retail 3x3 grid's one empty cell (column 3, row 3),
// drawing itself the way the eight retail buttons draw so it reads as a mode beside them.
// ---------------------------------------------------------------------------------------------
char LWH_BUTTON_TEXT[] = "LONG WAY HOME";
menubitmap_s lwhMainButton;

void MainButtonDraw( void *self )
{
	menubitmap_s *b = (menubitmap_s *)self;
	const qboolean focus = ( Menu_ItemAtCursor( b->generic.parent ) == b );

	qhandle_t shader = b->shader;
	if ( !shader && b->generic.name ) shader = ui.R_RegisterShaderNoMip( b->generic.name );

	ui.R_SetColor( colorTable[focus ? b->color2 : b->color] );
	UI_DrawHandlePic( b->generic.x - 14, b->generic.y, b->height, b->height, uis.graphicButtonLeftEnd );
	UI_DrawHandlePic( b->generic.x, b->generic.y, b->width, b->height, shader );
	ui.R_SetColor( NULL );

	UI_DrawProportionalString( b->generic.x + b->textX, b->generic.y + b->textY, LWH_BUTTON_TEXT,
		UI_LEFT | UI_SMALLFONT, colorTable[focus ? CT_WHITE : CT_BLACK] );
}

void MainButtonEvent( void *ptr, int notification )
{
	if ( notification != QM_ACTIVATED ) return;
	OpenStart();
}

// The harness seam: activating the item through its own callback is the same path a hand's ENTER
// takes once the cursor is on it (Menu_DefaultKey calls exactly this). A green build proves nothing
// here (AGENTS: an engine rebuild wipes the module symlinks), so the evidence drives this and reads
// the rendered menu.
void PressMainButton( void )
{
	ui.Printf( "LWH: activating the main menu's Long Way Home line\n" );
	MainButtonEvent( &lwhMainButton, QM_ACTIVATED );
}

qboolean StartScreens( const char *cmd )
{
	if ( !Q_stricmp( cmd, "ui_lwh_start" ) )
	{
		OpenStart();
		return qtrue;
	}
	if ( !Q_stricmp( cmd, "lwh_start_key" ) )
	{
		char arg[32];
		ui.Argv( 1, arg, sizeof( arg ) );
		int key = 0;
		if      ( !Q_stricmp( arg, "up" ) )    key = K_UPARROW;
		else if ( !Q_stricmp( arg, "down" ) )  key = K_DOWNARROW;
		else if ( !Q_stricmp( arg, "left" ) )  key = K_LEFTARROW;
		else if ( !Q_stricmp( arg, "right" ) ) key = K_RIGHTARROW;
		else if ( !Q_stricmp( arg, "enter" ) ) key = K_ENTER;
		else if ( arg[0] && !arg[1] )          key = arg[0];
		StartAct( key );
		return qtrue;
	}
	if ( !Q_stricmp( cmd, "lwh_mainmenu_press" ) )
	{
		// What the main menu's "Long Way Home" line does when it is activated.
		PressMainButton();
		return qtrue;
	}
	return qfalse;
}

} // namespace

void LWH_UI_MainMenuAdd( void *menu )
{
	menuframework_s *m = (menuframework_s *)menu;

	memset( &lwhMainButton, 0, sizeof( lwhMainButton ) );
	lwhMainButton.generic.type     = MTYPE_BITMAP;
	lwhMainButton.generic.flags    = QMF_HIGHLIGHT_IF_FOCUS;
	lwhMainButton.generic.x        = 481;   // the retail grid's one free cell (mm_buttons[8])
	lwhMainButton.generic.y        = 109;
	lwhMainButton.generic.name     = GRAPHIC_BUTTONRIGHT;
	lwhMainButton.generic.callback = MainButtonEvent;
	lwhMainButton.generic.ownerdraw = MainButtonDraw;
	lwhMainButton.width            = MENU_BUTTON_MED_WIDTH;
	lwhMainButton.height           = MENU_BUTTON_MED_HEIGHT;
	lwhMainButton.color            = CT_DKPURPLE1;
	lwhMainButton.color2           = CT_LTPURPLE1;
	lwhMainButton.textX            = MENU_BUTTON_TEXT_X;
	lwhMainButton.textY            = MENU_BUTTON_TEXT_Y;
	lwhMainButton.textcolor        = CT_BLACK;
	lwhMainButton.textcolor2       = CT_WHITE;

	Menu_AddItem( m, &lwhMainButton );
}

qboolean LWH_UI_StartScreens( const char *cmd )
{
	return StartScreens( cmd );
}
