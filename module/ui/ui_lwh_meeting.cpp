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
#include "lwh_textfit.h"

#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace {

// The overlay's band: the lower 144px of the 640x480 virtual frame -- 30%, close to the storyboard's
// lower quarter once the 16:9 scene above it is counted. The scene above is never painted.
const int OV_TOP = 336;

// The screen has three faces: the queue the player opens, the room a brief opens into, and the
// order the giver dispatches to one person (docs/orders.md). The last is the same machinery -- a
// rail, the situation, the orders as pills with their costs, and a free-text pill.
enum { FACE_QUEUE = 0, FACE_ROOM, FACE_ORDER };
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

// Is the room open on the ship's computer? The computer is the meeting's own machinery, addressed
// rather than convened (docs/staff-meetings.md): the host publishes the kind, and the room reads it.
// The only difference is that a computer session answers and stays open, so the frame shows the
// conversation instead of one shot (Task A) -- the same file, not a second UI.
bool ComputerRoom( void )
{
	char kind[32];
	ui.Cvar_VariableStringBuffer( "lwh_ship_meeting_kind", kind, sizeof( kind ) );
	return Q_stricmp( kind, "computer" ) == 0;
}

// The room the brief names (docs/staff-meetings.md, the owner's ruling): the place is on the brief,
// published by the host as `lwh_ship_meeting_room`, and the overlay shows it. LCARS lettering is caps,
// so the place is upper-cased for the frame; the table keeps the room's own name.
void RoomCaps( char *out, int size )
{
	char room[64];
	ui.Cvar_VariableStringBuffer( "lwh_ship_meeting_room", room, sizeof( room ) );
	if ( !room[0] ) Q_strncpyz( room, "the room", sizeof( room ) );
	int i = 0;
	for ( ; room[i] && i < size - 1; ++i )
		out[i] = static_cast<char>( toupper( static_cast<unsigned char>( room[i] ) ) );
	out[i] = 0;
}

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

// The one wrap, measured with the engine's own font rather than counting characters (lwh_textfit.h).
// A character budget clipped the option "The holodecks full, the shields dark" to "THE HOLODECKS
// FULL, THE SHIELD." and the computer's transcript to half a refusal -- a sentence that stops
// mid-phrase reads as broken -- and the owner's ruling is that on-screen text is never lost to fit:
// **it wraps.** Every line the wrap returns is measured against the column it will be drawn in; a
// line that still will not fit (one word wider than the column) is REPORTED rather than dropped in
// silence, and so is text the caller's vertical budget cannot hold.
static int MeasureStyle( const void *ctx, const char *text )
{
	return UI_ProportionalStringWidth( text, static_cast<int>( reinterpret_cast<intptr_t>( ctx ) ) );
}

// Wrap `text` for the column x..right in `style`. `maxLines` is the caller's vertical budget.
static LWH_Wrapped WrapMeasured( int style, const char *text, int x, int right, int maxLines )
{
	return LWH_WrapText( text, right - x, maxLines, MeasureStyle,
		reinterpret_cast<const void *>( static_cast<intptr_t>( style ) ) );
}

static void ReportOverflow( const char *text, int x, int right, int maxLines )
{
	ui.Printf( "LWH: meeting overlay: the column %d..%d cannot hold \"%s\" in %d line(s)\n",
		x, right, text, maxLines );
}

// Draw `text` wrapped inside the column x..right from y, one measured line at a time; returns the
// number of lines drawn. Text the budget cannot hold is reported, never silently clipped.
static int DrawWrapped( int x, int y, int right, int lineHeight, int style, vec4_t colour,
	const char *text, int maxLines )
{
	const LWH_Wrapped w = WrapMeasured( style, text, x, right, maxLines );
	for ( int i = 0; i < w.count; ++i )
		UI_DrawProportionalString( x, y + i * lineHeight, w.line[i], style, colour );
	if ( w.overflow ) ReportOverflow( text, x, right, maxLines );
	return w.count;
}

// The option pills and the free-text pill, shared by the room and the order face. Each label and cost
// is wrapped to its own measured column, and a row grows only when a wrapped label or cost needs it --
// so fitting content draws exactly as it did before (one line, the same pitch) and longer content
// wraps instead of being cut. `order` selects the order face's cvars and the free-text placeholder's
// wording; everything else is identical.
static const char *OptCvar( bool order, int i )
{
	return order ? va( "lwh_ship_order_opt%d", i ) : va( "lwh_ship_meeting_opt%d", i );
}

