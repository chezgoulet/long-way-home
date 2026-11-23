# Evidence: the crew's veto at the destination

**Session 2026-11-08.** Stage E's deferred item, `docs/endings.md`, "Structure": *"The crew get a
vote … the final expression of the captain's failure mode — losing the crew's confidence — is the run
where they choose the destination instead of you. Recommended: the veto fires once, at the
destination, and nowhere earlier."* The acceptance line is: *"The crew's veto can fire once at the
destination, and the log records who holds the conn after it does."*

## What was built

- **The read.** `CrewConfidence(const Ship&)` is the mean morale of everyone still aboard, where morale
  is already a read over deficit, outlook and holdings (`docs/morale.md`) and never a stored counter.
  `CrewWouldVeto` is that read against `CREW_CONFIDENCE_VETO` (35%, ours; a fresh crew reads 77%).
  Both are in `module/ship/ship_core.{h,cpp}`.
- **The choice.** `ChooseEnding(s, requested, vetoAlreadyFired)` is pure. When the confidence is gone
  and the crew disagrees with the course, it returns the place the crew choose, the person who holds
  the conn, and the reason. The crew choose from what the run accrued, in the design's order of
  claims: a settlement they would stay for, otherwise the Empire that reads the dead, otherwise Earth,
  which is always open. It is not a preference score and it is not ranked.
- **The once-only latch.** The caller passes `vetoAlreadyFired`; the game sets it when the veto has
  fired, and a second choice in the same endgame returns the requested destination unchanged.
- **The log.** `RecordEndingChoice` writes the fact into the official log with the conn-holder as the
  entry's **`who`** field. The record therefore names the person who holds the conn; the reading that
  follows is unchanged and the log is still the last frame.
- **Where it is reached.** `ship ending <destination> [nofile] [conduit]` asks the veto after the
  route is settled, and `ship wildcard` asks it before a play is made. Both are in
  `module/ship/g_ship.cpp`.
- **Neutrality.** The veto's own words are produced from a broken fixture inside `EndingToneFindings`
  and scanned by the same assertion as the destination table and the readings. The check's third
  falsification plants `better` in the log line and the assertion fails.

## The command

```sh
scripts/ending-check.sh
```

It compiles the ship tests on the host (no engine, no game data), runs the suite, prints
`test_ship_core --endings`, then falsifies five ways. The excerpt below is this session's run.

```text
== the crew's veto
  CONFIDENCE: a fresh crew 77%; a crew at breaking point 13% (the veto is below 35%)
  VETO: requested Earth -> chose The Klingon Empire; who holds the conn: Kathryn Janeway
  VETO LOG: [Kathryn Janeway] the crew would not make for Earth; the course is laid for The Klingon Empire (the crew's confidence is 13%, and 141 of 141 are at breaking point)
  VETO ONCE: 1
  VETO NEUTRAL: 0
```

The falsification, from the same run:

```text
==> endings: falsification three (the veto's log line planted with 'better'; its neutrality must fail)
    the planted veto fails the same assertion; the tree passes
```

## PASS lines, and what each is evidence of

- **PASS — a fresh crew does not veto.** `CONFIDENCE: a fresh crew 77%` is above the 35% line, and the
  test's `!CrewWouldVeto(s)` and `ChooseEnding(s, DEST_EARTH).vetoed == false` assert it.
- **PASS — a crew at breaking point vetoes, and chooses a place the run accrued.** With deficit 1 and
  outlook 0 on every record, confidence reads 13%; the retrofit is a funeral, so the Empire is open,
  and `VETO: requested Earth -> chose The Klingon Empire` is `TestEndingVeto`'s assertion that the
  chosen place is in `DestinationsFor`.
- **PASS — the log records who holds the conn.** `VETO LOG: [Kathryn Janeway] …` is the official log's
  entry after `RecordEndingChoice`: its `who` is the conn-holder, and the test asserts
  `s.log.back().who == veto.conn`.
- **PASS — it fires once.** `VETO ONCE: 1` is `ChooseEnding(broken, DEST_EARTH, true)`, which returns
  the requested destination and `vetoed == false`; the test fails if it vetoes again.
- **PASS — the veto's own words are neutral.** `VETO NEUTRAL: 0` is `EndingToneFindings({line, reason})`.
  The falsification plants `better` in the same line and the same assertion reports it, so the guard
  can fail for the reason it cares about.

## Decisions taken where the document leaves a choice

- **The threshold is 35%**, recorded in the header (`CREW_CONFIDENCE_VETO`) so it can be overruled. A
  fresh crew reads 77%; a crew with nothing left reads 13%.
- **The crew's choice order** is settlement → Empire → Earth. It is a read of the same record every
  destination reads, not a second store and not a preference score.
- **The conn-holder is the senior fit officer other than the player**, chosen from the crew record. The
  log's `who` field carries them; the fact says the course changed.
- **The veto is asked in the console path and in the wildcard path.** A crew that has lost confidence
  will not follow the captain's gamble; the play is probed first, so nothing is spent when the veto
  stops it.

## What could not be run here, and is not claimed

- **Not watched in a graphical session.** The veto is a model read and a log write; the endgame
  screen that follows is built and compiled (`docs/evidence/the-endgame-screen.md`), but its rendered
  frame is not claimed here.
- **No save change.** The confidence is a fold over saved state, and the once-latch is host state for
  the one-shot endgame. `SAVE_VERSION` remains **66**.
