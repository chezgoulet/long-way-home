// lwh_entities.cpp -- RPG-X entity classes that Long Way Home adopts, implemented in the module.
//
// The first is `target_shaderremap`: a placed entity that toggles the renderer's mapping of one
// shader to another at run time, so a section can visibly change and change back. It rides the
// single-player engine's own shader switch (`gi.RemapShader`, routed to RE_RemapShader by
// patches/0014), so no BSP edit is needed -- the same capability is a candidate for the Borg
// visibly taking the ship (hard problem #1).
//
// Ported from UberGames' RPG-X gamecode (rpgxEF, `code/game/g_target.c`); their Legal Notice
// permits source reuse with credit and does not conflict with Raven's STEF terms. Credit:
// UberGames / RPG-X. See NOTICE. Long Way Home's change is the module/home split: the logic lives
// here, and patches/0015 adds only the registration (a useFunc entry, a dispatcher case, and the
// spawn-table row).
//
// The single-player gentity_t is a fixed 1320 bytes (g_savetranscode.cpp enforces it) and has no
// shader-name fields, so the pair is kept in a module-side table keyed by entity number rather
// than added to the struct; the toggle state rides the entity's own `spawnflags`.
//
// Off by construction: with LWH_MODULE_DIR unset the class is not registered at all, and no retail
// map contains it, so retail behaviour is untouched.

#include "g_local.h"
#include "g_functions.h"
#include "lwh_entities.h"

#include <map>
#include <string>

namespace
{
	// entity index -> (shader in game at spawn, shader that replaces it)
	std::map<int, std::pair<std::string, std::string> > remaps;
}

void LWH_target_shaderremap_use( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	const std::map<int, std::pair<std::string, std::string> >::iterator it = remaps.find( ent - g_entities );
	if ( it == remaps.end() )
		return;
	const std::string &original = it->second.first;
	const std::string &replacement = it->second.second;

	// First use swaps original -> replacement; the next swaps it back. `spawnflags` carries the state.
	if ( ent->spawnflags == 0 )
	{
		gi.RemapShader( original.c_str(), replacement.c_str(), "0" );
		ent->spawnflags = 1;
	}
	else
	{
		gi.RemapShader( original.c_str(), original.c_str(), "0" );
		ent->spawnflags = 0;
	}
	gi.Printf( "LWH: target_shaderremap %s %s\n", ent->spawnflags ? "on" : "off", replacement.c_str() );
}

// The class, matching the RPG-X definition: keys "falsename" (in game at spawn) and "truename"
// (what replaces it).
void LWH_SP_target_shaderremap( gentity_t *ent )
{
	char *falsename = NULL, *truename = NULL;
	G_SpawnString( "falsename", "", &falsename );
	G_SpawnString( "truename", "", &truename );
	if ( !falsename || !falsename[0] || !truename || !truename[0] )
	{
		gi.Printf( S_COLOR_YELLOW "target_shaderremap without falsename/truename at %s\n", vtos( ent->s.origin ) );
		G_FreeEntity( ent );
		return;
	}
	remaps[ent - g_entities] = std::make_pair( std::string( falsename ), std::string( truename ) );
	ent->e_UseFunc = useF_target_shaderremap;
	gi.Printf( "LWH: target_shaderremap falsename=%s truename=%s\n", falsename, truename );
}