static void DrawPillBlock( int pillTop, int options, bool order, bool highlighting, int cursor )
{
	int maxRowLines = 1;
	for ( int i = 0; i < options && i < 6; ++i )
	{
		char buf[256], *f[4], whole[192];
		ui.Cvar_VariableStringBuffer( OptCvar( order, i ), buf, sizeof( buf ) );
		if ( Split( buf, '|', f, 4 ) < 4 ) continue;
		Com_sprintf( whole, sizeof( whole ), "%d %s", i + 1, f[0] );
		const LWH_Wrapped lab = WrapMeasured( UI_TINYFONT, whole, 172, 350, 2 );
		const LWH_Wrapped cst = WrapMeasured( UI_TINYFONT, f[1], 356, 632, 2 );
		int lines = lab.count > cst.count ? lab.count : cst.count;
		if ( lines < 1 ) lines = 1;
		if ( lines > maxRowLines ) maxRowLines = lines;
	}

	int dy = maxRowLines > 1 ? maxRowLines * 10 + 4 : 14;
	const int rows = options + 1;
	const int avail = 441 - pillTop;
	if ( rows > 0 && rows * dy > avail ) dy = avail / rows;
	if ( dy < 9 ) dy = 9;
	if ( maxRowLines > 1 && dy < maxRowLines * 10 + 2 )
		ui.Printf( "LWH: meeting overlay: the pill block from %d cannot hold %d-line pills\n", pillTop, maxRowLines );

	for ( int i = 0; i < options && i < 6; ++i )
	{
		char buf[256], *f[4], whole[192];
		ui.Cvar_VariableStringBuffer( OptCvar( order, i ), buf, sizeof( buf ) );
		if ( Split( buf, '|', f, 4 ) < 4 ) continue;
		Com_sprintf( whole, sizeof( whole ), "%d %s", i + 1, f[0] );
		const int y = pillTop + i * dy;
		const bool sel = highlighting && i == cursor;
		UI_FillRect( 158, y - 1, 476, dy - 1, colorTable[CT_BLACK] );
		if ( sel ) UI_FillRect( 158, y - 1, 476, dy - 1, colorTable[CT_DKPURPLE2] );
		UI_FillRect( 158, y - 1, 7, dy - 1, colorTable[i == 0 ? CT_LTGOLD1 : i == 1 ? CT_LTPURPLE1 : i == 2 ? CT_LTBLUE2 : CT_LTBLUE1] );
		DrawWrapped( 172, y, 350, 10, UI_TINYFONT, colorTable[sel ? CT_WHITE : CT_LTGOLD1], whole, 2 );
		DrawWrapped( 356, y, 632, 10, UI_TINYFONT, colorTable[sel ? CT_WHITE : CT_LTBLUE2], f[1], 2 );
	}
	{
		const int y = pillTop + options * dy;
		const bool sel = highlighting && cursor == options;
		UI_FillRect( 158, y - 1, 476, dy - 1, colorTable[CT_BLACK] );
		UI_FillRect( 158, y - 1, 476, dy - 1, colorTable[sel ? CT_DKPURPLE2 : CT_DKPURPLE3] );
		UI_FillRect( 158, y - 1, 7, dy - 1, colorTable[CT_LTPURPLE1] );
		UI_DrawProportionalString( 172, y, "SAY SOMETHING ELSE", UI_TINYFONT, colorTable[CT_LTPURPLE1] );
		UI_DrawProportionalString( 356, y, meet.typing ? meet.buf : ( order ? "type an order" : "type an answer" ),
			UI_TINYFONT, colorTable[meet.typing ? CT_WHITE : CT_DKGREY] );
		if ( meet.typing ) UI_DrawProportionalString( 356 + static_cast<int>( strlen( meet.buf ) ) * 6, y, "_", UI_TINYFONT, colorTable[CT_WHITE] );
		UI_FillRect( 596, y - 1, 38, dy - 1, colorTable[CT_LTPURPLE1] );
		UI_DrawProportionalString( 601, y, "VOICE", UI_TINYFONT, colorTable[CT_BLACK] );
	}
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
	DrawWrapped( 34, OV_TOP + 62, 118, 8, UI_TINYFONT, colorTable[CT_BLACK], L.watch, 2 );
	UI_FillRect( 0, OV_TOP + 79, 118, 15, colorTable[CT_LTBLUE1] );
	UI_DrawProportionalString( 6, OV_TOP + 81, "MOOD", UI_TINYFONT, colorTable[CT_BLACK] );
	// The mood's own column: the value sits left of x=74 because "at breaking point" is 67px and the
	// old 44px column cut it to "at b". Named as a call -- it is the one rail fact long enough to clip.
	DrawWrapped( 34, OV_TOP + 81, 118, 8, UI_TINYFONT, colorTable[CT_BLACK], L.mood, 2 );
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
	UI_DrawProportionalString( 470, OV_TOP + 30, "ROOM", UI_TINYFONT, colorTable[CT_LTORANGE] );
	for ( int i = 0; i < q && i < 5; ++i )
	{
		char buf[256], *f[5];
		ui.Cvar_VariableStringBuffer( va( "lwh_ship_meeting_q%d", i ), buf, sizeof( buf ) );
		if ( Split( buf, '|', f, 5 ) < 5 ) continue;
		const int y = OV_TOP + 46 + i * 16;
		const bool sel = i == meet.cursor;
		if ( sel ) UI_FillRect( 6, y - 1, 628, 14, colorTable[CT_DKPURPLE2] );
		UI_DrawProportionalString( 12, y, f[0], UI_TINYFONT, colorTable[sel ? CT_WHITE : CT_LTGOLD1] );
		DrawWrapped( 150, y, 465, 8, UI_TINYFONT, colorTable[sel ? CT_WHITE : CT_LTBLUE2], f[1], 2 );
		UI_DrawProportionalString( 470, y, f[4], UI_TINYFONT, colorTable[sel ? CT_WHITE : CT_LTPURPLE1] );
	}
	if ( !q ) UI_DrawProportionalString( 12, OV_TOP + 46, "NOTHING IS WAITING.  A meeting is called by the clock or a threshold.", UI_TINYFONT, colorTable[CT_LTBLUE2] );
	if ( q && meet.cursor >= q ) meet.cursor = q - 1;
	UI_DrawProportionalString( 12, 468, "UP/DOWN meeting   ENTER open it   ESC leave", UI_TINYFONT, colorTable[CT_LTPURPLE1] );
}

