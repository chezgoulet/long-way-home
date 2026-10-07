# Deck 14 — Stasis Chambers and Holodeck 1: parts list

The brief's section 4 leaves the parts list open, "to be completed from the pak census
(`models/mapobjects/stasis`, holodeck door frames)". This is that list. It is a census of the reuse
source (`tour/deck11`, the published deck the brief names), taken by `tools/shipmap/dressdeck14.py
--census`, plus the materials and models this re-dress adds on top of it, each checked against the
shipped game.

Deck 14 copies the **same room** decks 12 and 13 copied (`tour/deck11`, main engineering), so the
source census is the same 77 materials and 4 models. The **difference is what this re-dress adds**.

## Models (`misc_model`, `func_static` with a `model`)

The four in `tour/deck11`, carried by the copy:

| model | count in `tour/deck11` |
|---|---|
| `models/mapobjects/cargo/turbo_lite.md3` | 1 |
| `models/mapobjects/cargo/padd.md3` | 1 |
| `models/mapobjects/cargo/toolkit.md3` | 1 |
| `models/mapobjects/secret_items/hazbox_tel.md3` | 1 |

The **stasis pods** the brief asks for are the game's own pod model, the one the shipped stasis maps
place as `misc_model_breakable`:

| model | placed by this deck | where it comes from |
|---|---|---|
| `models/mapobjects/stasis/pod.md3` | 8 (a row along the north wall) | shipped paks (`models/mapobjects/stasis/`), the brief's section 4 |
| `models/mapobjects/cargo/toolkit.md3` | 1 (the "one human trace", at the foot of the row) | already in the source room |

The pod's `mins`/`maxs` (`-32 -19 -48` / `32 19 48`) are taken from the shipped stasis maps' own
placements, so the collision box matches the model. No new models: every model named here exists in
the shipped game.

## Entity classes in the source room

`light` 104, `target_speaker` 39, `trigger_multiple` 38, `waypoint` 25, `func_group` 24,
`waypoint_navgoal_2` 19, `func_usable` 18, `NPC_starfleet_random` 10, `func_static` 9,
`target_level_change` 9, `waypoint_navgoal_1` 5, `target_scriptrunner` 4, `waypoint_navgoal_4` 4,
`misc_model_breakable` 4, `func_door` 4, `waypoint_small` 4, `target_activate` 4, `trigger_once` 3,
`target_relay` 3, `target_deactivate` 2, `info_notnull` 2, `ref_tag` 2, `info_player_start` 2,
`target_position` 1, `target_interface` 1, `weapon_tricorder` 1, and the `NPC_*` spawns.

The re-dress keeps the room's own lights (dimmed), speakers, doors, panels, triggers, props and
navigation furniture, and drops deck 11's business: its script runners and script triggers, its NPC
spawns, the player starts, the turbolift network, the pickup and the editor tags. What it drops and
why is in `dressdeck14.py`.

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
| `engineering/holodecklights` | shipped image (`textures/engineering/`) — the glow strips on the holodeck arch (the game's own holodeck material) |
| `engineering/elight1` | the source room's own light material |
| `lwh/deck14floor`, `lwh/deck14wall0/1/2` | ours, shipped in the pak (`tools/shipmap/data/lwh_borg.shader`); deck-addressable so the Borg takeover can tint deck 14 alone |
| `lwh/panel` | ours, shipped in the pak (`tools/shipmap/data/lwh_panel.shader`): the local control panel's live surface |
| the holodeck door/frame's `voyager/voydoor3b`; the console's `engineering/console1`, `voyager/basic`; the pod pedestals' `engineering/supportedge`; the panel frames' `engineering/newgrey1` | already in the source room's own census |
| `cargo/cargodoor1`, `common/trigger` | already in the source and decks 12/13's own wiring |

## The check

Every face texture used by the generated `deck14.map` resolves: an image in the paks, a shader the
game or our own data declares, or a material in `tour/deck11`. The command and its verdict are in
`docs/evidence/deck14-stasis.md` ("the parts census"). **No new art.**

## A reference image

The brief's section 2 says none was found: there is no canon screenshot of deck 14's stasis chambers.
The closest is a working engineering plant room, which is exactly why the reuse target is
`tour/deck11`; the game's own stasis models and holodeck materials supply the pod and arch. Invented,
and logged as such in `docs/lore-ledger.md`.
