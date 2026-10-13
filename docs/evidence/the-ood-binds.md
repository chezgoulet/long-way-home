# Evidence: the officer of the deck binds — the consult, and the refusal's ground

The brief (C-11, ruled **2026-10-12**): *"WIRE IT"* — `OodMayOrder`/`OodRefusal` are consulted by
`DispatchOrder`, *"so the deck's documented authority can actually dispatch an order"*, and the refusal
is *"legible to whoever gave the order … rather than silently dropping it."* The design and the
authority for the pass are **`docs/crew-manifest.md`** (*"Who reports to whom"*), **`docs/orders.md`**
(what an order is and who may give one) and **`docs/access-and-authority.md`** (the two axes); the
register's ruling is **`docs/designs-against-code.md`** C-11.

Date: **2026-10-12**. Branch `feat/the-ood-binds`, cut from `testing` (at `4c6759a`). **Static host:
no engine, no game data.** Everything below is the unit tests
(`tests/ship/test_ship_core.cpp`, built by `scripts/test.sh`) and `scripts/ood-check.sh`, read off a
terminal; the rendered game is not in reach here.

Reproduce with:

```
export PATH=/home/robot/.cache/lwh-venv/bin:$PATH    # cmake (the static host)
scripts/ood-check.sh
scripts/test.sh
scripts/docs-check.sh
```

## The premise, checked against the tree — one leg was already built

The brief's premise is *"`OodMayOrder` and `OodRefusal` already exist and are unit-tested. **Nothing
consumes them.**"* **Half of that is no longer true on this branch, and it would be wrong to say
otherwise.** The register's own code pass of 2026-10-12 (`8592a8d`,
`docs/evidence/the-code-contradictions.md` §2a) already gave `OodMayOrder` its consumer: `MayOrder` (`module/ship/ship_core.cpp`)
returns true when the giver is the deck and `OodMayOrder` allows it, so `DeriveOrders`,
`OrderPermitted` and `DispatchOrder` all carry the deck's authority. The consult **already bound**, and
the register row C-11 was already `closed`.

What was **not** built is the brief's *other* leg. `OodRefusal` was consumed by nothing but
`OodMayOrder` and the unit test, and `DispatchOrder`'s refusal used the generic rule
(`"you have no authority over …"`), not the deck's own ground. That is the brief's own acceptance
clause — *"the refusal names its ground (`OodRefusal`'s own words) rather than a generic denial"* — and
that is what this pass wires. So the row is closed on **both** legs, and the two predicates now each
have a decision that consumes them: `OodMayOrder` makes the order *given*, `OodRefusal` names why it is
*not*.

## The line, and the document's own claim

**The line, as built** (`OodRefusal`, `module/ship/ship_core.cpp`): under their watch, the officer of
the deck **may give any enumerated operational order to anyone fit aboard below the captain, including
officers senior to them**; it **may not** order the captain, order itself, or order a person who is
dead or unfit; and its authority is **not access** — it opens no console and grants no clearance
(`MayOperate(deck, STN_ENGINEERING)` is **0**). The five order classes are all operational (see to,
secure, advance, evacuate, build), so the deck's reach is the ship's *operation*, which is the scope the
manifest gives it.

**The document's claim, beside it:**

- **`docs/crew-manifest.md:243-246`** — *"The **officer of the deck** — the watch officer on the
  bridge — has authority over the ship's operation for that watch, *including over officers senior to
  them who are not the captain*."* **The code matches it exactly**: the deck orders Tuvok (rank 4) and
  is refused the captain.
- **`docs/access-and-authority.md:17-20`** — *"**rank controls access absolutely; rank governs
  compliance probabilistically.** … Both locks, both real, and they should be modelled separately."*
  The code keeps them separate: the deck's ordering authority is *authority* (`OodMayOrder`), and it
  buys no access (`MayOperate` 0).

**No correction was needed in either document, and none was made.** The two documents do not disagree
with the code or with each other: `access-and-authority.md` scopes *access* (rank-derived, binary) and
reserves *authority* for exactly this kind of rule, so an OOD who orders a senior without operating
their station is the document's own design, not a contradiction of it. The only document prose that
could read as if `MayCommand` were the sole route to an order is `docs/orders.md:28-30`; it says the
authority *exists* and names `MayCommand` as the rank rule, which remains true — the deck's route is an
addition to it, recorded here and on the register row, not a change to that sentence.

## The demonstration — the model's own output

`test_ship_core --ood`:

```
== the line: the deck's authority is authority, not access
  the deck is Tom Paris (rank 3); MayOperate(engineering) 0
  may order Tuvok (rank 4): yes
  may order the captain Kathryn Janeway: the captain is not the deck's to order
== the consult binds: the deck's order to a senior is dispatched
  the deck -> Tuvok: given; orderRepairFirst 4
== the refusal is legible: the deck's order to the captain names its ground
  the deck -> the captain: refused
  the rule: "the captain is not the deck's to order"
  the rule is the deck's own words: yes
  the ship is unchanged: yes; nobody was given it: 0
```

Read as evidence rather than a screenshot, the lines are the acceptance:

- **the consult binds** — the deck is a rank-3 lieutenant, seniority alone would refuse the order to
  Tuvok (rank 4), and the order is **given**: `orderRepairFirst 4` is Tuvok's post, so the simulation
  applied it. This is `TestOodDispatch` made visible; the deck reaches an officer the rank rule alone
  cannot.
