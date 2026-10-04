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

## If it fails to start

- `cannot open shared object file` → the rootless dependencies are missing; they live in
  `build/deps` and the script adds them to the library path. Re-run
  `scripts/playtest-host-setup.sh` if that directory is gone.
- A black screen or an immediate exit with a Vulkan message → the display or driver, not the
  engine. Send the log; the Vulkan lines will say which.
