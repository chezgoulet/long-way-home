# Staff meetings — the per-participant brief, the agenda and the report

**Session 2026-10-31.** Stage D, slice 2. One branch, `feat/staff-meetings`, cut from
`origin/testing` after the affinities record (slice 1). This slice joins two things the corpus
already had: the meeting lane's overlay and async worker (M3/M4, built 2026-10-08/09 and recorded in
`docs/gates.md`), and the affinities priors merged the day before. The work here is the **brief they
consume**: per participant, built from marks, the log and a declared affinity, with an agenda
grounded in the record and a report published after the decision.

The two-log split was **not built here and was not needed**: the official log and the personal log
are already separate stores (`SAVE_VERSION` 47, `docs/the-record-and-the-log.md`). The Collective's
voice, voice input beyond the existing typed pill, and the computer-as-a-place are later slices and
were not half-built.

## What was built

- **The per-participant brief.** `BriefParticipant` now carries the person's declared `likes`,
  `dislikes` and `allegiance`; a **want** and a **will-not-say** derived from the strongest matching
  row, each naming its source; and a **stance** on every enumerated option with the affinity that
  explains it (`EnrichBrief`, `BriefStance`).
- **The disagreement is declared, not inferred from a roster average.** The authored skeleton marks
  how an option reads — `FLAVOUR_STARFLEET`, `FLAVOUR_MAQUIS`, or a taste topic. A participant's own
  allegiance or taste answers it. `TestMeetingBriefAffinity` holds two people in the same room with
  opposite stances and the rows that produce them.
- **The agenda is grounded in the record.** `MeetingBrief.agenda[]` carries the call, the instrument
  the room argues over, and each participant's strongest mark, open promise and readable log line.
  It is deduplicated and bounded; it is not a global summary.
- **The async generator consumes the brief.** `tools/meetings/worker.py` writes the agenda and every
  participant's wants, silences and stances into its prompt, and requires the disagreement to be
  written. It still invents neither options nor state, and it writes no script when the model is
  absent.
- **The report is published by the simulation.** `PublishMeetingReport` writes the decision, every
  agenda item with whether the decision addressed it, and the dissent a declared affinity explains,
  into the official log. The model has no route into it.
- **No save change.** The new view is a read of the saved brief and the static record; `EnrichBrief`
  rebuilds it after a load, and the round-trip test checks it. `SAVE_VERSION` remains **66**.

## The command

```sh
scripts/meeting-check.sh
```

Static: no engine, no game data, no model and no network. It compiles the ship tests on the host,
runs the whole suite, prints the observable criteria, and then falsifies twice. The output below is
the run of this session.

