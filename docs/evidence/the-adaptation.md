# Evidence: canon's twelve-shot adaptation, with the ramp inside it (C-15)

`docs/designs-against-code.md`, **C-15** — the finding this closes: *"Adaptation is a counter per
weapon: canon's phaser adapter lets at most twelve shots penetrate before the Borg adapt"*
(`docs/ship-systems.md:155`) against the code's continuous accumulator
(`s.enemy.adaptation = std::min(1.0f, s.enemy.adaptation + ours * (2.0f + s.borgAwareness))`), with no
shot counter and no twelve-shot cap. The owner's ruling, recorded 2026-10-12: **build canon's counter,
with the ramp shaping it inside.** Built on `feat/the-adaptation`, cut from `testing`, 2026-10-12, on the
engine/playtest host `sasquatch`.

**Observed, and the command that produced it.** Every figure below is the output of a command named
beside it, on this tree.

## The canon line, quoted with its ledger source

`docs/research/raw/Phaser_adapter.txt`: *"Using the adapter, phasers were set on a rotating modulation,
to allow at most twelve shots to penetrate Borg shielding before they adapted."* (`TNG` "The Best of Both
Worlds, Part II"; `docs/lore-ledger.md`). The twelve is canon and stays; the code's ramp is ours.

## What was built, and how the two compose

The adapter is a **count**, with the **ramp inside the bound** — two jobs, kept apart, per weapon:

- **The count is the bound.** `BORG_ADAPT_SHOTS = 12`. Each weapon — the phaser bank and the torpedo
  launchers separately, `Enemy::adapt[ADAPT_PHASER|ADAPT_TORPEDO]` — counts the shots that get through;
  the twelfth adapts *that weapon*, and after it the weapon does nothing (`AdaptAttenuation` returns 0).
- **The ramp is the shape.** `Adapter::ramp` rises by `BORG_RAMP_STEP` (per penetration), steepened by
  `borgAwareness`; each shot's effect falls by `1 − ramp`. So a weapon is not binary at shot twelve: it
  is twelve shots along the ramp.
- **The composition, said plainly, because it is not two free shapes.** The ramp is *capped* at
  `BORG_RAMP_CEIL = 0.9`: it can never reach full immunity on its own, so **only the count can adapt a
  weapon**. Absent a counter-play the two move together (the ramp is a monotone function of the count);
  what separates them is that a rotation moves the ramp and leaves the count, and a raid clears the
  count. That is the whole composition: count bounds, ramp shapes, and nothing but the count (or a raid)
  finishes a weapon.
- **A shot is one discharge.** A torpedo is one shot. The phaser is a beam with no shot of its own, so
  its shot is a **discharge**: `PHASER_SHOT_SECONDS = PHASER_MINUTES × 60 / 12 = 30 s` of firing, twelve
  to a nominal six-minute engagement — so the twelve-shot bound lands on the balance the code already had
  (`PHASER_MINUTES`).
- **Counter-play pushes both.** `Remodulate` moves every weapon's **ramp** down 0.5 (the count stands, so
  it buys damage per shot, not more shots). `RaidVinculum` clears every weapon's **count and ramp** (the
  crew get their twelve back) and suppresses growth for 300 s.

## The counter, demonstrated: twelve shots, the twelfth adapting the weapon

```
$ tests/ship/test_ship_core --adapt
== the adapter: canon's twelve shots, with the ramp inside the bound
  a Borg at red alert, unshielded; one phaser discharge every 30 s
    discharge  1: counted  1, ramp 0.08, damage 0.083, hull 0.917
    discharge  2: counted  2, ramp 0.17, damage 0.076, hull 0.840
    discharge  3: counted  3, ramp 0.25, damage 0.069, hull 0.771
    discharge  4: counted  4, ramp 0.33, damage 0.062, hull 0.708
    discharge  5: counted  5, ramp 0.42, damage 0.056, hull 0.653
    discharge  6: counted  6, ramp 0.50, damage 0.049, hull 0.604
    discharge  7: counted  7, ramp 0.58, damage 0.042, hull 0.562
    discharge  8: counted  8, ramp 0.67, damage 0.035, hull 0.528
    discharge  9: counted  9, ramp 0.75, damage 0.028, hull 0.500
    discharge 10: counted 10, ramp 0.83, damage 0.021, hull 0.479
    discharge 11: counted 11, ramp 0.90, damage 0.014, hull 0.465
    discharge 12: counted 12, ramp 0.90, damage 0.008, hull 0.457  ADAPTED -- the bank does nothing more
    discharge 13: counted 12, ramp 0.90, damage 0.000, hull 0.457  ADAPTED -- the bank does nothing more
    discharge 14: counted 12, ramp 0.90, damage 0.000, hull 0.457  ADAPTED -- the bank does nothing more
```

