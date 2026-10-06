# Evidence: the environment, in the world

Date: 2026-10-06. The environment-in-the-world task, on `feat/environment-in-the-world` cut from the
trunk `feature/g3-reactive-crew`. The ship's condition was modelled in detail and invisible in the
world; this closes that gap for **gravity** and for **a breach and its force field**.

The task's contract is `docs/ship-systems.md` (Life support: per-deck atmosphere, gravity and
temperature; force fields), and the pattern is the one S6 already set: `SyncDamage` makes a damaged
system spark where it is worked, `SyncFire` makes a burning deck smoke and flame, and this adds the
same shape for the environment rather than a second mechanism. Gravity is the engine's own per-person
mechanism (`ps.gravity` plus `SVF_CUSTOM_GRAVITY`, as `g_active.cpp` and `g_target.cpp` use it,
read in the engine source at `/home/c/big/git/upstream/efgame/src/game/`).

## What is built

**Task A — gravity, per person.**

- `ship::GravityScale(s, deck)` and `ship::ScaleGravity(world, scale)` in `module/ship/ship_core.*`
  are the arithmetic: the world's gravity scaled by the deck's plating (`Deck::gravity`, 0 freefall ..
  1 standard). At full plating the world's value stands, which is what hands gravity back to the
  engine. Unit-tested in `tests/ship` (`TestGravity`).
- `SyncGravity` in `module/crew/g_crew.cpp` drives one person from that: it sets `ps.gravity` and
  `SVF_CUSTOM_GRAVITY` for the player on a deck whose plating has failed, and the layer's embodied
  crew float on `BS_FLY` ("Moves around without gravity", `bstate.h`). **When the plating holds again
  it clears `SVF_CUSTOM_GRAVITY`, so the world's value returns** — the case the engine's own FIXME
  says it cannot do.
- **The way back** (a judgement call, below): the player's `ship boots` gives standard gravity while
  the deck is without it, so freefall is a hazard and not a trap.

**Task B — a breach you can feel, and a field you can see.**

- The authored effects live on deck 12's generated blockout (`tools/shipmap/gendeck.py`): a
  `trigger_push` named `lwh_breach_push` aimed at a `target_position` at the hole, a `trigger_hurt`
  named `lwh_breach_hurt` (10 damage, every 0.5 s), and a thin `func_usable` brush named
  `lwh_breach_field` across the hole (texture `voyager/field_activation3`). The stitcher turns the
  two box triggers into `mins`/`maxs` point entities after the compile, as it does every trigger.
- `SyncBreach` switches them from the state the ship already keeps: hull below whole and no field →
  push and hurt live; a field over the breach → push and hurt off and the brush solid; whole deck →
  everything off. No new entity class, no new engine concept.

**The gate.** `g_env`, default 0. With it unset `Crew_EnvFrame` returns before touching anything, and
no save field was added, so the ship behaves and saves exactly as before.

## What was observed, not asserted

The unit test, and `scripts/test.sh` / `scripts/check.sh` exiting 0:

```
$ /tmp/.../test_ship_core
ship_core: all checks passed

$ scripts/test.sh
all checks passed

$ scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts ; echo $?
ALL PASS
    the 8 rejections are exactly the known, documented set
0
```

The merged ship builds and passes the structural check; the two box triggers are the only entities
the new authoring added:

```
$ scripts/build-ship.sh
/tmp/tmp.XXXX/maps/voyager.bsp         OK                                 (1/17 lumps empty)
    496 entities added to /tmp/tmp.XXXX/maps/voyager.bsp
wrote /home/c/big/git/long-way-home/build/ship/out/longway_voyager.pk3
```

**Gravity** (`scripts/gravity-check.sh`), one headless run on `map voyager`, then the same with the
extension off:

```
$ scripts/gravity-check.sh
==> gravity, g_env 1
    SHIP: gravity test: standing at (-3480 -3360 -37744)
    SHIP: gravity test: deck 12 plating at 0%
    ENV: deck 12 plating at 0%; the player floats on 0 gravity
    SHIP: gravity test: floating: ps.gravity 0, custom 1, z -37743, on floor 0, crew floating 1
    ENV: magnetic boots on; the player has standard gravity
    SHIP: gravity test: boots on: ps.gravity 800, custom 0
    ENV: deck 12 plating at 0%; the player floats on 1 gravity
    SHIP: gravity test: boots off: ps.gravity 1, custom 1; deck recovers
    ENV: deck 12 has hold again; gravity is standard
    SHIP: gravity test: recovered: ps.gravity 800, custom 0, crew floating 0
==> gravity, g_env 0 (the extension off)
    SHIP: gravity test: floating: ps.gravity 800, custom 0, z -37768, on floor 1, crew floating 0
    SHIP: gravity test: boots on: ps.gravity 800, custom 0
    SHIP: gravity test: boots off: ps.gravity 800, custom 0; deck recovers
    SHIP: gravity test: recovered: ps.gravity 800, custom 0, crew floating 0
PASS  a deck's plating scales one person's gravity; the boots and a recovered deck give it back; the gate holds
```

