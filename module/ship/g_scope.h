// g_scope.h -- deck-scoped names for scripts on the merged ship (gate S3).
//
// Ten decks were ten levels, and their scripts were written for one level at a time: a deck's
// script that says "alarm" means that deck's alarm. On the merged ship every such name that more
// than one deck defines has been prefixed by the stitcher ("d04_alarm"), so while a script runs
// for an entity, a name lookup first tries the name scoped to that entity's deck.
//
// Plain ints and strings only: the ICARUS sources include this without the game headers.

#ifndef LWH_G_SCOPE_H
#define LWH_G_SCOPE_H

void LWH_ScopeBegin( int ownerEntityNum );      // CTaskManager::Update, before the script runs
void LWH_ScopeEnd( void );                      // ... and after
const char *LWH_ScopedName( const char *name ); // G_Find: the name to look for instead, or `name`

void LWH_ScopeStats( int *lookups, int *resolved );  // for the harness and the console

#endif
