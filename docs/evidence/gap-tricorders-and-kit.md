# Evidence: the tricorders-and-away-team-kit gap, first slice

Date: 2026-10-07. The fifth of the five owner-approved gaps (`docs/gates.md`): *state* kit as stores
entries (tricorders, phasers, EV suits, medical kit) with charge and condition; *control* loadout at
the transporter room or shuttlebay, scanning in the field; *visible* scan results at human scale —
the same writes to the chart, over a smaller radius; *failure* a dead charge, lost kit, or a scan
that reads wrong on a low battery; *location* transporter rooms, shuttlebay, every away site.
*Acceptance:* an away mission can be solved by scanning rather than by shooting.

## What is built

**State.** `Stores` gains the away kit: `tricorders`, `phasers`, `evSuits`, and a shared
`tricorderCharge` (0 dead .. 1 fresh). Save format is now **version 12**.

**Control.** `LoadAwayKit` loads a party's kit from the ship's locker (bounded) and records it;
`Scan` reads a site with the tricorders. A scan consumes charge, marks the site visited (the same
write the sensors make to the chart, over a smaller radius), and reports what it finds. Below a fifth
of a charge it reads *the next thing along* — a wrong reading the player may act on — and with no
charge at all it does nothing but say the tricorder is dead.

**Visible.** `lwh_ship_kit` (`AWAY KIT  TRICORDERS n (p%)  PHASERS n  EV SUITS n`) is drawn on the
Operations console; every loadout and scan writes to the log (the fourth gap's artifact). The console
reaches it as `ship kit <tricorders> <phasers> <evsuits> <charge>` and `ship scan`.

## What was proven

```
PASS  the tricorder reveals, misreads on a weak charge, and dies
```

`tests/ship/test_ship_core.cpp` (`TestAwayKit`): a charged tricorder reads the site correctly and
marks it visited; a weak charge reports a different kind (the site is a hostile, the reading is not);
a dead tricorder scans nothing and says so; and the kit survives a save and a load. The whole suite
passes, as do `test.sh`, `scripts/s2-check.sh`, `scripts/s4-check.sh`, `scripts/s10-check.sh` and
`scripts/g3-measure.sh`.

## What is left for this gap

- **The away site itself.** Scanning targets the sector beacon the ship is at; there is no walkable
  away site, no loadout at a transporter-room panel, and no human-scale scan yet. Teaching the
  tricorder to read the ship's own compartments (the same function, a smaller radius) is the honest
  next step.
- **Kit condition and loss.** Charge exists; condition, dropped kit and lost kit do not.
- **The acceptance run** — an away mission solved by scanning rather than shooting — needs the away
  site and a session.
