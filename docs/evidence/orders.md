# Orders — the third use of the same machine, dispatched to a person

`docs/orders.md` (the owner's design, 2026-10-10) and `docs/staff-meetings.md` (the machinery this
reuses: the brief, the authored skeleton, the enumerated options with their costs, the free-text pill,
the delivery direction per line, the novelty seam, the refusal), applied on `feat/orders` (cut from
`testing`), 2026-10-10.

**The premise, measured.** The order verbs were already built as hooks (`OrderBuild`, `OrderRepairFirst`,
`OrderSecurityTo`, `OrderEvacuate`, `OrderTriage`, `OrderAdvance`), the work queue already existed
(`docs/crew-work.md`), the authority already existed (`docs/access-and-authority.md`), and the meeting
machinery existed in full. **What did not exist was the act:** an order dispatched to *a person*, with
the orders *that person* can be given offered to the giver, and the recipient answering as a person.
This phase builds that act and nothing else.

**Observed, and the command that produced it.** Everything below is the output of a command named beside
it, on this tree, after `cmake -S ../upstream/efgame -B ../upstream/efgame/build-linux
-DLWH_MODULE_DIR=$PWD/module && cmake --build ../upstream/efgame/build-linux`. No model is called, no
audio is rendered, and no asset is added.

## What was built, and where

| file | what it is |
|---|---|
| `module/ship/order_set.def` | **new**: the one place a program reads the classes of order that exist — a row per class, with what it needs of the recipient and the job it pushes. |
| `module/ship/ship_core.{h,cpp}` | `OrderClass`/`OrderNeed` and the spec table (from the `.def`); `MayOrder`, `OrderPermitted`, `DeriveOrders`, `ResolveOrderTarget`, `OrderLabel`, `OrderRefusalRule`, `OrderSetFault`; `OrderBrief` and `BuildOrderBrief`; `DispatchOrder`; the order novelty seam (`OrderNoveltyKey` … `BespokeOrder`); `OrderLineKey`/`PlanOrderAudio`; two new `JobKind`s (`JOB_SECURE`, `JOB_EVACUATE`) derived in `UpdateJobs`. |
| `module/ship/g_ship.cpp` | publishes `lwh_ship_order_*`; the `ship order brief/give/say/close` console; the `g_shipTest 92` (headless lifecycle) and `93` (the frame) demos. |
| `module/ui/ui_lwh_meeting.cpp` | the order face: the recipient's rail, the situation, the orders as pills with their costs, the free-text pill, and the order given and the answer. Reuses the meeting overlay's band and helpers. |
| `tools/orders/orders.py` | **new**: the register check — fails when the `.def` and the dispatch drift in either direction. |
| `scripts/order-check.sh` | **new**: the register check and two headless engine runs. |
| `tests/ship/test_ship_core.cpp` | `TestOrderSetDerivation`, `TestOrderDispatchPushesAJob`, `TestOrderRefusalByRule`, `TestOrderRecipientAsPerson`, `TestOrderBespokeAndVoice`, `TestOrderSaveRoundTrip`; and `test_ship_core --orders`. |
| `tests/tools/test_order_set.py` | **new**: the register check fails both ways, planted, and passes on the tree. |

`module/ship/order_set.def` is included (X-macro) by `ship_core.h`, so the `OrderClass` enum and the
spec table cannot drift from the register:

```
$ python3 tools/orders/orders.py enumerate
ORDER_SEE_TO     ORD_NEED_POST                        JOB_REPAIR   -1
ORDER_SECURE_DECK ORD_NEED_SECURITY                   JOB_SECURE   -1
ORDER_ADVANCE_DECK ORD_NEED_SECURITY                  JOB_RECLAIM  -1
ORDER_EVACUATE_DECK ORD_NEED_SECURITY|ORD_NEED_COMMAND JOB_EVACUATE -1
ORDER_BUILD_PARTS ORD_NEED_ENGINEERING                JOB_BUILD    4
```

## Task A — the order as a one-recipient brief

`OrderBrief` is the meeting's shape with one person: the giver, the recipient, the situation, the
orders *that recipient* can be given (each with its resolved target, its label and what it costs), and
the recipient's own view (name, post, watch, mood, marks, promises, the log they can read) — the same
`BriefParticipant` a meeting carries. `BuildOrderBrief` is a pure read, and the spoken content is
scripted exactly as a meeting's is: an authored skeleton (the reply lines are authored in
`DispatchOrder`), and a bespoke path through the novelty seam. The delivery direction is carried per
line (`MeetingLine.delivery`), and the recipient's lines resolve through the cast map.

## Task B — the set is derived per recipient, and checked

