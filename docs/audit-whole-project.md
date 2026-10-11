# The whole-project review

**A dated snapshot — 2026-10-10, `testing` at `0ea6fa5`.** It records a *state*, not a current one; the ledger
(`docs/gates.md`) owns status. Three evidence streams, all read-only: **the project's own checks run against
the tree** (68 scripts, each with its own log and exit code), **a code inventory** (size, surface, duplication,
long functions, error handling), and **an audit against the six defect classes this project has actually
shipped**. The registers (`gates.md`, `omissions.md`, `walkthrough.md`, `path-to-playtest.md`) were read rather
than restated.

**Why it exists.** The owner asked for correctness, code quality, what is missing, and the foot guns — so that
the content system is built with the tools it needs rather than discovering them mid-authoring. **The single
most useful finding is in §2.1**: a capability the register says is reachable, that no player can reach, that
no guard can see. Everything else is in service of not repeating that shape while authoring content.

---

## 1. Does it work? — the checks, run

All **68 `scripts/*-check.sh`** were run on the playtest host, from `testing` at `0ea6fa5`, each with a 420-second
cap and its own log (`build/review-sweep/`, host-side), plus `docs-check` and `g3-measure`: **70 runs, 68 exit 0.**
**Both non-zero exits are the sweep's own doing, not the tree's:**

- **`deck-check` exits 1** with a usage line (0 seconds) — it takes a `DECK` argument and the sweep ran it bare.
  Given a deck it passes (verified earlier the same day against deck 14). It is a *parameterised* check and the
  driver could not know that.
- **`g3-measure` exits 124** — killed by the sweep's own 420-second cap. It is the ten-minute crew harness and
  needs roughly twenty-five minutes end to end; it passes when given the time (verified the same day).

So: **every check that was given what it needs passed.** What that does *not* mean is the subject of §2 — a
check that passes is not a check that proves what it claims, and thirty of them cannot.

## 2. The foot guns — found, and verified on this tree

### 2.1 The sharpest: a capability the register says is reachable, that cannot be reached

`recall` is claimed twice in one dispatch ladder, and the second claim is dead:

- `module/ship/g_ship.cpp:5646` — `else if ( !Q_stricmp( cmd, "recall" ) )`, the **transporter** branch, with
  **no argument guard**: it matches every `ship recall …` there is.
- `module/ship/g_ship.cpp:6021` — `else if ( !Q_stricmp( cmd, "recall" ) && a[0] && b[0] )`, the **crew-memory**
  recall, which can therefore never run.

**The consequence is not cosmetic.** `ship recall <who> <event>` — a request to read what a person remembers —
is silently served by the transporter branch, which reports the away team back aboard. It answers a different
question than it was asked, and it answers it confidently.

**And no guard can see it.** `tools/console/verbs.py` derives the verb set with `sorted(set(...))`, so a
duplicate collapses before the comparison; `scripts/hooks-check.sh` checks *presence*, not reachability, and
says so in its own header. **`docs/hook-register.md` lists `Recall` and `RecallSource` as reached from the
console — which is false.**

**The general lesson, and it is the same one as the duplicated harness case number:** *a guard that compares
two sets cannot see a name claimed twice; reachability needs a walk of the ladder, not a set.* `breach` is
claimed twice too (`:5504` and `:5779`) and both branches reach, because they are distinguished by argument
shape — so this is a fragile pattern as well as a broken instance.

### 2.2 A launcher that always exits 0

`scripts/run-engine.sh` pipes the engine into `tee` under `set +e` and then prints `exit: ${PIPESTATUS[0]}` —
**without exiting with it.** So the script's own exit status is always 0. Every caller that does
`if ! run-engine.sh; then …` (`scripts/g3-measure.sh:81`) is asking a question the script cannot answer, and
every check compensates by grepping the engine's log. **The log greps are not belt-and-braces; they are the
only signal**, and the exit-code path is a lie that reads like a safeguard.

### 2.3 Thirty checks that assert a screenshot exists, not what it shows

`[ -f "$SHOT" ]` proves a file was written. It cannot tell a lit room from a black one, a room from a corridor,
or the right screen from the previous screen still in the frame buffer. **Thirty of the checks assert exactly
that and nothing more** — among them `deck-check`, `deck06/07/12/13/14`, `damage-check`, `fire-check`,
`gravity-check`, `breach-check`, `emergency-check`, `computer-check`, `order-check`, `power-check`, `s10-check`,
`s3-check`, `s4-check`, `s4-panel-check`, `viewscreen-check`, `personal-log-check`, `playerbody-check`,
`appearance-check`.

**This is the class that cost this project a working day.** The security office rendered *pitch black* because
the meeting's post stood inside the wall, and the check for it passed — it had a file. Two checks now measure
the frame (`tools/meetings/legibility.py`, used by `meeting-place-check` and `room-survey-check`) and those are
the model for the rest: **a content-facing check should assert on pixels, not on paths.**

