# The wall-clock exit, and the duplicate console registrations — the evidence

This pass closes two things in the same neighbourhood:

- **O14 — the wall-clock exit is not wired to the engine's quit path** (`docs/omissions.md`, the row
  the sweep wrote; `docs/evidence/clocks-and-exits.md` named it as the remaining work in its own
  words: *"The engine's quit path calling `Suspend` is not wired here and is named as the remaining
  work, not implied to work."*).
- the residual **"~30 `Cmd_AddCommand: … already defined` on each level transition. Harmless, but it
  means the SP path's console registrations are not idempotent"** (`docs/gates.md`, Residuals).

Everything below is the output of the command named beside it, on the playtest host (`sasquatch`), on
`feat/the-exits` cut from **`testing@ce35aa0`**, with the module and the engine rebuilt from this tree
before any check ran:

```
$ cmake --build ../upstream/efgame/build-linux -j"$(nproc)"      # the game module (our logic)
$ cmake --build build-engine -j"$(nproc)"                        # the engine (the two patches)
[100%] Built target efgame
[100%] Built target longwayhome
```

The engine rebuild is named because it is the one this repository's notes warn about: the binary beside
which the bridge `dlopen()`s its modules was relinked, and `build-engine/libefgame.so` still points at
the module just built (checked, not assumed).

**One thing this pass found that the brief did not expect:** the count is **38**, not ~30, and two of
those 38 are not duplicate registrations at all — they are the same real name collision, once per load,
that the noise was hiding. That is the finding the brief asked for ("say whether that reading survives
being looked at"), and it is below.

## 1. The wall-clock exit — why this is an engine patch

`quit` is the engine's own console command. `Cmd_ExecuteString` finds it in the command table and runs
it *before* it ever offers the line to the game module (that offer is patch `0006`'s, and it is later in
the function), so the module cannot answer for a quit on its own. The exit therefore has to be a seam in
the engine's quit path, and the seam has to reach the module through the one thing the engine can call
(the SP bridge):

- `patches/0023` — `qcommon/common.c`: `Com_Quit_f` calls `SP_EngineQuit()` before `VM_Forced_Unload_Start()`,
  and takes the two shutdown reasons first, into locals, because the hook re-tokenises the command line.
- `patches/0023` — `sp/sp_bridge.cpp`: `SP_EngineQuit()` hands the module `ship quit` and does nothing
  when no module is running.
- `module/ship/g_ship.cpp` — the decision, and `g_shipQuit` (0 by default). Nothing about what a quit
  *means* is in the engine.

`ship quit` is not a player's command: it is dispatched before the console ladder (`Svcmd_Ship_f`,
`gi.argv(1)`), it is deliberately absent from the console API enumeration
(`module/ship/console_api.def`, `scripts/api-check.sh`), and it is the engine's voice, not the
computer's.

## 2. The quit, our mode — and the cvar

`scripts/exits-check.sh` runs four quits and a level transition. Each quit is a real engine quit — the
`quit` command on the command line, taken through `Com_Quit_f` — because a run that is `timeout`-killed
never reaches the quit path at all (the host's engine has no interactive console:
`sys/con_passive.c`'s `CON_Input` returns `NULL`, so a piped `quit` is silently discarded; that was
measured here before anything else was tried, and is why every run below ends with `+quit`).

```
$ scripts/exits-check.sh
==> suspend
==> default
==> ironman
==> retail
==> transition
    quit: the ship is left standing; the run is marked (left standing 1)
    quit: the log carries: day 0 08:02 [command] Kathryn Janeway: the ship is left standing; she keeps her own time
    quit: the simulation is shut down; no time passes while the game is closed
    quit: ironman will not suspend the world; the ship keeps her own time
    registrations on the transition: 0 message(s) (was 38)
PASS  holodeck suspends on quit and names what Suspend wrote; the default shuts the simulation down;
      ironman refuses; the retail quit is unchanged; a level transition announces no registration
```

