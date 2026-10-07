# Power assignment — what was built and what was measured

`docs/power-assignment.md` (the owner's ruling of 2026-10-07), applied on `feat/power-assignment` (cut from
`testing`), 2026-10-07.

**Observed, and the command that produced it.** Every figure below is the output of a command named beside it,
on this tree. Save format is **51**: the new per-system fields and the ship's mode, shortfall and grants append
to the blob, so older saves are invalid — the cost named in the brief.

## The allocation model as built

One model, in `module/ship/ship_core.{h,cpp}`. Every system now carries two things a person sets, plus the
provenance of the second:

| field | what it is |
|---|---|
| `System.share` | the allocation: the fraction of the system's full demand it is given, 0..1 |
| `System.enabled` | online or offline (unchanged) |
| `System.allocBy` | who set the share: `ALLOC_UNSET`, `ALLOC_PLAYER`, `ALLOC_DELEGATE`, or derived `ALLOC_AUTO` |
| `System.allocCrew` | the officer who set it under a delegation, or -1 |

Operational capacity follows the number: `output = health × (allocated / full demand) × manning`, so **a system
at forty per cent delivers forty per cent** — almost-on is a real state, not a category.

The allocator (`UpdatePower`) was rewritten. Manual mode (the default) honours every commitment in full and
**sheds nothing**; automatic mode honours the systems a person has set first, in full, and applies the ladder
(by `System.priority`) only to the systems nobody has set. The one override is damage — a destroyed system, a
dead conduit, no fuel — and it is written to the log as `"<system> is dark: damage, not a decision"`.

The ship carries `powerAuto` (off by default), `lastShortfall`, and `std::vector<BandGrant>` (the band
delegations). `PowerCommitted()` is the sum the commitments ask of the plant; `PowerShortfall()` is
`max(0, committed − available)`.

## The proving case, demonstrated

`test_ship_core --power` (the readout added for this evidence). The acceptance that matters — the holodecks at
full while the shields are dark, and the reverse, neither overridden by any ladder:

```
== the proving case: the player decides, and no ladder overrides it
  holodecks 100%  shields   0%   -> holodecks output 100%, shields output   0%  (provenance: the player)
  holodecks   0%  shields 100%   -> holodecks output   0%, shields output 100%  (provenance: the player)
```

The ladder would keep the shields (keep-priority 7) and shed the holodecks (20); the player's decision stands in
both directions, because in the default manual mode there is no ladder to apply.

## The ladder fires only in automatic mode

The acceptance to the letter — set an allocation, turn automatic mode off, change nothing else, and the ladder
does not fire:

```
== the ladder fires only in automatic mode, and never on a system a person has set
  AUTO on  at 0.55: cargo handling 0/20, holodecks 36/60 (the player's)
  AUTO off, nothing else changed: cargo handling 20/20, holodecks 36/60, short 536
```

With automatic mode on at a crystal ceiling of 0.55 the ladder sheds the cargo handling (an unset system) and
leaves the player's 60 % holodeck allocation alone. Turned off, with nothing else changed, the cargo handling is
fed again and the 536-EPS shortfall is reported. `TestLadderOnlyInAutoMode` asserts all three.

## Oversubscription is reported, never resolved

```
== oversubscription is reported, never resolved
  plant 820 EPS, committed 1730, SHORT 910; systems allocated 23 of 23, dark 0
```

`TestOversubscriptionReported` (with the plant aged to a 0.30 ceiling) asserts the shortfall is `1730 − 820`,
that every system keeps its allocation, and that the log carries `"power commitments exceed the plant by … EPS;
nothing is shed"`. It also asserts the console refuses an increase that would not fit and allows the decrease
back.

## The console (Task B), measured

`scripts/power-check.sh` — one headless engine run, `g_shipTest 71`, every change a key press on the
Engineering screen:

```
==> the allocation console (g_shipTest 71)
    SHIP: allocation test: life support 80%, automatic mode 0, committed 1338 of 1338, short 0
PASS  the Engineering console set an allocation key by key and showed committed against available
      screenshot: build/g3-home/baseEF/screenshots/lwh_power.tga
```

**Read the rendered thing, not the code that draws it.** The screenshot shows, in one list, every system's name,
its online/offline state, the **share** a person set (`SET`), the **power it is getting** (`POWER GETTING`,
EPS and a bar), **what that power buys** (`BUYS`, a percentage), and **who decided** (`WHO`: `PLAYER`, `AUTO`,
`UNSET`). The header carries `COMMITTED 1338 OF 1338`, the budget, and `AUTOMATIC MODE` / `MANUAL`. The
screenshot was taken with automatic mode on, which is why the unset systems read `AUTO`.

The first render was **not** acceptable and was corrected: the header overflowed 640 px, the recommendation
overlapped the generation block, and the 23-row list ran through the footer. Rows were tightened to 13 px, the
header shortened, and the recommendation and controls moved to the foot. The layout is legible now; whether it
is *pleasant* is the owner's.

## Commands, and what they returned

