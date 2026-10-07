// g_scope.cpp -- see g_scope.h.

#include "g_local.h"

#include "g_scope.h"

#include <cmath>
#include <cstdio>

namespace {

// The stitcher's geometry (tools/shipmap/stitch.py): deck n lies (n - 1) * pitch below deck 1,
// whose own geometry sits around z = -4000. `base` is chosen so that everything from a deck's
// floor to the top of the tallest deck (deck 9, 2,460 units) maps to that deck's number.
const float DECK_BASE = 1636.0f;

int scopeDeck = 0;      // 0 = no script is running, or the map is not a merged ship
int depth = 0;
char scoped[2][128];
int which = 0;
int lookups = 0;        // name lookups made while a script on some deck was running
int resolved = 0;       // ... that found a deck-scoped entity and were redirected to it

} // namespace

static cvar_t *g_shipDeckPitch; // vertical distance between decks on the merged map; 0 = an ordinary level

void LWH_ScopeBegin( int ownerEntityNum )
{
	++depth;
	if ( depth != 1 ) return; //a script that triggers another keeps the outer script's deck
	scopeDeck = 0;
	if ( !g_shipDeckPitch ) g_shipDeckPitch = gi.cvar( "g_shipDeckPitch", "0", 0 );
	const float pitch = g_shipDeckPitch->value;
	if ( pitch <= 0.0f || ownerEntityNum < 0 || ownerEntityNum >= globals.num_entities ) return;
	const gentity_t *owner = &g_entities[ownerEntityNum];
	if ( !owner->inuse ) return;
	const int deck = static_cast<int>( std::floor( ( -owner->currentOrigin[2] - DECK_BASE ) / pitch ) ) + 1;
	if ( deck >= 1 && deck <= 99 ) scopeDeck = deck;
}

void LWH_ScopeEnd( void )
{
	if ( depth > 0 && --depth == 0 ) scopeDeck = 0;
}

// If an entity carries this name scoped to the running script's deck, that is the one meant.
// With no script running, the name is being looked up for the player -- the turbolift's menu says
// "use tour_turbo_04", as it did when each deck was a level -- so it is the player's deck that counts.
static int PlayersDeck( void )
{
	if ( !g_shipDeckPitch ) g_shipDeckPitch = gi.cvar( "g_shipDeckPitch", "0", 0 );
	const float pitch = g_shipDeckPitch->value;
	if ( pitch <= 0.0f || !g_entities[0].inuse ) return 0;
	const int deck = static_cast<int>( std::floor( ( -g_entities[0].currentOrigin[2] - DECK_BASE ) / pitch ) ) + 1;
	return deck >= 1 && deck <= 99 ? deck : 0;
}

const char *LWH_ScopedName( const char *name )
{
	if ( !name || !name[0] ) return name;
	const int deck = scopeDeck ? scopeDeck : PlayersDeck();
	if ( !deck ) return name;
	++lookups;
	char *candidate = scoped[which ^= 1];
	snprintf( candidate, sizeof( scoped[0] ), "d%02d_%s", deck, name );
	for ( int i = 0; i < globals.num_entities; ++i )
	{
		const gentity_t *e = &g_entities[i];
		if ( !e->inuse ) continue;
		if ( ( e->targetname && !Q_stricmp( e->targetname, candidate ) )
			|| ( e->script_targetname && !Q_stricmp( e->script_targetname, candidate ) ) )
		{
			++resolved;
			return candidate;
		}
	}
	return name;
}

void LWH_ScopeStats( int *outLookups, int *outResolved )
{
	*outLookups = lookups;
	*outResolved = resolved;
}
