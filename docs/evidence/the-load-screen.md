# Evidence: the shuttle load screen (O9)

**What this is.** `docs/omissions.md` **O9**: *"The shuttle load-screen decision menu is not built."*
`docs/shuttles.md` carries the model and names what was missing (`"Not built: the load-screen menu"`);
`docs/evidence/every-screen.md` row 18 left it *"missing, left — a launch decision menu, not a station
screen."* This pass builds it. The acceptance, from the brief: **the decision is made on the load screen
and the run reflects the choice** — demonstrated by exercising the path and reading the result.

**Host:** `sasquatch`, the engine host. Branch `feat/the-load-screen`, cut from `origin/testing`.

## The design's row is the acceptance

`docs/shuttles.md` step 2: *"the screen shows the manifest (who is going, and therefore which posts go
empty), the loadout (what comes out of stores), the destination with its **chart confidence** (so you can
see whether you are sending people somewhere you barely scanned), the return condition, and the risk.
Canon-flavoured: a pre-flight check, not a menu."* Every one of those is on the screen, and the three
that are **decisions** (the shuttle, the destination, the return condition) plus the manifest all reach
the model.

## What changed

| file | what |
|---|---|
| `module/ui/ui_lwh_command.cpp` | **`ui_lwh_launch`** — the load screen (draw + keys), reached from the command console (`L`) and driven by `lwh_launch_key`, so what a test presses is what a hand presses |
| `module/ship/g_ship.cpp` | **publishes** the screen's reads (`lwh_ship_launch`, `_dest`, `_risk`, `_crew`, `_loadout`, `_return`); the `ship shuttlereturn <class> <0\|1\|2>` command; harness case **`g_shipTest 103`** |
| `module/ship/ship_core.{h,cpp}` | `ShuttleReturnName`, `SetShuttleReturn`, `ChartConfidence`, `LaunchRisk` |
| `module/ship/console_api.def` | the `shuttlereturn` verb |
| `docs/hook-register.md` | the four new public functions |
| `scripts/launch-check.sh` | **new**: the item's own gate |
| `scripts/screens-a11y.py` | the launch screen's title and a row, measured |
| `docs/shuttles.md`, `docs/omissions.md` (O9), `docs/gates.md`, `docs/evidence/every-screen.md` (row 18) | the docs this moves |

**No save-version bump.** The return condition is a field the shuttle already carried and already
serialised (`ship_core.cpp` round-trips `returnCondition`), so a save taken with a shuttle away by a
rendezvous replays identically. Nothing new is persisted.

## What I verified by running it

`scripts/launch-check.sh` exits **0**:

```
$ scripts/launch-check.sh
==> the shuttle load screen
    SHIP: launch test: at beacon 1, the links are 0 2
    SHIP: launch test: the bay reads "0|Class 2|100|in the bay;1|Type 6|100|in the bay;2|Type 8|100|in the bay;3|Aeroshuttle|100|in the bay;"
    SHIP: launch test: destinations "0|CHARTED;2|UNCHARTED;"
    SHIP: launch test: the risk table "0|0|LOW;0|2|HIGH;1|0|LOW;1|2|HIGH;2|0|LOW;2|2|HIGH;3|0|LOW;3|2|HIGH;"
    SHIP: launch test: the loadout "10 material, 10 rations, 10 parts"
    SHIP: launch test: the ways back "0|a transporter window;1|a rendezvous;2|its own power;"
    SHIP: launch test: the screen put Type 6 away, beacon 2, 2 aboard, back by a rendezvous
    SHIP: launch test: the log: Type 6 launches for beacon 2 with 2 aboard
PASS  the load screen carries the manifest, the loadout, the destination and the risk
PASS  the screen's choice is the run: the shuttle, the destination, the manifest and the return
PASS  the destination was a choice: the screen sent the shuttle to beacon 2, not 0
PASS  the load screen rendered, and its own pixels carry the decisions
PASS  the shuttle load screen decides on the screen and the run reflects the choice
```

