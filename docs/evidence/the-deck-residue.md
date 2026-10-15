# Evidence: the deck's residue — what the place permanently is now (slate 8)

**The finding this closes.** `docs/the-derivation.md` marked the **residue** field the one declaration
field that was **not ready**: reconciliation **7.8** found that "a reclaimed deck is fully restored", so a
scenario's residue could not name *the place* it changed, and `docs/scenario-atlas.md`'s slate 8 — *"a
sealed deck you can visit later and see what was left behind"* — had nowhere to be written. This pass
builds the missing primitive: the deck's **residue**.

**Host:** static. The state and its save were driven here (the unit suite and `--residue`); the world that
dresses a deck from the residue is the playtest host's.

## 1. What it is

A deck carries a **residue** — `RES_SEALED`, `RES_STRIPPED`, `RES_UNINHABITABLE` or `RES_WRITTEN_OFF`,
`RES_NONE` by default — kept **apart from the live state** beside it. Hull, atmosphere, fire and
assimilation all come back; the residue is what stays, and nothing in the model clears it. That is the
whole distinction: the *place* is not the *moment*.

```c
enum DeckResidue : uint8_t { RES_NONE = 0, RES_SEALED, RES_STRIPPED, RES_UNINHABITABLE, RES_WRITTEN_OFF, RES_COUNT };
uint8_t DeckResidue(const Ship &s, int deck);      // what the place is now (1..DECKS)
const char *DeckResidueName(uint8_t r);            // none, sealed, stripped, uninhabitable, written off
```

## 2. Who writes it, and it is one place

- **A write-off.** `WriteOff` already gave a deck up (the written-off list, `docs/damage-and-budgets.md`);
  the residue mirrors the loss kind, so a deck sealed and left keeps `RES_SEALED`, one stripped keeps
  `RES_STRIPPED`, one marked uninhabitable keeps `RES_UNINHABITABLE`, and a plain write-off keeps
  `RES_WRITTEN_OFF`.
- **A fired scenario's residue.** The loader's `ApplyEffect` already calls `WriteOff` for a deck
  (`module/ship/scenario_core.cpp`), so a scenario whose `residue` names a deck — slate 8 writes off
  *deck 12* — sets the residue through the one path, not a second one.

## 3. The demonstration

```sh
scripts/deck-residue-check.sh
```

```
PASS  deck 7: residue 'stripped'
PASS  deck 8: residue 'sealed'
PASS  deck 12: residue 'written off'
PASS  reloaded: deck 8 'sealed', deck 12 'written off', deck 7 'stripped'
```

`TestDeckResidue` asserts the empty default, each kind's mapping from the write-off, that a system's
write-off is **not** a place, the out-of-range read, and the round-trip; `test_ship_core --residue` is the
transcript above.

## 4. The falsification

The check is exercised **both directions**: it passes on the tree, and it **fails** on a plant that drops
the residue the write-off sets (the deck given up then keeps nothing, and `--residue` reports `RES_NONE`):

```
==> deck residue: the falsification -- drop the residue the write-off sets
PASS  the planted break fails the check, so the check has teeth (restored)
```

## 5. The save

The residue is part of the deck and round-trips. **Save format 66** — the cost is named in the header's
version line: an older save (65) does not load. A save taken after a deck is sealed or written off now
carries the place it left behind.

## 6. What this does not do, said plainly

- **It does not dress the world.** The residue is the **state** the world dresses a deck from; the presenter
  that turns `RES_SEALED` into a sealed hatch, or `RES_WRITTEN_OFF` into a stripped room, is the environment
  layer on the playtest host (`g_env`, `SyncDamage`/`SyncFire`'s pattern). This pass gives it something to
  read.
- **It does not clear.** Nothing restores a residue to `RES_NONE`, by design — a deck does not come back
  whole. If a future repair should lift it, that is a decision, and it belongs in this file when it is made.

## 7. The registers this pass moves

- The re-triage's Wave 3 lane **per-deck residue** is built; the scenarios that read it are 8
  (`a-fire-a-breach-a-deck`) and 9 (`organising`) (`docs/evidence/the-nine-manifests.md`).
- `docs/hook-register.md` gains `DeckResidue` (play) and `DeckResidueName` (console); the counts marker
  moves to 498, guarded by `tools/docs/check_register_counts.py`.