```text
== the brief, per participant: what each attendee knows, wants and will not say
  BRIEF: allocation in main engineering; 2 present; 5 agenda item(s)
  PARTICIPANT: Kathryn Janeway -- Starfleet (+0.85) [authored]
    likes: "coffee" (+0.90)[authored] "the Gothic holonovel" (+0.70)[authored] "Leonardo da Vinci's workshop" (+0.70)[authored] "seal" (+0.74)
    dislikes: "the shooting range" (-0.50)
    knows: 0 mark(s), 0 promise(s), 2 log line(s)
    wants: wants "coffee" kept (authored like +0.90)
    will not say: will not say what they think of "the shooting range" (declared dislike -0.50)
    option 1 "The chief's plan" (Starfleet procedure): SUPPORTS -- Starfleet (+0.85) reads it as Starfleet procedure
    option 2 "Refuse the chief" (Maquis pragmatism): OPPOSES -- Starfleet (+0.85) reads it as Maquis pragmatism
    option 3 "Run the ship's own ladder" (neutral): neutral
    option 4 "The holodecks full, the shields dark" (neutral): SUPPORTS -- likes "the Gothic holonovel" (+0.70)
  PARTICIPANT: B'Elanna Torres -- the Maquis (+0.85) [authored]
    likes: "repair" (+0.85)[authored] "young ensigns" (+0.85) "seal" (+0.50) "the orbital skydiving program" (+0.45)
    dislikes: "by-the-book officers" (-0.60)[authored] "woodworking" (-0.80)
    knows: 0 mark(s), 1 promise(s), 1 log line(s)
    wants: wants "repair" kept (authored like +0.85)
    will not say: will not say what they think of "woodworking" (declared dislike -0.80)
    option 1 "The chief's plan" (Starfleet procedure): OPPOSES -- the Maquis (+0.85) reads it as Starfleet procedure
    option 2 "Refuse the chief" (Maquis pragmatism): SUPPORTS -- the Maquis (+0.85) reads it as Maquis pragmatism
    option 3 "Run the ship's own ladder" (neutral): neutral
    option 4 "The holodecks full, the shields dark" (neutral): SUPPORTS -- likes "the orbital skydiving program" (+0.45)
  AGENDA 1 [the call]: the allocation is argued: 1350 EPS committed of 1350 available
  AGENDA 2 [an instrument]: the plant: 1350 committed of 1350 available (0 short)
  AGENDA 3 [a promise] raised by a participant: a promise B'Elanna Torres made to Crewman 021: the coolant line
  AGENDA 4 [the log] raised by a participant: the log Kathryn Janeway can read: B'Elanna Torres promises Crewman 021: the coolant line
  AGENDA 5 [the log] raised by a participant: the log B'Elanna Torres can read: a coolant line was replaced
== a meeting plays with the model absent: the authored skeleton is the floor
  no generated script is loaded; the skeleton has 4 outcome(s); outcome 1 resolved=1
  the meeting changed state only through the simulation's own apply; the brief is a read
== the published report addresses the agenda
  LOG: meeting report (allocation in main engineering): Kathryn Janeway decided "The chief's plan"; 5 agenda item(s)
  LOG: meeting agenda 1 (the call): the allocation is argued: 1350 EPS committed of 1350 available -- addressed by the decision
  LOG: meeting agenda 2 (an instrument): the plant: 1350 committed of 1350 available (0 short) -- the decision answers it
  LOG: meeting agenda 3 (a promise): a promise B'Elanna Torres made to Crewman 021: the coolant line -- recorded; the promise still stands
  LOG: meeting agenda 4 (the log): the log Kathryn Janeway can read: B'Elanna Torres promises Crewman 021: the coolant line -- heard, and recorded for the next watch
  LOG: meeting agenda 5 (the log): the log B'Elanna Torres can read: a coolant line was replaced -- heard, and recorded for the next watch
  LOG: meeting dissent: B'Elanna Torres opposed "The chief's plan" -- the Maquis (+0.85) reads it as Starfleet procedure
  report(s) 1; agenda line(s) 5 of 5; dissent line(s) 1
== the same state builds the same brief (determinism from the seed)
  two builds: agenda 2/2, same text and stances yes

==> meeting: the generator is deterministic and soft without a model (tests/tools)
Ran 14 tests in 0.115s

OK
...
    the worker writes the same script twice, and writes no script when the model is absent

==> meeting: falsification one (the stance rule ignores affinity; the brief test must fail)
      FAIL  a brief is built per participant from marks, the log and a declared affinity: f.stance[starOpt] == STANCE_SUPPORTS (line 6324)
      FAIL  a brief is built per participant from marks, the log and a declared affinity: m.stance[starOpt] == STANCE_OPPOSED (line 6325)
      FAIL  a brief is built per participant from marks, the log and a declared affinity: f.stance[maqOpt] == STANCE_OPPOSED (line 6326)
      FAIL  a brief is built per participant from marks, the log and a declared affinity: m.stance[maqOpt] == STANCE_SUPPORTS (line 6327)
    the broken copy fails on the per-participant brief; the tree passes

==> meeting: falsification two (the report is not published; the report test must fail)
      FAIL  the published report addresses the agenda and names the dissent: reports == 1 (line 6382)
      FAIL  the published report addresses the agenda and names the dissent: agendaLines == b.agendaCount (line 6383)
      FAIL  the published report addresses the agenda and names the dissent: dissent > 0 (line 6384)
      FAIL  the published report addresses the agenda and names the dissent: found (line 6392)
    the broken copy fails on the report; the tree passes

PASS  a meeting plays with the model absent; two participants receive different briefs for the
      same meeting, traceably to a declared affinity; the agenda is grounded in the record and
      the published report addresses every item and names the dissent; the brief and the
      generator are deterministic; and the falsifications fail for the reason they should.
```

## PASS lines, and what each one is evidence of

- **PASS — the brief is per participant, and the two are different.** Janeway and Torres are in the
  same allocation meeting. Janeway wants coffee kept and will not say what she thinks of the shooting
  range; Torres wants repair kept and will not say what she thinks of woodworking. Each want and
  silence names the row it came from. This is the Stage D criterion, on the model.
