// ui_lwh_start.cpp -- the way in: the "Long Way Home" line on the main menu, and the start-state
// selector it opens.
//
// This is G1 and the first half of G2 in docs/the-entry-point.md, made visible. The main menu is
// upstream's; the attach point that adds our line is one call (LWH_UI_MainMenuAdd), and everything
// that decides anything lives here.
//
// The selector is deliberately a **list of start states with one entry**, not a hardcoded "there is
// only one". docs/the-entry-point.md Part three rules that the entry point becomes a configurator
// with four dimensions; when it does, it extends STARTS[] and this screen, and nothing else has to
// move. The one entry shipped is the canon default, which is what docs/start-states.md protects.
//
// The values the run needs are not here, and not in the launcher: they live once, in
// configs/lwh-start.cfg, which both this screen and scripts/run-lwh.sh execute. That is the "one
// place" the brief asks for: the selector decides *which* start state, the config decides *how* a
// run is set up, and a second copy of a cvar cannot drift into existence.

#include "ui_local.h"

#include "lwh_ui.h"

namespace {

// ---------------------------------------------------------------------------------------------
// The start states. One entry: the canon default. A start state is data (docs/start-states.md);
// this list is the surface, and adding a state is adding a row.
// ---------------------------------------------------------------------------------------------
struct StartState
{
	const char *name;
	const char *blurb;      // one line, kept short: the selector draws it without wrapping
	const char *reason;     // the fiction's reason the crew accept it (docs/start-states.md)
};

const StartState STARTS[] =
{
	{
		"CANON",
		"The senior staff survive, the chain of command is intact, and the player is a junior officer.",
		"the default: a whole ship and a chain that runs from the captain down past you.",
	},
};
const int START_COUNT = (int)( sizeof( STARTS ) / sizeof( STARTS[0] ) );

menuframework_s startMenu;
int startCursor = 0;

// ---------------------------------------------------------------------------------------------
// Beginning the run. Close the menu first -- a menu pauses the SP simulation (patch 0007), and a
// map load needs frames -- then run the one config. This is exactly the retail New Game path
// (ui_game.cpp: UI_ForceMenuOff(), then "map ..."), which is why it is safe in the main menu.
// ---------------------------------------------------------------------------------------------
void BeginRun( void )
{
	ui.Printf( "LWH: beginning the run -- exec lwh-start.cfg\n" );
	UI_ForceMenuOff();
	ui.Cmd_ExecuteText( EXEC_APPEND, "exec lwh-start.cfg\n" );
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

	for ( int i = 0; i < START_COUNT; ++i )
	{
		const int y = 92 + i * 20;
		const bool selected = ( i == startCursor );
		if ( selected ) UI_FillRect( 38, y - 3, 566, 17, colorTable[CT_DKPURPLE2] );
		UI_DrawProportionalString( 48, y, STARTS[i].name, UI_SMALLFONT,
			colorTable[selected ? CT_WHITE : CT_LTGOLD1] );
	}

	if ( START_COUNT )
	{
		const StartState &s = STARTS[startCursor];
		UI_DrawProportionalString( 44, 150, s.blurb, UI_TINYFONT, colorTable[CT_LTBLUE2] );
		UI_DrawProportionalString( 44, 168, va( "Reason the crew accept it: %s", s.reason ),
			UI_TINYFONT, colorTable[CT_LTPURPLE1] );
	}

	UI_FillRect( 20, 442, 600, 10, colorTable[CT_DKPURPLE1] );
	UI_DrawProportionalString( 44, 410,
		"UP/DOWN choose   ENTER begin   ESC leave", UI_TINYFONT, colorTable[CT_LTPURPLE1] );
	UI_DrawProportionalString( 44, 426,
		"the run's values live once, in configs/lwh-start.cfg (executed here and by scripts/run-lwh.sh)",
		UI_TINYFONT, colorTable[CT_MDGREY] );
}

sfxHandle_t StartKey( int key )
{
	switch ( key )
	{
	case K_UPARROW:   if ( startCursor > 0 ) --startCursor; return menu_null_sound;
	case K_DOWNARROW: if ( startCursor < START_COUNT - 1 ) ++startCursor; return menu_null_sound;
	case K_ENTER:
	case K_KP_ENTER:  BeginRun(); return menu_null_sound;
	}
	return Menu_DefaultKey( &startMenu, key );
}

void OpenStart( void )
{
	startCursor = 0;
	memset( &startMenu, 0, sizeof( startMenu ) );
	startMenu.draw        = StartDraw;
	startMenu.key         = StartKey;
	// Fullscreen on purpose: this selector lives on the main menu, and the engine's CA_DISCONNECTED
	// path re-pushes the main menu whenever the active menu is *not* fullscreen (cl_scrn.c: the
	// "force menu up" case). A non-fullscreen selector is therefore replaced by the main menu the
	// frame after it opens. The in-game consoles set fullscreen=qfalse because they live in a
	// different branch; this one must match the menu it replaces.
	startMenu.fullscreen  = qtrue;
	startMenu.wrapAround  = qtrue;
	startMenu.initialized = qtrue;
	UI_PushMenu( &startMenu );
}

// ---------------------------------------------------------------------------------------------
// The main-menu line. A bitmap button in the retail 3x3 grid's one empty cell (column 3, row 3),
// drawing itself the way the eight retail buttons draw so it reads as a mode beside them rather
// than as a tool. It is added to the menu the upstream MainMenu_Init hands us.
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
		else if ( !Q_stricmp( arg, "enter" ) ) key = K_ENTER;
		else if ( arg[0] && !arg[1] )          key = arg[0];
		StartKey( key );
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
