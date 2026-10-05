# Location brief: Deck 12 — Environmental Control

## 1. Function, in one sentence

The watch station and plant that keeps fifteen decks breathing: one watchkeeper, a bank of monitors, and
the machinery that moves atmosphere, heat and pressure around the ship.

## 2. Canon anchors

- **Deck 12** carries navigational control (section B7), environmental control, the secondary command
  processors, and **section 42** -- source: the sourced deck list in
  `docs/research/voyager-interior-systems.md`, from Memory Alpha's Intrepid-class deck list.
- **Neighbours**: navigational control and the secondary command processors, so this is a working half-deck
  of systems rooms, not a corridor of quarters. **Section 42 sits on the same deck and is off-limits** --
  the precedent is "The Haunting of Deck Twelve", where something aboard lived in the ship's own systems.
  That adjacency is free drama and should not be built into this room, only left next door.
- **Episode evidence**: environmental control fails often enough in canon to be a running joke and a real
  threat -- the deck-level failures in the Year of Hell ledger are environmental. The *Haunting* episode is
  the strongest visual reference for how the ship's systems spaces look and behave.
- **Reference image**: a canon screenshot of environmental control, if one can be found on Memory Alpha;
  otherwise use the closest systems-room screenshot and say so.
- **Contested**: none. Deck 12 is uncontested; the contested site is the second holodeck (deck 14 or 6).

## 3. Reuse target

- **Copy**: `tour/deck11` (main engineering) -- 345 entities, the densest systems room in the deck set, with
  consoles, plant, and a working half-deck feel. It is the closest thing that already exists to a plant
  room a person watches over.
- **Change**: swap the warp core for an atmosphere plant; reduce the vertical scale; add a second exit as a
  Jefferies tube; put the watch console where the room can be *entered* rather than at its centre.
- **Keep**: the console arrangement, the railings, the lighting fixtures, the wall panels and the door
  hardware, including any small untidiness in the original -- it is what makes it read as real.

## 4. Parts list

To be completed from the pak census of `misc_model`, `func_static` and texture names in `tour/deck11` and
the campaign's engineering maps, so every piece placed is an object that already exists in the game.
Anything not in that list does not go in.

## 5. Spatial program

- **Footprint**: a working half-deck, two chambers rather than one.
- **Entries**: main door to the corridor; **Jefferies tube exit**, low and awkward, on the opposite side --
  the tube is what makes this room reachable when the turbolifts are down, which is a scenario we have
  already designed.
- **Circulation**: enter at one end, the plant is visible immediately behind a rail, the watch station is
  off to one side with sightlines to both the door and the plant. You should be able to come round a corner
  and be surprised.
- **Verticality**: a raised plant deck and a lower walkway, so the room reads as machinery rather than a
  floor with boxes on it.
- **Chokepoint**: the door, and the tube mouth. Both matter when someone is holding this space.
- **Posts**: one watch post at the console (seated or standing), one roving maintenance position.

## 6. Lighting brief

Harsh working light over the plant, warmer at the watch station, and console glow as a third source.
**Emergency lighting state required**: this is the room whose failure darkens other decks, and the red
state should be visible here first.

## 7. The hook

- **System node**: `SYS_LIFE_SUPPORT`, with this room as its location -- and the whole point is that its
  state is *reported somewhere else*: the bridge, the affected deck, the crew's quarters.
- **Compartment row**: `controller` crew/contested/sealed as normal; `compromise` matters here, because a
  sliced environmental control is a room that can empty a deck of air.
- **Job sites**: maintenance (the recurring small work), repair (after failure), reclamation (if the Borg
  reach the plant).
- **Control surfaces**: the watch console, plus a manual override that only engineering authority may use
  -- two people, one decision.
- **Crew posts**: one on watch, one roving, rotating across the three watches.

## 8. Acceptance checks

Affordances with navigation furniture at standing distance for both posts; no dead ends; `.nav` bakes;
post coverage holds in the crew harness; frame time holds. Then the owner walks it and answers one
question: **does it look like somewhere a person watches over fifteen decks of air, or like a box with a
console in it?**

## 9. Non-goals

No new art. No new species. Do not build the atmosphere plant accurately -- it needs to *read* as plant, not
simulate it. Do not build section 42 or the Haunting entity here; that is a scenario next door, and putting
it in this room would spend the adjacency.