- **the refusal is legible** — the deck's order to the captain is **refused**, and the rule it is
  refused by is **the deck's own words** (`the captain is not the deck's to order`), verbatim from
  `OodRefusal`, not the generic denial. Nothing was given (`nobody was given it: 0`) and no job was
  pushed (the ship is unchanged by the order; only the log records the refusal, as `docs/orders.md`
  requires: *"never silently dropped, and never invented around"*).

## Falsification — break the consult, and the check catches it

`scripts/ood-check.sh` compiles the test against a **broken copy** of `ship_core.cpp` in which the
deck's authority is **unconditioned** — *"the consult is always true"*, which is the brief's own
falsification:

```c
// from:  if (giver == OfficerOfTheDeck(s) && OodMayOrder(s, giver, recipient)) return true;
// to:    if (giver == OfficerOfTheDeck(s)) return true;
```

The broken copy must FAIL on the refusal leg — the order that should have been refused is given:

```
==> ood: the falsification (the consult always true, the test must fail)
      FAIL  the deck's refusal names its ground: !OrderPermitted(s, ood, captain, class0) (line 9484)
      FAIL  the deck's refusal names its ground: r.refused && !r.given (line 9487)
      FAIL  the deck's refusal names its ground: r.refusal == std::string(OodRefusal(s, ood, captain)) (line 9488)
      FAIL  the deck's refusal names its ground: r.refusal.find("captain") != std::string::npos (line 9489)
      FAIL  the deck's refusal names its ground: r.replyCount >= 1 && r.reply[0].text.find("not yours") != std::string::npos (line 9491)
      FAIL  the deck's refusal names its ground: s.jobs.size() == jobsBefore (line 9492)
    the broken copy fails on the refusal leg; the tree passes
```

Restored, the same build prints `ship_core: all checks passed`. The new check is `TestOodRefusalLegible`
(`tests/ship/test_ship_core.cpp`); the positive leg stays `TestOodDispatch`.

## The predicated functions are unchanged, and their tests still pass

`OodMayOrder` and `OodRefusal` are byte-identical to the tree the brief was written against; this pass
consumes them and changes neither. Their own test, `TestOodAuthority`, is unchanged and still passes —
in the demonstration above it is `may order Tuvok (rank 4): yes` and `the captain is not the deck's to
order`, and in the suite it reports no failure. The whole suite ends:

```
ship_core: all checks passed
```

`scripts/ood-check.sh`, `scripts/bills-check.sh`, `scripts/test.sh` and `scripts/docs-check.sh` exit 0.

## The save

**Nothing.** The consult is a derivation, and a refusal writes no ship state — no standing order is set
and no job is pushed (asserted: `s.jobs.size() == jobsBefore`). The log records the refusal, exactly as
the design requires, and the log is already in the format. `SAVE_VERSION` stays **63**; no field is
added, and a save written before this pass loads and replays identically. (The register's own C-11
disposition said the same of the first leg: *"C-11 writes no state."*)

## The player's side

The player at a console reads the refusal on the order path they gave it: `ship order give <n>` prints
`lastOrder.note`, which for a refused order is `refused (<the ground>); the ship is unchanged` — and
`<the ground>` is now the deck's own words. The refusal is also spoken in character as the recipient's
reply (`"With respect, the captain is not the deck's to order. That order is not yours to give."`),
which is line 0 of the `OrderOutcome` the surface holds. So *whoever gave the refused order sees why*.

## Judgement calls, named as calls

1. **Where the deck's line sits is this pass's call, and it is the code's existing line, not a new
   one.** `OodRefusal` refuses three things — the captain (`rank >= 6`), the deck itself, and a person
   who is dead or unfit — and permits the rest, including seniors. I read that as the manifest's
   *"authority over the ship's operation for that watch"* and kept it unchanged; widening or narrowing
   it (a department scope, a compartments-only scope) would be new authority, which the brief forbids.
2. **The ground is returned only when the deck is the giver.** `OrderRefusalRule` quotes `OodRefusal`
   only when `giver == OfficerOfTheDeck(s)`; every other giver keeps the generic denial (`"you have no
   authority over …"`), which the ensign case and its test (`TestOrderRefusalByRule`) rely on. This
   keeps the change to the one sentence the brief asks for.
3. **The check is a console transcript, not a frame.** The static host draws nothing; the refusal is
   proved at the model and the console-string level, and the screen's own render is named below.
4. **A second falsification, not a replacement.** The register's first leg is still falsified by
   reverting the `MayOrder` branch (recorded in `docs/evidence/the-code-contradictions.md`); this
   pass's falsification is the *always-true consult*, which fails the refusal leg. Both are kept.

## What could not be verified

- **The rendered frame.** No engine runs here, so the order brief's pill list and its refusal line were
  not *drawn*. The strings the surface reads are proved; whether the refusal *reads* as legible on the
  screen is the playtest host's, the same as every other surface's.
- **The console verb in the running engine.** `ship order give` reaches `DispatchOrder` in the source
  (`module/ship/g_ship.cpp`) and the module builds, but it was not driven through `Svcmd_Ship_f` on
  this host.
- **Whether ordering a senior *feels* like the deck taking command** — that is the owner's walkthrough,
  not a harness.

## Register and document corrections made in the same pass

- **`docs/designs-against-code.md` C-11** — a second disposition records the refusal leg; the row's
  marker is moved to `scripts/ood-check.sh`, which proves both legs.
- **`docs/gates.md`** — the bills entry gains the deck's binding and legible refusal, with the evidence
  named.
- **`docs/orders.md`, `docs/crew-manifest.md`, `docs/access-and-authority.md`** — read and quoted; no
  correction was needed and none was made (see *The line* above).
