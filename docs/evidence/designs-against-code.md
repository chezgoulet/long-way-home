# Evidence — the reconciliation register: the designs against the code

Date: 2026-10-11. Branch: `feat/the-designs-vs-code`, cut from `testing`. **Documentation only: no `module/`
file, no patch, no map and no engine was changed.** The only artifacts are
`docs/designs-against-code.md` (the register) and this record. `BRIEF.md` was deleted before committing, as
the brief requires.

This records: **what was observed**, the **commands that produced it**, and the three falsifications that keep
the register's `implemented` verdicts from being a reading.

---

## Part 1 — the three falsifications

The brief asks for three claims classified `implemented`, each checked by an independent witness. Each check
below **fails on a planted error as well as passing on the tree**, so it can fail for the reason we care about;
a classification verified only by reading would be a reading.

### 1. holdings is a fold, not a stored value

- **Claim:** `docs/morale.md:141-143` and `docs/character-derivation.md:30-35` — holdings is a fold over the
  marks; no stored field.
- **Command and observed result:**

```
$ scripts/holdings-check.sh
==> holdings: no stored field in module/ship (the fold, not a float)
    no stored holdings field in module/ship
    the plant is caught:
      FAIL  /tmp/tmp.VK1rs99rrY/ship/ship_core.cpp:10370: namespace ship { static void Planted(Ship &s) { s.crew[0].holdings = 0.5f; } }
      1 stored-field reference(s) found; holdings must be a fold

PASS  holdings is a fold; no stored field exists, and the guard catches one that is planted
$ echo $?
0
```

The second half is the point: the guard is shown **catching a planted `s.crew[0].holdings`**, so the pass is
not vacuous.

### 2. Battle stations and the emergency bills

- **Claim:** `docs/crew-manifest.md:105-116` — battle stations call and stand down with the hands moving, and
  the emergency bills clear a deck and muster the crew.
- **Command and observed result:**

```
$ scripts/bills-check.sh
== the officer of the deck: authority over seniors, not the captain
  the deck is Tom Paris (rank 3)
  may order Tuvok (rank 4): yes
  may order the captain Kathryn Janeway: the captain is not the deck's to order
  but the deck cannot operate the engine room: MayOperate 0

==> bills: the falsification (the battle bill's effect disabled, the test must fail)
      FAIL  the bills: called, observed, stood down: s.systems[SYS_WARP_DRIVE].manned < driveBefore (line 8446)
      FAIL  the bills: called, observed, stood down: clearing > 0 && triage > 0 (line 8455)
    the broken copy fails on the bills; the tree passes

PASS  the watch bill reads, battle stations call and stand down with the hands moving, the
      emergency bills clear a deck and muster the crew, the holes appear under loss, and the
      deck orders a senior officer but not the captain; the guard fails on a planted break
$ echo $?
0
```

### 3. The manner selected from the state

- **Claim:** `docs/character-derivation.md:264-291` — the crew's state selects a manner, raised where the
  state changes, and a save replays it.
- **Command and observed result:**

```
$ scripts/manner-check.sh
  wiring: the same person, just given an order:
    [crew] "Give me the hard one. I want to be the one who did it." -- steady, raised by an order (the morale band (fit): carrying on)
  surface: Vorik, voice munro, delivery report: "Give me the hard one. I want to be the one who did it."
  the cue: spoken; 1 render job(s) queued
  no bank: text (no bank); the line is still written and read
  no render: text (no render); the simulation ran
  same state, same manner twice: yes
  a save replays the manner: yes

PASS  the manner is selected from the state (steady and the bad states, each named, with the
      precedence when several are true); raised where the state changes and where the person
      acts; written to the log and handed to the voice; degrades to text with no bank or no
      render; the same state selects the same manner and a save replays it -- and the planted
      state before this pass makes the check fail.
$ echo $?
0
```

The check runs `--manners-broken` (the wiring removed) and requires the broken run to report itself
(`the selection is NOT wired`), so the pass is not vacuous.

Each classification is also reached by a named unit test in `tests/ship/test_ship_core.cpp`:
`TestHoldingsIsAFold` (line 5895), `TestTheBills` (line 8405), `TestMannerDeterminismAndSave` (line 6212),
built and run by `scripts/test.sh`.

---

## Part 2 — the counts

**400 claims** extracted and classified, by reading the code and not another document. Computed from the
register's own tables:

