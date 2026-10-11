# The residue list — the evidence

The brief (`BRIEF.md`, this pass) takes the whole-project review (`docs/audit-whole-project.md`, dated
2026-10-10 at `testing` `0ea6fa5`) and asks **which of its findings still stand on this tree**, because
several passes landed after the review was written. A finding that has since been fixed is a closed row
that still reads as open — the defect class this pass is about. Each item is answered *stands* /
*landed since* / *partly*, corrected in the review in place (the finding kept, the closing added), and
the ones that stand are closed here.

Everything below is the output of the command named beside it. This is a static host: **no engine ran,
no game data was touched.** Nothing that needs the engine is attempted; where a claim would need one, it
is named as unverified.

## The census

| # | the review's finding | verdict on this tree | what decided it |
|---|---|---|---|
| §2.2 (1) | `scripts/run-engine.sh` exits 0 even when the engine dies | **stands** — closed by this pass | the code: `set +e; … | tee "$LOG"; echo "exit: ${PIPESTATUS[0]}"` printed the status but never returned it |
| §2.3 (2) | thirty checks assert a path, not a picture | **partly landed since** — 21 of 33 now assert the picture, **12 still assert a path** | `python3` census over `scripts/*-check.sh`: 21 call `tools/frames/frame.py`; 12 do not |
| §2.7 (3) | thirty-six ignored return values | **stands, re-derived** — 51 on this tree; **3 load-bearing fixed, 48 left** | the call-site scan below; the audit's counts were "indicative" and five commits stale |
| §2.7 (4) | two silent catches, `dressdeck07.py` / `dressdeck14.py` | **partly** — 14's is live and fixed; 07's is unreachable (corrected) | running both tools on a synthetic map with a malformed light value |
| §2.5 (5) | `data/posts/deck04.posts.json`, a committed file nothing reads | **stands** — closed by this pass | `git grep` proves no reader; the file is removed and the generator is the source |
| §2.8 (6) | five dress copies; `KeyByName` three times, divergent | **stands** — census recorded, not refactored (the brief says so) | the census below |
| §3 (7) | hard-coded scratch paths | **partly** — two fixed, one left (guarded) | `grep -rn '/tmp' tests scripts tools` |

**Sections the review left open that other passes have since closed** (named for the record, not
re-opened here): §2.1 `recall` claimed twice, the second branch dead — fixed by `feat/the-console-ladder`
(`0170a4d`, "walk the dispatch ladder, and make both `recall` branches reachable"); §2.4
`appearance-check` printing `PASS` while skipping half — fixed by `feat/the-frame-assertions`
(`adc0e45`), it now exits 3; §2.6 four answers to the save version — fixed by `feat/the-number-owners`
(`55da7c9`), and `scripts/docs-check.sh` now reports *save-version: `module/ship/ship_core.h` = 56; the
owner defines it*.

## 1. The launcher's exit code (the load-bearing one)

**Before.** `scripts/run-engine.sh` captured the engine's status from the pipe and printed it, but the
last command was the `echo`, so the script always exited 0:

```sh
set +e
"$ENGINE" … | tee "$LOG"
echo
echo "exit: ${PIPESTATUS[0]}"        # printed, never returned
```

**After** (`scripts/run-engine.sh`):

```sh
set +e
"$ENGINE" +set fs_basepath "$BUILD_DIR" +set fs_homepath "$HOME_DIR" "$@" 2>&1 | tee "$LOG"
status=${PIPESTATUS[0]}
echo
echo "exit: $status"
exit "$status"
```

**A run that cannot succeed, with a stub engine that exits 7** (no engine binary is built on this host,
so the engine is a stub whose only job is to return a status; the launcher is what is under test):

```
$ FAKE_EXIT=7 scripts/run-engine.sh --build-dir /tmp/opencode/residue-demo/build \
      --engine /tmp/opencode/residue-demo/bin/fake-engine +map does_not_exist
…
fake engine running: +set fs_basepath … +map does_not_exist

exit: 7
$ echo $?
7
```