### 2.4 A check that passes while skipping half its claim

`scripts/appearance-check.sh:44–48`: with no `build/baseEF`, it prints **`PASS (half 1)`**, notes *"half 2
skipped: no game data"*, and `exit 0`. On a host without retail data a green `appearance-check` proves the
derivation and nothing about the rendered lineup. The disclosure is honest; **the exit code is not** — a
skipped half should not read as a passed check to anything that isn't a human reading the log.

### 2.5 A committed generated file that nothing reads

`data/posts/deck04.posts.json` is tracked by git, produced by `tools/posts/gen-posts.py`, and **no file in
`module/`, `scripts/` or `tools/` reads it** (verified: zero hits for `posts.json` / `deck04.posts`). It can
drift from the deck source permanently and silently. It is the only committed generated artifact; everything
else generated lives under the gitignored `build/`.

### 2.6 A number asserted in five places, owned by none

| where | says |
|---|---|
| `module/ship/ship_core.h:2728` (the code) | **56** |
| `docs/gates.md:100` | 54 |
| `docs/walkthrough.md:924` | "past 49" |
| `docs/handoff-live-crew-and-meetings.md:28` | `SAVE_VERSION = 12` |

**Four answers, one of them fifteen versions stale.** `tools/docs/owned_claims.json` registers two numbers —
the computer core's deck and the torpedo launchers' — so nothing checks this one. The save version is quoted
in five documents and owned by none of them, and in this project *a wrong number gets quoted back as settled*.

### 2.7 Ignored returns, and two silent catches

**36 call sites discard a status-returning `ship::` function's result.** The ones that matter most:
`ship::Unpack(...)` discarded at three sites (`g_ship.cpp:2138, 2480, 2498`) — **a save that fails to parse is
not noticed at the call site**; `Sicken(...)` six times in a row (`ship_core.cpp:1069–1080`); `WriteOff`,
`KillCrew`, `TakeBrief`, `OrderEvacuate`, `OrderTriage`, `SetPhaserYield`, `ShuttleBayHit`, `TakeSurvivors`
each discarded once or more.

Two silent catches, both the same pattern, both in the deck dressing tools:

```python
try: k["light"] = str(max(40, int(before * 0.7)))
except ValueError: pass          # tools/shipmap/dressdeck07.py:324, dressdeck14.py:305
```

A malformed light value leaves the brush's light unchanged, silently, in a generator whose output cannot be
told from a correct one afterwards.

### 2.8 Duplication — and one instance where the copies diverge

- **Five parallel copies of the deck-dressing program** — `tools/shipmap/dressdeck{06,07,12,13,14}.py`, with
  **~26 helpers existing as identical bodies in two to five files** (`fmt`, `aslines`, `brush_tex`, `contains`,
  `named_brush`, `_shell`, `_wall`, `clampf`, `compress_z`, `xform_brush`, …). ~3,400 lines where ~700 would do.
- **`KeyByName` is implemented three times with three different key maps** — `ui_lwh_command.cpp:535` (no
  `escape`), `ui_lwh_engineering.cpp:661` (adds it), `ui_lwh_meeting.cpp:587` (adds `escape`/`backspace`, drops
  left/right). **This is the duplication that bites**: the same helper name, three behaviours, so a key that
  works on one screen silently does nothing on the next.
- `Fmt` twice (`g_crew.cpp:140`, `g_ship.cpp:186`); `WriteFile` twice (`g_crew.cpp:1345`, `g_ship.cpp:127`);
  `_function_body` verbatim in `tools/console/verbs.py:37` and `tools/orders/orders.py:47`; the DejaVu font
  path triplicated across three tools.

## 3. Code quality, measured

- **`module/` is 25,497 lines**, 19,619 of it in `ship/`. `tests/` is 9,833; `tools/` 8,604 across 37 Python
  files; `scripts/` 4,866 across 80 shell scripts.
- **`ship_core.h` declares 432 functions** (429 unique; three overloaded) — the surface the hook register
  guards, now at **441 registered hooks**.
- **The giants**, and two are structural rather than incidental: `RunTest` **3,384 lines** and `Svcmd_Ship_f`
  **1,614 lines** — one function holding the whole test harness, one holding the whole console ladder. Then
  `Unpack` 486, `Publish` 468, `UpdateCrew` 413, `Pack` 317.
  **`Svcmd_Ship_f` being one 1,600-line else-if chain is precisely what let `recall` be claimed twice**: a
  ladder that long cannot be read by eye, and nothing walks it (`§2.1`).
- **Hard-coded scratch paths** in three places that will bite a second developer or a second machine:
  `tests/ship/test_ship_core.cpp` uses `/tmp/opencode/lwh-order-voice`; `tests/tools/test_voice_render.py` uses
  `/tmp/cache`; `scripts/run-engine.sh` probes `/tmp/sysroot`.
