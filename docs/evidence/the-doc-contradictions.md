# Evidence — the contradictions whose fault is the document, corrected in place

Date: 2026-10-11. Branch `feat/the-doc-contradictions`, cut from `testing`. **Documents only: no `module/`
file, no patch, no map and no engine changed; the save version does not move.** The register
`docs/designs-against-code.md` measured the corpus against the code and fixed nothing; this pass is the
repair it left, and it corrects only the side the register names as the **document's**.

The rule that decided each entry is the register's own *"Which side is wrong"* line, not this pass's taste:
where it names the document, the document is corrected; where it names the code, or gives a fork, the entry
is **handed to the code list** and the document is left alone. No promise was repaired by weakening it.

Every figure below is the output of a command named beside it, run on this tree.

---

## Part 1 — the disposition list

### 1a. The 23 numbered entries

| # | register says | disposition | where |
|---|---|---|---|
| **C-1** | document | **corrected** | `docs/story-and-semantics.md` conflict 2 marked RESOLVED 2026-10-11 (save 53) |
| **C-2** | document | **corrected** | `docs/morale.md`: save format 57 → **59** |
| **C-3** | ledger | **corrected** | `docs/lore-ledger.md`: sickbay 6 → **4** (`SICKBAY_BEDS`) |
| **C-4** | ledger, as a description of the code | **corrected** (to the `% 15` roll); **design fork handed over** | `docs/lore-ledger.md` |
| **C-5** | document's prose | **corrected** | `docs/budget-squaring.md`: "a fifteenth" → **15 per cent** |
| **C-6** | walkthrough body | **corrected** | `docs/walkthrough.md`: eighteen → **23** systems |
| **C-7** | walkthrough body | **corrected** | `docs/walkthrough.md`: the fresh-against-now display exists |
| **C-8** | walkthrough body | **corrected** | `docs/walkthrough.md` §1: the menu line exists (G1 CLOSED) |
| **C-9** | walkthrough body | **corrected** | `docs/walkthrough.md` deck-11 stop and G12's finding text |
| **C-10** | walkthrough body | **corrected** | `docs/walkthrough.md` §6 and the completeness verdict |
| **C-11** | fork | **handed to the code list** | predicate with no consumer; decide code or document |
| **C-12** | fork | **handed to the code list** | stationless repair party; two documents promise a cost |
| **C-13** | code | **handed to the code list** | `MEM_RUMOUR` declared, no writer |
| **C-14** | code (incomplete) | **handed to the code list** | one `shieldStrength` scalar, no sections/grids |
| **C-15** | fork | **handed to the code list** | twelve-shot counter vs continuous adaptation ramp |
| **C-16** | document (superseded by `endings.md`) | **corrected** | `docs/ship-systems.md`: the core breach is the one loss |
| **C-17** | document | **corrected** | `docs/ship-programme.md`: crew door handling built |
| **C-18** | policy's absolute wording | **corrected** | `docs/engine-extension-policy.md` rules 3–4, and `AGENTS.md` |
| **C-19** | both documents | **corrected** | `docs/engine-extension-policy.md` and `CONTRIBUTING.md` counts |
| **C-20** | document's absolute sentence | **corrected** | `docs/asset-doctrine.md` narrowed |
| **C-21** | fork — **the tool is the truth** (brief) | **corrected to the tool** | `docs/authoring-a-location.md`, `docs/location-brief-template.md` |
| **C-22** | the stale sentence | **corrected** | `docs/failure-is-content.md`'s "not built" paragraph |
| **C-23** | fork | **handed to the code list** | `WorkFactors` uses raw skills, not `EffectiveSkill` |

### 1b. The 11 contradicted rows recorded only in the domain tables

The register says a `contradicted` row also appears in a domain table, and that *"a handful of further
contradicted rows are recorded only in the tables."* They are accounted for here in full.