// The computer's session on the same room (docs/staff-meetings.md, the depth pass). A meeting answers
// one line; the computer keeps talking, so the frame shows the run of exchanges -- ask and answer on a
// line, a refusal in a colour of its own. The transcript is host state, published as cvars; the screen
// decides nothing and writes no ship state. Each turn WRAPS to the column (the owner's ruling: text is
// never clipped to fit), so a refusal reads whole rather than ending at the frame edge. `maxLines` is
// the caller's vertical budget -- the pills below need the space above the engine's version stamp --
// and it is spent on whole turns from the newest backward (a scrollback); the full answer is in the
// log. The return is the number of lines drawn.
static const int TRANSCRIPT_TURNS = 8;

// The transcript line for turn `i`, composed as the frame draws it; "" when the turn is empty.
static const char *ComposeTranscript( int i )
{
	static char composed[640];
	composed[0] = 0;
	char buf[768], *f[4];
	ui.Cvar_VariableStringBuffer( va( "lwh_ship_meeting_turn%d", i ), buf, sizeof( buf ) );
	if ( Split( buf, '|', f, 4 ) < 3 ) return composed;
	Com_sprintf( composed, sizeof( composed ), "> %s  ->  %s", f[0], f[1] );
	return composed;
}

static bool TurnRefused( int i )
{
	char buf[768], *f[4];
	ui.Cvar_VariableStringBuffer( va( "lwh_ship_meeting_turn%d", i ), buf, sizeof( buf ) );
	if ( Split( buf, '|', f, 4 ) < 3 ) return false;
	return Q_stricmp( f[2], "answered" ) != 0;
}

