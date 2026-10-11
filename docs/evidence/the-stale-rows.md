# Evidence: the rows that lie — every lead re-established by content, and marked

Date: 2026-10-11. Branch `feat/the-stale-rows`, cut from `testing`. **Documents and the check that
reads them only** — no engine, no game data, no `module/` change, and the save version does not move.

The brief's defect class: **a register row that still reads as *"not built"*, *"remaining"*, *"to
close"* or *"awaiting a ruling"* after the work has shipped.** A read-only sweep supplied nine leads;
the instruction was that its list is a lead, not a finding, and that each must be checked **by content**
— open the code or the evidence it names — because a document that says "not built" is exactly the
thing being questioned. Every figure below is the output of a command named beside it, run on this
tree.

The corpus registers the rule itself: `docs/omissions.md` — *"A row that still reads as unbuilt after
the work has shipped is the defect this section exists to prevent."* This pass executes that rule
across the other registers.

---

## Task A — every lead, established by content

| # | lead | verdict | what decided it (content, not the finding) |
|---|---|---|---|
| 1 | `gates.md` "at least one deck holding air independently of the main grid … **and not built**" | **stale** | The air feed is in the code: `SetAirSource`/`SetAirSourceBy`/`AirSourceOf`/`DeckLifeSupport` (`module/ship/ship_core.h:1966–1978`), the three sources as separate state (`SRC_IMPULSE_REACTORS`/`SRC_AUXILIARY`/`SRC_BATTERIES`), and `scripts/air-check.sh` exists and covers it. `docs/evidence/the-air.md` records the run (save version 56). |
| 2 | `gates.md` O1/C1 — *"What remains is the canon torpedo moments at eleven and six as content, not the mechanism"* | **half** | The mechanism is closed and *the row already says both*. It reads as content-only remaining, which is the truth; it needed a marker, not a correction of substance. |
| 3 | `gates.md` O2 — *"the holodeck reactor as a per-deck source … remains"* | **half, and the remaining half is genuinely absent** | Read the code, not the row: `SourceId` is `SRC_WARP_CORE`/`SRC_IMPULSE_REACTORS`/`SRC_AUXILIARY`/`SRC_BATTERIES` plus `AIR_GRID` (`ship_core.h:166,184`); there is no holodeck source. The holodeck reactor exists only as the trap, `JumpStartFromHolodeck` (`ship_core.cpp:4948`). Not closed by assumption. |
| 4 | `hook-triage.md` `EmitCue` — *"The meeting overlay … is its eventual play surface and **is not built**"* | **stale** | The overlay is built (`module/ui/ui_lwh_meeting.cpp`, M3 2026-10-08). A typed answer in it reaches the cue: the overlay sends `ship meeting say` (`ui_lwh_meeting.cpp:678`) → `SubmitRoomInput` (`g_ship.cpp:5426`) → `SubmitNovelAnswer` → `EmitCue` (`ship_core.cpp:8012,8031`). The clause "not built" is false. |
| 5a | `hook-triage.md` `LaunchShuttle`/`RecallShuttle` *"wait on the load screen (G19)"* | **not stale** | No shuttle UI exists: `grep -rln shuttle module/ui/` is empty; the two hooks' only callers are the `ship shuttle` verb (`g_ship.cpp:6619,6628`). `docs/shuttles.md:86` and `docs/omissions.md` O9 both carry the load-screen menu as unbuilt. |
| 5b | `hook-triage.md` `PromiseKindName` *"waits on the promise surface"* | **not stale** | Its only caller is the console; `docs/hook-register.md` marks it `nothing`, and `Promises` is still a genuine gap (no surface reads held promises). The wait is real. |
| 5c | `hook-triage.md` `SetRole` *"is left to the configurator lane"* | **half — the named owner shipped and did not carry it** | The configurator landed 2026-10-08 (`docs/gates.md`, `feat/the-configurator`) and sets name/rank/department/seat (`ui_lwh_start.cpp:125–132`, `SeatPlayerForState` in `ship_core.cpp:4312`); it never calls `SetRole`. `SetRole`'s only callers are `g_ship.cpp:5299,5681` (the `ship role` verb and `g_shipRole`). The gap is real; the ownership claim was stale. |
| 6a | `gates.md` "Still to do: the airponics bay and the galley … living quarters and bunking as a mapped place" | **not stale** | No station marker exists for either: the `lwh_station_<system>` placements are deck 12 (life support), deck 7 (computer core) and deck 6 (holodecks) only (`tools/shipmap/dressdeck*.py`). `file`/content read, not the row's word. |
| 6b | `gates.md` "What remains … the EMH (the mobile-emitter backlog item)" | **half — the EMH is built, the rest remains** | `ActivateEMH`/`EMHActive` (`ship_core.cpp:3121,3130`) and `SetMobileEmitter`/`MobileEmitter` exist, save versions 16 and 21 (`gates.md` "Then, in rough order" marks both ✅). The manner, the deck-12 countdown and the away site genuinely remain. |
| 6c | `gates.md` the two identical "Remaining (specified, not built): assimilation …" paragraphs (month report and two logs) | **not stale — duplicated** | The same paragraph appears twice; both correctly say assimilation taking the personal log and access levels is unbuilt, and both carry the "(the meeting brief … is now built)" parenthetical. `PurgeLogs`' comment (`ship_core.cpp:4875`) is the only assimilation/log code; no mechanism takes the log or access. |
| 6d | `gates.md` "The group order is a named seam and is not built (Q2)" | **not stale** | `docs/orders.md` Q2 is still an open owner question; no group-order code exists. |
| 6e | `gates.md` the two O5 mentions (Prime Directive institutional consequence) | **not stale** | Both point at O5, which `docs/omissions.md` carries open; the live Starfleet link is unbuilt. Consistent, not contradictory. |
| 6f | `gates.md` O8's appearance shortfall (no non-canon Vulcan/Ocampan/Talaxian face) | **not stale** | `docs/evidence/the-appearance.md:154–159,263,282` still records the placeholder carve as a deliberate shortfall. |
| 6g | `gates.md` crowded-room snapshot block — *"A real 150-body room and the two-machine wire were not measured"* | **not stale** | `docs/evidence/performance-survey.md:332–353` still records the 32-crew ceiling and the unmeasured wire. |
| 7 | `docs/omissions.md` C1–C5 "presented as still-open" | **not stale — already ruled in place** | The rows read `RULED`/`DEFERRED 2026-10-08` and carry markers; `check_register` resolves all five. The number-owners pass had already done it. |
| 8a | `docs/omissions.md` O19 dangling references | **not stale — corrected** | All three citations now read `docs/evidence/gap-the-log.md` (`the-record-and-the-log.md:20,64`; `the-month-report-and-the-toll.md:149`). Note: the register quotes "line 143", which has drifted to 149 as the file grew — the citation resolves; the line number is history. |
| 8b | `gates.md` entity dictionary flags-and-keys defect | **stale in presentation — it is closed** | It is fixed by regeneration (`tools/entitydict/entitydict.py`, which handles the spawnflag field) and G0's coverage was unaffected; it sat in *Residuals (named, queued, not forgotten)* and read as queued work. Marked closed. |
| 8c | `docs/HANDOFF.md` superseded marking | **not stale — marking is at the top and unmissable** | Title *"a dated snapshot (2026-10-06), superseded"* and a **SUPERSEDED 2026-10-09** blockquote at lines 1–9, before any content. |
| 9 | `gates.md` dates a day ahead of the wall clock | **stale convention, not stale dates** | Measured (below): 15 of 17 dated evidence documents in the 2026-10-05/06 sessions are dated one day ahead of their first commit. The convention is now recorded once in the ledger header; no date was rewritten. |

