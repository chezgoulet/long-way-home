# The reachability pass — the triage, the list, and the first slice

`docs/hook-register.md` (the register and its three states), `docs/hook-triage.md` (this pass's triage),
`docs/walkthrough.md` finding **G5**, `docs/scenario-atlas.md` (what the content needs),
`docs/rising-to-the-occasion.md` and `docs/borg-incursion.md` (the acts wired). Applied on
`feat/reachability`, cut from `testing` (`962fb70`), 2026-10-09, in the clone at
`/home/robot/lwh-reachability` on **mikoa**.

**What this is.** The brief's finding was that 129 hooks are reached only from the developer console and
35 by nothing. This pass (a) triages all 164 — written down in `docs/hook-triage.md`, (b) orders the
genuine gaps, and (c) wires the top of the list: the Tactical console's combat kit and command's squad.

## Task A — the triage, and its counts

The full table is `docs/hook-triage.md`. Every hook the register marks console-only (129) or reached by
nothing (35) is classified, with a specific reason:

| category | hooks |
|---|---|
| diagnostic by design | 96 |
| genuine gap | 62 |
| simulation-raised (the register's fourth case) | 6 |
| dead | 0 |

**No hook is dead.** The six with no caller anywhere each have a reason and something waiting
(`SpeciesCapability`, `SpeciesNeed`, `SpeciesSusceptibility` want a crew record; `PromiseKindName`,
`LogVisibilityName`, `BandGrantCount` are the names and counters a planned display reads). Deletion is
not taken — it is its own decision and its own evidence.

The triage is a judgement, and the criterion is in the document. The counts are derived from it; the
full reasoned table is the deliverable, not the number.

## Task B — the prioritised list

In `docs/hook-triage.md`: the 62 genuine gaps, ordered by what a player meets first and second by what
the scenario content needs, **led by G5** (the fight). The order's reasoning is written beside it. The
top four — `SetTarget`, `Remodulate`, `RaidVinculum`, `OrderAdvance` — are the slice below.

## Task C — the first slice, wired

**What was wired.** Four hooks the register marked console-only, each a player's act in a fight,
reached now from an in-game station screen:

| hook | before | now | screen key |
|---|---|---|---|
| `SetTarget` | `ship target` only | play | Tactical `A` cycles hull / weapons / engines / shields |
| `Remodulate` | `ship remodulate` only | play | Tactical `M` |
| `RaidVinculum` | `ship vinculum` only | play | Tactical `N` |
| `OrderAdvance` | `ship advance` only | play | Command `S` sends the squad to the deck under the cursor |

**Files changed, and why each.** No `ship_core` file was touched.

- `module/ship/g_ship.cpp` — the engine host:
  - `Publish()` publishes `lwh_ship_target_idx`, so the console can draw and cycle what Tactical aims at
    (the model read `Target` was console-only; the console now reads the index the ship publishes, as it
    reads every other control).
  - `Svcmd_Ship_f`'s station gate admits `remodulate` and `vinculum` to Tactical, so the screen's
    `ship as 1 ...` is held to the station and the person exactly as every other control is (the
    two-lock model), instead of being sent unrestricted.
  - a `g_shipTest 72` harness case sets up a Borg fight and drives the console keys, so the check can
    observe it headlessly.
- `module/ui/ui_lwh_engineering.cpp` — the Tactical console's `Act()` gains `A` (target), `M`
  (remodulate) and `N` (vinculum), reusing the keys that on Operations mean distress and trade; the
  footer names them.
- `module/ui/ui_lwh_command.cpp` — the command console's `Act()` gains `S` (send the squad), and the
  footer names it.
- `tools/hooks/reach_report.py` — the curated `UI_FUNCTIONS` set (the reachability tool's list of hooks
  a screen reaches) gains the four, with the source lines named. **This is a call:** the tool keeps a
  curated list rather than deriving UI reach from the screens, so a new key must be declared here; the
  keys above are the source of truth and the tool now matches them.
- `scripts/reachability-check.sh` — a new check in the house form (the model of `scripts/screens-check.sh`
  and `scripts/fire-check.sh`): one headless run of `g_shipTest 72`, grepping the engine's own output and
  looking for the screenshot.

**Observed, and the command that produced it.** Everything below was run on this tree on mikoa.

The reachability evidence moves exactly those four, and nothing else in the four:
```
$ python3 tools/hooks/reach_report.py --detail | grep -P '^(SetTarget|Remodulate|RaidVinculum|OrderAdvance)\t'
SetTarget       1       0       0       ui,console,test
Remodulate      1       0       0       ui,console,test
RaidVinculum    1       0       0       ui,console,test
OrderAdvance    1       0       0       ui,console,test
```
Before the pass the same four read `0  1  0  console,test`; the overall totals moved `play 191 -> 195`,
`console 122 -> 118` (the tool counts the register's fourth-case members as play; see `docs/hook-triage.md`,
judgement call 4).

The engine host and the UI module build and link, with the intended entry points and nothing else:
```
$ cmake --build /home/robot/upstream/efgame/build-linux -j$(nproc)
... [5/5] Linking CXX shared library libefgame.so
$ nm -D --defined-only /home/robot/upstream/efgame/build-linux/libefgame.so | awk '$2=="T"{print $3}'
_Z10GetGameAPIP13game_import_t@@EFGAME_1.0
_Z6vmMainiiiiiiiii@@EFGAME_1.0
_Z8dllEntryPFiizE@@EFGAME_1.0
$ nm -D --defined-only /home/robot/upstream/efgame/build-linux/libefui.so | awk '$2=="T"{print $3}'
_Z8GetUIAPIv@@EFUI_1.0
$ strings libefui.so | grep -c 'ship remodulate'   # 1; likewise `ship vinculum`, `ship target`, `ship advance`
```
No `.cpp` was added under `module/`, so the configure-time-glob trap does not apply; the build was
re-run anyway and the new strings are in the built libraries.

The checks that run everywhere:
```
$ scripts/test.sh
crew_core: all checks passed
ship_core: all checks passed
Ran 54 tests ... OK
hooks-check: ... PASS  every public function is registered, and every registered hook exists
python syntax ok; shellcheck ok; 20 patches; all checks passed
$ scripts/hooks-check.sh
PASS  every public function is registered, and every registered hook exists
```

The content checks (`scripts/check.sh`) pass against the exact GDK corpus (`scripts/fetch-gdk.sh`'s
pinned `real_scripts.zip`, 2,024 files) and the shipped `voy1.map`; mikoa did not carry them, so they
were fetched and the dictionaries placed beside the upstream checkout:
```
$ scripts/check.sh --source-map /tmp/gdkout/BaseEf/maps/voy1.map --script-corpus /tmp/opencode/gdk2/scripts
validator negative tests ... ALL PASS
compiler corpus pass: 2024 files: 2016 compiled and read back, 8 rejected by the compiler, 0 read-back failure(s)
the 8 rejections are exactly the known, documented set
```

**The rendered demonstration could not be produced on mikoa, and this is stated rather than hidden.**
`scripts/reachability-check.sh` is written and would run where the retail game data is; mikoa has none
(the engine reports `"pak0.pk3" is missing`, and no `pak*.pk3` exists anywhere on the host). On a
data-equipped host the demonstration is:
```
$ scripts/reachability-check.sh
==> the Tactical console's combat kit, by key
    counterplay test: aiming at WEAPONS (a key at Tactical set it)
    counterplay test: adaptation 10% after remodulation (cooldown 120 s)
    counterplay test: vinculum suppressed for 300 s (adaptation 0%)
    counterplay test: squad ordered to retake deck 1 (a key on the command console)
PASS  Tactical's combat kit is reachable in play: a key picks the target, remodulates and raids the
      vinculum, and command sends the squad
```
That transcript is what the harness `g_shipTest 72` prints; it is quoted from the code path, not
observed on mikoa. **The owner must run it on `sasquatch` to read the rendered screen.** This is the
one acceptance item this host could not complete, and it is named.

## The register is updated

`docs/hook-register.md`:
- the four rows' `reach` cells moved from `console` to `play`;
- they left the "reached only from the developer console" list and their group counts moved with them;
- the by-reachability counts now read **play 188, console 125, nothing 35** (the four moved);
- `scripts/hooks-check.sh` still passes (it checks names, and no name changed).

The register's fourth-case text is left intact: the six simulation-raised hooks remain counted under
`console`, as the register's own call has them, and they are named in the triage as a category of their
own.

## What could not be verified

- **The rendered proof.** Mikoa carries no retail game data, so the engine cannot load a map; the
  screenshot and the check's own PASS line are not observed here. The check and the harness case are
  committed; the command is above; it must be run on `sasquatch`.
- **Whether the triage's judgements are right** — whether a hook called *diagnostic* is one a player
  would in fact want. That is the owner's, and the list to read is the 62 genuine gaps.
- **Whether the footer lines still fit the 640px face.** The Tactical and command footers were trimmed
  or extended by a few characters; the accessibility measurement (`scripts/screens-a11y.py`) needs the
  rendered screenshots, which mikoa cannot make. No footer was lengthened beyond its previous width.
- **The player at the console.** The harness drives the keys (`lwh_eng_key`, `lwh_cmd_key`), which is the
  same path a hand takes, but no human sat at the screen. That is walkthrough G8, unchanged.
- **That a fight is winnable with the kit.** The unit tests cover the hooks' effects; whether the
  counter-play makes a Borg fight good is a sitting, not a check.

## Judgement calls, named as calls

1. **The first slice is the player's *response*, not the provocation.** The brief's top item is the
   systems under stress. A player cannot deliberately board, ignite or breach — and should not: those
   are the simulation's acts. Wiring a "start a fire" key would be inventing a mechanism, which the
   brief forbids. The honest slice is the acts the player *does* take when the simulation raises them:
   what to shoot, the Borg counter-play, and the squad. The provocation is the director (G4), reported.
2. **The four hooks were already built; nothing was invented.** Only the reach changed. The ship's
   authority model is preserved: Tactical's commands go through the station gate (the two-lock model),
   and the squad is command's (`OrderAdvance` refuses anyone who does not command).
3. **`reach_report.py`'s `UI_FUNCTIONS` is curated, and was updated.** A call: the tool does not derive
   UI reach from the screens, so a new key must be declared; the source of truth is the screens, and the
   four keys are named in the code and in the check.
4. **The register's fourth case is kept, not overturned.** The tool counts the simulation-raised hooks
   as play; the register counts deliberate reach and calls them console. This pass keeps the register's
   stricter reading and names the difference, rather than quietly reconciling it.
5. **No save format, no `ship_core` file, and no existing behaviour changed.** The module links with the
   same three exports and the UI with the same one; `scripts/test.sh` is green.
