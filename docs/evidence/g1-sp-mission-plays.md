# G1 evidence — the single-player mission plays, saves, and reloads

Date: 2026-10-04. Gate: **M2 met. M3 substantially demonstrated** (one acceptance item still needs a
player's judgement — see the end).

Playtest host, retail GOG data, unmodified binary from this repository, run through
`scripts/run-engine.sh`.

## The run, abridged to the lines that matter

```
ui: SP UI loaded (UI_API_VERSION=2)
ui: SetActiveMenu(main)
SP map 'borg1': spawning world + SP game
spmap: loading SP game module + map 'borg1'
EFSP: SP_LoadGame: ge=0x7cb8d227b720 apiversion=6 gentitySize=1320
EFSP: SP_SpawnServer: CM_LoadMap(maps/borg1.bsp)
EFSP: SP_SpawnServer: 158 inline models, entity string 115246 bytes
Parsing ext_data/boltOns.cfg
Parsing ext_data/NPCs.cfg
Parsing addon.npc
------ ICARUS Initialization ------
ICARUS version : 1.33
EFSP: SP_SpawnServer: ge->Init done. num_entities=561 linked=371
EFSP: SP_StartClient: client connected+begun
EFSP: SP_StartClient: first snapshot numEntities=476 ps.origin=(104 -936 5) ps.weapon=2
EFSP: SP_StartClient: cgame CG_INIT...
...loaded 12906 faces, 244 meshes, 0 trisurfs, 138 flares
...found 12306 VBO surfaces (78046 vertexes, 200676 indexes)
EFSP: CG_R_LOADWORLDMAP maps/borg1.bsp
EFSP: SP_StartClient: CG_INIT returned OK
EFSP: save: wrote saves/auto.sav (map borg1, t=1000, eAUTO)
EFSP Route-b: HM spmap/qagame/cgame NOT loaded; SP cgame is sole; clc.state=CA_ACTIVE
spmap: SP map 'borg1' loaded — switching to SP render mode
EFSP: Munro connected
EFSP: save: wrote saves/auto.sav (map borg1, t=234400, eFULL)
EFSP: load: saves/auto.sav -> map borg1 t=234400 (deferred reload)
EFSP: SP_FinishTransition: done numEnt=1022 ps.origin=(304 1099 268)
exit: 0
```

## What each of those proves

- **`SP_LoadGame: ge=… apiversion=6`** — `libefgame.so`, built by this project from Raven's released
  single-player source, loaded into a native Linux engine and handed its API. This is the M2 item.
- **`CM_LoadMap(maps/borg1.bsp)`** and the entity counts — the campaign's own level loading through
  the SP path, not a Holomatch arena.
- **ICARUS 1.33 + `NPCs.cfg` / `boltOns.cfg` / `addon.npc` parsed** — the mission scripting system
  and the data-driven NPC definitions are live, which is what the autonomy layer will build on.
- **`first snapshot … ps.weapon=2`** — the player entity exists with a weapon, predicted locally.
- **`CG_INIT returned OK`, `Route-b: … SP cgame is sole`** — the SP client game owns the session; the
  Holomatch path is deliberately not in the loop.
- **`loaded 12906 faces … 12306 VBO surfaces`** — the level is on screen, through the Vulkan renderer.
- **`EFSP: Munro connected`** — the campaign protagonist, i.e. the player, joined the level.
- **`save: wrote saves/auto.sav` → `load: saves/auto.sav -> map borg1 t=234400`** — a save written
  during play, then loaded back, with the SP path re-initialising cleanly afterwards
  (`SP_FinishTransition: done numEnt=1022`).

The second run of the session started, reached the SP menu (`SetActiveMenu(main)`, `1 mods parsed`)
and exited 0. No abnormal-exit prompt: the previous run had quit cleanly.

## Residuals worth naming rather than leaving as noise

- `Cmd_AddCommand: <name> already defined`, ~30 of them, on the level transition. Harmless — the SP
  path re-registers its console commands after a reload — but noisy, and it means those registrations
  are not idempotent.
- `MAX_PACKET_USERCMDS` printed twice at shutdown. Cosmetic.
- **No cutscenes**: FFmpeg is behind Ubuntu Pro on this host, so Bink playback is compiled out and
  `.bik` files are skipped. Milestone M5.
- Nothing has been checked against a *completed* mission yet.

## What remains for G1

One item, and it is a judgement rather than a measurement: **completing the first mission**. The
mechanics that make that possible are all demonstrated above — play, save, reload — so this is now a
question of someone playing borg1 through to its end and saying so. After that the gate is closed.
