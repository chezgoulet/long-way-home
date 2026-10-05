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
void Crew_OnUsed(struct gentity_s *self, struct gentity_s *user);   // NPC_Use
void Crew_OnResponded(struct gentity_s *self);                      // NPC_Respond
void Crew_OnTouched(struct gentity_s *self, struct gentity_s *other); // NPC_Touch
void Crew_WriteSave(void);                                          // G_LoadSave_WriteMiscData
void Crew_ReadSave(void);                                           // G_LoadSave_ReadMiscData
void Svcmd_Crew_f(void);                                            // server command "crew"

#endif
