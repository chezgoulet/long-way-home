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

## What S10 still needs

- **Enforcement: clearance and ironman are now enforced in the game** (both below). Being in command
  still issues no orders to anyone.
- **A character-creation screen**, and the player's character being the body the player walks in.
- **Orders**: in-command play needs the crew to carry out what is ordered — power priorities, alert,
  repair and security teams — which is the hierarchy the owner asked for.
- **Wall-clock catch-up has run only in tests.** The game calls it on load; no session has spanned
  a real absence.

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
