// lwh_entities.h -- RPG-X entity classes that Long Way Home adopts, implemented here (module
// logic) and registered in the game by patches/0015. Forward-declares gentity_s so the hook header
// can name the functions without g_local.h; the implementation includes it.
//
// Provenance: `target_shaderremap` is ported from UberGames' RPG-X gamecode (rpgxEF, g_target.c),
// whose Legal Notice permits source reuse with credit and does not conflict with Raven's STEF
// terms. Credit: UberGames / RPG-X. See NOTICE.

#ifndef LWH_ENTITIES_H
#define LWH_ENTITIES_H

struct gentity_s;

void LWH_SP_target_shaderremap( struct gentity_s *ent );
void LWH_target_shaderremap_use( struct gentity_s *ent, struct gentity_s *other, struct gentity_s *activator );

#endif
