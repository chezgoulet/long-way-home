# Evidence: S10 — play modes, clocks, rank and the player, first slice

Date: 2026-10-06. Gate S10 of `docs/ship-programme.md`. This slice is the rules, in the ship core,
evidenced by `tests/ship`, with the settings exposed in the game as cvars.

## The rules

- **Play mode** (`g_shipMode`): ironman is the default and the game — the ship is saved for you and
  only forward; holodeck allows saves at will.
- **Clock** (`g_shipClock`): accelerated (a ship's day in `g_shipDayScale`-compressed game time,
  only while playing), real time (a day is a day, only while playing), or wall clock (a day is a
  day and the ship lives on while the game is closed — on load she is advanced by the time away,
  at most thirty days of it).
- **Clearance**: a crew member operates the station their department works; a lieutenant commander
  or above may take any station; the alert is called by a lieutenant or above at Engineering or
  Tactical; only the captain and first officer command. The injured operate nothing.
- **The player's role** (`g_shipRole`): any post (the player's own clearance), in command (the ship
  answers to you), or Munro, the Hazard Team's ensign.
- **Character creation** (`ship character <name> <department> <rank>`): the player takes the place
  of a generated crew member of that department who stands no station — the complement does not
  grow and nobody's post is taken — with the name chosen and a rank up to lieutenant commander.

## What the tests establish

| property | test |
|---|---|
| Ironman is the default and allows no saves; holodeck does | modes and clocks |
| A minute played is an hour aboard when accelerated, and a minute aboard in real time or by the wall clock | modes and clocks |
| Two days away: only the wall-clock ship is two days on, and has burned the fuel | modes and clocks |
| A wall-clock ship left damaged for six hours is found repaired, with the parts spent | modes and clocks |
| A year away advances her thirty days, not three hundred and sixty-five | modes and clocks |
| Who may operate which station, who may call the alert and where, who commands — for Janeway, Chakotay, Tuvok, Torres, Kim, the Doctor, Vorik | rank, clearance and the player |
| A created character replaces a generated one: complement unchanged, no station taken, clearance as their department and rank give | rank, clearance and the player |
| In command, the player may operate anything; as Munro, Tactical only, and does not command | rank, clearance and the player |
| Mode, clock, role, the created character's name and rank, and the wall-clock stamp are in the save | rank, clearance and the player |

## Wall-clock catch-up in the game (2026-10-07)

The catch-up the tests established now runs in the game module. `g_shipTest 20` sets the wall clock
and calls it:

```
SHIP: wall-clock test: before, day 0, deuterium 100%
SHIP: wall-clock test: after two days away, day 2, deuterium 99%
SHIP: wall-clock test: a year away advanced her to day 32 (capped at 30)
```

Two days away is two days aboard and the fuel is burned; a year away advances her thirty days, not
three hundred and sixty-five. On a real load the engine calls the same `CatchUp` with the wall-clock
time stored in the save (`Ship_Frame`), so this is the code path a session exercises.

## The non-station panels open the ship's screens (2026-10-07)

The panels that are not stations now open Long Way Home's own screens, from the command a map's
`target_interface` fires (`genericmenu <id>`), exactly as the station panels do:

- a **log** or **padd** terminal opens the ship's log (`ui_lwh_log`, the fourth gap's browsable
  artifact);
- a **ready room** or **command** panel opens the command console (`ui_lwh_command`, the standing
  orders);
- a **personnel** or **crew** panel opens the personnel screen (`ui_lwh_character`, character
  creation).

Checked by `scripts/s4-check.sh` (final section, `g_shipTest 23`):

```
LWH: the log7 terminal opens the ship's log
LWH: the readyroom panel opens the command console
LWH: the personnel panel opens the personnel screen
```

The ids here are the map's, and the ship's own interfaces are named the way the published decks name
them; where a map uses a different id, it is an authoring fix, not the capability.

## What S10 still needs

- **The owner's playthrough**, which the gate's exit evidence also names.

## The player's character is the body (2026-10-07)

