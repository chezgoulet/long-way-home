# The doors, and whether the crew can reach its posts — the evidence

This pass closes the residual *"Crew are stopped by some doors — probably the ones meant to be shut"*
(`docs/gates.md`, Residuals). The source and the map had been read; the engine had not been asked.
**This asks it.** Everything below is the output of the command named beside it, on the playtest host
(`sasquatch`), on `feat/the-doors` cut from `testing@3b59de3`, with the module rebuilt from this tree
before any engine check ran:

```
$ cmake --build ../upstream/efgame/build-linux -j"$(nproc)"
[100%] Built target efgame
```

The pass this finishes was cut off by a power failure with its change and its census in the tree and
its verification chain unfinished. This pass read what was there, found two things wrong with it, fixed
both, ran the chain to the end and records it here. **Both corrections are named as calls** (and in the
list at the end); neither changes the pass's design.

## The change, and the two things this pass had to correct in it

The pass left three rules in the module, with a census and a walk to prove them:

1. **An ordinary door opens for any crew member** walking at it.
2. **A security door opens on the member's own access** — the record's marks decide; there is no list.
3. **A door the engine gave no touch trigger is not opened** — a hand's or a script's door.

### Correction 1 — the code did not implement rule 3

`OpenDoorFor` was called for any non-security door the walk was aimed at, whether or not it carried a
trigger, so the layer would have taken a script's door (the ship's `cup*`, `mlockdown*` and
`lwh_d06_door` among them) the moment a post stood behind one. Rule 3 is written in
`docs/ship-systems.md` and in the module's own comment, so the honest repair was to make the code keep
the promise rather than to weaken the documents. The pass's census had also never *exercised* rule 3 —
it asked the layer about a trigger-carrying door and the no-trigger kind only by classification — so the
fix came with the observation that closes that gap (`DoorForAHand`, `LayerMayOpen`, and the census's
third line, below).

### Correction 2 — the census's own two samples were one door

The census asks the engine's own use path about one ordinary door and the layer about a second, so the
two tests cannot be the same test. **On the merged ship they were.** The first two ordinary doors are
the two leaves of one double door (entities 62 and 63, `(unnamed)`); a `func_door` team shares one mover
state; the engine's own call on the first leaf therefore already moved the second, so **with the layer's
opening broken the layer's line still read `-> OPENED`.** The break was caught — by the access
assertion, not by the assertion that exists for it. That is the shape of a check weaker than it reads,
and it was found by breaking the mechanism, not by reading it.

**Fixed, and the fix is asserted:** the census picks its two ordinary samples from **different teams**
(`DoorTeam`), prints each sample's entity with its result, and `scripts/crew-door-check.sh` now requires
the two entity numbers to differ. The falsification below is the same break, now caught by the assertion
that should catch it.

### The engine ground the three rules stand on (`g_mover.cpp`)

- `Touch_DoorTrigger` opens a door only for a body within 72 units of the door centre *and moving at
  it* (dot ≥ 0.8); within 32 units it always opens; beyond 72 it never does. A body that has come to
  rest in a doorway never fires it.
- `Use_BinaryMover` refuses a `MOVER_SECURITY` (32) door until the **player's** `OBJ_SECURITYCODE`
  objective is succeeded — the campaign's gate, not a crew member's record.
- `SP_func_door` gives a door with a `targetname`, a `health`, or `PLAYER_USE` (128) **no touch
  trigger at all**: it goes to `Think_MatchTeam`, not `Think_SpawnNewDoorTrigger`.

The layer is called only from `ThinkMember` — only for a crew member, only under our layer — so the
player's own doors, and every door in the retail campaign, expansion and cMod, are untouched. The
census is cvar-gated (`g_crewDoors`) and is not enabled by any other check.

## Task A — the census, in the engine

One harness run (`scripts/crew-door-check.sh`), which the module drives with `g_crewDoors 1`: it walks
the loaded entity list, classifies every `func_door` from the engine's own state, and then **asks the
engine directly** about one door of each kind — the door's own use path, with a crew member as the
activator, watching the mover. The merged ship, and then the deck 4 walk:

```
$ scripts/crew-door-check.sh
==> census
    [census] doors: 213 total; 178 with a touch trigger; 152 ordinary, 26 security, 35 script-only, 0 player-use; 91 team leaves
    [census] doors: access: 5 of the embodied crew hold security access, 8 do not
    [census] doors: access: lwh_crew_003 is refused security '(unnamed)' (the record gives no access)
    [census] doors: access: lwh_crew_000 opened security '(unnamed)' -> OPENED (the record gives access)
==> walk
    [walk] watch5 at post 'post5' after 2400 ms
    [walk] watch1 at post 'post1' after 2650 ms
    [walk] watch3 at post 'post3' after 2850 ms
    [walk] watch4 at post 'post4' after 5950 ms
    [walk] watch2 failed to reach post 'post2'
    [walk]   stopped by security door '(unnamed)' (ent 280) 20 units away, bound for 'post2', at (-3530 -1250 -3962)
    [walk] watch6 at post 'post2' after 84900 ms
INFO  watch2 is still stopped by security door '(unnamed)' 20 units out: no access, and the door is meant to be shut; the suggestion is the error
PASS  every crew member the layer can get through its doors reached its post
PASS  the doors stop crew exactly where the design means them to
```

The census in full, from the same run's own log (`build/g3-home/doors-census.out`):

```
CREW: doors: census on voyager (probe lwh_crew_000)
CREW: doors: 213 total; 178 with a touch trigger; 152 ordinary, 26 security, 35 script-only, 0 player-use; 91 team leaves
CREW: doors: live: ordinary '(unnamed)' for lwh_crew_000 -> OPENED (ent 62)
CREW: doors: layer: ordinary '(unnamed)' for lwh_crew_000 -> OPENED (ent 64)
CREW: doors: live: security '(unnamed)' for lwh_crew_000 (the member's access says open; the engine's own gate is the player's security code) -> STAYED SHUT (ent 340)
CREW: doors: live: 'cup0' carries no touch trigger: a member walking into it is not seen by it
CREW: doors: layer: 'cup0' is a hand's or a script's door: the layer does not open it
CREW: doors: access: 5 of the embodied crew hold security access, 8 do not
CREW: doors: access: lwh_crew_003 is refused security '(unnamed)' (the record gives no access)
CREW: doors: access: lwh_crew_000 opened security '(unnamed)' -> OPENED (the record gives access)
```

**So, per door, in the engine:**

| what the engine says | count | what it does for a crew member |
|---|---|---|
| has the engine's own touch trigger, not security | **152** | opens — a member that walks into it gets through. The engine's path, observed (`ent 62 -> OPENED`) |
| security (`spawnflags & 32`) | **26** | refuses. The engine's gate is the **player's** `OBJ_SECURITYCODE`; a crew member does not hold it, so it stays shut (`ent 340 -> STAYED SHUT`). The layer opens one on the member's own access, and only then |
| no touch trigger at all (`targetname`, `health`, or `PLAYER_USE`) | **35** | never seen by a walking member, and **the layer does not open it either** — it is a hand's or a script's door (`'cup0' is a hand's or a script's door: the layer does not open it`) |

**The set that stops crew is exactly the set meant to be shut.** No ordinary door stops a crew member;
the security ones stop whoever the record does not clear; the script's doors are not the layer's to open
at all. The residual's *probably* is now a measured yes.

**A correction the pass recorded, kept here because a wrong number gets quoted back afterwards.** The
first census said only **87** doors carried a trigger. That was the instrument's error, not the map's: a
team door's trigger belongs to its *master* (`Think_SpawnNewDoorTrigger` runs on the master, and the
trigger's `owner` is the master), so every leaf was counted triggerless — and the merged ship has **91
team leaves**. Counting a leaf against its master's trigger gives the **178** above.

## Task B — the change: a member opens the door in front of it

1. **An ordinary door opens for any crew member.** `DoorAhead` finds the door the member is walking at;
   `OpenDoorFor` opens it through `Use_BinaryMover`, the same call the touch trigger makes, so the
   engine's own rules still apply.
2. **A security door opens on the member's own access, and only then.** `MayOpenDoor` asks the ship's
   record — `MayOperate(ship, idx, STN_TACTICAL)`, i.e. security's own department, a lieutenant
   commander and above, a trained cross-qualification, or a shift's delegation. **The marks decide;
   there is no list.** Where the access is granted the layer drives the mover itself, because the
   engine's gate is the player's objective and not the member's record; where it is not, the door stays
   shut and the member is named where it stops.
3. **A door with no touch trigger is not opened.** `DoorForAHand` is the engine's own test
   (`targetname || health || PLAYER_USE`), and `LayerMayOpen` — the one place the walk and the census
   both ask — declines it. A post behind one is a content error, and the walk names it as one.

**The access rule, observed** — the same census, on the same security door:

```
CREW: doors: access: 5 of the embodied crew hold security access, 8 do not
CREW: doors: access: lwh_crew_003 is refused security '(unnamed)' (the record gives no access)
CREW: doors: access: lwh_crew_000 opened security '(unnamed)' -> OPENED (the record gives access)
```

