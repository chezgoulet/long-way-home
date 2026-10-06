# Deck 12 — Environmental Control: parts list

The brief's section 4 leaves the parts list open, "to be completed from the packet census of
`misc_model`, `func_static` and texture names in `tour/deck11` and the campaign's engineering maps,
so every piece placed is an object that already exists in the game. Anything not in that list does
not go in."

This is that list. It is a census of the reuse source (`tour/deck11`, the published deck the brief
names), taken by `tools/shipmap/dressdeck12.py --census`, plus a check that every texture the
re-dressed `deck12.map` actually uses resolves to something that already exists: an image in the
shipped paks, a shader the game declares, one of our own deck-addressable aliases (no art), or a
material already in the source.

## Models (`misc_model`, `func_static` with a `model`)

| model | count in `tour/deck11` |
|---|---|
| `models/mapobjects/cargo/turbo_lite.md3` | 1 |
| `models/mapobjects/cargo/padd.md3` | 1 |
| `models/mapobjects/cargo/toolkit.md3` | 1 |
| `models/mapobjects/secret_items/hazbox_tel.md3` | 1 |

No new models. The re-dress places no model that is not on this list.

## Entity classes in the source room

`light` 104, `target_speaker` 39, `trigger_multiple` 38, `waypoint` 25, `func_group` 24,
`waypoint_navgoal_2` 19, `func_usable` 18, `NPC_starfleet_random` 10, `func_static` 9,
`target_level_change` 9, `waypoint_navgoal_1` 5, `target_scriptrunner` 4, `waypoint_navgoal_4` 4,
`misc_model_breakable` 4, `func_door` 4, `waypoint_small` 4, `target_activate` 4, `trigger_once` 3,
`target_relay` 3, `target_deactivate` 2, `info_notnull` 2, `ref_tag` 2, `info_player_start` 2,
`target_position` 1, `target_interface` 1, `weapon_tricorder` 1, and the `NPC_*` spawns.

The re-dress keeps the room's own classes (lights, speakers, doors, panels, triggers, props and
navigation furniture) and drops deck 11's business: its script runners and script triggers, its
NPC spawns, the player starts, the turbolift network, the pickup and the editor tags. What it drops
and why is in `dressdeck12.py`.

## Textures in the source room (top 25 by face count)

`engineering/newgrey1` 264, `hall/supportsegment_basic` 183, `common/caulk` 100, `hall/halllight` 91,
`conference/cwall_basic` 83, `engineering/enggrey` 75, `engineering/railsupports` 64,
`engineering/console1` 47, `voyager/basic` 44, `voyager/benchbasic` 43, `common/trigger` 41,
`engineering/elight1` 37, `engineering/supportedge` 34, `engineering/engtexture1` 33, `common/clip`
30, `cargo/cargodoor1` 29, `hall/hallfloor3` 29, `engineering/handrail` 28, `hall/paddy` 26,
`engineering/chrome` 26, `hall/paddys_friend` 23, `engineering/corefloor` 23, `engineering/engwall`
22, `voyager/voydoor3b` 22, `cargo/texture1` 20 — 77 distinct materials in all.

## The materials the re-dress adds on top of the source

Named here because the brief says anything not in the list does not go in:

| material | where it comes from |
|---|---|
| `jefferies/basesides`, `jefferies/sides` | shipped paks (`textures/jefferies/`) — the tube the brief asks for |
| `hall/hall_light_red` | a shader the game declares (`textures/hall/redlight.tga`), the red state |
| `engineering/elight1` | the source room's own light material |
| `lwh/deck12floor`, `lwh/deck12wall0/1/2`, `lwh/panel` | ours, shipped in the pak (`tools/shipmap/data/*.shader`); deck-addressable so the Borg takeover can tint deck 12 alone |
| `voyager/field_activation3`, `common/trigger`, `cargo/cargodoor1` | already in the source and the deck's own wiring |

## The check

Every face texture used by the generated `deck12.map` resolves: an image in the paks, a shader the
game or our own data declares, or a material in `tour/deck11`. The command and its verdict are in
`docs/evidence/deck12-redress.md` ("the parts census"). **No new art.**
