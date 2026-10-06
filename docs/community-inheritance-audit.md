# Community inheritance audit

The owner's question: are we leaving content or code on the table that the community has built? Checked
2026-10-05 against our own pins, the GitHub API, and the wider scene. **The short answer: our lineage is
current, and our harvesting of content and entities is thin.** Three things are on the table.

## What we ride, and whether we are behind

| source | our relationship | status |
|---|---|---|
| **lilium-voyager** (engine) | our single-player client builds on it | **current.** Its latest push is 2026-05-20, which is the commit our own banner prints (`1.40_GIT_0f7dcd8-2026-05-20`). The project has also moved to `clover-moe/lilium-voyager` |
| **cMod v1.30** (multiplayer) | mode 2, inherited as shipped, zero delta | **current.** v1.30 (2025-12) is still the latest release |
| **VoyagerSP-Android** (our module) | pinned at `0d8942e8` | **current**, last pushed 2026-07-18 |
| **GDK, map sources, script corpus** | harvested, and the validator round-trips against them | **fully used** |
| NetRadiant-custom, q3map2, bspc, msitools | toolchain | **adopted** |

So nothing is being missed in the lineage itself. The gap is on the content and entity side.

## On the table

### 1. RPG-X's entity set and roleplay gamecode -- the biggest item

`UberGames/rpgxEF` is alive (pushed 2026-06-20) and now **includes the gamecode**: RPG-X 2.2 beta source, with
engine under GPL-2 and the gamecode under Raven's STEF terms plus UberGames' own notice, which says plainly
that you **may use or adopt parts of this mod**.

Its `rpg-x_entities.def` is 92 KB and defines **115 classes** -- and they are exactly the kind of thing we are
hand-authoring:

- `func_forcefield`, `func_lightchange`, `func_mover`, `func_targetmover`, `func_train`, `func_plat`,
  `func_breakable`, `func_bobbing`, `func_pendulum`, `func_stasis_door`, `cinematic_camera`;
- and a large effect library (`fx_energy_stream`, `fx_borg_energy_beam`, `fx_electrical_explosion`, ...).

That maps onto two of our own work items: the fifth approved gap, **functional controls**, and the holodeck's
uses. A roleplay ship environment needed doors that move, lights that change, force fields that hold, platforms
that carry people and cameras that frame a scene -- which is what our "controls across the ship" list asks for.
Adopting the classes we need, with attribution, is cheaper than authoring them and gives us a second
community's testing as a warranty.

**Action:** read the RPG-X Legal Notice and `rpg-x_entities.def` in full; adopt the subset we need; record the
attribution. Also worth reading their *role and character* systems, which are the closest prior art to our
start states and crew records -- we have studied RPG-X already (`docs/prior-art-rpg-x.md`), but that was the
mod, not this gamecode.

### 2. Elite Reinforce -- a third single-player lineage, and its documentation

`kugelrund/Elite-Reinforce` (pushed 2025-09) is a desktop **single-player source port focused on bugfixes and
speedrunning**, carrying its own `client/`, `qcommon/`, `renderer/`, `game/`, `cgame/` and `icarus/` trees.

Two things are worth taking. Its **fix set** is a third independent pass over the same released source as our
module's lineage, so a diff of its tree against ours may contain bugs we never inherited. And its
`docs/movement_physics/` explains Elite Force's movement in depth -- air acceleration and velocity snapping,
and how it differs from vanilla Quake 3 -- which is the reference for whether the game *feels* like Elite Force.

**Action:** read its docs; diff its game source against our module's upstream; pick up fixes rather than
re-deriving them. Note we already know it runs on the playtest host.

### 3. Content: maps, mods, and the archives

- **Developer-made bonus Holomatch map pack** -- Raven's own extra maps, distributed through the old file
  hosts. Official, and the most likely to be cleanly redistributable.
- **Community map libraries** -- thousands of maps, and the older ones are usually offered for free
  distribution by their authors, but that is a per-pack question, not an assumption.
- **`UberGames/SP-Mod-Source-Code`** -- they are the scene's de facto source archivist: rpgxEF, RPG-X2 and this
  mirror are one cluster, and it should be swept as a unit rather than one repo at a time.
- **Voyager: Insurrection** (The Dark Project) -- a single-player total conversion we never catalogued, and the
  mod whose existence prompted Raven's SP SDK release. Its content and licence are unverified.

**Action:** verify each pack's licence before vendoring anything; ship a curated pack only for what is
explicitly redistributable; treat the rest as playable-but-not-bundled, documented as such.

## Verified since, at code level

### The licence, read properly -- and it splits the opportunity

RPG-X's own Legal Notice, verbatim: *"All assets used in this file and/or git repository are copyrighted to
the UberGames and/or their original creators. Any external use of these assets without explicit permission
from their original creators is not allowed. **Feel free to use the source code** included in this file and/or
git repository for your own mod as long as you give full credit to UberGames for our modification/code and as
long as this does not conflict with the original license of the STVEF HM gamecode."*

