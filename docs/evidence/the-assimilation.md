# Evidence — assimilation takes the log and the access

Date: 2026-10-11. Branch: `feat/the-assimilation`, cut from `testing`. Implements the remaining
acceptance of `docs/the-record-and-the-log.md`, *"What assimilation takes: memory and access"*, and
closes the boundary the two-log pass named: *"NPC private entries (and the Collective reading them on
assimilation) are the document's other acceptance items and are not built here"*
(`docs/evidence/the-two-logs.md`, judgement call 4). Save format **version 58**.

Nothing here is asserted without the command that produced it.

---

## What is built

**The transfer is a rule, not a copy.** Which minds the Collective holds is the one fact the whole
thing rests on, and it is `HeldByCollective`: a person wholly assimilated, or a person the Borg tick
has begun taking (`ship.collective`, a bounded saved list the tick writes at the take site, past the
dwell threshold — no dwell, no write). The private entries, the marks, the stations their authority
opens, and the voice are then **read from those crew records**, exactly as `PersonalVisibleTo` and
`Holdings()` are reads (the owner's C2 ruling, *"Derived"*). Nothing of the mind is duplicated into a
store that could drift from the person it came from, and a purge of the published log cannot touch it
because the personal log is not the published log.

- `TakeMind` — the Borg tick's take site, and a scenario's direct door. `HeldByCollective` is the read.
  The two edges that **end** a transfer are `RecoverCaptive` (a de-assimilation in the window) and
  `KillCrew` (the record closed).
- `CollectiveLog` — the private entries of everyone held, whether or not the published log was purged.
- `CollectiveMarks` — the marks that came with the minds.
- `CollectiveOperates` / `CollectiveRefusal` — the union of what the taken could operate, and the named
  limit on a station nobody taken could work.
- `CollectiveVoice` — the cast reference the taken person already had. `CastVoice` still refuses a taken
  person ("" — no voice, no line); the Collective does not, because it stole the voice with the mind,
  and `LineVoice` routes a meeting line spoken by a held person through it.

**NPC private entries.** The simulation now writes a private entry for a witness to a signed report that
contradicts what they saw (`SignReport`): "I saw …, and the report says otherwise", in that person's own
store. That is how an NPC comes to have a private log without content authoring one — the truth that
cannot go in the report — and it is exactly the entry the Collective reads if they are later taken.

---

## Observed — the unit transcript

```
$ cmake -S tests/ship -B /tmp/opencode/lwh-assim/test_ship -DCMAKE_BUILD_TYPE=Debug
$ cmake --build /tmp/opencode/lwh-assim/test_ship -j"$(nproc)"
$ /tmp/opencode/lwh-assim/test_ship/test_ship_core --assimilation
PASS  before the transfer the Collective holds 0 private entry(s)
PASS  after the transfer it holds 2, from Crewman 041: "I do not trust the captain's numbers" and "the captain was wrong about the nebula"
PASS  the authority it took: sickbay yes, tactical no
PASS  a security officer taken adds tactical: yes; engineering stays no
PASS  the voice the Collective speaks in is the taken person's own: "tuvok" (the ordinary cast gives "")
PASS  a purge does not end it: the Collective still holds 3 private entry(s)
PASS  a de-assimilation ends it: the mind is the person's again (yes)
all demonstrations passed
```

`scripts/assimilation-check.sh` runs the same transcript and fails if any line moves.

The rule, in the suite (`TestTheAssimilation`), also shows: the union is over **the taken** (a medical
ensign gives medical, a security officer adds tactical, and Engineering stays closed because nobody
taken could work it); a **death** ends the hold (`KillCrew`), as a de-assimilation does; the **Borg
tick itself** makes a mind held with no direct call; and the held list **survives a save** byte-for-byte.
The whole suite is green:

```
$ /tmp/opencode/lwh-assim/test_ship/test_ship_core ; echo $?
ship_core: all checks passed
0
```

---

## Observed — the in-game surface

Case 100 of the `g_shipTest` harness drives the console verb end to end: a mind is taken, `ship
collective` reads the private words, the authority and the voice, and then `ship report purge` runs —
and the entries stand. It is **not run in this pass**: this is a static host (no engine, no game data).
The command, for a host with the data:

```
$ scripts/run-lwh.sh +set s_useOpenAL 0 +set g_ship +set g_shipTest 100 +map tour/deck04
```

The console reads, as built (`module/ship/g_ship.cpp`):

```
SHIP:   <name>: <n> private entry(s), speaks as "<voice>", authority over <k> of 5 station(s)
SHIP:     private: <the taken person's own words>
SHIP: the Collective holds <n> mind(s)
SHIP:   it still cannot operate <STATION>: the Collective does not operate <STATION>: nobody it holds
        was cleared for it (it operates only what was taken)
```

---

## The seam with `docs/access-and-authority.md`, stated

What that document says about a credential, and what this feature does at the seam:

- **Access is technical and rank-derived; a credential is earned.** "Warp core ejection requires the
  chief engineer's or senior staff's authorization code — not a rank in the abstract, a *credential*
  held by particular people" (docs/access-and-authority.md). The document never says the authority is
  bound to whether its holder is *alive* in a way that would stop a non-person reading it; it says the
  **crew's console** is what refuses.
- **Its edge case, "a dead officer's credentials are retained; using them is reported, not built"**
  (corrected 2026-10-09, and withdrawn as a promise in `docs/omissions.md` **W1**): the computer does
  not know the holder is dead, so the *record* keeps the credentials, but the crew's console offering a
  silent use of them is a deliberate act the code does not implement.

**How this feature obeys it, and where the fork is.** `docs/the-record-and-the-log.md` uses exactly the
dead-officer logic one level up: *"The computer does not know they have been assimilated, so the ship is
lost through its own authority structure rather than through a pressure timer."*

- The crew's console path is unchanged and obeys the document: `MayOperate` requires a **fit** holder,
  so a dead or assimilated record grants the crew nothing through it, and there is still no console act
  that assumes a dead officer's authority (W1 stays reported-not-built).
- The Collective's authority is a **separate read** (`CollectiveOperates`, over `RecordOperates` — the
  rank/department/credential half without the status check). The Collective is not a person at a console;
  it holds a mind, and the authority came with it.

