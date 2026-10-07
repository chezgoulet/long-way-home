# Power assignment: the player decides, and the ladder is only a default

**Owner's ruling, 2026-10-07.** Verbatim:

> I want to make sure that the players and crew are able to define the systems that go online and offline and how
> much power and operational capacity they will have at any given time. This is the FTL style side of the
> playthrough. I want to make sure that doesn't get lost or turned into a predetermined cascade or hierarchy of
> which systems go offline, that is up to the players.

## The correction this document is

`docs/budget-squaring.md` specified a **shed order** — a priority ladder from life support down to cargo handling
— and `feat/the-budgets` implemented it as `SPECS[].prio` with an allocator that walks that order and stops when
the supply runs out. The design reasoned carefully about *which* order was right, and recommended one.

**That was the wrong shape, and it is the shape the owner has just named as the failure.** A ladder is a
*predetermination*: it decides, before the player ever arrives, that the holodecks go dark first and that the
warp drive is the last thing given up. It is a good answer written by someone who is not in the room, applied
without asking.

**The ship does not decide. The player and the crew decide.** That is the design.

**Implemented on `feat/power-assignment`, 2026-10-07.** The mechanism is in
`module/ship/ship_core.{h,cpp}`: `System::share` (the allocation, a number) and `System::allocBy`
(its provenance), `SetAllocation` / `SetAllocationBy`, `PowerCommitted` / `PowerShortfall`,
`SetPowerAuto`, `RecommendAllocation` / `AcceptRecommendation` / `RefuseRecommendation`, and
`GrantBand` / `RevokeBand`. The console is `module/ui/ui_lwh_engineering.cpp` (the FTL surface, and
every item above is on it), the engine check is `scripts/power-check.sh`, and the evidence is
`docs/evidence/power-assignment.md`. Save format **51**. The ladder's own tests now switch
automatic mode **on**, because that is the only place the ladder runs.

## What the mechanic actually is

**A reactive plant, and a set of commitments the player makes against it.**

- **The plant supplies a finite output**, which the crystal's ceiling sets (Part five of `docs/budget-squaring.md`).
  That half of the model stands: it is real, it varies, and it is not the player's fault.
- **Every system carries two things the player sets at any moment:** whether it is **online**, and **how much
  power it gets**. Not a tier. A number.
- **Operational capacity is a function of that number, not a category.** A system at forty per cent does not
  "go offline" — it **underperforms**, at forty per cent. A shield at half power holds against less. A sensor
  at half power sees nearer. Warp at half power is slower. This is the whole point: *almost-on* is a real state,
  and it is the state the ship spends most of the run in.
- **Nothing sheds itself.** If the commitments exceed the supply, the ship **tells the player by how much it is
  oversubscribed** and waits. It does not quietly choose. Silent self-shedding is the failure mode.

### The one exception, and it must be earned

**A physical failure removes capability. A policy decision never does.**

A destroyed conduit, a system with no health left, no deuterium, a deck open to space — those remove what the
ship *has*. That is damage, and damage is not a policy. Everything else is the player's.

## Automation: a convenience the player grants, never the mechanism

There are three legitimate forms of automation, and **each is a delegation the player granted, visible and
revocable** — never a hidden rule:

1. **An automatic mode the player switches on**, which uses the ladder as its policy. Off by default.
2. **A department head's recommendation.** The chief engineer says what they would do, on the console, with
   their reasoning. The player accepts or refuses. Refusing is recorded, and the officer remembers it.
3. **A delegated authority.** The player grants a chief engineer standing authority over a band of the budget —
   and it is theirs until revoked, and the log says who holds it.

**And the meeting is where it becomes a scene.** A staff meeting is where allocations are argued and set: the
brief carries the current commitments, the plant's output, and the department heads' competing asks; the
outcomes are allocation decisions; and the officers remember what they were overruled on. That is how the crew
participate in the FTL side rather than watching the computer do it.

## What the ladder becomes

**`SPECS[].prio` is demoted from the mechanism to a default policy used by automatic mode only.** Concretely:

- **Automatic mode off (the default): the allocator assigns exactly what the player and crew have set, in full,
  and reports any shortfall as a number.** It never cuts anything.
- **Automatic mode on: the ladder applies**, as the ship's own judgement, and the console says so.
- **The ladder's reasoning stays valuable** — it is the chief engineer's opinion, and it is what he recommends.
  It is not law, and it is never applied to a system a person has set.

**Never apply the ladder when a person has decided.** That is the rule, and it is the one that has to be
testable, because it is the one a future lane will break by accident.

## What the console must show

The FTL surface, and none of it is optional:

- **Every system, in one list**: its name, whether it is **online**, the **power it is getting**, and what that
  power buys — as a **number the player can read**, not a colour.
- **Total committed against total available, continuously.** When the commitments exceed the plant, the console
  says **by how much** and refuses to accept more until the player frees something.
- **The consequence of each choice, visible at the choice.** What the shields lose at seventy per cent; what
  the sensors lose; what the drive loses. A number, beside the control, always.
- **Who set each commitment** — the player, an officer acting on standing orders, or automatic mode. The same
  provenance discipline as the log.

## Acceptance

- **A player-set allocation is honoured exactly**, including the case that proves the design: **the holodecks
  running at full while the shields are dark, and the reverse.** Both must be reachable, both must stick, and
  neither may be overridden by any ladder.
- **Oversubscription is reported, never resolved.** Commit more than the plant supplies and the ship says by how
  much; nothing goes dark on its own.
- **The ladder applies when — and only when — automatic mode is on**, demonstrated by a test that sets an
  allocation, turns automatic mode off, changes nothing else, and shows the ladder not firing.
- **Partial allocation is a real state**: a system at forty per cent delivers forty per cent, and its effect is
  measurable rather than binary.
- **A delegation is visible and revocable**, and revoking it takes effect immediately.
- **The console shows committed against available**, and the shortfall as a number.

## Where this sits

`docs/ship-systems.md` owns the systems. `docs/damage-and-budgets.md` owns the two budgets. This document owns
**who decides** — and the answer is: **the player, then the crew they have delegated to, and the ship only ever
as the last resort of an unset decision.**
