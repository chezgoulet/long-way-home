# Evidence: S4 — station consoles, first slice

Date: 2026-10-06, updated 2026-10-07. Gate S4 of `docs/ship-programme.md` asks for every station's
console with its canonical purpose, and live status on the panels in the world. The consoles are the
first slice; the glance's anchored half is a later section. Reproduce with `scripts/s4-check.sh` and
`scripts/s4-glance-check.sh`.

## What is built

**Stations exist in the ship core** (`ship::Station`, tested in `tests/ship`): every system is
operated from exactly one of Tactical, Operations, Conn or Sickbay, and Engineering — which
distributes power — sees them all.

| station | operates | may also |
|---|---|---|
| Main Engineering | every system | set the power order; call the alert condition |
| Tactical | shields, phasers, torpedo launchers, tractor beam | call the alert condition |
| Operations | sensors, communications, transporters, life support, structural integrity, computer core, turbolifts, replicators, holodecks | |
| Conn | warp drive, impulse drive, navigational deflector, inertial dampers | |
| Sickbay | sickbay | |

**One screen serves every station** (`module/ui/ui_lwh_engineering.cpp`), showing the station's
own systems and offering only what that station may do.

**The panels already in the ship open them.** The retail game's station panels issue
`ui_engineeringstatus`, `ui_tactical`, `ui_ops` and `ui_navigation`. With the ship simulation
running those commands now open the working console for that station; with it off, the retail
screen opens as it always did. No map was changed for this.

## What was proven

```
PASS  opened by the retail panel's command, the Tactical console set the condition and switched its own system
PASS  the power order cannot be changed from Tactical
```

`ui_tactical` is issued as a panel would issue it. The console that opens lists four systems —
shields, phasers, torpedo launchers, tractor beam — and not the other fourteen. Key `3` puts the
ship at condition red; down and ENTER switches the phasers off; a priority key is pressed and the
phasers' place in the power order is unchanged afterwards. A screenshot is left at
`build/g3-home/baseEF/screenshots/lwh_tactical.tga`.

## The other station panels open their working console (2026-10-07)

The maps' station panels fire `genericmenu <screen>`. With `g_ship 1` our UI intercepts the ones
that name a station — `tactical`, `engineeringStatus`, `navigation`, and now `transporter` and
`astrometrics` — and opens the working console for the station that works that system, instead of the
retail screen. Operations works both the transporter and the sensors, so those two panels open the
Operations console (the transporter/astrometrics split into their own stations is not in the table;
that is a lore decision to make, recorded as invention). The turbolift, logs, padds and holodeck are
not stations and keep their own menus exactly as before.

Sickbay, which has no panel command of its own, gains its first real readout: the ship publishes
`lwh_ship_medical` — injured, in treatment, sickbay output, lost and assimilated — and the Sickbay
console draws it. `scripts/s4-check.sh` now opens the transporter's panel and the Sickbay console:

```
LWH: the transporter panel opens the OPERATIONS console
SHIP: medical state: INJURED 0   IN TREATMENT 0   SICKBAY OUTPUT 100%   LOST 0   ASSIMILATED 0
```

## The glance: the ship's state at the panel (2026-10-07)

The owner asked for **both**: live status on the panel in the world, and a full-screen console. The
console is above. This is the first of the two halves of the glance — a compact readout anchored to
the panel the player is standing at, before the engine paints live state onto the surface itself.

**How it is wired.** Only cgame draws the HUD, so `patches/0013` adds the one seam that lets our
module do it: `cgame/lwh_cgame_hooks.h` (an empty inline unless `LWH_MODULE_DIR` is set) and a single
call in `CG_Draw2D`. Everything else is ours, in `module/cgame/lwh_panel_glance.cpp`: it takes the
panel under the crosshair, or failing that the nearest usable within a pace, projects its top to the
screen, and draws three lines of live state — day and watch, condition and power, crew and stores,
with a boarders/Borg/damage alarm when there is one — in a small LCARS block coloured by the alert.

The state is read straight from the live ship (`Ship_Get()`), not from a copy; the game module and
cgame share their entity array in this port, and the ship's published cvars are not needed for it.

One limitation, named: the map does not yet say which usable panel is a *station* console, so the
fallback is any `func_usable` within a pace — a light switch can show the readout too. The per-deck
station table that S5 needs (`docs/HANDOFF.md`) is also what narrows this to a console.

**What was proven** (`scripts/s4-glance-check.sh`, one headless run on deck 4):

```
SHIP: glance test: standing at (-2614 -3814 -3858) looking at buzz (func_usable)
LWH: ship glance at liftlights (func_usable)
PASS  the ship's live state is drawn at the panel the player stands at
      screenshot: build/g3-home/baseEF/screenshots/lwh_glance.tga
```

The screenshot shows the block over the panel with `DAY 0 08:00  CONDITION GREEN`, `POWER
1160/1160  CREW FIT 141/141`, `TORPEDOES 38`. With `g_ship 0` the hook draws nothing and retail play
is unchanged. The whole 13-patch series was re-verified from a clean clone
(`scripts/bootstrap-upstream.sh`): it applies, builds, and exports the intended entry points only.

## What S4 still needs

- **Each station's real purpose.** Tactical can switch its weapons on and off; it cannot target or
  fire, because there is nothing outside the ship yet (S9). Conn cannot set a course. The transporter
  and astrometrics panels now open the Operations console and Sickbay shows medical state (above),
  but none of them has a control beyond on/off yet — the transporter needs a target and a recipient,
  sickbay needs triage, and astrometrics needs the survey; each needs its own controls and the core
  state they act on. The replicator panel still opens its retail screen.
- **The second half of the glance: the surface itself painted with state.** The anchored readout
  above is the first half; a renderer capability to upload live state into a texture bound to the
  panel's shader is the next, larger step (see `docs/engine-extension-policy.md`).
- **A person at a panel.** The commands and the readout were exercised by the harness and the
  camera; nobody has walked up to the tactical station on the bridge of the merged ship by hand.
- **The station table's edges are invention** (structural integrity and life support under
  Operations, the tractor beam under Tactical); see the lore ledger.
- Mouse support and LCARS artwork, as for S2.
