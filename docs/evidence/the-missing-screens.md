# Evidence: the missing screens, the inventory completed (2026-10-07)

Task A of the owner's brief of 2026-10-07: complete the screen inventory, then build what is missing.
The audit that produced the seventeen (`docs/evidence/access-and-authority.md`, *Task B*) enumerated the
screens that were implemented. **That was its blind spot: a screen nobody built cannot be found by
reading the code.** This document sweeps the design corpus for the surfaces it *names*, reconciles each
against the seventeen already judged and the `lwh_ship_*` names the module publishes, and then builds
the outright absences.

Reproduce the headless parts with:

```
scripts/test.sh
scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts
scripts/personal-log-check.sh
scripts/screens-check.sh
scripts/s2-check.sh ; scripts/s4-check.sh ; scripts/gaps-check.sh ; scripts/shuttle-check.sh ; scripts/clearance-check.sh
```

The rendered screens are under `build/g3-home/baseEF/screenshots/`. The accessibility numbers are
computed from the engine's own palette (`upstream/efgame/src/game/q_math.cpp`).

## Task A -- the definitive inventory

One row per screen, board, menu, readout, panel or terminal the corpus **names**, with the document
that asks for it, the verdict (**has one** / **thin** / **missing**), and where the evidence is. The
seventeen from the prior audit are rows 1-17; the rest are what the sweep found that the code-reading
audit could not.