### Three more, and they are mine

Beyond the list, the same defect class, found by grepping for present-tense build claims and "never
modelled" lists and checking each against the code:

| found by this pass | verdict | what decided it |
|---|---|---|
| `gates.md` M3 overlay — *"No model is called, no audio device is opened, and no line is rendered: **M4, M5 and M6 are not built**"* | **stale** | M4 (`feat/the-model-call`), M5 (`feat/voice-and-casting`) and M6 (`feat/the-computer`, `feat/voice-in`) are all built and recorded in the same document's next sections. |
| `gates.md` voice mechanism — *"what remains is the plumbing that renders the lines to audio and the meeting that plays them"* | **stale** | That plumbing is M5: `PlanMeetingAudio`, `tools/voice/render.py`, the clip cache and true line durations (`docs/evidence/voice-and-casting.md`). |
| `omissions.md` O3 — canon *"never modelled"* including *"holodeck power being incompatible with other systems"* | **half** | The incompatibility **is** modelled as the trap: `JumpStartFromHolodeck` (`ship_core.cpp:4948`), `docs/ship-systems.md:86`, save version 21. The other items (gel packs, variable-geometry nacelles, the EMH's holo-emitter dependence, cargo bays as a place, rations as a rationing constraint) still stand; the row overstated the list. |

### The count, stated

- **Stale, corrected: 5** — the air row; `EmitCue`; and, found by this pass, the M4/M5/M6 line and the
  voice-plumbing line in `gates.md`, and the entity dictionary's residual presentation.
- **Half: 5** — O1/C1, O2, the "five gaps remaining" EMH clause, `SetRole`, and (found by this pass) O3.
- **Not stale: 12** — `LaunchShuttle`/`RecallShuttle`, `PromiseKindName`, the airponics/quarters row,
  the duplicated assimilation paragraph, the group order, the two O5 mentions, the appearance carve, the
  unmeasured wire, C1–C5, O19, HANDOFF, and the dates.

Twenty-two verdicts across the brief's nine leads (several leads carried more than one row) plus the
three found here.

### Right and left alone, on purpose

The chronological **"Remaining for the meeting lane: M5/M6"** lines at the end of each meeting-phase
section were **not** rewritten: each names the *next phase of that lane* and is closed by the very next
section in the same document, so the chain is self-correcting to a reader. Correcting them would rewrite
the phase narrative, which the brief forbids. Named here as a call.

The **hook counts** do not reconcile and are **reported, not fixed**: `gates.md` says "129 hooks …
only from the console" and "62 genuine gaps", while `docs/hook-triage.md` says 122 console-only (its
own intro) and 59 genuine gaps (its table, *"falls from 62 to 59"*). Deciding which is right needs the
register's own re-derivation, and guessing would put a new wrong number in the ledger — the exact
failure this pass exists to stop.

