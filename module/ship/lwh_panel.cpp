// lwh_panel.cpp -- the status panel's live surface (S4's "glance", second half).
//
// The ship's state is drawn here, in the game module, into a small 32-bit RGBA image and handed to
// the renderer (gi.UpdatePanelImage), which overwrites the image the panel shader uses. The engine
// carries only that capability; what is drawn, and when, is ours. Inert unless the ship runs.

#include "g_local.h"

#include "ship_core.h"
#include "lwh_panel.h"

#include <cstdio>
#include <cstring>

namespace {

const int PANEL_W = 128, PANEL_H = 128; // square: the screen face is square, so one tile fills it
const int LINE_MS = 200;      // the panel is redrawn this often, not every frame

unsigned char panel[PANEL_W * PANEL_H * 4];
bool remapped = false;
int nextDraw = 0;

// ---- a 5x7 font, column-major, bit 0 at the top. Only the characters a status line needs. ----
struct Glyph5 { unsigned char c; unsigned char b[5]; };
const Glyph5 FONT[] = {
	{ ' ', { 0x00, 0x00, 0x00, 0x00, 0x00 } },
	{ '-', { 0x08, 0x08, 0x08, 0x08, 0x08 } },
	{ '.', { 0x00, 0x60, 0x60, 0x00, 0x00 } },
	{ ':', { 0x00, 0x36, 0x36, 0x00, 0x00 } },
	{ '/', { 0x20, 0x10, 0x08, 0x04, 0x02 } },
	{ '%', { 0x63, 0x13, 0x08, 0x64, 0x63 } },
	{ '0', { 0x3E, 0x51, 0x49, 0x45, 0x3E } },
	{ '1', { 0x00, 0x42, 0x7F, 0x40, 0x00 } },
	{ '2', { 0x42, 0x61, 0x51, 0x49, 0x46 } },
	{ '3', { 0x21, 0x41, 0x45, 0x4B, 0x31 } },
	{ '4', { 0x18, 0x14, 0x12, 0x7F, 0x10 } },
	{ '5', { 0x27, 0x45, 0x45, 0x45, 0x39 } },
	{ '6', { 0x3C, 0x4A, 0x49, 0x49, 0x30 } },
	{ '7', { 0x01, 0x71, 0x09, 0x05, 0x03 } },
	{ '8', { 0x36, 0x49, 0x49, 0x49, 0x36 } },
	{ '9', { 0x06, 0x49, 0x49, 0x29, 0x1E } },
	{ 'A', { 0x7E, 0x11, 0x11, 0x11, 0x7E } },
	{ 'B', { 0x7F, 0x49, 0x49, 0x49, 0x36 } },
	{ 'C', { 0x3E, 0x41, 0x41, 0x41, 0x22 } },
	{ 'D', { 0x7F, 0x41, 0x41, 0x22, 0x1C } },
	{ 'E', { 0x7F, 0x49, 0x49, 0x49, 0x41 } },
	{ 'F', { 0x7F, 0x09, 0x09, 0x09, 0x01 } },
	{ 'G', { 0x3E, 0x41, 0x49, 0x49, 0x7A } },
	{ 'H', { 0x7F, 0x08, 0x08, 0x08, 0x7F } },
	{ 'I', { 0x00, 0x41, 0x7F, 0x41, 0x00 } },
	{ 'J', { 0x20, 0x40, 0x41, 0x3F, 0x01 } },
	{ 'K', { 0x7F, 0x08, 0x14, 0x22, 0x41 } },
	{ 'L', { 0x7F, 0x40, 0x40, 0x40, 0x40 } },
	{ 'M', { 0x7F, 0x02, 0x0C, 0x02, 0x7F } },
	{ 'N', { 0x7F, 0x04, 0x08, 0x10, 0x7F } },
	{ 'O', { 0x3E, 0x41, 0x41, 0x41, 0x3E } },
	{ 'P', { 0x7F, 0x09, 0x09, 0x09, 0x06 } },
	{ 'Q', { 0x3E, 0x41, 0x51, 0x21, 0x5E } },
	{ 'R', { 0x7F, 0x09, 0x19, 0x29, 0x46 } },
	{ 'S', { 0x46, 0x49, 0x49, 0x49, 0x31 } },
	{ 'T', { 0x01, 0x01, 0x7F, 0x01, 0x01 } },
	{ 'U', { 0x3F, 0x40, 0x40, 0x40, 0x3F } },
	{ 'V', { 0x1F, 0x20, 0x40, 0x20, 0x1F } },
	{ 'W', { 0x3F, 0x40, 0x38, 0x40, 0x3F } },
	{ 'X', { 0x63, 0x14, 0x08, 0x14, 0x63 } },
	{ 'Y', { 0x07, 0x08, 0x70, 0x08, 0x07 } },
	{ 'Z', { 0x61, 0x51, 0x49, 0x45, 0x43 } },
};

const unsigned char *Glyph( unsigned char c )
{
	static const unsigned char blank[5] = { 0, 0, 0, 0, 0 };
	c = static_cast<unsigned char>( toupper( c ) );
	for ( size_t i = 0; i < sizeof( FONT ) / sizeof( FONT[0] ); ++i )
		if ( FONT[i].c == c ) return FONT[i].b;
	return blank;
}

const float BLACK[4]  = { 0.02f, 0.02f, 0.05f, 1.0f };
const float GOLD[4]   = { 0.95f, 0.65f, 0.15f, 1.0f };
const float BLUE[4]   = { 0.45f, 0.65f, 1.00f, 1.0f };
const float GREEN[4]  = { 0.30f, 0.90f, 0.40f, 1.0f };
const float YELLOW[4] = { 0.95f, 0.85f, 0.20f, 1.0f };
const float RED[4]    = { 1.00f, 0.20f, 0.15f, 1.0f };
const float DIM[4]    = { 0.16f, 0.18f, 0.30f, 1.0f };

void Put( int x, int y, const float *col )
{
	if ( x < 0 || y < 0 || x >= PANEL_W || y >= PANEL_H ) return;
	unsigned char *p = panel + ( y * PANEL_W + x ) * 4;
	p[0] = static_cast<unsigned char>( col[0] * 255 );
	p[1] = static_cast<unsigned char>( col[1] * 255 );
	p[2] = static_cast<unsigned char>( col[2] * 255 );
	p[3] = 255;
}

void Fill( int x, int y, int w, int h, const float *col )
{
	for ( int j = 0; j < h; ++j )
		for ( int i = 0; i < w; ++i )
			Put( x + i, y + j, col );
}

void Text( int x, int y, const char *s, const float *col )
{
	for ( ; *s; ++s )
	{
		const unsigned char *g = Glyph( static_cast<unsigned char>( *s ) );
		for ( int cx = 0; cx < 5; ++cx )
			for ( int ry = 0; ry < 7; ++ry )
				if ( g[cx] & ( 1 << ry ) ) Put( x + cx, y + ry, col );
		x += 6;
	}
}

// The whole picture, from the live ship. Four short lines and a power bar.
void Render( const ship::Ship *s )
{
	static const char *const WATCH[] = { "ALPHA", "BETA", "GAMMA" };
	static const char *const ALERTS[] = { "GREEN", "YELLOW", "RED" };
	static const float *const ALERT_COL[] = { GREEN, YELLOW, RED };
	const int alert = s->alert >= 0 && s->alert < 3 ? s->alert : 0;
	const int sod = s->SecondOfDay();

	for ( int i = 0; i < PANEL_W * PANEL_H; ++i )
	{
		panel[i * 4 + 0] = static_cast<unsigned char>( BLACK[0] * 255 );
		panel[i * 4 + 1] = static_cast<unsigned char>( BLACK[1] * 255 );
		panel[i * 4 + 2] = static_cast<unsigned char>( BLACK[2] * 255 );
		panel[i * 4 + 3] = 255;
	}
	Fill( 0, 0, PANEL_W, 12, ALERT_COL[alert] );      // the alert band
	Fill( 0, 114, PANEL_W, 1, DIM );

	char line[64];
	Com_sprintf( line, sizeof( line ), "DAY %d  %02d:%02d  %s", s->Day(), sod / 3600, sod % 3600 / 60,
		WATCH[s->Watch() >= 0 && s->Watch() < 3 ? s->Watch() : 0] );
	Text( 6, 22, line, GOLD );
	Com_sprintf( line, sizeof( line ), "CONDITION %s", ALERTS[alert] );
	Text( 6, 46, line, ALERT_COL[alert] );
	Com_sprintf( line, sizeof( line ), "POWER %d/%d  CREW %d/%d", s->PowerAvailable(), s->PowerAllocated(),
		s->CrewFit(), static_cast<int>( s->crew.size() ) );
	Text( 6, 70, line, BLUE );

	int boarders = 0, assimilated = 0, hurt = 0;
	for ( int d = 0; d < ship::DECKS; ++d )
	{
		if ( s->decks[d].intruders > 0.0f ) boarders += static_cast<int>( s->decks[d].intruders + 0.999f );
		if ( s->decks[d].assimilated > 0.0f ) assimilated = 1;
	}
	for ( int i = 0; i < ship::SYS_COUNT; ++i )
		if ( s->systems[i].health < 0.999f ) ++hurt;
	if ( boarders || assimilated )
		Com_sprintf( line, sizeof( line ), "%s%s", boarders ? va( "BOARDERS %d ", boarders ) : "",
			assimilated ? "BORG ABOARD" : "" );
	else if ( hurt )
		Com_sprintf( line, sizeof( line ), "DAMAGE %d SYSTEMS  TORP %d", hurt, s->stores.torpedoes );
	else
		Com_sprintf( line, sizeof( line ), "ALL SYSTEMS NOMINAL  TORP %d", s->stores.torpedoes );
	Text( 6, 94, line, ( boarders || assimilated ) ? RED : GOLD );

	const int available = s->PowerAvailable() > 0 ? s->PowerAvailable() : 1;
	int bar = s->PowerAllocated() * ( PANEL_W - 4 ) / available;
	if ( bar > PANEL_W - 4 ) bar = PANEL_W - 4;
	if ( bar < 0 ) bar = 0;
	Fill( 2, 118, PANEL_W - 4, 6, DIM );
	Fill( 2, 118, bar, 6, BLUE );

	// The screen's viewed face maps this image mirrored, so flip it to read left to right.
	for ( int y = 0; y < PANEL_H; ++y )
		for ( int x = 0; x < PANEL_W / 2; ++x )
		{
			unsigned char *a = panel + ( y * PANEL_W + x ) * 4;
			unsigned char *b = panel + ( y * PANEL_W + ( PANEL_W - 1 - x ) ) * 4;
			for ( int c = 0; c < 4; ++c ) { unsigned char t = a[c]; a[c] = b[c]; b[c] = t; }
		}
}

} // namespace

void LWH_Panel_Frame( const ship::Ship *s )
{
	if ( !s ) return;
	cvar_t *on = gi.cvar( "g_shipPanel", "1", 0 );
	if ( !on || !on->integer ) return;

	if ( !remapped )
	{// point the ship's console panels at the live shader once. Names that are not present are
	  // harmlessly ignored by the renderer.
		static const char *const CONSOLES[] = { "textures/transporter/panel", "textures/sickbay/panel12", "textures/engineering/conpanel" };
		for ( size_t i = 0; i < sizeof( CONSOLES ) / sizeof( CONSOLES[0] ); ++i )
			gi.RemapShader( CONSOLES[i], "textures/lwh/panel", "0" );
		remapped = true;
	}

	if ( level.time < nextDraw ) return;
	nextDraw = level.time + LINE_MS;
	Render( s );
	gi.UpdatePanelImage( "textures/lwh/panel", panel, PANEL_W, PANEL_H );
}