**What `Suspend` wrote is shown, not asserted.** The mark (`left standing 1`) and the entry it appends
to the official log, in the engine's own output. The unit side of the same act is pinned in
`tests/ship` (`TestTheQuitExit`, added by this pass): one entry, scope `command`, under
`CommandingOfficer`, and a second exit writes nothing more.

```
$ scripts/test.sh                     # its "ship simulation: unit tests" section
  left standing: mark 1, 1 entry: [command] Kathryn Janeway: the ship is left standing; she keeps her own time
  ship_core: all checks passed
```

**The cvar's default is off, and off is the old behaviour.** The `default` run above sets no value at
all — `g_shipQuit` is `0` in the code — and the run is shut down, which is what quitting did before the
exit existed. `1` is the other exit. Both are supported and the player chooses (`docs/ship-model.md`).

**Ironman refuses.** `Suspend`'s guard is `MaySuspend` (holodeck only), alongside `SavesAllowed`, and the
quit path does not have its own opinion: the `ironman` run is refused in the engine's own words.

## 3. The retail half, measured against the un-wired engine

The claim is not "a retail quit is probably fine"; it is that with the module answering nothing, the
retail quit is the same bytes. The comparison below is the same command — `+set g_ship 0 +efsp borg1
+wait 400 +quit`, a campaign map, the module loaded and live, `g_ship 0` — run on the engine with the
two patches reverted and on the engine with them:

```
$ SDL_AUDIODRIVER=dummy timeout 150 xvfb-run -a scripts/run-engine.sh --home-dir <unwired> \
      +set s_useOpenAL 0 +set g_ship 0 +efsp borg1 +wait 400 +quit > unwired.out
$ git -C ../upstream apply patches/0023*.patch patches/0024*.patch
$ cmake --build build-engine -j"$(nproc)"
$ SDL_AUDIODRIVER=dummy timeout 150 xvfb-run -a scripts/run-engine.sh --home-dir <wired> \
      +set s_useOpenAL 0 +set g_ship 0 +efsp borg1 +wait 400 +quit > wired.out
$ F='0x[0-9a-f]+|Prewarmed|^(module|engine|data|writes to|log|exit):'
$ diff <(grep -vE "$F" unwired.out) <(grep -vE "$F" wired.out)
6c6
< …/homes/un-retail/baseEF
---
> …/homes/w-retail/baseEF
105d104
< Cmd_AddCommand: messagemode2 already defined
```

Two differences and no others: the scratch home path, and the removed diagnostic (section 4). The
`----- Client Shutdown (Client quit) -----` banner, the shutdown order, the reason strings and every
other line of the retail quit are byte-identical. **The reason strings are identical because the patch
takes them first** — a hook that re-tokenises the command line would otherwise have turned `"Server
quit"`/`"Client quit"` into the leftover token, which is exactly the sort of thing this comparison
exists to catch.

## 4. The duplicate registrations — 38, and two of them are not duplicates

**Measured before anything was changed.** One SP map load, then a second (a level transition: the bridge
`dlclose()`s and reloads `libefgame.so`, and the cgame re-registers its console commands through
`CG_ADDCOMMAND`):

```
$ SDL_AUDIODRIVER=dummy timeout 200 xvfb-run -a scripts/run-engine.sh --home-dir <home> \
      +set s_useOpenAL 0 +set g_ship 0 +efsp borg1 +wait 300 +efsp borg2 +wait 300 +quit
$ grep -c 'already defined' <out>
38
```

The count on this tree is **38**, not ~30: 36 names appear once each and `messagemode2` twice, once per
load. The 36 include `kill`, `give`, `god`, `noclip`, `notarget`, `undying`, `setviewpos`,
`setobjective`, `viewobjective`, `+info`/`-info`, `+range`/`-range`, `+analysis`/`-analysis`,
`+zoom`/`-zoom`, `sizeup`/`sizedown`, `weapon`, `weapnext`/`weapprev`, `nextframe`/`prevframe`,
`nextskin`/`prevskin`, `testgun`, `testmodel`, `tcmd`, `viewpos`, `writecam`, `cam_enable`/`cam_disable`,
`lock_disable`, `saveholodeck` and `restore_holodeck_mapname`.

