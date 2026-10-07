# Evidence: access and authority, the consoles and their clearance (2026-10-07)

The owner's brief of 2026-10-07, and the owner's mid-flight addition: the clearance audit, its fixes,
remote call-up and the lock-out, and the screen audit on five axes. Reproduce the headless parts with
`scripts/clearance-check.sh` (one engine run), `scripts/test.sh` and `scripts/s4-check.sh`. The
accessibility numbers are measured with the palette in `upstream/efgame/src/game/q_math.cpp` and the
rendered console screenshots under `build/g3-home/baseEF/screenshots/`.

## What was proven, in the real engine

```
SHIP: clearance 1 (not cleared): "you are not cleared for MAIN ENGINEERING; the Chief Engineer or a lieutenant commander may open it, or a delegation for the shift"
SHIP: clearance 2 (delegated for the shift): sensors off
SHIP: clearance 3 (revoked): "you are not cleared for MAIN ENGINEERING; the Chief Engineer or a lieutenant commander may open it, or a delegation for the shift"
SHIP: clearance 4 (locked out): "LOCKED OUT: B'Elanna Torres has locked Reyes out of MAIN ENGINEERING"
SHIP: clearance 5 (override): "OVERRIDE PENDING AT MAIN ENGINEERING - TAKES IN 15 MIN"
PASS  a locked control names who can open it (MAIN ENGINEERING, the Chief Engineer)
PASS  a department head's delegation for the shift opened the station
PASS  the revocation took the access back, and the refusal is legible
PASS  the lock-out is logged and the console that stopped answering names both hands
PASS  the emergency override is begun, slow and logged, and a second officer agreed
```

The unit tests (`tests/ship`, `TestAccessAndAuthority`) additionally prove: access is unchanged at
morale 0 (access and authority are separate); a grant lapses when the shift's clock runs out; a
credential can be revoked; the two-person override takes in fifteen minutes and a solo force takes
forty-five and is recorded as one hand; a sliced system refuses the captain (`Hijacked`); and all of
delegations, the override and the lock-outs survive save and reload byte for byte (save version 48).

## Task A — the acceptance inventory, item by item

| item | verdict | the line that decides it |
|---|---|---|
| Access and authority are separate quantities | **built** | `ship_core.cpp` `MayOperate` (rank/credential/dept) vs `morale`; test sets morale 0 and the access is identical |
| A locked control is visible and named | **built** | `AccessRefusal` (`ship_core.cpp`), drawn by `ui_lwh_engineering.cpp` from `lwh_ship_refusal` |
| Same action refused at one post, permitted at another | **built** | `OperatedFromRefusal`; `g_ship.cpp` `PlayerMayOperate`; `s4-check.sh` |
| Delegation and revocation, both logged | **built** | `Delegate`, `RevokeDelegation`, `RevokeCredential`; log scope `crew` |
| Emergency override: slow, loud, logged, two people | **built** | `BeginOverride`, `ConfirmOverride`, `UpdateAccess`; fifteen vs forty-five minutes |
| The captain operates the whole ship alone, at a cost | **built** | rank >= 4 ship-wide; the cost is position (one console), preserved by remote call-up |
| Remote call-up; physical work needs presence | **built** | `MayCallUp`; the crew's jobs (`docs/crew-work.md`) are unchanged |
| Lock-out: logged, names its author, tells the person by name, marks their record | **built** | `LockOut`, `LockoutNotice`, `Remember(MEM_LOCKOUT, MEM_SAW, negative)` |
| A sliced system refuses everyone, including the captain | **built** | `Hijacked`; the `g_ship.cpp` switch/priority guard fires after clearance |
| A dead officer's credentials are still usable and still a decision | **absent — reported, not built** | `MayOperate` refuses a record that is not `CREW_FIT`; the silent use of a dead officer's authority was **withdrawn from the acceptance** and left to the memory/consent layer. Reason: it is a scene, not a clearance-table entry, and a silent grant would weaken the promise rather than keep it |

Two document promises were corrected rather than weakened:

- **The refusal text** changed from a generic "you are not cleared for this station" to a named one.
  The old text was a real defect: it told the player nothing and could not be improved without the
  model naming the opener (`docs/access-and-authority.md`, *What the console shows*).
- **The dead officer's credentials** bullet is withdrawn as stated and recorded as an open item.

