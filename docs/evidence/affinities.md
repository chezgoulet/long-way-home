# Affinities and allegiance — the priors

**Session 2026-10-31.** Stage D, slice 1: the record named as the missing prior in
`docs/affinities-and-allegiance.md`, and the two entry points it is allowed to use. One branch,
`feat/affinities`, one slice; the meeting overlay, the per-participant brief, the two-log split and
the Collective's voice are not in this pass.

## What was built

- **The record.** On every `CrewMember`: `likes[]`, `dislikes[]`, `pursuits[]`, and an `allegiance`
  with a strength and what it is to. The whole roster is generated from the roster seed
  (`DeriveAffinities`), authored over for the named few where canon speaks, and derived again on
  load. Each row carries `authored` so the two sources are distinguishable.
- **Valence.** `EffectiveValence` sits at the one existing mark writer, `Remember`. An event that
  names a thing the person likes raises the mark by that strength; a thing they dislike lowers it;
  no match leaves the base exactly as it was. The bond store is unchanged — the fold over
  `valence × salience` reads whatever the writer produced.
- **Choice.** `PreferredProgram` is the program a recreation hour on the holodeck picks. `PreferredTask`
  is the job kind a person asks for, and `VolunteerJob` returns the queue entry that answers it.
- **No save change.** The record is static content, like species, skills and the face: generated
  from the seed and never stored. `SAVE_VERSION` remains 66 and an older save still loads.

## The command

```sh
scripts/affinity-check.sh
```

It compiles the ship tests on the host (no engine, no game data), runs the suite, prints
`test_ship_core --affinities`, and then falsifies twice: a valence fold that ignores dislikes, and a
schedule that stops picking the liked program. The output below is the run of this session.

```text
==> affinities: the unit test suite, built here

==> affinities: the record and the two entry points (test_ship_core --affinities)
  the record: 141 crew, 432 likes, 218 dislikes, 205 pursuits, 16 authored rows
  AUTHORED: Kathryn Janeway -- likes "coffee" (food, +0.90); allegiance Starfleet (+0.85)
  AUTHORED: Harry Kim -- likes "clarinet quartets" (music, +0.90)
  AUTHORED: The Doctor -- likes "Photons Be Free" (program, +0.90)
  AUTHORED: B'Elanna Torres -- allegiance the Maquis (+0.85)
  NOT AUTHORED: Seven's music studies (absent, as required), Chakotay's boxing (absent, as required) -- the two the document could not verify
  EVENT: "coffee", base valence +0.10, the same order to both
  CREW A: Kathryn Janeway -- likes "coffee" (+0.90): effective +1.00, mark +1.00, bond +1.00
  CREW B: Tuvok -- dislikes "coffee" (-0.50): effective -0.40, mark -0.40, bond -0.40
  PROGRAM: Kathryn Janeway -- recreation on deck 6, program "the Gothic holonovel" (liked +0.70)
  TASK: Tom Paris asks for job 0 (repair); the queue's first repair is job 0


==> affinities: falsification one (the valence fold ignores dislikes; the same-event test must fail)
      FAIL  a like raises and a dislike lowers the same event's valence for two crew: lowered < base (line 629)
      FAIL  a like raises and a dislike lowers the same event's valence for two crew: std::fabs(lowered - (base - dislike.strength)) < 1e-5f (line 631)
    the broken copy fails on the same-event test; the tree passes

==> affinities: falsification two (the schedule stops picking the liked program; the choice test must fail)
      FAIL  choice: the liked program and the loved task are selected: s.crew[who].program == "Captain Proton" (line 682)
    the broken copy fails on the choice test; the tree passes

PASS  the record is generated for the whole crew and authored where canon speaks; the same
      event lands positively on the one who likes it and negatively on the one who dislikes it,
      through the one mark writer and the unchanged bond fold; the schedule picks the liked
      program and the queue has the task the person asks for; and the guard fails on a valence
      fold that ignores dislikes and on a schedule that does not pick the program
```

## PASS lines, and what each one is evidence of

- **PASS — the record is the whole crew, and bounded.** `the record: 141 crew, 432 likes, 218
  dislikes, 205 pursuits, 16 authored rows`. Each list is bounded (`AFFINITY_MAX` 4,
  `PURSUIT_MAX` 2); the counts are the printed record, not a claim.