**A run that can succeed, the stub exiting 0:**

```
$ FAKE_EXIT=0 scripts/run-engine.sh --build-dir /tmp/opencode/residue-demo/build \
      --engine /tmp/opencode/residue-demo/bin/fake-engine +map voyager
…
exit: 0
$ echo $?
0
```

**A caller's `set -e` now catches it** — the dependent line must not run:

```
$ cat /tmp/opencode/residue-demo/caller.sh
#!/usr/bin/env bash
set -e
FAKE_EXIT=7 /home/robot/lwh-numbers/scripts/run-engine.sh --build-dir /tmp/opencode/residue-demo/build \
    --engine /tmp/opencode/residue-demo/bin/fake-engine +map does_not_exist
echo "THIS LINE MUST NOT PRINT: the caller continued past a dead engine"

$ /tmp/opencode/residue-demo/caller.sh
…
exit: 7
$ echo $?
7
```

The line after the call did not print. **`docs/audit-whole-project.md` §2.2's "a launcher that always
exits 0 makes every other check here decoration" is answered**: `scripts/g3-measure.sh:81`
(`if ! … run-engine.sh …`) can fire for the first time, and every check that pipes the engine's log keeps
its own grep. The callers that wrap the launcher in `timeout … || true` are unaffected — they already
treated a non-zero exit as "the run is over, read the log".

## 2. Checks that assert a path, not a picture

The review said "thirty". The frame-assertions pass classified the 23 places the review named and
corrected the count in `docs/frame-assertions.md`. **The number this pass was asked for is what is left**,
counted by the checks' own text: a check is *path-only* when it runs `[ -f … ]` on a screenshot (a `.tga`
literal, or a variable it assigns a `.tga` path) **and** does not call the frame instrument.

The census (reproducible: for each `scripts/*-check.sh`, does it assert a screenshot path, and does it
call `tools/frames/frame.py`?):

```
scripts that reference a written frame: 33
assert the picture (call tools/frames/frame.py): 21
    appearance, computer, damage, deck, deck06, deck07, deck12, deck13, deck14, emergency,
    every-screen, meeting-place, order, personal-log, playerbody, power, room-survey, s2, s3, s4,
    s4-panel
assert a path only (a [ -f … ] on a screenshot): 12
    borg-deck, borgfx, breach, fire, gaps, gravity, meeting-overlay, reachability, s4-glance,
    s9, screens, viewscreen
```

**Twelve remain.** Four of them (`breach`, `fire`, `gravity`, `viewscreen`) are the review's own
finding, sharpened by the frame-assertions pass: their frames **do not carry the claim** (a force field
against a near-black compartment; smoke against a dark deck; a number not a picture; a draw whose trace
is timing-fragile), so the register deliberately left them on the log line. The other eight
(`borg-deck`, `borgfx`, `gaps`, `meeting-overlay`, `reachability`, `s4-glance`, `s9`, `screens`) are the
"wider class" the register named as remaining. **This pass converts none of them** — the brief says not
to — and reports the number instead. Whether a check that now sees its picture sees the *right* picture
is the owner's eye, and only his.

## 3. Ignored return values

The review said "thirty-six", on a stale clone, and flagged its own count as "indicative". Re-derived on
this tree: every call to a `ship::` function declared `bool` (and the order dispatchers), whose result is
the whole expression statement and is discarded. **51 call sites before this pass, 48 after.**

**Fixed — the load-bearing, silent ones.** Three discards of `ship::Unpack` (a save that failed to parse
was not noticed at the call site; the parse left a value unchanged):