**The decisions, read back.** The harness started on the **Class 2** and the **first** destination, then
drove the screen: `T` (Type 6), `right` (the second destination), `A`/`down`/`A` (mark two crew), `C`
(back by a rendezvous), `ENTER` (commit). The run did what the screen said — **Type 6**, **beacon 2**
(the second, not `0`), **2 aboard**, **back by a rendezvous** — and the ship's own record carries it
(`Type 6 launches for beacon 2 with 2 aboard`).

**Both screens, measured.** `every-screen-check.sh` exits **0** with the new screen included in the
accessibility pass: the launch title ink is **32 px** (SMALLFONT 28–38) and a column label **19 px**
(TINYFONT 15–24). `test.sh`, `docs-check.sh`, `hooks-check.sh`, `check.sh` and `shuttle-check.sh` all
exit **0**.

**Which half is the engine's drawing, and which half I drove — plainly.**
- **The engine's drawing:** every pixel of the screen. The layout is our UI code
  (`module/ui/ui_lwh_command.cpp`), executed by the engine's UI module and rendered by the engine from
  the palette in `q_math.cpp`; the values it draws are published by the ship (`g_ship.cpp`'s `Publish`)
  and are reads — `ChartConfidence`, `LaunchRisk` and the loadout are the ship's, not the screen's.
- **What I drove:** the **keystrokes** — the harness's `lwh_launch_key` presses are the screen's own key
  handler, so the path is the one a hand takes; and the ship's beacon and its visited flag were set by
  the harness at the start (setup, so the destination is a real choice). The screen itself decides
  nothing: it sends the same `ship shuttlereturn` / `ship launch` the console answers.

The two rendered frames are `build/g3-home/baseEF/screenshots/lwh_launch.tga` (opened) and
`lwh_launch_set.tga` (after the manifest and the return are set), **1280×1024**, differing by **11.7 %**
whole-frame. The screen is delivered for the **owner's eye** — a pre-flight check, not a menu, is his to
judge.

## What I could not verify, and why

- **Whether it reads as a pre-flight check** — the owner's eye. The frame is delivered; I have not
  captioned it as reading well.
- **The launch as a two-lock act.** Is sending people command's? The design does not say, and the load
  screen's commit is the **existing, ungated** `ship launch`; I did not invent an authority for it. Named
  as a call below.
- **A hand on the console rather than the screen.** The `L` key opens the screen from the command
  console, but the check drives `ui_lwh_launch` directly (the screens' established pattern).

## What I deliberately left unbuilt, and why

- **The away-mission loop** (beam down, kit, scan, return, write-back) — scenario content, registered and
  named, not started. The load screen is the decision at the *moment of launch*; what happens after is
  the loop.
- **A shuttle as an away-mission vehicle distinct from the transporter** — `docs/shuttles.md`'s other
  unbuilt line, content, not this screen.
- **Per-choice greying of inapplicable options** — the ship refuses what does not apply and the screen
  shows what the model affords, as the beacon block does.

## Judgement calls, named as calls

1. **The screen is a read-and-send surface, and I did not gate the launch.** Its commit is the existing
   `ship launch`; the every-screen screens carry a two-lock, but the design never rules that launching is
   command's, so inventing a refusal would be a promise I cannot source. If the owner rules that sending
   people is command's, the gate belongs on `ship launch` and the screen shows the refusal — a small,
   named follow-up.
2. **The `brackets` bug, caught by reading the screen, not the code.** The first render drew `[X]`/`[ ]`
   and the font has no bracket glyph, so it came out `CNJ` — a *wrong character* on a control. I changed
   the mark to `+`/`-`, which the palette proves, and re-read the pixels. This is the "read the rendered
   thing" rule earning its place.
3. **The destination needed a beacon with more than one link.** The first beacon has one link, so the
   destination would not have been a *choice*; the harness stands the ship at beacon 1 (links 0 and 2)
   before opening the screen, and the check fails if the two destinations are equal.
4. **The risk is a lookup, not the screen's arithmetic.** `LaunchRisk` is the ship's, published as a
   `class|beacon|risk` table so the screen combines nothing.
5. **The loadout is the model's rule, shown.** `LaunchShuttle` takes up to 10 of each store; the screen
   shows what will leave, computed by the ship from the same rule — it does not restate it.
