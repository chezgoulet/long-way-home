// ui_lwh_ending.cpp -- the endgame screen: the reading, and whose record it becomes.
//
// The destinations were reached by the console and the model; this is the screen they have
// (docs/endings.md, "Structure" and "Acceptance"). It is a **list of readers**, in the design's
// order, and it is deliberately not a score screen: there is no rank, no quality and no score, and
// nothing is sorted. The rows come from the ship's own table (g_ship publishes them from
// ship::EndingScreenRows), the fixed words come from the same place so the model's neutrality
// assertion scans them too, and the screen only chooses. The record is still the last frame: the
// screen closes, the three-second arrival plays, and the reading and official log are printed after
// it.
//
// Operation: UP/DOWN choose a destination, LEFT/RIGHT turn the approach (the grind or the conduit)
// and Earth's filing question, ENTER arrive, ESC leave. A place that has not invited the run carries
// the cost of a play; ENTER makes the play through `ship wildcard`.

#include "ui_local.h"

#include "lwh_ui.h"
#include "lwh_textfit.h"

#include <cstdio>
#include <cstring>

namespace {

const int END_MAX_ROWS = 8;

struct EndRow
{
	int  dest;
	int  open;
	char name[64];
	char reader[64];
	char lens[192];
	char offer[192];
	char play[192];
};

struct EndScreen
{
	menuframework_s menu;
	int  cursor;
	int  count;
	EndRow row[END_MAX_ROWS];
	int  knowledge;
	int  won;
	int  conduit;
	int  filed;
	char title[128];
	char subtitle[256];
	char earned[128];
	char playHead[128];
	char approach[128];
	char grind[64];
	char conduitName[64];
	char conduitNote[192];
	char file[192];
	char keys[192];
	char last[256];
} endScreen;

void ReadCvar( const char *name, char *out, int size )
{
	out[0] = 0;
	ui.Cvar_VariableStringBuffer( name, out, size );
	out[size - 1] = 0;
}

int ReadInt( const char *name )
{
	char buf[32];
	ReadCvar( name, buf, sizeof( buf ) );
	return atoi( buf );
}

// "id|name|reader|lens|open|offer|play" -- the name, reader and lens may contain spaces, the bars
// are the only separators. A missing trailing field is empty, not a parse failure.
void SplitRow( char *text, EndRow &row )
{
	char *field[7] = { text, 0, 0, 0, 0, 0, 0 };
	for ( int i = 1; i < 7; ++i )
	{
		char *bar = field[i - 1] ? strchr( field[i - 1], '|' ) : 0;
		if ( !bar ) { field[i] = const_cast<char *>( "" ); continue; }
		*bar = 0;
		field[i] = bar + 1;
	}
	row.dest = atoi( field[0] );
	Q_strncpyz( row.name, field[1], sizeof( row.name ) );
	Q_strncpyz( row.reader, field[2], sizeof( row.reader ) );
	Q_strncpyz( row.lens, field[3], sizeof( row.lens ) );
	row.open = atoi( field[4] );
	Q_strncpyz( row.offer, field[5], sizeof( row.offer ) );
	Q_strncpyz( row.play, field[6], sizeof( row.play ) );
}

void LoadScreen( void )
{
	ReadCvar( "lwh_ship_ending_title", endScreen.title, sizeof( endScreen.title ) );
	ReadCvar( "lwh_ship_ending_subtitle", endScreen.subtitle, sizeof( endScreen.subtitle ) );
	ReadCvar( "lwh_ship_ending_earned", endScreen.earned, sizeof( endScreen.earned ) );
	ReadCvar( "lwh_ship_ending_play", endScreen.playHead, sizeof( endScreen.playHead ) );
	ReadCvar( "lwh_ship_ending_approach", endScreen.approach, sizeof( endScreen.approach ) );
	ReadCvar( "lwh_ship_ending_grind", endScreen.grind, sizeof( endScreen.grind ) );
	ReadCvar( "lwh_ship_ending_conduit", endScreen.conduitName, sizeof( endScreen.conduitName ) );
	ReadCvar( "lwh_ship_ending_conduit_note", endScreen.conduitNote, sizeof( endScreen.conduitNote ) );
	ReadCvar( "lwh_ship_ending_file", endScreen.file, sizeof( endScreen.file ) );
	ReadCvar( "lwh_ship_ending_keys", endScreen.keys, sizeof( endScreen.keys ) );
	ReadCvar( "lwh_ship_ending_last", endScreen.last, sizeof( endScreen.last ) );
	endScreen.knowledge = ReadInt( "lwh_ship_ending_knowledge" );
	endScreen.won = ReadInt( "lwh_ship_ending_won" );
	{
		char route[64];
		ReadCvar( "lwh_ship_ending_route", route, sizeof( route ) );
		endScreen.conduit = endScreen.conduitName[0] && !Q_stricmp( route, endScreen.conduitName );
	}
	endScreen.count = ReadInt( "lwh_ship_ending_rows" );
	if ( endScreen.count < 0 ) endScreen.count = 0;
	if ( endScreen.count > END_MAX_ROWS ) endScreen.count = END_MAX_ROWS;
	for ( int i = 0; i < endScreen.count; ++i )
	{
		char raw[640];
		ReadCvar( va( "lwh_ship_ending_row%d", i ), raw, sizeof( raw ) );
		SplitRow( raw, endScreen.row[i] );
	}
}

int SelectedDest( void )
{
	if ( endScreen.cursor < 0 || endScreen.cursor >= endScreen.count ) return -1;
	return endScreen.row[endScreen.cursor].dest;
}

void Commit( void )
{
	if ( !endScreen.won || endScreen.cursor < 0 || endScreen.cursor >= endScreen.count ) return;
	const EndRow &row = endScreen.row[endScreen.cursor];
	if ( !row.open && row.play[0] == 0 ) return; // nothing to commit, and no play to make
	const char *verb = row.open ? "ship ending" : "ship wildcard";
	const bool atEarth = row.dest == 0;
	const char *file = ( row.open && atEarth && !endScreen.filed ) ? " nofile" : "";
	const char *route = endScreen.conduit ? " conduit" : "";
	UI_ForceMenuOff();
	ui.Cmd_ExecuteText( EXEC_APPEND, va( "%s %d%s%s\n", verb, row.dest, file, route ) );
}

bool Act( int key )
{
	const int total = endScreen.count + 2; // the destinations, then the approach, then the file
	switch ( key )
	{
	case K_UPARROW:
		if ( endScreen.cursor > 0 ) --endScreen.cursor;
		return true;
	case K_DOWNARROW:
		if ( endScreen.cursor < total - 1 ) ++endScreen.cursor;
		return true;
	case K_LEFTARROW:
	case K_RIGHTARROW:
		if ( endScreen.cursor == endScreen.count )
		{
			if ( endScreen.knowledge ) endScreen.conduit = !endScreen.conduit;
		}
		else if ( endScreen.cursor == endScreen.count + 1 && SelectedDest() == 0 )
		{
			endScreen.filed = !endScreen.filed;
		}
		return true;
	case K_ENTER:
	case K_KP_ENTER:
		Commit();
		return true;
	}
	return false;
}

int MeasureStyle( const void *ctx, const char *text )
{
	return UI_ProportionalStringWidth( text, static_cast<int>( reinterpret_cast<intptr_t>( ctx ) ) );
}

int DrawWrapped( int x, int y, int width, int lineHeight, int style, vec4_t color, const char *text,
	int maxLines )
{
	const LWH_Wrapped w = LWH_WrapText( text ? text : "", width, maxLines, MeasureStyle,
		reinterpret_cast<const void *>( static_cast<intptr_t>( style ) ) );
	for ( int i = 0; i < w.count; ++i )
		UI_DrawProportionalString( x, y + i * lineHeight, w.line[i], style, color );
	return w.count;
}

void EndDraw( void )
{
	UI_FillRect( 0, 0, 640, 480, colorTable[CT_BLACK] );
	UI_FillRect( 20, 16, 600, 24, colorTable[CT_LTGOLD1] );
	UI_FillRect( 20, 44, 600, 8, colorTable[CT_DKPURPLE1] );
	UI_FillRect( 20, 442, 600, 10, colorTable[CT_DKPURPLE1] );
	UI_DrawProportionalString( 44, 20, endScreen.title, UI_SMALLFONT, colorTable[CT_BLACK] );
	UI_DrawProportionalString( 44, 48, endScreen.subtitle, UI_TINYFONT, colorTable[CT_LTPURPLE1] );

	// The destinations: name, reader, and whether the run has earned it or would have to make a play.
	UI_DrawProportionalString( 40, 66, endScreen.earned, UI_TINYFONT, colorTable[CT_LTORANGE] );
	UI_DrawProportionalString( 44, 84, "PLACE", UI_TINYFONT, colorTable[CT_MDGREY] );
	UI_DrawProportionalString( 270, 84, "READER", UI_TINYFONT, colorTable[CT_MDGREY] );
	UI_DrawProportionalString( 460, 84, "STATE", UI_TINYFONT, colorTable[CT_MDGREY] );
	for ( int i = 0; i < endScreen.count; ++i )
	{
		const int y = 100 + i * 17;
		const bool sel = endScreen.cursor == i;
		if ( sel ) UI_FillRect( 36, y - 2, 566, 16, colorTable[CT_DKPURPLE2] );
		const EndRow &row = endScreen.row[i];
		UI_DrawProportionalString( 44, y, row.name, UI_TINYFONT,
			colorTable[sel ? CT_WHITE : ( row.open ? CT_LTGOLD1 : CT_MDGREY )] );
		UI_DrawProportionalString( 270, y, row.reader, UI_TINYFONT,
			colorTable[sel ? CT_WHITE : CT_LTBLUE2] );
		const char *state = row.open ? "EARNED" : ( row.play[0] ? "A PLAY" : "NOT OPEN" );
		UI_DrawProportionalString( 460, y, state, UI_TINYFONT,
			colorTable[sel ? CT_WHITE : ( row.open ? CT_LTGOLD1 : CT_LTORANGE )] );
	}

	// The selected row's lens, and what it would take: the design's own words, not a score.
	int y = 100 + endScreen.count * 17 + 8;
	if ( endScreen.cursor >= 0 && endScreen.cursor < endScreen.count )
	{
		const EndRow &row = endScreen.row[endScreen.cursor];
		const int lines = DrawWrapped( 44, y, 560, 15, UI_TINYFONT, colorTable[CT_LTBLUE2], row.lens, 3 );
		y += lines * 15;
		const char *detail = row.open ? row.offer : ( row.play[0] ? row.play : row.offer );
		y += DrawWrapped( 44, y, 560, 15, UI_TINYFONT,
			colorTable[row.open ? CT_LTGOLD1 : CT_LTORANGE], detail, 2 ) * 15;
	}

	y += 4;
	{
		const bool sel = endScreen.cursor == endScreen.count;
		if ( sel ) UI_FillRect( 36, y - 2, 566, 16, colorTable[CT_DKPURPLE2] );
		UI_DrawProportionalString( 44, y, endScreen.approach, UI_TINYFONT,
			colorTable[sel ? CT_WHITE : CT_LTORANGE] );
		UI_DrawProportionalString( 260, y, endScreen.conduit ? endScreen.conduitName : endScreen.grind,
			UI_TINYFONT, colorTable[sel ? CT_WHITE : CT_LTGOLD1] );
		if ( !endScreen.knowledge )
			UI_DrawProportionalString( 380, y, endScreen.conduitNote, UI_TINYFONT, colorTable[CT_MDGREY] );
		y += 17;
	}
	{
		const bool sel = endScreen.cursor == endScreen.count + 1;
		if ( sel ) UI_FillRect( 36, y - 2, 566, 16, colorTable[CT_DKPURPLE2] );
		UI_DrawProportionalString( 44, y, endScreen.file, UI_TINYFONT,
			colorTable[sel ? CT_WHITE : CT_LTORANGE] );
		UI_DrawProportionalString( 380, y, endScreen.filed ? "FILE THE RECORD" : "HAND IT BACK UNREAD",
			UI_TINYFONT, colorTable[SelectedDest() == 0 ? ( sel ? CT_WHITE : CT_LTGOLD1 ) : CT_MDGREY] );
	}

	UI_DrawProportionalString( 44, 412, endScreen.keys, UI_TINYFONT, colorTable[CT_LTPURPLE1] );
	UI_DrawProportionalString( 44, 428, endScreen.last, UI_TINYFONT, colorTable[CT_LTPURPLE1] );
}

sfxHandle_t EndKey( int key )
{
	if ( Act( key ) ) return menu_null_sound;
	return Menu_DefaultKey( &endScreen.menu, key );
}

void OpenEnd( void )
{
	memset( &endScreen, 0, sizeof( endScreen ) );
	LoadScreen();
	endScreen.cursor = 0;
	endScreen.filed = 1;
	endScreen.menu.draw = EndDraw;
	endScreen.menu.key = EndKey;
	endScreen.menu.fullscreen = qfalse;
	endScreen.menu.wrapAround = qtrue;
	endScreen.menu.initialized = qtrue;
	UI_PushMenu( &endScreen.menu );
	ui.Cvar_Set( "ui_liveMenu", "1" );
}

qboolean EndingScreens( const char *cmd )
{
	if ( !Q_stricmp( cmd, "ui_lwh_ending" ) )
	{
		OpenEnd();
		return qtrue;
	}
	if ( !Q_stricmp( cmd, "lwh_ending_key" ) )
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
		Act( key );
		return qtrue;
	}
	return qfalse;
}

} // namespace

qboolean LWH_UI_EndingScreens( const char *cmd )
{
	return EndingScreens( cmd );
}
