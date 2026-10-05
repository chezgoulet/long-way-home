// g_ship.h -- the ship simulation's entry points into the game module (gate S2).
//
// Like the crew layer, everything here is inert unless switched on (`g_ship 1`): with the cvar at
// its default the module behaves, and saves, exactly as it did before.

#ifndef LWH_G_SHIP_H
#define LWH_G_SHIP_H

namespace ship { struct Ship; }

void Ship_RegisterCvars(void);  // G_InitCvars
void Ship_Init(void);           // end of InitGame
void Ship_Frame(void);          // G_RunFrame
void Ship_Shutdown(void);       // ShutdownGame
void Ship_WriteSave(void);      // G_LoadSave_WriteMiscData
void Ship_ReadSave(void);       // G_LoadSave_ReadMiscData
void Svcmd_Ship_f(void);        // server command "ship"

// The live ship, or NULL when the simulation is off. For the rest of the module (consoles, crew).
ship::Ship *Ship_Get(void);

#endif
