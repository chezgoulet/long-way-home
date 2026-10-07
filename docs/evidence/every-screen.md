# Evidence: every screen missing or thin, built or thickened (2026-10-07)

The owner's second brief of 2026-10-07: work the inventory in `docs/evidence/the-missing-screens.md`
to the end -- **no row left missing, no row left thin, without a reason recorded** -- and build what
the list marks missing. The two outright absences it names are the **month report editor** (row 20)
and the **job-queue board** (row 19); the sweep also found three *thin* screens, the **beacon
choices** (row 21), the **tricorder survey** (row 22) and the **star chart** (row 23). All five are
addressed here; two rows (12, the glance; 18, the shuttle load-screen) are left with their reasons.

Order by what the game needs most: the two absences first (row 20 the report is the period beat and
row 19 the queue is the crew economy's face), then the three thin screens a post actually works
from (the beacon's choices, the survey, the chart).

Reproduce with:

```
scripts/every-screen-check.sh
scripts/test.sh
scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts
scripts/screens-check.sh ; scripts/s2-check.sh ; scripts/s4-check.sh ; scripts/gaps-check.sh
scripts/clearance-check.sh ; scripts/shuttle-check.sh
```

The rendered screens are under `build/g3-home/baseEF/screenshots/`. The accessibility numbers are
computed from the engine's own palette (`upstream/efgame/src/game/q_math.cpp`) by
`scripts/screens-a11y.py`.

## What was built

| row | screen | file | the decision it carries |
|---|---|---|---|
| 20 | **the month report editor** (`ui_lwh_report`) | `module/ui/ui_lwh_command.cpp` | strike a line (`S`), soften a number (`F`), sign it to the crew (`ENTER`) or file it upward (`U`) -- the lie and its direction, with the diff the record keeps drawn beside it |
| 19 | **the job-queue board** (`ui_lwh_jobs`) | `module/ui/ui_lwh_command.cpp` | command sets a job's place (`LEFT`/`RIGHT`) and orders a build (`B`) -- *priority is where rank lives* |
| 23 | **the chart you can work** (`ui_lwh_chart`) | `module/ui/ui_lwh_command.cpp` | where to make for (`UP`/`DOWN`, `ENTER`), with the forecast for each beacon one jump away |
| 22 | **the tricorder survey** (`ui_lwh_survey`) | `module/ui/ui_lwh_engineering.cpp` | what to spend a scan on -- the site the ship is at, or a compartment -- out of one shared charge, and a weak charge reads wrong |
| 21 | **the beacon's choices**, as keys | `module/ui/ui_lwh_engineering.cpp` | which channel to open at this site: `L` hail, `M` trade, `A` distress at Operations; the run is the Conn's (`X`) |

`module/ship/g_ship.cpp` publishes the state the screens read (`lwh_ship_report`,
`lwh_ship_report_diff`, `lwh_ship_jobs`, `lwh_ship_sector_map`, `lwh_ship_survey`,
`lwh_ship_survey_reading`, `lwh_ship_beacon`, `lwh_ship_may_command`) and carries the one new model
command, `ship job <index> <priority>`. The model gains `SetJobPriority` (`module/ship/ship_core.*`),
gated by `PlayerMayCommand` like the other command acts.

**No new art.** The screens use only the existing palette (`CT_LTGOLD1`, `CT_LTBLUE2`, `CT_WHITE`,
`CT_RED`, `CT_LTORANGE`, `CT_LTPURPLE1`, the `CT_DKPURPLE*` furniture) and the existing `TINYFONT` /
`SMALLFONT`. No save-version bump: nothing new is persisted (jobs and the report were already in the
save, and `SetJobPriority` only changes a field already written).

## The two-lock, demonstrated on the new screens

Every act of the report editor and the job board is gated by the ship's own `PlayerMayCommand` -- the
**person** axis of the two-lock model, not a clearance model of the screens'. A post officer (and a
created character never commands until command passes to them) opens the screen, sees the refusal by
name, and changes nothing; the same run then shows whoever commands editing the report and setting
the queue's order.

```
    report test: a post officer, may command 0
    report test: the post officer's strike was refused: "the report is the commanding officer's to write"; struck lines 0
    jobs test: the post officer's reorder was refused: "priority is command's to set"
    report test: in command, may command 1
    report test: in command, struck 1, softened 1; the diff is "- home 75000 light years; 75 years nominal, 77 now, +0.0 since the last entry- the crew stands at 141 of 141; 0 wounded, 0 lost, 0 assimilated+ the crew stands at 70 of 141; 0 wounded, 0 lost, 0 assimilated"
    report test: signed to the crew yes; a fresh draft is open yes
    jobs test: in command, 3 job(s), 1 build; the queue reads "0|repair|shields|44|5;1|seal|deck 9|20|20;2|build|10 spare parts|0|40;"
```

The rendered refusals are `lwh_report_locked.tga` ("REFUSED: THE REPORT IS THE COMMANDING OFFICER'S
TO WRITE") and `lwh_jobs_locked.tga` ("REFUSED: PRIORITY IS COMMAND'S TO SET"). The screen remains
**visible and named**, not hidden, which is the doctrine of `docs/access-and-authority.md`: a locked
control says what it needs and who can open it.

The station ruling is untouched. The survey is Operations' one content type (the sensors) and lives
on the Operations console; the beacon choices are Operations' because the ship already says the
hailing channels are; the chart and the report are command's, reached from the command console. No
new screen invents a content type, and every one of them still reaches what the person's clearance
opens.

## What was proven, in the real engine

```
==> every-screen-check.sh: PASS  a post officer opens both screens and every act on them is refused, by name, changing nothing
==> every-screen-check.sh: PASS  whoever commands strikes a line, softens a number, and signs it to the crew -- the record keeps the diff
==> every-screen-check.sh: PASS  whoever commands sets the queue's order and orders a build -- the board of the work, as its own face
==> every-screen-check.sh: PASS  the chart is workable: the sector, the forecast, and setting a course by key from the screen
==> every-screen-check.sh: PASS  the survey is a decision: a scan of the site or a compartment, out of one shared charge, and the reading returns
==> every-screen-check.sh: PASS  the beacon's choices are keys on Operations now, and the hail reaches the ship
==> every-screen-check.sh: PASS  every new screen was rendered and photographed
==> every-screen-check.sh: PASS  the missing screens are built, the thin ones thickened, and the two-lock and accessibility hold
==> test.sh:   all checks passed
==> check.sh:  EXIT 0
==> screens-check.sh, s2-check.sh, s4-check.sh, gaps-check.sh, clearance-check.sh, shuttle-check.sh: PASS
```

The model function behind the board is unit-tested too (`tests/ship`, `TestJobQueue`): a
non-commander's order is refused and changes nothing; command's place survives the tick's rebuild;
a job that does not exist is not a target.

## Measured accessibility numbers

Contrast is WCAG 2.x on the engine's own palette, each colour against the surface it **actually sits
on** -- the black console background, the selected-row fill `CT_DKPURPLE2`, or the title band a black
heading is drawn on. Computed by `scripts/screens-a11y.py --palette ../upstream/efgame/src/game/q_math.cpp`:

| colour | sits on | ratio | |
|---|---|---|---|
| `CT_LTGOLD1` | `CT_BLACK` | 13.94:1 | PASS |
| `CT_LTBLUE2` | `CT_BLACK` | 7.26:1 | PASS |
| `CT_LTPURPLE1` | `CT_BLACK` | 7.39:1 | PASS |
| `CT_LTORANGE` | `CT_BLACK` | 7.11:1 | PASS |
| `CT_WHITE` | `CT_BLACK` | 21.00:1 | PASS |
| `CT_RED` | `CT_BLACK` | 5.25:1 | PASS |
| `CT_WHITE` | `CT_DKPURPLE2` (selected row) | 6.39:1 | PASS |
| `CT_LTGOLD1` | `CT_DKPURPLE2` (selected row) | 4.24:1 | PASS |
| `CT_BLACK` | `CT_LTGOLD1` (title band) | 13.94:1 | PASS |
| `CT_BLACK` | `CT_LTBLUE2` (title band) | 7.26:1 | PASS |

Colour-blind safety is unchanged and holds for the new screens: every signal carries a **word**, not
only a colour. The report's edit state is the tag (`S` struck, `E` edited, `+` added) and the struck
line is drawn in `CT_RED` with its own diff line beneath; the refusal is the word `REFUSED`; the
queue's progress is the bar **and** the `%`; the survey's trouble state is the word (`NO AIR`,
`AFIRE`, `HULL BREACHED`).

Type height, measured on the pixels of the rendered screenshots (1280x1024; the horizontal scale is
2, the vertical scale is 1024/480 = 2.133):

```
    PASS lwh_report.tga: the SMALLFONT title on its band ink 31 px
    PASS lwh_jobs.tga:   the SMALLFONT title on its band ink 31 px
    PASS lwh_survey.tga: the SMALLFONT title on its band ink 32 px
    PASS lwh_report.tga: a TINYFONT label ink 21 px
    PASS lwh_jobs.tga:   a TINYFONT column label ink 20 px
    PASS lwh_survey.tga: a TINYFONT survey row ink 20 px
```

So SMALLFONT renders ≈31 px (≈15 virtual px) and TINYFONT ≈20 px (≈9 virtual px), the same fonts
the rest of the consoles use; nothing new is smaller than TINYFONT.

## Before and after

| screen | before | after |
|---|---|---|
| the month report editor | an absence -- no screen existed | `lwh_report.tga`, `lwh_report_locked.tga` |
| the job-queue board | the queue only inside `lwh_engineering.tga` and `lwh_command.tga` | `lwh_jobs.tga`, `lwh_jobs_locked.tga` |
| the chart | the chart line on the Conn (`lwh_conn_single.tga`) | `lwh_chart.tga` |
| the survey | the kit line on `lwh_ops.tga` | `lwh_survey.tga` |
| the beacon's choices | the Ops footer with no hail key (prior `lwh_ops.tga`, kept at `/tmp/opencode/shots/`) | `lwh_ops_beacon.tga` |

The "before" of the report editor is an absence, as the personal log's was; the honest photograph of
an absence is the console it would have been reached from, the command console (`lwh_command.tga`).

## What was left, and why

- **Row 12, the glance** -- a read at the in-world panel. The control it is missing is *in the
  world*: an interactive surface on the panel brush, which is engine/world work and outside the
  console scope of both briefs (the prior audit parked it with the world-rendering accessibility
  finding). It is a **readout by design**; the decision it lacks is a world interaction, not a
  console act. Recorded, not silently dropped.
- **Row 18, the shuttle load-screen** -- a **decision menu at the moment of launch**, not a station
  console. `docs/shuttles.md` asks it to make you decide (the manifest, the loadout, the
  destination's confidence, the return condition); the model (`LaunchShuttle`, with its manifest and
  cargo) exists, and naming it a menu rather than forcing it into the console pattern is the honest
  answer. Left, with the reason.

## What could not be verified

**Whether any screen is pleasant to operate.** That is the owner's walkthrough, and the only check
that settles *fun*. A screenshot proves a colour renders and a layout fits; it does not prove the
report editor is good to use, or that the queue board reads at a glance. The **glance's in-world
control** also remains unverified because it is unbuilt (row 12). And the screen-by-screen
comparison of what a *post* sees against what the *captain* sees (docs/crew-work.md: the same queue
reads differently by post) is a design intent the two-lock demonstrates mechanically but not
experientially.

## Judgement calls, named as calls

1. **The report editor is command's, and the command is gated.** `ship report strike|soften|edit|add|
   sign|file|purge` now checks `PlayerMayCommand`; reading the open draft and the diff stays open.
   This changes the developer command's behaviour from unrestricted to command-only. The call: the
   brief requires the two-lock on every new screen, and the report *is* the signer's -- the
   alternative was a clearance check living in the UI, which the rulings forbid. Existing unit tests
   call the core functions directly and are unaffected.
2. **The report's edit acts on the screen are strike, soften and sign.** A full text editor for a
   line (`EditReportLine`, `AddReportLine`) needs a typing surface the engine's menus do not offer;
   the decisions that matter -- what to remove, what number to soften, who reads it -- are all
   reachable by key. Typing remains at the developer command. If the owner wants free-text editing
   in-world, that is a larger UI affordance, this session is the keys.
3. **The queue's order is set one step at a time** (`LEFT` sooner, `RIGHT` later), not by a number
   typed in. It matches the console's own idiom (the Engineering list moves a system's priority the
   same way) and keeps "command sets the order" a hand act.
4. **The chart sets a course; it does not jump.** The jump is the Conn's (`J`/`K`/`L`, unchanged),
   and the chart is command's decision of *where*; the screen sends `ship course`. This keeps the
   station's own act with the station.
5. **The survey is Operations' single content type**, not a new console. It is the sensor/away-kit
   job, so it lives where the sensors are read (and `U` on the Ops console now opens the board, where
   it used to fire a survey blind). The ruling is unharmed: Operations still carries one content
   type.
6. **The beacon block names the choices but does not grey them by applicability.** The ship refuses
   a channel that does not apply here (Hail always answers, Trade needs a trader, Distress a
   distress call), and the block shows what the *kind* affords; the model remains the judge. A
   per-choice greyed state is available if the owner wants it.

## The second-pass verdicts (the inventory, rows 12-23)

| # | surface | verdict now |
|---|---|---|
| 12 | the glance | thin, left -- in-world control, out of console scope; a readout by design |
| 18 | shuttle load-screen | missing, left -- a launch decision menu, not a station screen |
| 19 | job-queue board | **built** (`ui_lwh_jobs`) |
| 20 | month report editor | **built** (`ui_lwh_report`) |
| 21 | beacon choices | **thickened** (keys on Operations; the Conn runs) |
| 22 | tricorder survey | **thickened** (`ui_lwh_survey`) |
| 23 | star chart | **thickened** (`ui_lwh_chart`) |

No row is left missing or thin without the reason above; every row changed states what it was, what
it now is, and the file that carries it.