| # | surface | asked for by | verdict | where |
|---|---|---|---|---|
| 1 | Engineering systems list / power distribution | `ship-systems.md` (tier 1), `damage-and-budgets.md` | has one | `ui_lwh_engineering.cpp` stn 0; `lwh_engineering.tga`; `scripts/s2-check.sh` |
| 2 | Tactical console | `ship-systems.md` (tier 2), `scenario-atlas.md` | has one (**thickened**, see C) | stn 1; `lwh_tactical.tga`; `scripts/s4-check.sh`, `scripts/screens-check.sh` |
| 3 | Operations console | `ship-systems.md`, `scenario-atlas.md` | has one | stn 2; `lwh_ops.tga`; `scripts/s4-check.sh` |
| 4 | Conn console | `navigation-counter.md`, `scenario-atlas.md` | has one (single-type, see the axes) | stn 3; `lwh_conn_single.tga`; `scripts/screens-check.sh` |
| 5 | Sickbay console | `ship-systems.md` (tier 2), `gap-triage-and-sickbay.md` | has one | stn 4; `lwh_sickbay.tga`; `scripts/s4-check.sh` |
| 6 | Breach puzzle | `borg-incursion.md` | has one | `ui_lwh_engineering.cpp`; `lwh_breach.tga`; `scripts/s5-check.sh` |
| 7 | Triage board | `gap-triage-and-sickbay.md` | has one (**thickened**, see C) | `ui_lwh_triage`; `lwh_triage_*.tga`; `scripts/screens-check.sh` |
| 8 | Official log read | `gap-the-log.md`, `the-record-and-the-log.md` | has one | `ui_lwh_log`; `lwh_log.tga`; `scripts/gaps-check.sh` |
| 9 | **Personal log** | `the-record-and-the-log.md`, `gates.md` G2 (*Personal Log* menu) | **was missing; built** (see B) | `ui_lwh_personal`; `lwh_personal.tga`; `scripts/personal-log-check.sh` |
| 10 | Command console (standing orders, write-offs) | `damage-and-budgets.md`, `scenario-atlas.md` | has one | `ui_lwh_command`; `lwh_command.tga`, `lwh_access.tga`; `scripts/clearance-check.sh` |
| 11 | Personnel / character screen | `start-states.md`, `access-and-authority.md` | has one | `ui_lwh_character`; `lwh_personnel.tga` |
| 12 | The glance (in-world readout) | `scenario-atlas.md` ("what you can see"), `access-and-authority.md` | **thin** -- a read at the panel, no control | `lwh_glance.tga`; `scripts/s4-glance-check.sh` |
| 13 | The painted panel surface | `asset-doctrine.md`, `ship-master-map.md` | has one | `lwh_panel.tga`; `scripts/s4-panel-check.sh` |
| 14 | The viewscreen | `scenario-atlas.md` (the outside) | has one | `lwh_viewscreen.tga`; `scripts/viewscreen-check.sh` |
| 15 | The alert state | `ship-systems.md`, `failure-is-content.md` | has one | every console header; `scripts/condition-check.sh` |
| 16 | The navigation counter | `navigation-counter.md` | has one | every console; `scripts/nav-check.sh` |
| 17 | The turbolift deck menu | `ship-systems.md` (movement), `ship-master-map.md` | has one (retail menu, our list) | `lwh_turbolift.tga`; `ui_lwh_turbolift` |
| 18 | **Shuttle load-screen** | `shuttles.md` ("Not built: the load-screen") | **missing** -- and it is a *menu*, not a station screen (see below) | model: `LaunchShuttle`; check: `scripts/shuttle-check.sh` |
| 19 | Damage-control board / the job queue as a face | `damage-and-budgets.md` (the board must be operable in real time), `crew-work.md` ("the queue needs a board") | **thin** -- the queue exists (`Jobs`) and shows as the Engineering list and the command console's GIVEN UP, but there is no board of the queue itself | model `Jobs`; `lwh_engineering.tga`, `lwh_command.tga` |
| 20 | The month report screen (draft, edit, strike, sign) | `the-record-and-the-log.md` (the month report is the period beat) | **missing** -- the model is complete (`DraftReport`/`StrikeReportLine`/`SignReport`), the *editing screen* is not built | model `MonthReport`; no screen |
| 21 | The beacon-choice screen (hail / trade / distress / run) | `exploration-and-science.md`, `scenario-atlas.md` | **thin** -- the choices are console commands (`ship hail\|trade\|distress\|run`) with no key on any console | `ui_lwh_engineering.cpp` Ops footer has no hail key; `scripts/s4-check.sh` drives them by command |
| 22 | The tricorder / away-kit readout | `gap-tricorders-and-kit.md`, `exploration-and-science.md` | **thin** -- `ship scan`/`ship scancomp` print; the Ops console shows only the kit line, not a survey screen | `ui_lwh_engineering.cpp` stn 2; `scripts/gaps-check.sh` |
| 23 | The star chart / forecasts | `navigation-counter.md`, `exploration-and-science.md` | **thin** -- a chart line on the Conn and forecasts on the command console; no chart you can work | `lwh_ops.tga`, `lwh_command.tga` |
| 24 | The wall of names / sealed quarters | `morale.md`, `the-record-and-the-log.md` | has one (on the personnel screen) | `lwh_personnel.tga` |
| 25 | The Medical Log / Disease Library / visit roster (Virtual Voyager menus) | `program-proposal-v2.md` §4, `gates.md` G2 | **thin, now a read layer** on the Sickbay console (see the portfolios) | `lwh_sickbay.tga`; `scripts/screens-check.sh` |
| 26 | Astrometrics / Cargo / Recipes / Social Calendar / Library / Engineering Library (Virtual Voyager menus) | `program-proposal-v2.md` §4, `gates.md` G2 | **retail content, reconciled**: the game ships these menus; our consoles carry the ones the posts need (sensors on Tactical, the medical record on Sickbay) and leave the rest to the retail Virtual Voyager menus | `docs/lore-ledger.md`, the portfolios |
| 27 | Shooting Range / Weapon Library (Virtual Voyager menus) | `program-proposal-v2.md` §4, `gates.md` G2 | retail Holomatch content, not a ship console; no ship-side screen | G2 evidence |
| 28 | The dead officer's credentials | `access-and-authority.md` (reported, not built, line 147) | **not a screen** -- a scene, and it is reported rather than built | `docs/access-and-authority.md` |
| 29 | The two exits (left standing / shut down) | `ship-model.md`, `the-record-and-the-log.md` | **no screen** -- the exits are console acts and a record mark, not a surface | model `Suspend`/`LeftStanding` |

**The three known candidates, reconciled.** The **personal log** was the one outright absence of the
seventeen and is now built (B). The **shuttle load-screen** is a **menu**, not a station screen: it is
the manifest/loadout/confidence decision at the moment of launch, and it is treated as its own thing
(D). The item the brief expected near `docs/access-and-authority.md` line 147 is the **dead officer's
credentials**, which is *reported, not built*, and is not a screen at all. The sweep found the others in
rows 19-23 above: the **job-queue board**, the **month report editor**, the **beacon-choice controls**,
the **tricorder survey** and the **star chart**.