int DrawTranscript( int x, int right, int top, int lineHeight, int maxLines )
{
	const int turns = static_cast<int>( ui.Cvar_VariableValue( "lwh_ship_meeting_turns" ) );
	if ( turns <= 0 )
	{
		UI_DrawProportionalString( x, top, "THE COMPUTER IS READY.  ASK IT SOMETHING.", UI_TINYFONT, colorTable[CT_LTBLUE2] );
		return 1;
	}

	// How many of the newest turns fit? Whole turns only: the oldest whole turns are dropped, so a
	// wrapped line never runs into the next entry's space. If even the newest turn alone is taller
	// than the budget, its column is the thing to widen -- it is drawn to the budget and reported.
	const int first = turns > TRANSCRIPT_TURNS ? turns - TRANSCRIPT_TURNS : 0;
	int shownFrom = turns;
	int used = 0;
	int capped = -1;
	for ( int i = turns - 1; i >= first; --i )
	{
		const char *composed = ComposeTranscript( i );
		if ( !composed[0] ) continue;
		const LWH_Wrapped w = WrapMeasured( UI_TINYFONT, composed, x, right, LWH_WRAP_MAX_LINES );
		if ( used + w.count > maxLines )
		{
			if ( used == 0 ) { capped = i; shownFrom = i; used = maxLines; }
			break;
		}
		used += w.count;
		shownFrom = i;
	}

	int y = top;
	for ( int i = shownFrom; i < turns; ++i )
	{
		const char *composed = ComposeTranscript( i );
		if ( !composed[0] ) continue;
		const LWH_Wrapped w = WrapMeasured( UI_TINYFONT, composed, x, right, maxLines );
		vec4_t &colour = colorTable[TurnRefused( i ) ? CT_LTORANGE : CT_WHITE];
		for ( int j = 0; j < w.count; ++j )
		{
			UI_DrawProportionalString( x, y, w.line[j], UI_TINYFONT, colour );
			y += lineHeight;
		}
		if ( w.overflow ) ReportOverflow( composed, x, right, maxLines );
		if ( i == capped ) break;
	}
	return used > 0 ? used : 1;
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
	const bool computer = ComputerRoom();

	UI_FillRect( 150, OV_TOP, 490, 480 - OV_TOP, colorTable[CT_BLACK] );
	UI_FillRect( 0, OV_TOP, 640, 4, colorTable[CT_LTGOLD1] );
	DrawRail( L );

	// The line being answered -- or, on the computer, the whole conversation so far. Task A
	// (docs/evidence/model-call.md): the engine's version stamp is drawn by UI_MenuFrame2 AFTER every
	// menu's content, at (371,445) with the tiny font, so no fill of ours can hide it and it must be kept
	// in a clear strip. The answer is measured first; the pills below are then placed against what is
	// left above y=443, so the stamp lands in a gap.
	UI_DrawProportionalString( 158, OV_TOP + 6, computer ? "THE COMPUTER  -  ASK IT ANYTHING" : "THE LINE BEING ANSWERED",
		UI_TINYFONT, colorTable[CT_LTORANGE] );
	int answerLines = 0;
	if ( computer )
	{
		// The transcript wraps to its column; its budget is what is left before the pills' floor and
		// the version stamp's clear strip (y=441), so a wrapped line never runs into the pills' space.
		const int rows = OptionCount() + 1;
		int maxLines = ( 441 - ( rows * 9 + 6 ) - ( OV_TOP + 18 ) ) / 8;
		if ( maxLines < 1 ) maxLines = 1;
		answerLines = DrawTranscript( 158, 632, OV_TOP + 18, 8, maxLines );
	}
	else if ( L.ok ) answerLines = DrawWrapped( 158, OV_TOP + 18, 632, 11, UI_SMALLFONT, colorTable[CT_WHITE], L.text, 2 );

	if ( !computer && meet.mode == MODE_DONE )
	{
		char outcome[128];
		ui.Cvar_VariableStringBuffer( va( "lwh_ship_meeting_opt%d", meet.playOutcome ), outcome, sizeof( outcome ) );
		char *bar = strchr( outcome, '|' );
		if ( bar ) *bar = 0;
		DrawWrapped( 158, OV_TOP + 50, 632, 9, UI_TINYFONT, colorTable[CT_LTBLUE2],
			va( "DECIDED: %s  -  the simulation applied it; the log records it.", outcome ), 2 );
	}
	else
	{
		// The pills: each the option's label and the short description the design asks for (its
		// cost), each wrapped by measurement to its column and its row grown only if the text needs
		// it. The block is packed above the version stamp's clear strip: it starts just below the
		// answer and compresses its line height if the outcomes and the free-text pill would reach
		// y=443. Task A lifts the row ~16 units from where M3 drew it, which is what moves the
		// free-text pill clear of the stamp (a fixed 16px pitch with four outcomes put it at y=438,
		// inside the stamp at y=445).
		const int answerPitch = computer ? 8 : 11;   // the transcript is tiny font; a meeting line is small
		const int pillTop = OV_TOP + 18 + ( answerLines > 0 ? answerLines : 1 ) * answerPitch + 6;
		DrawPillBlock( pillTop, options, false, meet.mode == MODE_CHOOSING, meet.cursor );
	}

	// The room itself: the brief names the place it is held in, and who is present, with the scope
	// each one's brief was built from -- a post reads its own scope, command reads all
	// (docs/the-record-and-the-log.md). Naming the place is the owner's ruling (docs/staff-meetings.md):
	// the screen stops saying "IN THE ROOM" about a room that does not exist.
	{
		char roomCaps[64], line[512];
		RoomCaps( roomCaps, sizeof( roomCaps ) );
		Com_sprintf( line, sizeof( line ), "IN %s:", roomCaps );
		const int n = static_cast<int>( ui.Cvar_VariableValue( "lwh_ship_meeting_parts" ) );
		for ( int i = 0; i < n && i < 6; ++i )
		{
			char buf[192], *f[5];
			ui.Cvar_VariableStringBuffer( va( "lwh_ship_meeting_part%d", i ), buf, sizeof( buf ) );
			if ( Split( buf, '|', f, 5 ) < 5 ) continue;
			Q_strcat( line, sizeof( line ), va( "%s%s (%s)", i ? "   " : "  ", f[0], f[4] ) );
		}
		// The participants line is wrapped to its column like everything else. It shares the bottom
		// strip with the free-text pill and the engine's stamp; see the evidence for the call.
		DrawWrapped( 158, 458, 632, 8, UI_TINYFONT, colorTable[CT_LTBLUE2], line, 2 );
	}
	// Task A: meet.said is drawn in the speaker rail's lower strip (x < 150), not at y=446 in the
	// free-text row. The rail is entirely left of the version stamp's box (x>=371), so the seam's
	// answer can never be read as the stamp's contents whatever its length. Named as a call.
	// The host records what was said (ship meeting say / ship computer say) and publishes it; the room
	// draws that, so the same screen shows a meeting's seam answer and the computer's refusal. The
	// local buffer is only the fallback for what the screen itself typed before the host answered.
	{
		char hostSaid[320] = "";
		ui.Cvar_VariableStringBuffer( "lwh_ship_meeting_said", hostSaid, sizeof( hostSaid ) );
		const char *said = ( !meet.typing && hostSaid[0] ) ? hostSaid : meet.said;
		if ( said[0] ) DrawWrapped( 6, OV_TOP + 98, 148, 8, UI_TINYFONT, colorTable[CT_LTORANGE], said, 5 );
	}

	UI_DrawProportionalString( 158, 470, meet.typing
		? "type   ENTER submit   ESC cancel"
		: "UP/DOWN  number  ENTER speak  T type an answer  R replay  ESC queue",
		UI_TINYFONT, colorTable[CT_LTPURPLE1] );
}

