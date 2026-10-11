# Evidence — owners for the numbers, and a register that cannot go stale unnoticed

Date: 2026-10-11. Branch: `feat/the-number-owners`, cut from `testing` (`c3368c1`). **Documents, Python
and the check that runs in CI only** — no code, no engine, no `module/` change, and the save version
does not move. This closes the two findings of `docs/audit-whole-project.md` §2.6 (a number asserted in
many places, owned by none) and §4 (the registers have gone stale, and that is their own stated
defect).

Two halves, as the brief asks: **Task A**, the save version and its siblings registered with owners and
a recorded rule for telling a current-tense statement from a historical one; **Task B**, a check that a
register row cannot sit unbuilt while the proof it was waiting for exists.

Everything below is a command that was run and its result. The transcripts are the output of that
tree; the counts were re-taken on the committed tree.

---

## Task A — owners for the numbers, and the distinction that keeps it honest

### A1 — the registered claims and where each owner lives

Four claims are added to `tools/docs/owned_claims.json`; the two that were already there are unchanged.
The full registry is six claims.

| claim | value | owner (the file that fixes it) | the other files that must agree |
|---|---|---|---|
| `computer-core-deck` *(existing)* | 10 | `docs/ship-master-map.md` | `module/ship/ship_core.cpp`, `docs/budget-squaring.md`, `docs/lore-ledger.md`, `docs/access-and-authority.md` |
| `torpedo-launchers-deck` *(existing)* | 10 | `docs/ship-master-map.md` | `module/ship/ship_core.cpp`, `docs/budget-squaring.md`, `docs/access-and-authority.md` |
| `save-version` | 56 | **`module/ship/ship_core.h`** (`const uint16_t SAVE_VERSION = 56;`) | every `*.md` outside `docs/evidence/` that asserts it in the assertion form |
| `crew-complement` | 141 | **`module/ship/ship_core.h`** (`const int COMPLEMENT = 141;`) | `docs/lore-ledger.md`, `docs/ship-systems.md`, `docs/crew-roster.md`, `docs/budget-squaring.md`, `docs/the-entry-point.md` |
| `crew-save-budget-bytes` | 256 | **`module/crew/crew_core.h`** (`saveBytesPerNpc = 256;`) | `docs/g3-reactive-crew.md`, `docs/ship-model.md`, `docs/scenario-manifest.md`, `docs/crew-roster.md`, `docs/handoff-lwh-persistent-ship.md` |
| `com-hunk-megs` | 768 | **`scripts/run-lwh.sh`** (the one launcher; `com_hunkMegs 768`) | `docs/walkthrough.md`, `docs/the-entry-point.md`, `docs/HANDOFF.md` |

A code constant is owned by the file that defines it, as the brief directs for the save version; the
same rule is extended to the crew complement, the per-NPC save budget and the hunk, because each is a
code or launch fact rather than a design preference. That extension is a judgement call, named below.

### A2 — how the check tells a current claim from a history

**Decision (recorded in `tools/docs/owned_claims.json`, `_tense_convention`, and in the header of
`tools/docs/check_claims.py`): the assertion form is the marker.**

- A statement is **current-tense** only when it writes the constant's own assignment — the shape a
  document uses to tell a reader what the constant *is*: ``SAVE_VERSION = <n>``. Every such statement
  anywhere in the live documents must equal the owner's value.
- Every **prose** reference — `save version 54`, `save format unchanged (54)`, `past 49`,
  `(save version 41)` — is a **historical statement**: how a dated ledger entry, walkthrough, handoff
  or evidence file refers to a value that was current at its date. It is not wrong for being
  superseded, and the check does not read it.
- `docs/evidence/` is not scanned at all: it is a dated record, allowed to hold superseded values with
  an annotation.
- A file may be **exempted by name, with its reason**, when it is *recording* another file's assertion
  rather than making one. One file is exempt: `docs/audit-whole-project.md` — a dated snapshot whose
  §2.6 table records *where* the number is asserted and quotes the handoff's line as its evidence.

Why this and not a blanket "dated document" exemption: a whole-file rule would have exempted the one
document that was actually wrong (the handoff, itself dated), and the brief asks the check to flag any
*current-tense* statement while not flagging the ledger's own history. The assertion form separates
exactly those two, needs no new syntax in the prose, and is the form the one stale statement was
already written in.

**Demonstrated — a current-tense disagreement is caught, a historical statement is not.** The tree
contains no current-tense assertion today (the one that existed has been corrected, below), so the
error is planted. A planted assertion in a live document:

