# Evidence: S2 — the ship in the game, and the Engineering console

Date: 2026-10-05. Gate S2 of `docs/ship-programme.md`. Reproduce with `scripts/s2-check.sh`
(three headless engine runs, about three minutes).

## What was proven, in the real engine

```
PASS  the reloaded ship matches the saved one, system by system
PASS  alert, damage and the reordered priority all took effect and persisted
PASS  the Engineering console, operated key by key with the game running, changed the ship
```

1. **The ship runs in the game.** `g_ship 1` starts the simulation with the map; it advances with
   the game clock (a ship's day every 24 minutes by default, `g_shipDayScale`).
2. **The save holds the ship.** A ship put to red alert, damaged, breached and re-prioritised is
   saved; a second run loads it and finds the same ship, compared line by line.
3. **The console drives it.** Main Engineering's screen is opened and operated by key press: `3`
   for condition red, down twice and ENTER to switch the third system (inertial dampers) off, then
   back up and RIGHT to demote life support one place in the power order. Afterwards the ship is at
   condition red, the dampers are off, and life support stands second. The keys go through the same
   function a keyboard does; the screen sends the ship the commands a hand would.
4. **The world does not stop for the console.** The game clock runs throughout: the test's later
   steps are scheduled on it and would never fire otherwise.

A screenshot of the console is taken at the end of the run and left at
`build/g3-home/baseEF/screenshots/lwh_engineering.tga` (not committed: it is rendered with the
game's fonts). It shows the header in the alert colour, the four power sources with output bars,
and the eighteen systems in power order with allocation, output, condition and crew at station.

## How the pieces talk

The ship is in the game module and the screen is in the UI module — separate libraries. The game
publishes the ship's state four times a second into `lwh_ship_*` cvars; the screen reads them and
sends `ship ...` commands back. Nothing in the screen simulates anything.

## Three things the engine could not do, and now can

Found by the test failing, not by reading:

- **Console commands never reached the single-player game module.** The engine offers a command
  to the game only while a server is running, and the SP bridge has none — so `nav`, `npc`,
  `runscript` and every other developer command in the released game were dead in this port, and
  so were ours. `patches/0006`. (This also means the `crew` console commands documented for G3 did
  not work when typed until now; the G3 measurement never used them.)
- **Any menu froze the game.** `patches/0007` lets a menu declare itself live.
- **A "fullscreen" menu stops the game being drawn, and the game's frame is driven from its
  draw.** No patch: the console screen simply is not fullscreen in that sense.

## What S2 does not do

- **The owner has not operated it.** `scripts/run-engine.sh +set g_ship 1 +map tour/deck04`, then
  `ship console` in the game console (or bind a key to `ui_lwh_engineering`).
- **The console is opened by a command, not by walking up to a panel.** In-world panels are S4.
- **Keyboard only.** No mouse on this screen yet.
- **It is drawn in flat colour.** The LCARS shape, not yet LCARS artwork.
- **The level-change carry is written and still unexercised.**
- **Only Engineering.** Every other station is S4.