`OrderPermitted(giver, recipient, order)` is the one place the derivation reads; `DeriveOrders` returns
the set. The set follows the person, and it is checked in **both** directions by `OrderSetFault` (an
order in the set the person cannot be given; an order the person can be given that the set omits):

```
$ tests/ship/test_ship_core --orders
the common set is derived, not written out:
  engineer (Chakotay, warp drive): see to the warp drive, fabricate 4 spare parts
  security (Tuvok, no post): take security to the deck 1, retake the deck 1, clear the deck 1
  the same security officer, given the phaser post: see to the phasers, take security to the deck 1, retake the deck 1, clear the deck 1
```

An engineer and a security officer are offered different orders, and **the same officer's set changes
when their post changes** (given a station, the "see to" order appears, naming the system they now
hold). The headless run states both directions with each failure planted and caught:

```
$ scripts/order-check.sh        (excerpt)
    order test: tree consistent=1; planted foreign caught=1; planted missing caught=1
```

The register check is the `api-check.sh` pattern: the `.def` is the register, `ApplyOrder` is the
surface derived from the code, and the check fails when they drift. **Both directions, planted:**

```
$ printf 'ORDER_CLASS(ORDER_GHOST, ...)\n' >> /tmp/orders_ghost.def
$ python3 tools/orders/orders.py check --register /tmp/orders_ghost.def ; echo $?
orders.py: /tmp/orders_ghost.def enumerates 6 order class(es); module/ship/ship_core.cpp handles 5
FAIL  the register names and the dispatch does not handle (1): ORDER_GHOST
1

$ sed -i 's/case ORDER_BUILD_PARTS: {/case ORDER_GHOST_CASE: break;\n case ORDER_BUILD_PARTS: {/' /tmp/orders_ghost_dispatch.cpp
$ python3 tools/orders/orders.py check --dispatch /tmp/orders_ghost_dispatch.cpp ; echo $?
orders.py: module/ship/order_set.def enumerates 5 order class(es); /tmp/orders_ghost_dispatch.cpp handles 6
FAIL  the dispatch handles and the register does not name (1): ORDER_GHOST_CASE
1
```

On the tree the same check passes. `tests/tools/test_order_set.py` proves both failures in-process.

## Task C — dispatch, and the effect is a job

Choosing an order pushes a job into the existing queue, with its kind and target, and the recipient's
response is scripted like a meeting's (skeleton with no model, the delivery per line, the recipient's
own voice, no inference on arrival, a replay from the log):

```
$ scripts/order-check.sh        (excerpt)
    order test: dispatched given=1; job 0 kind repair target warp drive
```

`JOB_SECURE` and `JOB_EVACUATE` are appended to the four kinds the queue already derived, and
`UpdateJobs` derives a secure job from a standing `orderSecurityTo` and an evacuation job from a
standing `orderEvacuate`, so an order's job has a face in the queue while the order stands.

## Task D — the refusal, and the person

An order the giver has no authority to give is **refused by the rule's name, in character, with the
state unchanged**:

```
$ scripts/order-check.sh        (excerpt)
    order test: ensign refused=1 rule "you have no authority over Crewman 035"; state unchanged=1
```

The recipient speaks the rule (`"With respect, you have no authority over Crewman 035. That order is not
yours to give."`) and the log carries `refused: you have no authority over …`. Nothing is dropped
silently and nothing is invented around.

