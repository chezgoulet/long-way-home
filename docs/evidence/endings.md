# Evidence: endings — destinations as readers, neutrality, and the three-second arrival

**Session 2026-10-31.** Stage E of `docs/path-to-playtest.md`, cut from `origin/testing` on
`feat/endings`. The specification is `docs/endings.md`; `docs/borg-incursion.md`'s loss condition is
the one unwinnable end and is unchanged.

## What was built

**Destinations are readers.** `module/ship/ship_core.{h,cpp}` now carries a destination table of six
readers, each with a name, the reader, the part of the record it reads, and what makes it part of the
run's accrued set. The set is derived from the record (`DestinationsFor`): **Earth is always open**,
and the others are earned. A destination is not a score screen: `ReadEnding` returns the evidence
that reader reads and the conclusion in that reader's terms, and `EndingReading` has no rank or
quality field. The same run read by Earth and by the Klingon Empire reaches different evidence and a
different conclusion.

**Neutrality is an assertion, not a promise.** `EndingToneFindings` scans the shipped table and one
reading of every destination for comparative and tone words (`better`, `worse`, `best`, `worst`,
`finally`, `home`, `safe`, and the rest of the list in the source). It matches whole words, so
`reader` is not `read` and a proper name is not a verdict. The check is in the model and the test
suite, not in a pull-request body.

**The arrival and the log.** `ARRIVAL_MS` is 3000 and `ARRIVAL_SECONDS` is 3 — one value, not two
copies. The engine side (`module/ship/g_ship.cpp`) starts the arrival on the engine's own cinematic
camera (`CGCam_Enable`, `CGCam_Move`, `CGCam_Fade`) and releases it on the same 3000 ms timer. The
destination is chosen at the end (`ship destinations`, `ship ending <name> [nofile]`); after the
three seconds the module prints the reader's reading and then the official log, and the log is the
last thing printed. The same timer and camera run for every destination and every route.

**Declining to file.** `docs/endings.md` says arriving at Earth and reporting to Starfleet are
separable, and that whichever way the question is answered the game cuts to the log. The build takes
the document at its word: **the player may decline to file**. `ReadEnding(s, DEST_EARTH, false)`
returns a reading with `read == false`, "the ship arrived at Earth; the record was not filed. It is
handed back unread", and the log is still the last frame. No destination's arrival is conditional on
filing, and filing is offered only at Earth (`MayFileAt`).

**No save change.** The destination and the filing decision are a one-shot ending read. They are host
state, derived at the end, and never written into the record, so `SAVE_VERSION` stays **66** and an
older save still loads. The end-of-run log line changed from "the ship is home" to "the run is at its
end", the sector readout from `HOME` to `END`, and the panel's navigation line from `HOME` to `GOAL`,
so the ending's own log and UI do not use the tone word `home`.

## The command

```sh
scripts/ending-check.sh
```

It compiles the ship tests on the host (no engine, no game data), runs the suite, prints
`test_ship_core --endings`, and then falsifies twice: a destination renamed "the best Earth" must
fail the neutrality assertion, and an arrival shortened to 2500 ms must fail the arrival assertion.
The output below is this session's run.

```text
==> endings: the unit test suite, built here

==> endings: destinations, readers, neutrality and the arrival (test_ship_core --endings)
== the set the run accrued
  the record: 141 crew, 0 dead, 0 held minds, 0 promises, 0 signed reports
  OFFERED (default run): Earth
  EARTH ALWAYS: 1
  the earned run: 141 crew, 1 dead, 1 held mind, 1 promise kept, 1 trader met, the log purged
  OFFERED (earned run): Earth, The Klingon Empire, A Delta Quadrant settlement, The Collective, A sale of the ship, No reader
== the same record, two readers
  Earth line: month reports signed: 1; drafted and unsigned: 0
  Klingon line: deaths recorded: 1
  readings differ: 1
  each names what it reads: 1
  Earth reading: Starfleet reads the month reports and the regulations; the reading is the record as filed, with every difference between the draft and the signed report still in it.
  Klingon reading: The Klingon Empire reads the deaths and the word given: the dead are named, the funerals are counted, and each promise is read as kept or broken.
  EARTH FILED: read 1; EARTH UNFILED: read 0, and the log is still the last frame
== neutrality
  NEUTRALITY: 0 findings in the shipped strings (6 destinations x 4, plus 6 readings)
  ORDER: Earth, The Klingon Empire, A Delta Quadrant settlement, The Collective, A sale of the ship, No reader
  RANK FIELD: none (the order is the design's, not a ranking)
== the arrival
  ARRIVAL: 3000 ms (3 s), the engine's own camera and fade; one clip for every destination and route
  THE LOG: the last frame is the official log (1 entries)
  LAST: the run is at its end
PASS  the run reaches more than one destination; the same record is read differently by reader; no comparative or tone word is in the shipped strings; the arrival is three seconds and the log is the last frame


==> endings: falsification one (a destination named 'the best Earth'; the neutrality assertion must fail)
    the planted destination fails the same assertion; the tree passes

==> endings: falsification two (the arrival shortened to 2500 ms; the arrival assertion must fail)
    the shortened arrival fails the same assertion; the tree passes

PASS  one run reaches more than one destination; the same record is read differently by two
      readers, each naming its evidence; no comparative or tone word is in the shipped ending
      strings; the arrival is 3000 ms and the log follows it; and the guard fails on a planted
      'best' and on a 2500 ms arrival
```

