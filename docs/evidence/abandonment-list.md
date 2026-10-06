# Evidence: the ship's written-off list (the abandonment north star as a user interface)

Date: 2026-10-06. Task 2 of `BRIEF.md`, and the first conflict named in
`docs/story-and-semantics.md` ("*the abandonment list is the headline claim and it does not exist*").
`docs/damage-and-budgets.md` asks the ship to carry a written list of what she has given up, so the
player can walk past the sealed hatch and remember why. Verified against the committed tree before this
work: there was no such list in `module/ship/ship_core.h`, none in the save, and no `SAVE_VERSION` that
carried one. This is a read over decisions the simulation already makes, not a new subsystem.

## What is built

**State.** `Ship` gains a bounded `losses` vector of `LossEntry { time, kind, system, target, what,
who }`: ship time, the kind of loss (`LOSS_SEALED`, `LOSS_STRIPPED`, `LOSS_UNINHABITABLE`,
`LOSS_WRITTEN_OFF`), whether the thing given up is a compartment (a deck) or a system, its target,
its name, and **who decided**. Save format is now **version 41** (the changelog comment names it).

**Writes.** `WriteOff(s, system, target, kind)` is the one moment the ship gives something up. It is
idempotent -- a thing is on the list once -- and bounded: past `LOSS_MAX` (24) the **oldest** entry
falls off. It is reached from the console (`ship writeoff <deck|system-name> [sealed|stripped|
uninhabitable|written]`) and from the command console's `W` key, which writes off the deck the cursor
is on. The author is `CommandingOfficer(s)`.

**Reads.** `ship losses [kind]` prints the list newest-first, optionally filtered to one kind -- the
same shape as the log's `ship log [count] [scope]`. `Publish` exports it for the screens as
`lwh_ship_losses` (`when|kind|what|who;`) and `lwh_ship_loss_count`.

**Rendering, through the existing panel path.** The command console
(`module/ui/ui_lwh_command.cpp`) draws a `GIVEN UP` column from `lwh_ship_losses`, one row per entry.
The live status panel (`module/ship/lwh_panel.cpp`) paints `GIVEN UP <n>` / `NOTHING GIVEN UP` on the
wall, so the list is visible **without entering a special mode**. No new renderer; both are extensions
of paths already there.

**Our call (recorded).** The design says "a read over decisions the simulation is already making", but
the simulation's deck `controller` (`CTRL_SEALED`, `CTRL_UNINHABITABLE`) is *derived every tick* and
reverses when the air or a field returns -- writing it automatically would list a deck that was
resealed a minute later. `WriteOff` is therefore a deliberate act (the player's signature, as
`docs/story-and-semantics.md` puts it), not an automatic hook on the derived controller. The four
kinds are the design's own vocabulary, and the author is whoever commands because "everyone aboard
remembers who made the call".

## What was proven

The unit test `TestWrittenOffList` (`tests/ship/test_ship_core.cpp`) covers the write, the read-back,
the refusal of an invalid target or kind, the idempotency, the bound and the eviction order, and a
save/load round trip. It is part of the suite, which prints:

```
$ cmake --build <build> && <build>/test_ship_core
ship_core: all checks passed
```

The test binary also carries a reproducible evidence mode, `--losses`, which writes four things off,
reads the list back, and reloads it:

```
$ <build>/test_ship_core --losses
PASS  day 0 08:00  [sealed       ] deck 9        decided by Kathryn Janeway
PASS  day 0 08:00  [stripped     ] phasers       decided by Kathryn Janeway
PASS  day 0 08:00  [uninhabitable] deck 5        decided by Kathryn Janeway
PASS  day 0 08:00  [written off  ] deck 12       decided by Kathryn Janeway
PASS  reloaded: 4 entries survive save and reload, newest deck 12 (Kathryn Janeway)
```

The bound and eviction order are checked in the test: writing off every compartment and every system
(15 + 18 = 33 entries) leaves `LOSS_MAX` = 24, the oldest 9 (decks 1..9) evicted first, and the
newest entry retained.

The **console read** is observed in the game, headless, through the module's own harness:

```
$ xvfb-run -a scripts/run-engine.sh +set g_ship 1 +set g_shipTest 48 +map tour/deck04
SHIP: --- what the ship has given up ---
SHIP: day 0 08:02 [sealed] deck 9: decided by Kathryn Janeway
SHIP: day 0 08:02 [stripped] phasers: decided by Kathryn Janeway
SHIP: day 0 08:02 [uninhabitable] deck 5: decided by Kathryn Janeway
SHIP: day 0 08:02  [uninhabitable] deck 5  (decided by Kathryn Janeway)
SHIP: day 0 08:02  [stripped] phasers  (decided by Kathryn Janeway)
SHIP: day 0 08:02  [sealed] deck 9  (decided by Kathryn Janeway)
```

The first three lines are the module reading its own list back; the last three are the `ship losses`
console command itself, newest first. Exit status 0.

**The listed files compile and link into the game modules.** The engine-side edits were built with the
pinned upstream:

```
$ cmake --build ../upstream/efgame/build-linux -j$(nproc)
[ 18%] Built target efui
[100%] Built target efgame
```

`libefui.so` carries `module/ui/ui_lwh_command.cpp`; `libefgame.so` carries `module/ship/lwh_panel.cpp`
and `module/ship/g_ship.cpp`. Exit status 0.

## The scripted checks

```
$ scripts/test.sh
==> ship simulation: unit tests
ship_core: all checks passed
...
all checks passed
```
Exit status **0**. The tail is `all checks passed`; the full run shows the crew and ship unit tests, 44
tool tests, the Python syntax pass, shellcheck, and 15 well-formed patches.

```
$ scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts
==> validator negative tests
...
ALL PASS

==> compiler corpus pass (every shipped script: compile, then read back)
    2024 files: 2016 compiled and read back, 8 rejected by the compiler, 0 read-back failure(s)
```
Exit status **1**, unchanged by this work and pre-existing: the GDK's 2,024-file script corpus contains
8 files the official compiler rejects (the three named in `docs/evidence/g0-script-compiler.md` plus
five that are not scripts), which `docs/gates.md` already records. Every check that bears on this
change passes: `check.sh` runs `test.sh` first (which passes), and its entity-dictionary stage and all
validator negative tests report `ALL PASS`. Before this run the dictionaries were missing from
`../upstream` (the copy `scripts/fetch-gdk.sh` line 91 makes); restoring that copy was the only
environment fix, and it is a build-directory operation, not a repository write.

## What is left

- **In a real session.** The panel line and the console column are drawn by the existing panel path and
  the command screen, both of which compile and link; the console read is observed headless above. The
  *rendered* panel line needs a renderer to see, which a headless run does not have -- the standing
  session caveat, not a new one.
- **Deck persistence** is the carry-file round trip (`Ship_Shutdown` packs, `Ship_Init` unpacks); the
  unit test proves the pack/unpack itself, and the deck change uses no other path. It is not separately
  observed in a multi-map session.