---

## Task B — corrected in place, visibly, and marked

Every corrected row keeps its original text (struck) and carries what closed it and the date. Every
established row that has shipped or partly shipped carries the register's machine marker.

### B1 — the third state: `partly`

The brief's marker form is `state=<open|closed|partly>` and calls it *"the form the register check
already reads"*. It did not: `tools/docs/check_register.py`'s regex read `open|closed` only, and three
prior passes had disagreed about how to mark a half row — the number-owners pass marked a half row
`closed`, the walkthrough pass marked half rows `open` and left them unannotated. Rather than pick one
of two wrong answers, the third state was added to the check, with tests:

```diff
-    r"state=(?P<state>open|closed)\s+"
+    r"state=(?P<state>open|closed|partly)\s+"
```

`partly` proves the **built** half (the marker must resolve, like `closed`), so the check catches the
proof vanishing and the state tells every reader the row is not fully closed. It does **not** catch the
remaining half shipping — that stays prose, like every remaining half in these registers. Two tests
were added (`test_pass_when_a_partly_row_marker_resolves`, `test_fail_when_a_partly_row_marker_is_absent`).

### B2 — the registers the check watches

`docs/gates.md` and `docs/hook-triage.md` are now registers the check reads, alongside the omissions
register and the walkthrough:

```diff
- python3 tools/docs/check_register.py --register docs/omissions.md --register docs/walkthrough.md
+ python3 tools/docs/check_register.py \
+     --register docs/omissions.md \
+     --register docs/walkthrough.md \
+     --register docs/gates.md \
+     --register docs/hook-triage.md
```

### B3 — the markers

