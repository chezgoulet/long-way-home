# Evidence — the two logs, and what a purge leaves behind

**Session 2026-10-31.** Stage D, slice 3. One branch, `feat/two-logs`, cut from `origin/testing`
after the meeting slice (#164). This slice answers the Stage D criterion:

> **A purge removes the published report and leaves the `MEM_LOG`-sourced marks orphaned, not erased.**

Save format **version 66**, unchanged. The record does not change shape: `Memory::event` already
carried a `uint16_t` and `Memory::orphaned` already existed.

## What already existed, established before writing

- **The two stores were already separate.** The published report is an ordinary `LogEntry` in the
  official `Ship::log` (`module/ship/ship_core.h`). A person's marks are the `Memory` vector in
  their own `CrewMember`. There is no type field shared between them; they are two stores with two
  lifetimes, which is what `docs/the-record-and-the-log.md` asks for.
- **The report is a document, not a store inside the log.** `PublishMeetingReport` writes it as
  ordinary log lines whose first entry opens `meeting report (...)`. It is a clearly separable
  kind by its own text, and the purge is whole-log by design (`PurgeLogs`; the month report's
  evidence, judgement 4, already records that the published log is the artifact and the diff in
  `reports` is the record). This slice did not add a second report store, because the separate
  stores the criterion needs are the log and the marks.
- **The purge/orphan rule already existed.** `PurgeLogs` clears `s.log` and sets `orphaned` on every
  `MEM_LOG` mark. `TestPurgeMovesRegard` already demonstrated that a purged citation still leaves
  the mark held.
- **What was missing was the meeting-facing writer and the reader.** `PublishMeetingReport` wrote
  no mark at all, so a purge after a meeting had nothing to orphan. And `ship recall` printed
  "read it in the log" even after the report it referred to was gone.

So this slice is a **purge path**, not a separation.

## What was built

- **The meeting writes the mark where the reading happened.** `PublishMeetingReport` now writes
  `MEM_MEETING` (event 11) into every crew member who could read the report's command scope and was
  **not** in the room, with source `MEM_LOG` and neutral valence. The people in the room were there;
  they are not given a read mark they did not earn, and nothing is silently reattributed to them.
- **A mark's provenance says what is true now.** `MarkProvenance` is the one place that turns a
  mark's source into words. A `MEM_LOG` mark with its report reads *read it in the published
  report*; an orphaned one reads *read it in a published report that has since been purged*. It
  never invents a replacement citation and never changes the source to `MEM_SAW` or `MEM_TOLD`.
- **The report's kind is named, not guessed.** `IsMeetingReport` is the one place that decides
  whether a log entry is a published meeting report (its opening line is `meeting report (`).
  `ship log` uses it to mark the line, and the tests use it rather than re-spelling the prefix.
- **The console reader says what is true now.** `ship recall <crew> <event>` now prints
  `MarkProvenance`, so the player reading a mark after a purge is told the report is gone rather
  than shown a stale "read it in the log".
- **The brief still builds.** `BuildBrief` reads marks by event, person, source, valence and
  salience; it does not resolve a citation, so it neither crashes nor invents a report when the
  report is gone. The check builds a brief after the purge to prove it.

## The command

```sh
scripts/two-log-check.sh
```

Static: no engine, no game data, no model and no network. It compiles the ship tests on the host,
runs the whole suite, prints the observable criterion, and then falsifies twice. The output below is
the run of this session.

```text
==> two logs: the unit test suite, built here

==> two logs: the report is a document, the marks are the person's (test_ship_core --two-logs)
== before the purge: the meeting's report is published
  the meeting: allocation in main engineering; decided "The chief's plan" (applied 1)
  report(s) in the official log: 1
  reader: Chakotay (crew 1, not in the room)
  reader's mark before: event 11 (a meeting); provenance read it in the published report; orphaned 0
== after the purge: the report is gone, the mark remains
  purge ran: 1; official log entries: 0
  meeting report(s) remaining: 0
  reader's mark after: held 1; event 11 (a meeting); provenance read it in a published report that has since been purged; orphaned 1
  not reattributed: source read-it-in-the-log, not saw-it and not told
  a brief still builds after the purge: 2 present, 2 agenda item(s)
  save and reload carry the orphaned mark and no report: log 0, orphan 1, identical 1
all demonstrations passed


==> two logs: falsification one (the purge does not orphan; the test must fail)
      FAIL  a purge removes the meeting's report and leaves its MEM_LOG marks orphaned: mark && mark->orphaned (line 7734)
      FAIL  a purge removes the meeting's report and leaves its MEM_LOG marks orphaned: mark && std::strcmp(MarkProvenance(*mark), "read it in a published report that has since been purged") == 0 (line 7735)
      FAIL  a purge removes the meeting's report and leaves its MEM_LOG marks orphaned: backOrphan (line 7754)
    the broken copy fails on the orphaned mark; the tree passes

==> two logs: falsification two (the meeting writes no mark; the test must fail)
      FAIL  a purge removes the meeting's report and leaves its MEM_LOG marks orphaned: mark != nullptr (line 7708)
      FAIL  a purge removes the meeting's report and leaves its MEM_LOG marks orphaned: mark && mark->source == MEM_LOG (line 7709)
      FAIL  a purge removes the meeting's report and leaves its MEM_LOG marks orphaned: mark && !mark->orphaned (line 7710)
      FAIL  a purge removes the meeting's report and leaves its MEM_LOG marks orphaned: mark && std::strcmp(MarkProvenance(*mark), "read it in the published report") == 0 (line 7711)
      FAIL  a purge removes the meeting's report and leaves its MEM_LOG marks orphaned: MemoryCount(s.crew[reader]) == beforeMarks + 1 (line 7712)
      FAIL  a purge removes the meeting's report and leaves its MEM_LOG marks orphaned: mark != nullptr (line 7731)
      FAIL  a purge removes the meeting's report and leaves its MEM_LOG marks orphaned: MemoryCount(s.crew[reader]) == beforeMarks + 1 (line 7732)
      FAIL  a purge removes the meeting's report and leaves its MEM_LOG marks orphaned: mark && mark->source == MEM_LOG (line 7733)
      FAIL  a purge removes the meeting's report and leaves its MEM_LOG marks orphaned: mark && mark->orphaned (line 7734)
      FAIL  a purge removes the meeting's report and leaves its MEM_LOG marks orphaned: mark && std::strcmp(MarkProvenance(*mark), "read it in a published report that has since been purged") == 0 (line 7735)
      FAIL  a purge removes the meeting's report and leaves its MEM_LOG marks orphaned: mark && std::strcmp(MarkProvenance(*mark), "saw it") != 0 (line 7737)
      FAIL  a purge removes the meeting's report and leaves its MEM_LOG marks orphaned: mark && std::strcmp(MarkProvenance(*mark), "was told") != 0 (line 7738)
      FAIL  a purge removes the meeting's report and leaves its MEM_LOG marks orphaned: backOrphan (line 7754)
    the broken copy fails on the missing mark; the tree passes

PASS  a meeting publishes its report into the official log and the people outside the room who
      could read it carry it as a MEM_LOG mark; a purge removes the report and leaves those marks held,
      orphaned and not reattributed; a reader tolerates the missing report and says so; and
      the falsifications fail for the reason they should.
```

## The unit test

`TestPurgeLeavesMeetingMarksOrphaned` (`tests/ship/test_ship_core.cpp`) asserts, in one place:

- the meeting publishes exactly one report into the official log;
- a crew member outside the room holds `MEM_MEETING` with source `MEM_LOG`, not `MEM_SAW`, and is
  not orphaned while the report exists;
- the people in the room hold no `MEM_LOG` meeting mark (the mark is not invented for them);
- after `PurgeLogs` the log is empty and no meeting report remains;
- the reader still holds the mark, the count is unchanged, the source is still `MEM_LOG`, and
  `orphaned` is set;
- `MarkProvenance` says the report has been purged, and does not say "saw it" or "was told";
- a brief still builds from the record once the report is gone;
- the orphaned mark and the empty log survive save and load byte-for-byte (`Pack(back) == blob`).

## The module compiles

The console readers (`ship recall`, `ship log`) live in `module/ship/g_ship.cpp`, so the module was
built as well as the tests. **The build was private to this session**, in a clone at
`/tmp/opencode/upstream-two-logs` with this tree's patch series applied, so the shared
`/home/c/big/git/upstream/efgame/build-linux` and the module symlinks beside it were not touched.

```text
$ cmake -S efgame -B efgame/build-linux -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release \
      -DLWH_MODULE_DIR=/home/c/work/lwh-two-logs/module
$ cmake --build efgame/build-linux -j12
...
[100%] Linking CXX shared library libefgame.so
[100%] Built target efgame
-rwxrwxr-x 1 c c 3023232 .../efgame/build-linux/libefgame.so
```

Exit 0. No engine process was left running.

## The required suites

`scripts/test.sh` and `scripts/check.sh`, with the content arguments recorded in
`docs/evidence/affinities.md` (`--source-map` a clean inhabited single-player map, `--script-corpus`
the GDK scripts, `--build-root` the upstream build). Both exit 0; the pasted runs are below.

```text
$ scripts/test.sh ; echo $?
...
==> ship simulation: unit tests
ship_core: all checks passed
...
==> hooks: the register matches the public surface
hooks-check: module/ship/ship_core.h declares 517 public functions; docs/hook-register.md registers 517 hooks
PASS  every public function is registered, and every registered hook exists
...
==> docs: the hook register's stated counts agree with its rows and the surface
    docs/hook-register.md: counts agree with the rows and the surface (517 hooks: play 312, console 163, nothing 37, content 5)
...
==> shell: shellcheck
    ok

==> patches: numbered without gaps, and each one parses
    24 patches

all checks passed
0
```

```text
$ scripts/check.sh \
    --source-map /home/c/big/git/long-way-home/build/gdk/maps/eliteforce_borg_maps/borg1.map \
    --script-corpus /home/c/big/git/long-way-home/build/gdk/scripts \
    --build-root /home/c/big/git/upstream ; echo $?
...
==> compiler corpus pass (every shipped script: compile, then read back)
    2408 files: 2394 compiled and read back, 14 rejected by the compiler, 0 read-back failure(s)
...
PASS  the 2 unresolved references are exactly the recorded boundary
0
```

## Judgement calls, named

1. **The report stays in the official log; no new store is added.** The criterion's "two logs" are
   the published document and the person's marks, and those were already distinct stores. Adding a
   `MeetingReport` vector would have changed the save shape to buy a separation that already
   exists. The report is identifiable by its own opening line and its marks, and the purge is
   whole-log by the document's own design.
2. **Only the people outside the room who could read the report get a `MEM_LOG` mark.** This
   mirrors `SignReport`, where the crew who did not see an event read the published version and
   remember it as read, and it keeps the log's own scope rule: command reads all, the command
   department reads its scope. The people in the room saw the meeting; giving them a read mark would
   invent a second provenance for something they already know. No reader is silently reattributed.
3. **One `MEM_MEETING` mark per person, not one per agenda item.** The event is the meeting itself;
   the agenda is already carried in the report and in the brief. A mark per agenda item would
   duplicate `MEM_DEATH`/`MEM_PROMISE` marks and move bonds through a second path, which is more
   than this criterion asks. The bounded per-person memory stays bounded.
4. **Neutral valence.** The meeting happened and was read; what it decided is in the report. A
   polarity would need a stance the reader outside the room does not have, and inventing one would
   be the kind of silent attribution this slice exists to prevent.
5. **No save-version bump.** `Memory::event` is already a `uint16_t`; `MEM_MEETING = 11` is a new
   value, not a new field. `Memory::orphaned` already existed. An older save still loads, and a save
   taken now replays identically.

## What this cannot show, said plainly

- **It does not identify which meeting.** The mark says *a meeting*, and its orphaned provenance
  says the report is gone; it does not carry a report id. The person remembers reading that a
  meeting was held, not the report's serial number. A citation id would be a save-shape change and
  was not needed for this criterion.
- **It does not make the purge selective.** A purge still clears the whole published log, as the
  month report's evidence records. The criterion is about lifetimes, not about deleting one report
  out of many.
- **It is not an in-engine run.** This is the static criterion: the model, the store and the reader,
  compiled and exercised on the host. The overlay's own in-engine run remains the meeting lane's
  evidence and was not repeated here.
