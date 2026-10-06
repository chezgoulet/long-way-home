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

**Blockout building** — a central auxiliary core column and two cargo islands, reachable on the merged
ship (`scripts/deck-check.sh 7`).
