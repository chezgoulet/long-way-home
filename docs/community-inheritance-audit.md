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

## What I did not verify in this pass

Honestly: the RPG-X Legal Notice's actual terms (the readme says *may use or adopt parts*, but the notice is the
authority); whether Voyager: Insurrection is still downloadable anywhere; the current liveness of the map
archives and their individual licences; and whether Elite Reinforce's fixes are already present in our module
-- that last one is a real task, not a check.

## Verdict

**Not leaving the lineage on the table** -- lilium, cMod and the SP port are all current, and the tooling and
GDK are fully harvested. **Leaving two things on the table**: RPG-X's ship-and-set entity classes, which are
the functional controls we are about to author by hand, and Elite Reinforce's fix list and movement
documentation. Plus one licensing question over community map content that would be volume rather than
architecture.
