# Evidence — the navigation counter: how far, and how long

Date: 2026-10-06. Branch: `feat/the-navigation-counter`, cut from `feature/g3-reactive-crew` (read in
the working tree at `67b5790`). Implements `docs/navigation-counter.md` in full. Save format
**version 45**.

The document calls itself the emotional centre of the design. What existed was `NavigationCounter`,
which returned `DilithiumRange` — the fuel gauge — and a comment saying there was no second quantity.
There was a second quantity, and it was already in the save: the ship's position (`sector`,
`beacon`, `sectorNumber`, `SECTORS_TO_CROSS`) and the inputs to her effective speed (crystal
quality, engine health, crew, route). This branch is a **projection over that model**, not a new one,
and it removes the misnamed stand-in.

Nothing here is asserted without the command that produced it.

---

## What was built

**The read.** `Navigation NavigationCounter(const Ship &s)` (pure) returns:

- `distanceLy` — light years remaining. The crossing is `SECTORS_TO_CROSS × (SECTOR_BEACONS−1)` = 33
  forward jumps; the ship has made the sectors behind her plus the beacons reached in this one, and
  the goal beacon of the last sector is home. The total (75,000 ly) and the goal are canon; the step
  mapping is `[inv]`.
- `nominalYears` — that distance over canon's effective rate, 1,000 c: *what the journey costs if
  nothing changes*.
- `currentYears` — over the speed the ship can **actually sustain**: 1,000 c × crystal life ×
  drive output × crew at the post × route resupply, with each factor 1 on a whole ship and a
  researched crystal (`quality > 1`) the one factor allowed above 1.
- `changeYears` — `currentYears` since the counter was last written down: **the derivative**.
- `warp` — false when there is no crystal, no drive or no pylons: the honest answer is not a bigger
  number, it is that home stops getting closer.

**The write.** The counter is recorded in the log about weekly (`NAV_LOG_DAYS = 7 [inv]`), from
`Advance`; that record also moves the derivative's baseline, and signing a report does too. The log
is written by the simulation and never read by it.

**Where it shows.** The in-world panel (`lwh_panel.cpp`) and the HUD panel glance
(`lwh_panel_glance.cpp`), painted on the surface with the rest of the state; every station console
(`ui_lwh_engineering.cpp`, from `lwh_ship_nav`); the ready-room command console (`ui_lwh_command.cpp`),
which also carries the forecasts; and `ship nav` from any console. Command alone sees the forecasts —
the estimate under each available course (`NavigationForecasts`), the rank line the document draws.

**The report.** `DraftReport`'s headline is now the counter's change since the last entry, and
`MonthReport::counter`/`counterChange` are the estimated years, not the fuel range. The judgement
call in `docs/evidence/the-month-report-and-the-toll.md` §1 ("that is a new model, not this
function") is superseded: the model was already there.

---

## Observed

The projection, printed without the game (`test_ship_core --nav`):

```
$ cmake -S tests/ship -B /tmp/ship && cmake --build /tmp/ship -j && /tmp/ship/test_ship_core --nav
PASS  a new ship reads 75000 light years out: 75 years nominal, 77 at current capability (the gap is shown)
PASS  wreck the crystal: 77 -> 82 years at current capability
PASS  a researched better crystal: 70 years, against 75 nominal (the one positive loop)
PASS  no crystal: warp impossible; home stops getting closer (the failure is immobility, not death)
PASS  a jump toward home: 75000 -> 72727 light years
PASS  recorded in the log: "navigation: 72727 light years from home, 73 years nominal, 74 at current capability"
PASS  forecasts (command alone): beacon 0: 77 years; beacon 2: 72 years
PASS  the month report's headline: "home 75000 light years; 75 years nominal, 77 now, -23.5 since the last entry"
all demonstrations passed
```

The same read at the console inside the game, on deck 4 (`scripts/nav-check.sh`):

```
$ scripts/nav-check.sh
==> navigation counter
SHIP: nav test: the counter before anything
SHIP: 75000 light years from home; 75 years nominal, 77 at current capability
SHIP:   +0.0 years since the last entry; effective speed 980 c
SHIP:   forecast: beacon 1 uncharted (not yet charted); 74 years from there
SHIP: nav test: the crystal wrecked
SHIP: 75000 light years from home; 75 years nominal, 82 at current capability
SHIP:   +5.2 years since the last entry; effective speed 917 c
SHIP:   forecast: beacon 1 uncharted (not yet charted); 79 years from there
SHIP: nav test: after a jump toward home
SHIP: 72727 light years from home; 73 years nominal, 79 at current capability
SHIP:   +2.9 years since the last entry; effective speed 915 c
SHIP:   forecast: beacon 0 open space; 82 years from there
SHIP:   forecast: beacon 2 uncharted (not yet charted); 77 years from there
PASS  the counter reads the distance and both figures, worsens when the crystal is wrecked, falls when the ship jumps, and command sees the forecasts
```

The visible state, painted on the panel surface the player stands at (`scripts/s4-panel-check.sh`;
the generated deck 6 status screen, two screens in view). The line `NAV 75000LY 77YR` is the counter,
present without opening any screen:

```
$ scripts/s4-panel-check.sh
==> panel
SHIP: panel test: standing on deck 6 at (-4512 -3584 -19312) facing the screen
PASS  the ship's live state is painted onto the panel surface
      screenshot: /home/c/big/git/long-way-home/build/g3-home/baseEF/screenshots/lwh_panel.tga
```

The same read on the station and ready-room consoles, photographed
(`g_shipTest 14` for Operations, `g_shipTest 11` for the command console; the Ops screen shows
`NAV 75000 LY OUT 75 YR NOMINAL 77 YR NOW +0.0 SINCE LAST` below the system list, and the ready
room shows the same line plus `FORECASTS — THE ESTIMATE UNDER EACH COURSE / BEACON 1 UNCHARTED: 74
YR`). Main Engineering's console does not draw the line because all eighteen of its systems fill the
screen; the `ship nav` query, the panel and the HUD carry it there.

The unit tests (`TestNavigation`) additionally assert, and the whole `test_ship_core` run passes:
distance is the position model's (a jump is fewer light years; the last sector's goal beacon is
zero); a wrecked drive and a spent crystal lose the estimate; a researched crystal beats nominal;
the change is negative when the ship moves closer; the counter is recorded in the log over time; the
forecasts exist; and it survives a save and load byte-for-byte (`Pack(back) == blob`), with the
report's `counter` equal to the read and its headline naming the change.

