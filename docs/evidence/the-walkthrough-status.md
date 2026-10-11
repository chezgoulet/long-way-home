# Evidence: the walkthrough's findings, reconciled row by row

Date: 2026-10-11. Branch `feat/the-walkthrough-status`, cut in its own clone on **mikoa**. This
completes the review's own §4 finding about the walkthrough (`docs/audit-whole-project.md`): the
walkthrough's G-list carried *"running commentary rather than a status column"* and *"needs the same
row-by-row read the omissions register is getting."* **Documents and the check that runs in CI only** —
no engine, no code, no `module/` change, and the save version does not move.

Every claim below is the output of a command named beside it, run on this tree. Where a verdict rests
on a check that passes rather than on a person having seen the thing, it says so.

---

## Task A — every finding classified, against the tree and the ledger

For each of **G1–G25** the finding was re-read against the code or the document it is *about* (not
against a diff) and against `docs/gates.md`, which owns status. A finding was called **closed** only
where a ledger entry or a pass's evidence says so and the content was verified. `docs/confidence-and-verification.md`
governs what a claim is worth; a finding that could not be closed is left open and says what it waits on.

### The table

| # | finding | verdict | what closed it (a quotation) / what it waits on |
|---|---|---|---|
| **G1** | no way to start LWH from the menus | **closed** | `docs/gates.md`: *"the main menu carries a **"Long Way Home"** line beside the retail buttons … `scripts/run-lwh.sh` is the launcher. The run's values live once, in `configs/lwh-start.cfg`."* Verified: the config exists and both entry paths execute it. **Rests on a check + a rendered menu (the press simulated).** |
| **G2** | the opening situation does not exist | **closed** | `docs/gates.md` (*The configurator*): *"a **configurator with four dimensions**"*; *"`FillVacancies` derives every vacant seat by the same seniority rule play uses"*; *"the `log_seed` is generated from what was chosen."* `docs/evidence/the-configurator.md` checks the four "what the opening must do" items. **Rests on `scripts/configurator-check.sh` + a rendered menu.** |
| **G3** | nothing tells you what you hold | **partly closed** | The opening statement is built (the log seed names the player's post, authority and reports — `docs/evidence/the-configurator.md`); the row's **second** "To close" item, *a way to find the consoles without the `ui_lwh_*` names*, is not — that is G8/G20. |
| **G4** | no scenario content, nothing recruits one | **open** | `docs/gates.md`: *"G5 — capstone scenario ⏳ pending"*; `docs/omissions.md` **O15**: *"the slate itself is still to author."* `scenarios/` holds two measurement harnesses. |
| **G5** | systems under stress reachable only from the console | **partly closed** | `docs/gates.md` (*the hook register*): *"the first slice wires the top of that list: `SetTarget`, `Remodulate` and `RaidVinculum` on the Tactical console and `OrderAdvance` on the command console"* (`scripts/reachability-check.sh`). The provocation (boarding/fire/breach) is still console-only — that is G4. **Rests on a check.** |
| **G6** | published decks carry no station markers | **open** | `docs/gates.md` S5: *"blocked (a marker at their interface panels leaks — sky/trigger brushes) and need chosen open-space origins by a person"*; `docs/evidence/s5-stations.md`: *"was tried and **reverted**."* |
| **G7** | the red emergency strip does not read as red | **open** | `docs/evidence/access-and-authority.md`: *"the red emergency strips toggle correctly and do not read as red from the camera."* Only the owner's eye can overrule it. |
| **G8** | a person at a panel is unproven | **open** | `docs/gates.md` S4: *"A person at a panel remains."* `docs/evidence/reachability.md`: *"no human sat at the screen. That is walkthrough G8, unchanged."* |
| **G9** | generated decks' aesthetics unjudged | **open** | The deck evidence withholds the judgement it names as the risk (`docs/evidence/deck06-composition.md`: *"This is the biggest risk."*). Owner's walkthrough. |
| **G10** | crew post coverage on generated decks never measured | **open** | Every deck evidence file: *"was not measured"* / *"Authored, not exercised."* Needs a crew scenario per generated deck. |
| **G11** | environment per deck, authored on one deck | **partly closed** | `docs/gates.md` residual C4: *"RULED 2026-10-08: PER DECK."* The authored hazard is still on deck 12 only. |
| **G12** | warship's power budget; headroom ruling not built | **closed** | `docs/gates.md` (*The ship's budgets squared*): *"five new systems … at the document's demands"*; *"warp core 1,400 (scaled by the crystal's ceiling), impulse reactors 250, auxiliary fusion 90, batteries 60; fresh supply 1,800"*; *"the engineering console shows the budget twice, fresh against now."* `docs/evidence/the-air.md`: *"a deck holding its air on its own source with the main grid down."* Verified in `SPECS[]`/`g_ship.cpp`. **Rests on `scripts/power-check.sh` + a content read.** |
| **G13** | documentation contradictions | **closed** | `docs/gates.md` (*The documentation contradictions, settled* ✅ 2026-10-09): all ten settled in place. The row's own SETTLED note stands. **Rests on `scripts/docs-check.sh` (the mechanical half only).** |
| **G14** | morale a scalar; crew have no manner | **closed** | The row's own note: *"CLOSED 2026-10-08 (`feat/the-character-layer`)."* The manner is *"demonstrated at three morale levels (`scripts/character-check.sh`)."* **Rests on a check.** |
| **G15** | no crew scenario per generated deck; fleet NPCs cannot hold posts | **open** | `docs/gates.md` G3: *"Is adding crew acceptable for this gate? None of deck04's own eight NPCs can be given a post … and that is not what was measured."* Owner's ruling + G10. |
| **G16** | the record's endgame pieces are not built | **partly closed** | `docs/gates.md` (*The meeting, phase one*): *"a brief is built **per participant from their marks and the log**."* The assimilation transfer (`docs/the-record-and-the-log.md`, "What assimilation takes") and the endings (`docs/endings.md`) remain design only. |
| **G17** | no away site | **open** | `docs/evidence/gap-tricorders-and-kit.md`; `docs/omissions.md` **O11**. A scan is of a chart row. Needs an away-site map and a session. |
| **G18** | the air/endurance countdown is not at deck 12 | **open** | `docs/evidence/gap-air-and-endurance.md`: *"deck 12 itself is still a generated hall with a station marker. Drawing the countdown at the environmental-control panel is the deck-12 build item."* |
| **G19** | the shuttle load-screen is not built | **open by design** | `docs/evidence/every-screen.md` row 18: *"a **decision menu at the moment of launch**, not a station console … Left, with the reason."* |
| **G20** | the glance has no in-world control; no sealed-quarters prompt | **open** | `docs/evidence/every-screen.md` row 12: *"in-world control, out of console scope; a readout by design."* |
| **G21** | the airponics bay and quarters are not mapped places | **open** | `docs/gates.md` (*Still to do*): *"the airponics bay and the galley as mapped places"*; *"living quarters and bunking as a mapped place."* |
| **G22** | the viewscreen/opponent are schematic | **open** | `docs/evidence/s9-the-outside.md`: *"The viewscreen is a drawn schematic … not the engine rendering the contact's model"*; *"What remains … is an engine render-target extension."* |
| **G23** | Borg assimilation visible on generated decks only; no drone faces | **open** | `docs/gates.md` S8: *"the published decks still need the stitcher pass or the engine section tag."* `docs/evidence/s8-the-borg.md` names both halves. |
| **G24** | the retail-MP human half and cutscenes | **open** | `docs/gates.md` G7: *"Still needs a human: the client's menus, the server browser, connecting to a local server, and input."* `docs/evidence/deps-durable-and-cutscenes.md`: *"Bink cutscene playback: enabled"*, but *"whether a cutscene plays still wants a session."* |
| **G25** | Ironman's edges | **open** | `docs/evidence/s10-modes-and-roles.md`: *"Not done: the save and load menus still offer what will be refused; a save on quitting; and loading the ironman save a second time is not prevented."* |