## Task B -- what was built

### The personal log screen (the outright absence)

The store existed (`WritePersonalLog`), the privacy rule was implemented and tested
(`PersonalVisibleTo`, `PersonalLog`; `TestTheTwoLogs`), and there was nowhere to read it. It is half the
record: the place the truth goes when it cannot go in the report (`docs/the-record-and-the-log.md`).

- **`ui_lwh_personal`** -- a new screen in `module/ui/ui_lwh_engineering.cpp`, drawn from `lwh_ship_personal`,
  which the module publishes from `ship::PersonalLog( vessel, vessel.player )` -- **one owner's entries and
  nobody else's**. The screen cannot show another person's words because the model never returns them.
- **Reached from where a console reaches the others.** The log terminal's panel opens `ui_lwh_log`; `P`
  on that screen opens the personal log, and `L` returns. `ui_lwh_personal` is also a console command.
- **Private.** `scripts/personal-log-check.sh` drives the screen: the player writes a private entry, the
  owner's read has it, **the official read of every scope does not**, and **another person's read is
  empty**. The unit test `TestTheTwoLogs` proves the store-level rule.

```
    personal test: Reyes wrote a private entry; the owner's read has 1
    personal test: official read (all scopes) contains it: no
    personal test: another person's read has 0
    personal test: the private screen reads "D0 08:01|I did not put this in the report;" for Reyes
PASS  the personal log screen exists, is private, and reads only its owner's store
```

### The station portfolios, and the two axes (the owner's additions)

The owner's two additions of 2026-10-07 are implemented and recorded in `docs/access-and-authority.md`
(rulings) and `docs/lore-ledger.md` (sourcing, per portfolio):

- **Reading and operating are different privileges.** A system is operated from exactly one station
  (`OperatedFrom`, the S4 invariant); another station may **read** it (`StationReads`) and cannot change
  it. The console draws its own systems as **controls** (a priority number, a switch) and the rest as
  **readouts** (tagged `RD`, in the read colour, with `ENTER`/priority doing nothing).
- **The station shapes the console; the person filters it.** Multi-content is *earned by the job*:
  **Tactical** (the sensor picture internal and external, and the comms traffic -- Operations speaks) and
  **Sickbay** (the medical record and what the Doctor is running) carry a read layer. **Engineering,
  Operations and the Conn carry one content type each** and publish no read layer. And from **every**
  console the player reaches what their clearance opens: the remote call-up, generalised.

```
    axes test: reads for Tactical "computer core|sensors|communications", Ops "", Sickbay "life support|computer core"; Operations' portfolio ""
    remote call-up: sensors operated from CONN
    axes test: Conn reaches sensors: yes, operated (refusal "")
    axes test: an uncleared ensign at Conn: "you are not cleared for CONN; the Conn officer or a lieutenant commander may open it, or a delegation for the shift"
PASS  the station decides the console's content types; the person decides what they reach, from any console
```

The single-type console demonstrated is the **Conn** (`lwh_conn_single.tga`): four systems, no `RD` rows,
no portfolio line -- and it still reaches sensors for a commander and still refuses an ensign. **Why it
stays single-type:** the helm's job is flying, one content type; the sensor and comms picture belongs to
Tactical and Operations, and putting a menu on the flight console is the swiss-army failure the ruling
names.

## Task C -- the two screens thickened, and why these two

The audit's gamification findings were: *Tactical is inert with no contact*; *Sickbay is thin without
casualties*; *the triage board reports and cannot act*; *the glance has no in-world control*. The test
is whether a screen offers a **decision** or only a **readout**. Measured by that:

- **Tactical** offered no decision with no contact: *literally inert*. Fixed.
- **The triage board** *reports and cannot act*: a readout, no decision. Fixed.
- **Sickbay** already offers the surgical field (one action) and is *thin*, not decisionless.
- **The glance** has no in-world control but is engine/world work, out of the console scope of this
  brief and of the world-rendering accessibility finding the prior audit already parked.

So the two worst are **Tactical** and the **triage board**, and those are the two fixed.

### Tactical -- the phaser bank's setting

