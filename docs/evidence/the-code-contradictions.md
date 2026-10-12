# Evidence — the contradictions whose fault is the code, repaired and falsified

Date: **2026-10-12**. Branch `feat/the-code-contradictions`, cut from `testing` (which carries the
doc-side pass, `docs/evidence/the-doc-contradictions.md`, and the register `docs/designs-against-code.md`).
**Static host: no engine, no game data.** Everything below is the unit tests
(`tests/ship/test_ship_core.cpp`, built by `scripts/test.sh`), read off a terminal; the rendered game
is not in reach here.

The rule is the brief's, not this pass's taste. The register measured the corpus against the code and
fixed nothing; the doc-side pass corrected the side the register named as the **document's** and handed
**twelve rows** here whose closure is a code change or a ruling. This pass takes those twelve, plus the
design fork the register's own **C-4** row hands to the code list. **One of the twelve, 3.20, was closed
on the base before this pass rebased onto it** — the owner's instruction corrected `docs/crew-roster.md`
and closed the row (PR #133, merged 2026-10-12) — so it is recorded here as resolved, not re-dispositioned.
Where the code is wrong it is fixed; where the code is *right* and the design intent differs, the fork is
stated and the code is left alone; where the register's verdict looks wrong, that is said.

Every figure is the output of a command named beside it, on this tree.

---

## Part 1 — the list, with a disposition for every row

The register names the code as the wrong side for **three** rows — **C-13**, **C-14**, **3.2** — and
hands **nine forks** over (C-11, C-12, C-15, C-23, 3.3, 3.20, 3.23, 8.52, 8.53). C-4's design fork is a
tenth handed-over item, though the doc pass counts C-4 among its corrections. **3.20 was closed on the
base** before this pass rebased (PR #133, the owner's instruction), so twelve rows carry a disposition
here:

| row | register says | disposition | what closed it / why not |
|---|---|---|---|
| **C-11** | fork: the OOD predicate is wired to nothing | **fixed** | `MayOrder` gains the consumer; `OodMayOrder` now reaches a decision. The register's own code option, taken. |
| **C-12** | fork: the repair party is stationless | **handed back** | The stationless party is the commented design; charging the station is gameplay, not a defect. Fork stated. |
| **C-13** | code: `MEM_RUMOUR` declared, no writer | **fixed** | `WriteOff` writes the mark: aboard `MEM_SAW`, the rest `MEM_RUMOUR`. The distortion half is left and named. |
| **C-14** | code (incomplete): one shield scalar | **disputed** | A scalar where the design implies a set; building the shield model is the feature the brief forbids. Fork stated. |
| **C-15** | fork: twelve-shot counter vs ramp | **handed back** | A rate where the document implies a threshold. Fork stated. |
| **C-23** | fork: the roll uses `WorkFactors`, not `EffectiveSkill` | **handed back** | Both encode one design; which name moves is the owner's. Fork stated. |
| **3.2** | code: the Doctor is given rank 3 | **fixed** | The Doctor carries rank 0 — the no-commission rank the record already uses for Neelix and Seven. |
| **3.3** | fork: nine departments vs the code's five | **handed back** | The nine are the document's own "proposal". Adopting them is content, not repair. Fork stated. |
| **3.20** | contradicted: 25 bytes vs `SAVE_MEMBER_BYTES=20` | **resolved on the base** | The owner's instruction corrected `docs/crew-roster.md` to three labelled figures and closed the row (PR #133); not re-dispositioned here. |
| **3.23** | fork: four job kinds vs the code's six | **handed back** | The vocabulary is the owner's; adding or removing kinds changes the queue's contract. Fork stated. |
| **8.52** | fork: 64 played characters vs SP 1 | **handed back** | Both are shipped engine data; which ceiling the premise rests on is Track D's ruling. |
| **8.53** | fork: the complement is hull data vs `COMPLEMENT` | **handed back** | One hull is built; multi-hull complement is a feature. Fork stated. |
| **C-4** | design fork handed to the code list | **handed back** | The doc half is closed; moving `% 15` to `% 10` or keeping the roll is not decidable from the evidence. |

**The counts.**

- **Fixed: 3** — **C-11**, **C-13**, **3.2**. Each has its own falsification (Part 2) and a closed
  register marker (Part 6).
- **Handed back: 8** — **C-4**, **C-12**, **C-15**, **C-23**, **3.3**, **3.23**, **8.52**, **8.53**.
- **Disputed: 1** — **C-14**. Out of the **three** the register pins on the code (C-13, C-14, 3.2):
  **two fixed, one disputed**; the other forks are handed back.
- **Resolved on the base, not by this pass: 1** — **3.20** (PR #133).
- **A row the register missed: 1** — `docs/memory-and-consequence.md:74` (the write-off mark), Part 4.

---

## Part 2 — the three fixes, each with its falsification

The falsification is the same shape for all three: **point the new check at the unfixed code, watch it
fail, restore.** Each transcript below is `cmake --build` of `tests/ship` then `test_ship_core`, with
one item reverted.

### 2a. C-11 — the OOD's predicate gains its consumer

**Change.** In `module/ship/ship_core.cpp`, `MayOrder` (`:9940`) now consults the predicate when the
giver is the deck:

```cpp
// The officer of the deck commands the deck's operation for the watch, and that reaches officers
// senior to them who are not the captain (docs/crew-manifest.md, "Who reports to whom";
// docs/orders.md). The predicate was built and unit-tested but no decision consumed it (register
// C-11); this is the decision that does, so the deck's documented authority can dispatch an order.
if (giver == OfficerOfTheDeck(s) && OodMayOrder(s, giver, recipient)) return true;
```

`DeriveOrders`, `OrderPermitted`, `OrderRefusalRule` and `DispatchOrder` all read `MayOrder`, so the
whole order path obeys the deck. The register wrote C-11 as a fork (`DispatchOrder` should consult the
OOD rule, *or* the document changes); this takes the code option the register itself named.

**The new check** — `TestOodDispatch`, `tests/ship/test_ship_core.cpp` (the predicate test
`TestOodAuthority` is kept, unchanged):

```cpp
static void TestOodDispatch()
{
	g_test = "the officer of the deck may dispatch an order to a senior";
	Ship s = NewShip();
	Tick(s, 1.0f);
	const int ood = OfficerOfTheDeck(s);
	CHECK(ood >= 0);
	int ordered = -1;
	for (int i = 0; i < static_cast<int>(s.crew.size()); ++i) {
		const CrewMember &c = s.crew[i];
		if (c.status != CREW_FIT || c.brigged || c.away) continue;
		if (c.rank > s.crew[ood].rank && c.rank < 6 && c.post < SYS_COUNT) { ordered = i; break; }
	}
	CHECK(ordered >= 0);
	CHECK(s.crew[ordered].rank > s.crew[ood].rank);   // seniority alone would refuse the deck
	CHECK(!MayCommand(s.crew[ordered]));
	const OrderBrief b = BuildOrderBrief(s, ood, ordered);
	const int idx = OptionOf(b, ORDER_SEE_TO);
	CHECK(idx >= 0);                                  // offered because the deck may give it
	const OrderOutcome r = DispatchOrder(s, b, idx, ood);
	CHECK(r.given && !r.refused);
	CHECK(s.orderRepairFirst == s.crew[ordered].post);
}
```

**Falsification** (the `if (giver == OfficerOfTheDeck(s) …)` branch removed; `test_ship_core`):

```
FAIL  the officer of the deck may dispatch an order to a senior: idx >= 0 (line 8618)
FAIL  the officer of the deck may dispatch an order to a senior: r.given && !r.refused (line 8620)
FAIL  the officer of the deck may dispatch an order to a senior: s.orderRepairFirst == s.crew[ordered].post (line 8621)
3 check(s) failed
```

Restored, the same build prints `ship_core: all checks passed`.

### 2b. C-13 — `MEM_RUMOUR` gains its writer

**Change.** `module/ship/ship_core.h` names the event a write-off leaves; `module/ship/ship_core.cpp`,
`WriteOff`, marks the crew:

```cpp
// The mark a write-off leaves (docs/memory-and-consequence.md): "a deck written off is a mark in
// everyone who was aboard, and a rumour in everyone who was not." Those who were on the deck saw
// it (MEM_SAW); everyone else learns it second-hand (MEM_RUMOUR), and the two versions differ --
// which is what makes the same afternoon answer differently depending on where a person stood.
// This is also what gives MEM_RUMOUR a writer: it was declared and nothing produced it (C-13).
for (int i = 0; i < static_cast<int>(s.crew.size()); ++i) {
	if (s.crew[i].status == CREW_DEAD || s.crew[i].status == CREW_ASSIMILATED) continue;
	const bool aboard = !system && s.crew[i].deck == target;
	Remember(s, i, MEM_LOSS, -1, aboard ? MEM_SAW : MEM_RUMOUR, aboard ? -0.6f : -0.3f);
}
```

The enum gains `MEM_LOSS = 10` (appended; `Memory.event` is already `uint16_t`, so the save layout does
not move). **What is left, and named:** the distortion/attribution model — a rumour's *teller*, and how
the telling drifts — is not built. The mark now exists and carries its provenance; the register's rows
4.22 (sources) and 4.23 (distortion) stay `partial`/`absent`.

**The new check** — added to `TestWrittenOffList`:

```cpp
{
	Ship w = NewShip();
	for (CrewMember &c : w.crew) c.deck = 0;
	w.crew[0].deck = 9;
	w.crew[1].deck = 9;
	CHECK(WriteOff(w, false, 9, LOSS_SEALED));
	CHECK(RecallSource(w.crew[0], MEM_LOSS) == MEM_SAW);
	CHECK(RecallSource(w.crew[1], MEM_LOSS) == MEM_SAW);
	int saw = 0, rumoured = 0, unknown = 0;
	for (const CrewMember &c : w.crew) {
		const int src = RecallSource(c, MEM_LOSS);
		if (src == MEM_SAW) ++saw;
		else if (src == MEM_RUMOUR) ++rumoured;
		else ++unknown;
	}
	CHECK(saw == 2);
	CHECK(rumoured == COMPLEMENT - 2);
	CHECK(unknown == 0);
}
```

**Falsification** (the writer loop removed; `test_ship_core`):

```
FAIL  the ship carries a written list of what she has given up: RecallSource(w.crew[0], MEM_LOSS) == MEM_SAW (line 1094)
FAIL  the ship carries a written list of what she has given up: RecallSource(w.crew[1], MEM_LOSS) == MEM_SAW (line 1095)
FAIL  the ship carries a written list of what she has given up: saw == 2 (line 1103)
FAIL  the ship carries a written list of what she has given up: rumoured == COMPLEMENT - 2 (line 1104)
FAIL  the ship carries a written list of what she has given up: unknown == 0 (line 1105)
```

Restored: `ship_core: all checks passed`.

### 2c. 3.2 — the Doctor is a post with no rank

**Change.** `module/ship/ship_core.cpp`, the `NAMED` table: the Doctor's rank is `0`, not `3`:

```cpp
{"The Doctor", "doctor", 0, DEPT_MEDICAL, SYS_SICKBAY, SPECIES_HOLOGRAM}, // a post with no rank: CMO, no commission [lore]
```

`0` is the record's existing rank for a person with a post and no commission (Neelix the civilian, Seven
the consultant are both `0`). Rank now says *no commission*; the post's department says *what he may
operate* — `MayOperate(doctor, STN_SICKBAY)` stays true and `MayOperate(doctor, STN_CONN)` stays false,
which is the document's own line: his authority *"does not extend to the conn."* **What is left, and
named:** the *situational* authority the document also gives him (*"sickbay, medical decisions,
quarantine"*) is a per-post authority the record has no field for — register row 3.14, still `absent`.
Rank `0` also renders as the label "Crewman" (the same label Neelix and Seven carry); a rank-less label
is part of that same unbuilt field, not something this fix invents.

**The new check** — `TestDoctorNoCommission`:

```cpp
static void TestDoctorNoCommission()
{
	g_test = "the Doctor is a post with no rank";
	Ship s = NewShip();
	const CrewMember &doctor = s.crew[6];
	CHECK(doctor.name == "The Doctor");
	CHECK(doctor.rank == 0);
	CHECK(!MayCommand(doctor));
	CHECK(MayOperate(doctor, STN_SICKBAY) && !MayOperate(doctor, STN_CONN));
}
```

**Falsification** (rank restored to 3; `test_ship_core`):

```
FAIL  the Doctor is a post with no rank: doctor.rank == 0 (line 3671)
1 check(s) failed
```

Restored: `ship_core: all checks passed`.

---

## Part 3 — the forks, both sides quoted, for one reading

Each of these is an owner's ruling. Neither side is a defect; the code is left as it is.

**C-12 — does assigning a repair empty a station?**
- *Document* (`docs/ship-systems.md:139`; repeated `docs/crew-work.md:12`): *"crew time comes from the
  same people who hold posts — assigning a repair empties a station. That single link is what makes the
  crew model and the damage model one system instead of two."*
- *Code* (`module/ship/ship_core.cpp:1745-1746`): *"An engineer on duty with no station of their own
  joins the damage-control party"*; a post-holder is counted at their station first, and `ORDER_SEE_TO`
  only sets a queue priority.
- **Ruling wanted:** charge the station (the documents' link) or keep the stationless party (the code's
  commented design)?

**C-14 — one shield scalar, or six sections and fourteen grids?**
- *Document* (`docs/ship-systems.md:39,113`): *"shields | fourteen external grids; six sections
  (fwd/port/stbd/aft/dorsal/ventral)"*; *"six sections, fourteen grids, percentages that buckle, bypass
  conditions (phased polaron weapons ignore them entirely)."*
- *Code* (`module/ship/ship_core.h:1959`): `float shieldStrength = 1.0f; // what stands between enemy
  fire and the hull; 0 = hits land` — one number, read by the combat path.
- **Ruling wanted:** build the section/grid model (a feature), or accept the scalar as the design and
  correct the document. The register calls the code "incomplete"; I dispute that a missing set is a
  defect rather than an unbuilt feature.

**C-15 — twelve shots, or a continuous ramp?**
- *Document* (`docs/ship-systems.md:149-150`): *"Adaptation is a counter per weapon: canon's phaser
  adapter lets at most twelve shots penetrate before the Borg adapt."*
- *Code* (`module/ship/ship_core.cpp:3572`): `s.enemy.adaptation = std::min(1.0f, s.enemy.adaptation +
  ours * (2.0f + s.borgAwareness));` — a continuous accumulator, reduced by `Remodulate`/`RaidVinculum`,
  with no shot counter and no twelve-shot cap.
- **Ruling wanted:** build the counter, or keep the ramp and correct the document.

**C-23 — does the failure roll call `EffectiveSkill`?**
- *Document* (`docs/character-derivation.md:232-246`): *"This is `EffectiveSkill(c, skill)` … This is what
  decides whether a repair is quick, slow, or dangerous … The failure roll's odds are now wired."*
- *Code* (`module/ship/ship_core.cpp:5542-5548`): `WorkFactors` builds the skill term from **raw**
  `c.skills[skill]` plus the aptitude trait, adding conditions and deficit as separate factors;
  `EffectiveSkill` is called only from `WorkReading`, `OfferRising`/`RisingOdds` and the console.
- **Ruling wanted:** make the roll call `EffectiveSkill`, or keep the parallel construction and name it
  in the document.
- **Measured 2026-10-12** (`docs/evidence/the-failure-roll-measured.md`): a condition **does** reach the
  roll today, through `WorkFactors` — 4,724 of 20,000 anomalies clean against 7,347 carrying a sharp debuff
  (+0.1312 odds), with `COND_NONE` and a nominal system moving nothing. **Fork A** (the roll calls
  `EffectiveSkill`, condition subsumed) cuts the condition's weight in the odds multiplier from +0.50 to
  +0.0600 (**8.3×**) and moves **1 test** (`TestConditionMovesTheOdds`, 5 checks, verified on a throwaway
  worktree); **fork B** (the document names `WorkFactors`) moves **0 tests and 0 lines of code**, changing two
  sentences. The fork stays open for the owner.

**3.3 — nine departments, or the code's five?**
- *Document* (`docs/crew-manifest.md:52-71`): nine departments whose sizes add to 141 — Command 9,
  Operations 16, Engineering 36, Security 18, Science 15, Medical 12, Flight 10, Support 19,
  Civilians 6 — under its own words *"these numbers are ours — a proposal, to be tuned."*
- *Code* (`module/ship/ship_core.cpp:241`): `static const int DEPT_SIZE[DEPT_COUNT] = {20, 50, 25, 30, 16};
  // sums to COMPLEMENT [inv split]` — five departments.
- **Ruling wanted:** adopt the nine-way split (and carry it through roster, bills and deck map), or keep
  the five.

**3.20 — is 25 bytes per crew member wrong? — *resolved on the base, not a fork this pass states.***
- *Document, as first cut* (`docs/crew-roster.md:94`): *"G3's save cost is **25 bytes per crew member**
  against a budget of 256."*
- *Code* (`module/crew/crew_core.h:219-220`): `SAVE_HEADER_BYTES = 16; SAVE_MEMBER_BYTES = 20;` — with
  `docs/evidence/g3-reactive-crew-measured.md` measuring *"25 bytes per NPC of layer state (budget 256)".*
- **Closed before this pass rebased:** PR #133, on the owner's instruction, corrected `docs/crew-roster.md`
  to carry all three figures labelled (the blob's 20 + 16-byte header, the layer's 25, the save's ≈9 KB per
  member) and closed the row with a marker. There is nothing for this pass to rule or fix, and the row is
  left as the base has it.

**3.23 — four job kinds, or six?**
- *Document* (`docs/crew-work.md:20`): *"kind — maintenance, repair, build, reclaim."*
- *Code* (`module/ship/ship_core.h:1692`): `enum JobKind { JOB_REPAIR, JOB_SEAL, JOB_RECLAIM,
  JOB_BUILD, JOB_SECURE, JOB_EVACUATE, JOB_KIND_COUNT };` — six, two of them added by the orders pass.
- **Ruling wanted:** the job vocabulary (and whether *maintenance* exists).

**8.52 — is the played-character ceiling 64 or 1?**
- *Document* (`docs/design-north-star.md:175`): *"the ceiling is **64 played characters** — the engine's
  `MAX_CLIENTS`".*
- *Code* (`patches/0018`): the multiplayer header defines `MAX_CLIENTS 64`; the Retail single-player
  header defines `MAX_CLIENTS 1 // 128`.
- **Ruling wanted:** which ceiling the multiplayer premise rests on (Track D).

**8.53 — is the complement hull data or a constant?**
- *Document* (`docs/design-north-star.md:192`): *"The complement is hull data, not a constant."*
- *Code* (`module/ship/ship_core.h:23`): `const int COMPLEMENT = 141;`.
- **Ruling wanted:** multi-hull complement data (a feature), or one hard-coded Voyager.

**C-4 — 40/30/20/10, or the `% 15` roll?**
- *Document* (`docs/lore-ledger.md:242`, now corrected to the code): the design split is *"40% empty,
  30% hostile, 20% derelict, 10% Borg"*; the code rolls `% 15`.
- *Code* (`module/ship/ship_core.cpp:2686-2689`): `const uint32_t roll = next() % 15;` then `roll < 4 ?
  EMPTY : roll < 7 ? HOSTILE : roll < 9 ? DERELICT : roll < 10 ? BORG : …`, plus trader, distress,
  belt and pre-warp.
- **Ruling wanted:** move the divisor to `% 10`, or keep the roll.

---

## Part 4 — the row the register missed

`docs/memory-and-consequence.md:74` states a rule the register never extracted:

> *"a deck written off is a mark in everyone who was aboard, and a rumour in everyone who was not"*

The code wrote **no mark at all** on a write-off (`WriteOff`, `module/ship/ship_core.cpp`), so the
sentence was false on both halves, not only the rumour half the register's C-13 names. It is a
contradiction the register missed, and it is closed by the same fix as C-13 (the writer now marks
everyone, with `MEM_SAW` aboard and `MEM_RUMOUR` elsewhere). Found by following the `MEM_RUMOUR`
writer the register said did not exist to the document's own example of one.

---

## Part 5 — the save

**No fix moves the save format. `SAVE_VERSION` is 59 before and after.**

```
$ grep -n "SAVE_VERSION" module/ship/ship_core.h | head -1
2988:const uint16_t SAVE_VERSION = 59; ...
```

- **C-11** writes no state.
- **C-13** appends one `MemoryEvent` value (`MEM_LOSS = 10`); `Memory.event` was already a `uint16_t`,
  and the marks serialize through the existing `Memory` path, so no width or layout moved. A save made
  before the change loads unchanged.
- **3.2** changes a stored `uint8_t` from `3` to `0`; no format moves. A save made before it loads (and
  a save made after replays identically, which `TestSave` and the round-trip checks in
  `TestWrittenOffList` confirm).

The replay rule is not weakened: the new marks are written by the same deterministic path as every other
mark, and `scripts/test.sh` includes the byte-equal round-trips.

---

## Part 6 — the registers corrected, in place and visibly

`docs/designs-against-code.md` gained a **`Disposition 2026-10-12 (code pass)`** line/cell on each of
the thirteen rows, and a short **code-pass summary** before the counts. The three fixed rows carry the
register check's machine-readable marker, `state=closed`, naming the new proof:

```
docs/designs-against-code.md:162  C-11  <!-- register-row id=C-11 state=closed marker=symbol:tests/ship/test_ship_core.cpp:TestOodDispatch -->
docs/designs-against-code.md:186  C-13  <!-- register-row id=C-13 state=closed marker=symbol:module/ship/ship_core.cpp:MEM_RUMOUR -->
docs/designs-against-code.md:486  D3-2  <!-- register-row id=D3-2 state=closed marker=symbol:tests/ship/test_ship_core.cpp:TestDoctorNoCommission -->
```

The ten handed-back rows are **left unmarked**, following the precedent the doc pass recorded
(`docs/evidence/the-number-owners.md` B7; `docs/evidence/the-walkthrough-status.md` call 2): their
closure is a ruling that does not exist yet, and a marker for a ruling nobody has made would be
invented. The check stays blind to them by construction — its stated limit. **No verdict was changed**;
only disposition text and three markers were added.

**What the markers do and do not prove** (the check's own caveat): each marker proves the named symbol
still exists — for the two test markers, that the test that would catch a reverted fix is present; for
C-13, that `MEM_RUMOUR` occurs in `module/ship/ship_core.cpp`, i.e. has a writer. It does **not** prove
the fix is correct; the falsifications in Part 2 are what show the checks fail for the right reason.

---

## Part 7 — the checks the brief requires

```
==> ship simulation: unit tests
ship_core: all checks passed

==> crew direction layer: unit tests
crew_core: all checks passed
```

```
$ PATH=/home/robot/.cache/lwh-venv/bin:$PATH scripts/test.sh ; echo $?
...
==> python: syntax
    ok

==> shell: shellcheck
    ok

==> patches: numbered without gaps, and each one parses
    24 patches

all checks passed
0
```

```
$ PATH=/home/robot/.cache/lwh-venv/bin:$PATH scripts/docs-check.sh ; echo $?
==> docs: cited paths resolve
    199 markdown files, ... all resolve

==> docs: owned numbers agree with their owner
    ... 6 claim(s) registered, all agree with their owner

==> docs: register rows agree with the tree
    docs/designs-against-code.md:162: row C-11 closed; marker resolves ('TestOodDispatch' occurs in tests/ship/test_ship_core.cpp)
    docs/designs-against-code.md:186: row C-13 closed; marker resolves ('MEM_RUMOUR' occurs in module/ship/ship_core.cpp)
    docs/designs-against-code.md:486: row D3-2 closed; marker resolves ('TestDoctorNoCommission' occurs in tests/ship/test_ship_core.cpp)
    docs/designs-against-code.md:504: row 3.20 closed; marker resolves (docs/crew-roster.md exists)
    ... 60 register row(s) annotated, all agree with the tree
0
```

(`scripts/check.sh` needs the GDK — a shipped `.map` and the script corpus — and cannot run on this
static host; `scripts/test.sh`, which `check.sh` calls first, is the runnable half, and passes.)

---

## Part 8 — what could not be established, and the judgement calls

**Could not establish:**

1. **Whether the register's remaining verdicts are right.** This pass executes the rows the register
   pins on the code and hands the forks back; where one verdict looked wrong (C-14) it is said so, not
   silently overruled. The register is a dated measurement, and `state=` is authored.
2. **Anything whose only witness is the rendered game.** No engine and no game data were run; the OOD's
   sentence, the shield model, the Borg ramp and the rest are static-host readings, and their forks are
   the owner's. None of the three fixes needs the engine — each is proved by a unit test here.
3. **The forks themselves.** By construction: the evidence does not decide them.

**Judgement calls, named as calls:**

1. **C-11 was written as a fork and is fixed as code.** The register named both options (*"Decide the
   code (`DispatchOrder` should consult the OOD rule) or the document"*) and the brief calls this class
   out explicitly (*"the fix is the consumer, not the predicate's deletion"*). The code option is taken.
2. **`MEM_RUMOUR`'s writer is the deck write-off, and a new `MEM_LOSS` event carries it.** The
   document's own example of a rumour is *"a deck written off … a rumour in everyone who was not"*; the
   event value is appended so no save moves. The alternative — a general social relay — would be a
   feature.
3. **"No commission" is represented as rank `0`, not a new field.** Rank `0` is what Neelix and Seven
   already carry; the fuller model (situational post-authority, a blank rank label) is row 3.14's
   `absent`, named and left.
4. **C-14 is disputed rather than fixed.** The register says the code; the brief's own example of a
   design decision is *"a scalar where the design implies a set"*, and building the shield model is the
   feature the brief forbids. The fork is stated instead.
5. **3.20 is recorded as resolved on the base, not re-disputed.** The row was closed by the owner's
   instruction (PR #133) after this branch was cut; the pass rebased onto it and left the resolution in
   place rather than re-litigating a settled ruling.
6. **C-4 is counted among the rows handed here though the doc pass counts it among its corrections.**
   Its own register row hands the design fork to the code list, so the fork is dispositioned here; the
   row stays `state=closed` for the document half.
7. **`BRIEF.md` was deleted before committing**, as the brief requires, and is not among the files.

## Files

- **Changed:** `module/ship/ship_core.h`, `module/ship/ship_core.cpp`, `tests/ship/test_ship_core.cpp`
  (the three fixes and their checks).
- **Register:** `docs/designs-against-code.md` — a `Disposition 2026-10-12 (code pass)` on each of the
  twelve rows this pass dispositioned, a code-pass summary, and three `register-row` markers; **no
  verdict changed**.
- **Evidence:** this file.
- **Deleted:** `BRIEF.md`.