So: **code is adoptable with credit; assets are not adoptable without permission.** Their game module carries
Raven's own `STEF Game Source License.doc` in `code/game`, `code/cgame` and `code/ui` -- the same terms our
module already rides -- and the embedded Lua layer is MIT. Their maps, models, textures and sounds are off
limits unless someone asks them.

That generalises, and it is the real shape of the community harvest: **the community's code is largely
shareable, the community's art largely is not.** Which suits us, because our art already comes from the game we
licence, and what we have been hand-authoring is code.

### 51 entity classes we do not have

Measured, not guessed: RPG-X defines 115 classes, our retail dictionary defines 214, and **64 are shared --
leaving 51 classes that exist only in RPG-X**, and they are almost exactly our functional-controls list:

- **`func_`** -- `func_forcefield`, `func_mover`, `func_targetmover`, `func_door_rotating`,
  `func_brushmodel`, `func_lightchange`;
- **`target_`** -- the ship's systems, as entities: `target_turbolift`, `target_levelchange`, `target_warp`,
  `target_gravity`, `target_shiphealth`, `target_selfdestruct`, `target_repair`, `target_doorlock`,
  `target_holodeck`, `target_teleporter`, `target_shaderremap`, `target_zone`, `target_objective`,
  `target_alert`, `target_boolean`, `target_remove_powerups`, `target_evosuit`, `target_shake`,
  `target_serverchange`;
- **`fx_`** -- `fx_fire`, `fx_fountain`, `fx_particle_fire`, `fx_phaser`, `fx_torpedo`, `fx_transporter`;
- **`ui_`** -- `ui_holodeck`, **`ui_msd`**, `ui_transporter`;
- **`trigger_`** -- `trigger_radiation`, `trigger_transporter`;
- and `path_point`, `item_botroam`, `_decal`, `_skybox`, `cinematic_camera`, `info_player_intermission`.

Three of those are worth naming individually:

- **`target_shaderremap`** -- runtime shader switching. That is a candidate mechanism for the Borg visibly
  taking the ship, which we had listed as an engine capability that does not exist.
- **`ui_msd`** -- a master systems display, as an entity. We are building panel surfaces by hand.
- **`target_turbolift`, `target_shiphealth`, `target_selfdestruct`, `target_warp`, `target_gravity`** -- the ship
  systems we are modelling, already expressed as things a designer can place in a map.

Their gamecode also embeds an **entity-definition parser** and a **Lua scripting layer**, which is prior art for
our module boundary and worth reading before we extend ICARUS further.

### Elite Reinforce: four fixes we do not have, verified by grep

Our tree contains no reference to borg1, forge3 or a holodeck save restriction, and none of our fourteen patches
addresses them. Elite Reinforce's history does:

- **"Fix the borg1 freeze"** -- with a cvar to disable it. **borg1 is the map our own mission evidence plays
  on**, so this is not an abstract fix.
- **"Better fix for infinite loop in forge3"**, after reverting a first attempt -- a bug they took two passes to
  get right.
- **"Fix occasional menu softlock"** -- and we are actively working in the menus.
- **"Add cmd to allow saving on holodeck maps"** -- directly relevant, since our Virtual Voyager and holodeck
  work involves saving inside those maps.
- plus two authoring tools worth having: **showing NPC paths** (useful for our crew and post work) and
  **highlighting entities with death scripts**. And the `docs/movement_physics/` explanations of air
  acceleration and velocity snapping as the reference for movement fidelity.

All of it is single-player game code under the same STEF licence we ride, so porting it into our module is
licence-compatible. The work is a three-way diff, not a rewrite.

## Still unverified

Whether **Voyager: Insurrection** is downloadable anywhere, and the individual licences of the community map
packs. Given the pattern above, assume the maps' *art* is permission-gated until a pack says otherwise, and
treat Raven's own developer bonus maps as the likeliest clean case.

## Verdict

**Not leaving the lineage on the table** -- lilium, cMod and the SP port are all current, and the tooling and
GDK are fully harvested. **Leaving three things on it**, now verified at code level: **51 entity classes** in
RPG-X that retail lacks and that are precisely our functional-controls list -- with `target_shaderremap` a
candidate mechanism for the Borg asset conversion and `ui_msd` a master systems display we were about to build
by hand; **four single-player fixes** in Elite Reinforce, including the borg1 freeze on the map our own evidence
plays, and a command allowing saves on holodeck maps; and **the community's art**, which is permission-gated
rather than free, and which we do not need because our art comes from the game we licence.

Adopting the code is licence-compatible today and needs credit; adopting the art needs a conversation with its
authors. The first is worth doing this week.