**The fork, given rather than chosen silently:** whether the ship should *also* refuse the Collective —
i.e., whether "the computer does not know" should be true of the Borg too, or whether the design's horror
(*the ship is lost through its own authority structure*) is the point. **The design chooses the horror**:
the transfer is not gated. So the two rules do not contradict; they are two locks, and the transfer is the
case that tests which lock the Borg turn. **What is *not* built, and is named:** the crew's own deliberate
use of a dead officer's credentials remains W1's reported-not-built item, and this pass does not touch it.

---

## The counter-case

- **A purge does not end it.** The log is taken from the mind, not the disk (docs/the-record-and-the-log.md).
  `PurgeLogs` empties the published log and leaves the personal store; the transcript shows the entries
  standing after a purge. Demonstrated.
- **A de-assimilation ends it.** `RecoverCaptive`, within the design's narrow window, releases the mind;
  what the Collective held of that person is gone. Demonstrated (`--assimilation`), and in the suite.
- **A death ends it.** `KillCrew` closes the record; `HeldByCollective` is false and the Collective no
  longer holds a mind to speak with. Demonstrated in the suite.

---

## Save-format effect

**Version 58.** The only new saved state is the bounded list of crew indices the drones have begun
taking (`Ship::collective`), so a save taken mid-window replays and a de-assimilation ends the transfer
across a load. The cost, named: an older save does not load, as every bump has. `tools/docs/owned_claims.json`
moves the registered `save-version` value 57 → 58.

---

## Judgement calls, named

1. **The transfer begins when the drones begin taking a mind, not only at full assimilation.** The
   design says *"assimilation adds a mind to the Collective"*; the owner's ruling makes the recovery
   window the counter-play (*"Reversal gets harder the longer it has been"*), and a counter-case that
   cannot end a *completed* transfer would be decoration. The tick records the take the moment it
   writes the first wound, past the dwell threshold, so a de-assimilation in the window has something
   to end. A wholly assimilated person is held by their status alone. [our call]
2. **The held state is stored; everything else is derived.** The list of held minds must survive a
   save, so it is a field. Nothing of the mind is stored — no copied log, marks or access — so no
   number can disagree with the record it came from (the C2 ruling's shape).
3. **An NPC's private entry is written by the toll, not by content.** `SignReport` already writes the
   witness's line to the official log; the same truth now goes privately into their own store. This is
   the smallest non-inventing way to close *"NPC private entries … not built"* without authoring prose.
4. **`TakeMind` is public.** It is the tick's take site and a scenario's direct door, in the shape of
   `KillCrew` (the scenario's door to a death) — not an internal smuggled out for a test.

---

## What could not be verified, and is said plainly

- **Whether it *feels* like the person the Collective took** — the horror the design is after — is the
  owner's judgement, not this document's. No transcript can show that a quoted private line lands.
- **The in-game console (`g_shipTest 100`) was not run**: static host, no engine, no game data.
- **`scripts/check.sh` (G0) was not run**: it needs the GDK and the bootstrapped upstream tools, which
  this host does not carry. `scripts/test.sh`, `scripts/docs-check.sh`, `scripts/hooks-check.sh`,
  `scripts/api-check.sh` and `scripts/harness-check.sh` are green here.
- **The voice is the source, not sound.** No audio is produced, generated or committed; `CollectiveVoice`
  names the reference the renderer would use, and the ear remains the owner's, as in M5.