| row | register says | disposition | where |
|---|---|---|---|
| 1.56 (`ship-programme.md:79`) | contradicted, "stale; the swap is built" | **corrected** | `docs/ship-programme.md`: Borg asset swap built |
| 2.16 (`budget-squaring.md:3`) | contradicted (header vs Parts two/4a) | **corrected** | `docs/budget-squaring.md` header + Parts two/three/four |
| 3.2 (`crew-manifest.md:29-32`) | contradicted, "the code is the likely wrong side" | **handed to the code list** | the Doctor is given rank 3 |
| 3.3 (`crew-manifest.md:52-71`) | contradicted, "fork: the nine-way split is not in code" | **handed to the code list** | proposal vs five-way `DEPT_SIZE` |
| 3.20 (`crew-roster.md:94`) | contradicted | **the evidence will not decide — handed over** | see the disagreement in Part 4 |
| 3.23 (`crew-work.md:20`) | contradicted | **handed to the code list** | four job kinds vs the code's six |
| 6.2 (`staff-meetings.md:20-24`) | contradicted, "the correction paragraph is stale" | **corrected** | `docs/staff-meetings.md`: the room landed 2026-10-10 |
| 6.27 (`hook-register.md:673`) | contradicted, "under-count by 30" | **corrected** | `docs/hook-register.md` count → **479** |
| 6.28 (`hook-register.md:159,371,583-590`) | contradicted, "not re-derived" | **corrected** | section headings re-derived (87, 56, 203); totals 479 |
| 8.52 (`design-north-star.md:175`) | contradicted, "fork: the shipped SP path is 1" | **handed to the code list** | 64-client ceiling vs SP 1 |
| 8.53 (`design-north-star.md:192`) | contradicted, "fork: one hard-coded hull" | **handed to the code list** | "complement is hull data" vs `COMPLEMENT` |

### The counts, against the register's own

- **Corrected: 22** — **17** numbered (C-1…C-10, C-16…C-22) and **5** table-only (1.56, 2.16, 6.2, 6.27, 6.28).
- **Handed to the code list: 12** — **6** numbered (C-11, C-12, C-13, C-14, C-15, C-23) and **6** table-only
  (3.2, 3.3, 3.20, 3.23, 8.52, 8.53).
- Of the 12 handed over, **9 are forks** the evidence will not decide (C-11, C-12, C-15, C-23, 3.3, 3.20,
  3.23, 8.52, 8.53) and **3 are code faults** the register pinned on the code (C-13, C-14, 3.2).
- **Unaccounted for: none.**

**Reconciling with the register's numbers.** The register reports **23** numbered contradictions and **33**
`contradicted` table rows. **Twenty** of the numbered entries carry a table row, and two of those (C-18 and
C-21) carry two rows each, so **22** of the 33 rows are the table form of a numbered entry; the other **11**
rows are table-only. Three numbered entries (C-13, C-16, C-22) have no `contradicted` table row of their own.
23 + 11 = **34 distinct findings**, all dispositioned.

---

## Part 2 — the corrections, quoted before and after

Each corrected row keeps its original text and gains what closed it and when, in the register's own idiom.
Four representative pairs; the rest are in the documents themselves, each carrying a `**Corrected
2026-10-11**` note.

| # | before (quoted) | after |
|---|---|---|
| **C-1** | *"`CrewMember` in `ship_core.h` carries a single `float morale` … Either the components land or the derivations stop being claimed."* | *"**2. Morale … — RESOLVED 2026-10-11 (save version 53).** … the single `float morale` is **gone** — `Morale(c)` reads the three components … The claim of absence above is kept only as the record of the conflict."* |
| **C-3** | *"Sickbay \| 6 under treatment at once; 12 hours each at full output"* | *"Sickbay \| **4** under treatment at once (three standard and one surgical, `SICKBAY_BEDS`) … **CORRECTED 2026-10-11 (C-3): this row read "6 under treatment" …**"* |
| **C-8** | *"**There is no way to start Long Way Home from the game's menus.** `g_ship` defaults to `0` …"* | *"**CORRECTED 2026-10-11 (C-8) — the menu way in now exists; the finding is kept as history.** … The main menu now carries a **"Long Way Home"** line … This walkthrough's own **G1** row is CLOSED."* |
| **C-20** | *"**Nothing generated and nothing extracted enters the repository.**"* | *"**No game assets, no third-party images, and no generated audio or meshes enter the repository as content.** … Our own **rendering** `docs/art/lcars-meeting-overlay.png` is tracked … the absolute was the wrong party, so it is narrowed …"* |

---

## Part 3 — the two shapes the brief named, settled

### 3a. The self-contradiction, and the sweep it forced (C-22)

`docs/failure-is-content.md` said at its close, *"Not built: each modelled system's three designed failure
states (this covers the core; the other systems still fail one way)…"* — while **two subsections below** the
same file records that every system's three states are mechanised (`FailureStateOf` / `UpdateSystemStates`),
and the code agrees (save format 40, `TestSystemStates`).

The stale sentence is corrected in place and the finding kept.

**The sweep.** Grepping the corrected document for the same claim:

```
$ grep -n "failure states\|fail one way\|Not built" docs/failure-is-content.md
35:Design each system with three failure states, so failure has a range rather than a switch:
122:- Every modelled system has three failure states designed, not just one.
144:Not built: each modelled system's three designed failure states (this covers the core; the other
145:systems still fail one way), and the written consequence of every catastrophe in the crew records.
147:### System failure states (2026-10-07)
149:The acceptance's first line -- *every modelled system has three failure states designed, not just
```

The claim of **absence** is made **once** (lines 144–145). Every other line states the design or accepts the
built state; none of them needed correcting.

**And the same sweep over the other documents this pass corrected** (a self-contradiction is rarely alone):

- `docs/budget-squaring.md` — the C-5 slip and the D2-16 header are self-contradictions (the header against
  its own Part five table; "a fifteenth" against `0.15` twenty lines above). Parts two, three and four are
  now marked as the pre-ruling history so the header is unambiguous.
- `docs/walkthrough.md` — the same five claims (C-6…C-10) recur in **two** places each (the route stop and
  the gap-list/verdict text); both are corrected.
- `"24-64"` also occurs at `docs/g3-reactive-crew.md:77`, a **third** site the register did not name; it is a
  dated G3 measurement's own definition of "standable", so it is annotated, not rewritten.
- `"eighteen systems"` also occurs at `docs/budget-squaring.md:121`, inside the Part two snapshot now marked
  as history.

### 3b. The tool-versus-document case (C-21)

`docs/authoring-a-location.md:30` and `docs/location-brief-template.md:65` said *"standing distance (24-64
units)"*, and both tell the author to run the post generator. **The tool is the truth**, so its constant was
read from its own source and the documents were corrected to it:

```
$ sed -n '37p' tools/posts/gen-posts.py
STANDING_MIN, STANDING_MAX = 24.0, 96.0
```

Both documents now say **24-96** and name the constant.

---

## Part 4 — the one verdict I disagreed with, named

**Row 3.20 — `"25 bytes per crew member"` (`docs/crew-roster.md:94`).** The register's verdict is
`contradicted`, witness `SAVE_MEMBER_BYTES = 20`. **I did not correct it, and I disagree with the verdict:**
the 25 is a *measurement*, not a stale echo —

```
$ grep -n "25 bytes" docs/evidence/g3-reactive-crew-measured.md
24:PASS  save-size increase within budget   25 bytes per NPC of layer state (budget 256); ...
$ sed -n '219,220p' module/crew/crew_core.h
const uint16_t SAVE_VERSION = 1;
const size_t SAVE_MEMBER_BYTES = 20;
```

`docs/evidence/g3-reactive-crew-measured.md` (a dated measurement) states 25 bytes per NPC of layer state;
the code's `20` is the packed member width, with the 16-byte header separate. **`docs/evidence/the-number-owners.md`
already recorded exactly this** as *"two documents disagree on the measured per-crew save cost (20 bytes + a
16-byte header vs 25 bytes). This is the owner's to reconcile."* So it is a fork, not a document fault, and
changing the document to 20 would replace a measured number with a different one — the exact failure
`AGENTS.md` warns against. It is handed to the code list with the fork stated.

---

## Part 5 — the markers, and the rows left unmarked

Each **corrected** row now carries the register's machine-readable marker **on the register row** — an HTML
comment that `tools/docs/check_register.py` reads — so the correction is watched, not merely written:

```
<!-- register-row id=C-3 state=closed marker=symbol:module/ship/ship_core.h:SICKBAY_BEDS -->
```

`docs/designs-against-code.md` is now a register `scripts/docs-check.sh` reads (a fifth `--register`), the
same way the `partly` state and the other registers were wired in earlier passes.