## PASS lines, and what each one is evidence of

- **PASS — one run reaches more than one destination.** The earned fixture is one record with a
  death and a funeral, a kept promise, a taken mind, a trader met and a purged log; all six
  destinations are open from it. The default run prints only Earth, which is the design's "at least
  one is always open" rather than a claim that every run has six.
- **PASS — the same record is read differently by reader, with the evidence named.** Earth's first
  line is `month reports signed: 1`, the Klingons' is `deaths recorded: 1`; the conclusions name the
  month reports and the deaths respectively. `readings differ: 1` and `each names what it reads: 1`
  are computed, not asserted in prose.
- **PASS — neutrality is over the shipped strings.** `NEUTRALITY: 0 findings` scans 6 destinations ×
  4 fields plus a reading of each one; `RANK FIELD: none` records that there is no field to sort by.
  The falsification plants `the best Earth` and the same function finds `best in "the best Earth"`.
- **PASS — the arrival is three seconds and the log follows.** `ARRIVAL: 3000 ms (3 s)` is the one
  constant the module and the check read; `THE LOG: the last frame is the official log` is the hand
  back. The second falsification shortens the constant to 2500 ms and the same assertion fails.
- **PASS — filing is separable from arriving.** Earth read with `filed = true` has `read == 1`;
  the same record read with `filed = false` has `read == 0` and a different conclusion, and the log
  is still the last frame.

## The gates, as run this session

```sh
scripts/test.sh
# exit 0; "all checks passed": crew_core and ship_core (including TestEndingReaders,
# TestEndingAvailability, TestEndingNeutrality and TestEndingArrival), the Python tool tests, the
# hook register (526 functions, 526 hooks), the patch series (24 patches), docs-check, py_compile
# and shellcheck.

scripts/check.sh \
  --source-map /home/c/big/git/long-way-home/build/gdk/maps/eliteforce_borg_maps/borg1.map \
  --script-corpus /home/c/big/git/long-way-home/build/gdk/scripts \
  --build-root /home/c/big/git/upstream
# exit 0. A bare invocation is a usage error; this is the recorded content invocation from
# docs/evidence/affinities.md.

scripts/docs-check.sh
# exit 0: cited paths resolve, owned numbers agree, register rows agree, hook counts agree.
```

**The module also builds against the engine.** A separate build tree (so the shared checkout's
`build-linux` was not disturbed) configured our module and built both libraries:

```sh
cmake -S /home/c/big/git/upstream/efgame -B /home/c/big/git/upstream/efgame/build-lwh-endings \
  -DLWH_MODULE_DIR=/home/c/work/lwh-endings/module
cmake --build /home/c/big/git/upstream/efgame/build-lwh-endings -j4
# [100%] Built target efgame ; libefgame.so (3,058,184 bytes)
```

That proves the arrival's engine calls (`CGCam_Enable`, `CGCam_Move`, `CGCam_Fade`) compile and link
against the engine and its camera. It does not prove the rendered arrival; that is the stand-in named
below.

## What could not be run here, and is not claimed

- **The rendered arrival was not watched in a graphical session.** The module compiles against the
  engine's own camera and times the camera move and the fade at 3000 ms, but this static host did not
  load the ship and record a frame sequence. The arrival's *duration* is proven by the one constant
  the module and the check share; its *rendered look* is not claimed.
- **Visually, the arrival is a stand-in against the project's own bar.** The engine renders it — a
  96-unit camera move over the map the player is standing in, with a white-to-clear fade and the
  engine's cinematic bars — but the project ships no Voyager exterior model and no system skybox, so
  it is not yet the show's shot of the ship coming out of warp into a system. The refusal to script
  an arrival is honoured (the camera and the fade are the engine's own primitives, no cutscene and no
  external footage); the picture is the part still owed.
- **No destination was witnessed from a save.** The destination and the filing decision are host
  state and are not saved, deliberately: the ending is one shot with nothing after the log to replay.
  The reading is a pure function of the record, so the same save at the same record reads the same.

## Decisions taken where the document leaves a choice

- **The names.** The document's names are used except where a name is itself a verdict: "A Delta
  Quadrant settlement" is the document's "A Delta Quadrant home" (and its own "settlement offer"),
  "A sale of the ship" is "The cynical one", and "The Collective" is the reader inside "A
  half-assimilated arrival". The departures are recorded in the source beside the table and in the
  PR for the owner.
- **Availability from the record, not a reputation field.** There is no reputation field in the
  module, and adding one would be a second store. The accrued set is read from what play already
  writes: funerals and kept promises for the Klingons; a kept promise, no broken one, and a crew
  that could settle for the settlement; a mind held for the Collective; a trader met and a hull for
  the sale; a purged log (or no crew) for no reader. Earth is always open.
- **The order is not a ranking.** The table is in the order `docs/endings.md` names the destinations,
  and there is no rank, score or quality field. The check prints the order and the absence of a rank
  field rather than leaving it to be inferred.
- **The player may decline to file.** The document asks the question and does not answer it; the
  build answers **yes**, and the reading says the record was not filed and is handed back unread. The
  arrival still happens, the counter still stops, and the log is still the last frame.