```
$ printf '\nThe save format is at `SAVE_VERSION = 12`.\n' >> docs/ship-model.md
$ python3 tools/docs/check_claims.py ; echo $?
FAIL  save-version: docs/ship-model.md:143 states 12 as the current value (the assertion form); owner module/ship/ship_core.h says 56
6 claim(s) registered; at least one contradicts its owner
...
1
$ git checkout -- docs/ship-model.md
```

A historical statement of a superseded number — the ledger's own kind of sentence — passes:

```
$ printf '\nHistorical: the configurator pass landed at save version 54.\n' >> docs/ship-model.md
$ python3 tools/docs/check_claims.py ; echo $?
    ...
    save-version: module/ship/ship_core.h = 56; the owner defines it; 0 document assertion(s), all agree
    ...
0
$ git checkout -- docs/ship-model.md
```

The ledger's own `docs/gates.md:100` (**Save version 54**) and `docs/walkthrough.md:924` ("past 49")
are prose history and are not flagged — that is the false positive the pass exists to avoid.

### A3 — what was actually wrong, corrected in place

`docs/handoff-live-crew-and-meetings.md:28` (in `testing` at `c3368c1`) read:

```
- **The save format is versioned with a changelog.** `module/ship/ship_core.h`: `SAVE_VERSION = 12`, whose comment
```

It states the constant as a fact about the code, and the code reads **56** — fifteen versions stale.
It is corrected in place, the finding kept:

```
- **The save format is versioned with a changelog.** `module/ship/ship_core.h` defines `SAVE_VERSION`, whose
  comment names what each increment added (...). Follow that style: bump, and say what the new number carries.
  *(Corrected: was cited as `= 40`, ...)*
  *(Corrected 2026-10-11: this bullet **asserted the constant itself** — a current-tense claim — and it had
  gone fifteen versions stale (the code reads 56). **This brief is a dated snapshot; its figures are of its
  date.** The version at the brief's date was 12; the current value is the code's alone, in
  `module/ship/ship_core.h`, and this brief no longer states it. The rule is recorded in
  `docs/evidence/the-number-owners.md`.)*
```

The file's "What already exists" section also gains a header line marking the whole brief a **dated
snapshot — 2026-10-06**, whose figures are of that date. The value was **not** updated to 56: a dated
brief that restates the current constant is a second place that can rot, which is the defect the item
is about. The value is left with the code.

**Demonstration that the check catches the defect as it stood** — the original line, planted back into
a live document and caught:

```
$ printf '\nThe save format is at `SAVE_VERSION = 12`.\n' >> docs/ship-model.md
$ python3 tools/docs/check_claims.py ; echo $?
FAIL  save-version: docs/ship-model.md:143 states 12 as the current value (the assertion form); owner module/ship/ship_core.h says 56
1
```

### A4 — the other numbers, and the one that disagrees

- **Registered with an unambiguous owner:** the crew complement (141, code), the per-NPC save budget
  (256, code), the engine hunk (768, the launcher). See the table in A1.
- **Left unregistered, and why:** the **BSP size** (54 MB) is a measurement of a build artifact — no
  file *fixes* it, and the two places that state it (`docs/HANDOFF.md:106`, a superseded dated
  snapshot; `docs/walkthrough.md:38`, the run instructions) are both records of a measurement. Naming
  an owner would be inventing one. It is reported here instead.
- **A disagreement between two measurements — the owner's, not this pass's.** The per-crew save cost is
  stated as two different numbers:
  - `docs/g3-reactive-crew.md` — *"Measured: **20 bytes per NPC plus a 16-byte header**"*;
  - `docs/crew-roster.md:95`, `docs/gates.md:187`, `docs/walkthrough.md:359`, and
    `docs/evidence/g3-reactive-crew-measured.md` — **25 bytes per crew member** (of layer state).

  The budget (256) is not in dispute; the measured cost is. Both have a witness, so the pass records
  the disagreement rather than picking a winner. **Finding: two documents disagree on the measured
  per-crew save cost (20 bytes + a 16-byte header vs 25 bytes). This is the owner's to reconcile.**
- **Checked and not a disagreement:** the crew complement is 141 at launch with ~150 in service and 153
  at drydock (`docs/design-north-star.md:193`, `docs/ship-systems.md:37`) — different points on the
  timeline, not two answers to one question.

---

## Task B — a register cannot go stale unnoticed

### B1 — the marker, and where it lives

`tools/docs/check_register.py`, class 3 of `scripts/docs-check.sh`. **The marker lives in the register
itself**, on the row, as an invisible HTML comment, so the claim and its marker are one place:

```
| **C1** | ... <!-- register-row id=C1 state=closed marker=check:scripts/power-check.sh --> |
```

Marker kinds: `file:<path>` / `check:<path>` (a file exists), `symbol:<path>:<name>` (a name occurs in
a file), `text:<path>:<substring>` (a string occurs in a file). `state=open` means the row still
claims the capability unbuilt or the question unruled; `state=closed` means it has been corrected.

No sidecar file was needed: the register can carry the marker, and does.

### B2 — what it fails on

- **`state=open` and the marker resolves** — the row still reads as unbuilt, but what it was waiting
  for exists. This is the register's own stated defect.
- **`state=closed` and the marker does not resolve** — a corrected row whose proof has gone (renamed,
  deleted). Cheap to express, so it is checked.
- **a malformed marker**, and **a register with no markers at all** (so a broken scan cannot pass
  vacuously).

### B3 — the failure on the tree as it stands, and the pass after correction

The five contested rows were annotated **as they stood** (`state=open`) *before* their prose was
corrected, and the check failed — naming the rows and the proofs that had overtaken them:

```
$ python3 tools/docs/check_register.py ; echo $?
FAIL  docs/omissions.md:85: row C1 still reads as unbuilt/unruled, but its marker resolves (scripts/power-check.sh exists)
FAIL  docs/omissions.md:86: row C2 still reads as unbuilt/unruled, but its marker resolves ('DERIVED' occurs in docs/gates.md)
FAIL  docs/omissions.md:87: row C3 still reads as unbuilt/unruled, but its marker resolves ('DEFERRED' occurs in docs/gates.md)
FAIL  docs/omissions.md:88: row C4 still reads as unbuilt/unruled, but its marker resolves ('MinutesOfAir' occurs in module/ship/ship_core.cpp)
FAIL  docs/omissions.md:89: row C5 still reads as unbuilt/unruled, but its marker resolves ('permadeath' occurs in docs/gates.md)
6 register row(s) annotated; at least one disagrees with the tree
    docs/omissions.md:46: row O2 closed; marker resolves (scripts/air-check.sh exists)
1
```

After the rows were corrected in place and flipped to `state=closed`, it passes:

```
$ python3 tools/docs/check_register.py ; echo $?
    docs/omissions.md:46: row O2 closed; marker resolves (scripts/air-check.sh exists)
    docs/omissions.md:88: row C1 closed; marker resolves (scripts/power-check.sh exists)
    docs/omissions.md:89: row C2 closed; marker resolves ('DERIVED' occurs in docs/gates.md)
    docs/omissions.md:90: row C3 closed; marker resolves ('DEFERRED' occurs in docs/gates.md)
    docs/omissions.md:91: row C4 closed; marker resolves ('MinutesOfAir' occurs in module/ship/ship_core.cpp)
    docs/omissions.md:92: row C5 closed; marker resolves ('permadeath' occurs in docs/gates.md)
    6 register row(s) annotated, all agree with the tree
0
```

The reverse direction, demonstrated on a scratch register:

```
$ printf '| **C1** | ruled, but the proof is gone <!-- register-row id=C1 state=closed marker=check:scripts/no-such-power-check.sh --> |\n' > build/register-demo.md
$ python3 tools/docs/check_register.py --register build/register-demo.md ; echo $?
FAIL  build/register-demo.md:1: row C1 is marked closed, but its marker is absent (check:scripts/no-such-power-check.sh)
1
```

### B4 — the rows corrected in place

`docs/omissions.md`. Each keeps the finding and records the ruling (`docs/gates.md`, *Residuals*), per
the register's own rule. The section header is corrected too (it read "need the owner's ruling before
they can be ordered"; C1–C5 were ruled 2026-10-08, C6 remains open). The five cells now read:

- **C1 — RULED 2026-10-08 — the headroom curve.** (was: the power budget and the system table;
  `docs/budget-squaring.md` was "proposed, awaiting the owner's ruling. Nothing here is
  implemented".) Ruled for the number set; implemented 2026-10-07, PR #53, save format 50. *What
  remains is the canon torpedo moments at eleven and six — content, not mechanism.* Marker:
  `check:scripts/power-check.sh`.
- **C2 — RULED 2026-10-08 — "Derived".** (was: allegiance drift.) `Bond(s, a, b)` is already the fold;
  `holdings` is the half not yet folded and is named as the remaining work. Marker:
  `text:docs/gates.md:DERIVED`.
- **C3 — DEFERRED 2026-10-08 to the scenario and content work.** (was: Q-class encounters.) It is
  content, registered in `docs/scenario-atlas.md`. Marker: `text:docs/gates.md:DEFERRED`.
- **C4 — RULED 2026-10-08 — "Air is per deck".** (was: per-compartment or per-deck.) It was already per
  deck in the code; `docs/ship-model.md` was the outlier. Marker:
  `symbol:module/ship/ship_core.cpp:MinutesOfAir`.
- **C5 — RULED 2026-10-08 — the player's choice, as a mode.** (was: succession after death.) Permadeath
  on: death ends the run; off: the run continues. Marker: `text:docs/gates.md:permadeath`.
- **O2** (in the Findings table) is annotated `state=closed, marker=check:scripts/air-check.sh`; see
  the finding below.

### B5 — **O2 was already closed: the brief's premise is stale, and this is a finding**

The brief (from `docs/audit-whole-project.md` §4) states that **O2** "says the three independent
sources *are not modelled*". **It does not.** `docs/omissions.md:46`, on `testing` at `c3368c1`, reads
**"CLOSED 2026-10-10 (`feat/the-air`, `docs/evidence/the-air.md`)** — the three independent power
sources are modelled: ...". O2 was closed by commit `6c447b0` on `feat/the-air`, the same pass the
audit's stale clone (five commits behind) could not see. Verified:

```
$ git show HEAD:docs/omissions.md | grep -n 'CLOSED 2026-10-10' | head -1
46:| **O2** | **CLOSED 2026-10-10 (`feat/the-air`, `docs/evidence/the-air.md`)** — the three independent power sources are modelled: ...
```

So the register check **cannot** fail on O2 on this tree: the row is closed and its proof
(`scripts/air-check.sh`) exists. It is annotated closed and passes. The intended failure is
demonstrated on a scratch register carrying an O2-shaped **open** row, which the check catches:

```
$ printf '| **O2** | the three independent power sources are not modelled <!-- register-row id=O2 state=open marker=check:scripts/air-check.sh --> |\n' > build/register-demo.md
$ python3 tools/docs/check_register.py --register build/register-demo.md ; echo $?
FAIL  build/register-demo.md:1: row O2 still reads as unbuilt/unruled, but its marker resolves (scripts/air-check.sh exists)
1
```

The audit's stale claim about O2 is itself an instance of the defect class the pass is about — a
register row read from memory rather than from the tree. `docs/gates.md`'s residual O2 entry is the
honest one: it records that the *sources* are built and that the **holodeck reactor as a per-deck
source** is the remaining piece.

### B6 — where the marker approach is coarse

Stated in the check's own header as well as here, because it is the part that would be tempting to
skip:

- **A marker proves a symbol exists** — a file, a name in a file, a line of a ruling. It does **not**
  prove the symbol does what the row describes, that the row's prose is true, or that a capability is
  complete. `MinutesOfAir` existing does not prove per-deck air is *finished*; it proves the name is
  there.
- **`state=` is authored by the person correcting the row.** The check cannot tell whether that person
  was right; it can only catch a row whose own machine-readable marker has gone false.
- **Only annotated rows are read.** An unannotated stale row is invisible to it — which is why the
  rows left unannotated are listed below.

It is a smoke alarm, not a verdict.

### B7 — numbered limits

- **Claims registered: 6** (2 existing, 4 added).
- **Register rows annotated: 6** — `O2` and `C1`–`C5`, in `docs/omissions.md`.
- **Register rows left unannotated: 20** — `O1`, `O3`–`O19` (18 findings rows), `W1`, and `C6`.
- **Walkthrough rows annotated: 0.**

**Why the 20 were left unannotated.** The rule is the brief's: do not annotate a row whose claim cannot
be reduced to a marker.

- **`O3`–`O7`, `O9`–`O15`, `O18`** describe work that is genuinely unbuilt, and the honest marker would
  be a file or symbol that does not exist yet. Annotating them `state=open` with an invented future
  filename would pass, guard nothing until someone happens to use that name, and put a guess in the
  register. Several are also multi-item (O3's six canon systems, O6's five bills, O15's nine
  scenarios), which no single marker can stand for.
- **`O16`, `O17`, `O19`** are "built but unregistered" / a one-line citation fix, closed by a *ledger*
  edit recorded in `docs/gates.md`; their closure is already carried there, and each is closed in
  place with its evidence named. Annotating them was judged beyond the rows the review verified.
- **`O1` and `O8`** are closed with evidence (`docs/evidence/the-budgets.md`,
  `docs/evidence/the-character-layer.md`). They were not in the brief's verified set; annotating them
  would be a second pass's read, and this pass did not make it.
- **`W1`** (a dead officer's credentials) is deliberately deferred to the memory/consent layer; the
  capability and its closing layer are both named, but no symbol or file exists to mark it.
- **`C6`** (Track D's four questions) is the one contested row still open. Its closure is four owner
  rulings with no file, symbol or check to name; marking it would invent a marker.

The **walkthrough** was left unannotated. Its G-list is a narrative of gaps, not a status column
(`docs/audit-whole-project.md` §4 says it "carries running commentary rather than a status column"),
and the two rows the audit names — G13 (settled) and G5's first slice (built) — close against
evidence documents, not a single symbol. The brief allows them to be added "if the markers are
unambiguous"; they are not, so they were left, and this is the reason.

---

## The whole check, on the committed tree

```
$ scripts/docs-check.sh ; echo $?
==> docs: cited paths resolve
    177 markdown files, 1530 docs/ references, all resolve

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
    ... C1–C5 ...
    6 register row(s) annotated, all agree with the tree
0
```

Unit tests: `tests/tools/test_docs_check.py` goes from 16 to 29 tests (13 added: six for the
current-tense coded claim, seven for the register check). `scripts/test.sh` runs them. The check is
class 3 of `scripts/docs-check.sh`, which `scripts/test.sh` already wires in.

The three scripts the acceptance names, run on this tree:

```
$ scripts/test.sh            # crew + ship unit tests, tools, hooks, api, orders, harness, docs, python, shellcheck, patches
all checks passed            (exit 0)
$ scripts/docs-check.sh      # paths + owned numbers + register rows
... 6 claim(s) registered, all agree with their owner; 6 register row(s) annotated, all agree (exit 0)
$ scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts
    ... 2024 files: 2016 compiled and read back, 8 rejected by the compiler (the known set), 0 read-back failure(s)
    the 8 rejections are exactly the known, documented set          (exit 0)
```

`check.sh` needs the GDK content (fetched to the gitignored `build/gdk` by `scripts/fetch-gdk.sh`); it
runs no engine.

---

## What could not be verified, and judgement calls

**Could not verify:**

- **Whether a row's prose is still true beyond its marker.** The check proves a symbol exists; it
  cannot prove the sentence about it. The register's prose is a person's read, and this pass's read is
  the corrections in B4.
- **Whether the check's `state=` values are themselves right.** They are authored; the check cannot
  audit its own authour.
- **Whether the 20 unannotated rows are still accurate.** Only six rows are read mechanically; the
  rest are unguarded by this check, by construction.
- **Whether the 25-vs-20-bytes discrepancy is a units clash, a changed measurement, or one wrong
  number.** Two measurements disagree; reconciling them needs the measurement, which this pass did
  not re-run (no engine, no game data on this host).
- **Whether the save version should move.** This pass did not touch code; the constant stays 56.

**Judgement calls, named as calls:**

1. **The tense convention is the assertion form.** A whole-file "dated snapshot" exemption would have
   exempted the one wrong file; the assertion form separates a current claim from history without new
   syntax. Recorded in the registry and the check header.
2. **One named exemption (`docs/audit-whole-project.md`)**, because its table *quotes* another file's
   assertion rather than making one. A per-file list with a reason is narrower and more auditable than
   a heuristic.
3. **Code constants are owned by the code, extended beyond the save version** to the crew complement,
   the per-NPC budget and the hunk — the brief's rule, applied where a number is a code or launch fact.
4. **The brief's premise about O2 is stale and this is reported, not smoothed over.** O2 is already
   closed; the check is demonstrated on an O2-shaped scratch row instead. See B5.
5. **The handoff was reframed as a dated snapshot, not updated to 56.** A dated brief that restates the
   current constant is the defect the item is about; the value is left with the code.
6. **`text:` markers are allowed for a row whose closure is a ruling rather than a build** (C2, C3, C5):
   the proof is the recorded ruling in `docs/gates.md`. The check's header says what a marker does and
   does not prove.
7. **BSP size left unregistered** — a measurement with no owning file; naming one would be inventing an
   owner. Reported in A4 instead.
8. **Only the six verified rows annotated**, and the walkthrough left alone; the 20 unannotated rows and
   the reasons are listed in B7.

**Note on the brief's own presence:** `BRIEF.md` was in the tree while this pass ran and was deleted
before the commit, as it instructs. The counts above are re-taken with it absent.
