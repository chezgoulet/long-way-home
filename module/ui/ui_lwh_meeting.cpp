// ui_lwh_meeting.cpp -- the meeting overlay (docs/staff-meetings.md, phase three / M3).
//
// A meeting must become something a player attends. The queue of briefs the simulation has enqueued
// is a screen a hand can open; opening one takes the brief (TakeBrief) and shows the room: a speaker
// rail (name, post, watch, mood), the line being answered, the enumerated options as pills with their
// costs, and a free-text pill carrying a VOICE affordance. A pill plays its authored-skeleton lines
// and then sends the choice back to the simulation, which applies exactly one enumerated outcome
// (ApplyMeetingOutcome). The screen decides nothing and writes no ship state: it reads the ship's
// published cvars and sends `ship meeting` commands, exactly as the station consoles do.
//
// The overlay is anchored to the lower part of the frame so the scene stays visible above it, and it
// is the storyboard's shape (docs/art/lcars-meeting-overlay.png): LCARS, minimal, one rail on the
// left, the answer line, then the pills. The look is the owner's to judge.
//
// VOICE does not exist yet. Its affordance is drawn from the start, on the free-text pill, so the
// final step is a substitution rather than a redesign.

#include "ui_local.h"

#include "lwh_ui.h"

#include <cstdlib>
#include <cstring>