// ---- the order face (docs/orders.md) ------------------------------------------------------------
//
// The same machinery as a room, for one person: the recipient's rail, the situation, the orders
// they can be given as pills with their costs, and a free-text pill. Once one is given, the frame
// shows the order being given and the recipient's answer. The screen sends `ship order` commands;
// the simulation dispatches them.

int OrderOptions( void ) { return static_cast<int>( ui.Cvar_VariableValue( "lwh_ship_order_options" ) ); }
int OrderReplies( void ) { return static_cast<int>( ui.Cvar_VariableValue( "lwh_ship_order_replies" ) ); }

Line RecipientRail( void )
{
	Line L;
	memset( &L, 0, sizeof( L ) );
	ui.Cvar_VariableStringBuffer( "lwh_ship_order_recipient", L.name, sizeof( L.name ) );
	ui.Cvar_VariableStringBuffer( "lwh_ship_order_post", L.post, sizeof( L.post ) );
	ui.Cvar_VariableStringBuffer( "lwh_ship_order_watch", L.watch, sizeof( L.watch ) );
	ui.Cvar_VariableStringBuffer( "lwh_ship_order_mood", L.mood, sizeof( L.mood ) );
	L.ok = L.name[0] != 0;
	return L;
}

// One reply line as the host published it: rail(name|post|watch|mood)|text|delivery.
Line ReadOrderReply( int i )
{
	Line L;
	memset( &L, 0, sizeof( L ) );
	char buf[384];
	ui.Cvar_VariableStringBuffer( va( "lwh_ship_order_reply%d", i ), buf, sizeof( buf ) );
	char *f[7];
	const int n = Split( buf, '|', f, 7 );
	if ( n < 6 ) return L;
	Q_strncpyz( L.name, f[0], sizeof( L.name ) );
	Q_strncpyz( L.post, f[1], sizeof( L.post ) );
	Q_strncpyz( L.watch, f[2], sizeof( L.watch ) );
	Q_strncpyz( L.mood, f[3], sizeof( L.mood ) );
	Q_strncpyz( L.text, f[4], sizeof( L.text ) );
	Q_strncpyz( L.delivery, f[5], sizeof( L.delivery ) );
	L.ok = true;
	return L;
}

