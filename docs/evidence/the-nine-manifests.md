# Evidence: the nine manifests — the slate as declarations, and what Wave 3 must contain

**What this is.** `docs/scenario-atlas.md` designs a first slate of nine scenarios; `docs/evidence/the-manifest-contract.md`
built the declaration contract one of them uses. This pass authors **the other eight as manifests** —
`scenarios/*/scenario.json` — so the slate is a **specification the systems can be derived from** rather
than prose that systems are guessed against. The nine are data; nobody plays a manifest. Authoring them
before the systems exist is cheap and it tells Wave 3 exactly which primitives to contain, and in what
order.

**Host:** static. The manifests validate and build here; the rendered surfaces are the playtest host's.

## 1. The nine, and each validates and builds

```sh
for d in scenarios/*/; do python3 tools/validator/validate.py "$d"; python3 tools/scenario/scenario_build.py "$d"; done
```

```
deck01-eleven-minutes: errors=0   11 field(s) declared
deck01-the-other-captain: errors=0   11 field(s) declared
deck06-organising: errors=0   11 field(s) declared
deck08-price-of-fuel: errors=0   11 field(s) declared
deck08-the-shortcut: errors=0   11 field(s) declared
deck09-the-ones-who-stay: errors=0   11 field(s) declared
deck11-gel-pack: errors=0   11 field(s) declared
deck12-a-fire-a-breach-a-deck: errors=0   11 field(s) declared
deck15-nineteen-relays: errors=0   11 field(s) declared
```

`scripts/scenario-check.sh` (part four) validates and builds **every** manifest under `scenarios/`, so a
manifest that stops validating — or a tenth that is malformed — fails the suite. It requires at least the
nine, so none can be dropped silently.

| # | scenario | canon root (the atlas's slate) | home deck |
|---|---|---|---|
| 1 | `deck11-gel-pack` | the gel pack fever | 11 |
| 2 | `deck15-nineteen-relays` | nineteen relays — the EPS runs to the turbolift network | 15 |
| 3 | `deck08-price-of-fuel` | the price of fuel — deuterium low, and the species who owns the source | 8 |
| 4 | `deck09-the-ones-who-stay` | a world offers a life; a crew record leaves for good | 9 |
| 5 | `deck08-the-shortcut` | a route that cuts years off, at a structural cost | 8 |
| 6 | `deck01-eleven-minutes` | a live link home, and command changes | 1 |
| 7 | `deck01-the-other-captain` | a Starfleet crew who survived by monstrous things | 1 |
| 8 | `deck12-a-fire-a-breach-a-deck` | decompression, a bulkhead sealed with crew on the wrong side | 12 |
| 9 | `deck06-organising` | the Borg aboard, unopposed, the clock running | 6 |

Each declares **all eleven** contract fields: `canon_root`, `breeding_conditions`, `systems`, `staff`,
`cost`, `residue`, `visibility`, `warmth`, `positions`, `decision`, `lines`. The two authoring laws hold:
no line, label, decision or residue names a person; every line declares a delivery direction.

## 2. The derivation — what Wave 3's primitive set must be

This is the whole point of authoring the manifests first: **the slate, not the register's order, decides
Wave 3.** Aggregated across the nine:

**Systems read and written (16 of the 23):** life support (5 scenarios), sensors (3), shields (3),
replicators (3), structural integrity (3), phasers (2), warp drive (2), sickbay (2), turbolifts (2),
communications (1), torpedo launchers (1), computer core (1), impulse drive (1), navigational deflector
(1), holodecks (1), gravity plating (1). **All exist** in `SPECS[]`.

**Posts staffed (9):** engineering (6), operations (4), medical (4), the chair (3), security (3), command
(2), the conn (1), support (1), sickbay (1). **All exist** as systems, departments or the command seats.

**Effects the cost, ignore and residue write (11 of the 13 kinds):** `sicken` (10), `damage_system` (10),
`write_off` (4), `shield_hit` (3), `ignite` (3), `promote` (2), `take_survivors` (2), `air_source` (2),
`kill` (2), `fabricate_parts` (1), `breach` (1). **Every one maps onto a primitive the contract already
names** (`docs/evidence/the-manifest-contract.md`, §3).

**Visibility (5 stations + 5 decks):** engineering, operations, sickbay, conn, tactical; decks 6, 9, 11,
12, 15. **Warmth (7 beats):** manner, funeral, promise, wall, rations, quarters, holodeck.

**The finding, and it is the reason to author the slate first.** **The nine scenarios read no primitive
that does not already exist.** The register's ~14 absent systems — per-faction reputation, first-contact
tech level, the board, research beyond dilithium — are *not read by this slate*: they belong to the
atlas's later content, not to Wave 3. So Wave 3's contents are the **enabling primitives the evaluator
needs to fire and resolve what the nine declare**, not a system per absent register row:

1. **Per-deck residue** (the one `residue` field mid-switch needs) — the deck a residue *names*.
2. **The option-cost application and the ignore-branch** — the `cost.answer`/`cost.ignore` effects
   resolved by the evaluator.
3. **The voice seam for a declared line** — the `lines` spoken, not only logged.
4. **The reporting rule** — the `staff`/`positions` chain a scenario's decision travels.
5. **`WorkReading` at the post** — the visibility the slate declares at each post.
6. **The condition-subject shape** — so `system_condition`, `deck_life_support` and `minutes_of_air`
   can name *which* system or deck, not only the ship's worst.

Each Wave 3 lane is **tagged with the scenarios that read it**:

| Wave 3 lane | the scenarios that read it |
|---|---|
| per-deck residue | 8 (`a-fire-a-breach-a-deck`), 9 (`organising`) |
| option-cost + ignore | all nine |
| the voice seam for a declared line | all nine |
| the reporting rule | 2, 3, 6, 7, 8, 9 |
| `WorkReading` at the post | 2, 5, 8 |
| the condition-subject shape | 2, 8, 9 |

## 3. The evaluator's own gaps, confirmed by the slate

The seven new genuine gaps the re-triage found (`docs/evidence/the-retriage.md`) are **not** read by this
slate — the Collective's reads and `SetAirSourceBy` belong to later content — so Wave 3's head is the six
above, and the seven are the tail, tagged to the scenarios that will read them (a Borg scenario that
addresses the Collective; a delegated-air scenario).

## 4. What this does not do

- **It does not play a scenario.** The nine are declarations; one scenario end to end is Wave 4, and it is
  the owner's verdict on whether a manifest *reads as a situation* that closes that.
- **It does not build Wave 3.** Each lane above is its own branch and its own evidence, in the order this
  table gives.
- **The `crew` blocks are placeholders.** Each manifest carries the example's four-member crew block so it
  reuses a map the installation has (the `E005` reuse path); the real crew section is set when the scenario
  is authored against a built deck.

## 5. The registers this pass moves

- `docs/omissions.md` **O15** — the nine declarations are authored; the nine scenarios remain the content.
- `docs/scenario-atlas.md` — the slate records its manifest names.
