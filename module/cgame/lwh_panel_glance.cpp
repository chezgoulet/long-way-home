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
			vec3_t c;
			for ( int a = 0; a < 3; ++a ) c[a] = ( e->absmin[a] + e->absmax[a] ) * 0.5f;
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
	static const int ALERT_COLOR[3] = { CT_LTBLUE1, CT_YELLOW, CT_RED };
	const int alert = s->alert >= 0 && s->alert < 3 ? s->alert : 0;
	const int sod = s->SecondOfDay();
	char l1[64], l2[80], l3[80];
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

	// A small LCARS block sitting above the panel.
	const float w = 234.0f, h = 46.0f;
	const float bx = static_cast<float>( x ) - w * 0.5f, by = static_cast<float>( y ) - h;
	CG_FillRect( bx, by, w, h, colorTable[CT_BLACK] );
	CG_FillRect( bx, by, w, 3.0f, colorTable[ALERT_COLOR[alert]] );
	CG_DrawProportionalString( static_cast<int>( bx ) + 5, static_cast<int>( by ) + 4, l1, CG_SMALLFONT, colorTable[ALERT_COLOR[alert]] );
	CG_DrawProportionalString( static_cast<int>( bx ) + 5, static_cast<int>( by ) + 17, l2, CG_SMALLFONT, colorTable[CT_LTGOLD1] );
	CG_DrawProportionalString( static_cast<int>( bx ) + 5, static_cast<int>( by ) + 30, l3, CG_SMALLFONT, colorTable[CT_LTBLUE1] );
}