### The count, stated

- **Closed: 5** — G1, G2, G12, G13, G14.
- **Partly closed: 4** — G3, G5, G11, G16.
- **Open: 16** — G4, G6, G7, G8, G9, G10, G15, G17, G18, G19, G20, G21, G22, G23, G24, G25.

25 total. Each row carries its **2026-10-11 status in place**, its own finding text kept (the row is the
evidence the gap was found; the closing is the register's business).

### The corrections quoted — three rows, before and after

**G1 — was:** the row opened straight into `- **Missing:** g_ship defaults to 0 … A person with no
knowledge of the code cannot find it.` **now:** a status line heads it —
*"**Status 2026-10-11 — CLOSED.** `docs/gates.md` (The way in — the menu line and the launcher ✅ built
2026-10-08): "the main menu carries a Long Way Home line beside the retail buttons … `scripts/run-lwh.sh`
is the launcher …"* — with the `Missing` / `Says so` / `To close` bullets left exactly as written.

**G2 — was:** `- **Missing:** docs/start-states.md designs the opening as a situation … The shipped ship
is the whole, canonical default only.` **now:** headed by *"**Status 2026-10-11 — CLOSED.** … a
**configurator with four dimensions** … `FillVacancies` derives every vacant seat by the same seniority
rule play uses … the `log_seed` is generated from what was chosen."*