Read: with the gate on the player's own gravity is 0 and the engine's custom flag is set — z does not
change over 1.5 s (the player does not fall); one crew member on the deck floats on `BS_FLY`; the
boots restore 800 and clear the flag; the deck recovering restores 800 and clears the flag — the
FIXME case. With the gate off none of it runs: the engine's own value (800) stands, the player lands
(z falls the 24 units to the floor), and no `ENV:` line is printed.

**A breach and its field** (`scripts/breach-check.sh`):

```
$ scripts/breach-check.sh
==> breach, g_env 1
    SHIP: breach test: standing at (-3480 -3360 -37744)
    SHIP: breach test: deck 12 hull 0.00, air 1.00, minutes of air 3.8
    ENV: deck 12 is breached; the air is going (push and hurt on, field off); the push throws (310 0 288)
    SHIP: breach test: field off, thrown: health 93, velocity (310 0 248), at (-3371 -3360 -37650)
    SHIP: breach test: field off: health 86, velocity (0 0 0), air 0.99, minutes of air 3.7
    ENV: a force field is up on deck 12; the air holds
    SHIP: breach test: field on: health 86, velocity (0 0 0), minutes of air -1.0, published field 1
==> breach, g_env 0 (the extension off)
    SHIP: breach test: field off, thrown: health 100, velocity (0 0 0), at (-3480 -3360 -37767)
    SHIP: breach test: field off: health 100, velocity (0 0 0), air 0.99, minutes of air 3.7
    SHIP: breach test: field on: health 100, velocity (0 0 0), minutes of air -1.0, published field 1
PASS  a breach pushes and hurts where it is authored; the field stops it and holds the air; the gate holds
```

Read: the breach switches on the authored push and hurt. The push throws the body toward the hole —
velocity (310 0 248), and the origin moves from x −3480 to −3371 (east, toward the hole) — and the
hurt volume takes health from 100 to 93 to 86. The air is going (minutes of air 3.7). Raising the
field prints that the air holds, the push and hurt stop (velocity 0, health holds at 86), the
published clock returns −1, and the field brush is on — `build/g3-home/baseEF/screenshots/lwh_breach.tga`
is the visible LCARS containment display it renders. With the gate off the ship state is the same but
the world does nothing: health stays 100, velocity 0, and no `ENV:` line.

## What I could not verify

- **Whether any of it reads as being aboard a dying ship.** That is the owner's walkthrough, and it is
  still the check that closes this. A transcript can say the player floats and the air goes; it cannot
  say the deck feels like one that is failing.
- **Per compartment, not per deck.** The model has no per-compartment atmosphere (the engine has no
  pressure concept at all, and the design models it per deck), so the authored breach compartment
  stands for deck 12 as a whole. One breach effect per deck.
- **The field's animation and its reading at a distance** were not judged; the screenshot shows the
  texture present and lit, not that it looks like a force field to a person.

## Judgement calls, named as calls

1. **The way back is magnetic boots.** This engine has no zero-gravity movement (recorded in
   `docs/outside-the-ship.md`), and the run shows the floating player is not on the floor
   (`on floor 0`), so they cannot walk. Rather than floor the per-person gravity, which would defeat
   the effect, the player gets `ship boots`: standard gravity for them while the deck is without it,
   cleared when the deck recovers. It is a console/panel command; making it a bound key or a panel
   button is a later UI call.
2. **The push aims at a direction marker, not the wall.** `trigger_push` computes a ballistic arc to
   its target; a target at the far bulkhead makes a lethal launch, so the target sits a short throw
   from the trigger, giving a survivable hop toward the hole. It is still aimed at the hole's
   direction. The geometry, and the hurt's 10 damage per 0.5 s, are invented and in `docs/lore-ledger.md`.
3. **The breach is deck 12's.** Environmental control and life support are deck 12's (`docs/ship-systems.md`),
   so the authored compartment is there; the placements are blockout detail and will move with the
   deck's re-dress. Triggers are entities and survive it.
4. **Crew float only the layer's own crew.** `SyncGravityCrew` touches only embodied crew
   (`shipEmbodied`) and never a script's NPC, honouring the layer's standing rule.
5. **The environment runs from the crew frame hook, gated by its own cvar.** It is called from
   `Crew_Frame` (the same frame hook `SyncDamage`/`SyncFire` use) but reads the ship directly, so it
   needs no crew to run; with `g_env 0` the hook returns before touching anything.