- **Marked: 22** — every corrected row, `state=closed`, each marker naming the proof the register's own
  witness names (a code symbol, a check script, a file, or the tool's constant).
- **Left unmarked: the 12 handed-over rows**, because their closure is a code change or an owner's ruling
  that does not exist yet, and the precedent in these registers is **not to invent a future filename** for a
  marker (`docs/evidence/the-number-owners.md` B7; `docs/evidence/the-walkthrough-status.md` call 2). The
  check is blind to them by construction — its own stated limit.

**What the markers do and do not prove, said plainly** (the check's own caveat): a marker proves the named
symbol/file/check still exists. It does **not** prove the corrected prose is true, and it does **not** watch
the document text itself — it watches the proof the row rests on. For a contradiction row, that is the right
thing to watch: it catches the proof vanishing, which is when the row must be re-read.

---

## Part 6 — the checks the brief requires

```
$ bash scripts/docs-check.sh ; echo $?
==> docs: cited paths resolve
    198 markdown files, 2150 docs/ references, all resolve

==> docs: owned numbers agree with their owner
    computer-core-deck: docs/ship-master-map.md = 10; 4 statement(s) agree
    torpedo-launchers-deck: docs/ship-master-map.md = 10; 3 statement(s) agree
    save-version: module/ship/ship_core.h = 59; the owner defines it; 4 document assertion(s), all agree
    crew-complement: module/ship/ship_core.h = 141; 5 statement(s) agree
    crew-save-budget-bytes: module/crew/crew_core.h = 256; 5 statement(s) agree
    com-hunk-megs: scripts/run-lwh.sh = 768; 6 statement(s) agree
    6 claim(s) registered, all agree with their owner

==> docs: register rows agree with the tree
    ... docs/omissions.md, docs/walkthrough.md, docs/gates.md, docs/hook-triage.md ...
    docs/designs-against-code.md:45: row C-1 closed; marker resolves ('Morale' occurs in module/ship/ship_core.h)
    ... 22 designs-against-code rows, all resolve ...
    56 register row(s) annotated, all agree with the tree
0
```

```
$ PATH=/home/robot/.cache/lwh-venv/bin:$PATH scripts/test.sh ; echo $?
...
==> python: syntax
    ok

==> shell: shellcheck
    ok

==> patches: numbered without gaps, and each one parses
    24 patches

all checks passed
0
```

`BRIEF.md` was deleted before committing, as the brief requires; the counts above are the committed tree's
(198 files, without the brief).

---

## Part 7 — what could not be established, and the judgement calls

**Could not establish:**

1. **Whether the register's verdicts are themselves right.** This pass executes them; where one looked wrong
   (3.20) it was said so, not silently overruled. The register is a dated measurement, and its `state=` is
   authored.
2. **The re-derivation the hook counts need beyond the row counts.** The section totals (75, 87, 56, 20, 20,
   203, 18 = **479**) and the reachability split are derived here with `tools/hooks/reach_report.py`, but the
   register's own `by group` grouping is a person's classification (the tool's docstring says so), and the old
   by-group table is left as its dated snapshot.
3. **Anything whose only witness is the rendered game.** No engine and no game data were run; the register is
   a static-host measurement and so is this repair.

**Judgement calls, named as calls:**

1. **The dispositions are recorded on the register rows and marked there, not in the corrected documents.**
   The brief asks to *"record the disposition beside the row"* and to give *"each corrected row … the marker
   the register check reads"*; the rows are the register's, so that is where both went. The verdict text of
   every row is untouched — only a `**Disposition 2026-10-11:**` bullet (numbered entries) or a trailing cell
   note (table rows) was added.
2. **`AGENTS.md` was narrowed, not left alone (C-18).** The register names the policy's absolute *"and its
   restatement in `AGENTS.md`"*; the static-file rule is about state, not about a false rule, so the rule was
   narrowed.
3. **Row 3.20 is handed over, not corrected** (Part 4) — a **disagreement with the register's verdict**,
   named as one.
4. **C-4's row is corrected to what the code does, and its design fork handed to the code list.** The register
   names the ledger as wrong *as a description of the code* but says the *fix* is a fork; with no code change
   permitted here, the description is corrected and the fork is handed over.
5. **The hook-count row (6.28) was corrected though the register wrote "not re-derived".** Leaving a known
   under-count in place is itself a wrong number; the row counts are mechanically countable, so they were
   counted and the citation given.
6. **The brief's carve-out ("do not touch … the doors") was read as the *feature*, not the sentence.** C-17 is
   a document-side contradiction about doors in `docs/ship-programme.md`; the sentence was corrected and no
   door code was touched.

## Files

- The corrected documents: `docs/story-and-semantics.md`, `docs/morale.md`, `docs/lore-ledger.md`,
  `docs/budget-squaring.md`, `docs/walkthrough.md`, `docs/ship-systems.md`, `docs/ship-programme.md`,
  `docs/engine-extension-policy.md`, `CONTRIBUTING.md`, `AGENTS.md`, `docs/asset-doctrine.md`,
  `docs/authoring-a-location.md`, `docs/location-brief-template.md`, `docs/failure-is-content.md`,
  `docs/staff-meetings.md`, `docs/hook-register.md`, `docs/g3-reactive-crew.md`.
- `docs/designs-against-code.md` — a `Disposition 2026-10-11` on every numbered entry and every table-only
  contradicted row, and 22 `register-row` markers; **no verdict changed**.
- `scripts/docs-check.sh` — the register is now a fifth `--register`.
- `docs/evidence/the-doc-contradictions.md` — this file.