- No `TODO`/`XXX`/`HACK` markers exist in `module/` or `tools/` — the debt here is structural, not annotated,
  which is why this review had to be run rather than grepped.

## 4. The registers have gone stale — and that is their own stated defect

`docs/omissions.md` says of itself: *"A row that still reads as unbuilt after the work has shipped is the
defect this section exists to prevent."* **It has that defect, verified:**

- **O2** still reads *"The three independent power sources … are not modelled."* The mechanism is built and
  its check passed on the playtest host an hour before this was written (`scripts/air-check.sh`: a deck holding
  air on its own source with the grid down, three sources as separate state, a shortfall as a number).
- **C1, C2, C3, C4 and C5** still present as questions *"needing the owner's ruling"*. **All five were ruled on
  2026-10-08** and the rulings are in `docs/gates.md` — C1 the headroom curve (implemented), C2 *"Derived"*,
  C3 deferred to content, C4 *"Air is per deck"* (and now built), C5 permadeath as a player-chosen mode.
- **The walkthrough's G-list has the same exposure.** G13 is settled (the corrections pass) and G5's first
  slice is built (the reachability pass), and the file carries running commentary rather than a status column.
  **It needs the same row-by-row read the omissions register is getting here; this review did not complete it.**

**Nothing checks a register row against the tree.** The docs check polices *paths* and two *numbers*; a claim
of the form *"X is not built"* has no owner and no guard, which is how the register's own warning came true.

## 5. What is missing, and what the content system will need

**Holes that would bite during authoring** (worth closing *before* content, because they are the tools content
is judged with):

1. **Reachability needs a walk, not a set** — a guard over the console ladder that fails when a verb is claimed
   twice or shadowed. Without it, `recall` is not a one-off; it is the shape of the next one too.
2. **Frame-content assertions** — the thirty existence-only checks (`§2.3`). The content system's whole output
   is *places, screens and people*, and the only two checks that can tell a room from a black frame are the two
   written this week. **This is the highest-value tool before content.**
3. **A skipped check must not exit 0** (`§2.4`), or a host without data will report a green suite that tested
   half of itself.
4. **A launcher that returns its engine's status** (`§2.2`), so a crash is distinguishable from a log without
   the expected line.
5. **Owners for the numbers** — the save version first (`§2.6`), and the pattern registered in
   `owned_claims.json` so the next one is a row rather than a review finding.
6. **A register-against-tree check** (§4) — even a coarse one: for each row a document may not claim X unbuilt
   while a named marker exists.

**Holes that are content, not tooling** (registered, ordered, and not blocking the tools):

- **O15 — the nine-scenario slate**, with the rank-and-role rule (*author the event once, instantiate it
  through the post*) — the largest absence, and what the meeting, order, cast and room machinery has been for.
- **O11 — the outside's site archetypes** and the away-mission loop, and **G17**'s away site, without which the
  away mission's own acceptance cannot run.
- **O3** canon named and never modelled (gel packs, the EMH's power dependence, nacelles, cargo bays,
  replicator rations as a resource), **O4** first contact (awareness and travelling reputation), **O5** the
  live Starfleet link, **O6** the crew's bills (battle stations, damage control), **O7** the Hazard Team as a
  unit, **O13** the security squad embodied, **O14** the wall-clock exit wired to the engine's quit path.
- **W1** a dead officer's credentials, and the cast's thin voices — both deliberately deferred by the owner,
  and both recorded.

**Not holes, and worth saying so:** the client's three modes, the ship as state, the save, the meetings, the
orders, the computer, the casting, the voice mechanism, the air, and the rooms all exist and all pass their
checks. **The machinery is there. What is not there is the content, and the checks that would judge it.**

## 6. The review's own limits

- **Two of the three streams ran against a stale clone.** The code inventory and the foot-gun audit both read
  `e95fcbd` — five commits behind `testing` — so their **line numbers are approximate**. Every headline finding
  in §2 was **re-verified on the current tree** (`0ea6fa5`) before being written here; the *lists* in §2.7 and
  §2.8 (ignored returns, duplication) were not re-derived line by line, and their counts should be read as
  indicative.
- **A scripted reconciliation of the registers was attempted and discarded.** It confirmed a stale row
  (`O2`) because it ran against the stale clone, and it read `borgAwareness` as first-contact awareness and a
  comment as a mechanism. **The register reconciliation was therefore done by reading, and §4 covers the
  omissions register and names the walkthrough as unfinished rather than claiming it.**
- **Nothing here was built, fixed or changed.** No engine process was left running; the review ran the checks
  and read the tree.
- **Not measured at all:** frame timings on a real desktop, the two-machine multiplayer wire, and anything
  needing a person — the owner's own play sessions remain the only witness for feel.
