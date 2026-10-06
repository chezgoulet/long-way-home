# Harvest A — the Elite Reinforce fixes and tools

The mandate's first community harvest (`docs/handoff-full-scope.md` §1b) asks for a three-way diff
between `kugelrund/Elite-Reinforce`, our upstream and our module, and to port its four single-player
fixes and two authoring tools as patches in our series, cvar-gated where they offer one.

**Result: there is no delta to port. Every item is already in our pinned upstream, inherited rather
than borrowed.** The audit's premise was wrong: it checked our *module* and our *fourteen patches*,
neither of which mentions borg1, forge3 or a holodeck save — but it did not check the upstream game
source those patches sit on, and the upstream already carries all of it. `docs/handoff-full-scope.md`
says a design document wins a disagreement and the disagreement is stated; this is that statement.

## Method

- Cloned `https://github.com/kugelrund/Elite-Reinforce.git` and read its full history (55 commits).
- Located the six commits the audit named, and read each diff.
- Grepped the pinned upstream (`VoyagerSP-Android` @ `0d8942e8`, the SHA `scripts/bootstrap-upstream.sh`
  clones) for the same code.

## The comparison

| Elite Reinforce item | commit | our upstream |
|---|---|---|
| Fix the borg1 freeze | `7c66616` | `src/game/g_active.cpp:247` — the guard is present |
| Add cvar to disable the freeze fix | `d17fbe4` | `g_active.cpp:247` gates on `g_fixFreezeBorg1`; cvar declared `g_local.h:181`, defined `g_main.cpp:46`, default `"1"` at `g_main.cpp:171` |
| Better fix for the forge3 infinite loop | `051ab36` | `src/game/g_trigger.cpp:607` — `if (height < 0) height = 0` in `AimAtTarget`, exactly the "better fix"; the reverted first attempt (`338d037`, an `isfinite(dist)` test) is correctly absent |
| Fix occasional menu softlock | `9d05ff6` | `src/ui/ui_menu.cpp:3782` — `ingameFlag = qfalse` at the top of `UI_MainMenu`, exactly as committed |
| Add command to save on holodeck maps | `e70977c` | `src/cgame/cg_consolecmds.cpp:97` `CG_SaveHolodeck_f`, registered at line 178, with its `restore_holodeck_mapname` companion at 179 |
| Highlight entities with death scripts | `28af0fc`, `c505b2a` | `src/cgame/cg_players.cpp:3252` — the `cg_highlightDeathScripts` powerup highlight is present |
| Show NPC paths | `c5b0b74`, `c505b2a` | `src/game/NPC_stats.cpp:32` `NPC_ShouldShowPath`, `g_nav.cpp`/`g_navigator.cpp` `ShowPath`, cvar `g_showPaths` at `g_main.cpp:173` |

Every one of the seven lands. The borg1 fix — on the very map our own G1 mission evidence plays
(`docs/evidence/g1-sp-mission-plays.md`) — is on by default in our build, since the cvar defaults
to `1`. We were never exposed.

The two authoring tools the mandate wanted (`show NPC paths`, `highlight entities with death
scripts`) are likewise already available: `g_showPaths` and `cg_highlightDeathScripts`.

## What this means for the order

Harvest A is **closed with no code**: the work was done upstream before our pin. Harvest B (RPG-X's
entity classes) remains, and is the larger item; it is taken up separately.