- **PASS — the authored few are canon, and the unverified two are absent.** Janeway's coffee, Kim's
  clarinet, the Doctor's *Photons Be Free* and Torres's Maquis allegiance are printed as authored.
  Seven's music studies and Chakotay's boxing print as absent, as required by the design document.
- **PASS — the same event lands differently, traceably.** A base valence of `+0.10` on the event
  "coffee" becomes `+1.00` for Janeway (a declared like, +0.90) and `-0.40` for Tuvok (a declared
  dislike, -0.50). The two marks are what the bond fold reads: `Bond` prints `+1.00` and `-0.40`.
  This is the Stage D exit criterion, in the model.
- **PASS — no match leaves the bond fold alone.** `TestBondFoldUnchangedWithoutAffinity` writes a
  mark with an unrelated like on the record and checks the mark and `Bond` are exactly the base
  valence. The valence mechanism is a modifier, not a second store.
- **PASS — choice is observable.** At 17:30 (the alpha watch's recreation) Janeway is on the
  holodeck deck with `the Gothic holonovel`, the program her record likes; and with a repair
  outstanding, Tom Paris asks for job 0, the repair.
- **PASS — the guard has teeth.** Making the valence fold ignore dislikes fails the same-event test;
  making the schedule stop picking the liked program fails the choice test. Both are shown above,
  then the tree passes.

## The gates, as run this session

```sh
scripts/test.sh
# exit 0; crew_core all checks passed, ship_core all checks passed (172 test functions, 2,328 CHECK
# sites in tests/ship/test_ship_core.cpp, including the four new affinity tests), 143 Python tool
# tests, 510 hooks registered, 24 patches well formed, "all checks passed".

scripts/check.sh \
  --source-map /home/c/big/git/long-way-home/build/gdk/maps/eliteforce_borg_maps/borg1.map \
  --script-corpus /home/c/big/git/long-way-home/build/gdk/scripts \
  --build-root /home/c/big/git/upstream
# exit 0. It re-runs scripts/test.sh (above), then:
#   entity dictionary: 318 classes parsed, 212 with documented keys, 232 spawnflag sets
#   validator negative tests: 12 of 12 PASS (E001-E011 and W005)
#   compiler corpus: 2408 files, 2394 compiled and read back, 14 rejected by the compiler
#                    (exactly the documented set), 0 read-back failures
#   script references: 2 unresolved, exactly the recorded boundary
#   "all checks passed"
```

`docs-check.sh` (inside `test.sh`) is clean, including the save-version claim: the owner still
defines `SAVE_VERSION = 66` and every asserted statement agrees. The hook register moved from 502 to
510 rows, and its marker is re-derived and checked.

**Fixture note, so the next run does not repeat it:** `check.sh`'s `--source-map` must be a clean,
inhabited single-player map with navigation entities. A holomatch map (the first attempt used
`hm_temple.map`) fails E003 as a clean fixture; `borg1.map` is the one that works. The GDK content and
the upstream tools were read-only inputs from the existing checkouts, not vendored into this tree.

## What could not be run here, and is not claimed

- **The in-engine observation of the program.** This check exercises the model directly. Watching a
  crew member walk to the holodeck and start the liked program in the running game needs a session
  with the game data and a person at the screen; it was not run.
- **The meeting overlay, the per-participant brief, the two-log split and the Collective's voice.**
  Out of scope for this slice, and not half-built.
- **Allegiance's behavioural weighting.** The record carries the allegiance and its strength; the
  document's Maquis/Starfleet effect on how an event lands and whether an order is carried is
  **not** wired in this slice. It is recorded rather than claimed.
- **A pursuit producing an artifact or event.** The pursuits are recorded; the artifact/event and
  the log line it feeds are not built.

## Reproduce

```sh
scripts/affinity-check.sh        # the demonstration above
scripts/test.sh                  # the whole check suite, including the new unit tests
scripts/check.sh --source-map <a clean inhabited single-player .map> \
                 --script-corpus <the GDK scripts directory> --build-root <the upstream build>
```