void OrderDraw( void )
{
	const int options = OrderOptions();
	const int replies = OrderReplies();
	char to[64];
	ui.Cvar_VariableStringBuffer( "lwh_ship_order_recipient", to, sizeof( to ) );
	UI_FillRect( 150, OV_TOP, 490, 480 - OV_TOP, colorTable[CT_BLACK] );
	UI_FillRect( 0, OV_TOP, 640, 4, colorTable[CT_LTGOLD1] );
	DrawRail( RecipientRail() );
	UI_DrawProportionalString( 158, OV_TOP + 6, va( "ORDERS  -  TO %s", to[0] ? to : "NOBODY" ),
		UI_SMALLFONT, colorTable[CT_LTGOLD1] );
	char situation[256];
	ui.Cvar_VariableStringBuffer( "lwh_ship_order_situation", situation, sizeof( situation ) );
	int sitLines = situation[0] ? DrawWrapped( 158, OV_TOP + 20, 632, 9, UI_TINYFONT, colorTable[CT_LTBLUE2], situation, 2 ) : 0;

	if ( replies > 0 )
	{// the order has been given: show the giver's line and the recipient's answer
		int y = OV_TOP + 20 + ( sitLines > 0 ? sitLines : 1 ) * 10 + 2;
		for ( int i = 0; i < replies && i < 4; ++i )
		{
			const Line R = ReadOrderReply( i );
			if ( !R.ok ) continue;
			UI_DrawProportionalString( 158, y, i == 0 ? "THE ORDER" : "THE ANSWER", UI_TINYFONT, colorTable[CT_LTORANGE] );
			y += 10;
			// The line or the answer wraps to its column like every other; the y advances by what it took.
			y += DrawWrapped( 158, y, 632, 8, UI_TINYFONT,
				colorTable[i == 0 ? CT_LTGOLD1 : CT_WHITE], R.text, 2 ) * 8 + 5;
		}
		char note[256];
		ui.Cvar_VariableStringBuffer( "lwh_ship_order_note", note, sizeof( note ) );
		if ( note[0] ) DrawWrapped( 158, 452, 632, 9, UI_TINYFONT, colorTable[CT_LTBLUE2], note, 2 );
	}
	else
	{// the orders this person can be given, each with what it costs, and the free-text pill
		const int pillTop = OV_TOP + 20 + ( sitLines > 0 ? sitLines : 1 ) * 10 + 4;
		DrawPillBlock( pillTop, options, true, true, meet.cursor );
	}
	{
		char hostSaid[320] = "";
		ui.Cvar_VariableStringBuffer( "lwh_ship_order_said", hostSaid, sizeof( hostSaid ) );
		const char *said = ( !meet.typing && hostSaid[0] ) ? hostSaid : meet.said;
		if ( said[0] ) DrawWrapped( 6, OV_TOP + 98, 148, 8, UI_TINYFONT, colorTable[CT_LTORANGE], said, 5 );
	}
	UI_DrawProportionalString( 158, 470, meet.typing
		? "type   ENTER submit   ESC cancel"
		: "UP/DOWN  number  ENTER give  T type an order  ESC queue",
		UI_TINYFONT, colorTable[CT_LTPURPLE1] );
}