| row | state | marker | what it names |
|---|---|---|---|
| `gates.md` air-source | closed | `check:scripts/air-check.sh` | the independent air feed's check |
| `gates.md` O1 | partly | `check:scripts/power-check.sh` | the power console; the torpedo content remains |
| `gates.md` O2 | partly | `check:scripts/air-check.sh` | the three sources; the holodeck reactor remains |
| `gates.md` gaps-remaining | partly | `symbol:module/ship/ship_core.cpp:ActivateEMH` | the EMH, since built |
| `gates.md` meeting-phases | closed | `check:scripts/meeting-overlay-check.sh` | M3 |
| `gates.md` voice-plumbing | closed | `check:scripts/audio-check.sh` | M5's render plumbing |
| `gates.md` entity-dictionary | closed | `file:tools/entitydict/entitydict.py` | the regenerating tool |
| `omissions.md` O3 | partly | `symbol:module/ship/ship_core.cpp:JumpStartFromHolodeck` | the holodeck incompatibility, since built |
| `hook-triage.md` EmitCue | closed | `file:module/ui/ui_lwh_meeting.cpp` | the overlay |
| `hook-triage.md` SetRole | partly | `check:scripts/configurator-check.sh` | the lane that shipped without it |

### B4 — the rows left unmarked, and why

`state=` is authored; the check cannot audit its author. A row whose closure is a **person's act** or
**content with no named artifact** is left unmarked rather than given an invented future filename,
which would pass, guard nothing, and put a guess in the register (the precedent in
`docs/evidence/the-number-owners.md`, B7, and `the-walkthrough-status.md`, call 2). Left unmarked:
LaunchShuttle/RecallShuttle (the load screen is content; G19 names no artifact), PromiseKindName (a
promise *display* is unnamed), gates 6a (airponics/quarters as mapped places), 6c (assimilation), 6d
(group order), 6e (O5), 6f (the appearance carve), 6g (the unmeasured wire), 7 (C1–C5 already marked
in `omissions.md`), 8a (O19 corrected), 8c (HANDOFF is prose, not a row). Their verdict is in the table
above; the check is blind to them by construction, which is its own stated limit.

### B5 — the check watches the marked rows: planted, caught, restored

**A. A scratch register with all three failure shapes, caught:**

```
$ python3 tools/docs/check_register.py --register /tmp/opencode/stale-rows/planted.md ; echo $?
FAIL  /tmp/opencode/stale-rows/planted.md:1: row air-source still reads as unbuilt/unruled, but its marker resolves (scripts/air-check.sh exists)
FAIL  /tmp/opencode/stale-rows/planted.md:2: row gone is marked closed, but its marker is absent (check:scripts/no-such-check.sh)
FAIL  /tmp/opencode/stale-rows/planted.md:3: row half is marked partly, but its marker is absent (file:docs/no-such.md)
3 register row(s) annotated; at least one disagrees with the tree
1
```

**B. Planted in the live ledger, caught, and restored.** The `air-source` row toggled to `state=open`,
the row's own proof already shipped, so the check names it; restored, and passing again:

```
$ sed -i 's/id=air-source state=closed/id=air-source state=open/' docs/gates.md
$ python3 tools/docs/check_register.py --register docs/gates.md ; echo $?
FAIL  docs/gates.md:519: row air-source still reads as unbuilt/unruled, but its marker resolves (scripts/air-check.sh exists)
8 register row(s) annotated; at least one disagrees with the tree
    docs/gates.md:416: row gaps-remaining partly closed; the built half's marker resolves ('ActivateEMH' occurs in module/ship/ship_core.cpp)
    docs/gates.md:629: row meeting-phases closed; marker resolves (scripts/meeting-overlay-check.sh exists)
    docs/gates.md:959: row voice-plumbing closed; marker resolves (scripts/audio-check.sh exists)
    docs/gates.md:977: row entity-dictionary closed; marker resolves (tools/entitydict/entitydict.py exists)
    docs/gates.md:1014: row O1 partly closed; the built half's marker resolves (scripts/power-check.sh exists)
    docs/gates.md:1062: row O2 partly closed; the built half's marker resolves (scripts/air-check.sh exists)
    docs/gates.md:1071: row O3 partly closed; the built half's marker resolves ('JumpStartFromHolodeck' occurs in module/ship/ship_core.cpp)
1
$ cp /tmp/opencode/stale-rows/gates.before.md docs/gates.md
$ python3 tools/docs/check_register.py --register docs/gates.md >/dev/null ; echo $?
0
```

### B6 — the check passing on the corrected corpus