The member without access is refused; the member whose record gives security's access opens it. That is
the security door's refusal, and its permission, demonstrated on a real map door and real crew records.

## Task C — the reachability, per starting position, and the fork the brief names

`crewgen suggest` is a starting point: **it cannot see doors.** The deck 4 positions it proposes were
walked in the engine. The layer names the door that stops a member, with the door entity and the
distance in the engine's own units:

```
    [walk] watch5 at post 'post5' after 2400 ms
    [walk] watch1 at post 'post1' after 2650 ms
    [walk] watch3 at post 'post3' after 2850 ms
    [walk] watch4 at post 'post4' after 5950 ms
    [walk] watch2 failed to reach post 'post2'
    [walk]   stopped by security door '(unnamed)' (ent 280) 20 units away, bound for 'post2', at (-3530 -1250 -3962)
    [walk] watch6 at post 'post2' after 84900 ms
```

### The fork, settled

The brief named three answers and left the pass to choose. **What decided it is the engine's own
behaviour per door and the design's own access rule — not a preference:**

1. **Ordinary doors open to the crew.** Decided by the engine: all 152 non-security doors carry the
   touch trigger, and the engine's own use path opened the sample for a crew member (`ent 62 ->
   OPENED`). The layer's opening is therefore not a new permission — it is a guarantee that the Trek
   trigger's own terms (moving at the door, within 72; always within 32) cannot refuse a body that has
   come to rest at a doorway.
2. **Access is derived from the marks.** Decided by the design (`docs/access-and-authority.md`:
   security over doors, force fields and the brig) and by what the engine's gate actually is: the
   player's single `OBJ_SECURITYCODE`, which answers a different question from *what this member may
   pass*. The record answers that question, and the census shows both answers: 5 of the embodied crew
   hold security's access, 8 do not; the one who does opens the door, the one who does not is refused.
3. **Where a crew member could not reach, the door was right and the content was wrong.** The deck 4
   case is a **security door** between `watch2`'s *suggested* start and its post: the member held no
   access, so the door stayed shut, and the member stopped in the doorway — 20 units from the door's
   centre, with its post 132 units beyond it. **That is the simulation working**, and it was not
   flattened into a bug: the layer does not open a security door for a member the record does not
   clear. **What was wrong was the suggested starting position** — `crewgen suggest` cannot see doors
   and says so. *(The pass's evidence said "the committed scenario moved it"; that is corrected rather
   than repeated: `scenarios/deck04-watch` has carried `watch2`'s near-side position since it was
   authored on 2026-10-05, `110fc15`, and no scenario or map change is made by this pass.)*

## The falsification — the check fails when the mechanism is broken

Three runs of the same script on the same tree, with the layer's door opening (a) working, (b) made a
**no-op**, (c) with the **hand's-or-a-script's rule removed**. Both breaks are one line each, built from
the fixed source and reverted afterwards (`/tmp/opencode/make-door-breaks.sh`; nothing broken is
committed).

**Broken 1 — `OpenDoorFor` returns immediately** (the old behaviour: the layer opens nothing). The layer
sample stays shut while the engine's own sample opens, and the run exits non-zero:

```
$ scripts/crew-door-check.sh           # OpenDoorFor a no-op
==> census
    [census] doors: 213 total; 178 with a touch trigger; 152 ordinary, 26 security, 35 script-only, 0 player-use; 91 team leaves
    [census] doors: access: 5 of the embodied crew hold security access, 8 do not
    [census] doors: access: lwh_crew_003 is refused security '(unnamed)' (the record gives no access)
    [census] doors: access: lwh_crew_000 opened security '(unnamed)' -> STAYED SHUT (the record gives access)
==> census-frozen
    [census-frozen] doors: 213 total; 178 with a touch trigger; 152 ordinary, 26 security, 35 script-only, 0 player-use; 91 team leaves
    [census-frozen] doors: access: 5 of the embodied crew hold security access, 8 do not
    [census-frozen] doors: access: lwh_crew_003 is refused security '(unnamed)' (the record gives no access)
    [census-frozen] doors: access: the layer's doors are frozen (g_crewDoorOpen 0): lwh_crew_000's access opened nothing