```
scripts/test.sh
  -> all checks passed (crew, ship, tools, python, shellcheck, 19 patches)
     ship_core: all checks passed (TestThePlayerDecides, TestLadderOnlyInAutoMode,
     TestOversubscriptionReported, TestPartialAllocation, TestRecommendationAndDelegation among them)

scripts/check.sh --source-map build/gdk/maps/eliteforce_voyager_maps/voy1.map \
                 --script-corpus build/gdk/scripts
  -> entity dictionary 318 classes; validator negative tests ALL PASS;
     2,016/2,024 scripts compile and read back, 8 known rejections

scripts/s2-check.sh --map tour/deck04
  -> PASS the reloaded ship matches the saved one; PASS alert/damage/priority persisted;
     PASS the Engineering console changed the ship

scripts/power-check.sh --map tour/deck04
  -> PASS the Engineering console set an allocation key by key and showed committed against available

cmake --build upstream/efgame/build-linux
  -> libefgame.so and libefui.so built (only pre-existing -Wwrite-strings warnings)
```

Each unit test, and the assertion it carries:

| test | what it proves |
|---|---|
| `TestThePlayerDecides` | the proving case, both directions, provenance the player's |
| `TestLadderOnlyInAutoMode` | the ladder fires in automatic mode, and not after it is turned off |
| `TestOversubscriptionReported` | the shortfall as a number, nothing goes dark, the console refuses more |
| `TestPartialAllocation` | 40 % sets 24 of the sensors' 60 EPS and delivers output 0.40 |
| `TestRecommendationAndDelegation` | the recommendation and its mark; a band grant, held by a name, revoked at once |
| `TestSave` (extended) | the new fields round-trip: `Pack(back) == blob`, and the ship carries on identically |

## What could not be verified

- Whether the allocation screen is **pleasant to operate**, and whether choosing feels like command rather than
  bookkeeping. The brief reserves that for the owner's walkthrough. This lane only proves it is legible and
  operable key by key.
- **Nothing else.** The remaining acceptance items are asserted by the unit tests above, and the engine checks
  exit 0.

## Judgement calls, named as calls

1. **An over-committed plant is not physically conserved.** In manual mode, when the commitments exceed the
   plant, `PowerAllocated()` may exceed `PowerAvailable()`: the commitments are honoured and the shortfall is
   reported, exactly as `docs/power-assignment.md` asks ("reported, never resolved… nothing goes dark"). The
   conservation invariant (`TestPowerIsConserved`) now holds **in automatic mode**, where the allocator spends
   only what the plant supplies; `TestOversubscriptionReported` asserts the manual behaviour. The console makes
   this state hard to reach by hand — it refuses an increase that would not fit — so it arises from the plant
   falling under an existing commitment, which is the shortfall the player must clear.
2. **No fuel is the hard override.** A plant with no fuelled source left goes dark and the log says so, which is
   what makes "the batteries alone hold life support for three hours" still true when the cells are empty. The
   old test's silent selection of life support was itself the failure the brief names; it is now the player who
   sets life support to full and everything else dark, and the batteries hold it. `TestBatteriesKeepTheCrewAlive`
   was rewritten accordingly, and the arithmetic is unchanged.
3. **The budget bands are ours.** "A band of the budget" is undefined in the corpus, so `BandOf` partitions the
   systems by function (`BAND_PROPULSION`, `BAND_TACTICAL`, `BAND_SCIENCE`, `BAND_OPERATIONS`, `BAND_COMFORT`),
   and one officer holds one band at a time. This is a naming choice, not a canon one.
4. **A delegation's commitments survive its revocation.** Revocation ends the authority at once (a further
   delegated set is refused), but the allocations the officer already made stand, and their provenance remains
   `DELEGATE`. Read as "the authority to change it ended", not "the decisions were voided".
5. **The recommendation is always on the console** rather than a pending object: `RecommendAllocation` is a pure
   read, and the player may accept or refuse it at any time. Refusing writes a `MEM_OVERRULED` mark on the
   department head and logs it; accepting adopts the ladder's shares as the player's own.
6. **Unset is a provenance.** A system nobody has set reads `UNSET` (and, in automatic mode, `AUTO`), rather
   than being attributed to the player. The three authors the brief names are the player, an officer under
   standing orders, and automatic mode; a default has no author.

## The seam a staff meeting would call

The meeting is deliberately **not built** (the brief, Task C3). Its seam is these calls and reads, and nothing
else:

- `RecommendAllocation(s)` — the chief engineer's allocation and reasoning, per system
- `SetAllocation(s, id, percent)` and `SetAllocationBy(s, id, percent, officer)` — a decision, with provenance
- `AcceptRecommendation(s)` / `RefuseRecommendation(s)` — the answer, and the refusal's mark
- `GrantBand(s, grantor, grantee, band)` / `RevokeBand(s, grantee, band)` — standing authority, and its end
- `PowerCommitted(s)`, `PowerAvailable()`, `PowerShortfall(s)` — the numbers the room argues over
- the record: `LogEvent` for what was said, `Remember` for what each officer carries away