```
$ python3 - <<'PY'   # counts the verdict cell of every register row, by domain
... (the script is in the register's "The counts" section) ...
PY
dom name                                                  imple  parti  absen  contr  super  unver   tot
  1 The ship and its systems                                 41      8      0      5      2      5    61
  2 Power, damage and the two budgets                        28      4      1      4      4      1    42
  3 The crew and their work                                  27     13      5      7      2      3    57
  4 The record, the log and the memory                       23      5      3      0      0      1    32
  5 Orders, access and authority                             22      2      1      0      1      0    26
  6 The rooms, the screens and the meetings                  24      3      1      8      1      2    39
  7 Encounters and the outside                               24     11      7      0      0      0    42
  8 The start, the endings, the modes                        32      8     10      3      1      2    56
  9 The engine, the policy and the pipeline                  19      9      1      6      7      3    45
    TOTAL                                                   240     63     29     33     18     17   400
```

**23 headline contradictions** are quoted with both sides in the register's contradictions section; the tables
carry **33** `contradicted` rows in total.

---

## Part 3 — the contradictions, both sides quoted

The full set is in `docs/designs-against-code.md`. The ones recorded here demonstrate the shapes:

| # | document says | code does | side wrong |
|---|---|---|---|
| C-1 | `story-and-semantics.md:132-133` "a single `float morale`" | `ship_core.h:511-517` three derived reads; save 53 "the old single `float morale` is gone" | document |
| C-3 | `lore-ledger.md:209` sickbay "6 under treatment at once" | `ship_core.h:2276` `SICKBAY_BEDS = 4` | ledger |
| C-4 | `lore-ledger.md:242` sector "40% empty, 30% hostile, 20% derelict, 10% Borg" | `ship_core.cpp:2686-2689` `roll % 15` → 4/3/2/1 of 15 | ledger |
| C-6 | `walkthrough.md:390` "eighteen systems" | `ship_core.h:31-58` `SYS_COUNT` 23 | walkthrough body |
| C-7 | `walkthrough.md:554` "no nominal-against-current power display" | `g_ship.cpp:722-735` fresh against now | walkthrough body |
| C-11 | `orders.md:28-30` the OOD "may order … senior officers who are not the captain" | `OodMayOrder:6167` called only from tests; dispatch uses `MayOrder:9930` | fork (predicate with no consumer) |
| C-13 | `memory-and-consequence.md:47,55` rumour as a source, distorted and attributable | `MEM_RUMOUR` declared `ship_core.h:381`, no writer anywhere | code |
| C-14 | `ship-systems.md:39,113` fourteen grids, six sections | `ship_core.h:1956` one `shieldStrength` scalar | code (incomplete) |
| C-18 | `engine-extension-policy.md:33-37` "every engine extension is cvar-gated … saves must round-trip" | `patches/0008:12`, `patches/0018:15` raise ceilings unguarded and state saves do not load | policy wording |
| C-21 | `authoring-a-location.md:30` standing distance "24-64" | `tools/posts/gen-posts.py:37` `24.0, 96.0` | fork |

---

## Part 4 — what was not done, and what could not be established

- **Nothing was fixed.** No document's row was edited, `docs/omissions.md` was not touched, and
  `docs/gates.md`'s state was not touched. Every contradiction is left as a lane for the pass that repairs.
- **Static host.** No engine was run. The falsifications are the unit tests and the check scripts; anything
  whose only witness is a rendered screen, the shipped map sources, the paks or the pinned upstream is
  recorded `unverified`, with the reason, in the register.
- **Judgement is not a claim.** Whether a console is pleasant, a deck reads as a place, or the red strip
  reads as red is the owner's and is not classified.
- **The forks.** Where the document and the code disagree and neither is provably later, the register gives
  the fork and does not choose: the sector's roll, the twelve-shot adaptation, and the standing distance.

---

## Part 5 — the checks required by the brief

```
$ scripts/docs-check.sh ; echo $?
==> docs: cited paths resolve
    198 markdown files, 2066 docs/ references, all resolve

==> docs: owned numbers agree with their owner
    ... 6 claim(s) registered, all agree with their owner

==> docs: register rows agree with the tree
    ... 34 register row(s) annotated, all agree with the tree
0
```

```
$ scripts/test.sh ; echo $?
...
all checks passed
0
```

(The counts above include the register and this evidence file; both are new, and both exit clean.)