namespace {

// The overlay's band: the lower 144px of the 640x480 virtual frame -- 30%, close to the storyboard's
// lower quarter once the 16:9 scene above it is counted. The scene above is never painted.
const int OV_TOP = 336;

// The room has two faces: the queue the player opens, and the room a brief opens into.
enum { FACE_QUEUE = 0, FACE_ROOM };
enum { MODE_CHOOSING = 0, MODE_PLAYING, MODE_DONE };

struct MeetingScreen {
	menuframework_s menu;
	int face;
	int cursor;       // the queue row, or the option (an index one past the last option is the free-text pill)
	int mode;         // choosing, playing the chosen outcome's lines, or done
	int playOutcome;  // the outcome being played
	int playLine;     // the line being played
	int playNext;     // ms when the next line plays
	bool typing;      // the free-text pill is collecting characters
	char buf[96];
	int bufLen;
	char said[320];   // what the novelty seam did with the last typed input
} meet;

// Split on `sep`, preserving empty fields ("A||B" -> 3 fields). Returns the field count.
int Split( char *s, char sep, char **out, int max )
{
	int n = 0;
	out[n++] = s;
	for ( char *p = s; *p && n < max; ++p )
		if ( *p == sep ) { *p = 0; out[n++] = p + 1; }
	return n;
}

int QueueCount( void ) { return static_cast<int>( ui.Cvar_VariableValue( "lwh_ship_meeting_queue" ) ); }
int OptionCount( void ) { return static_cast<int>( ui.Cvar_VariableValue( "lwh_ship_meeting_options" ) ); }
int LineCount( int outcome ) { return static_cast<int>( ui.Cvar_VariableValue( va( "lwh_ship_meeting_lc%d", outcome ) ) ); }

// One skeleton line as the ship published it: speaker name|post|watch|mood|text|delivery|seconds. The
// delivery is read into the struct so it is carried with the line (Task C) and is deliberately never
// drawn: the seam to synthesis owns it, not the frame. The 7th field (M5) is the line's own duration,
// measured from the rendered clip: it paces the pills, so the options are offered for as long as the
// line actually runs.
struct Line {
	char name[32], post[40], watch[16], mood[24], text[200], delivery[20];
	float seconds;
	bool ok;
};
Line ReadLine( int outcome, int line )
{
	Line L;
	memset( &L, 0, sizeof( L ) );
	char buf[384];
	ui.Cvar_VariableStringBuffer( va( "lwh_ship_meeting_line%d_%d", outcome, line ), buf, sizeof( buf ) );
	char *f[7];
	const int n = Split( buf, '|', f, 7 );
	if ( n < 6 ) return L;
	Q_strncpyz( L.name, f[0], sizeof( L.name ) );
	Q_strncpyz( L.post, f[1], sizeof( L.post ) );
	Q_strncpyz( L.watch, f[2], sizeof( L.watch ) );
	Q_strncpyz( L.mood, f[3], sizeof( L.mood ) );
	Q_strncpyz( L.text, f[4], sizeof( L.text ) );
	Q_strncpyz( L.delivery, f[5], sizeof( L.delivery ) );
	L.seconds = n >= 7 ? static_cast<float>( atof( f[6] ) ) : 0.0f;
	L.ok = true;
	return L;
}

// The milliseconds a line's pill is offered for: its measured duration when it has one, else a small
// floor so a text-only line (no model) still advances.
static int LineMs( int outcome, int line )
{
	const Line L = ReadLine( outcome, line );
	return L.seconds > 0.05f ? static_cast<int>( L.seconds * 1000.0f ) : 650;
}

void Send( const char *command ) { ui.Cmd_ExecuteText( EXEC_APPEND, va( "%s\n", command ) ); }

// Fit `src` into the column from x to right, measuring with the engine's own font rather than counting
// characters. A character budget clipped the option "The holodecks full, the shields dark" to
// "THE HOLODECKS FULL, THE SHIELD." -- a sentence that stops mid-phrase reads as broken rather than as
// a short label, and the owner's ruling is that on-screen text is not lost to fit. Whole words are
// taken while they fit; the column cannot give the rest a second line, so what does not fit is
// REPORTED rather than dropped in silence.
void FitToColumn( char *out, int size, const char *src, int x, int right )
{
	const int width = right - x;
	if ( UI_ProportionalStringWidth( src, UI_TINYFONT ) <= width )
	{
		Q_strncpyz( out, src, size );
		return;
	}

	int took = 0;
	const char *p = src;
	out[0] = 0;
	while ( *p )
	{
		char word[96], candidate[192];
		int wl = 0;
		while ( *p == ' ' ) ++p;
		while ( *p && *p != ' ' && wl < static_cast<int>( sizeof( word ) ) - 1 ) word[wl++] = *p++;
		word[wl] = 0;
		if ( !word[0] ) break;
		if ( out[0] ) Com_sprintf( candidate, sizeof( candidate ), "%s %s", out, word );
		else          Q_strncpyz( candidate, word, sizeof( candidate ) );
		if ( UI_ProportionalStringWidth( candidate, UI_TINYFONT ) > width ) break;
		Q_strncpyz( out, candidate, size );
		took = 1;
	}
	if ( !took ) Q_strncpyz( out, src, size );  // one word wider than the column: show it whole
	ui.Printf( "LWH: meeting overlay: the column %d..%d is too narrow for \"%s\"; showing \"%s\"\n",
		x, right, src, out );
}


// A word-wrapped line, up to `maxLines`; returns the number drawn.
int Wrapped( int x, int y, const char *text, int style, vec4_t colour, int maxChars, int lineHeight, int maxLines )
{
	char line[160];
	int n = 0, col = 0;
	const char *p = text;
	line[0] = 0;
	while ( *p && n < maxLines )
	{
		const char *word = p;
		while ( *p && *p != ' ' ) ++p;
		const int wlen = static_cast<int>( p - word );
		if ( col && col + 1 + wlen > maxChars )
		{
			UI_DrawProportionalString( x, y + n * lineHeight, line, style, colour );
			++n;
			line[0] = 0; col = 0;
			if ( n >= maxLines ) break;
		}
		if ( col ) line[col++] = ' ';
		for ( int i = 0; i < wlen && col < static_cast<int>( sizeof( line ) ) - 1; ++i ) line[col++] = word[i];
		line[col] = 0;
		while ( *p == ' ' ) ++p;
	}
	if ( n < maxLines && col ) { UI_DrawProportionalString( x, y + n * lineHeight, line, style, colour ); ++n; }
	return n;
}

// The rail: the speaker of the line being answered -- name, post, watch and mood, the storyboard's
// four facts. Colour is the storyboard's: a purple elbow, an amber name band, then the reads.
void DrawRail( const Line &L )
{
	UI_FillRect( 0, OV_TOP, 150, 480 - OV_TOP, colorTable[CT_BLACK] );
	UI_FillRect( 0, OV_TOP, 128, 22, colorTable[CT_LTPURPLE1] );   // the elbow
	UI_FillRect( 0, OV_TOP + 26, 128, 28, colorTable[CT_LTGOLD1] ); // the name band
	UI_DrawProportionalString( 8, OV_TOP + 28, L.name[0] ? L.name : "THE ROOM", UI_SMALLFONT, colorTable[CT_BLACK] );
	UI_DrawProportionalString( 8, OV_TOP + 42, L.post, UI_TINYFONT, colorTable[CT_BLACK] );
	UI_FillRect( 0, OV_TOP + 60, 118, 15, colorTable[CT_LTBLUE2] );
	UI_DrawProportionalString( 6, OV_TOP + 62, "WATCH", UI_TINYFONT, colorTable[CT_BLACK] );
	UI_DrawProportionalString( 74, OV_TOP + 62, L.watch, UI_TINYFONT, colorTable[CT_BLACK] );
	UI_FillRect( 0, OV_TOP + 79, 118, 15, colorTable[CT_LTBLUE1] );
	UI_DrawProportionalString( 6, OV_TOP + 81, "MOOD", UI_TINYFONT, colorTable[CT_BLACK] );
	UI_DrawProportionalString( 74, OV_TOP + 81, L.mood, UI_TINYFONT, colorTable[CT_BLACK] );
}

void QueueDraw( void )
{
	UI_FillRect( 0, OV_TOP, 640, 480 - OV_TOP, colorTable[CT_BLACK] );
	UI_FillRect( 0, OV_TOP, 640, 4, colorTable[CT_LTGOLD1] );
	UI_DrawProportionalString( 12, OV_TOP + 8, "STAFF MEETINGS  -  THE QUEUE", UI_SMALLFONT, colorTable[CT_LTGOLD1] );
	const int q = QueueCount();
	UI_DrawProportionalString( 300, OV_TOP + 10, va( "%d waiting.  The room takes the oldest.", q ), UI_TINYFONT, colorTable[CT_LTPURPLE1] );
	UI_DrawProportionalString( 12, OV_TOP + 30, "KIND", UI_TINYFONT, colorTable[CT_LTORANGE] );
	UI_DrawProportionalString( 150, OV_TOP + 30, "DECIDING", UI_TINYFONT, colorTable[CT_LTORANGE] );
	for ( int i = 0; i < q && i < 5; ++i )
	{
		char buf[256], *f[4];
		ui.Cvar_VariableStringBuffer( va( "lwh_ship_meeting_q%d", i ), buf, sizeof( buf ) );
		if ( Split( buf, '|', f, 4 ) < 4 ) continue;
		const int y = OV_TOP + 46 + i * 16;
		const bool sel = i == meet.cursor;
		if ( sel ) UI_FillRect( 6, y - 1, 628, 14, colorTable[CT_DKPURPLE2] );
		UI_DrawProportionalString( 12, y, f[0], UI_TINYFONT, colorTable[sel ? CT_WHITE : CT_LTGOLD1] );
		UI_DrawProportionalString( 150, y, f[1], UI_TINYFONT, colorTable[sel ? CT_WHITE : CT_LTBLUE2] );
	}
	if ( !q ) UI_DrawProportionalString( 12, OV_TOP + 46, "NOTHING IS WAITING.  A meeting is called by the clock or a threshold.", UI_TINYFONT, colorTable[CT_LTBLUE2] );
	if ( q && meet.cursor >= q ) meet.cursor = q - 1;
	UI_DrawProportionalString( 12, 468, "UP/DOWN meeting   ENTER open it   ESC leave", UI_TINYFONT, colorTable[CT_LTPURPLE1] );
}

void RoomDraw( void )
{
	const int options = OptionCount();
	// Which line is being answered: the highlighted option's opening line, or the line playing.
	int showOutcome = meet.cursor < options ? meet.cursor : 0;
	int showLine = 0;
	if ( meet.mode == MODE_PLAYING || meet.mode == MODE_DONE )
	{
		showOutcome = meet.playOutcome;
		showLine = meet.playLine < LineCount( showOutcome ) ? meet.playLine : LineCount( showOutcome ) - 1;
		if ( showLine < 0 ) showLine = 0;
	}
	const Line L = ReadLine( showOutcome, showLine );

	UI_FillRect( 150, OV_TOP, 490, 480 - OV_TOP, colorTable[CT_BLACK] );
	UI_FillRect( 0, OV_TOP, 640, 4, colorTable[CT_LTGOLD1] );
	DrawRail( L );

	// The line being answered. Task A (docs/evidence/model-call.md): the engine's version stamp is
	// drawn by UI_MenuFrame2 AFTER every menu's content, at (371,445) with the tiny font, so no fill
	// of ours can hide it and it must be kept in a clear strip. The answer is measured first; the
	// pills below are then placed against what is left above y=443, so the stamp lands in a gap.
	UI_DrawProportionalString( 158, OV_TOP + 6, "THE LINE BEING ANSWERED", UI_TINYFONT, colorTable[CT_LTORANGE] );
	int answerLines = 0;
	if ( L.ok ) answerLines = Wrapped( 158, OV_TOP + 18, L.text, UI_SMALLFONT, colorTable[CT_WHITE], 74, 11, 2 );

	if ( meet.mode == MODE_DONE )
	{
		char outcome[128];
		ui.Cvar_VariableStringBuffer( va( "lwh_ship_meeting_opt%d", meet.playOutcome ), outcome, sizeof( outcome ) );
		char *bar = strchr( outcome, '|' );
		if ( bar ) *bar = 0;
		UI_DrawProportionalString( 158, OV_TOP + 50, va( "DECIDED: %s  -  the simulation applied it; the log records it.", outcome ),
			UI_TINYFONT, colorTable[CT_LTBLUE2] );
	}
	else
	{
		// The pills: each the option's label and the short description the design asks for (its
		// cost), each fitted to its column by measurement (FitToColumn). The block is packed above
		// the version stamp's clear strip: it starts just below the answer and compresses its line
		// height if the outcomes and the free-text pill would together reach y=443. Task A lifts the
		// row ~16 units from where M3 drew it, which is what moves the free-text pill clear of the
		// stamp (a fixed 16px pitch with four outcomes put it at y=438, inside the stamp at y=445).
		int dy = 14;
		const int pillTop = OV_TOP + 18 + ( answerLines > 0 ? answerLines : 1 ) * 11 + 6;
		const int rows = options + 1;
		const int avail = 441 - pillTop;
		if ( rows > 0 && rows * dy > avail ) dy = avail / rows;
		if ( dy < 9 ) dy = 9;
		for ( int i = 0; i < options && i < 6; ++i )
		{
			char buf[256], *f[4], label[192], cost[192], whole[192];
			ui.Cvar_VariableStringBuffer( va( "lwh_ship_meeting_opt%d", i ), buf, sizeof( buf ) );
			if ( Split( buf, '|', f, 4 ) < 4 ) continue;
			Com_sprintf( whole, sizeof( whole ), "%d %s", i + 1, f[0] );
			FitToColumn( label, sizeof( label ), whole, 172, 350 );
			FitToColumn( cost, sizeof( cost ), f[1], 356, 632 );
			const int y = pillTop + i * dy;
			const bool sel = i == meet.cursor && meet.mode == MODE_CHOOSING;
			UI_FillRect( 158, y - 1, 476, dy - 1, colorTable[CT_BLACK] );
			if ( sel ) UI_FillRect( 158, y - 1, 476, dy - 1, colorTable[CT_DKPURPLE2] );
			UI_FillRect( 158, y - 1, 7, dy - 1, colorTable[i == 0 ? CT_LTGOLD1 : i == 1 ? CT_LTPURPLE1 : i == 2 ? CT_LTBLUE2 : CT_LTBLUE1] );
			UI_DrawProportionalString( 172, y, label, UI_TINYFONT, colorTable[sel ? CT_WHITE : CT_LTGOLD1] );
			UI_DrawProportionalString( 356, y, cost, UI_TINYFONT, colorTable[sel ? CT_WHITE : CT_LTBLUE2] );
		}
		// The free-text pill: an entry field, visibly different, with the VOICE affordance from the
		// start even though voice is not built.
		{
			const int y = pillTop + options * dy;
			const bool sel = meet.cursor == options && meet.mode == MODE_CHOOSING;
			UI_FillRect( 158, y - 1, 476, dy - 1, colorTable[CT_BLACK] );
			UI_FillRect( 158, y - 1, 476, dy - 1, colorTable[sel ? CT_DKPURPLE2 : CT_DKPURPLE3] );
			UI_FillRect( 158, y - 1, 7, dy - 1, colorTable[CT_LTPURPLE1] );
			UI_DrawProportionalString( 172, y, "SAY SOMETHING ELSE", UI_TINYFONT, colorTable[CT_LTPURPLE1] );
			UI_DrawProportionalString( 356, y, meet.typing ? meet.buf : "type an answer", UI_TINYFONT, colorTable[meet.typing ? CT_WHITE : CT_DKGREY] );
			if ( meet.typing ) UI_DrawProportionalString( 356 + static_cast<int>( strlen( meet.buf ) ) * 6, y, "_", UI_TINYFONT, colorTable[CT_WHITE] );
			UI_FillRect( 596, y - 1, 38, dy - 1, colorTable[CT_LTPURPLE1] );
			UI_DrawProportionalString( 601, y, "VOICE", UI_TINYFONT, colorTable[CT_BLACK] );
		}
	}

	// The room itself: who is present, and the scope each one's brief was built from -- a post reads
	// its own scope, command reads all (docs/the-record-and-the-log.md).
	{
		char line[512];
		line[0] = 0;
		const int n = static_cast<int>( ui.Cvar_VariableValue( "lwh_ship_meeting_parts" ) );
		for ( int i = 0; i < n && i < 6; ++i )
		{
			char buf[192], *f[5];
			ui.Cvar_VariableStringBuffer( va( "lwh_ship_meeting_part%d", i ), buf, sizeof( buf ) );
			if ( Split( buf, '|', f, 5 ) < 5 ) continue;
			Q_strcat( line, sizeof( line ), va( "%s%s (%s)", i ? "   " : "IN THE ROOM:  ", f[0], f[4] ) );
		}
		UI_DrawProportionalString( 158, 458, line, UI_TINYFONT, colorTable[CT_LTBLUE2] );
	}
	// Task A: meet.said is drawn in the speaker rail's lower strip (x < 150), not at y=446 in the
	// free-text row. The rail is entirely left of the version stamp's box (x>=371), so the seam's
	// answer can never be read as the stamp's contents whatever its length. Named as a call.
	if ( meet.said[0] ) Wrapped( 6, OV_TOP + 98, meet.said, UI_TINYFONT, colorTable[CT_LTORANGE], 24, 9, 4 );

	UI_DrawProportionalString( 158, 470, meet.typing
		? "type   ENTER submit   ESC cancel"
		: "UP/DOWN  number  ENTER speak  T type an answer  R replay  ESC queue",
		UI_TINYFONT, colorTable[CT_LTPURPLE1] );
}

void Draw( void )
{
	if ( meet.face == FACE_QUEUE ) { QueueDraw(); return; }
	// The playback advances on the frame clock: the outcome's lines, one at a time, then the decision.
	if ( meet.mode == MODE_PLAYING )
	{
		const int now = ui.Milliseconds();
		if ( now >= meet.playNext )
		{
			++meet.playLine;
			if ( meet.playLine >= LineCount( meet.playOutcome ) )
			{
				Send( va( "ship meeting choose %d", meet.playOutcome ) );
				meet.mode = MODE_DONE;
				if ( meet.playLine > 0 ) meet.playLine = LineCount( meet.playOutcome ) - 1;
			}
			else meet.playNext = now + LineMs( meet.playOutcome, meet.playLine );
		}
	}
	RoomDraw();
}

int KeyByName( const char *name )
{
	if ( !Q_stricmp( name, "up" ) ) return K_UPARROW;
	if ( !Q_stricmp( name, "down" ) ) return K_DOWNARROW;
	if ( !Q_stricmp( name, "enter" ) ) return K_ENTER;
	if ( !Q_stricmp( name, "escape" ) ) return K_ESCAPE;
	if ( !Q_stricmp( name, "backspace" ) ) return K_BACKSPACE;
	return name[0] && !name[1] ? name[0] : 0;
}

bool Act( int key )
{
	if ( meet.face == FACE_QUEUE )
	{
		const int q = QueueCount();
		switch ( key )
		{
		case K_UPARROW: if ( q ) meet.cursor = ( meet.cursor + q - 1 ) % q; return true;
		case K_DOWNARROW: if ( q ) meet.cursor = ( meet.cursor + 1 ) % q; return true;
		case K_ENTER: case K_KP_ENTER:
			if ( !q ) return true;
			Send( "ship meeting open" );   // the room takes the oldest brief (TakeBrief)
			meet.face = FACE_ROOM;
			meet.cursor = 0;
			meet.mode = MODE_CHOOSING;
			meet.typing = false;
			meet.bufLen = 0; meet.buf[0] = 0;
			meet.said[0] = 0;
			return true;
		}
		return false;
	}

	// The room.
	if ( meet.typing )
	{
		if ( key & K_CHAR_FLAG )
		{
			const int c = key & ~K_CHAR_FLAG;
			if ( c >= 32 && c < 127 && meet.bufLen < static_cast<int>( sizeof( meet.buf ) ) - 1 )
			{
				meet.buf[meet.bufLen++] = static_cast<char>( c );
				meet.buf[meet.bufLen] = 0;
			}
			return true;
		}
		switch ( key )
		{
		case K_BACKSPACE:
			if ( meet.bufLen > 0 ) meet.buf[--meet.bufLen] = 0;
			return true;
		case K_ENTER: case K_KP_ENTER:
			if ( meet.bufLen > 0 )
			{
				// Typed text is not a branch: it goes to the novelty seam and the simulation applies
				// nothing (ship meeting say). It can never become a branch by accident.
				Send( va( "ship meeting say \"%s\"", meet.buf ) );
				Q_strncpyz( meet.said, "typed answer sent to the novelty seam", sizeof( meet.said ) );
			}
			meet.typing = false;
			return true;
		case K_ESCAPE:
			meet.typing = false;
			return true;
		}
		return true;
	}

	const int options = OptionCount();
	switch ( key )
	{
	case K_UPARROW: if ( options ) meet.cursor = ( meet.cursor + options ) % ( options + 1 ); return true;
	case K_DOWNARROW: if ( options ) meet.cursor = ( meet.cursor + 1 ) % ( options + 1 ); return true;
	case '1': case '2': case '3': case '4': case '5': case '6':
		// A number speaks that option at once: select it and play its branch.
		if ( meet.mode == MODE_CHOOSING && key - '0' >= 1 && key - '0' <= options )
		{
			meet.cursor = key - '0' - 1;
			meet.playOutcome = meet.cursor;
			meet.playLine = 0;
			meet.playNext = ui.Milliseconds() + LineMs( meet.playOutcome, 0 );
			meet.mode = MODE_PLAYING;
		}
		return true;
	case 't': case 'T': meet.cursor = options; meet.typing = true; meet.bufLen = 0; meet.buf[0] = 0; return true;
	case 'r': case 'R':
		if ( meet.mode == MODE_DONE ) { meet.mode = MODE_PLAYING; meet.playLine = 0; meet.playNext = ui.Milliseconds() + 650; }
		return true;
	case K_ENTER: case K_KP_ENTER:
		if ( meet.mode != MODE_CHOOSING ) return true;
		if ( meet.cursor >= options ) { meet.typing = true; meet.bufLen = 0; meet.buf[0] = 0; return true; }
		meet.playOutcome = meet.cursor;
		meet.playLine = 0;
		meet.playNext = ui.Milliseconds() + 650;
		meet.mode = MODE_PLAYING;
		return true;
	case K_ESCAPE:
		Send( "ship meeting close" );
		meet.face = FACE_QUEUE;
		meet.mode = MODE_CHOOSING;
		return true;
	}
	return false;
}

sfxHandle_t Key( int key )
{
	if ( Act( key ) ) return menu_null_sound;
	return Menu_DefaultKey( &meet.menu, key );
}

// Append typed text to the free-text pill, as a hand's characters would: the test path and the hand
// path go through the same buffer, so what a test drives is what a hand drives.
void TypeText( const char *text )
{
	meet.face = FACE_ROOM;
	meet.typing = true;
	for ( const char *p = text; *p && meet.bufLen < static_cast<int>( sizeof( meet.buf ) ) - 1; ++p )
		meet.buf[meet.bufLen++] = *p;
	meet.buf[meet.bufLen] = 0;
}

void Open( int face )
{
	meet.face = face;
	meet.cursor = 0;
	meet.mode = MODE_CHOOSING;
	meet.typing = false;
	meet.bufLen = 0; meet.buf[0] = 0;
	meet.said[0] = 0;
	memset( &meet.menu, 0, sizeof( meet.menu ) );
	meet.menu.draw = Draw;
	meet.menu.key = Key;
	meet.menu.fullscreen = qfalse; // see ui_lwh_engineering.cpp: a fullscreen menu stops the game
	meet.menu.wrapAround = qtrue;
	meet.menu.initialized = qtrue;
	UI_PushMenu( &meet.menu );
	ui.Cvar_Set( "ui_liveMenu", "1" );
}

} // namespace

// Our meeting screens, called from LWH_UI_ConsoleCommand. `lwh_meet_key` drives the screen by name,
// so what a test presses is exactly what a hand presses.
qboolean LWH_UI_MeetingScreens( const char *cmd )
{
	if ( !Q_stricmp( cmd, "ui_lwh_meeting" ) ) { Open( FACE_QUEUE ); return qtrue; }
	if ( !Q_stricmp( cmd, "ui_lwh_meeting_room" ) ) { Open( FACE_ROOM ); return qtrue; }
	if ( !Q_stricmp( cmd, "lwh_meet_key" ) )
	{
		char arg[32];
		ui.Argv( 1, arg, sizeof( arg ) );
		Act( KeyByName( arg ) );
		return qtrue;
	}
	if ( !Q_stricmp( cmd, "lwh_meet_type" ) )
	{//a test types what a hand types, into the same free-text buffer
		char text[128];
		ui.Argv( 1, text, sizeof( text ) );
		TypeText( text );
		return qtrue;
	}
	return qfalse;
}