The player's crew record and clearance were already the player's; the model was still the retail
Munro. Now `ApplyPlayerBody` (`module/ship/g_ship.cpp`) sets the player's head, torso and legs from
the crew record when a character is chosen (or the Munro role taken), through the game's own
`headModel`/`torsoModel`/`legsModel` path — the character's own face, and the uniform their
department wears (command red, science and medical blue, the rest gold; the female torso has no red
skin, so a woman of command wears the neutral cut). It runs once a map, and again on `ship body`,
`ship role` or `ship character`. The Starfleet humanoid models share one animation set, so no anim
reset is needed. `scripts/playerbody-check.sh` (`g_shipTest 33`) creates a Security character typed
`tuvok` and reports:

```
    SHIP: Tuvok Test walks as head tuvok/default, torso crewthin/gold, legs crewthin/default
PASS  the player's character is the body walked in: the head is the character's, the uniform the department's
```

A photograph is left behind, but the player is first-person, so it does not show the body; the model
swap is proven by the game accepting the names (no model-load warning) and is for the owner to see in
a third-person view. This is off entirely while the simulation is off (`g_ship 0`), so the retail
campaign is untouched.

The **career** is now reached from a UI too: the command console shows the player's character and a
field promotion, and the ship's answer is drawn on it (`lwh_ship_promote`); `scripts/promote-check.sh`
(`g_shipTest 36`) proves the confirmation reaches that cvar.

## Clearance, enforced in the game (later the same day)

Consoles now send every command under their station's name (`ship as <station> ...`), and the ship
holds it to that station's authority and to the player's clearance; the screen decides nothing and
shows the ship's refusal in red. `scripts/s4-check.sh` makes the player a security ensign and tries
four things:

```
    you are Reyes, crew number 97
    off refused at MAIN ENGINEERING: you are not cleared for this station
    alert refused at TACTICAL: calling the alert needs a lieutenant or above
    off refused at TACTICAL: that system is not operated from this station
    clearance test: sensors on, phasers off, condition 0
PASS  a security ensign operates Tactical's own systems and nothing else, and does not call the alert
```

Until a character is chosen the player is nobody in particular and is not held to a rank; a
hijacked system refuses any console. Commands typed bare at the game's own console remain a
developer's and are unrestricted.

## Ironman, enforced in the game (later the same day)

`patches/0012`: while `g_ironman` is set — which the ship simulation does unless `g_shipMode 1` —
the engine's `save` and `load` commands refuse any name but the one forward save, so the menus, a
key binding and the console are held to it alike. The game writes that save every minute. Dying
resumes it rather than the level's autosave. `scripts/s10-check.sh`:

```
    Ironman: save 'byhand' refused. The ship is saved for you, and only forward.
    Ironman: load 'auto' refused. The ship is saved for you, and only forward.
PASS  ironman refuses a save and a load by hand, and writes its one save forward
```

Not done: the save and load *menus* still offer what will be refused; a save on quitting; and
loading the ironman save a second time is not prevented (copying the file aside defeats it, as in
any game with an ironman mode).

## Orders (later the same day)

What being in command is for. Three standing orders, given only by whoever commands
(`PlayerMayCommand`), obeyed until changed, and kept in the save:

| order | effect | tested |
|---|---|---|
| repair first | the damage-control party sees to that system before any other, critical or not | with life support and the holodecks both damaged, the party goes to the holodecks |
| security to a deck | a guard of four goes there whether or not anyone has boarded | four more on the deck; when boarders arrive the guard is already fighting them |
| evacuate a deck | nobody stays: stations there are left on automation, off-duty crew go to the mess | the deck empties and the warp drive runs at half; vented with boarders aboard, the boarders die and no crew are lost; resealed and refilled, the crew return |

Nobody in particular, and an ensign, are refused. The last row found a real sequence: sent back the
moment the hull was sealed, the crew arrived on a deck with no air yet and were injured. The order
has to stand until the deck is breathable — which the test now does, and which is the captain's to
judge.