| site | what a silent failure did |
|---|---|
| `module/ship/g_ship.cpp:2345` (model test) | `back` stayed default-constructed; the "reloaded ship classifies the same input" line was a claim about an empty ship |
| `module/ship/g_ship.cpp:2696` (voice-in test) | the typed path ran from the live ship, not the pre-input snapshot, so the byte-identical claim compared the wrong states |
| `module/ship/g_ship.cpp:2714` (voice-in test) | `keep` stayed default-constructed; both forms were then replayed from an empty ship |

Each now captures the result and prints a named `FAILED` line when it is false, so the log says *why* the
check fails instead of leaving a wrong comparison to be read as a right one. No behaviour changes on a
successful parse.

**Left, and why.** The remaining 48, grouped:

- **cannot fail as called, or the `false` means "already so"** (36 sites): `SetAirSource` ×4 (decks
  9–11, valid source), `SetSurgicalField`, `SetMobileEmitter`, `SetAirponics`, `SetPhaserYield` ×2
  (valid enum), `WriteOff` ×3 (valid deck/system/kind), `Sicken` ×6 and `Cure` ×6 (the `false` is
  "already has it" / "does not have it" — idempotence, not an error), `KillCrew` ×4 (the `false` is
  "already dead"), `TakeBrief` ×2 (the `false` is "the queue is empty", and both sites are
  drain-until-empty / best-effort), `VoiceRetire` ×6 (best-effort retire), `ResolvePromise` ×2 (the
  `false` is "already resolved").
- **can fail, but nothing downstream depends on the result** (12 sites): `Hail` (`false` = nothing to
  hail — but the console prints no line either way; **recorded, not changed**, see judgement call 3),
  `RevokeBand` (the sibling `accept`/`refuse` above it *do* report; this one does not — recorded),
  `TractorHold`, `TransportBack`, `ShuttleBayHit`, `RebuildShuttle`, `TakeSurvivors`, `CacheRendered`,
  `OrderEvacuate`, `OrderTriage`.

The full 48, by file and line, is the output of the call-site scan described under **what could not be
verified** (it is a heuristic: it finds whole-expression-statement discards and does not type-check).
**This pass fixes only the save-parse class**, as the brief scopes it.

**The 48 left, as found** (`file:line: function: statement`):

