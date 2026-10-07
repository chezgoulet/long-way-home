# Location brief: Deck 7 — Auxiliary Computer Core, Cargo, Labs, Escape Pods

## 1. Function, in one sentence

The depth deck: the auxiliary computer core the ship falls back to when the main core is lost, with
cargo bays, labs and escape-pod access around it.

## 2. Canon anchors

- **Deck 7** carries the auxiliary computer core, cargo, labs and escape pods — source: the sourced
  deck list in `docs/research/voyager-interior-systems.md`.
- **Neighbours**: the main core is upstairs on deck 9/10, so this is the fallback and the depth.
- **Episode evidence**: escape pods recur; "the computer is the ship's memory" is a canon theme.
- **Reference image**: none; the game's cargo and computer assets exist.
- **Contested**: none.

## 3. Reuse target

- **Copy**: `tour/deck11` for the industrial read; the core column is the deck's spine.
- **Change**: the warp core for an auxiliary core column; add cargo islands.
- **Keep**: the plant-room discipline.

## 4. Parts list

To be completed from the pak census (computer core, cargo crates, escape pod hatch).

## 5. Spatial program

- **Footprint**: one chamber with a central core column and cargo islands.
- **Entries**: turbolift lobby; a **Jefferies tube**.
- **Circulation**: around the core column; cargo islands to the sides.
- **Verticality**: the core column is tall; escape-pod access overhead.
- **Posts**: a computer-core watch position.

## 6. Lighting brief

Working light, console glow at the core; emergency red state.

## 7. The hook

- **System node**: `SYS_COMPUTER_CORE` (auxiliary depth).
- **Job sites**: maintenance, repair, reclamation.
- **Control surfaces**: the core panel (Operations).

## 8. Acceptance checks

Reachable on foot; no dead ends; `.nav` bakes. Owner walks it.

## 9. Non-goals

No new art.

## 10. Status (2026-10-07)

**Re-dressed from `tour/deck11` and awaiting the owner's walkthrough.** Built the way
`docs/authoring-a-location.md` requires and as decks 12, 13 and 14 were: the room is **copied from
the reuse target the brief names** (`tour/deck11`, main engineering) and changed to be this room —
the vertical scale reduced (404 to 240), the raised core deck cleared for one chamber, the warp core
replaced by an **auxiliary core column** at the centre with the core panel at its base, **two cargo
islands** of the game's own crates added to the sides, an **overhead escape-pod hatch** on a short
walkable platform, and a Jefferies tube mouth at the far end routing to the main computer core
(`tour/deck10`). The room's own consoles, railings, lighting fixtures, wall panels, door hardware and
props come along with the copy. The parts list is `docs/locations/deck07-parts-list.md`; the tool is
`tools/shipmap/dressdeck07.py`, deck 14's pattern, called by `scripts/build-ship.sh`.

Done: the copy and its changes; the computer-core system node and the deck's wiring; the core panel
and the `corewatch` post; navigation baked; the **emergency lighting state** (`hall/hall_light_red`,
tagged `_07`, switched by the module at red alert or a life-support failure). The copied interior is
**detail** geometry, because a fourth full re-dress passes q3map2's vis-cluster ceiling otherwise.
Evidence, with the commands: `docs/evidence/deck07-auxcore.md`.

Open, and only the owner can close it: **does the depth deck read as the core the ship falls back
to?** The compression is a hypothesis (404 to 240), and the copied interior is now detail geometry —
invisible at runtime, but it means the room's structure is the shell only. Walk it; if the core does
not read or the light is wrong, that is the next pass.