```
$ bash scripts/docs-check.sh ; echo $?
==> docs: cited paths resolve
    184 markdown files, 1696 docs/ references, all resolve

==> docs: owned numbers agree with their owner
    computer-core-deck: docs/ship-master-map.md = 10; 4 statement(s) agree
    torpedo-launchers-deck: docs/ship-master-map.md = 10; 3 statement(s) agree
    save-version: module/ship/ship_core.h = 56; the owner defines it; 0 document assertion(s), all agree
    crew-complement: module/ship/ship_core.h = 141; 5 statement(s) agree
    crew-save-budget-bytes: module/crew/crew_core.h = 256; 5 statement(s) agree
    com-hunk-megs: scripts/run-lwh.sh = 768; 6 statement(s) agree
    6 claim(s) registered, all agree with their owner

==> docs: register rows agree with the tree
    docs/omissions.md:46: row O2 closed; marker resolves (scripts/air-check.sh exists)
    docs/omissions.md:47: row O3 partly closed; the built half's marker resolves ('JumpStartFromHolodeck' occurs in module/ship/ship_core.cpp)
    docs/omissions.md:88: row C1 closed; marker resolves (scripts/power-check.sh exists)
    docs/omissions.md:89: row C2 closed; marker resolves ('DERIVED' occurs in docs/gates.md)
    docs/omissions.md:90: row C3 closed; marker resolves ('DEFERRED' occurs in docs/gates.md)
    docs/omissions.md:91: row C4 closed; marker resolves ('MinutesOfAir' occurs in module/ship/ship_core.cpp)
    docs/omissions.md:92: row C5 closed; marker resolves ('permadeath' occurs in docs/gates.md)
    docs/walkthrough.md:617: row G1 closed; marker resolves (configs/lwh-start.cfg exists)
    docs/walkthrough.md:633: row G2 closed; marker resolves (scripts/configurator-check.sh exists)
    docs/walkthrough.md:779: row G12 closed; marker resolves (scripts/power-check.sh exists)
    docs/walkthrough.md:800: row G13 closed; marker resolves (scripts/docs-check.sh exists)
    docs/walkthrough.md:845: row G14 closed; marker resolves (scripts/character-check.sh exists)
    docs/gates.md:416: row gaps-remaining partly closed; the built half's marker resolves ('ActivateEMH' occurs in module/ship/ship_core.cpp)
    docs/gates.md:519: row air-source closed; marker resolves (scripts/air-check.sh exists)
    docs/gates.md:629: row meeting-phases closed; marker resolves (scripts/meeting-overlay-check.sh exists)
    docs/gates.md:959: row voice-plumbing closed; marker resolves (scripts/audio-check.sh exists)
    docs/gates.md:977: row entity-dictionary closed; marker resolves (tools/entitydict/entitydict.py exists)
    docs/gates.md:1014: row O1 partly closed; the built half's marker resolves (scripts/power-check.sh exists)
    docs/gates.md:1062: row O2 partly closed; the built half's marker resolves (scripts/air-check.sh exists)
    docs/gates.md:1071: row O3 partly closed; the built half's marker resolves ('JumpStartFromHolodeck' occurs in module/ship/ship_core.cpp)
    docs/hook-triage.md:57: row EmitCue closed; marker resolves (module/ui/ui_lwh_meeting.cpp exists)
    docs/hook-triage.md:134: row SetRole partly closed; the built half's marker resolves (scripts/configurator-check.sh exists)
    22 register row(s) annotated, all agree with the tree
0
```

---

## The dates — the measurement behind the convention

`docs/gates.md:388–396` and `docs/path-to-playtest.md:140–141` flagged that ledger dates run ahead of the
wall clock. The convention was established by measuring every dated evidence document against its first
commit:

```
$ python3 - <<'PY'
# for each docs/evidence/*.md with a "Date:" line, compare to its first-commit date
...
PY
differ: 17
  backlog-materials-and-crew.md   doc=2026-10-07 commit=2026-10-06
  deck07-auxcore.md               doc=2026-10-07 commit=2026-10-06
  deck13-redress.md               doc=2026-10-07 commit=2026-10-06
  deck14-stasis.md                doc=2026-10-07 commit=2026-10-06
  gap-air-and-endurance.md        doc=2026-10-07 commit=2026-10-06
  gap-morale-and-fatigue.md       doc=2026-10-07 commit=2026-10-06
  gap-the-log.md                  doc=2026-10-07 commit=2026-10-06
  gap-triage-and-sickbay.md       doc=2026-10-07 commit=2026-10-06
  gap-tricorders-and-kit.md       doc=2026-10-07 commit=2026-10-06
  s10-modes-and-roles.md          doc=2026-10-06 commit=2026-10-05
  s4-station-consoles.md          doc=2026-10-06 commit=2026-10-05
  s5-stations.md                  doc=2026-10-07 commit=2026-10-05
  s6-damage-and-casualties.md     doc=2026-10-06 commit=2026-10-05
  s7-intruders-and-control.md     doc=2026-10-06 commit=2026-10-05
  s8-the-borg.md                  doc=2026-10-06 commit=2026-10-05
  s9-the-outside.md               doc=2026-10-06 commit=2026-10-05
  voice-review.md                 doc=2026-10-07 commit=2026-10-08
```