```
module/ship/g_ship.cpp:1575  SetAirSource      ship::SetAirSource( vessel, 9, ship::SRC_AUXILIARY );
module/ship/g_ship.cpp:1596  SetAirSource      ship::SetAirSource( vessel, 9, ship::AIR_GRID );
module/ship/g_ship.cpp:1597  SetAirSource      ship::SetAirSource( vessel, 10, ship::SRC_BATTERIES );
module/ship/g_ship.cpp:1598  SetAirSource      ship::SetAirSource( vessel, 11, ship::SRC_BATTERIES );
module/ship/g_ship.cpp:1694  VoiceRetire       ship::VoiceRetire( voiceMixer, ship::PROD_CUE_PLAYER, cue );
module/ship/g_ship.cpp:1698  VoiceRetire       ship::VoiceRetire( voiceMixer, ship::PROD_RENDERER, line );
module/ship/g_ship.cpp:1706  VoiceRetire       ship::VoiceRetire( voiceMixer, ship::PROD_CUE_PLAYER, cue );
module/ship/g_ship.cpp:1721  CacheRendered     ship::CacheRendered( voiceRender, job.key, ship::VoiceCachePath( … ) );
module/ship/g_ship.cpp:1861  TakeBrief         ship::TakeBrief( vessel );
module/ship/g_ship.cpp:2456  VoiceRetire       if ( slow.deferral >= 0 ) ship::VoiceRetire( m, ship::PROD_CUE_PLAYER, slow.deferral );
module/ship/g_ship.cpp:3427  TransportBack     ship::TransportBack( vessel, &note );
module/ship/g_ship.cpp:3466  OrderEvacuate     ship::OrderEvacuate( vessel, 11 );
module/ship/g_ship.cpp:3701  KillCrew          ship::KillCrew( vessel, 10, "a hull breach on deck 9" );
module/ship/g_ship.cpp:3811  OrderTriage       ship::OrderTriage( vessel, 1 );
module/ship/g_ship.cpp:3837  WriteOff          ship::WriteOff( vessel, false, 9, ship::LOSS_SEALED );
module/ship/g_ship.cpp:3838  WriteOff          ship::WriteOff( vessel, true, ship::SYS_PHASERS, ship::LOSS_STRIPPED );
module/ship/g_ship.cpp:3839  WriteOff          ship::WriteOff( vessel, false, 5, ship::LOSS_UNINHABITABLE );
module/ship/g_ship.cpp:4770  SetPhaserYield    ship::SetPhaserYield( vessel, ship::YIELD_STUN );
module/ship/g_ship.cpp:4772  SetPhaserYield    ship::SetPhaserYield( vessel, ship::YIELD_VAPORIZE );
module/ship/g_ship.cpp:5806  RevokeBand        if ( !Q_stricmp( a, "revoke" ) ) ship::RevokeBand( vessel, officer, … );
module/ship/g_ship.cpp:5841  SetSurgicalField  ship::SetSurgicalField( vessel, !Q_stricmp( b, "on" ) || ( !b[0] && !… ) );
module/ship/g_ship.cpp:6170  Hail              else if ( !Q_stricmp( cmd, "hail" ) ) { ship::Hail( vessel ); Publish(); return; }
module/ship/g_ship.cpp:6203  TractorHold       if ( ship::InCombat( vessel ) ) { ship::TractorHold( vessel ); Publish(); return; }
module/ship/g_ship.cpp:6327  TakeBrief         ship::TakeBrief( vessel );
module/ship/g_ship.cpp:6656  ShuttleBayHit     ship::ShuttleBayHit( vessel, static_cast<float>( atof( a ) ) );
module/ship/g_ship.cpp:6665  RebuildShuttle    else { ship::RebuildShuttle( vessel, … ); gi.Printf( … ); }
module/ship/g_ship.cpp:6671  TakeSurvivors     ship::TakeSurvivors( vessel, atoi( a ) );
module/ship/g_ship.cpp:6733  SetMobileEmitter  ship::SetMobileEmitter( vessel, Q_stricmp( b, "off" ) != 0 );
module/ship/g_ship.cpp:6740  SetAirponics      ship::SetAirponics( vessel, Q_stricmp( b, "off" ) != 0 );
module/ship/ship_core.cpp:1069 Sicken           Sicken(s, i, COND_EXHAUSTED, "hasn't slept enough", …);
module/ship/ship_core.cpp:1071 Sicken           Sicken(s, i, COND_HUNGRY, "the galley is empty", …);
module/ship/ship_core.cpp:1073 Sicken           Sicken(s, i, COND_HYPOXIC, "the deck has no air", …);
module/ship/ship_core.cpp:1075 Sicken           Sicken(s, i, COND_AFRAID, "the ship is at battle stations", …);
module/ship/ship_core.cpp:1077 Sicken           Sicken(s, i, COND_GRIEVING, "lost someone and still carries it", …);
module/ship/ship_core.cpp:1080 Sicken           Sicken(s, i, COND_MEDITATION_DUE, "the meditation cycle, unkept", …);
module/ship/ship_core.cpp:1082 Cure             if (c.fatigue <= 0.15f || a == ACT_SLEEP) Cure(s, i, COND_EXHAUSTED);
module/ship/ship_core.cpp:1083 Cure             if (fed) Cure(s, i, COND_HUNGRY);
module/ship/ship_core.cpp:1084 Cure             if (!airless) Cure(s, i, COND_HYPOXIC);
module/ship/ship_core.cpp:1085 Cure             if (s.alert != ALERT_RED) Cure(s, i, COND_AFRAID);
module/ship/ship_core.cpp:1086 Cure             if (Trauma(c) < 0.40f) Cure(s, i, COND_GRIEVING);
module/ship/ship_core.cpp:1087 Cure             if (!(c.fatigue > 0.60f && a != ACT_SLEEP)) Cure(s, i, COND_MEDITATION_DUE);
module/ship/ship_core.cpp:4037 ResolvePromise   ResolvePromise(s, i, true);
module/ship/ship_core.cpp:4096 KillCrew         KillCrew(s, s.player, cause.empty() ? std::string("wounds") : cause);
module/ship/ship_core.cpp:4388 KillCrew         KillCrew(s, holder, st.cause && st.cause[0] ? st.cause : "the Caretaker's array");
module/ship/ship_core.cpp:4529 ResolvePromise   if (s.clock > p.deadline) ResolvePromise(s, i, false);
module/ship/ship_core.cpp:5587 KillCrew         if (result == RISING_DIED) KillCrew(s, o.candidate, cause);
module/ship/ship_core.cpp:8018 VoiceRetire      if (out.cue >= 0) VoiceRetire(m, PROD_CUE_PLAYER, out.cue);
module/ship/ship_core.cpp:8030 VoiceRetire      if (out.cue >= 0) VoiceRetire(m, PROD_CUE_PLAYER, out.cue);
```

