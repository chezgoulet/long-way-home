# Evidence — the G13 contradictions corrected, and the docs-consistency check

Date: 2026-10-09. Branch: `feat/the-corrections`, cut from `testing`. The pass the brief called
"the documentation contradictions — correct them, and catch the next ten". Documentation only: no
code, no engine, no `module/`.

Two things are recorded here: **the ten items of `docs/walkthrough.md` finding G13, each re-verified
and settled**, and **the check that catches two mechanical classes without being remembered**,
wired into `scripts/test.sh` and demonstrated failing on a planted error.

---

## Part 1 — the ten, settled

Each was re-checked against the code, the owning document and the evidence, not trusted from the
finding. "Annotated" means an evidence file, which is a dated record and is annotated rather than
rewritten; "corrected" means a plain design/state document, corrected in place.

| # | it said | it says now | settled by |
|---|---|---|---|
| 1 | computer core deck: `s1` prints 7, `s3`/`s5` say 9 | the **code is right** (deck 10) and the stale **evidence is annotated** (s1, s3, s5; and s7 as a fourth site the finding did not name) | `module/ship/ship_core.cpp` `SPECS[]` (deck 10, comment citing `docs/ship-master-map.md`); `docs/evidence/access-and-authority.md`, "Location appropriateness" |
| 2 | `gates.md` "the next work is the deck build order … deck 12 and deck 13 first" | all five absent decks are **built** (7, 12, 13, 14 re-dressed; 6 composed); the walkthrough is what closes them | `docs/ship-master-map.md`, "The deck build, in order" |
| 3 | `story-and-semantics.md` conflicts 1 (no abandonment list) and 7 (no counter) | both marked **RESOLVED** with their evidence | `docs/evidence/abandonment-list.md` (save 41); `docs/evidence/the-navigation-counter.md` (save 45) |
| 4 | month-report evidence judgement call 1: headline is `DilithiumRange`, "a new model" | **annotated SUPERSEDED** by `feat/the-navigation-counter` (save 45); the real counter's change is the headline | `docs/evidence/the-navigation-counter.md` |
| 5 | `access-and-authority.md`: dead credentials "still work" vs the acceptance "reported, not built" | the edge-case bullet **corrected** to match the acceptance: retained, using them reported-not-built, older phrasing withdrawn | the document's own acceptance; `docs/omissions.md` **W1** |
| 6 | `path-to-playtest.md`: Stage A "in flight", B/C pending | **A built** 2026-10-06, **B built** 2026-10-06, **C in progress** (decks built; walkthrough open) | `docs/evidence/environment-in-the-world.md`; `docs/evidence/player-in-the-world.md`; `docs/ship-master-map.md` |
| 7 | `HANDOFF.md` dated snapshot, stale throughout | **plainly marked a dated snapshot, superseded 2026-10-09**; the "keep this file current" instruction withdrawn | `docs/gates.md` (the ledger, authority for state) |
| 8 | Maquis contradiction "still present in `gates.md`" | **verified already corrected** in `gates.md` (omissions sweep); the stale defect note in `docs/path-to-playtest.md` corrected | `docs/gates.md`; `docs/omissions.md`, "Ledger corrections" |
| 9 | EMH called "Sickbay's `B` companion" | **annotated `E`, not `B`**; `B` is the surgical field | `module/ui/ui_lwh_engineering.cpp` lines 724–725 and the board footer |
| 10 | `budget-squaring.md` header says "nothing here is implemented" | **verified already corrected** (header says implemented 2026-10-07); one residual body sentence marked pre-ruling | the document's own header; `docs/evidence/the-budgets.md` |

Where the **code** was the wrong one: **nowhere in this pass.** In item 1 the code (deck 10) is the
source that settled the evidence; the code's entry had already been corrected on 2026-10-07 (save
version 50's system table), and this pass annotated the documents that still disagreed.

### What was annotated rather than rewritten, and the dates

