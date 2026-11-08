# Evidence — the liked program, as the model can carry it

**Session 2026-11-08.** Stage D. `docs/affinities-and-allegiance.md` says a liked program must
**appear in a crew member's downtime and be observed on the deck**. The model half existed:
`PreferredProgram` was already what a recreation hour picks. What this slice adds is the **carrying**
half — the data a surface on the deck can draw, a console read, and the program named in the log —
and it states plainly which observation still needs the owner's eyes.

Save format remains **66**: `program` is derived every tick and never stored, exactly as before.

---

## What was built

- `ProgramsOnDeck(s, deck)` returns each fit person on that deck with the program the schedule chose;
  `ProgramManifest(s, deck)` is the one-line form.
- `ship crew <deck>` now prints the program beside a person in recreation (`"Captain Proton"`), so a
  developer can read the deck's actual content at the console.
- The world cvar `lwh_ship_holodeck` publishes the same manifest, so an in-world panel on deck 6 can
  draw it without a second query.
- `RunHolodeck(..., HOLO_RECREATION, crew)` now **names the program in the log**:
  `Kathryn Janeway takes recreation on the holodeck: "Captain Proton"`. That is the event a session
  can point at even when the surface is not being looked at.

## The command

```sh
scripts/stage-d-check.sh program
```

It compiles the ship suite on the host (no engine, no game data), runs the whole suite, prints
`test_ship_core --program`, then falsifies once: a schedule that stops choosing the liked program
must make the same transcript fail.

```text
==> stage D: the ship suite, built here

==> the liked program, as the model can carry it (test_ship_core --program)
== the liked program, at the schedule
  CREW: Kathryn Janeway -- recreation on deck 6, program "Captain Proton"
  ON DECK 6: 20 program(s) -- Kathryn Janeway: Captain Proton; Tom Paris: Captain Proton; B'Elanna Torres: the orbital skydiving program; The Doctor: Photons Be Free; Vorik: a da Vinci workshop; Rick Biessman: a jazz club; Austin Chang: a da Vinci workshop; Chell: the Gothic holonovel; Juliet Jurot: a Klingon opera program; ...
  RUN: 1; the log says: Kathryn Janeway takes recreation on the holodeck: "Captain Proton"
  OUTSTANDING: whether the program reads as a program *on the deck* is the owner's eye in a session
all demonstrations passed

==> falsification: a schedule that stops choosing the program must fail the transcript
    the broken copy fails the transcript; the tree passes

PASS  the schedule chooses the liked program, the deck read carries it, and the recreation
      call names it; a planted schedule that stops choosing fails the same transcript; the
      in-world observation of the program on the deck remains the owner's session
```

The rules are also in the suite (`TestLikedProgramModel`), which is what `scripts/test.sh` runs.

## PASS lines, and what each is evidence of

- **PASS — the schedule chooses the liked program.** A record given a liked program (`Captain
  Proton`, `+0.90`) is in `ACT_RECREATION` on deck 6 at 17:30, and `program` is that program. This is
  the model's choice, not a claim about the screen.
- **PASS — the deck read carries it.** `ProgramsOnDeck(deck 6)` returns the person with the program,
  and `ProgramManifest` names both; `ProgramsOnDeck` on a deck with no program is empty.
- **PASS — the log can point at it.** `RunHolodeck` writes the program into the official log, so a
  sighting in a session has a record behind it.
- **PASS — the guard has teeth.** A schedule planted to clear the program fails the transcript; the
  tree passes.

## The observation that still needs the owner

**Stand on deck 6 during an alpha-watch recreation hour and see whether the program reads as a
program on the deck** — whether the room, the arch and the bodies there make `"Captain Proton"` feel
like something running rather than a string in a console. The model now carries the data, the console
reads it, the cvar publishes it, and the log names it; **what cannot be proved off-device is that the
deck renders as the program it says it is.** The in-engine run was not performed here (static host,
no engine, no game data).

Related, and not claimed: the crew bodies on the holodeck are embodied by the existing S5 world
layer, but there is no authored holodeck *set* per program — that is location work under Stage C.
