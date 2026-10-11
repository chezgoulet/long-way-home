# The holdings fold: the allegiance ruling's unfinished half

`docs/morale.md` (the ruling, C2), `docs/affinities-and-allegiance.md` (the bond fold and *"No separate
relationship table"*), `docs/the-record-and-the-log.md` (purge), `docs/gates.md` (residual C2) and
`module/ship/ship_core.{h,cpp}`, on `feat/the-holdings-fold`, cut from `testing`, 2026-10-11.

**The ruling in one line, and the half it left.** The owner ruled the allegiance read *derived*, not
stored. The check that followed found the ruling splits in two: `Bond(s, a, b)` was *already* a fold
over the marks, and the design already said so twice — so the per-pair bond needed no change. **The
half that was not done was `holdings`**, one of morale's three components, which was a stored `float`
nudged by events (`RISING_HOLDINGS_RISE = 0.12f`). This pass makes `holdings` the same fold as `Bond`,
removes the stored field and its save slot, and demonstrates the consequence the ruling named rather
than discovered: **a purge moves regard.**

**Observed, and the command that produced it.** Everything below is the output of a command named
beside it, on this tree. `scripts/test.sh` compiles and runs the unit tests (`test_ship_core`); the
module build (`scripts/build-ship.sh`) is the engine side and is named where it was run.

## What was built, and where

One fold, in `module/ship/ship_core.{h,cpp}`, beside the bond fold it copies.

- **`Holdings(const CrewMember &c)`** — the fold: `holdings = clamp01(HOLDINGS_BASE + HOLDINGS_SLOPE ×
  clamp(Σ valence × salience, −1, +1))` over the marks the person carries. It is `Bond` widened from one
  name to all of them and mapped from ±1 to 0..1. `HOLDINGS_BASE = 0.5f` is the person with no marks:
  neither held nor lost; `HOLDINGS_SLOPE = 0.5f` spans the range.
- **The orphan rule.** A mark whose citation a purge removed (`m.orphaned`) is still held but carries
  no weight in the sum — so a purge moves regard. This is the one place the fold reads a field `Bond`
  does not read, and it is the ruling's named consequence.
- **`Morale(c)`** reads `Holdings(c)` in place of the field; `MoraleReason(c)` likewise.
- **Removed:** `CrewMember::holdings`, its seeding in `DeriveCharacter`, the tick's `holdingsTarget`
  easing, the `c.holdings` writes in the funeral, holodeck therapy and rising-witness paths, the save
  slot in `Pack`/`Unpack`, and the constant `RISING_HOLDINGS_RISE`. The two harness writes in
  `module/ship/g_ship.cpp` went with the field.
- **The rising rule is unchanged in intent and now works through the mark.** `RisingWitnesses` already
  wrote a `MEM_RESCUE` mark naming the hero (`RISING_WITNESS_BOND = 0.7f`); the fold reads it, so the
  occasion moves regard *because* it leaves a mark — and the mark's salience decays unless retold,
  which keeps the old "until reinforced" behaviour through memory rather than a stored step.

## The fold, and its test

`TestHoldingsIsAFold` (`tests/ship/test_ship_core.cpp`) works on a bare `CrewMember`, no ship: the fold
reads the person's own marks. It shows regard rise with repeated good marks, fall with bad, and stop
counting a faded or orphaned one — from the marks alone, with no stored value.

```
$ scripts/test.sh          # ... test_ship_core ...
  holdings fold: no marks 0.500, one good mark 0.700, two good 0.850, with a bad mark 0.350,
                 bad mark faded 0.600, bad mark orphaned 0.600
```

The exact arithmetic is checked against `HOLDINGS_BASE + HOLDINGS_SLOPE × valence × salience` on the
first mark, so the shape is pinned, not only its direction.

## The storage is gone

**The search.** `tools/ship/check_holdings.py` scans `module/ship/` for a stored field — a declaration
(`float holdings`), a member read or write (`.holdings`, `->holdings`), or an assignment — in code
only (comments and string literals stripped, so the save changelog's prose about the retired field is
not a hit):

```
$ python3 tools/ship/check_holdings.py module/ship
    no stored holdings field in module/ship
```

**The struct, before and after.** From `module/ship/ship_core.h`:

```
before   float holdings = 0.75f;  // 0 alone and blaming .. 1 held: bonds, allegiances
after    (no field; Morale() reads Holdings(c), the fold over the marks)
```

**The save slot, before and after.** From `module/ship/ship_core.cpp`:

```
before   w.F(c.deficit); w.F(c.outlook); w.F(c.holdings);          // Pack
after    w.F(c.deficit); w.F(c.outlook);

before   c.deficit = r.Unit(); c.outlook = r.Unit(); c.holdings = r.Unit();   // Unpack
after    c.deficit = r.Unit(); c.outlook = r.Unit();
```

`SAVE_VERSION` is now **57** (`module/ship/ship_core.h`), and the character-layer round-trip tests
(`TestMoraleAndFatigue`, `TestCharacterSaveRoundTrip`) check that `Holdings(u.crew[i])` still reads
what it read before the save, since the marks came back with the record.

**The guard, in both directions.** `scripts/holdings-check.sh` runs the scan on the folded tree (must
pass) and on a copy with a planted stored field (must fail) — a guard that cannot fail for the reason
we care about is decoration:

```
$ scripts/holdings-check.sh
==> holdings: no stored field in module/ship (the fold, not a float)
    no stored holdings field in module/ship
    the plant is caught:
      FAIL  .../ship/ship_core.cpp:9647: ... { s.crew[0].holdings = 0.5f; }
      1 stored-field reference(s) found; holdings must be a fold

PASS  holdings is a fold; no stored field exists, and the guard catches one that is planted
```

`scripts/holdings-check.sh` is wired into `scripts/test.sh`, so CI runs both directions every time.

## The consequence: a purge moves regard

`TestPurgeMovesRegard` gives a crew member two positive marks — one witnessed (`MEM_SAW`), one read in
the published log (`MEM_LOG`) — publishes the log, and purges it. The mark is not erased (the crew do
not forget); its citation is, and with it the weight it held on regard:

```
$ scripts/test.sh          # ... test_ship_core ...
  regard before the purge 1.000, after 0.750 (the MEM_LOG mark is held but orphaned)
```

1.000 is the two marks summing to +1.0 (clamped); 0.750 is the witnessed mark alone (`0.5 + 0.5 ×
0.5`). The check also requires the mark to survive the purge (`MemoryCount == 2`, `Recall` true), so
what fell is the *weight*, not the memory — which is exactly `docs/the-record-and-the-log.md`'s
"orphans rather than erases", read by morale.

## The rising rule still works

`TestRisingCostAndWitnesses` was already in the tree; this pass strengthens it to prove the path is the
mark, not a number. The witness's regard rises by the act, and **dropping the mark's salience returns
it to where it began**, then restoring it brings it back:

```
	const float held = Holdings(f.s.crew[f.w1]);
	CHECK(held > holdBefore);                      // the occasion moved regard
	// zero the witness mark ...
	CHECK(std::fabs(Holdings(f.s.crew[f.w1]) - holdBefore) < 1e-4f);
	// restore it ...
	CHECK(std::fabs(Holdings(f.s.crew[f.w1]) - held) < 1e-4f);
```

## The save format, and what it costs

The per-person float left `CrewMember` and the save, so `SAVE_VERSION` is **57** and an older save
does not load. **That is the deliberate cost, and it is named rather than discovered**: `AGENTS.md`
says a change that makes a save load differently is a defect, not a detail, and the bump is the
deliberate version of it. `docs/morale.md` carries the same statement to the player-facing side. The
alternative — reading a 56 save by skipping the retired float — was not taken: the record is not meant
to carry a number that no longer exists, and the save is a start state, not a migration contract.

## The checks, and what they returned

```
$ scripts/test.sh        -> all checks passed (crew + ship unit tests, tools, hooks, api, orders,
                            harness, docs, holdings, python, shellcheck, patches)
$ scripts/check.sh       -> (G0, entity dictionary + script round-trip; see its own output)
$ scripts/docs-check.sh  -> all three classes agree (owned numbers: save-version 57)
```

The unit suite now prints the two new transcripts above; `scripts/holdings-check.sh` prints the
guard's two directions.

## What could not be verified

- **Whether the numbers *feel* right in play.** `HOLDINGS_BASE` (0.5), `HOLDINGS_SLOPE` (0.5), and the
  shift the fresh crew reads (a person with no marks is 0.5, where the retired field began around
  0.75) are `[inv]`, small, and recorded so the owner can overrule them. **This is the owner's
  walkthrough**, and no engine run was made (the brief forbids one on this host).
- **A hold/fold at scale across a long run.** The fold is checked on constructed marks and on the
  rising and purge paths; a full-run distribution of regard is not measured here, and no game data or
  engine was used.
- **The rising harness on the engine.** `scripts/rising-check.sh` needs the game and was not run; the
  C++ path it drives is covered by `TestRisingCostAndWitnesses`, and `module/ship/g_ship.cpp` no longer
  writes a stored holding.

## Judgement calls, named as calls

1. **The fold is over *all* the person's marks, not only those naming the captain.** `holdings` is
   morale's "who they hold with: bonds, grudges, allegiances", and the rising witness is a peer, not
   command — so a peer heroism must move it, which it does only if the fold is the sum over all marks.
   `Bond(a, b)` is the same fold narrowed to one name. **This is a call.**
2. **An orphaned mark is held but weightless.** The design says a purge orphans rather than erases, and
   names "purging the records moves regard" as the consequence; the only way both are true is for the
   fold to give an orphaned mark no weight while the mark remains for `Recall`. **This is a call.**
3. **The mapped range is the whole 0..1** (`HOLDINGS_BASE 0.5`, `HOLDINGS_SLOPE 0.5`), so a person can
   still read "alone and blaming" (0) and "held" (1), and the weighted morale mean keeps its shape.
   **This is a call**; a base of 0.75 would have matched the retired field's start but could not reach
   0, and the brief's own model reaches both ends.
4. **The seed no longer colours a fresh person's holdings.** With the field gone, a person with no
   marks reads the same base for everyone; outlook and the marks are what differentiate a fresh crew.
   The RNG draw that used to seed it is kept and discarded, so the seed's *later* derivations (species,
   traits, the face) are unchanged by this ruling. **This is a call**, and the one place a player might
   notice the difference at first glance.
5. **`RISING_HOLDINGS_RISE` is deleted, not repurposed.** The design's intent — "the regard moves with
   it" — is carried by the witness mark's valence, which the fold reads; keeping a second constant that
   nudged a float would be the stored value in disguise. **This is a call.**