**The three fixed, for completeness:** `g_ship.cpp:2345` (now `const bool reloaded`),
`g_ship.cpp:2696` (now `const bool restored`), `g_ship.cpp:2714` (now `const bool kept`), each followed by a
named `FAILED` line when false.

## 4. The silent catches in the deck-dressing tools

**`tools/shipmap/dressdeck14.py` — live, and fixed.** The light value was rewritten inside a
`try/except ValueError: pass`, so a malformed source light was swallowed and the brush kept its
unreadable value. Demonstrated by running the tool on a synthetic map whose source carries
`"light" "not-a-number"`.

**Before** (the tool at `HEAD`, no warning; the map still carries the bad value):

```
$ python3 tools/shipmap/_old14_demo.py --deck11 …/deck11-min.map --out …/out14old 2>old14.err
  wrote …/out14old/deck14.map
$ cat old14.err
                                    (nothing)
$ grep -A3 '"classname" "light"' …/out14old/deck14.map | grep -m1 '"light"'
"light" "not-a-number"               # the brush's light is unchanged, silently
```

**After** (the fixed tool, with the value on stderr and the floor written):

```
$ python3 tools/shipmap/dressdeck14.py --deck11 …/deck11-min.map --out …/out14 2>new14.err
$ cat new14.err
dressdeck14: light 'not-a-number' at origin '-3400 -3400 -3926.88' is not a number; using the floor 40
$ grep -A3 '"classname" "light"' …/out14/deck14.map | grep -m1 '"light"'
"light" "40"
```

The fix says so on stderr, counts it in the report (`light_value_bad`), and writes the floor rather than
keep the unreadable value — a wrong number that cannot be told from a correct one is worse than a
reported one.

**`tools/shipmap/dressdeck07.py` — the finding, corrected.** The review's description ("a malformed
light value leaves the brush's light unchanged, silently") is **false for 07 on this tree**: the parse
above the catch already sanitises the value (`except ValueError: before = 0`), making the inner
`int(before * 0.7)` unable to raise — the catch was **unreachable dead code that read like a guard**.
The real silent parse in 07 is the one above it, which read a malformed light as 0 with no word. Both are
now made visible (stderr + `light_value_bad`), and the dead catch is removed:

```
$ python3 tools/shipmap/dressdeck07.py --deck11 …/deck11-min.map --out …/out07new 2>new07.err
$ cat new07.err
dressdeck07: light 'not-a-number' at origin '-3400 -3400 -3919.2' is not a number; reading it as 0
```

07's output map is unchanged (the malformed value became 40 before and after), so there is no behaviour
change for a readable source; only the silence is gone.