## Owner addition — remote call-up, the lock-out, and location appropriateness

- **The conflict**, named and resolved in the document: full access was expensive because it meant
  walking to the system. Resolution: **command travels remotely; physical work does not.** A rank-4
  officer calls up a system from any console (`MayCallUp`, logged as a remote call-up), but a repair,
  seal, hatch or valve still needs a hand there; the bridge stays staffed because the officer is still
  *at a console* and can be at only one.
- **The lock-out** is a first-class act with a memory consequence: `LockOut` writes `MEM_LOCKOUT` with
  `MEM_SAW` and negative valence on the locked crew member's record, the log names the author, and
  `LockoutNotice` returns "B'Elanna Torres has locked Reyes out of MAIN ENGINEERING" for the console
  that has stopped answering.
- **Location appropriateness**: the panel-to-console table now carries a focus system (transporter →
  transporters, astrometrics → sensors, environmental → life support, Conn → deflector, sickbay →
  sickbay), so a console opens on what its location is for. Two entries in the simulation's own
  system table disagreed with `docs/ship-master-map.md` and were corrected: the **computer core** and
  the **torpedo launchers** are on deck 10 (the main core; the fore tubes), not deck 9.

## Task B — the screens, and the five axes

Enumerated by what the player sees, from the two UI files, the glance, and the painted panel.

| # | screen | gamification | fun | usability | accessibility | lore |
|---|---|---|---|---|---|---|
| 1 | Engineering systems list | decision: switch/prioritise/shed; the best decisions are which system to lose | the comfort of routine; legible at a glance | cursor and Enter always visible; one level deep | accent blue moved `LTBLUE1`→`LTBLUE2` (2.94→7.25:1); type 9px tiny / 16px small | names and figures trace to `ship-systems.md`; **fixed**: computer core and torpedo deck |
| 2 | Tactical console | fire/target/condition; a real choice (hull vs weapons) | holds when a contact is present; inert with none | same as 1 | as 1 | weapons/shields canon; tractor from Tactical [inv] |
| 3 | Operations console | beam/recall/survey/force field; the transporter is the decision | the risk statement before the act is the point | same as 1; **fixed**: opens focused on the transporter | as 1 | transporter deck 4 canon |
| 4 | Conn console | course and jump; the forecasts are command's | the navigation counter gives it the emotional centre | same as 1; opens on the deflector | as 1 | canon helm systems |
| 5 | Sickbay console | read the ward, raise the surgical field | thin without casualties | **fixed**: the seven-field ward line was overflowing; now `TINYFONT` | as 1 | beds 3+surgical [lore] |
| 6 | Breach puzzle | strong: a timed constrained sequence with two failure modes | yes — the best screen | buffer, trace bar, deadline all visible | red trace warning also carries the number | rules are the ship's (`MakeBreach`/`BreachScore`) |
| 7 | Triage board | a read of the order command set; no action here | thin: it reports | one level, scroll from the command console | white names on black; severity bar is colour + `%` | triage order matches `Patients` |
| 8 | Log read | a reader, not a controller (correct — the log is a document) | yes: the return surface | scroll visible; columns aligned | who-column moved to `LTBLUE2` | official vs personal distinction is canon |
| 9 | **Personal log** | — | — | **absent: model only, no screen** | — | the store exists (`WritePersonalLog`); the screen does not |
| 10 | Command console (standing orders) | decision: orders, promotion, write-off; the abandonment list is the player's signature | yes: it is where the player's hand shows | cursor and deck always visible; **new** ACCESS block shows override, lock-outs, delegations | as 1 | orders/write-off canon-grounded |
| 11 | Personnel / character screen | a decision (name, dept, rank) | one-off | clear cursor | as 1 | ranks capped below commander |
| 12 | The glance (in-world readout) | a read at the panel; no control in-world yet | the state follows you | anchors to the panel faced | `LTBLUE1`→`LTBLUE2`; the alert word as well as the colour | positions are reads, not snapshots |
| 13 | The painted panel surface | a read; no control | the ship's face shows its state | repeats on generated decks (world-UV authoring defect) | colours already light | day/watch/condition/power/nav — same model |
| 14 | The viewscreen (beside the glance) | a read; targeting is Tactical's | the image scales with hull | schematic, exact bars below | red frame + text label | enemy subsystem names from the model |
| 15 | The alert state | a decision (1/2/3) where permitted | the alarm is the sound | condition word in every header | state is carried by the **word** and the band, not colour alone | green/yellow/red canon |
| 16 | The navigation counter | a read; the forecasts are command's | the design's emotional centre | drawn on every console and the panel | `LTBLUE2` | `NAV_LIGHT_YEARS` = 75,000 [lore]; floors [inv] |
| 17 | The turbolift deck menu | a decision (which deck) | retail comfort | retail menu | retail | fifteen-deck list, ours |

