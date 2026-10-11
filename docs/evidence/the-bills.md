# Evidence: the bills — who does what when it goes wrong

The pass that closes **O6** (`docs/omissions.md`), the finding that *"the crew manifest's bills are
designed and unbuilt"*. The design and the authority for the pass is **`docs/crew-manifest.md`**
(*"The bills: the answer to 'who does what'"* and *"Who reports to whom"*); the queue, the watch and
assignment are `docs/crew-work.md`; the officer of the deck's authority meets `docs/access-and-authority.md`;
what a bill does is read from `docs/ship-systems.md` and `docs/damage-and-budgets.md`; the surface is
where the console screens live (`docs/evidence/every-screen.md`, `docs/evidence/the-missing-screens.md`).

Reproduce with:

```
export PATH=/home/robot/.cache/lwh-venv/bin:$PATH    # cmake (the static host)
scripts/bills-check.sh
scripts/test.sh
scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts
```

The static host runs no engine. The demonstration below is the model's own transcript
(`test_ship_core --bills`, the same strings the screen draws); the rendered frame is the playtest
host's, and is named as such under "What could not be verified".

## The census first — what already existed, and was therefore not rebuilt

**A bill built twice is worse than one built late**, so the census came before any change. By symbol
and file, before this pass:

| the manifest asks for | what already existed | where |
|---|---|---|
| **the watch bill** (who is on watch, at which station, when it turns) | the per-record `watch` field; `WATCHES`, `SECONDS_PER_WATCH`; `ScheduledActivity`; `Ship::Watch()`; the watch-change meeting `MEET_WATCH_CHANGE` and its schedule `lastWatchMeeting`; `CoverWithCrew`/`WatchCoverage`; `PostsNeeded` | `module/ship/ship_core.{h,cpp}`; `tests/ship` `TestRoster`, `TestSchedule`, `TestADayAboard` |
| **the queue and assignment** (the crew-work economy) | `Job`, `Jobs`, `OrderBuild`, `SetJobPriority`, `UpdateJobs`; the job-queue board `ui_lwh_jobs`; `ship jobs` / `ship job`; `JobKind` (repair/seal/reclaim/build/secure/evacuate) | `module/ship/ship_core.*`, `module/ui/ui_lwh_command.cpp`; `TestJobQueue` |
| **the condition** (green/yellow/red) | `Alert`, `SetAlert`, `SuppressByAlert`, `MayCallAlert`, `StressNow`; the panel's `CONDITION` band (`module/ship/lwh_panel.cpp`); `ship alert` | `module/ship/ship_core.*`, `module/ship/g_ship.cpp` |
| **the damage-control *party*** | unposted engineers join the repair parties; `SPECS[].repairing`; `orderRepairFirst`; the log author "damage control" | `module/ship/ship_core.cpp` `UpdateCrew`; `TestDamageControl` |
| **a security response** | the security squad `OrderAdvance`/`advanceDeck`, the boarders' defenders | `module/ship/ship_core.*`; `TestSquad` |
| **clearing a deck** | `OrderEvacuate`/`orderEvacuate` and `JOB_EVACUATE` | `module/ship/ship_core.*`; `TestJobQueue` |
| **access and the lock-out** (the seam with the OOD) | `MayOperate`/`RecordOperates`, `MayCallAlert`, `MayCallUp`, `Delegate`/`RevokeDelegation`, `BeginOverride`/`ConfirmOverride`, `LockOut`/`LockoutNotice`, `AccessRefusal` | `module/ship/ship_core.*`; `TestAccessAndAuthority` |

**The census's finding is the honest one:** the watch bill and the queue are already the
simulation's, so this pass **extends** them and does not rebuild them. What did **not** exist:

- a **bill assignment per crew record** — a battle station and an emergency assignment, distinct from
  the normal post;
- **battle stations** as a bill distinct from a plain red alert — calling it reconfigures hands to
  posts that are *not* their normal ones, and standing it down returns the routine;