**G12 — was:** `- **Missing / contradictory:** … There are no labs, no astrometrics, no non-essential
lighting and no cargo handling in the model … the constant 1,500-vs-1,540 shortfall remains.` **now:**
headed by *"**Status 2026-10-11 — CLOSED.** … five new systems … the source rebalance … fresh supply
1,800 … the engineering console shows the budget twice, fresh against now"*, and the correction is
verified by content in `module/ship/ship_core.cpp` (`SPECS[]` carries the five added loads) and
`module/ship/g_ship.cpp` (the fresh-against-now budget). **The old prose is kept beneath the status.**

**G13 and G14** already carried their own in-place closings (G13 `SETTLED 2026-10-09`; G14 `CLOSED
2026-10-08`); this pass adds the machine-readable marker to each and leaves the finding text untouched.

### A correction forced by the row-by-row read: the section cross-references

Sections 2–11 cited G-numbers that did **not** match the gap list (they read as an earlier draft's
numbering), so a reader following a pointer landed on the wrong finding. Corrected in place and
visibly on 2026-10-11:

| site | was | now |
|---|---|---|
| deck 4, deck 8, Section 10 (the away site) | `(**G18**)` | `(**G17**)` |
| deck 12 (the red emergency strip) | `(**G9**)` | `(**G7**)` |
| Section 8 ("meet one by playing") | `(**G6**)` | `(**G4**)` |
| Section 11 (the endings) | `(**G17**)` | `(**G16**)` |
| deck 14 (`HOLODECK_RECREATION_DECK` flag) | `(**G13**)` | a deliberate non-change, no gap number |

Left as written: the deck-6 coverage note's `(**G15**)` (a defensible fit — G15 is *"no crew scenario …
(see G10)"*). Recorded because a wrong number is worse than a missing one: it gets quoted back as
settled.

---

## Task B — corrected in place, visibly, and marked

### B1 — the marker, and where it lives

Each closed row carries the register's own HTML-comment marker, so `tools/docs/check_register.py`
reads it and the row cannot silently rot back to "unbuilt":

```
<!-- register-row id=G1 state=closed marker=file:configs/lwh-start.cfg -->
```

The five closed rows and their proofs:

| row | marker | what it names |
|---|---|---|
| G1 | `file:configs/lwh-start.cfg` | the one file the run's launch values live in |
| G2 | `check:scripts/configurator-check.sh` | the configurator's proving cases |
| G12 | `check:scripts/power-check.sh` | the power console's budget |
| G13 | `check:scripts/docs-check.sh` | the check that keeps the mechanical contradictions settled |
| G14 | `check:scripts/character-check.sh` | the manner demonstrated at three morale levels |

### B2 — the walkthrough is a register the check watches

`scripts/docs-check.sh` class 3 now reads **both** registers:

```diff
- python3 tools/docs/check_register.py
+ python3 tools/docs/check_register.py --register docs/omissions.md --register docs/walkthrough.md
```

### B3 — the three demonstrations

**A. A planted un-closed row whose proof exists — caught.** A scratch register carrying an
`O2`-shaped open row whose marker resolves:

```
$ cat > build/walkthrough-demo/planted.md <<'EOF'
| **G1** | there is no way to start Long Way Home from the game's menus <!-- register-row id=G1 state=open marker=file:configs/lwh-start.cfg --> |
EOF
$ python3 tools/docs/check_register.py --register build/walkthrough-demo/planted.md ; echo $?
FAIL  build/walkthrough-demo/planted.md:1: row G1 still reads as unbuilt/unruled, but its marker resolves (configs/lwh-start.cfg exists)
1 register row(s) annotated; at least one disagrees with the tree
1
```

**B. The register check failing on the tree as it stood — the pass was needed.** The walkthrough's
closed rows, read as their stale prose had them (open), against proofs that had already shipped:

```
$ python3 tools/docs/check_register.py --register build/walkthrough-demo/stale.md ; echo $?
FAIL  build/walkthrough-demo/stale.md:1: row G1 still reads as unbuilt/unruled, but its marker resolves (configs/lwh-start.cfg exists)
FAIL  build/walkthrough-demo/stale.md:2: row G2 still reads as unbuilt/unruled, but its marker resolves (scripts/configurator-check.sh exists)
FAIL  build/walkthrough-demo/stale.md:3: row G12 still reads as unbuilt/unruled, but its marker resolves (scripts/power-check.sh exists)
FAIL  build/walkthrough-demo/stale.md:4: row G13 still reads as unbuilt/unruled, but its marker resolves (scripts/docs-check.sh exists)
FAIL  build/walkthrough-demo/stale.md:5: row G14 still reads as unbuilt/unruled, but its marker resolves (scripts/character-check.sh exists)
5 register row(s) annotated; at least one disagrees with the tree
1
```

**C. Planted in the live register, caught, and restored.** G1 toggled to `state=open` in
`docs/walkthrough.md`, the CI check then failing end to end; restored, and passing again:

```
$ sed -i 's/id=G1 state=closed/id=G1 state=open/' docs/walkthrough.md
$ bash scripts/docs-check.sh >/dev/null 2>build/walkthrough-demo/planted-docscheck.log ; echo $?
1
$ grep G1 build/walkthrough-demo/planted-docscheck.log
FAIL  docs/walkthrough.md:617: row G1 still reads as unbuilt/unruled, but its marker resolves (configs/lwh-start.cfg exists)
$ cp /tmp/opencode/walkthrough.before-plant.md docs/walkthrough.md
$ python3 tools/docs/check_register.py --register docs/walkthrough.md >/dev/null ; echo $?
0
```

**The corrected register, on the committed tree**, via the CI check:

```
$ bash scripts/docs-check.sh ; echo $?
==> docs: cited paths resolve
    181 markdown files, 1592 docs/ references, all resolve
==> docs: owned numbers agree with their owner
    ... 6 claim(s) registered, all agree with their owner
==> docs: register rows agree with the tree
    docs/omissions.md:46: row O2 closed; marker resolves (scripts/air-check.sh exists)
    ... C1–C5 ...
    docs/walkthrough.md:617: row G1 closed; marker resolves (configs/lwh-start.cfg exists)
    docs/walkthrough.md:633: row G2 closed; marker resolves (scripts/configurator-check.sh exists)
    docs/walkthrough.md:779: row G12 closed; marker resolves (scripts/power-check.sh exists)
    docs/walkthrough.md:800: row G13 closed; marker resolves (scripts/docs-check.sh exists)
    docs/walkthrough.md:845: row G14 closed; marker resolves (scripts/character-check.sh exists)
    11 register row(s) annotated, all agree with the tree
0
```

### B4 — the rows left unannotated, named with the reason

**Annotated: 5** (G1, G2, G12, G13, G14). **Left unannotated: 20** (G3, G4, G5, G6, G7, G8, G9, G10,
G11, G15, G16, G17, G18, G19, G20, G21, G22, G23, G24, G25).

The rule is `docs/omissions.md`'s and `tools/docs/check_register.py`'s own: **do not annotate a row
whose closure cannot be reduced to a marker.** The 20 fall into two kinds:

- **the closure is a person's act** — a walkthrough, a session, an owner's ruling: **G3, G6, G7, G8,
  G9, G15, G19, G24**, and the remaining halves of **G11** and **G16**. A marker cannot name a person's
  eye or a two-machine session.
