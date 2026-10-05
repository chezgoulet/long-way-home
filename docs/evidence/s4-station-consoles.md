# Evidence: S4 — station consoles, first slice

Date: 2026-10-06. Gate S4 of `docs/ship-programme.md` asks for every station's console with its
canonical purpose, and live status on the panels in the world. This is the first slice of it.
Reproduce with `scripts/s4-check.sh`.

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

## What S4 still needs

- **Each station's real purpose.** Tactical can switch its weapons on and off; it cannot target or
  fire, because there is nothing outside the ship yet (S9). Conn cannot set a course. The
  transporter, sickbay, astrometrics and replicator panels have no ship console at all — their
  retail screens still open. Each needs its own controls, and the core needs the state they act on.
- **Live status on the panels in the world** — the "glance" half of the owner's "both". Needs
  drawing ship state onto panel surfaces: an engine capability not started.
- **A person at a panel.** The commands were issued by the harness; nobody has walked up to the
  tactical station on the bridge of the merged ship and pressed use.
- **The station table's edges are invention** (structural integrity and life support under
  Operations, the tractor beam under Tactical); see the lore ledger.
- Mouse support and LCARS artwork, as for S2.
