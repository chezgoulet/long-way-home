# Evidence: the configurator — the player chooses the start state

Date: 2026-10-08. Branch `feat/the-configurator`, cut from `testing`. This is the second half of the
walkthrough's **G2** and the whole of `docs/the-entry-point.md` **Part three**: the entry point is a
**configurator**, not a preset list, and every mechanism built for the character layer becomes
reachable through it. The design's rule is the shape of the work: *"The menu grants the situation.
The fiction supplies the reason. The simulation then holds you to it."* (`docs/start-states.md`).

**Observed, and the command that produced it.** Everything below is the output of a command named
beside it. The unit tests are in `scripts/test.sh`; the acceptance the owner judges is printed by
`scripts/configurator-check.sh`; the rendered path was driven under `xvfb-run` with the engine's own
screenshots.

## What is built, and where

- **The mechanism, `module/ship/ship_core.{h,cpp}`.** A start state is data: the `StartState` struct
  (the four dimensions), the shipped `STARTS` list (`StartStateCount` / `StartStateAt`: **CANON**,
  **THE CHAIR**, **ALL-FICTITIOUS**), the command seats (`SeatName` … `SeatForType`), the player's
  seat occupancy (`Ship::seatHolder`, saved), the career path (`Ship::career`, saved), and
  **`ApplyStartState`** / **`FillVacancies`** / **`WriteLogSeed`**. It went beside `CrewMember` and
  `BuildRoster` for the reason `docs/evidence/the-character-layer.md` records: the module build finds
  sources with a configure-time glob, so a new `.cpp` is not compiled until cmake re-configures.
- **The surface, `module/ui/ui_lwh_start.cpp`.** The "Long Way Home" line still opens the start
  screen; the screen is now the four dimensions over the `STARTS[]` list. It decides nothing about
  the ship: it writes the run's cvars (`g_shipConfigured`, `g_shipFictitious`, `g_shipCareer`,
  `g_shipCasualties`, `g_shipPlayerName`, `g_shipPlayerRank`, `g_shipPlayerDept`,
  `g_shipPlayerSeat`) and executes `configs/lwh-start.cfg`. The UI module and the game module are
  different libraries, so this is the seam they have always used.
- **The apply site, `module/ship/g_ship.cpp`.** `Ship_Init` reads those cvars and applies the state
  when the ship exists. **`g_shipConfigured` defaults to `0`, so a run that was not configured is
  untouched** — the canon default path is exactly what it was. `PublishStartState` fills the cvars
  the screen would read; `ship startstate ...` reaches the same mechanism from the console.
- **The check, `scripts/configurator-check.sh`**, and the unit tests named below.

## Task A — the four dimensions, and the interface with character creation

| dimension | where it lives | what the player decides |
|---|---|---|
| **who you are** | `StartState.playerName/playerRank/playerDept` + `Ship::career` | the register, rank, department and path; **composed with character creation (S10)**, below |
| **who died** | `StartState.casualty[SEAT_COUNT]` | per command seat, survivor or casualty; any, all or none |
| **who fills the gaps** | `FillVacancies` (`Ship::seatHolder`) | the vacancies are **derived**, never written; or the roster is fictitious entirely |
| **the career path** | `Ship::career` | Starfleet junior, lower decks, Maquis, the chair |

**The interface: the start state chooses the *situation*; character creation chooses the *person*.**
`CreateCharacter` composes rather than competes. If the situation has already seated the player, a
later creation **names that person and does not move them** — a seat the situation granted is not
undone, and a rank the seat fixes is not overridden. If the situation did not seat the player,
creation picks a member and the state does not override the person. Demonstrated by
`TestCharacterCreationComposes`:

```
$ <build>/test_ship_core        # TestCharacterCreationComposes
  the situation seats the player in the chair; CreateCharacter renames that same record to "Okoro";
  s.player is unchanged, still the chair, still rank 6.
  an ordinary post: the state seats a junior, and CreateCharacter("Reyes", ENGINEERING, 2) renames
  that same record, which still holds no seat.
```

## Task B — the three proving cases, with what was seen