The decision the console carries when there is no contact. `docs/ship-systems.md` tier 2 already names
"a power setting from stun to vaporize"; it is now a standing decision with a cost:

- `stores.phaserYield` (`YIELD_STUN`/`HEAVY_STUN`/`KILL`/`VAPORIZE`), saved (version 49, round-tripped).
- It **asks more of the power budget**: 100 / 125 / 150 / 175 per cent of the bank's nominal demand
  (`EffectiveDemand`), so a vaporize setting competes with shields for EPS.
- It **does more to a hull**: 0.4 / 0.7 / 1.0 / 1.35 times the damage (`UpdateOutside`).
- `V` on the Tactical console cycles it; the console shows `PHASER BANK VAPORIZE   POWER 174%`.

```
    tactical test: phaser bank VAPORIZE, demand 262 of 150 (174%)
    tactical test: stun asks 150, vaporize asks 262; vaporize asks 112 more
PASS  Tactical offers a real decision with no contact -- the phaser setting -- and it changes the power budget
```

And the unit test proves the effect, not just the label:
`hull after a minute: vaporize 0.858, kill 0.917, stun 1.000` (`TestPhaserYieldAndReading`).

### The triage board -- it acts

It read the ward and nothing else. Now it has a cursor over the ward and Sickbay's own decisions:

- `B` raise/lower the surgical field; `E` call the Doctor (the EMH); `R` **recover a captive** in the
  Borg window (the most loaded medical decision, and the board was where it could not be made); and
  `T` the triage order -- *which is command's*: the board offers it and the ship judges (the two-lock
  model, not a special case).

```
    triage test: surgical field 1, EMH 1, triage order 1, ward 4, recovery window 0
PASS  the triage board now acts: the surgical field, the Doctor, a captive recovered, and the order (command's)
```

## Task D -- the shuttle load-screen: a menu, said so

`docs/shuttles.md` records the load-screen as not built, and asks it to "make you decide something" --
the manifest (which posts empty), the loadout out of stores, the destination with its chart confidence,
the return condition, the risk: "a pre-flight check, not a menu." It is a **decision menu at the moment
of launch**, not a station console, and it is **not built here**. Calling it a menu rather than forcing
it into the console pattern is the point: it is a distinct thing, and the model behind it
(`LaunchShuttle` with its manifest and cargo, `scripts/shuttle-check.sh`) already exists. This is named
as a judgement call below.

## Measured accessibility numbers

Contrast is WCAG 2.x on the engine's palette against the black console background (and against the
selected-row fill `CT_DKPURPLE2`):

| colour | used for | vs black | vs selected fill |
|---|---|---|---|
| `CT_LTGOLD1` | value text, headers | 13.94:1 PASS | 4.23:1 PASS |
| `CT_LTPURPLE1` | footer/hint text | 7.36:1 PASS | 2.24:1 FAIL (footer never sits on the fill) |
| `CT_LTORANGE` | column labels | 7.12:1 PASS | 2.16:1 FAIL (labels never sit on the fill) |
| `CT_WHITE` | selected row, body | 21.00:1 PASS | 6.38:1 PASS |
| `CT_RED` | refusals, alarms, the recovery window | 5.25:1 PASS | 1.59:1 FAIL (refusals sit on black) |
| **`CT_LTBLUE2`** | **body/secondary, reads, the portfolio, the title band** | **7.25:1 PASS** | **2.20:1 FAIL (the read tags sit on black, not the fill)** |
| `CT_LTBLUE1` (replaced in the prior audit) | -- | 2.97:1 FAIL | 1.11:1 FAIL |
| `CT_DKGREY` | disabled rows | 2.03:1 FAIL (deliberately dim, `OFF` only) | 1.63:1 |

Colour-blind safety is unchanged and now covers the new tags: **every new signal carries a word, not
only a colour** -- a read is the word `(read)` and the tag `RD`, not merely blue; the recovery window is
the words `RECOVERY WINDOW` and `taken N%`, not only red; the phaser setting is the word `VAPORIZE`,
not only gold. The alert-red neighbours remain close (`CT_RED` vs `CT_LTORANGE` 1.36:1, vs
`CT_LTPURPLE1` 1.40:1), which is why the word always accompanies it.

