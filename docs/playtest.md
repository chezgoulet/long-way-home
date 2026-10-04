# Playtest — first run of the native client

What to run, what should happen, and what to send back.

## Run it

From a logged-in desktop session on the playtest host:

```
cd /home/c/big/git/long-way-home
./scripts/run-engine.sh
```

It needs a real X session, not SSH: Vulkan requires DRI3 to present, and a virtual display has none.
It reads the game data from `build/baseEF` (a symlink to your Steam install — read-only) and writes
configuration and saves to `build/home`, deliberately separate so nothing is written back into the
installation.

Every run is logged to `build/run-<timestamp>.log`.

## What should happen

1. A window opens with the Elite Force console text scrolling, then the LCARS main menu.
2. In the log, roughly in order: `Lilium Voyager SP 1.40` → `21675 files in pk3 files` →
   `Client Initialization Complete` → `SP: map/transition/use/save/load commands registered` →
   `R_Init` → `Vulkan` device selection.
3. Start a mission — either **New Game** from the menu, or open the console (`~`) and type
   `spmap voy1`. The SP game module (`libefgame.so`) loads on the **map** load, not at startup, so
   this is the step that exercises it.
4. Look for `SP_LoadGame` / `libefgame` lines in the log once the map begins to load.

## What to send back

- The log file (`build/run-<timestamp>.log`) — it is the evidence, so send it even if everything
  looked fine.
- Whether the menu rendered, and whether a map began to load.
- Anything that looked wrong on screen: missing textures, wrong resolution, no sound, a hang.

## Known absences in this build

- **No cutscenes.** The host's FFmpeg packages are behind Ubuntu Pro, so Bink playback is compiled
  out; `.bik` files are skipped rather than crashed on. Cinematics are milestone M5.
- **Saves are fresh.** Retail PC saves are not compatible with the port yet (upstream documents
  this), so start a new campaign rather than loading an old save.
- **No configuration yet.** The first run creates its own config; the retail `efconfig.cfg` is not
  read.

## The safe-video-settings prompt, explained

If the engine asks whether to "start with safe video settings", it is telling you that the previous
run did not exit cleanly. The marker is a **PID file** (`build/home/baseEF/*.pid`), written at startup
and removed on a clean exit — so any run that was killed leaves it, and the next run offers the dialog.

It is harmless: answering yes only sets `com_abnormalExit` for that run. To stop being asked, quit from
the game menu instead of killing the process, or delete the stale file:

```
rm -f build/home/baseEF/*.pid
```

Worth knowing because it is also invisible in a headless run: an unanswered dialog blocks startup, and
the engine simply stops logging. Several of my own smoke tests stopped that way before I traced it.

## Scripts live under `real_scripts/` in the game data

A map's `usescript "voy9/intro"` is satisfied by `real_scripts/voy9/intro.IBI` inside the paks — pak0
carries 1,615 of them and the expansion pak another 306, 1,918 in total. The GDK script corpus is a
2000 snapshot, older than the expansion, so a scenario built from it will reference scripts that only
the installation provides. Tell the validator where the game is and it stops reporting them:

```
python3 tools/validator/validate.py <scenario> --data "<installation>" --entitydict <dict.json>
```

## If it fails to start

- `cannot open shared object file` → the rootless dependencies are missing; they live in
  `build/deps` and the script adds them to the library path. Re-run
  `scripts/playtest-host-setup.sh` if that directory is gone.
- A black screen or an immediate exit with a Vulkan message → the display or driver, not the
  engine. Send the log; the Vulkan lines will say which.