`<build>/test_ship_core --starts` prints them in full. Verbatim, abridged to the lines that matter
(the whole is `scripts/configurator-check.sh`'s input):

```
== the configurator: the proving cases (docs/the-entry-point.md, Part three)
  [baseline] a fresh ship, no start state: 141 crew, save 9296 bytes

  [0] CANON -- The senior staff survive, the chain of command is intact, and the player is a junior officer.
      career: Starfleet junior (the ceiling is intact above you; you are sent into the breach)
      seat: the chair                = Kathryn Janeway
      seat: the first officer's seat = Chakotay
      ...
      player: Ensign Reyes, department duties; may authorise their station's work; 24 in their department
      log: Day 0, 0800. ... no one in the command crew was lost; ... No rescue is coming: ...

  [1] THE CHAIR -- The captain was lost with the array; the chair is vacant and the senior officer remaining inherits it.
      seat: the chair                = Reyes  (derived: the named holder was lost)
      seat: the first officer's seat = Chakotay
      ...
      player: Captain Reyes, the chair; in command: the ship's orders are theirs to give; 139 crew report to them
      log: Day 0, 0800. ... lost with her: Kathryn Janeway (the chair); ... No rescue is coming: ...

  [2] ALL-FICTITIOUS -- None of the show characters appear: ...
      roster: generated entirely -- no show character appears
      seat: the chair                = Crewman 009
      seat: the first officer's seat = Crewman 013
      seat: the security seat        = Crewman 074
      ...
      player: Ensign Mara Reyes, department duties; may authorise their station's work; 24 in their department
      log: Day 0, 0800. ... no one in the command crew was lost; ... a crew nobody here has served with before. ...
```

- **The canon default (owned).** Nobody dies, the chain is intact, the player is a junior officer.
  It is `STARTS[0]`, and the configurator opens on it (`OpenStart` calls `SeedFromPreset(0)`), so a
  player who wants the retail feel gets it by accepting what is there. **It is also driven end to
  end through the rendered menu** (below).
- **The captain.** The chair is vacant, and the derivation fills it: the state names a rank, not a
  seat, and `SeniorFitOfficer` seats the player because the player is the senior officer remaining.
  **The record names the casualty** — *"lost with her: Kathryn Janeway (the chair)"* — which is the
  fiction supplying the vacancy that created the post.
- **The all-fictitious crew.** The roster is generated entirely; no show character appears in the
  content, and (below) the post-not-name law is checked end to end.

### The captain and the canon default through the rendered menu

The engine runs headless under `xvfb-run` with the software Vulkan device. The menu route was driven
through the menu item's own callback and the screen's own key handler — the same path a hand's ENTER
takes — because there is no headless input injection. The screenshots are the engine's own.

```
$ cat > build/home/baseEF/drive_chair.cfg <<'EOF'
wait 2600
lwh_mainmenu_press
wait 250
screenshot lwh_cfg_chair
wait 250
lwh_start_key right          # the next entry in the list: THE CHAIR
wait 250
lwh_start_key enter          # the same callback a hand's ENTER fires
EOF
$ SDL_AUDIODRIVER=dummy xvfb-run -a ./scripts/run-lwh.sh --menu \
    +set s_useOpenAL 0 +set g_shipTestPos 1 +set g_shipTest 28 +exec drive_chair.cfg
...
LWH: activating the main menu's Long Way Home line
LWH: beginning the run -- THE CHAIR, career the chair, the canon crew
SHIP: start state applied from the menu: the canon crew, career the chair, Captain Reyes, the chair;
      in command: the ship's orders are theirs to give; 139 crew report to them
SHIP:   first log entry: Day 0, 0800. The Caretaker's array is gone and the ship is alone; lost with
      her: Kathryn Janeway (the chair); ... No rescue is coming: ... Captain Reyes, the chair; ...
SHIP: simulation active, 141 crew, a day every 24 minutes
spmap: SP map 'voyager' loaded — switching to SP render mode
EFSP: Munro connected
```

The canon default and the all-fictitious case were driven the same way (`drive.cfg`,
`drive_fict.cfg`), and each printed its own `start state applied from the menu` line:

```
LWH: beginning the run -- CANON, career Starfleet junior, the canon crew
SHIP: start state applied from the menu: the canon crew, career Starfleet junior, Ensign Reyes, ...
LWH: beginning the run -- ALL-FICTITIOUS, career Starfleet junior, an all-fictitious crew
SHIP: start state applied from the menu: an all-fictitious crew, career Starfleet junior, Ensign Reyes, ...
```

Screenshots: `build/home/baseEF/screenshots/lwh_cfg_mainmenu.tga`, `lwh_cfg_screen.tga`,
`lwh_cfg_chair.tga`, `lwh_cfg_chair_edit.tga`, `lwh_cfg_fictitious.tga`, `lwh_deck01.tga`.

## How a vacancy is derived rather than written

**One rule, `FillVacancies(Ship&, reason)`.** For each command seat whose holder is dead, assimilated
or absent, it reads the roster through the rule play already uses — `SeniorFitOfficer` for the chair
and the first officer, `DepartmentHead` for a department seat — and seats the most senior fit
candidate, promoting their rank to the seat's if it is lower, and recording the rise. It is
idempotent: a full chain is left alone. It is called **twice** by design:

1. from `ApplyStartState`, when the configurator closes the casualties and derives the gaps; and
2. from `Advance` (the tick), so **a vacancy created mid-run is filled by the same rule** — the one
   function, not two implementations.

`TestVacancySameRule` demonstrates the identity directly: the senior security officer after a
configurator casualty and after a mid-run `KillCrew` + `FillVacancies` are **the same record**, and
that record is the highest-ranked fit security officer left. Nothing hand-writes the seat: the state
sets a **rank**, and the derivation decides the seat. `TestCaptainVacancyDerived` shows the converse —
the same casualty with a junior player leaves the **senior officer** in the chair, because the
derivation, not the state, chose.

## How the post-not-name law held in the all-fictitious case

The law (`docs/the-entry-point.md`, part three): **content addresses the post; the simulation resolves
the post to a person.** The all-fictitious case is its first end-to-end test. Two checks:

- `TestFictitiousNoCanon` walks all 141 records: **no name and no type is a show character**, every
  seat is held by a generated record, and the command authority and the first log entry are signed by
  generated names — *"Janeway" does not appear*.
- The headless run's own output: `grep` over the run's content lines for the show names returns
  nothing in the log, the roster, or the authority. The one line that names a canon asset is the
  embodiment fallback, and it is named rather than smoothed over, below.

**The residual, named.** `ApplyPlayerBody` falls back to `headModel munro/default` (or `torres/default`)
for a crew member whose `type` is not a named character, because the game ships no generic head model.
Every generated crew member already used it before this pass; the law surfaces it. It is a
**rendering asset**, not content addressing a person, but the run does read it, so it is recorded as
the one remaining canon dependency in the fictitious case and as a seam: removing it needs a generic
head asset in the game data (art this repository may not add) or an engine-side alias.

## The log seed, at the default, and it describes what was chosen

`LogSeedText` is generated from the state and the ship, and `WriteLogSeed` inserts it **first**, so
later entries are measured against it. At the canon default:

```
Day 0, 0800. The Caretaker's array is gone and the ship is alone; no one in the command crew was
lost; the ship is whole enough to fly, condition green, with microfractures in the core, no array
behind us, and the Maquis still aboard. No rescue is coming: there is no one out here to hear, and no
way back. Ensign Reyes, department duties; may authorise their station's work; 24 in their
department. The path taken: Starfleet junior -- the ceiling is intact above you; you are sent into
the breach.
```

At THE CHAIR it states the loss, the ship's condition, that no rescue is coming, the player's own
position and the path — the same fields, describing what was actually configured:

```
... lost with her: Kathryn Janeway (the chair); ... No rescue is coming: ... Captain Reyes, the chair;
in command: the ship's orders are theirs to give; 139 crew report to them. The path taken: the chair
-- the conn is yours from the first minute: the ship, the crew, the fuel, the clock.
```

The four things the opening must do are the four clauses of that entry and of `PlayerPositionLine`:
the log entry (the losses + no rescue), the empty chair or its absence (the seat and the casualty it
names), who is doing what now (the derivation's rise, in the log), and **the player's own position**
(rank, post, what they may authorise, who reports to them). No cutscene is involved.

## The save format, and what it costs

**Save version 54.** Three things joined the save: `cfg.fictitious` (1 byte — it must be known before
`BuildRoster`, or the canon crew would be rebuilt on load), `career` (1 byte), and `seatHolder[8]`
(16 bytes, `0xFFFF` = vacant); and **each crew member's rank is now stored** (141 bytes) so a derived
promotion survives a save. The log's `what` field cap was raised from 63 to 600 bytes (a `U16`
length), because the log seed must survive a reload whole — the old 63-byte cap silently truncated it,
which `TestStartStateSaveRoundTrip` would have caught as a replay divergence.

Measured on this tree:

```
$ <build>/test_ship_core --starts
  [baseline] a fresh ship, no start state: 141 crew, save 9296 bytes
      [0] CANON            save:  9823 bytes   (+527)
      [1] THE CHAIR        save: 10271 bytes   (+975)
      [2] ALL-FICTITIOUS   save: 10603 bytes   (+1307)
```

- **The fixed cost is +159 bytes** (`16 + 2 + 141`), i.e. about **+1 byte per head plus 18**. The
  character-layer baseline was 9137 bytes at version 53; 9296 − 9137 = 159.
- The rest is the log seed itself: the canon entry is ~430 bytes, and THE CHAIR adds the death and
  the rise. All comfortably inside the "roughly four hundred bytes per head" the design budgeted.
- **Older saves do not load** — the version bump invalidates them, which is the cost named here, in
  the save header, and in `docs/gates.md`.

`TestStartStateSaveRoundTrip` packs and unpacks all three cases and checks `Pack(back) == blob`, the
seat occupancy, the career, and the log seed's survival, so a configured run replays identically.

## The checks

```
$ scripts/test.sh
==> ship simulation: unit tests
ship_core: all checks passed
==> hooks: the register matches the public surface
hooks-check: module/ship/ship_core.h declares 366 public functions; docs/hook-register.md registers 366 hooks
PASS  every public function is registered, and every registered hook exists
all checks passed

$ scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts
==> entity dictionary ... 318 classes
==> validator negative tests ALL PASS
==> compiler corpus pass: 2024 files: 2016 compiled and read back, 8 rejected -- exactly the known set

$ scripts/configurator-check.sh
==> the configurator: the three proving cases
PASS  the canon default is intact; the chair is vacant and derived from the casualty the record names;
      the all-fictitious crew names no show character anywhere; every log seed states no rescue is coming
```

The module builds (`efgame`, `efui`) with only the pre-existing `-Wwrite-strings` warnings, and the
modules export their intended entry points (the trap `AGENTS.md` names — a module file that was not
compiled in fails only at `dlopen`).

## What could not be verified

- **Whether a start state reads as a *situation* rather than as a settings screen.** That is the
  owner's, and it needs a picture; the screenshots above are the engine's own, but the judgement is
  his.
- **Whether the empty chair lands without a cutscene.** The roster and the log carry it, and the
  seat is derived; whether it *reads* is his.
- **A literal mouse click.** The headless harness has no input injection, so the menu item's callback
  and the screen's key handler were driven by console commands — the same functions
  `Menu_DefaultKey` calls. The rendering is the engine's own; the press is simulated. Disclosed.
- **Mode 2 (retail multiplayer).** Untouched by construction: this change is in the single-player
  game and UI modules; a two-machine match still needs a person.

## What the seams would need

- **The appearance derivation** (`docs/character-attributes.md`, *Appearance*). Not built, as the
  brief directs. The seam is `CrewMember.type` and `ApplyPlayerBody`: a derived appearance would add
  per-record appearance data beside `type`, and read it there. It would need the attribute document's
  schema and a source of faces; no art may be added.
- **Permadeath** (C5, ruled 2026-10-08). A *mode*, not built. The seam is `Config::mode` and
  `PlayerDead` / `AssumeCommand`: the mode would decide whether a closed player record ends the run
  or devolves command. It would need the mode's UI and the ending surface (`docs/endings.md`).
- **The arcs the career paths imply** — resentment, affinity, who trusts you. Not built, as directed.
  The seam is `Ship::career` (stored) plus the affinities work; the arc would read it.

## Judgement calls, named as calls

1. **The derivation may seat the player only where the player already holds the seat's rank.** The
   chair case needs the player seated by the rule (a rank-6 player *is* the senior officer); the
   all-fictitious case must not hand a rank-1 ensign the chair by accident. So the primary read may
   choose the player, and the fallback read never does. This is a call; it is stated in the code.
2. **The command seats are the eight the show names.** Captain, first officer, security, engineering,
   medical, sciences, operations, conn — the senior staff the canon default fixes. The hazard team is
   not a command seat; it is a unit. A further seat is a row in `SEATS`.
3. **The seat list in the UI is data, and the mechanism is in the model, so the *names* of the three
   shipped cases exist on both sides of the module split.** The UI cannot link `ship_core`, so the
   preset *names and blurbs* are mirrored in `STARTS[]` in the screen; the *effect* (casualties,
   fictitious, career, record) is written as cvars and interpreted once, in the model. The
   `configurator-check.sh` print is the model's, and it is the authority.
4. **The player is always seated, even in the canon default, when the configurator is used.** The
   design's default is "a junior officer", so the configurator seats a generated junior of the
   chosen department. A run that was *not* configured keeps its old path (`g_shipRole`, or no player),
   because `g_shipConfigured` defaults to `0` — the default is untouched.
5. **The log `what` cap moved from 63 to 600 bytes with a `U16` length.** A truncated log seed is a
   replay divergence, and the format was being version-bumped anyway; the cost is bounded and named.
6. **The rank is now stored for every crew member.** It was not, so a promotion was lost on reload —
   a latent divergence the derived seats would have made visible. This is a correctness fix that
   happens to cost 141 bytes.
7. **The residual body-model dependency is left in place and named, not hidden.** Removing it needs
   an asset this repository may not add; the law that binds *content* is demonstrated independently.
8. **`FillVacancies` runs every tick.** It is idempotent and changes nothing on an ordinary watch
   (the canon tests are unchanged), and running it there is what makes "the same rule in play" true
   rather than aspirational.
