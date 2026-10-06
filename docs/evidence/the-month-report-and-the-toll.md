# Evidence — the month report, the promise and the lie, and the toll

Date: 2026-10-06. Branch: `feat/the-month-report-and-the-toll`, cut from `feature/g3-reactive-crew`
(checked at `6e1455e`). Implements `docs/the-record-and-the-log.md` and Part 2 of
`docs/memory-and-consequence.md`. Save format **version 44**.

The memory layer already existed and was **not** rebuilt: `struct Memory` (event · person · source ·
time · valence · salience), the four provenances, the six events, `MemoryDecay`, `Bond` and the
save-version-18 record were read in the working tree before anything was written. What was missing
was the **wiring**: `MEM_PROMISE` and `MEM_LIE` were declared and never produced, and the log had no
periodic beat. This branch adds the producers, not the structure.

Nothing here is asserted without the command that produced it.

---

## Task A — the promise and the lie

**A promise.** `MakePromise(officer, crew, kind, what[, deadline])` writes the mark (`MEM_PROMISE`,
source `MEM_SAW`, positive) naming the promiser, and **holds the claim** in `Ship::promises` so it can
mature. It is bounded (`PROMISE_MAX = 16 [inv]`).

**A promise matures.** `ResolvePromise(index, kept)`: kept, the mark's valence strengthens to +1 and
the bond rises; broken, the valence turns to −0.9 and the bond falls, and the log carries the reason
**in the crew member's voice** (`who` is the crew member, not the ship). A deadline that passes with
nothing said is broken from inside `Tick` (`MaturePromises`). `Promote` closes an open
`PROMISE_PROMOTION` as kept — "when the thing is done."

**A lie.** A lie is *not* written when someone speaks falsely. `SignReport` writes `MEM_LIE` when a
**signed** report line contradicts a mark whose source is `MEM_SAW`: the mark belongs to the person
who saw it (`Remember(s, witness, MEM_LIE, signer, MEM_SAW, …)`), and it names the signer.

**Direction and modulation.** The toll is paid **downward**: only a witness whose rank is below the
signer's, reading a report published `REPORT_TO_CREW`, loses trust. A report filed `REPORT_UPWARD` is
read by nobody below, so the same falsehood costs nothing from below. Divergent `faction` amplifies
(×1.6) and alignment suppresses (×0.5); a pre-existing negative bond amplifies further (×1.3).
The magnitudes are invented `[inv]`; the direction is the document's.

**Read, in the working tree** (`module/ship/ship_core.h:213` at `6e1455e`): `struct Memory`, the four
`MemorySource` values and the six `MemoryEvent` values. **Added:** `Memory::orphaned`, the
`Promise`/`MonthReport` types, and the functions above.

---

## Task B — the month report

- **Drafted honestly from the record.** `DraftReport` builds the headline from the navigation counter's
  change since the last entry and the ship's condition from state, then one line per witnessed event
  (`MEM_SAW`) in the period — a death seen by three people drafts once. The counter is the code's
  existing scoreboard, `DilithiumRange`; see the judgement call below.
- **Edited by the player, through the console path.** `ship report strike <line>`, `soften <line>
  [factor]`, `edit <line> <text>`, `add <scope> <text>`.
- **The record keeps the diff.** `ReportDiff` reads `draft` against `text`; the published version goes
  to the log and the diff stays on the `MonthReport` in `Ship::reports`. `ship report diff` prints it.
- **Purge.** `PurgeLogs` empties the published log and sets `orphaned` on every `MEM_LOG`-sourced mark
  — the citation is gone, the mark is not. It never touches assimilation, per the document.

---

## Observed

The demonstration prints PASS lines for each acceptance item. Command and full output:

```
$ cmake -S tests/ship -B /tmp/ship && cmake --build /tmp/ship -j && /tmp/ship/test_ship_core --month
PASS  promise kept: mark 1 valence 1.00, bond 0.60 -> 1.00
PASS  promise broken: bond 0.60 -> -0.90; the log, in the crew member's voice: Crewman 041: Kathryn Janeway said they would be brought back. It was not done.
PASS  a deadline passed with nothing said: the promise is broken
PASS  a struck death in a signed report: witness holds a lie mark = 1, bond toward the signer 0.00 -> -0.30
PASS  the same falsehood filed upward: lie mark = 0, bond change +0.0000
PASS  the report drafted 2 lines, was edited, and the record kept:
- the ship can still make 3000 light years, +3000 since the last entry
+ the ship can still make 1500 light years, +3000 since the last entry
+ all is well
PASS  purge: log entries 0, the MEM_LOG mark still held = 1, orphaned = 1
all demonstrations passed
```

The unit tests (`TestMonthReportAndToll`) cover the same ground and additionally assert: the mark's
provenance is `MEM_SAW`; the reason is signed by the crew member; the witness's bond moves and a
filed-upward report leaves it untouched; the whole of it survives a save and load byte-for-byte
(`Pack(back) == blob`).

```
$ /tmp/ship/test_ship_core
...
ship_core: all checks passed
$ echo $?
0
```

The module (the console path included) compiles: `make` in the configured upstream build produced
`Built target efgame` and `Built target efui`, no errors.

---

## The two required suites

```
$ scripts/test.sh ; echo $?
...
==> patches: numbered without gaps, and each one parses
    15 patches

all checks passed
0
```

```
$ scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts ; echo $?
...
==> compiler corpus pass (every shipped script: compile, then read back)
    2024 files: 2016 compiled and read back, 8 rejected by the compiler, 0 read-back failure(s)
      ... (all eight named) ...
    the 8 rejections are exactly the known, documented set
0
```

Both exit **0**.

---

## Judgement calls, named

1. **What "the navigation counter" is.** There is no distance-to-Earth function in the code; the only
   counter is `DilithiumRange`, which the header itself calls "the scoreboard". To avoid computing a
   second quantity (the brief's own instruction), the report's headline is `NavigationCounter` =
   `DilithiumRange` and its change since the last entry. The `--month` output shows the label as the
   code's own meaning, not as "light years from home". If the owner wants the document's
   distance-and-estimate counter, that is a new model, not this function.
2. **A line is "contradicted" when it is struck, softened or edited at all.** Additions carry no
   event/person, so an added claim has no witness and cannot be contradicted. This is the smallest
   rule that makes "the lie made with the player's hands" mechanical.
3. **Multiple promises from one officer to one crew member share one mark.** `Remember` reinforces by
   `(event, person)` and the memory layer has no per-promise id. The claims are held separately in
   `promises`; resolving one moves the shared mark. Faithful to "hearing the same thing again sharpens
   the mark", but a limit worth knowing.
4. **Purge clears the whole published log, not only report lines.** The log entries are the published
   artifact; the doc's "a hole where the log would have been" is the effect. The diff in `reports`
   survives, because it is the record, not the log.
5. **`ReadsReport` is the audience, not the scope string.** Direction is modelled by
   `REPORT_TO_CREW` versus `REPORT_UPWARD`, which is exactly the brief's "a report nobody below
   reads". Per-scope clearance filtering (`docs/gap-the-log.md`) is not re-implemented here.

## Left unbuilt, and said so

The two logs as separate stores (official and personal), the meeting brief built per participant,
assimilation taking the personal log and the access levels, and the Collective speaking in the
assimilated person's voice — all specified in `docs/the-record-and-the-log.md`, none of them built
here. The brief stops at the promise, the lie, the report and the purged mark.