- the **emergency bills** (fire, decompression, intruder, abandon ship) as callable bills;
- the **damage-control organisation** as a *reporting* rule (the repair parties answer to the
  engineer of the watch, whatever the rank of anyone present);
- the **officer of the deck**: the post, and its authority over officers senior to them who are not
  the captain.

## What was built

`module/ship/ship_core.{h,cpp}` — the model; `module/ship/g_ship.cpp` — the console verb `ship bill`
and the published state; `module/ui/ui_lwh_command.cpp` — the screen `ui_lwh_bills`, reached by `B`
at the command console; `module/ship/console_api.def` — the enumerated `bill` verb.

- **The per-record assignment** is derived from the department and the post at roster build
  (`DeriveBills`, `BattleRoleFor`) and **stored** (save format **59**), so the reconfiguration is
  data on the record and the holes can be named by symbol. The command crew hold the bridge
  (`rank >= 4`); engineering and the labs are **damage control**; medical is **sickbay**; security is
  **armed**; the rest with no post are **casualty clearing**.
- **Battle stations** (`CallBattleStations`/`StandDownBattleStations`) set the condition red
  (canon: Voyager's general quarters is *code-blue stations*) and move each hand to their battle
  station; a plain `SetAlert(ALERT_RED)` still means "all hands to their own station", so the
  difference is demonstrable.
- **The emergency bills** (`CallEmergency`/`StandDownEmergency`) name a deck and muster it: a fire
  or a decompression clears the deck and sends the parties to it; an intruder sends security; and
  **abandon ship** musters the crew at the shuttlebay whatever the watch.
- **The damage-control organisation** (`EngineerOfTheWatch`).
- **The officer of the deck** (`OfficerOfTheDeck`, `OodMayOrder`, `OodRefusal`).

## The demonstration — the model's own output

`test_ship_core --bills`:

```
== the census: the watch bill is the roster, the bills are the organisation over it
  ALPHA WATCH 0800-1600: 60 on duty, 22 at a post, 40 off watch
  battle bill by role: own post 6; damage control 80; casualty clearing 15; sickbay 16; armed 24
  engineer of the watch: B'Elanna Torres
  officer of the deck: Tom Paris
  the holes: 0 on a full crew (a full crew is three-deep); the read names what loss makes
  with all but one transporter hand lost: 1 hole(s): transporters rests on one hand (Crewman 053);

== battle stations: called, observed, stood down
  before: warp drive manned 3, condition 0, battle 0
  called: condition 2, battle 1; warp drive manned 0, sensors repairing 3
  stood down: condition 0, battle 0; warp drive manned 3

== the emergency bills: a fire clears its deck; abandon ship musters the crew
  fire on deck 9: 4 were on it, 0 non-party hand(s) left; the fire bill: damage control fights it on deck 9; everyone else leaves the deck
  abandon ship: 130 of 141 fit hands muster at the shuttlebay (deck 10)

== the officer of the deck: authority over seniors, not the captain
  the deck is Tom Paris (rank 3)
  may order Tuvok (rank 4): yes
  may order the captain Kathryn Janeway: the captain is not the deck's to order
  but the deck cannot operate the engine room: MayOperate 0
```

Read as evidence rather than a screenshot, the lines are the acceptance:

- **the watch bill** — the watch in force, who is on duty and who is at a post (the roster, read as
  a bill);
- **battle stations, called** — `condition 2` is red, `battle 1` is called, and the **warp drive's
  own hands fall from 3 to 0** while **3 hands repair the sensors**: the hands have left their posts
  for their battle stations. **Stood down**, the drive is manned 3 again and the condition is green;
- **the emergency bills** — the fire leaves **0 non-party hands** on deck 9, and **abandon ship
  musters 130 of 141** fit hands at the shuttlebay;
- **the officer of the deck** — the deck **may order Tuvok (rank 4)**, **may not order the captain**,
  and **cannot operate the engine room** (`MayOperate 0`).

The crew's recorded state is what changes: `activity` and `deck` per record in `UpdateCrew`
(`TestTheBills` asserts the casualty-clearing and sickbay decks hold the right hands), and the
`battle`/`emergency` bytes on the record.

## The player's surface

**`ship bill [status]`** prints the same census and state to the console, and
**`ship bill battle|standdown|fire <deck>|decomp <deck>|intruder <deck>|abandon`** calls and stands
them down — command's, or the bridge watch's (a lieutenant at Engineering or Tactical), refused by
name to anyone else, exactly as `ship alert` is.

**The screen `ui_lwh_bills`** (`B` at the command console, or the console command `ui_lwh_bills`)
draws, from the module's published state, what a player reads and sets:

- `lwh_ship_bill_watch` — the watch in force and the counts (the watch bill);
- `lwh_ship_bill_battle_state` / `lwh_ship_bill_battle` — `GENERAL QUARTERS` or `the watch`, and the
  roster grouped by battle station;
- `lwh_ship_bill_emergency` — what the called emergency bill does, or `none called`;
- `lwh_ship_bill_holes` — the count and the names (wrapped, never clipped: `LWH_WrapText`);
- `lwh_ship_bill_ood` / `lwh_ship_bill_eow` — who holds the deck and the watch;
- the keys `B` battle, `X` stand down, `F` fire, `D` decompression, `I` intruder, `A` abandon, with
  a deck under `UP`/`DOWN`; and the ship's own refusal in `lwh_ship_refusal`.

The screen sends `ship bill …`, the same commands the console answers, so the surface and the
transcript cannot disagree. **Which keys open which screen is what a hand presses**, driven in the
harness by `lwh_bills_key` as `lwh_jobs_key` and `lwh_chart_key` are.

## The officer of the deck, demonstrated both ways — and the fork

**May:** under their watch the officer of the deck may give an operational order to anyone aboard,
**including officers senior to them who are not the captain** (`OodMayOrder` true, above: the deck is
a lieutenant, rank 3, and orders Tuvok, rank 4).

**May not:** order the captain (`"the captain is not the deck's to order"`), order themselves, or
give an order as a person who is not fit or not on watch.

**The fork, said rather than smoothed over.** `docs/crew-manifest.md` puts the deck's authority
*over the ship's operation for that watch, including over officers senior to them*.
`docs/access-and-authority.md` says **rank controls access absolutely**, and its scope ladder gives
`post` access to everyone below a department head. The brief for this pass called the deck's
authority *an access rule*. **It is not one, and the two documents do not in fact contradict** —
because `docs/access-and-authority.md` separates the two axes itself (*"Access is technical… Authority
is social"*). The resolution implemented, and recorded here as ours so it can be overruled:

> **The officer of the deck's authority is authority, not access.** The deck may *order* a senior
> officer; it gains no *clearance* over their station. In the run above the deck orders Tuvok and
> `MayOperate(deck, STN_ENGINEERING)` is 0. The alternative — extending the deck's access to match
> its authority (lock-outs, remote call-up over seniors) — is the fork, and it is **not** taken: it
> would make a lieutenant's console open a commander's systems, which the rank-derived rule forbids.

## Falsification — break the mechanism, and the check catches it

`scripts/bills-check.sh` compiles the unit test against a **broken copy** of `ship_core.cpp` in which
the battle bill's effect is disabled (`if (a == ACT_ON_DUTY && s.battleStations)` → `if (false)`) and
requires the test to **fail** on the bills; it then requires the tree to pass. A guard that cannot
fail for the reason we care about is decoration, so the check proves the test has teeth:

```
==> bills: the falsification (the battle bill's effect disabled, the test must fail)
    FAIL the bills: called, observed, stood down: s.systems[SYS_WARP_DRIVE].manned < driveBefore
    the broken copy fails as required; the tree passes
```

The same script asserts each transcript line above (the drive falling and returning, the cleared
deck, the muster count, and the deck ordering a senior and not the captain), so a silent regression
in any of them fails the check.

## The save

The save format moves: **version 58 → 59** (`SAVE_VERSION`, `module/ship/ship_core.h`), because the
bills are state the simulation holds. The cost, named: a per-record `uint8_t battle` and
`uint8_t emergency` (2 bytes × 141 = 282 bytes), and the called state — `battleStations`,
`battleCalledBy`, `battleAt`, `emergency`, `emergencyDeck`, `emergencyCalledBy`, `emergencyAt`
(23 bytes). **An older save does not load**, as every bump has invalidated them; `TestTheBills`
round-trips the called bills and the per-record bytes byte-for-byte (`Pack(back) == blob`).

## Judgement calls, named as calls

1. **The department-to-battle-role mapping is ours.** `docs/crew-manifest.md` names the *examples*
   (the kitchen to casualty clearing, the labs to damage control, the Doctor's staff triage, the
   shuttlebay arms for boarding) over nine departments; the simulation has five
   (`DEPT_COMMAND`, `DEPT_ENGINEERING`, `DEPT_SECURITY`, `DEPT_SCIENCES`, `DEPT_MEDICAL`). The mapping
   in `BattleRoleFor` is ours: engineering and the sciences to damage control, medical to sickbay,
   security to armed, and the unposted command hands to casualty clearing. The command crew
   (`rank >= 4`) hold the bridge, so the captain is not sent to a stretcher party.
2. **Casualty clearing is the mess hall** (`MESS_DECK`, deck 2), chosen as the ship's largest
   mustering space; the manifest names the function, not the room.
3. **The officer of the deck is a lieutenant or lieutenant commander, of the command department, on
   duty.** The manifest says "lieutenant or above"; the captain and first officer *command the ship*,
   so they are not the deck while anyone junior can hold it, and a lieutenant holds the deck while a
   lieutenant commander sleeps. This is a call; the restriction is one line to change.
4. **The OOD's authority is authority, not access** (the fork above).
5. **The holes are measured under loss, and said so.** The sim's roster is **three-deep by
   construction** — every post is filled on every watch — so on a full crew the read returns **0
   holes**, and it names them only once attrition thins a post (the transcript shows the transporter
   becoming one hand's when the rest are lost). The manifest's *single-specialist* hole ("the only
   transporter chief aboard") is not reproduced by the post model, which assigns by station rather
   than by credential; the read is honest about the model it measures rather than inventing a hole.
6. **Battle stations sets the condition red.** The manifest equates general quarters with code-blue
   stations; the model ties the bill to the alert so the panel and the consoles show it, while a
   plain red alert remains "all hands to their own station".
7. **The screen is reached by `B` at the command console** (the next free key beside `O`/`J`/`A`/`M`),
   and by the console command `ui_lwh_bills`; `lwh_bills_key` drives it as the other screens are
   driven.

## What could not be verified

- **Whether calling battle stations *feels* like command.** That is the owner's walkthrough; no
  harness can answer it. A transcript proves the hands moved and the state changed, not that the
  moment lands.
- **The rendered frame** of `ui_lwh_bills`. The static host runs no engine and has no game data, so
  the screen was compiled (both `libefgame.so` and `libefui.so` build) but not drawn here. The pixel
  check is the playtest host's, exactly as every other screen's is; the surface is named in the
  acceptance above with the cvars it reads and the keys it sends.
- **The console verb in the running engine.** `ship bill` is in the dispatch and the API
  (`scripts/api-check.sh` holds the two together), and the module builds, but it was not driven
  through `Svcmd_Ship_f` on this host.

## Register corrections made in the same pass

- **`docs/omissions.md` O6** — corrected in place, the finding kept, and given its marker
  (`state=closed marker=check:scripts/bills-check.sh`), so the register check watches it.
- **`docs/gates.md`** — the bills registered under the ship programme (*The bills — who does what
  when it goes wrong*), with the save version and the acceptance.
