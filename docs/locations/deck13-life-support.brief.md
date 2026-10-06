# Location brief: Deck 13 — Life Support Plant

## 1. Function, in one sentence

The machinery the environmental-control watch (deck 12) watches over: the atmosphere processors, water
reclaimers and heat exchangers that keep fifteen decks breathing, sitting ten metres below Main
Engineering where the plasma and coolant feeds already run.

## 2. Canon anchors

- **Deck 13** carries the **life-support plant**, about ten metres below Main Engineering — source: the
  sourced deck list in `docs/research/voyager-interior-systems.md` (Memory Alpha's Intrepid-class list).
- **Neighbours**: Main Engineering (deck 11) above and environmental control (deck 12) above it, so this
  is the bottom of the systems spine; the Jefferies tubes connect upward.
- **Episode evidence**: the year-of-Hell failures are environmental, and this is where the plant that
  fails lives; canon rarely shows it, which is what makes it a place to invent.
- **Reference image**: none found; the closest is an engineering plant room. Say so.
- **Contested**: none. Deck 13 is uncontested; the contested site is the second holodeck (deck 14 or 6).

## 3. Reuse target

- **Copy**: `tour/deck11` (Main Engineering) — a working plant room, as for deck 12, with the warp core
  swapped for an atmosphere plant and the vertical scale reduced.
- **Change**: warp core to atmosphere processor; add a raised machinery catwalk; a Jefferies tube mouth.
- **Keep**: the plant-room feel, the railings, the fixtures.

## 4. Parts list

To be completed from the pak census of `misc_model`, `func_static` and texture names in `tour/deck11`,
so every piece placed exists in the game. Anything not in that list does not go in.

## 5. Spatial program

- **Footprint**: a long plant hall, one chamber with a central machinery island.
- **Entries**: a door from the turbolift lobby; a **Jefferies tube** at the far end (the route down from
  Engineering), low and awkward.
- **Circulation**: enter at one end, the machinery island down the middle, a catwalk along one wall.
- **Verticality**: a raised catwalk along the plant, so the room reads as machinery.
- **Posts**: one watch/roving maintenance position on the catwalk.

## 6. Lighting brief

Harsh working light over the plant; an **emergency red** state, because this is the plant whose failure
darkens other decks.

## 7. The hook

- **System node**: `SYS_LIFE_SUPPORT`, with deck 12 the control room and this the plant.
- **Job sites**: maintenance, repair, and reclamation if the Borg reach the plant.
- **Control surfaces**: a local plant panel (engineering authority).

## 8. Acceptance checks

Reachable on foot from the turbolift and up a Jefferies tube; no dead ends; `.nav` bakes; frame time
holds. Then the owner walks it and answers: does it read as the plant that keeps the ship breathing?

## 9. Non-goals

No new art. Do not simulate the plant; it must read as plant.

## 10. Status (2026-10-07)

**Re-dressed from `tour/deck11` and awaiting the owner's walkthrough.** Built the way
`docs/authoring-a-location.md` requires and as deck 12 was: the room is **copied from the reuse
target the brief names** (`tour/deck11`, main engineering) and changed to be this room — the vertical
scale reduced (404 to 224), the warp core replaced by a central machinery island of atmosphere
processors, a raised catwalk added along the north wall with a stair, a Jefferies tube mouth at the
far end from the door, and the local plant panel on the catwalk. The room's own consoles, railings,
lighting fixtures, wall panels, door hardware, props and "small untidiness" come along with the copy.
The parts list is `docs/locations/deck13-parts-list.md`; the tool is `tools/shipmap/dressdeck13.py`,
deck 12's pattern, called by `scripts/build-ship.sh`.

Done: the copy and its changes; the life-support system node and the deck's wiring; the local plant
panel and the `plantwatch` post; navigation baked; the **emergency lighting state**
(`hall/hall_light_red`, switched by the module at red alert or a life-support failure), the second
half of the pair. Evidence, with the commands: `docs/evidence/deck13-redress.md`.

Open, and only the owner can close it: **does it read as the plant that keeps the ship breathing?**
The compression is a hypothesis, the same class deck 12 named — a hall can be made low and read as
machinery, or as squashed — and the room reads dark under the copied lighting. The plant, the catwalk
and the post are placed by measurement, not by eye. Walk it; if the plant does not read or the light
is wrong, that is the next pass.
