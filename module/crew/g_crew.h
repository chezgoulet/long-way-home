// g_crew.h -- the reactive-crew direction layer's entry points into the game module.
//
// These are the only names the patched upstream source refers to (patches/0005). Each is a no-op
// unless the layer is switched on with `g_crew 1`, so with the cvar at its default the module
// behaves, and saves, exactly as it did before the layer existed.

#ifndef LWH_G_CREW_H
#define LWH_G_CREW_H

struct gentity_s;

void Crew_RegisterCvars(void);                                      // G_InitCvars
void Crew_Init(void);                                               // end of InitGame
void Crew_FrameBegin(void);                                         // G_RunFrame, first thing
void Crew_Frame(void);                                              // G_RunFrame, after every entity has thought
void Crew_PlayerFrame(void);                                        // G_RunFrame: the player's body in the world (g_player)

// The dead are not reloaded (Stage B): true to refuse a respawn. The engine's respawn hook calls
// this; with the extension off it is false, and the retail behaviour stands.
bool LWH_BlockRespawn(struct gentity_s *ent);

// The player works a system at their console (Stage B): the odds are rolled with the player as the
// operator, so a degraded system can let go at the person holding the controls. False when the
// layer is off or no character is the player.
bool Crew_PlayerUseSystem(int system);
void Crew_OnUsed(struct gentity_s *self, struct gentity_s *user);   // NPC_Use
void Crew_OnResponded(struct gentity_s *self);                      // NPC_Respond
void Crew_OnTouched(struct gentity_s *self, struct gentity_s *other); // NPC_Touch
void Crew_WriteSave(void);                                          // G_LoadSave_WriteMiscData
void Crew_ReadSave(void);                                           // G_LoadSave_ReadMiscData
void Svcmd_Crew_f(void);                                            // server command "crew"

// The environment in the world (g_env 1): gravity per person, and the breach effects. Magnetic
// boots are the player's way back out of freefall, and are the "ship boots" console command.
void Crew_ToggleBoots(void);
bool Crew_BootsOn(void);
int Crew_Floating(void);   // embodied crew floating on BS_FLY right now (the deck's plating)

#endif