- `docs/evidence/s1-ship-core.md` — **superseded 2026-10-07**: the `--day` transcript prints the
  computer core on deck 7 and the torpedo launchers deck 9. Annotated, transcript kept.
- `docs/evidence/s3-merged-map-measured.md` — **superseded 2026-10-07, same day**: "the computer
  core is on deck 9". Annotated.
- `docs/evidence/s5-stations.md` — **superseded 2026-10-07**: two table rows and the canon-placement
  paragraph name deck 9. Annotated.
- `docs/evidence/s7-intruders-and-control.md` — **superseded 2026-10-09**: a boarder objective walks
  "toward deck 9". Annotated. *(A fourth site, beyond the three the finding named.)*
- `docs/evidence/the-month-report-and-the-toll.md` — **superseded 2026-10-06, same day**: judgement
  call 1's `DilithiumRange` headline. Annotated at the call.
- `docs/evidence/backlog-materials-and-crew.md` — **correction 2026-10-09**: the EMH key slip.
  Annotated; the original phrase is kept visible.

In every case the original text is still there, with the date and what superseded it.

---

## Part 2 — the check

`scripts/docs-check.sh`, wired into `scripts/test.sh`. It covers the two classes the brief names as
mechanically verifiable:

1. **A cited `docs/...` path that does not resolve** (`tools/docs/check_paths.py`). Every `docs/...`
   token in every markdown file must be a file, a directory, or a glob that matches one. Paths that
   are *deliberately* cited although absent — because a document is recording a dangling reference
   (O19) or rejecting one (`docs/movement_physics/`) — are listed with their reasons in
   `tools/docs/allow-missing.txt`.
2. **A claimed value that contradicts its owning document** (`tools/docs/check_claims.py`, claims in
   `tools/docs/owned_claims.json`). The two claims registered today are the pair the finding turned
   on: the main computer core's deck and the modelled torpedo launchers' deck. Each names its owner
   (`docs/ship-master-map.md`), the value the owner fixes, and every other place that states it —
   including `module/ship/ship_core.cpp`, the arbiter for what is built.

Unit tests: `tests/tools/test_docs_check.py` (10 tests), run by `scripts/test.sh`.

### What it cannot catch, said plainly

- **Whether a description is still true** — a section that says a thing is unbuilt when it was built
  last week. That is a person's read; the class-2 check polices *numbers with an owner*, not prose.
- **A contradiction nobody registered.** Class 2 catches drift only in claims in
  `owned_claims.json`; registering one is a JSON row.
- **Stale claims in `docs/evidence/`.** The check deliberately does not scan evidence files, because
  they are dated records allowed to hold superseded values with an annotation. A stale value there is
  a person's to notice (the item-1 sites are exactly that).
- Whether the corpus contains contradictions nobody has read closely enough to notice.

### Demonstrated: failing on a planted error, passing when removed

Class 1 — a planted dangling path in `docs/confidence-and-verification.md`:

```
$ printf '\n<!-- planted for the demonstration: docs/evidence/does-not-exist-planted.md -->\n' >> docs/confidence-and-verification.md
$ scripts/docs-check.sh ; echo $?
==> docs: cited paths resolve
FAIL  docs/confidence-and-verification.md:45: cites `docs/evidence/does-not-exist-planted.md`, which does not exist
1 dangling docs/ reference(s) of 1169
1
```

```
$ git checkout -- docs/confidence-and-verification.md
$ scripts/docs-check.sh ; echo $?
==> docs: cited paths resolve
    157 markdown files, 1168 docs/ references, all resolve

==> docs: owned numbers agree with their owner
    computer-core-deck: docs/ship-master-map.md = 10; 4 statement(s) agree
    torpedo-launchers-deck: docs/ship-master-map.md = 10; 3 statement(s) agree
    2 claim(s) registered, all agree with their owner
0
```

Class 2 — a planted wrong deck in `docs/budget-squaring.md`'s demand table:

```
$ sed -i 's/| 3 | computer core | 10 |/| 3 | computer core | 9 |/' docs/budget-squaring.md
$ scripts/docs-check.sh ; echo $?
==> docs: cited paths resolve
    157 markdown files, 1168 docs/ references, all resolve

==> docs: owned numbers agree with their owner
FAIL  computer-core-deck: docs/budget-squaring.md:119 says 9, owner docs/ship-master-map.md says 10
2 claim(s) registered; at least one contradicts its owner
    torpedo-launchers-deck: docs/ship-master-map.md = 10; 3 statement(s) agree
1
```

```
$ sed -i 's/| 3 | computer core | 9 |/| 3 | computer core | 10 |/' docs/budget-squaring.md
$ scripts/docs-check.sh ; echo $?
==> docs: cited paths resolve
    157 markdown files, 1168 docs/ references, all resolve

==> docs: owned numbers agree with their owner
    computer-core-deck: docs/ship-master-map.md = 10; 4 statement(s) agree
    torpedo-launchers-deck: docs/ship-master-map.md = 10; 3 statement(s) agree
    2 claim(s) registered, all agree with their owner
0
```

### Counts re-taken on the committed tree (2026-10-09, landing pass)

The transcripts above read *157 markdown files, 1168 references* (1169 with the planted line). The
committed tree yields **156 files, 1156 references** (**1157** planted) — one file and twelve references
fewer, and the counts are the only thing that differs. The cause is this brief: `BRIEF.md` was still in
the tree when the transcripts were taken and is deleted before committing, as the brief requires, so the
recorded figures count a file the commit does not contain. **The demonstration reproduces exactly** — the
planted line reports at `docs/confidence-and-verification.md:45`, the run exits 1, and `git checkout --`
followed by a re-run exits 0.

### O19, re-verified still closed

The dangling reference `docs/gap-the-log` / `docs/gap-the-log.md` was fixed in the omissions sweep.
Re-checked 2026-10-09: it occurs only in `docs/omissions.md` (recording the fix) and `docs/gates.md`
(recording the fix); the live citations in `docs/the-record-and-the-log.md` (lines 20, 64) and
`docs/evidence/the-month-report-and-the-toll.md` all read `docs/evidence/gap-the-log.md`. The path
check would flag any recurrence, and `allow-missing.txt` names the two quoted-as-missing forms so the
records do not read as live citations.

---

## Part 3 — what could not be verified, and judgement calls

**Could not verify:** whether the corpus holds contradictions nobody has read closely enough to
notice. This pass settles the ten and catches two mechanical classes; it does not claim the corpus is
clean.

**Judgement calls, named as calls:**

1. **Item 8 was re-verified and found already corrected.** The finding said the Maquis contradiction
   was "still present in `gates.md`"; the code read on 2026-10-09 found only the two consistent
   entries. The stale text is `path-to-playtest.md`'s own defect note, which is what was corrected.
2. **Item 10 was re-verified and found already corrected in its header.** A residual body sentence
   ("Nothing here is implemented") was marked pre-ruling rather than left false.
3. **A fourth item-1 site was annotated** (`s7`), beyond the three the finding named — the same fact,
   found by the same read, so not a re-audit.
4. **`HANDOFF.md` was marked superseded, not brought up to date.** The brief allowed either and asked
   for the call to be named. Bringing it current would duplicate `docs/gates.md` and would rot again;
   the ledger is the authority, and a second state file is the defect the item itself is about.
5. **Class 2 is registry-based and does not scan `docs/evidence/`.** That is the honest boundary given
   the annotation rule: an evidence file is allowed to hold a superseded value with a note. The
   demonstration plants the error in a *live* document, where the check is meant to bite.
6. **The check's minimum-reference guard** (`--min-refs`, default 50) exists so a broken extractor
   cannot pass vacuously; the unit tests fix its negative behaviour.
