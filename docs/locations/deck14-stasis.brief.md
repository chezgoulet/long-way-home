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

**Blockout building** — stasis pods along a wall and a holodeck doorframe, reachable on the merged ship
(`scripts/deck-check.sh 14`).
