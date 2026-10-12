# Evidence — the sector honours its stated split (C-4)

Date: **2026-10-12**. Branch `feat/the-sector-split`, cut from `testing`. **Static host (`mikoa`): no
engine, no game data.** The witness is `tests/ship/test_ship_core.cpp`, built and run by `scripts/test.sh`;
the rendered game is not in reach here.

Where the ruling is. `docs/designs-against-code.md`'s C-4 row is the authority, and the owner's ruling sits
on the documentation branch `the-second-rulings` (commit `b4cdd33`), not on this branch: here the C-4 entry
still ended at the code pass's **handed back**. This pass carries the ruling into the row on this branch —
*"honour the stated split (÷10), and add the four kinds to the ledger"* — and implements it.

## The work

1. `BuildSector` (`module/ship/ship_core.cpp`) rolls `next() % 10` for the four headline classes, so the
   interior beacons carry **4/3/2/1 of ten** — the ledger's own 40/30/20/10. Beacon 0 stays forced empty and
   beacon 11 stays the goal.
2. The four kinds `docs/lore-ledger.md` omitted — **trader, distress, belt, pre-warp** — are now recorded
   there, each with its share and where it came from.
3. The one test that encoded the old roll is updated, and its old claim is named (below).

## The shape, and why — the judgement call

**The four kinds do not fit a ten-way split, and the brief said to say so.** Four plus three plus two plus
one is already all ten: `% 10` leaves no slot for a trader, a distress call, a belt or a pre-warp world, and
the ruling — *honour the split* — does not cover reconciling the two. Left as `% 10` alone they would be
dead code, produced by nothing.

The shape chosen: **the four kinds are drawn from the roll's empty outcome.** A ten-way roll decides the
four headline classes; when it lands on empty, a second `% 10` roll leaves six parts in ten genuinely empty
and makes one part in ten each a trader, a distress call, a resource belt and a pre-warp world. So:

- the four headline classes keep the ledger's **exact** 40/30/20/10, and the 40% is the **quiet** share — a
  beacon with no threat and no wreck, which these four contacts also are; and
- each of the four kinds is **4% of the interior beacons** (1/10 of the 40%), and 24% is genuinely empty.

Alternatives considered and not taken: diluting the four headline classes to make room (that fails the
ruling's own numbers); a fifteen-outcome roll keeping 4/3/2/1 *of the headline group* (that is the old
`% 15` roll, and its absolute shares are 26.7/20/13.3/6.7, not the ledger's); and treating the four as an
overlay field beside `kind` (it would change the save format, and `docs/evidence/the-code-contradictions.md`
records that no fix here moves it). Verifying the empty share still hosts the sector's phenomenon: at 24%
empty, the expected number of empty interior beacons is 2.4 a sector, so the first-empty-beacon phenomenon
placement is unaffected.

## The split, demonstrated

`tests/ship/test_ship_core.cpp`, `TestSectorSplit`: 2000 sectors, 20000 interior beacons, counted by kind.
The line below is the test's own output, printed by the check itself.

```
$ cmake -S tests/ship -B build/ship -DCMAKE_BUILD_TYPE=Debug && cmake --build build/ship -j
$ build/ship/test_ship_core | grep 'sector split'
  sector split over 20000 interior beacons: quiet 40.31% (empty 24.42, trader 3.91, distress 4.06, belt 3.93, pre-warp 4.00), hostile 29.64%, derelict 20.02%, borg 10.03%
```

Beside the ledger's stated shares, and beside what the old `% 15` roll produced (the same measurement, run
against the unfixed roll for the falsification below):

| what | ledger states | now measured | old `% 15` roll |
|---|---|---|---|
| quiet (the empty class) | 40% | **40.31%** | 59.82% |
| hostile | 30% | **29.64%** | 20.44% |
| derelict | 20% | **20.02%** | 13.04% |
| Borg | 10% | **10.03%** | 6.70% |
| — of the quiet share: | | | |
| genuinely empty | 24% | **24.42%** | 26.68% (empty was 4/15) |
| a trader | 4% | **3.91%** | 6.67% |
| a distress call | 4% | **4.06%** | 6.74% |
| a resource belt | 4% | **3.93%** | 6.57% |
| a pre-warp world | 4% | **4.00%** | 13.17% |

The four classes sum to 100% of the interior; the four kinds sum to the 40% quiet share. Sampling error at
20000 beacons is under half a percentage point a class, which is why the check's tolerance is ±2 points —
several standard deviations, and an order of magnitude tighter than the gap to the old roll.

## The four kinds, in the ledger

`docs/lore-ledger.md`, *The outside* table, the row *A sector* rewritten and four rows added after it: each
names its share (**4% of the interior beacons**) and its provenance (**drawn from the roll's empty (quiet)
outcome**). The mechanical detail — what a belt yields, what a pre-warp contact pays — stays in its existing
rows; the new rows carry the share and the provenance, which is what was unrecorded.

## The tests: what changed, and the old claim each made

- **`TestSectorSplit` — added.** It is the check that catches the roll, and the falsification below points at
  it. It asserts the four classes at the ledger's 40/30/20/10 (±2 points), each of the four kinds at 4%, that
  all four are reachable, and that the ends (beacon 0 empty, beacon 11 the goal) do not move.
