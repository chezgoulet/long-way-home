# Evidence: the player in the world

Date: 2026-10-06. Stage B, on `feat/the-player-in-the-world`, cut from the trunk
`feature/g3-reactive-crew`. The player's body was modelled (`PlayerIncapacitated`, `wounds`,
`severity`, `recovery`, `exposure`) and had no counterpart in the room: the model's causes did not
reach the person holding the controls. This closes that gap for **the player's own body**, using the
same pattern Stage A set for the environment (`SyncDamage`/`SyncFire`/`SyncGravity`).

**Note on the contract document.** The brief points at `docs/path-to-playtest.md`, Stage B. That file
is on `testing`, which this branch predates, and is not on this branch; the brief's own Goal, What to
build and Acceptance sections were taken as the contract instead.

## What is built

**The causes reach the person.** `ship::SetPlayerDeck` is the world telling the model which deck the
player's body occupies, and `Tick` uses it in place of the roster routine while the player is on
their feet (`module/ship/ship_core.cpp`). So a deck without air, a deck on fire, and the radiation of
a failing core reach the player where they stand. `ship::UseSystemBy` is `UseSystem` with the
operator named by index: a degraded console lets go at the player who worked it, not at the
station's own hand. `ship::WoundPlayer` writes the world's damage into the same record any casualty
carries, so a boarder or a weapon lands on the person the ward treats.

**The state is one the ship acts on.** `ship::AttendIncapacitatedPlayer` (called from the tick)
sends the senior medical hand, names them in the log, and the player is carried and treated by the
existing casualty path (`TreatCasualties`). A player who is down operates nothing and commands
nothing (`PlayerMayOperate`/`PlayerMayCommand`), whatever their role.

**The body in the world** (`module/crew/g_crew.cpp`, gated by `g_player`): `Crew_PlayerFrame` reads
the record and drives the player's health (`ship::PlayerBodyHealth`: fit is whole, an untreated
injury is a body losing ground, a closed record is nothing), and reads the world's damage back into
the record. `LWH_BlockRespawn` refuses the engine's death-respawn (`load *respawn`) while the
extension is on and the record is closed, and command has passed to the senior fit officer.

**The gate.** `g_player`, default 0. With it unset the frame hook returns before touching anything,
the console's `ship operate` refuses, and no save field was added: the ship behaves and saves exactly
as before.

## What was observed, not asserted

The unit test covers the player's injury path and the recovery, and the two gates:

```
$ scripts/test.sh
ship_core: all checks passed
...
==> patches: numbered without gaps, and each one parses
    16 patches
all checks passed
```

`scripts/check.sh` (the GDK validator and the script-corpus round trip):

```
$ scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts ; echo $?
ALL PASS
    2024 files: 2016 compiled and read back, 8 rejected by the compiler, 0 read-back failure(s)
    the 8 rejections are exactly the known, documented set
0
```

The patch series applies and builds from scratch (`scripts/bootstrap-upstream.sh`, exit 0):

```
==> applying 16 patch(es)
    ...
    0015-adopt-rpgx-target-shaderremap.patch
    0016-no-reload-on-death.patch
...
-rwxrwxr-x 1 c c 2166872 ... efgame/build-linux/libefgame.so
  libefgame.so : ...
```

**The player in the world** (`scripts/playerworld-check.sh`), one headless run on `map voyager` then
the same with the extension off:

```
$ scripts/playerworld-check.sh
==> the player in the world, g_player 1
    SHIP: player test: Test Player at the life support console, 40% health, 40% condition, red alert
    PLAYER: Test Player is hurt; the ship treats them as any casualty
    SHIP: player test: the console let go after 0 draws: status 1, severity 0.80
    SHIP: player test: [sickbay] The Doctor: Test Player was hurt when the life support console let go
    SHIP: player test: [sickbay] The Doctor: The Doctor attends Test Player on deck 12; they are carried to sickbay
    SHIP: player test: hurt by the console 1, attended 1; body health 20 of 100, incapacitated 1
    SHIP: player test: after a day in the ward: status 0
    SHIP: player test: recovered: status 0, body health 100 of 100
    SHIP: player test: in the airless compartment: status 1, severity 0.30, deck 12, moving 1
    PLAYER: Test Player is hurt; the ship treats them as any casualty
    SHIP: player test: left the deck: at (-3936 -2778 -13151), record deck 4, still moving 1, status 1
    SHIP: player test: in the airless compartment past the limit: status 2
    PLAYER: Test Player is dead; the body falls where it stood
    SHIP: player test: [sickbay] The Doctor: Test Player dead, no air
    PLAYER: no reload; Test Player's record is closed and the ship carries on
    SHIP: player test: dead 1, body health -999, respawn blocked 1, command "Kathryn Janeway"
==> the player in the world, g_player 0 (the extension off)
    SHIP: player test: the console let go after 0 draws: status 0, severity 0.00
    SHIP: player test: hurt by the console 0, attended 0; body health 100 of 100, incapacitated 0
    SHIP: player test: in the airless compartment: status 0, severity 0.00, deck 5, moving 1
    SHIP: player test: dead 0, body health 100, respawn blocked 0, command "Test Player"
PASS  ...
```

Read: the player stands at the life support console on a 40% grid and works it; the console lets go
at the operator (`status 1`, `severity 0.80`), the body follows the record down (`health 20 of 100`),
and the log names the operator and the hand who attended ("The Doctor attends Test Player on deck
12; they are carried to sickbay"). The ward returns the record to fit and the body to whole. In the
breached, airless compartment the player's record is injured (`status 1`, `severity 0.30`, `deck 12`)
**while still moving 1** (`pm_type != PM_DEAD`), and using the turbolift puts them on deck 4 with
`record deck 4` — they can leave. Past the air limit the record closes (`status 2`), the body falls
(`health -999`), the engine's respawn is refused (`respawn blocked 1`), and command has passed from
the lost player to `Kathryn Janeway`. With `g_player 0` none of it happens: no `PLAYER:` line, the
console refuses, the record and body are untouched, and the respawn is not blocked.

## What I could not verify

- **Whether it feels like being hurt aboard a dying ship.** That is the owner's walkthrough, and it
  is still the check that closes this. A transcript can say the record is injured and the body
  followed it; it cannot say the deck feels dangerous.
- **The engine's death screen and the real input path.** The harness was driven by the game's own
  respawn function (`LWH_BlockRespawn`) and the model directly; a human pressing attack on the death
  screen was not walked. The refusal is the same call, but the screen itself was not photographed.
- **A boarder or a weapon from a body in the world.** The mechanism that carries the world's damage
  into the record is tested and used (`WoundPlayer`), and the harness drives it through the record,
  but no hostile NPC was landed on the player and allowed to shoot them in a session. The console,
  the air and the fire are the causes shown end to end.

## Judgement calls, named as calls

1. **Death is a witness, not a succession.** The player's record closes, the body falls, and the
   engine's respawn is refused (`patches/0016`), so the dead are not reloaded. Command devolves to
   the senior fit officer (`CommandingOfficer`), whom the log now names, and the run carries on with
   the ship's record intact. The model *can* hand the chair to a successor (`AssumeCommand`, unit
   tested), but the world does not currently give the player that new body: that — whether death
   ends the evening or the player plays on as the next officer — is the owner's call, and I have not
   guessed it.
2. **The body health mapping is ours.** `100 × (1 − severity)`, floored at 1 and capped below whole,
   is invented; it only has to be monotone so the record and the body agree. In `docs/lore-ledger.md`.
3. **The player's deck is per-deck, not per-compartment.** The model has no per-compartment
   atmosphere, so "where the player is" is a deck, and a hazard fills the deck. This is the same
   limitation Stage A recorded for the breach.
4. **A downed player operates nothing.** `PlayerMayOperate`/`PlayerMayCommand` refuse while the
   record is not fit, even in command role. The alternative — an injured captain still giving orders
   from the deck — was not taken; the ship acts instead.
5. **The clock for the ward is the existing one.** Recovery is `TREATMENT_HOURS` per patient and the
   ward's own output; nothing new was added for the player, so the wait in the transcript is the
   ordinary casualty wait. The harness advances it with `ship::Sleep`, which is the sleep state's own
   fast-forward, not a shortcut past a consequence.