The twelfth discharge lands; from then the bank does nothing, and the count stays at twelve.

## The ramp inside the bound, demonstrated: two shots in and twelve shots in differ in *how*

```
== the ramp inside the bound
  two shots in:    counted 2, ramp 0.17, hull 0.840
  twelve shots in: counted 12, ramp 0.90, hull 0.457 (adapted)
```

Two shots in the weapon is barely adapted (ramp 0.17, most of each shot lands); twelve shots in it is
adapted (ramp 0.90 and the count binds). Same weapon, same count of shots — different *how*, not only
*whether*.

## The counter-play, demonstrated: a rotation moves the ramp, a raid clears the count

```
== the counter-play: a rotation moves the ramp, a raid clears the count
  six shots in:  counted 6, ramp 0.50
  remodulated:   counted 6, ramp 0.00 (the count stands)
  vinculum raid: counted 0, ramp 0.00 (the count is cleared; suppressed 300 s)
```

`Remodulate` moves the ramp and leaves the count at six — the remaining six shots now land harder.
`RaidVinculum` clears the count, giving the twelve back, and while it holds the bank still lands but the
Collective learns nothing (`TestTheAdapter`: the hull falls, the count is held). Both push against
something, and they push against different things. `TestTheAdapter` asserts both, and `TestCounterPlay`
still asserts the cooldown and the raid's cost. The world side, on this host:

```
$ scripts/counterplay-check.sh
==> counter-play
SHIP: counterplay test: adaptation 60% before remodulation
SHIP: modulation rotated; enemy adaptation now 10%
PASS  the phaser modulation can be rotated, breaking the Borg's adaptation so the shots land again
```

## The falsification: remove the cap, and the check catches it

The bound is enforced in three places (the charge gate, the discharge loop's guard, and the charge reset
once adapted). All three were removed — count unbounded, charge banked regardless — rebuilt, and the
suite run:

```
FAIL  credentials, the brig, grief, the career and the Borg's adaptation: cubeHull > raiderHull (line 1941)
FAIL  credentials, the brig, grief, the career and the Borg's adaptation: cube.enemy.adapt[ADAPT_PHASER].ramp > 0.0f (line 1942)
FAIL  credentials, the brig, grief, the career and the Borg's adaptation: cube.enemy.adapt[ADAPT_PHASER].penetrations > 0 (line 1943)
FAIL  the adapter: twelve shots adapt a weapon, and the ramp shapes them: s.enemy.adapt[ADAPT_PHASER].penetrations == BORG_ADAPT_SHOTS (line 3023)
FAIL  the adapter: twelve shots adapt a weapon, and the ramp shapes them: std::fabs(s.enemy.hull - hullAtTwelve) < 1e-4f (line 3024)
FAIL  Borg awareness and a grudge read by the simulation: cubeHull(0.9f) > cubeHull(0.0f) (line 3435)
6 check(s) failed
```

Restored, the suite exits 0. **The check can fail for the reason this pass cares about**, and does.

## Awareness: what `borgAwareness` does now

The ramp it used to multiply is inside a bound, so it can no longer shorten the twelve. It steepens the
ramp instead — the same twelve shots, shaped harder:

```
== awareness steepens the ramp, but does not shorten the twelve
  awareness 0%:  counted 12, ramp 0.90, hull 0.457
  awareness 90%: counted 12, ramp 0.90, hull 0.648 (the same twelve, shaped harder)
```

Both reach twelve; the aware Collective takes 0.46 of a hull where the cold one takes 0.54. The other
jobs of awareness (a quarter of hostiles become Borg above 0.4, and the sector turns more theirs) are
untouched. `TestAwarenessAndLoyalty` still asserts `cubeHull(0.9f) > cubeHull(0.0f)`.

## The save

The single `float adaptation` is gone; the two adapters (count, ramp, banked phaser charge) are saved on
the enemy and on the wingman. `SAVE_VERSION` moves **60 → 61**, and `tools/docs/owned_claims.json` moves
with it. Deliberate cost, named: **an older save (60) no longer loads.** The round-trip is in the
readout above (`counted 5 -> 5; ramp 0.42 -> 0.42; identical yes`) and in `TestTheAdapter`'s save checks
(plus the existing `Describe(back) == Describe(w)` round-trips in `TestCombat` and `TestSave`).

## The commands, and what they prove

