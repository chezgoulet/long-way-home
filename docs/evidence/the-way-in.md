# Evidence: the way in (G1 and the first half of G2)

Date: 2026-10-08. Branch `feat/the-way-in`, cut from `testing`. This closes the walkthrough's **G1**
(there is no way to start Long Way Home from the game's menus) and the first half of **G2** (the
opening situation does not exist — this pass ships only the canon default), as scoped by
`docs/the-entry-point.md` Part one, and constrained by Part three's ruling that the entry point
becomes a configurator with four dimensions.

The design's rule is the shape of the work: *"The menu grants the situation. The fiction supplies
the reason. The simulation then holds you to it."* (`docs/start-states.md`). The surface that starts
the run is the surface that chooses the start state, so both are here.

## What is built

**The launcher — `scripts/run-lwh.sh`.** One thing a person runs. It installs the merged ship's pak
and the launch config into the playtest home and starts the engine with `com_hunkMegs 768`. Without
`--menu` it executes the config and loads the ship; with `--menu` it stops at the main menu so the
mode is chosen there.

**The config — `configs/lwh-start.cfg`, the one place the values live.** It sets `g_ship 1`, sets
`g_shipDeckPitch 3072`, and loads `map voyager`. Both entry paths execute this same file:
`scripts/run-lwh.sh` with `+exec lwh-start.cfg`, and the main menu's Long Way Home line with `exec
lwh-start.cfg` from the UI module. There is no second copy of the values to drift. The file is
packaged into `longway_voyager.pk3` by `scripts/build-ship.sh`, so any installation that carries the
ship carries the way in.

**The menu line — `module/ui/ui_lwh_start.cpp`, attached by `patches/0020`.** A `menubitmap_s`
button, `LONG WAY HOME`, added to the retail main menu after the eight shipped buttons, drawn in the
retail 3×3 grid's one free cell (column 3, row 3). It is a rectangle in the same shape, colour and
font as New Game, Virtual Voyager and Mods, so it reads as a mode beside them. The engine-side
change is one attach point (`LWH_UI_MainMenuAdd(&s_main_menu)`) and an inline no-op when our UI
module is not compiled in; the button, its callback and everything it decides live in the module.

**The start-state selector.** The button opens `LONG WAY HOME — WHERE YOU BEGIN`, a **list of start
states with one entry in it** (`CANON`), with the entry's blurb and "the reason the crew accept it"
(`docs/start-states.md`). It is deliberately a list, not an assumption that there is only one: Part
three's configurator extends the array and the screen, and nothing else moves. Choosing an entry
calls `UI_ForceMenuOff()` and executes `lwh-start.cfg` — the retail New Game path exactly
(`ui_game.cpp`: `UI_ForceMenuOff(); ui.Cmd_ExecuteText(EXEC_APPEND, "map borg1")`).

**Two robustness fixes this change forced into the open, both recorded as calls below.** A stale
`Lilium Voyager SP.pid` left by a killed run makes the next start hang in `Com_Init`; the launcher
clears it (as the other check scripts already do). And the main menu's opening animation ended only
on a frame whose counter was exactly `buttonCnt-1`, so at a low frame rate the counter can step past
it and the menu hangs before drawing any button — including the new line; `patches/0020` changes
that one comparison to `>=`.

## The rendered path, and what was seen

The engine runs headless under `xvfb-run` with the software Vulkan device. The menu route was driven
through the menu item's own callback and its own key handler — the same path a hand's ENTER takes
(`Menu_DefaultKey` calls the item callback; `StartKey` handles the selector) — because there is no
headless input injection available. The button is **rendered, not asserted**: the screenshots below
are the engine's own.

```
$ rm -f build/home/baseEF/screenshots/*.tga
$ cat > build/home/baseEF/drive.cfg <<'EOF'
wait 2600
screenshot lwh_mainmenu
wait 250
lwh_mainmenu_press            # the Long Way Home item's callback, as a hand's ENTER would call it
wait 250
screenshot lwh_startstates
wait 250
lwh_start_key enter           # choose the listed canon state
EOF
$ SDL_AUDIODRIVER=dummy xvfb-run -a ./scripts/run-lwh.sh --menu \
    +set s_useOpenAL 0 +set g_shipTestPos 1 +set g_shipTest 28 +exec drive.cfg
installed: .../build/home/baseEF/longway_voyager.pk3
installed: .../build/home/baseEF/lwh-start.cfg
starting at the main menu -- choose 'LONG WAY HOME'
...
execing drive.cfg
ui: SetActiveMenu(main)Wrote screenshots/lwh_mainmenu.tga
LWH: activating the main menu's Long Way Home line
Wrote screenshots/lwh_startstates.tga
LWH: beginning the run -- exec lwh-start.cfg
execing lwh-start.cfg
SP map 'voyager': spawning world + SP game
EFSP: SP_SpawnServer: CM_LoadMap(maps/voyager.bsp)
SHIP: simulation active, 141 crew, a day every 24 minutes
spmap: SP map 'voyager' loaded — switching to SP render mode
EFSP: Munro connected
SHIP: deck 1 blockout: standing at (-3654 -3203 -3956)
Wrote screenshots/lwh_deck01.tga
```

- `screenshots/lwh_mainmenu.tga` — the rendered main menu: **NEW GAME, LOAD GAME, CONFIGURE,
  VOYAGER CREW, CREDITS, EXIT PROGRAM, VIRTUAL VOYAGER, MODS**, and in the third column's last cell,
  **LONG WAY HOME**. Read on the screen, not asserted from the build.
- `screenshots/lwh_startstates.tga` — the rendered selector: `LONG WAY HOME — WHERE YOU BEGIN`, the
  one entry `CANON`, its blurb *"The senior staff survive, the chain of command is intact, and the
  player is a junior officer."*, and its reason.
- `screenshots/lwh_deck01.tga` — the ship the menu loaded, first person, on deck 1.

The log is the sequence that produced them: the menu item activated, the config executed, the
merged map loaded, the simulation started with **141 crew**, and the player connected. **No console
was opened and no cvar was typed in the session.**

The canon default that loaded is `docs/start-states.md`'s default and the model's own, from the test
binary:

```
$ <build>/test_ship_core --day
day 0 08:00  alpha watch  condition green  crew fit 141 of 141
...
```

the chain of command is the intact canon roster (Janeway at the conn by default), and the player's
default record is **Munro** — a junior officer (`ship_core.cpp`: `if (s.crew[i].type == "munro")
s.player = i;`), the body the shipped ship already walks in.

## The cvars stayed off by default

`g_ship` is registered `0` (`module/ship/g_ship.cpp`: `gi.cvar("g_ship", "0", 0)`); its default did
not move. The launcher's `--menu` route sets nothing but the engine's hunk. Only choosing the menu
line — executing `configs/lwh-start.cfg` — turns the simulation on.

The ship map loads with the cvars unset and **the simulation does not start**:

```
$ xvfb-run -a ./scripts/run-engine.sh --home-dir build/home \
    +set s_useOpenAL 0 +set com_hunkMegs 768 +map voyager
EFSP: SP_SpawnServer: CM_LoadMap(maps/voyager.bsp)
EFSP: Munro connected
$ grep -c "SHIP: simulation active" <log>
0
```

The same map, through the menu, prints `SHIP: simulation active, 141 crew`. So the map-loading path
is independent of the simulation, and only the menu (or the launcher's direct route, which executes
the same config) starts it.

## Mode 1 was re-proven, not assumed

A new main-menu line is exactly the change that can disturb the campaign, so it was re-run against
the rebuilt module. The command below is the one the retail New Game button fires:

```
$ xvfb-run -a ./scripts/run-engine.sh --home-dir /tmp/lwh-campaign +set s_useOpenAL 0 +map borg1
SP map 'borg1': spawning world + SP game
EFSP: SP_SpawnServer: CM_LoadMap(maps/borg1.bsp)
EFSP: SP_SpawnServer: ge->Init done. num_entities=564 linked=371
EFSP: SP_StartClient: CG_INIT returned OK
spmap: SP map 'borg1' loaded — switching to SP render mode
EFSP: Munro connected
```

The campaign map loads, the SP game initialises, the cgame comes up and the player connects. No
`SHIP: simulation active` line appears — the ship layer is inert with the cvar unset. (The rendered
main menu above shows all eight retail buttons present, which is the other half of "unmodified in
behaviour": the line is added beside them, not in place of one.)

## Mode 2, stated plainly

Retail multiplayer is unaffected by construction: mode 2 is **cMod as shipped**, a separate client
launched by `scripts/run-cmod.sh`, and this change touches only the single-player UI module and the
SP main menu. It was **not run here**; a two-machine LAN/VPN match still needs a person
(`docs/gates.md` G7, `docs/walkthrough.md` G24). Nothing in this change reaches that path.

## g_shipDeckPitch: what it is now, and where the number comes from

**It belongs to the launcher's own config, not to a default.** It stays `0` in the code, because `0`
means "an ordinary level": a non-zero pitch turns on the deck-scoped name lookup and the crew
layer's per-deck arithmetic, and defaulting it on would misread every retail map and the
single-deck crew scenarios. It is set where a merged-ship run is set up — once, in
`configs/lwh-start.cfg` — and never typed.

**The number is not folklore: 3072 is the stitcher's own pitch.** `tools/shipmap/stitch.py` places
deck *n* at `(n - 1) * pitch` below deck 1, its `--pitch` default is `3072`, and it refuses a pitch
that is not a multiple of 1024 because world-aligned textures would slip. `3072 = 3 × 1024`. The
shipped `voyager` map was built with that default, so 3072 is the value that matches it; the module
documentation (`g_scope.cpp`, `g_crew.cpp`) and the config now say so together.

## The checks

```
$ scripts/test.sh
...
==> patches: numbered without gaps, and each one parses
    20 patches
all checks passed
```

Exit status **0**. (This now includes `scripts/run-lwh.sh` under shellcheck.)

```
$ scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts
==> entity dictionary
parsed 318 entity classes ...
==> validator negative tests
ALL PASS
==> compiler corpus pass ...
    2024 files: 2016 compiled and read back, 8 rejected by the compiler, 0 read-back failure(s)
    the 8 rejections are exactly the known, documented set
```

Exit status **0**. The 8 rejections are the permanent, expected set the script already tolerates
(sound tables, configuration, editor backups and the three named compiler failures).

The patch series was confirmed to apply by reversing it against the patched upstream tree
(`git apply --check -R patches/0020-...` → ok), and the built modules export only their intended
entry points with no unresolved `LWH_*` symbols — the trap `AGENTS.md` names, where a module file
that was not compiled in fails only at `dlopen`.

## What could not be verified

- **Whether the menu looks *right*.** The screenshots show a line in the same shape as the retail
  buttons, in the free grid cell; whether "Long Way Home" reads as a mode beside the others rather
  than as a small addition is the owner's, and it needs a picture. `docs/the-entry-point.md` says
  the same of this item.
- **A literal mouse click or a literal keypress.** The headless harness has no input injection, so
  the menu item's callback and the selector's key handler were driven by console commands — the
  same functions `Menu_DefaultKey` calls. The *rendering* is the engine's own; the *press* is
  simulated. This is disclosed rather than smoothed over.
- **Mode 2 end to end** (see above).
- **A real frame rate and a real display.** The menu opening animation's frame-skip is what the
  `>=` fix addresses; under a real renderer the original `==` usually lands.

## Judgement calls, named as calls

1. **The values live in a config, and both entry paths execute it.** The brief says "the launcher's
   line is the only place they live". I read that as: one source, no second copy. A shell launcher
   and a C++ menu cannot share a literal line, so the one source is `configs/lwh-start.cfg`, the
   launcher references it, and the menu executes it. `com_hunkMegs` stays on the launcher's command
   line because it is a LATCHed startup allocation that cannot be exec'd at all — it is engine
   memory, not a run value, and it is documented as such.
2. **The launcher has a `--menu` mode.** Task A says the launcher "loads the map"; the acceptance
   says the ship loads "from the menu". The default loads the map; `--menu` stops at the menu and
   lets the menu do it. Both execute the same config.
3. **The menu line is a bitmap item added after the retail eight, and the engine-side change is an
   attach point.** The rule is game logic in `module/`, attach points in patches. The button, its
   callback and the selector are module code; the patch carries one call, one inline no-op and the
   opening-animation robustness fix.
4. **The opening-animation fix is in the patch, not the module, because it is in upstream code.**
   `iMax == buttonCnt-1` → `>=` changes nothing except a hang that would otherwise hide the line.
   Recorded here so it is visible, not smuggled.
5. **The selector is a list with one entry, and the entry is *data*.** It does not hardcode "one
   start state"; it renders `STARTS[]`. Part three's configurator extends that array and the screen.
6. **The menu entry appears even when the ship is not installed.** Clicking it then fails `exec`
   with the engine's own "couldn't exec" line. Installing the ship is the launcher's job (or
   `build-ship.sh --out build/home/baseEF`), and the entry is meaningful only once it is installed;
   hiding the line conditionally would mean the UI had to read the filesystem, which is a larger
   change than this pass, and the contract is that the three modes are visible.
7. **I did not build the configurator, the character layer, or touch the roster** (as instructed):
   this pass ships the mechanism the configurator will extend, and the canon default only.
