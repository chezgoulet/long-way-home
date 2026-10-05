# Evidence: S1 — the ship core

Date: 2026-10-05. Gate S1 of `docs/ship-programme.md`: the ship as a working system, headless.
`module/ship/ship_core.*`, tested by `tests/ship` (run by `scripts/test.sh`, so by CI).

## What the tests establish

| property | test |
|---|---|
| A new ship is whole: 141 crew, every system at full output except those stood down at condition green, batteries idle | a new ship is whole and consistent |
| Power allocated never exceeds power supplied; no system draws more than its demand or delivers more than its health | power is conserved |
| At battle stations demand exceeds supply, and what goes without is whatever stands last in the priority list; a console can reorder the list | shedding follows priority |
| With every reactor down the batteries carry life support and nothing optional, for three hours, then stop; air goes stale slowly; restoring a reactor restores the ship | batteries carry the critical systems, then run out |
| A breached deck vents in minutes, cannot be refilled while open, and its neighbours are unaffected | a breached deck vents, and only that deck |
| Fuel is spent in proportion to load; without antimatter the core stops and fusion continues; without deuterium only batteries remain | fuel is spent by what runs |
| Everyone is someone; every station has its hands on every watch; the same seed is the same crew | the roster |
| Each watch: 8 h duty, 8 h sleep, the rest lived; exactly one watch on duty at any hour | the daily routine |
| Across two full days, watch changes included, no station is ever short-handed and nobody ends exhausted | a day aboard |
| Red alert brings everyone awake to stations; a station whose crew are dead runs at half | red alert is all hands |
| Damage caps output; a wreck draws no power; repair restores | damage caps output |
| One six-hour step and 360 one-minute steps arrive at the same ship | deterministic, and independent of how time is sliced |
| The whole ship packs into under 2 KB, restores identically, continues identically, and rejects truncated, foreign, newer or corrupt records untouched | the save is the ship |

## A day aboard

`test_ship_core --day`, the ship undamaged at condition green:

```
day 0 08:00  alpha watch  condition green  crew fit 141 of 141
power 1160 supplied, 1160 allocated  deuterium 100.0%  antimatter 100.0%  batteries 100%  torpedoes 38
  source warp core              1000 of 1000  health 100%
  source impulse reactors        160 of  300  health 100%
  source auxiliary fusion          0 of  120  health 100%
  source emergency batteries       0 of   80  health 100%
  life support             deck 12  power  60/ 60  manned 1/1  health 100%  output 100%
  structural integrity     deck 11  power  80/ 80  manned 1/1  health 100%  output 100%
  inertial dampers         deck 11  power  40/ 40  manned 1/1  health 100%  output 100%
  computer core            deck  7  power  60/ 60  manned 1/1  health 100%  output 100%
  shields                  deck  1  power   0/200  manned 1/1  health 100%  output   0%
  sensors                  deck  8  power  60/ 60  manned 2/2  health 100%  output 100%
  warp drive               deck 11  power 400/400  manned 3/3  health 100%  output 100%
  impulse drive            deck 10  power 100/100  manned 2/2  health 100%  output 100%
  phasers                  deck  1  power   0/150  manned 2/2  health 100%  output   0%
  torpedo launchers        deck  9  power   0/ 30  manned 2/2  health 100%  output   0%
  navigational deflector   deck 11  power  50/ 50  manned 1/1  health 100%  output 100%
  communications           deck  1  power  20/ 20  manned 1/1  health 100%  output 100%
  transporters             deck  4  power  60/ 60  manned 1/1  health 100%  output 100%
  sickbay                  deck  5  power  30/ 30  manned 2/2  health 100%  output 100%
  turbolifts               deck  1  power  20/ 20  manned 0/0  health 100%  output 100%
  tractor beam             deck 10  power  60/ 60  manned 1/1  health 100%  output 100%
  replicators              deck  2  power  60/ 60  manned 0/0  health 100%  output 100%
  holodecks                deck  6  power  60/ 60  manned 0/0  health 100%  output 100%

hour  watch  on-duty  mess  recreation  quarters   Janeway        Torres          Crewman 100
08:00  0       60     41       0        40      duty     d1    duty     d11    meal     d2 
09:00  0       60      0      41        40      duty     d1    duty     d11    rec      d6 
10:00  0       60      0      41        40      duty     d1    duty     d11    rec      d6 
11:00  0       60      0      41        40      duty     d1    duty     d11    rec      d6 
12:00  0       60      0       0        81      duty     d1    duty     d11    personal d9 
13:00  0       60      0       0        81      duty     d1    duty     d11    personal d9 
14:00  0       60      0       0        81      duty     d1    duty     d11    personal d9 
15:00  0       60     40       0        41      duty     d1    duty     d11    sleep    d9 
16:00  1       40     60       0        41      meal     d2    meal     d2     sleep    d9 
17:00  1       40      0      60        41      rec      d6    rec      d6     sleep    d9 
18:00  1       40      0      60        41      rec      d6    rec      d6     sleep    d9 
19:00  1       40      0      60        41      rec      d6    rec      d6     sleep    d9 
20:00  1       40      0       0       101      personal d3    personal d3     sleep    d9 
21:00  1       40      0       0       101      personal d3    personal d3     sleep    d9 
22:00  1       40      0       0       101      personal d3    personal d3     sleep    d9 
23:00  1       40     41       0        60      sleep    d3    sleep    d3     meal     d2 
00:00  2       41     40       0        60      sleep    d3    sleep    d3     duty     d7 
01:00  2       41      0      40        60      sleep    d3    sleep    d3     duty     d7 
02:00  2       41      0      40        60      sleep    d3    sleep    d3     duty     d7 
03:00  2       41      0      40        60      sleep    d3    sleep    d3     duty     d7 
04:00  2       41      0       0       100      sleep    d3    sleep    d3     duty     d7 
05:00  2       41      0       0       100      sleep    d3    sleep    d3     duty     d7 
06:00  2       41      0       0       100      sleep    d3    sleep    d3     duty     d7 
07:00  2       41     60       0        40      meal     d2    meal     d2     duty     d7 
```

Sixty on alpha watch and forty on the others because all nineteen named crew stand alpha — true to
the series, and something S5 will have to live with at the first watch change.

## What S1 does not do

It is not in the game yet (S2). Fatigue is tracked but has no effect. Injury, death and assimilation
are states the core carries and respects, with nothing yet that causes them (S6–S8). Crew are located
by deck, not by room. The generated crew have placeholder names. Every figure is in
`docs/lore-ledger.md`, most of them invented, and many of the canonical ones recalled rather than
checked.