FAIL  the layer did not open an ordinary door for a crew member
```

and, from the same run's `build/g3-home/doors-census.out`, the two samples side by side:

```
CREW: doors: live: ordinary '(unnamed)' for lwh_crew_000 -> OPENED (ent 62)
CREW: doors: layer: ordinary '(unnamed)' for lwh_crew_000 -> STAYED SHUT (ent 64)
```

**Broken 2 — `DoorForAHand` always false** (the layer would take a script's door). The run exits
non-zero on the rule that exists to stop exactly that:

```
$ scripts/crew-door-check.sh           # DoorForAHand always false
==> census
    [census] doors: 213 total; 178 with a touch trigger; 152 ordinary, 26 security, 35 script-only, 0 player-use; 91 team leaves
    [census] doors: access: 5 of the embodied crew hold security access, 8 do not
    [census] doors: access: lwh_crew_003 is refused security '(unnamed)' (the record gives no access)
    [census] doors: access: lwh_crew_000 opened security '(unnamed)' -> OPENED (the record gives access)
==> census-frozen
    [census-frozen] doors: 213 total; 178 with a touch trigger; 152 ordinary, 26 security, 35 script-only, 0 player-use; 91 team leaves
    [census-frozen] doors: access: 5 of the embodied crew hold security access, 8 do not
    [census-frozen] doors: access: lwh_crew_003 is refused security '(unnamed)' (the record gives no access)
    [census-frozen] doors: access: the layer's doors are frozen (g_crewDoorOpen 0): lwh_crew_000's access opened nothing
FAIL  the no-touch-trigger door was not declined by the layer's own rule
```

and the raw line that failed it, from the same run:

```
CREW: doors: layer: 'cup0' has no touch trigger: the layer WOULD OPEN IT
```

**Restored — the same check on the fixed source again** is the run quoted in full in Task A and Task C
above (`build/doors/check-final-c.log`, `DOOR_C_EXIT=0`). `scripts/crew-door-check.sh` also carries the
pass's own frozen test — `g_crewDoorOpen 0`, the old behaviour planted from the console — and fails if
the frozen census reports the layer opening anything. The two breaks above are the stronger form: a
compiled-in break, not a cvar.

## The measurement

The G3 measurement (`scripts/g3-measure.sh`, the charter's ten-minute run) on this tree, with the doors
change in it and the corrections above made — **before and after the pass's own correction**, because
the interrupted pass ran its chain on the module as it left it (`build/doors/g3-after.log`):

Before (the doors change as the pass left it — rule 3 not implemented, the census's samples one door):

```
PASS          post coverage at every sample                      minimum 100% over 121 samples (bar 90%)
PASS          every NPC reaches its post within the bound        slowest 6000 ms (bound 90000 ms)
PASS          zero navigation failures                           0 stuck, 0 out of world
G3 measured run: PASS
```

After (this pass's corrections):

```
PASS          5-10 crew present on one deck                      6 on tour/deck04
PASS          post coverage at every sample                      minimum 100% over 121 samples (bar 90%)
PASS          observation run long enough                        600 s sampled (bar 600 s)
PASS          every NPC reaches its post within the bound        slowest 5950 ms (bound 90000 ms)
PASS          zero navigation failures                           0 stuck, 0 out of world
PASS          address -> acknowledgement within the bound        6 of 6 acknowledged, slowest 1500 ms (bound 5000 ms)
PASS          no ICARUS script regressions                       0 precedence violations; scripts in flight 4 with the crew, 4 without
PASS          a script takes a post-holder, and gives them back  1 script run on a crew member at post: the layer yielded 1 time(s), and had them back on duty 1 time(s)
PASS          save/load restores posts and schedule cursor       6 crew compared field by field; after the reload coverage is 100%
PASS          save-size increase within budget                   25 bytes per NPC of layer state (budget 256); the whole save grew 53583 bytes with 6 crew added
PASS          frame time holds                                   game frame 285 us with the crew, 200 us without (slack 1000 us); layer 5 us avg, 49 us max per frame
G3 measured run: PASS
```

**The correction changes nothing the measurement can see** — the same verdicts, the slowest arrival
5950 ms against 6000 ms (jitter, not a change), and 0 stuck either way. The measurement is on the
committed scenario (`scenarios/deck04-watch`), whose positions are the ones that can be walked,
including `watch2`, the member the *suggestion* cannot get to its post. For the record, the run before
the doors change at all is the previous pass's, on the floor fix alone
(`docs/evidence/the-posts-and-the-floor.md`): *"`zero navigation failures` — 0 stuck, 0 out of world"*.

S5 (the ship's routine: who is aboard, and whether they walk to their place) on the same tree:

```
$ scripts/s5-check.sh
    ship time 08:00, deck 2: the ship has 41 here (showing up to 24), 24 embodied
    ship time 09:04, deck 2: the ship has 0 here (showing up to 24), 0 embodied
    ship time 15:04, deck 2: the ship has 40 here (showing up to 24), 24 embodied
    ship time 17:04, deck 2: the ship has 0 here (showing up to 24), 0 embodied
    ship time 23:04, deck 2: the ship has 41 here (showing up to 24), 24 embodied
    ship time 01:04, deck 2: the ship has 0 here (showing up to 24), 0 embodied
    ship time 07:04, deck 2: the ship has 60 here (showing up to 24), 24 embodied
    ship time 09:04, deck 2: the ship has 0 here (showing up to 24), 0 embodied
