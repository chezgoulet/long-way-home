# Evidence: S9 — the outside, first slice

Date: 2026-10-06. Gate S9 of `docs/ship-programme.md`. This slice is the rules, in the ship core,
evidenced by `tests/ship`. In the game it is reachable from the console (`ship chart`,
`ship jump <beacon>`, `ship fire`).

## The rules

- **A sector of twelve beacons**, its shape and contents fixed by the ship's seed: a chain that can
  always be crossed, with some shortcuts. Each beacon is empty, holds a hostile ship, a derelict to
  salvage, or the Borg.
- **A jump** goes one link, needs the warp drive delivering at least half its output, and burns
  deuterium and antimatter. A ship cannot jump out of a fight it cannot outrun — it is the same test.
- **A fight is the ship's systems against theirs.** What the phasers deliver strips the enemy's
  shields and then holes its hull. A torpedo is spent against whatever shields remain, and what is
  left of it reaches the hull. Our own shields recharge by what the shield system delivers — faster
  than an ordinary raider strips them, slower than the Borg do — and hold nothing without it.
- **What gets through lands**: on a deck's hull and on the systems stationed there, a different
  deck each time, in an order fixed by the count of hits so far.
- **With our shields down the enemy sends its party across**, once, to Main Engineering — boarders
  from a raider, drones from the Borg — and S7's and S8's rules take it from there.
- **A derelict** yields spare parts, once.

## What the tests establish

| property | test |
|---|---|
| The sector can always be crossed; links run both ways; the same seed is the same sector and another seed is another | the sector |
| A jump goes only along a link, costs fuel, and is refused with a crippled drive | the sector |
| A derelict gives 25 parts on the first visit and none on the second | the sector |
| At condition green we do the enemy no harm, our shields are down, their fire damages the ship, and their boarders are in Main Engineering | ship to ship |
| At red alert their shields fall and ours come up; a torpedo is spent; within twenty minutes the enemy is destroyed and the firing stops | ship to ship |
| With the phasers wrecked and no parts, half an hour does the enemy no harm; a torpedo is absorbed by its shields; with the launchers wrecked none can be fired | ship to ship |
| The Borg send drones: Main Engineering is marked Borg | ship to ship |
| Saved mid-fight, the ship restores and continues identically | ship to ship |

This is where the gates join up. In the first row nothing was arranged by the test beyond jumping
to the beacon: the damage, the casualties that follow from it, and the boarding fight in
Engineering are S6 and S7 running because S9 gave them something to do.

## An opponent with systems, choices, pursuit and an end (2026-10-07)

Built and tested (`TestEnemySystemsAndOutsideChoices`, console `g_shipTest 19`):

- **An opponent with systems of its own.** `Enemy` carries weapons, engines and a shield generator
  beside hull and shields; Tactical targets one (`ship target hull|weapons|engines|shields`), and a
  broken subsystem changes the fight — no weapons, no fire; no engines, no running or chasing; no
  generator, no shields coming back. More than one kind flies the sector (raider, warship, Borg
  vessel), each with its own firepower and appetite.
- **Choices at a beacon.** New beacon kinds — a trader, a distress call, and the sector's end — and
  the controls: `Trade` (parts for supplies and fuel), `AnswerDistress` (a wreck to help, or a trap),
  `Hail` (a hostile answers with its weapons), and `Disengage` (run, on the same drive test as a jump).
- **Pursuit.** A raider whose hull is going but whose engines survive breaks off and follows: it
  closes one jump each time the ship runs, catches up as a fresh fight, and while it is on the tail
  the damage-control party works at half rate. The ship cannot sit still to repair.
- **More sectors, and an end to reach.** Each sector's last beacon is its end; crossing it opens the
  next sector, and the third crossed is home (`Won`). The Conn's chart and the Operations console show
  the sector and the end.
- **To see and hear.** The Tactical console draws the contact's hull, shields, weapons and engines and
  the target; the Conn draws the chart. Engine-side, cgame now **shakes the screen on a hit** (a red
  edge flash for half a second after each hit the ship takes, read from the ship's own hit count) and
  draws a **live viewscreen** beside the panel the player stands at: an image of the contact redrawn
  every frame from the ship's state — a schematic of the ship scaled and coloured by its remaining
  hull, wrapped in a shield bubble that fades as the shields fall, with the exact fractions barred
  beneath and the targeted subsystem named. `scripts/viewscreen-check.sh` photographs it (`g_shipTest 30`);
  console `LWH: viewscreen <target> hull n% shields n%` says what it drew. (This closed a real bug:
  the glance could latch onto a panel *behind* the player and then never reach the screen; the panel
  search now requires a panel the player faces.)
  The **alert klaxon**: the condition changing plays the game's own sounds
  (`sound/ambience/voyager/redalert.mp3`, `alarm1.mp3`) around the player, so a red alert is heard as
  well as shown; console `ship klaxon red|yellow` sounds one on demand.
- **More than one contact at a time.** Some raiders have a **wingman** (`Ship.contact2`): both fire on
  the ship, the second is drawn in the tactical readout, and when the primary is destroyed the wingman
  becomes the primary. Console `ship wingman`. Tested in `TestMultipleContacts`.

## What S9 still needs

- The viewscreen is a **drawn schematic** of the contact (hull/shields bars and a ship-shaped image),
  not the engine rendering the contact's model to a surface: the renderer has no render-to-texture
  path, and the panel-image seam (`patches/0014`) uploads a module-drawn image, which is what this
  uses. What remains, if the owner wants a photographic image, is an engine render-target extension.
- Every rate is invented and in the lore ledger.