bool OrderAct( int key )
{
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
		case K_BACKSPACE: if ( meet.bufLen > 0 ) meet.buf[--meet.bufLen] = 0; return true;
		case K_ENTER: case K_KP_ENTER:
			if ( meet.bufLen > 0 ) Send( va( "ship order say \"%s\"", meet.buf ) );
			meet.typing = false;
			return true;
		case K_ESCAPE: meet.typing = false; return true;
		}
		return true;
	}
	const int options = OrderOptions();
	switch ( key )
	{
	case K_UPARROW: if ( options ) meet.cursor = ( meet.cursor + options ) % ( options + 1 ); return true;
	case K_DOWNARROW: if ( options ) meet.cursor = ( meet.cursor + 1 ) % ( options + 1 ); return true;
	case '1': case '2': case '3': case '4': case '5': case '6':
		if ( key - '0' >= 1 && key - '0' <= options )
		{
			meet.cursor = key - '0' - 1;
			Send( va( "ship order give %d", key - '0' ) );
		}
		return true;
	case 't': case 'T': meet.cursor = options; meet.typing = true; meet.bufLen = 0; meet.buf[0] = 0; return true;
	case K_ENTER: case K_KP_ENTER:
		if ( meet.cursor >= options ) { meet.typing = true; meet.bufLen = 0; meet.buf[0] = 0; return true; }
		Send( va( "ship order give %d", meet.cursor + 1 ) );
		return true;
	case K_ESCAPE:
		Send( "ship order close" );
		meet.face = FACE_QUEUE;
		return true;
	}
	return false;
}