Fifteen run **one day ahead**; one two days ahead (`s5-stations`) and one a day behind
(`voice-review`). Every other dated document agrees with its commit. So the convention is not "the
dates are wrong"; it is that **a date names the working session** and, for the sessions of 2026-10-05
and 2026-10-06, that session label ran ahead of the host's commit clock. The rule recorded once, in
`docs/gates.md`'s header: the commit timestamp is the machine's record of *when a change landed*; the
document date is the session label; **history is not renumbered**.

---

## The three commands the acceptance names

All three exit 0 on this tree, on a static host with no game data and no engine:

```
$ bash scripts/docs-check.sh ; echo $?
... 20 register row(s) annotated, all agree with the tree
0

$ scripts/test.sh            # builds the crew and ship unit tests, runs the Python tests, shellcheck
...
all checks passed            # exit 0

$ scripts/check.sh --source-map <GDK>/maps/brig-map/_brig.map \
                   --script-corpus <GDK>/scripts --build-root ../upstream ; echo $?
...
ALL PASS
    2024 files: 2016 compiled and read back, 8 rejected by the compiler, 0 read-back failure(s)
      the 8 rejections are exactly the known, documented set
0
```

`scripts/check.sh` needs the GDK, which is fetched by `scripts/fetch-gdk.sh` and is not in the
repository; the copy used here was an already-fetched `build/gdk` on this host. No engine and no game
data were run for this pass.

---

## What could not be established, and the judgement calls

1. **The cause of the session-date offset is not established** — only the offset, and its extent. A
   fixed timezone explains neither `s4` (an afternoon commit, +1) nor `gap-morale` (just after local
   midnight, +1, impossible for any zone east of UTC without a full-day offset). The convention records
   the *fact* and the rule; it does not invent a cause.
2. **`partly` was added to the check.** The brief called it a state the check "already reads"; it did
   not. The tool was corrected rather than the brief silently weakened, per the repository's own rule
   that a claim the code cannot keep is fixed in the code — or said plainly. Say plainly here: the
   brief's "already reads" was wrong about a third state, and the tool now reads it.
3. **`gates.md` and `hook-triage.md` were wired into `docs-check.sh`.** The brief's leads live there and
   its task is to make the check watch the marked rows, so the registers had to be read. This follows
   the walkthrough pass, which wired a second register the same way.
4. **The two "M4/M5/M6" corrections are mine.** They were not in the sweep's list; they are the same
   defect class, found by grepping present-tense build claims.
5. **The hook-count mismatch is reported, not fixed.** See above: a fix needs the register's
   re-derivation, and a guess is the defect.
6. **The "Remaining for the meeting lane" chain was left as written.** It is phase narrative, closed by
   the next section of the same document; rewriting it is the history rewrite the brief forbids.
7. **The `state=` values are this pass's read and the check cannot audit their author.** Where a verdict
   is a judgement (the half rows), it is named as one above.

## Files

- `tools/docs/check_register.py` — the `partly` state, with its semantics in the docstring.
- `tests/tools/test_docs_check.py` — a pass and a fail for `partly`.
- `scripts/docs-check.sh` — `docs/gates.md` and `docs/hook-triage.md` wired in.
- `docs/gates.md` — the corrected and marked rows, and the recorded dates convention.
- `docs/hook-triage.md` — `EmitCue` and `SetRole` corrected and marked.
- `docs/path-to-playtest.md` — the dates defect bullet now records the convention instead of an open
  question.
- `docs/evidence/the-stale-rows.md` — this file.
