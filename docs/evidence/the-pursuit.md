# Evidence — a pursuit puts an artifact into the world

**Session 2026-11-08.** Stage D. `docs/affinities-and-allegiance.md` makes `pursuits[]` the part of a
person that matters most, because *"pursuits matter more than likes, because they generate artifacts
and events, which is the fuel the whole memory system runs on."* The record was already generated;
nothing in the world ever came of it. This slice builds the acceptance item: **a pursuit produces an
artifact or event that the log can point at.**

Save format remains **66**: the event is a `Memory` whose `event` is a `uint16`, and the artifact is
named deterministically from the record, so no new saved field moves the version.

---

## What was built

- `PursuitArtifact(c)` names what the person makes, deterministically from their record (name, type,
  strongest pursuit and the roster seed). A poem by one record is always the same poem; the title is
  not regenerated differently on load.
- `WorkOnPursuit(s, crew)` is the act: it checks the person is fit and has a pursuit they have not
  yet completed, writes the artifact into the official log under their own name, and takes a
  `MEM_PURSUIT` mark. It returns the log line, or `""`. **One piece per person per run**, so a reload
  cannot replay it.
- `HasPursued(c)` is the read: the `MEM_PURSUIT` mark, which is what makes the bound survive a save
  without a stored cursor.
- **The schedule calls it.** `UpdateCrew` gives downtime one hand's turn per ship hour, chosen by the
  clock. The hour is read back from the marks' own `time`, so a reload in the same hour makes no
  second piece and no cursor is stored. A zero-time tick (a load) writes nothing.

The artifact is a **log event** rather than a new stored object: it is the form that costs the save
nothing and is already the store the record reads. The log line names the maker because the
simulation resolved them; no authored artifact names a crew member (the two laws in
`docs/scenario-atlas.md`).

## The command

```sh
scripts/stage-d-check.sh pursuits
```

It compiles the ship suite on the host (no engine, no game data), runs the whole suite, prints
`test_ship_core --pursuits`, then falsifies once: a `WorkOnPursuit` that makes nothing must make the
same transcript fail.

```text
==> stage D: the ship suite, built here

==> a pursuit puts an artifact in the log (test_ship_core --pursuits)
== a pursuit, put into the world
  the maker: Chakotay; the pursuit: a poem (+0.90)
  the artifact: a poem, "A Message Home"
  the log line: Chakotay finishes a poem, "A Message Home"
  the mark: MEM_PURSUIT held 1; a second making produces ""
  the schedule made 4 piece(s) in a recreation hour
all demonstrations passed

==> falsification: a pursuit that makes nothing must fail the transcript
    the broken copy fails the transcript; the tree passes

PASS  a pursuit puts a deterministic artifact into the log, marks it, and cannot make it
      twice; a planted pursuit that makes nothing fails the same transcript
```

The rules are also in the suite (`TestPursuitArtifact`), which is what `scripts/test.sh` runs.

## PASS lines, and what each is evidence of

- **PASS — the artifact is deterministic and named.** `a poem, "A Message Home"` is a function of the
  record; the test checks the second call returns the same string.
- **PASS — it reaches the log.** The log line is `Chakotay finishes a poem, "A Message Home"`, and
  it is the last official entry. The log can point at it.
- **PASS — the mark is held, and cannot be made twice.** `MEM_PURSUIT held 1`; a second call returns
  `""` and adds no entry. This is the bound that survives a save without a new field.
- **PASS — the schedule produces one.** A recreation hour makes pieces (`4` in the printed run),
  without a console call. The hour gate is the clock read back from the marks, so the count is
  bounded rather than one per simulation step.
- **PASS — the guard has teeth.** A `WorkOnPursuit` planted to return nothing fails the transcript;
  the tree passes.

## What is not verified, and is said plainly

- **The artifact is a record, not a prop.** There is no generated object on a table and no new mesh;
  the acceptance says *an artifact **or** event*, and this is the event half. A visible artifact in
  the world would be a location/prop change and is not claimed here.
- **The maker working on it is not watched.** `ProgramsOnDeck`/`ProgramManifest` exist for the
  holodeck program; there is no similar in-world view of someone writing or cooking, and the owner's
  eye on that is outstanding.
- **The title pool is ours `[inv]`.** The design does not give titles; each kind has four small,
  non-canonical names so the artifact can be pointed at. They are not attributed to a named
  character's canon work.
