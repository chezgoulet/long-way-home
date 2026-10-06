# Evidence: the air-and-endurance gap, first slice

Date: 2026-10-07. The third of the five owner-approved gaps (`docs/gates.md`): *state* atmosphere per
compartment plus battery and auxiliary endurance; *control* sealing, force fields, rerouting, power
allocation; *visible* a countdown wherever the problem is; *failure* a compartment that runs out, a
ship that goes dark; *location* environmental control (deck 12), the EPS grid, every sealed
compartment. *Acceptance:* a breach produces a number, and the number moves when the crew act.

## What is built

**State and clocks.** `Deck` gains `forceField`. Two new core clocks read the same rates the
simulation uses, so the number and the world never disagree:
- `MinutesOfAir(s, deck)` — the time until a deck's atmosphere reaches `AIRLESS`, or **-1** if it is
  holding or refilling. An intact deck reads -1; a fully breached one reads minutes.
- `MinutesToDark(s)` — the time until the first source supplying now runs out (reactor fuel, or the
  emergency cells at their current draw), or -1 if nothing supplies.
Save format is now **version 10** (the force field).

**Control.** Sealing a hull is `RepairDeck` (already the console's `ship seal <deck>`); a force field
is `SetForceField` (console `ship field <deck> on|off`) and holds a breached deck's air. Rerouting
and power allocation are the ones S1–S2 already have (source on/off, priority, the EPS order).

**Visible.** The ship publishes `lwh_ship_clocks`: `DECK n AIR m MIN   ...   ENDURANCE hH mmM`,
with `lwh_ship_clocks_alarm` set when any deck is losing air or the ship is within the hour. The
Operations console draws it in red on an alarm, blue otherwise — "a countdown wherever the problem
is".

## What was proven

```
PASS  a breach has a number; seal it or hold the field and the number goes
```

`tests/ship/test_ship_core.cpp` (`TestAirAndEndurance`): an intact deck has no countdown; a fully
breached one reads a finite number of minutes; a force field over the breach takes the countdown
away; sealing the hull is the other answer; with the reactors off the batteries are the ship's
endurance (at most their three hours), and bringing a reactor back stretches it. The whole suite
passes, as do `test.sh`, `scripts/s2-check.sh`, `scripts/s4-check.sh` and `scripts/s10-check.sh`.

## What is left for this gap

- **Environmental control as a place.** Deck 12 is a generated hall with a station marker; the
  countdown is drawn on the Operations console, not yet at the environmental-control panel itself.
- **Force fields from the panel.** The field is a console command; it wants a control on the Ops or
  environmental panel.
- **Auxiliary endurance as its own number.** `MinutesToDark` reports the first source to run out; the
  contract names battery and auxiliary endurance separately, and would show both.