```
tests/ship/test_ship_core          -> ship_core: all checks passed   (TestTheAdapter, TestCounterPlay,
                                      TestCrewJusticeAndBorg, TestAwarenessAndLoyalty among them)
tests/ship/test_ship_core --adapt  -> the readout above
scripts/test.sh                    -> all checks passed (crew, ship, tools, hooks, api, orders, harness,
                                      docs, python, shellcheck, patches)
scripts/docs-check.sh              -> register rows agree with the tree (C-15's marker resolves)
scripts/counterplay-check.sh       -> PASS  the phaser modulation can be rotated, breaking the Borg's
                                      adaptation so the shots land again (engine run, g_shipTest 41)
cmake --build ../upstream/efgame/build-linux
                                   -> libefgame.so rebuilt; ship::Remodulate is in the artifact
scripts/check.sh                   -> NAMED, NOT RUN: it needs the pinned upstream's source map and the
                                      shipped script corpus. The command is
                                      scripts/check.sh --source-map <voy1.map> --script-corpus <shipped scripts> --build-root <upstream>
```

The module artifact was asserted before the engine run: `nm -C ../upstream/efgame/build-linux/libefgame.so`
lists `ship::Remodulate`, and the library's mtime is after the edit — the engine host is the module host,
not the map host.

## The boundary — what this host could not prove

- **How the twelve *feels* in a played fight** — whether a six-minute, twelve-shot window with a
  remodulation inside it reads as the tension loop `docs/borg-incursion.md` describes — is the owner's
  walkthrough, not a number. This pass does not claim it.
- The engine run above covers the counter-play's rotation (g_shipTest 41). A world-side run that fires
  twelve *phaser discharges* and prints the count on a console is not built; the count is proved in the
  simulation (`--adapt`) and the rotation in the engine.

## Judgement calls, named

1. **What counts as a penetration.** A shot that the adaptation did not stop — i.e. that delivers any
   energy to the Borg. The enemy's *own* shields are a separate soak and do not change the count: the
   adapter measures the weapon's discharge against the Collective, not what the shield stopped. The
   alternative — count only shots that reach the hull, a "miss" being a shield-absorbed shot — is a
   different game and was not taken; it would tie adaptation to the enemy's shield state. This is the
   brief's own warning ("a miss that counts is a different game"); the line is drawn here and recorded.
2. **A phaser shot is a discharge of `PHASER_MINUTES / 12`.** The beam has no shot, so one was defined,
   and it was defined to land the twelve-shot bound exactly on the existing `PHASER_MINUTES` balance
   rather than to invent a second figure. Recorded as ours.
3. **Per weapon, and the ramp is per weapon too.** The document said "a counter per weapon" and the
   design's own trap (`docs/borg-incursion.md`) requires adaptation per weapon *type*; the code's single
   scalar could not express "the phasers are adapted, the torpedoes are not". So the phaser and the
   torpedo each carry a count and a ramp, and the scalar is gone. This is a balance change: torpedoes
   now adapt on their twelfth shot, not their second.
4. **The ramp is capped (`BORG_RAMP_CEIL = 0.9`).** Without a cap the ramp could reach immunity before
   twelve (with high awareness), the weapon would stop penetrating, and the count would stall short of
   twelve — the count would not be the bound. The cap is what makes the count the bound, and it is the
   composition.
5. **`Remodulate` moves the ramp; `RaidVinculum` clears the count.** The brief asked which; a rotation
   cannot undo a completed adaptation (a count is not a rate), which is exactly why the raid exists. A
   rotation that had nothing to break (all ramps already zero, or only adapted weapons) is refused.
6. **`borgAwareness` steepens the ramp, it does not shorten the twelve.** Canon's number is fixed and
   awareness was a rate; inside a bound a rate can only be a shape. Recorded because a reader may expect
   awareness to reduce the count.

## Files

- `module/ship/ship_core.h` — `Adapter`, `AdaptWeapon`, `Enemy::adapt[]`, `BORG_ADAPT_SHOTS`,
  `PHASER_SHOT_SECONDS`, `BORG_RAMP_STEP`, `BORG_RAMP_CEIL`, `TORPEDO_RAMP_STEP`, `SAVE_VERSION = 61`.
- `module/ship/ship_core.cpp` — `AdaptAttenuation`; `UpdateOutside` (discharges and the count),
  `FireTorpedo`, `Remodulate`, `RaidVinculum`, the save and the load.
- `module/ship/g_ship.cpp` — the counter-play test setups and the console readout, reading the adapter's
  ramp.
- `tests/ship/test_ship_core.cpp` — `TestTheAdapter`, `BorgFight`, `PrintAdaptation` (`--adapt`), and the
  updated `TestCounterPlay`, `TestCrewJusticeAndBorg`, `TestAwarenessAndLoyalty`.
- `tools/docs/owned_claims.json` — the save-version owner, 60 → 61.
- `docs/ship-systems.md`, `docs/borg-incursion.md`, `docs/lore-ledger.md`, `docs/designs-against-code.md`
  (C-15's row and marker), `docs/path-to-playtest.md` — the corrected statements.
