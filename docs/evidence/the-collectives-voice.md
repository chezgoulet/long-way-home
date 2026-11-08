# Evidence — the Collective's voice

**Session 2026-11-08.** Stage D, the last named slice of the crew in the world: an assimilated person
speaks in the Collective's voice, using their own taken words. Implements the remaining acceptance of
`docs/the-record-and-the-log.md`, *"The Collective's voice"*, and closes the gap
`docs/designs-against-code.md` row 19 named: *"no code builds a line from `CollectiveLog`."*

Nothing here is asserted without the command that produced it. Save format remains **66**: the
private words, the held-mind list and the voice are all reads of records the design already saves.

---

## What was built

The transfer was already a rule (`docs/evidence/the-assimilation.md`): `HeldByCollective`,
`CollectiveLog`, `CollectiveMarks`, `CollectiveOperates`, `CollectiveVoice`, and `LineVoice` routing a
held speaker through the stolen cast reference. What was missing was the **line itself**.

- `CollectiveSpeaks(s, crew)` returns the newest private entry that person wrote, read from
  `CollectiveLog` — the mind, not the published log, so a purge cannot empty it — plus the voice the
  transfer stole (`CollectiveVoice`). It is empty when the person is not held or left no words.
- `CollectiveLine(s, crew, MeetingLine&)` turns that into the meeting language: the held person is
  the speaker, their own words are the text, and the delivery is `DELIVERY_FLAT` — *the log read
  aloud*, in `DeliveryName`'s own words. That delivery is what travels to synthesis through
  `LineToSynthesis`, so the render key is built under the taken voice and no audio is produced here.
- The design's counter-case is unchanged: a de-assimilation in the narrow window releases the mind,
  and the Collective can no longer speak the words. A death ends it the same way (`KillCrew`).

## The command

```sh
scripts/stage-d-check.sh collective
```

It compiles the ship suite on the host (no engine, no game data), runs the whole suite, prints
`test_ship_core --collective`, and then falsifies once: a `CollectiveSpeaks` that holds no words must
make the same transcript fail. The output below is the run of this session.

```text
==> stage D: the ship suite, built here

==> the Collective's voice (test_ship_core --collective)
PASS  the mind is held: 1; it has private words: 1
PASS  the words are the taken person's own: "the captain was wrong about the nebula"
PASS  the voice is the stolen cast reference: "tuvok" (the ordinary cast gives "")
PASS  it speaks as a line: speaker 40, delivery flat
PASS  the render seam takes it under the stolen voice: key present
PASS  a de-assimilation ends it: mind the person's again: 1; the Collective can speak: 0
all demonstrations passed

==> falsification: a Collective that cannot speak its taken words must fail the transcript
    the broken copy fails the transcript; the tree passes

PASS  the Collective speaks a taken person's own words in the stolen voice, and a planted
      speaker that cannot speak fails the same transcript
```

The rule is also in the suite (`TestTheCollectiveVoice`), which is what `scripts/test.sh` runs.

## PASS lines, and what each is evidence of

- **PASS — the mind is held, and it has private words.** `CollectiveSpeaks` returns
  `held = 1, hasWords = 1` only after `TakeMind`; before the transfer it returns neither. The words
  are the newest entry the person wrote (`"the captain was wrong about the nebula"`), not a
  re-authored line.
- **PASS — the voice is the stolen reference.** The output prints `"tuvok"` and the ordinary cast's
  `""`. This is `CollectiveVoice` reaching `CastVoiceOfRecord` for a wholly assimilated record, while
  `CastVoice` refuses the same person.
- **PASS — it is a line, not a quotation in a comment.** `CollectiveLine` fills `speaker`, `text` and
  `DELIVERY_FLAT`; `MeetingLineKey` then produces a render key under that voice with no audio
  written. The repository still carries the mechanism and no generated voice.
- **PASS — a de-assimilation ends it.** `RecoverCaptive` in the window releases the mind;
  `CollectiveLine` returns false and the words are the person's again.
- **PASS — the guard has teeth.** The falsified build holds no words and the transcript fails; the
  tree passes. A guard that cannot fail for the reason we care about is decoration.

## What is not verified, and is said plainly

- **The ear is the owner's.** No audio is generated, committed or attached; the check observes the
  reference the renderer would use, not the sound. The mechanism is the analyser/synthesiser path
  `docs/asset-doctrine.md` and `docs/staff-meetings.md` require; every line it produces is
  player-local and pruned with the save.
- **The in-engine scene was not run** — this is a static host, no engine and no game data. The
  console already reads the held mind and the voice (`ship collective`), and the meeting/order render
  seam is `MeetingLineKey`; the scene in which the Collective speaks is the owner's session.
- **Judgement call [our call]:** the line is the person's **newest** private entry, quoted verbatim,
  rather than a paraphrase. The design says *"using their own words"*; paraphrasing would make the
  model, or this code, an author of the horror rather than a reader of it.