PASS  through 4 meals and 4 spells between them, the crew embodied were the crew the ship had aboard
INFO  of 24 embodied, 24 reached their place within the meal hour and 0 gave up
PASS  most of the crew embodied walked to their place on the deck
```

And the two checks that need no engine — `scripts/test.sh` ("all checks passed") and
`scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts`
(exit 0, "the 8 rejections are exactly the known, documented set") — both pass on this tree.

## What could not be confirmed

- **Whether the layer's *ordinary* door opening is ever actually needed on live content.** The engine's
  own touch trigger already opens an ordinary door for a body within 32; on deck 4 no member was stopped
  by an ordinary door. The layer's opening is a guarantee against the case where the navigator leaves a
  body short of the trigger — **that case was not produced in live content here**. The census's layer
  test is a direct call to the layer's own function, not a walked body, and is named as a call.
- **Rule 3 on real content is an observation, not a walk.** The census asks the layer's own rule about a
  real no-trigger door (`cup0`, and the class of 35) and the layer declines. **No post in the content
  walked here stands behind one**, so the walk never had to name a script door; the break-2 run is what
  shows the rule is enforced and not decorative. Of `DoorForAHand`'s three clauses, only `targetname`
  is exercised by this content: the merged ship has **no** `PLAYER_USE` door and none with a `health`,
  so those two branches rest on the engine's own rule and the reading of `SP_func_door`, not on a run.
- **The second suggested position the residual names.** `watch2`'s cause is measured and named.
  `watch6`'s suggested position also differs from the committed one; on this tree the member is first
  sent to cover `post2` (the director working, `docs/g3-reactive-crew.md` §8) and reaches at 85 s, and
  the earlier failure's exact cause is not re-measured here.
- **The merged ship's seeded posts.** The census classifies the ship's doors. Whether every seeded post
  on the merged ship is reachable needs the S5 walk on the merged ship, which is not measured here; the
  reachability measurement here is the deck 4 scenario.
- **The frames.** All of this is headless `xvfb`; nothing here is a judgement of what a player sees.

## Judgement calls, named as calls

1. **Rule 3 was implemented, not documented away.** The code did not do what three documents said; the
   documents were kept and the code changed, with the reason written down. The alternative — restating
   the rule as "the layer opens any non-security door" — would have weakened a promise to fit the code.
2. **The census's samples were made independent, and the independence is asserted.** A check whose
   layer assertion could be satisfied by the engine's own call is decoration for the break it exists to
   catch; the two samples now come from different teams and the check refuses to read a shared one.
3. **The security door follows the record, not the player's objective.** The design gives security the
   doors; the layer asks the ship. Where the record says yes, the layer drives the mover itself, because
   `Use_BinaryMover` answers the campaign's question, not the member's.
4. **The deck 4 block is a content error, not a door to force** — and the content that was wrong is the
   *suggestion*, not the map, the post, or the committed scenario. The layer does not open a security
   door for a member the record does not clear, and the class of error is now named in the log
   (`stopped by script door` / `stopped by security door`) rather than guessed.
5. **`DoorAhead`'s reach (320 units) and its 0.7 "on the way" test are the layer's own, not the
   engine's.** The engine's trigger numbers are 72/32; the layer needs to identify a door before a body
   is at the doorway, so it looks further and tests the door's own middle against the direction of
   travel. It is a heuristic, named as one, and the failure line prints the distance so a wrong guess is
   visible.
6. **The census's layer tests are direct calls to the layer's own functions**, on real map doors and
   real crew records; the walk is the separate, real-content measurement.
7. **The pass's evidence was corrected in place rather than smoothed.** Two claims did not survive
   re-testing — "the committed scenario moved it" (it never did) and the census's layer assertion (it
   did not test what it looked like it tested) — and both are named here, with what replaced them.
8. **This pass ran after an outage that cut off the pass before it.** The tree carried the change and
   the census uncommitted; the finishing pass rebuilt the module from that tree, corrected what it
   found wrong, ran the chain to the end and recorded it here.