### The three acceptance points the document names

- **It changes when the state changes**: the `nav-check.sh` transcript shows the same read worsen
  from 77 to 82 years the moment the crystal is wrecked.
- **It survives save and load, and is recorded in the log over time**: `TestNavigation` packs,
  unpacks and re-packs the ship byte-for-byte and checks `navCounterLast` and the read match;
  `--nav` prints the weekly log record.
- **Visible without a special mode, readable on every deck, the nominal/current gap displayed**: the
  panel screenshot, the `lwh_ship_nav` line on every station console, and both figures in every
  transcript above.

---

## The two required suites

```
$ scripts/test.sh ; echo $?
...
==> patches: numbered without gaps, and each one parses
    15 patches

all checks passed
0
```

```
$ scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts ; echo $?
...
==> compiler corpus pass (every shipped script: compile, then read back)
    2024 files: 2016 compiled and read back, 8 rejected by the compiler, 0 read-back failure(s)
      ... (all eight named) ...
    the 8 rejections are exactly the known, documented set
0
```

Both exit **0**. The module compiles: `make` in the configured upstream build produced `Built target
efui` and `Built target efgame` with no errors.

---

## Judgement calls, named

1. **The position model is the counter's source, not the fuel range.** `sectorNumber`, `beacon`,
   `SECTORS_TO_CROSS` and `SECTOR_BEACONS` already describe where the ship is; the counter projects
   light years from them (33 forward jumps over 75,000 ly `[inv]`) and never from `DilithiumRange`.
   The brief asked for this to be checked before any "new model" was accepted, and the check passed.
2. **A healthy ship reads a current figure slightly above nominal (77 vs 75), because no resupply is
   charted yet.** This is the document's "no refuelling site charted" factor applied honestly from
   turn one, and it is the gap the acceptance asks to be displayed. The four speed factors are
   invented `[inv]`; chosen so the document's own worked example (71,240 ly, 62% crystal, worn
   engine, no source → 76 years) reproduces to the year.
3. **No warp is `warp = false`, not a large number.** With no crystal or drive the counter says
   "home stops getting closer", the immobility the design names, rather than an infinity that would
   read like a value.
4. **The derivative is in years, and its baseline is the last time the counter was written down**
   (the weekly log record, or a signed report). The document's example change is `+1.8 years`, so
   years is the unit; a monthly report therefore shows the change since the last record.
5. **`changeYears` is clamped to the record, not the log.** The model never reads the log
   (`docs/the-record-and-the-log.md`'s law); the baseline is the saved scalar `navCounterLast`,
   which the periodic record updates.
6. **Save version 45.** No new bytes are stored, but `navCounterLast` and the report's
   `counter`/`counterChange` changed meaning (years, not light years), so a version-44 save is not
   silently misread; the version gate rejects it.
7. **The forecasts are per available course, one jump out**, carrying the distance from there and
   the years at current capability, with charted/hazard stated. The document's "fifty-four years if
   it works, ninety if it does not" wants a per-route risk split; the honest projection here gives
   the estimate from each course and names what is known to be there, and stops there.

## What could not be verified

- **The human check** — "a player who looks at it after a hard month should feel the arrow move."
  The arrow moves in the transcripts and a month is simulated in the tests, but whether it *feels*
  is the owner's judgement, as the document says.
- **The rank line at a table** — that a non-command operator is shown the distance and estimate but
  not the forecasts. The console path enforces it (`PlayerMayCommand` gates `NavigationForecasts`
  in `ship nav` and in `lwh_ship_forecast`), and the headless run exercised it as command; a second
  run as a junior post was not made.
- **The forecasts under a real multi-course chart** — the headless run saw the chain links of the
  opening sector, not a hand-built branching chart.
