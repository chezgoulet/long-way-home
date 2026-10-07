// lwh_panel_glance.cpp -- S4's "glance": the ship's live state at the panel the player is looking
// at, drawn in the world rather than on a full-screen console.
//
// This is the first of the two halves the owner asked for ("both"): a compact readout anchored to
// the panel, before the engine paints live state onto the surface itself. It lives in the game
// module's cgame half because only cgame draws the HUD. The engine patch that calls it carries a
// one-line seam (cgame/lwh_cgame_hooks.h) and no logic; everything here is ours.
//
// With the ship off (Ship_Get() == NULL) the function returns without drawing, so retail play is
// untouched.

#include "cg_local.h"

#include "g_ship.h"
#include "ship_core.h"

// The panel is whatever the player's own trace is on: a station console or interface the map made
// usable. The readout is built from the live ship, never from a snapshot of it.
void LWH_CG_DrawPanelGlance( void )
{
	ship::Ship *s = Ship_Get();
	if ( !s || !cg.snap ) return;
	if ( cg.levelShot || cg.snap->ps.stats[STAT_HEALTH] <= 0 ) return;

	// S9's "see and hear": a hit shakes the screen -- a red edge flash for half a second after each
	// hit the ship takes lands. The ship counts its hits (s->hits); cgame sees the count change here.
	{
		static uint32_t lastHits = 0;
		static int hitFlashAt = 0;
		if ( s->hits != lastHits ) { lastHits = s->hits; hitFlashAt = cg.time; }
		const int since = cg.time - hitFlashAt;
		if ( hitFlashAt && since >= 0 && since < 500 )
		{
			const float w = 44.0f * ( 1.0f - since / 500.0f );
			CG_FillRect( 0, 0, 640, w, colorTable[CT_RED] );
			CG_FillRect( 0, 480 - w, 640, w, colorTable[CT_RED] );
			CG_FillRect( 0, 0, w, 480, colorTable[CT_RED] );
			CG_FillRect( 640 - w, 0, w, 480, colorTable[CT_RED] );
		}
	}

	// The panel: what the crosshair is on, or failing that the nearest usable within a pace -- a
	// console is read standing at it, not by aiming at a pixel.
	gentity_t *panel = NULL;
	trace_t tr;
	vec3_t start, end;
	VectorCopy( cg.refdef.vieworg, start );
	VectorMA( start, 256.0f, cg.refdef.viewaxis[0], end );
	CG_Trace( &tr, start, vec3_origin, vec3_origin, end, cg.snap->ps.clientNum,
		MASK_OPAQUE | CONTENTS_BODY | CONTENTS_ITEM | CONTENTS_CORPSE );
	if ( tr.entityNum > 0 && tr.entityNum < ENTITYNUM_WORLD )
	{
		gentity_t *h = &g_entities[tr.entityNum];
		if ( h->inuse && h->classname
			&& ( !Q_stricmp( h->classname, "func_usable" ) || !Q_stricmp( h->classname, "target_interface" ) ) )
			panel = h;
	}
	if ( !panel )
	{
		float best = 96.0f * 96.0f;
		for ( int i = 1; i < MAX_GENTITIES; ++i )
		{
			gentity_t *e = &g_entities[i];
			if ( !e->inuse || !e->classname || Q_stricmp( e->classname, "func_usable" ) ) continue;
			vec3_t c, to;
			for ( int a = 0; a < 3; ++a ) c[a] = ( e->absmin[a] + e->absmax[a] ) * 0.5f;
			// Only a panel the player faces: a panel behind the player would throw the readout
			// off the back of the view and then never reach the screen.
			VectorSubtract( c, cg.refdef.vieworg, to );
			if ( DotProduct( to, cg.refdef.viewaxis[0] ) <= 1.0f ) continue;
			const float d = DistanceSquared( cg.refdef.vieworg, c );
			if ( d < best ) { best = d; panel = e; }
		}
	}
	if ( !panel || !panel->inuse || !panel->classname ) return;
	{
		static int lastPanel = -1;
		const int idx = static_cast<int>( panel - g_entities );
		if ( idx != lastPanel )
		{
			lastPanel = idx;
			CG_Printf( "LWH: ship glance at %s (%s)\n", panel->targetname ? panel->targetname : "?", panel->classname );
		}
	}

	// Project the top-centre of the panel onto the virtual 640x480 screen the HUD draws in.
	vec3_t at, local;
	at[0] = ( panel->absmin[0] + panel->absmax[0] ) * 0.5f;
	at[1] = ( panel->absmin[1] + panel->absmax[1] ) * 0.5f;
	at[2] = panel->absmax[2] + 6.0f;
	VectorSubtract( at, cg.refdef.vieworg, local );
	const float fwd = DotProduct( local, cg.refdef.viewaxis[0] );
	if ( fwd < 1.0f ) return; // behind the player
	const float right = DotProduct( local, cg.refdef.viewaxis[1] );
	const float up = DotProduct( local, cg.refdef.viewaxis[2] );
	const int x = (int)( 320.0f + ( 320.0f / fwd ) * ( 90.0f / cg.refdef.fov_x ) * right );
	const int y = (int)( 240.0f - ( 240.0f / fwd ) * ( 90.0f / cg.refdef.fov_y ) * up );

	// The live state, three short lines and an alarm when there is one.
	static const char *const ALERTS[3] = { "GREEN", "YELLOW", "RED" };
	static const int ALERT_COLOR[3] = { CT_LTBLUE2, CT_YELLOW, CT_RED };
	const int alert = s->alert >= 0 && s->alert < 3 ? s->alert : 0;
	const int sod = s->SecondOfDay();
	char l1[64], l2[80], l3[80], l4[80];
	Com_sprintf( l1, sizeof( l1 ), "DAY %d  %02d:%02d  CONDITION %s", s->Day(), sod / 3600, sod % 3600 / 60, ALERTS[alert] );
	Com_sprintf( l2, sizeof( l2 ), "POWER %d/%d   CREW FIT %d/%d", s->PowerAvailable(), s->PowerAllocated(),
		s->CrewFit(), static_cast<int>( s->crew.size() ) );
	int hurt = 0;
	for ( int i = 0; i < ship::SYS_COUNT; ++i )
		if ( s->systems[i].health < 0.999f ) ++hurt;
	int boarders = 0, assimilated = 0;
	for ( int d = 0; d < ship::DECKS; ++d )
	{
		if ( s->decks[d].intruders > 0.0f ) boarders += static_cast<int>( s->decks[d].intruders + 0.999f );
		if ( s->decks[d].assimilated > 0.0f ) assimilated = 1;
	}
	if ( boarders || assimilated )
		Com_sprintf( l3, sizeof( l3 ), "%s%s%s", boarders ? va( "BOARDERS %d  ", boarders ) : "",
			assimilated ? "BORG ABOARD  " : "", hurt ? va( "%d SYSTEMS DAMAGED", hurt ) : "" );
	else
		Com_sprintf( l3, sizeof( l3 ), "%sTORPEDOES %d", hurt ? va( "%d SYSTEMS DAMAGED   ", hurt ) : "", s->stores.torpedoes );

	// The navigation counter (docs/navigation-counter.md): the crew's shared fact, on the panel the
	// player is standing at, so it is visible without entering a special mode.
	{
		const ship::Navigation nav = ship::NavigationCounter( *s );
		if ( !nav.warp )
			Com_sprintf( l4, sizeof( l4 ), "NO WARP: HOME STOPS GETTING CLOSER; %d LY OUT", static_cast<int>( nav.distanceLy + 0.5f ) );
		else
			Com_sprintf( l4, sizeof( l4 ), "HOME %d LY   %d YR NOM   %d YR NOW", static_cast<int>( nav.distanceLy + 0.5f ),
				static_cast<int>( nav.nominalYears + 0.5f ), static_cast<int>( nav.currentYears + 0.5f ) );
	}

	// A small LCARS block sitting above the panel.
	const float w = 234.0f, h = 59.0f;
	const float bx = static_cast<float>( x ) - w * 0.5f, by = static_cast<float>( y ) - h;
	CG_FillRect( bx, by, w, h, colorTable[CT_BLACK] );
	CG_FillRect( bx, by, w, 3.0f, colorTable[ALERT_COLOR[alert]] );
	CG_DrawProportionalString( static_cast<int>( bx ) + 5, static_cast<int>( by ) + 4, l1, CG_SMALLFONT, colorTable[ALERT_COLOR[alert]] );
	CG_DrawProportionalString( static_cast<int>( bx ) + 5, static_cast<int>( by ) + 17, l2, CG_SMALLFONT, colorTable[CT_LTGOLD1] );
	CG_DrawProportionalString( static_cast<int>( bx ) + 5, static_cast<int>( by ) + 30, l3, CG_SMALLFONT, colorTable[CT_LTBLUE2] );
	CG_DrawProportionalString( static_cast<int>( bx ) + 5, static_cast<int>( by ) + 43, l4, CG_SMALLFONT, colorTable[CT_LTBLUE2] );
	// The viewscreen, when there is a contact: a live image of it beside the panel, redrawn each
	// frame from the same state the Tactical console reads -- a schematic of the ship scaled and
	// coloured by its hull, wrapped in a shield bubble that fades as its shields fall. Bars below
	// carry the exact fractions, as the consoles do.
	if ( s->enemy.present && s->enemy.hull > 0.0f )
	{
		const float vw = 168.0f, vh = 64.0f;
		const float vx = bx + w + 6.0f, vy = by - 18.0f;
		CG_FillRect( vx, vy, vw, vh, colorTable[CT_BLACK] );
		CG_FillRect( vx, vy, vw, 3.0f, colorTable[CT_RED] );
		CG_FillRect( vx, vy, 3.0f, vh, colorTable[CT_RED] );
		char lc[64];
		Com_sprintf( lc, sizeof( lc ), "VIEWSCREEN  TGT %s", ship::EnemySubsystemName( s->target ) );
		CG_DrawProportionalString( static_cast<int>( vx ) + 6, static_cast<int>( vy ) + 5, lc, CG_SMALLFONT, colorTable[CT_RED] );

		// The image itself: the contact, centred, scaled to its remaining hull.
		const float cx = vx + vw * 0.5f, cy = vy + 30.0f;
		const float scale = 0.6f + 0.4f * s->enemy.hull;
		const int hullCol = s->enemy.hull > 0.6f ? CT_LTGOLD1 : s->enemy.hull > 0.25f ? CT_LTORANGE : CT_RED;
		CG_FillRect( cx - 26.0f * scale, cy - 11.0f * scale, 52.0f * scale, 5.0f * scale, colorTable[hullCol] ); // saucer
		CG_FillRect( cx - 7.0f * scale,  cy - 6.0f * scale,  14.0f * scale, 15.0f * scale, colorTable[hullCol] ); // hull
		CG_FillRect( cx - 23.0f * scale, cy - 15.0f * scale, 4.0f * scale, 11.0f * scale, colorTable[hullCol] ); // nacelle
		CG_FillRect( cx + 19.0f * scale, cy - 15.0f * scale, 4.0f * scale, 11.0f * scale, colorTable[hullCol] ); // nacelle
		// The shield bubble: an ellipse whose colour dims with the shields, drawn row by row.
		if ( s->enemy.shields > 0.02f )
		{
			const int sc = s->enemy.shields > 0.5f ? CT_LTBLUE2 : CT_LTPURPLE1;
			for ( int dy = -12; dy <= 12; ++dy )
			{
				const float t = static_cast<float>( dy ) / 12.0f;
				const float halfW = 42.0f * sqrtf( 1.0f - t * t );
				CG_FillRect( cx - halfW, cy + static_cast<float>( dy ) * 1.1f, halfW * 2.0f, 1.0f, colorTable[sc] );
			}
		}
		// The exact fractions, under the image.
		const float bw = vw - 12.0f;
		CG_FillRect( vx + 6.0f, vy + 52.0f, bw, 4.0f, colorTable[CT_DKPURPLE3] );
		CG_FillRect( vx + 6.0f, vy + 52.0f, bw * s->enemy.hull, 4.0f, colorTable[CT_LTGOLD1] );
		CG_FillRect( vx + 6.0f, vy + 58.0f, bw, 4.0f, colorTable[CT_DKPURPLE3] );
		CG_FillRect( vx + 6.0f, vy + 58.0f, bw * s->enemy.shields, 4.0f, colorTable[CT_LTBLUE2] );

		// Evidence: once a second, say what the live image is showing.
		static int lastLog = 0;
		if ( cg.time - lastLog > 1000 )
		{
			lastLog = cg.time;
			CG_Printf( "LWH: viewscreen %s hull %d%% shields %d%%\n", ship::EnemySubsystemName( s->target ),
				static_cast<int>( s->enemy.hull * 100.0f + 0.5f ), static_cast<int>( s->enemy.shields * 100.0f + 0.5f ) );
		}
	}
}