- **PASS — the disagreement is traceable.** On option 1, "The chief's plan", the skeleton declares
  `Starfleet procedure`; Janeway supports it because `Starfleet (+0.85) reads it as Starfleet
  procedure`, Torres opposes it because `the Maquis (+0.85) reads it as Starfleet procedure`. On
  option 2 the signs are exactly reversed. The reasons are the rows, not an explanation written
  afterwards.
- **PASS — the agenda is grounded in the record.** Item 1 is the call; item 2 is the plant; item 3 is
  the open promise Torres made to Crewman 021; items 4 and 5 are log lines each person can read, from
  their own scope. Nothing is a roster summary.
- **PASS — the meeting plays with the model absent.** No generated script is loaded; the authored
  skeleton has four outcomes and outcome 1 resolves. The unit test is the model-free floor; the
  worker test below adds the other half: with no model, the worker writes no script.
- **PASS — the report is published and addresses every item.** One report line, five agenda lines
  (5 of 5), and one dissent line naming Torres and the Maquis row that explains it. The report is in
  the official log, read back by `ReadOfficialLog`.
- **PASS — determinism.** Two builds from the same state produce the same agenda text and the same
  stances. The worker test produces the same script twice from the same brief.
- **PASS — the guard has teeth.** Making `BriefStance` return neutral fails the per-participant test;
  removing the `PublishMeetingReport` call fails the report test. Both are shown above, then the tree
  passes.

## The gates, as run this session

```sh
scripts/test.sh
# exit 0; crew_core all checks passed; ship_core all checks passed (the full suite, including
# TestMeetingBriefAffinity and TestMeetingReportAddressesAgenda); 14 worker tests; 515 hooks
# registered against 515 declared; docs-check clean; "all checks passed".

scripts/check.sh \
  --source-map /home/c/big/git/long-way-home/build/gdk/maps/eliteforce_borg_maps/borg1.map \
  --script-corpus /home/c/big/git/long-way-home/build/gdk/scripts \
  --build-root /home/c/big/git/upstream
# exit 0. It re-runs scripts/test.sh, then:
#   entity dictionary: 318 classes parsed, 212 with documented keys, 232 spawnflag sets
#   validator negative tests: ALL PASS (E001-E010, E011 and W005)
#   compiler corpus: 2408 files, 2394 compiled and read back, 14 rejected by the compiler
#   script references: 2 unresolved, exactly the recorded boundary
#   "all checks passed"

scripts/meeting-check.sh
# exit 0; the output above.
```

`docs-check.sh` is clean, including the save-version claim: the owner still defines
`SAVE_VERSION = 66`, and the new brief view is not stored, so no save statement moved.

**Build note.** The module was configured and built against the pinned upstream with this tree's
`module/` (`cmake -S ../upstream/efgame -B /tmp/lwh-module-build -DLWH_MODULE_DIR=$PWD/module`), so
the new `g_ship.cpp` manifest fields and the new `ship_core.h` surface are compiled, not just read.
It is not an engine run.

## What could not be run here, and is not claimed

- **The in-engine overlay run.** `scripts/meeting-check.sh --engine` (the queue, the allocation
  decision and the skeleton on arrival under `xvfb-run`) needs the game data and a built engine
  binary in this tree; neither is present on this host, and it was not run. The overlay and the
  no-model arrival were demonstrated in the meeting lane's own evidence
  (`docs/evidence/meeting-overlay.md`, `docs/evidence/model-call.md`); this slice's criterion is the
  brief, the agenda and the report, and those are shown above.
- **A live model's prose.** No model was contacted. The worker test uses a fixed answer to prove
  determinism and the absent-model path; whether the generated dialogue reads well is the owner's
  judgement and is not claimed.
- **The Collective's voice, voice input beyond the typed pill, and the computer-as-a-place.** Out of
  scope for this slice, and not half-built.
- **Allegiance's valence and compliance weighting.** The record now weights a meeting's stance, but
  the Maquis/Starfleet *morale* consequence of an event or an order is still not wired.

## Reproduce

```sh
scripts/meeting-check.sh        # the demonstration and the two falsifications
scripts/test.sh                 # the whole check suite, including the new unit tests
scripts/check.sh --source-map <a clean inhabited single-player .map> \
                 --script-corpus <the GDK scripts directory> --build-root <the upstream build>
```