## 5. The committed generated file nothing reads

**The search that proves it is unread.** On the tree before this pass (`HEAD~1`), the only mentions of
the file anywhere are the review that names it — no reader:

```
$ git grep -n -E 'deck04\.posts|posts\.json' HEAD~1
HEAD~1:docs/audit-whole-project.md:88:`data/posts/deck04.posts.json` is tracked by git, produced by `tools/posts/gen-posts.py`, and **no file in
HEAD~1:docs/audit-whole-project.md:89:`module/`, `scripts/` or `tools/` reads it** (verified: zero hits for `posts.json` / `deck04.posts`). It can
```

After the removal the same search finds only the documents that record the removal and the two guard
references this pass adds (the `.gitignore` line and the generator's docstring) — still no reader:

```
$ git grep -n -E 'deck04\.posts|posts\.json'
docs/audit-whole-project.md:116  … (the review, kept and corrected)
docs/evidence/the-residue.md:…   … (this file)
docs/gates.md:979                … (the ledger entry)
$ git grep -n 'data/posts'
.gitignore:29:data/posts/
tools/posts/gen-posts.py:25:The output is generated and is never committed (`.gitignore` guards `data/posts/`)…
```

`tools/posts/gen-posts.py` takes an explicit `--json out.json`; nothing writes to `data/posts/` and
nothing opens the file. It is removed (`git rm data/posts/deck04.posts.json`), `.gitignore` gains
`data/posts/`, and the generator's docstring now says its output is not committed. The generator is the
source.

## 6. The census the brief asked for (no refactor)

**The dress copies.** Five parallel programs — `tools/shipmap/dressdeck{06,07,12,13,14}.py` — **3,415
lines** (the review said "~3,400 where ~700 would do"). **38 helper names are shared by two or more
tools**; 18 of them are byte-identical across every file that has them, 20 diverge:

```
identical (18): _arrival _shell _wall add_box aslines brush_corners brush_tex census clampf
                compress_origin compress_z contains fmt in_core model named_brush transform_line
                xform_brush
divergent (20): __init__ _as_detail _blocked _clear_len _console _copy_brushes _copy_room
                _door_and_lobby _find_open _fixtures _lighting _tube _waypoints _wiring add
                floor_at main point rep w
```

**`KeyByName`, three times, three behaviours** (the review's "the duplication that bites"):

| where | maps | divergence |
|---|---|---|
| `module/ui/ui_lwh_command.cpp:535` | up, down, left, right, enter | **no `escape`** |
| `module/ui/ui_lwh_engineering.cpp:666` | up, down, left, right, enter, **escape** | adds `escape` |
| `module/ui/ui_lwh_meeting.cpp:621` | up, down, enter, escape, backspace | **drops `left`/`right`**; adds `escape`/`backspace` |

So a key that works on one screen silently does nothing on the next. **Not refactored** — the brief says
the fix here is the named list.

## 7. The hard-coded scratch paths

`grep -rn '/tmp' tests scripts tools` finds three (the review's three):

- `tests/ship/test_ship_core.cpp` used `/tmp/opencode/lwh-order-voice` — **fixed** to `"voice"`, the
  relative path the other seven `vr.dir` sites in the same file already use. Nothing in that test writes
  to the directory (`PlanOrderAudio` only queues), so a fresh clone did not in fact *trip* on it, but it
  named a harness-private path for no reason.
- `tests/tools/test_voice_render.py` used `/tmp/cache` — **fixed** to a path derived from
  `tempfile.gettempdir()`, so the test depends on no host's `/tmp` layout.
- `scripts/run-engine.sh` probes `/tmp/sysroot/usr/lib/x86_64-linux-gnu` — **left**. It is a guarded
  fallback (`[ -d "$candidate" ] && …`): absent, it is skipped, so a fresh clone cannot trip on it, and
  it is a real fallback for the playtest host's rootless dependency tree. Changing it would break the
  host it exists for.

**Which a fresh clone would trip on: none of the three, in fact.** Two were nonetheless harness-private
or host-specific names and are fixed; the third is a deliberate, guarded fallback.

## The checks

- `scripts/test.sh` — **all checks passed** (crew, ship, tools including `test_frames`, hooks, api,
  orders, harness, docs, python, shellcheck, patches: 20).
- `scripts/check.sh --source-map /tmp/gdkout/BaseEf/maps/voy1.map --script-corpus /tmp/opencode/gdk2/scripts`
  — **exit 0**: entity dictionary 318 classes, validator negative tests all pass, corpus 2,024 files
  (2,016 compiled and read back, the 8 known rejections).
- `scripts/docs-check.sh` — **exit 0** (paths resolve; owned numbers agree; register rows agree).

`PATH` carries `/home/robot/.cache/lwh-venv/bin` first, or `test.sh` fails for a missing `cmake` rather
than a broken test.

## What could not be verified

- **Whether a check that now sees its picture sees the *right* picture.** The frame instrument proves
  legible / changed, not right. Only the owner's eye closes that. It applies to all 21 converted checks
  and to the 12 that remain on a path.
- **The engine** never ran here: no engine binary is built on this static host. Every engine-facing
  change in this pass — the three `Unpack` sites in `module/ship/g_ship.cpp` — is **not compiled by
  `scripts/test.sh`**, which builds only `module/ship/ship_core.cpp` and `module/crew/crew_core.cpp`.
  The launcher's exit-code demonstration uses a stub engine. The `g_ship.cpp` edits are textually sound
  (the symbol and pattern are used elsewhere in the same function) but **unbuilt here**, and the checks
  that read those harness cases (`model-call-check`, `voice-in-check`) are engine checks and did not
  run.
- **The dress tools' effect on the real maps.** The silent-catch demonstration uses a synthetic map
  carrying one malformed light; the real deck sources have not been re-dressed here (they need the GDK
  and the map toolchain).
- **The ignored-return scan is heuristic.** It reads every `module/ship/*.cpp` call to one of the 150
  `bool`-returning functions declared in `ship_core.h` (plus the order dispatchers) and keeps the ones whose
  result is a whole expression statement; it does not type-check, and it counts intentional discards the same
  as oversights. Its count (51 before this fix, 48 after) is the current derived number, not a target.

## Judgement calls, named as calls

1. **The engine's status is the launcher's status, not `tee`'s.** `PIPESTATUS[0]` is returned; a failed
   `tee` alone does not fail the launch. A launch that cannot write its log is a different finding and is
   not one the audit made.
2. **"What is left" for item 2 is counted as 12** — by the checks' own `[ -f … ]` text against a
   screenshot, minus the instrument. The four behavioural checks (`breach`, `fire`, `gravity`,
   `viewscreen`) are among the 12: they *do* assert a path and *must*, because their frames do not carry
   the claim. They are a closed answer, not a remaining task.
3. **`Hail` and `RevokeBand` were left.** Both can return `false` and both then print nothing, where
   their siblings (`trade`, `distress`, `run`, `cross`; `accept`, `refuse`) report. That is a genuine
   silent no-op in the console, but it is **feedback, not data integrity**, and this pass scoped the fix
   to the save-parse class. Named here so the next pass has the list.
4. **The `dressdeck07` finding was corrected, not just fixed.** The review's mechanism was wrong for 07;
   the row keeps the finding and gains the correction, per the register rule.
5. **The `data/posts/` guard is a `.gitignore` line, not a deleted directory.** The generator's `--json`
   is explicit; the guard stops the next contributor recommitting the output.
6. **The census is a census, not a refactor.** Items 2 and 6 were answered with numbers and named
   divergences, because the brief forbids converting the rest and refactoring the copies in this pass.
