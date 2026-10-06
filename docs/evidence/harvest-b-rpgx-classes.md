# Harvest B — RPG-X's entity classes

The mandate's second community harvest (`docs/handoff-full-scope.md` §1b): adopt the subset of
RPG-X's 51 retail-absent entity classes that our work needs, with their game-code implementations,
a validator dictionary entry each, and a map fixture exercising what is adopted. Licence: **code is
adoptable with credit to UberGames; assets are not** (`docs/community-inheritance-audit.md`).

## Adopted so far

### `target_shaderremap` — 2026-10-07, verified

A placed entity that points one shader name at another at run time, so a section can visibly change
and change back. It rides the single-player engine's own shader switch, `gi.RemapShader` (routed to
`RE_RemapShader` by `patches/0014`), so it needs no BSP edit — the same capability is a candidate for
the **first hard problem** (the Borg visibly taking the ship).

- **Logic** in `module/ship/lwh_entities.cpp` (spawn function and the toggle). The shader pair is
  kept in a module-side table keyed by entity number; the toggle state rides the entity's own
  `spawnflags`. `gentity_s` is not changed — it is fixed at 1320 bytes by the save transcoder
  (`g_savetranscode.cpp`), so a new field would break retail saves.
- **Registration** by `patches/0015`, as attach points only: a `useF_target_shaderremap` enum entry,
  a dispatcher case in `GEntity_UseFunc`, the spawn-table row, and the hook declarations in
  `lwh_hooks.h` (empty inlines without `LWH_MODULE_DIR`, and no retail map contains the class, so
  retail behaviour and saves are untouched).
- **Dictionary**: `tools/entitydict/lwh_entities.def`, merged by `scripts/check.sh`, so the
  validator's class check covers it.
- **Fixture and proof**: `tools/shipmap/gendeck.py` places one on each generated deck;
  `scripts/shaderremap-check.sh` (`g_shipTest 32`) loads the merged ship and finds:

```
    LWH: target_shaderremap falsename=textures/lwh/panel truename=textures/lwh/borg
    SHIP: shaderremap test: found target_shaderremap at (-4416 -3584 -19336)
    LWH: target_shaderremap on textures/lwh/borg
    LWH: target_shaderremap off textures/lwh/borg
PASS  the adopted target_shaderremap spawns, and firing it toggles a shader at run time and back
```

The absence of any `RE_RemapShader ... not found` line is the proof the renderer accepted both
names and the swap landed.

## What was measured getting there (why the first cut failed)

**The single-player entity system differs from the multiplayer tree the class was written for.** The
multiplayer source under `EFAndroid-SP/app/jni/efcode` sets `ent->use = fn` and carries
`CS_SHADERSTATE` + `AddRemap`; the single-player source under `efgame/src` does not. There:

1. **Use functions are an indexed table, not a pointer.** `gentity_s` holds `useFunc_t e_UseFunc`
   (an index into the enum at `src/game/g_functions.h`, dispatched by `GEntity_UseFunc` in
   `g_functions.cpp`), so a new class needs an enum entry **and** a dispatcher case.
2. **There are no shader-name fields**, and the struct size is fixed for the save transcoder, so the
   pair is stored module-side instead.
3. **The engine syscall is the path**: `gi.RemapShader(old, new, offsetTime)`, not a configstring.

Grepping the wrong tree for the mechanism is what cost the first attempt; the module now includes
the enum header (`g_functions.h`) it needs.

## Still to adopt (from the audit's 51), each only when a work item needs it

`func_forcefield`, `func_mover`, `func_door_rotating`, `func_lightchange`; the ship systems as
entities (`target_turbolift`, `target_warp`, `target_gravity`, `target_shiphealth`, `target_repair`,
`target_selfdestruct`, `target_holodeck`, `target_teleporter`, `target_zone`, `target_objective`,
`target_alert`); `trigger_radiation`, `trigger_transporter`; `ui_msd`, `ui_transporter`,
`ui_holodeck`; and the `fx_` set.

A next candidate worth naming: **`target_shaderremap` + the Borg layer** is the cleanest attack on
the first hard problem, but a global remap by name cannot scope to a section, so the published-deck
scoping question is unchanged (`docs/evidence/s8-the-borg.md`).