Screens not named in the brief that were found and judged: the **viewscreen**, the **alert state**,
the **navigation counter**, and the **painted panel image** (a screen drawn on a surface, not a menu).

## Measured accessibility numbers

Contrast is WCAG 2.x on the engine's own palette (`q_math.cpp`) against the black console background:

| colour | used for | before | after |
|---|---|---|---|
| `CT_LTGOLD1` | value text, headers | 13.81:1 PASS | unchanged |
| `CT_LTPURPLE1` | footer/hint text | 7.35:1 PASS | unchanged |
| `CT_LTORANGE` | column labels | 7.09:1 PASS | unchanged |
| `CT_WHITE` | selected row | 21.00:1 PASS | unchanged |
| `CT_RED` | refusals, alarms | 5.25:1 PASS | unchanged |
| **`CT_LTBLUE1`** | **body/secondary text and the Ops/Sickbay title band** | **2.94:1 FAIL** | **replaced with `CT_LTBLUE2` = 7.25:1 PASS** |
| `CT_DKGREY` | disabled rows | 1.99:1 FAIL | unchanged (deliberately dim, `OFF` only) |

On the selected-row fill (`CT_DKPURPLE2`): the selected row draws `CT_WHITE` (6.43:1 PASS);
`CT_LTBLUE1` on the fill was 1.11:1 and `CT_LTPURPLE1` 2.25:1, both FAIL — another reason the body
blue was moved. Type height, measured from the rendered 1280×1024 console screenshot (2× the 640×480
space): **SMALLFONT ≈ 16 virtual px (32 px rendered); TINYFONT ≈ 9 virtual px (19 px rendered).**

Colour-blind safety: the alarm red against its neighbours is close — `CT_RED` vs `CT_LTORANGE`
1.35:1, vs `CT_LTPURPLE1` 1.40:1, vs `CT_LTGOLD1` 2.63:1 — so **state is now never carried by colour
alone**: every alert band also carries the word (`CONDITION RED`), every severity bar carries its
number, and the header text names the condition. The one place colour is the only signal is the
world's emergency strips, below.

## The world's emergency strips: an accessibility finding in the world

`docs/evidence/deck*-redress.md` and the screenshots (`lwh_emergency*.tga`,
`build/g3-home/baseEF/screenshots/`) show the same defect the deck series found on five decks: **the
red emergency strips toggle correctly and do not read as red from the camera** — with the strip on,
the wall shows a row of small mixed-colour pixels (`lwh_emergency.tga`), and with it off, nothing
(`lwh_emergency_off.tga`). The toggle is correct (the state changes); the *signal* is not perceptible.
This is an accessibility failure wearing a rendering finding's clothes, and it is **out of the console
scope** of this brief: fixing it is authoring an emissive red strip material in the deck maps, which
falls under the deck briefs and the no-new-art rule. It is reported, not changed here.

## Before and after

The accent-blue change and the Sickbay font change alter what a screen looks like, so both were
photographed.

- Before: `build/g3-home/baseEF/screenshots/lwh_engineering.tga` etc. as they stood before this
  change (copies kept at `/tmp/opencode/shots/` during the audit).
- After: regenerate with `scripts/s2-check.sh` (Engineering), `scripts/s4-check.sh` (Tactical,
  Operations, Sickbay), and the direct runs below.

The command console now also draws an ACCESS block (override / lock-outs / delegations) when any is
present, and the Engineering console draws the override or the lock-out line and offers `Y` for the
override.

## What could not be verified

**Whether any of it is pleasant to use.** That is the owner's walkthrough, and it is the only check
that settles *fun*. A screenshot proves a colour renders and a layout fits; it does not prove the
console is good to operate. That judgement is not made here.