Type height, measured from the rendered 1280×1024 screenshots (2× the 640×480 UI space): **TINYFONT ≈
9 virtual px (19 px rendered)**; **SMALLFONT ≈ 15-16 virtual px (30-32 px rendered)**. The new screens
use the existing fonts only; nothing new is smaller than TINYFONT.

## Before and after

Every screen change is photographed. The "before" images are the prior build's own render (kept from
`build/g3-home` before the change); the "after" are regenerated.

| screen | before | after |
|---|---|---|
| Tactical (the phaser setting) | `lwh_tactical_before.tga` (prior build) | `lwh_tactical_after.tga`, `lwh_tactical.tga` |
| Triage board (it acts) | `lwh_triage_before.tga` (prior build) | `lwh_triage_after.tga` |
| Personal log (new screen) | `lwh_log.tga` -- the surface it left, which had no personal affordance | `lwh_personal.tga` |
| Conn (single-type, the axes) | unchanged shape (no read layer) | `lwh_conn_single.tga` |
| Sickbay (portfolio + reads) | the prior `lwh_sickbay.tga` | `lwh_sickbay.tga` with `RD` rows and the `READS` portfolio line |
| Ops / Engineering (own systems) | unchanged shape (single-type) | `lwh_ops.tga`, `lwh_engineering.tga` |

The personal-log "before" is an absence, and the honest photograph of an absence is the screen it was
reached from. The Conn and the Ops/Engineering screens are shown as "after" with no read layer, which is
the single-type ruling rendered.

## What was proven, in the real engine

```
==> test.sh: all checks passed
==> check.sh: EXIT 0
==> personal-log-check.sh: PASS  the personal log screen exists, is private, and reads only its owner's store
==> screens-check.sh: PASS  the two screens offer a real decision, and the two axes hold
==> s2-check.sh: PASS  the reloaded ship matches the saved one, system by system  (save version 49)
==> s4-check.sh: PASS ... (tactical, clearance, panels, station purposes, non-station panels)
==> gaps-check.sh: PASS  the tricorder reads a compartment, the surgical field holds, ...
==> shuttle-check.sh: PASS  the ship knows where each shuttle is
==> clearance-check.sh: PASS  a locked control names who can open it (MAIN ENGINEERING, the Chief Engineer)
```

## What could not be verified

**Whether any screen is pleasant to operate.** That is the owner's walkthrough, and the only check that
settles *fun*. A screenshot proves a colour renders and a layout fits; it does not prove the console is
good to operate. The swiss-army failure the owner names is likewise a judgement about feel: I can show
the Conn publishes no read layer, not that it feels right.

## Judgement calls, named as calls

1. **The two screens to thicken: Tactical and the triage board.** The brief said pick the two *worst*
   by the decision-vs-readout test; Sickbay already offers an action and the glance is engine work. If
   the owner reads "worst" as including the glance, that is a different session (the world, not the
   console).
2. **The shuttle load-screen is a menu, left unbuilt.** `docs/shuttles.md` asks it to make you decide;
   the decision is real, but it is a launch-time menu, not a station console, and naming it as such is
   the honest answer rather than forcing it into the console pattern.
3. **The month report editor (row 20) and the job-queue board (row 19) are left built-in-model,
   missing-in-screen.** Both are genuine absences the sweep found. They are left because the brief's
   order is "the personal log first, then whatever else is genuinely missing, in order of what the game
   needs most," and after the personal log the two owner-approved additions (the portfolios and the two
   axes) were the larger, named need. They are recorded here as the next screens, not silently dropped.
4. **The phaser-yield ladder is ours.** `docs/ship-systems.md` names the setting; the percentages
   (100/125/150/175) and the damage factors (0.4/0.7/1.0/1.35) are invented and recorded in
   `docs/lore-ledger.md`.
5. **The portfolio contents.** The *names* are the game's own Virtual Voyager menus (game evidence at
   the top confidence tier). The *contents* are the ship's own state; the visit roster and the research
   line are marked invented in the ledger, and the sensor/comms items are marked recalled-episode. No
   technical-manual console layout is invented anywhere, per `docs/confidence-and-verification.md`.
6. **Save version 49**, for the phaser setting. One field, round-tripped byte-for-byte in
   `TestPhaserYieldAndReading`; older saves are invalidated as every version bump has invalidated them.
