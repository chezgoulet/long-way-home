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

- **Enforcement in the game.** The rules exist; nothing yet applies them. Ironman does not stop a
  manual save or a load; the consoles do not ask `PlayerMayOperate` before acting; being in command
  issues no orders to anyone.
- **A character-creation screen**, and the player's character being the body the player walks in.
- **Orders**: in-command play needs the crew to carry out what is ordered — power priorities, alert,
  repair and security teams — which is the hierarchy the owner asked for.
- **Wall-clock catch-up has run only in tests.** The game calls it on load; no session has spanned
  a real absence.