- **the closure is content or a build item nothing has named yet**: **G4, G5, G10, G17, G18, G20, G21,
  G22, G23, G25**. The honest marker would be an invented future filename, which the register declines
  — it would pass, guard nothing until someone happened to use that name, and put a guess in the
  register.

Each unannotated row's own 2026-10-11 "Status" line says which kind it is, in its own words.

---

## What rests on a check, and what rests on a person

- **Verdicts that rest on a check or a content read** (a capability proved mechanically, not seen):
  **G1, G2, G5, G12, G13, G14** — and the closed halves of **G3, G11, G16**. For these, *a green check
  is not the answer to the question*: `scripts/configurator-check.sh` proves the configurator's cases,
  not that a start state *reads* as a situation; `scripts/power-check.sh` proves the budget, not that
  it feels tight.
- **Verdicts that rest on a person having seen it** (and therefore cannot be closed from here): **G6,
  G7, G8, G9, G15, G24** — the walkthrough, the session, the owner's ear or eye. **G7** is the sharpest:
  the rendered evidence says the strip does *not* read as red, and only the owner can overrule it.
- **The open rows are open because content or a session is missing**, not because the verdict is
  uncertain — `docs/gates.md` names each.

## What could not be verified

- **Whether any "closed" finding is closed in the sense the owner cares about.** A first slice passing
  its check is not a thing working on his screen. G1 and G2 are the clearest: the menu line is built
  and its callback was driven by console command because the headless harness has no input injection,
  so the *click* is simulated (`docs/evidence/the-way-in.md`, "What could not be verified").
- **The 20 unannotated rows.** The check is blind to them by construction; they are guarded by nothing
  but a person's next read. That is the register check's own stated limit, not a new finding.
- **Whether the row-by-row read is itself right.** `state=` is authored by this pass; the check cannot
  audit its author. Where a verdict is a judgement (G3's "partly", G11's "partly", G16's "partly"), it
  is named as one.
- **Whether the section cross-references were the only ones wrong.** The read covered every `(**G…)`
  marker in the file; a pointer written some other way would not have been caught.

## Judgement calls, named as calls

1. **A "partly closed" row is marked `state=open`, not `state=closed`.** The marker format has two
   states; a row with a built half and an unbuilt half is still not closed, so it stays `open` and is
   left unannotated (its remaining half has no nameable artifact). Encoding it as `closed` would let
   the check pass while the row's own "To close" is unmet — the defect the check exists to catch.
2. **The five closed rows are the only markers.** This follows the omissions register's precedent
   (`docs/evidence/the-number-owners.md`, B7), which left 20 rows unannotated for the same reason:
   an invented future filename is a guess, not a guard.
3. **`scripts/docs-check.sh` is edited, not `tools/docs/check_register.py`.** The walkthrough is wired
   in as a second `--register`, which is what the check already supports; the tool's semantics are
   untouched.
4. **The section cross-references were corrected as part of the pass, though the brief names only the
   G-rows.** A falsified pointer is the same class of defect the pass exists to stop, and leaving it
   would have the corrected gap list pointing at the wrong findings. Named here because it is beyond
   the literal task.
5. **The deck-14 flag's `(**G13**)` was removed rather than re-pointed**, because no gap-list row
   covers it — it is a deliberate non-change (Section 15, call 4), not a gap. Re-pointing it to a
   close-but-wrong row would repeat the fault.
6. **`docs/gates.md` was read, not edited.** The ledger owns status; where a finding contradicted it
   (G1, G2, G12 and the rest), the finding was corrected to the ledger, and the ledger was left alone.
7. **No engine and no game data.** Every engine check named here (`scripts/air-check.sh`,
   `scripts/reachability-check.sh`, the deck checks) was **not run** on this host; they were read in
   the evidence that records their run. The commands run here are `scripts/docs-check.sh`,
   `scripts/test.sh` and `scripts/check.sh` (the last against the fetched GDK, no engine).

## Files

- `docs/walkthrough.md` — the 25 status lines, the five markers, the register note, and the section
  cross-reference corrections.
- `scripts/docs-check.sh` — class 3 now reads the walkthrough register as well.
- `docs/evidence/the-walkthrough-status.md` — this file.