void Draw( void )
{
	if ( meet.face == FACE_ORDER ) { OrderDraw(); return; }
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
	if ( meet.face == FACE_ORDER ) return OrderAct( key );
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
		// A number speaks that option at once: select it and play its branch. On the computer it asks
		// at once instead and the room stays open -- the same key, a session rather than one shot.
		if ( meet.mode == MODE_CHOOSING && key - '0' >= 1 && key - '0' <= options )
		{
			meet.cursor = key - '0' - 1;
			if ( ComputerRoom() ) { Send( va( "ship meeting choose %d", meet.cursor + 1 ) ); }
			else
			{
				meet.playOutcome = meet.cursor;
				meet.playLine = 0;
				meet.playNext = ui.Milliseconds() + LineMs( meet.playOutcome, 0 );
				meet.mode = MODE_PLAYING;
			}
		}
		return true;
	case 't': case 'T': meet.cursor = options; meet.typing = true; meet.bufLen = 0; meet.buf[0] = 0; return true;
	case 'r': case 'R':
		if ( meet.mode == MODE_DONE ) { meet.mode = MODE_PLAYING; meet.playLine = 0; meet.playNext = ui.Milliseconds() + 650; }
		return true;
	case K_ENTER: case K_KP_ENTER:
		if ( meet.mode != MODE_CHOOSING ) return true;
		if ( meet.cursor >= options ) { meet.typing = true; meet.bufLen = 0; meet.buf[0] = 0; return true; }
		if ( ComputerRoom() ) { Send( va( "ship meeting choose %d", meet.cursor + 1 ) ); return true; }
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
	// The face is left as it is: the same buffer serves the meeting's free-text pill and the order's.
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

// The wrap self-check that scripts/computer-check.sh runs, in the engine, against the **live** strings
// and the **engine's own font** -- the very metric the frame draws with. It wraps every string the
// overlay is showing into the same column and budget the frame uses, and prints each line's measured
// width against its column. A line wider than its column, or a string that needs more lines than the
// budget, is an OVERFLOW: the check fails and names it. This is the test the frames cannot be -- fit
// proved by the same measurement the frame uses, not by the eye -- and it is the property the wrap is
// built to: for the real strings, including the longest refusals, every line returned fits.
namespace {

static int g_wrapStrings = 0, g_wrapLines = 0, g_wrapOverflow = 0;

static void WrapCheckOne( const char *name, const char *text, int x, int right, int style, int maxLines, bool showLines = false )
{
	if ( !text || !text[0] ) return;
	++g_wrapStrings;
	const LWH_Wrapped w = WrapMeasured( style, text, x, right, maxLines );
	int widest = 0, over = 0;
	for ( int i = 0; i < w.count; ++i )
	{
		const int width = UI_ProportionalStringWidth( w.line[i], style );
		if ( width > widest ) widest = width;
		if ( width > right - x )
		{
			++over;
			ui.Printf( "WRAP: OVERFLOW %s line %d: %d > %d \"%s\"\n", name, i, width, right - x, w.line[i] );
		}
		if ( showLines ) ui.Printf( "WRAP:   | %s\n", w.line[i] );
	}
	if ( w.overflow )
	{
		++over;
		ui.Printf( "WRAP: OVERFLOW %s: needs more than %d line(s) \"%s\"\n", name, maxLines, text );
	}
	g_wrapLines += w.count;
	g_wrapOverflow += over;
	ui.Printf( "WRAP: %-16s col %3d..%3d  %2d line(s), widest %3d%s\n", name, x, right, w.count, widest,
		over ? "  <-- OVERFLOW" : "" );
}

// Every live surface the overlay draws text on. The columns and budgets are the drawing code's own.
static void WrapCheck( void )
{
	g_wrapStrings = g_wrapLines = g_wrapOverflow = 0;

	const int turns = static_cast<int>( ui.Cvar_VariableValue( "lwh_ship_meeting_turns" ) );
	for ( int i = 0; i < turns && i < TRANSCRIPT_TURNS; ++i )
	{
		char name[32];
		Com_sprintf( name, sizeof( name ), "transcript%d", i );
		WrapCheckOne( name, ComposeTranscript( i ), 158, 632, UI_TINYFONT, 4, true );
	}

	char said[320] = "";
	ui.Cvar_VariableStringBuffer( "lwh_ship_meeting_said", said, sizeof( said ) );
	WrapCheckOne( "rail-echo", said, 6, 148, UI_TINYFONT, 5, true );

	const int options = OptionCount();
	for ( int i = 0; i < options && i < 6; ++i )
	{
		char buf[256], *f[4], whole[192], name[32];
		ui.Cvar_VariableStringBuffer( va( "lwh_ship_meeting_opt%d", i ), buf, sizeof( buf ) );
		if ( Split( buf, '|', f, 4 ) < 4 ) continue;
		Com_sprintf( whole, sizeof( whole ), "%d %s", i + 1, f[0] );
		Com_sprintf( name, sizeof( name ), "pill%d-label", i + 1 );
		WrapCheckOne( name, whole, 172, 350, UI_TINYFONT, 2 );
		Com_sprintf( name, sizeof( name ), "pill%d-cost", i + 1 );
		WrapCheckOne( name, f[1], 356, 632, UI_TINYFONT, 2 );
	}

	const int q = QueueCount();
	for ( int i = 0; i < q && i < 5; ++i )
	{
		char buf[256], *f[5], name[32];
		ui.Cvar_VariableStringBuffer( va( "lwh_ship_meeting_q%d", i ), buf, sizeof( buf ) );
		if ( Split( buf, '|', f, 5 ) < 5 ) continue;
		Com_sprintf( name, sizeof( name ), "queue%d", i );
		WrapCheckOne( name, f[1], 150, 465, UI_TINYFONT, 2 );
	}

	const Line L = ReadLine( 0, 0 );
	if ( L.ok )
	{
		WrapCheckOne( "meeting-line", L.text, 158, 632, UI_SMALLFONT, 2 );
		WrapCheckOne( "rail-name", L.name, 8, 128, UI_SMALLFONT, 1 );
		WrapCheckOne( "rail-post", L.post, 8, 128, UI_TINYFONT, 1 );
		WrapCheckOne( "rail-mood", L.mood, 34, 118, UI_TINYFONT, 2 );
	}

	ui.Printf( "WRAP: checked %d string(s), %d line(s), overflows %d\n", g_wrapStrings, g_wrapLines, g_wrapOverflow );
}

} // namespace

// Our meeting screens, called from LWH_UI_ConsoleCommand. `lwh_meet_key` drives the screen by name,
// so what a test presses is exactly what a hand presses.
qboolean LWH_UI_MeetingScreens( const char *cmd )
{
	if ( !Q_stricmp( cmd, "ui_lwh_meeting" ) ) { Open( FACE_QUEUE ); return qtrue; }
	if ( !Q_stricmp( cmd, "ui_lwh_meeting_room" ) ) { Open( FACE_ROOM ); return qtrue; }
	if ( !Q_stricmp( cmd, "ui_lwh_orders" ) ) { Open( FACE_ORDER ); return qtrue; }
	if ( !Q_stricmp( cmd, "lwh_order_key" ) )
	{
		char arg[32];
		ui.Argv( 1, arg, sizeof( arg ) );
		Act( KeyByName( arg ) );
		return qtrue;
	}
	if ( !Q_stricmp( cmd, "lwh_order_type" ) )
	{//a test types what a hand types, into the same free-text buffer
		char text[128];
		ui.Argv( 1, text, sizeof( text ) );
		TypeText( text );
		return qtrue;
	}
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
	if ( !Q_stricmp( cmd, "lwh_wrap_check" ) )
	{//the measured fit of every live string, in the engine's own font (scripts/computer-check.sh)
		WrapCheck();
		return qtrue;
	}
	return qfalse;
}
