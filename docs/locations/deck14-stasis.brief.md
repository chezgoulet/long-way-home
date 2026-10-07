# Location brief: Deck 14 — Stasis Chambers and Holodeck 1

## 1. Function, in one sentence

The stasis deck: banks of stasis pods that hold the injured and the frozen, and **Holodeck 1** next
door — the contested placement, decided here as deck 14 (deck 6 holds Holodeck 2).

## 2. Canon anchors

- **Deck 14** carries **stasis chambers**, with the second holodeck contested between decks 14 and 6 —
  source: the sourced deck list in `docs/research/voyager-interior-systems.md`.
- **Neighbours**: engineering support, so the deck is services, not crew.
- **Episode evidence**: stasis pods recur (the dead, the frozen, the Doctor's patients).
- **Reference image**: none; the game's own stasis assets exist (`models/mapobjects/stasis`).
- **Contested**: the holodeck's deck. **Decided: Holodeck 1 is on deck 14, Holodeck 2 on deck 6.**
  Recorded as our call.

## 3. Reuse target

- **Copy**: `tour/deck11` for the industrial read.
- **Change**: the warp core for banks of stasis pods; add a holodeck doorframe.
- **Keep**: the plant-room discipline.

## 4. Parts list

To be completed from the pak census (`models/mapobjects/stasis`, holodeck door frames).

## 5. Spatial program

- **Footprint**: one chamber, a row of stasis pods along a wall.
- **Entries**: turbolift lobby; a **Jefferies tube** at the far end.
- **Circulation**: past the pods to the holodeck end.
- **Verticality**: flat; the pods read as a machinery wall.
- **Posts**: a stasis/holodeck maintenance position.

## 6. Lighting brief

Cold and dim over the pods, warm at the holodeck; emergency red state.

## 7. The hook

- **System node**: `SYS_HOLODECKS` (Holodeck 1); stasis is a casualty path.
- **Job sites**: maintenance; reclamation.
- **Control surfaces**: the holodeck arch.

## 8. Acceptance checks

Reachable on foot; no dead ends; `.nav` bakes. Owner walks it.

## 9. Non-goals

No new art. Do not script the dead-in-stasis scenario here.

## 10. Status (2026-10-07)

**Re-dressed from `tour/deck11` and awaiting the owner's walkthrough.** Built the way
`docs/authoring-a-location.md` requires and as decks 12 and 13 were: the room is **copied from the
reuse target the brief names** (`tour/deck11`, main engineering) and changed to be this room — the
vertical scale reduced (404 to 208), the raised core deck cleared so the room is the brief's one flat
chamber, the warp core replaced by a row of the game's own stasis pods along the north wall, a
holodeck doorframe at the far end (Holodeck 1, the decided placement) with its control panel and
maintenance post, a Jefferies tube at the west end, and the room lit by function: cold over the pods,
warm at the holodeck. The room's own consoles, railings, lighting fixtures, wall panels, door
hardware, props and "small untidiness" come along with the copy. The parts list is
`docs/locations/deck14-parts-list.md`; the tool is `tools/shipmap/dressdeck14.py`, deck 13's pattern,
called by `scripts/build-ship.sh`.

Done: the copy and its changes; the `SYS_HOLODECKS` station marker (`lwh_station_17`) and the
`lwh_stasis_post`/`stasiswatch` maintenance post; the lift edge and the tube to deck 13; navigation
baked; the **emergency lighting state** (`hall/hall_light_red`, tagged `_14`, switched by the module
at red alert or a life-support failure). Evidence, with the commands: `docs/evidence/deck14-stasis.md`.

Open, and only the owner can close it: **does the walk from the cold pods to the warm holodeck read
as the contrast this brief intends?** The compression to 208 and the cleared core deck are
hypotheses, and the copied engineering interior still dominates the room; the function lighting is
authored, not yet judged. The pods, the holodeck and the post are placed by measurement, not by eye.
Walk it; if the pods do not read as a ward or the light is wrong, that is the next pass.