- **`TestNavigation`, the `Pack(back) == blob` check — updated.** The old test saved a ship *immediately*
  after jumping to beacon 1 and asserted the blob round-tripped through `Unpack`. **Its old claim, named:**
  *beacon 1 is not hostile for this seed.* Under the old `% 15` roll that happened to hold, so no contact was
  owed and the round-trip was clean. Under `% 10` beacon 1 is a hostile beacon: `Unpack` ends with
  `Tick(s, 0.0f)`, the derive fires, and with our shields down at a green alert the enemy's boarders land and
  write a log line (`Board`, `ship_core.cpp`), so the reloaded blob was larger than the saved one and the
  check failed. The fix clears the incidental contact after the jump, so the round-trip is the counter's, not
  the beacon's. **Nothing about the counter's own claim changed.**
- **`TestSector` and `TestCombat` — checked, not changed.** `TestSector` guards its derelict checks with
  `if (derelict > 0)` and `TestCombat` asserts only that a hostile exists somewhere (`CHECK(hostile > 0)`),
  which still holds — hostile is 30% of the interior. No other test encoded the old roll; the full suite
  passes with only the two above touched.

## The falsification

Break the roll back to the old `% 15` (restore the four-kind ternary at rolls 10–14 in `BuildSector`),
rebuild, run:

```
$ build/ship/test_ship_core | grep "sector's split\|check(s) failed\|sector split over"
  sector split over 20000 interior beacons: quiet 59.82% (empty 26.68, trader 6.67, distress 6.74, belt 6.57, pre-warp 13.17), hostile 20.44%, derelict 13.04%, borg 6.70%
FAIL  the sector's split: std::fabs(quiet - 0.40f) < 0.02f (line 1522)
FAIL  the sector's split: std::fabs(share(BEACON_HOSTILE) - 0.30f) < 0.02f (line 1523)
FAIL  the sector's split: std::fabs(share(BEACON_DERELICT) - 0.20f) < 0.02f (line 1524)
FAIL  the sector's split: std::fabs(share(BEACON_BORG) - 0.10f) < 0.02f (line 1525)
FAIL  the sector's split: std::fabs(share(BEACON_TRADER) - 0.04f) < 0.02f (line 1527)
FAIL  the sector's split: std::fabs(share(BEACON_DISTRESS) - 0.04f) < 0.02f (line 1528)
FAIL  the sector's split: std::fabs(share(BEACON_BELT) - 0.04f) < 0.02f (line 1529)
FAIL  the sector's split: std::fabs(share(BEACON_PREWARP) - 0.04f) < 0.02f (line 1530)
8 check(s) failed
```

Eight of the check's rows fail, every one of them. Restore `% 10` and the second roll: `ship_core: all checks
passed`. The check has teeth, and it fails for the reason the row is about.

## What could not be verified

- **The rendered game.** This is a static host with no engine and no game data; the split is a simulation
  property with no screen of its own, so the unit tests are the whole witness here.
- **A pre-pass save.** The sector's *shape* is not stored: `Pack` writes the seed and only what the ship has
  met (`sector[i].visited/surveyed/looted`, the phenomenon's revealed attributes, `ship_core.cpp` `Pack`),
  and `Unpack` regenerates the sector from the seed (`BuildSector(s, 0)`). So a save written before this pass
  loads a **different** sector under the new roll. The save **format** does not move — `SAVE_VERSION` stays
  **60**, no field is added — but the composition it regenerates is the ruling's change, and it is named here
  rather than hidden. (`docs/evidence/the-code-contradictions.md` Part 5 records that no fix here moves it.)
- **A latent save fixed point, exposed and left alone.** The navigation test's failure is not only this
  seed's layout: `Unpack`'s trailing `Tick(s, 0.0f)` is not idempotent for a ship saved standing at a
  green-alert hostile beacon with boarders aboard — the derive writes a boarding log the saved state did not
  carry, so `Pack(Unpack(blob)) != blob`. This is pre-existing (it would fail under the old roll too for a
  seed with a hostile at the saved beacon) and **out of this brief's scope** — it is a save-replay question,
  not the sector split. Recorded so it is not lost.

## The judgement calls, in one place

1. **Where the four kinds live.** Drawn from the empty outcome, recorded as the quiet share's occupants — the
   only shape that keeps the four headline classes at the ledger's exact 40/30/20/10 *and* produces all four
   kinds. Recorded in `docs/lore-ledger.md` and in the code comment at the roll.
2. **Their shares.** One part in ten of the empty outcome each (4% of the sector), six parts left empty. The
   old shares were unequal (pre-warp took two of the five extra outcomes); the even split is the new design
   and is stated as such.
3. **The empty share's name.** The ledger's "empty" is now "quiet" — the class, of which genuinely-empty is
   the largest part. This is the visible correction of the row: "40% empty" could not remain literally true
   once four non-empty kinds were drawn from it.
4. **The navigation test's update.** Clearing the incidental contact, rather than hunting a seed whose
   beacon 1 is empty, keeps the test about what it names.

## The register marker

The C-4 row in `docs/designs-against-code.md` is marked closed and carries
`<!-- register-row id=C-4 state=closed marker=symbol:tests/ship/test_ship_core.cpp:TestSectorSplit -->` — the
marker resolves to the check that holds the split, so `scripts/docs-check.sh` fails if the check is renamed
or removed.