**Does "harmless" survive being looked at? Yes, with one exception, and the exception was invisible
inside the noise.** The handler the bridge registers is its own `SP_CgameConsoleCmd`
(`sp/sp_bridge.cpp`), which lives in the main binary and *not* in the reloaded module, so the engine's
command table never holds a pointer into unloaded code; the re-registration changes nothing (the
engine's early return is its whole effect) and the command still routes to whichever module is current.
So **36 of the 38 are noise** — the same-handler repeats on the second load. The other two are the same
message, once per load, and it is not a duplicate at all: `messagemode2` is claimed by the **client
console** (`cl_console.c`: `Cmd_AddCommand("messagemode2", Con_MessageMode2_f)`) *and* by the cgame's
table (`cg_consolecmds.cpp`: `{ "messagemode2", CG_Weapon_f }`), which is a genuine collision that
retail had too. It is left exactly as retail had it, and it is named here rather than swept up with the
rest.

**The fix is in the registration, not the caller** (`patches/0024`, `qcommon/cmd.c`):
re-defining a command with the handler it already has changes nothing, so it is silent; a definition
that *differs* is a real collision and is reported, to `developer` rather than to everyone (the same
posture as patches `0017`/`0019`/`0021`). The transition:

```
$ scripts/exits-check.sh        # the transition run
    registrations on the transition: 0 message(s) (was 38)

$ SDL_AUDIODRIVER=dummy timeout 200 xvfb-run -a scripts/run-engine.sh --home-dir <home> \
      +set s_useOpenAL 0 +set developer 1 +set g_ship 0 +efsp borg1 +wait 300 +efsp borg2 +wait 300 +quit
$ grep -c 'already defined' <out>
2                                   # one per load, and it is messagemode2
```

### Falsified, not asserted

**The registrations.** Revert `0024` alone, rebuild, run the same transition:

```
$ git -C ../upstream apply --reverse patches/0024*.patch
$ cmake --build build-engine -j"$(nproc)"
$ SDL_AUDIODRIVER=dummy timeout 200 xvfb-run -a scripts/run-engine.sh --home-dir <home> \
      +set s_useOpenAL 0 +set g_ship 0 +efsp borg1 +wait 300 +efsp borg2 +wait 300 +quit > falsify-registration.out
$ grep -c 'already defined' falsify-registration.out
38
$ git -C ../upstream apply patches/0024*.patch
```

**The quit.** Revert `0023` alone, rebuild, run the holodeck quit that suspends in the tree:

```
$ git -C ../upstream apply --reverse patches/0023*.patch
$ cmake --build build-engine -j"$(nproc)"
$ SDL_AUDIODRIVER=dummy timeout 150 xvfb-run -a scripts/run-engine.sh --home-dir <home> \
      +set s_useOpenAL 0 +set g_ship 1 +set g_shipMode 1 +set g_shipQuit 1 +efsp tour/deck04 +wait 400 +quit > falsify-quit.out
$ grep -c 'SHIP: quit' falsify-quit.out
0
$ git -C ../upstream apply patches/0023*.patch
```

That is the un-wired behaviour it replaces: the run quits, the module is never asked, nothing is marked
and nothing is written.

## 5. The suites

```
$ scripts/test.sh ; echo $?
==> patches: numbered without gaps, and each one parses
    24 patches

all checks passed
0

$ scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts ; echo $?
ALL PASS                                    (the validator's negative tests)
==> entity dictionary
parsed 318 entity classes from SP_entities.def, HM_entities-def.txt, lwh_entities.def
==> compiler corpus pass (every shipped script: compile, then read back)
    2408 files: 2394 compiled and read back, 14 rejected by the compiler, 0 read-back failure(s)
    the 14 rejections are exactly the known, documented set
PASS  the 2 unresolved references are exactly the recorded boundary
0

$ scripts/s5-check.sh ; echo $?
PASS  through 4 meals and 4 spells between them, the crew embodied were the crew the ship had aboard
PASS  most of the crew embodied walked to their place on the deck
0

$ scripts/damage-check.sh ; echo $? ; scripts/emergency-check.sh ; echo $?    # the four world-effect checks
PASS  a damaged system sparks where it is worked, in the world
0
PASS  the emergency lighting state comes on at battle stations and stands down on all four decks; the gate holds
0
$ scripts/fire-check.sh ; echo $? ; scripts/breach-check.sh ; echo $?
PASS  a burning deck is embodied as smoke and flame across the deck
0
PASS  a breach pushes and hurts where it is authored; the field stops it and holds the air; the gate holds
0
```

`scripts/test.sh` now reads **24 patches** (0023 and 0024 joined the series) and still exits 0 with the
new unit test in it. The one failure in the first sweep of these suites was this pass's own: the four
documents above cited `docs/evidence/the-exits.md` before it existed, and `scripts/docs-check.sh`
refused them (class 1, a cited path that does not resolve) — which is the check working.

## What could not be verified

- **The mark surviving the restart it is meant to describe.** `Suspend` sets `leftStanding`, and the
  save carries it (version 43, round-tripped in `tests/ship`); but a quit writes no save, so a run that
  quits and is then loaded from an older save loses the mark with the hours. Whether a quit should write
  one is a save-policy question this brief put out of scope ("no new systems"; "saves must replay
  identically"), and it is named here as the open piece rather than folded into the claim. What is
  claimed, and shown, is the exit: the engine asks, the module marks, and the record carries the entry.
- **A quit from the in-game menu or the console by a person.** The runs above take the command line's
  `quit`, which reaches the same `Com_Quit_f`; a person pressing the menu's quit is the standing session
  caveat (as in `docs/evidence/clocks-and-exits.md`).
- **A retail *multiplayer* (cMod) quit.** Mode 2 is a different client (`scripts/run-cmod.sh`, the
  inherited cMod build); it does not load our engine or module, so the patch cannot reach it, and it was
  not run. The retail claim measured here is the single-player campaign path on this port's engine.
- **A quit while a level is loading.** The bridge answers nothing unless `g_spActive`, which is the
  honest boundary: a quit mid-load is a retail quit.

## Judgement calls, named

1. **The exit is a cvar, and it is off by default.** `g_shipQuit 0` means the simulation is shut down —
   the fourth case of `docs/ship-model.md`, not a clock — and 1 means the ship is left standing. The
   extension policy's rule 3 asks for off-by-default; here it also matches the design, which says both
   exits are supported and the player chooses. It is the design's choice made explicit rather than
   inferred from which map was loaded.
2. **The engine patch takes the shutdown reasons into locals.** That is a change to released code shape
   in `Com_Quit_f`, made deliberately: the hook re-tokenises the command line, and without it a retail
   quit's shutdown banner would say `quit` instead of `Server quit`/`Client quit`. The alternative — the
   bridge saving and restoring the tokeniser state — hides the coupling instead of stating it. The
   comparison in section 3 is the proof that the strings survive.
3. **`patches/0024` silences a message in every mode, and that is named as the rule-3 call.** The
   console output changes for retail too (that line is gone). What does *not* change is the command
   table: the duplicate registration already returned without doing anything, so the state after
   registration is bit-identical, and the only thing removed is a diagnostic that described a no-op.
   Gating it behind a cvar would have made the residual's removal opt-in, which is the opposite of
   closing it. The real collision it was hiding is reported under `developer` and named above.
4. **`ship quit` is the engine's command, not the computer's.** It is dispatched before the console
   ladder so the ladder's walk (`tools/console/verbs.py`) and the console API enumeration do not carry
   it; that is intended, and it is why `scripts/api-check.sh` still reads 131 verbs. A person who types
   `ship quit` gets the exit taken, without the quit — odd, harmless, and written down here.
5. **The check is a fifth `*-check.sh` script** (`scripts/exits-check.sh`), not a case in
   `scripts/clock-check.sh`: the clocks check proves the model, and this proves the engine seam, and the
   two fail for different reasons. It is not in `scripts/test.sh`, which by its own rule runs only what
   needs no game data.