**The bespoke path** classifies the typed order against the option intent descriptors (the same seam a
meeting's free text takes). With no classifier verdict loaded, every typed order is novel and is
**refused in character** — the computer's own invariant — and the simulation applies nothing:

```
$ scripts/order-check.sh        (excerpt)
    order test: bespoke novel refused=1; matched given=1
```

**The recipient answers as a person.** Their morale, conditions and traits move the line:
`test_ship_core --orders` shows a low-morale engineer answering *"Aye. Because you asked."*, while
`TestOrderRecipientAsPerson` shows a person of good morale answering *"Aye -- on it."* with no mark.

## Task E — the log

An order is logged as an act of its own, under the **scope `order`**: the giver's line is
`orders <recipient>: <label>`; the recipient's reply is logged in their own name; a refusal is
`refused: <rule>`. This is named as a choice: it is distinct from a meeting's `command` decision and
from the queue's `work ordered` line, so the record reads "who ordered whom to do what" without
conflating it with either. It is a scope, not a new `MemoryEvent` or a new log kind — the log already
carries `LogEntry.scope`, so a new act did not need a new type.

## The voice, and the call counts

A pill calls no model: the run reports **`model calls 0`** for an enumerated order. The recipient's
lines carry a cast key and are queued in the async window by `PlanOrderAudio` before dispatch, so a
dispatch calls no synthesiser on arrival:

```
$ scripts/order-check.sh        (excerpt)
    order test: voice key a10c470269ff43e0; planned 5 line(s); model calls 0
```

A bespoke order that *matches* dispatches the enumerated option; one that does not is refused. The
same text classifies the same way on replay (`OrderNoveltyKey` is content-addressed over the recipient,
the text and every option's intent, label and cost), so a replay costs no inference.

## A save replays

The order and its job round-trip:

```
$ scripts/order-check.sh        (excerpt)
    order test: save replay loaded=1, orderRepairFirst=6, job back=1
```

`TestOrderSaveRoundTrip` proves the same in-process.

## The picture

The order on the screen — the brief opened, the order being given, and the recipient's answer —
photographed, because whether *being ordered* reads as a moment rather than a menu is the owner's to
judge:

```
$ scripts/order-check.sh
    screenshot: build/g3-home/baseEF/screenshots/lwh_order_give.tga    (the brief: the recipient's rail,
                the situation, the orders as pills with their costs, the free-text VOICE pill)
                lwh_order_answer.tga                                    (THE ORDER: "Crewman 035 -- see to
                the warp drive."  THE ANSWER: "Aye -- on it."  GIVEN AND LOGGED)
```

The frame is the meeting overlay's lower-quarter band (`module/ui/ui_lwh_meeting.cpp`), edited, not a
new screen.

## What could not be verified

- **Whether an order *feels* like an order** — the weight of giving one, and the recipient's response
  landing as a person rather than a yes-man. That is the owner's, and it needs the picture.
- **The embedding classifier (M4) for orders.** As for meetings, no classifier is resident here, so
  every typed order is novel; the seam and its verdict index exist and a loaded verdict dispatches, but
  the tuned threshold is not measured here.
- **The group order** is a named seam and is not built (see "Judgement calls").

## The mark question (docs/orders.md Q3), and the choice made

`docs/orders.md` leaves open *what an order that is refused or resented costs*, and where the mark
lands. The behaviour implemented, and defensible: an order a recipient **takes badly** — morale below
0.35, `TRAIT_POOR_WITH_AUTHORITY`, or a live `COND_EXHAUSTED`/`COND_AFRAID` — writes a **`MEM_ORDER`
mark toward the giver with negative valence**, and the line changes to *"Aye. Because you asked."*. This
uses the existing event (`MEM_ORDER`, already declared) and the existing mark system, so a grudge can
form over repeated orders and decay if unreinforced. It is a choice, not the settled design: the owner
has not decided whether a resented order lands as a mark toward the *giver*, a hit to *holdings*, or
both, and this records the one implemented rather than inventing a policy.

## Judgement calls, named as calls

1. **The group order is a named seam, not built.** `docs/orders.md` names a recipient *or a group*.
   This pass implements the single recipient only, as the brief directs. What a group order would need:
   a recipient set (a department, a watch, the security squad) rather than one index on the brief; a
   set intersection — the orders every member can be given, or the union with the authority checked per
   member — and a dispatch that pushes one job per member or one collective job. None of that is guessed
   at here.
2. **Two new `JobKind`s, and the save version is *not* bumped.** `JOB_SECURE` and `JOB_EVACUATE` are
   appended after the four the queue derived; a job kind is a single byte in the record and the two new
   values are only ever written by this build, so a save made before them loads and replays identically.
   Bumping `SAVE_VERSION` would reject every older save for a format-compatible change, which the rules
   forbid ("a change that makes a save load differently is a defect"). The addition is recorded in the
   `JobKind` comment instead.
3. **`ORDER_EVACUATE_DECK` needs security or command, not any post-holder.** An evacuation is an
   authority's order; the first draft offered it to any system-holder and that read wrong.
4. **The order's target is resolved by the simulation, not chosen by the model.** `ResolveOrderTarget`
   names the system the recipient holds, the most threatened deck (then the deck in most trouble, then
   the recipient's own), or the class's fixed count. A "see to" order on a whole system sets the
   standing order and the job appears when the work does — the queue is rebuilt from state, so a repair
   is only in it when there is something to repair.
5. **`ORDER_SEE_TO` is a class, and its target follows the post.** The design's "hold the plant" is the
   post-holder's "see to" with the plant as the target; a class whose target is derived is how one row
   covers every post without a hand-written per-person table.
6. **The register check reads `ApplyOrder`, the run-time guard reads the predicate.** They are two
   different questions — "is every registered class one the simulation can apply?" (static, Python) and
   "is this returned set exactly the permitted set?" (run-time, C++, both directions, plantable) — and
   both are shown failing in both directions.
7. **"Go and look" is not a row.** The design names it as the junior's order, and it would be a survey
   job; it is left as the register's next row rather than built with a job kind the queue has no work
   for. Named because the design names it.
