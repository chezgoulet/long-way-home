# The wrap — text is never clipped to fit, and a test that can catch it when it is

`docs/design-creed.md` (the owner's ruling: *on-screen text is never clipped to fit — it wraps*),
`docs/staff-meetings.md` (the meeting overlay, the computer addressed on it), `docs/orders.md` (the
order face), `docs/evidence/the-configurator.md` (the same defect in the start screen, 2026-10-09),
`docs/evidence/the-computer-depth.md` (the pass whose frame showed the third), and
`docs/evidence/model-call.md` (the transcript's placement relative to the engine's version stamp), on
`feat/the-wrap` (cut from `testing`), 2026-10-10.

**Observed, and the command that produced it.** Everything below is the output of a command named
beside it, on this tree, after
`cmake --build ../upstream/efgame/build-linux --target efgame --target efui`. The engine runs are
headless (`xvfb-run`, `SDL_AUDIODRIVER=dummy`); the frames are rendered, not inferred. No asset, no
model file and no generated content is committed.

The ruling had been broken three times by three passes — the configurator's description, the meeting
pills, and the computer's transcript. The transcript was the third: in the pass's own frame
(`lwh_computer_refused.tga`) the refusal read

```
>> ALERT RED  ->  THAT IS NOT YOURS TO CALL: THE ALERT IS CALLED BY A
```

and stopped at the frame edge, losing *"…called by a lieutenant or above, at Engineering or Tactical,
or by command"* — the half that explains itself. The rail's echo of the same line was clipped too.

## What was built, and where

| file | what it is |
|---|---|
| `module/ui/lwh_textfit.h` | **the one wrap**: measured, whole-word, engine-free. `LWH_WrapText` returns the lines it would draw, each measured no wider than the column it was given; a single word wider than the column is returned whole rather than clipped, and text the caller's line budget cannot hold is reported (`overflow`) rather than dropped. `LWH_MeasureFn` is the caller's own metric, so the engine passes `UI_ProportionalStringWidth` and a character count is never used as a proxy. |
| `module/ui/ui_lwh_meeting.cpp` | the transcript, the rail's echo, the rail's facts, the meeting line, the option pills, the queue's decision column, the participants line, the `DECIDED:` line and the order face's replies, situation and note now call the one wrap; the old character-count `Wrapped` and the drop-and-report `FitToColumn` are gone. The transcript budgets itself in **lines** (not turns) and drops the oldest **whole** turns, so a wrapped line never runs into the next entry's space. |
| `module/ui/ui_lwh_start.cpp` | the configurator's `DrawWrappedText`, the measured wrap the owner asked to copy, is now a thin drawing wrapper around `LWH_WrapText` — the three surfaces cannot drift apart again. |
| `module/ship/g_ship.cpp`, `module/ui/ui_lwh_meeting.cpp` | the `lwh_wrap_check` command, run from `g_shipTest 85`: it wraps every live string into the frame's own column, with the engine's own font, and prints each line's measured width. |
| `tests/ship/test_ship_core.cpp`, `tests/ship/CMakeLists.txt` | `TestTextFit` and the `--textfit` / `--textfit-unwrapped` prints: the test, over the real strings. |
| `scripts/computer-check.sh` | asserts the measured fit: no `WRAP: OVERFLOW`, and the refusal's own end is a wrapped line of both the transcript and the rail's echo. |

## Task A — the wrap, measured

**The transcript and the rail's echo.** The transcript wraps to its column `x=158..632` (474px) and
the rail's echo to `x=6..148` (142px), both with the engine's own font. The transcript's budget is the
space above the pills' floor and the engine's version stamp's clear strip (`y=441`): with four pill
rows that is four lines, spent on whole turns from the newest backward. Measured live, in the engine,
by `lwh_wrap_check`:

```
WRAP: transcript7      col 158..632   2 line(s), widest 468
WRAP:   | > alert red -> that is not yours to call: the alert is called by a lieutenant or above, at Engineering or Tactical,
WRAP:   | or by command
WRAP: rail-echo        col   6..148   4 line(s), widest 140
WRAP:   | alert red -> that is not yours to
WRAP:   | call: the alert is called by a
WRAP:   | lieutenant or above, at Engineering
WRAP:   | or Tactical, or by command
```

The refusal reads whole in both places, and **no line is wider than its column** (`overflows 0`,
below). The before/after frames are `lwh_computer_refused.tga`, `lwh_computer_session.tga` and
`lwh_computer.tga` under `build/g3-home/baseEF/screenshots/` (not committed). The after frames: the
refusal completes (`…OR TACTICAL, / OR BY COMMAND`), the rail's echo completes, and the session's
three exchanges all read whole (the status line wraps to two lines and the other two stay on one).

**The vertical budget.** A transcript entry can be taller than one line (the captain's log answer is
three lines at 474px), so the transcript is a scrollback: it shows the newest **whole** turns that fit
the line budget and drops the oldest whole turns, which is why the refused frame's transcript shows
only the last two exchanges. It never wraps a line into the next entry's space, and the pills move
down by `answerLines` as before. The full text is in the log and the console.

## The sweep — what else was looked at, and what was found

The same check ran over the surfaces the brief names. The measured columns are the frame's own.

| surface | column | finding |
|---|---|---|
| the computer's transcript | 158..632 (474) | **clipped before**; now wrapped, the refusal whole (widest 468). |
| the rail's echo | 6..148 (142) | **clipped before**; now wrapped, whole (widest 140). |
| the rail's mood fact | was 74..118 (44) | **found clipping**: `at breaking point` is 67px, cut to “at b”. The value now starts at x=34 (column 34..118, 84px) — the one rail fact long enough to clip. Named as a call. |
| the meeting line being answered | 158..632 (474) | measured now (was a character count); the authored lines fit on one line (widest 361). |
| the computer's pills / the meeting pills | label 172..350 (178), cost 356..632 (276) | **clean** for the real strings — every pill and cost fits on one line (widest cost 201). The pill block now wraps and grows a row if a future label or cost needs it instead of dropping words. |
| the queue's deciding column | 150..465 (315) | **clean** — the longest decision is 231px. Measured now. |
| the order face | pills 172..350 / 356..632; situation, replies, note 158..632 | **clean** — the derived pills, costs, situation and replies fit (longest 220px). Measured now. |
| the participants line | 158..632 (474) | measured; it shares the bottom strip with the free-text pill and the engine's stamp, a pre-existing overlap named under the calls. |
| the `DECIDED:` line | 158..632 (474) | **clipped before** for a long option; now measured (two lines). |

The order face is not live in the computer run, so its strings are covered by the unit test
(`tests/ship`, below) rather than by `lwh_wrap_check`; the rail's mood is empty on the computer's own
line (its speaker is not crew), so it too is covered by the unit test.

## Task B — the test that can catch it

The wrap is a function that returns the lines it would draw (`LWH_WrapText`), and the test asserts
that **every line it returns fits the column it was wrapped for, for the real strings** — every
meeting's pills and lines, the computer's answers and refusals, the order pills and replies, the
queue decisions, and the rail's name and mood — and not a short sample:

```
$ tests/ship/test_ship_core
  the wrap: 304 real string(s) (longest 144 chars), every line measured against its column; widest line 468
  the same assertion on the unwrapped string fails on 9 of them (so the test has teeth)
ship_core: all checks passed
```

**The test can fail, and the failure is named.** Run against the *unwrapped* string — the same
assertion with each string drawn on one line — it catches nine of the real ones and stops:

```
$ tests/ship/test_ship_core --textfit-unwrapped
FAIL unwrapped  transcript:status      628 > 474  "> status  ->  alert green, alpha watch, 141 of 141 fit; …"
FAIL unwrapped  transcript:crew        514 > 474  "> crew  ->  the complement: 141 crew, 141 fit; command 20, …"
FAIL unwrapped  transcript:dilithium   647 > 474  "> dilithium  ->  dilithium 100% (range 3000 ly, warp possible); …"
FAIL unwrapped  transcript:nav         524 > 474  "> nav  ->  75000 light years from home; …"
FAIL unwrapped  transcript-jr:alert red 562 > 474 "> alert red  ->  that is not yours to call: the alert is called by a lieutenant or above, …"
FAIL unwrapped  rail-echo              554 > 142  "alert red  ->  that is not yours to call: the alert is called by a lieutenant or above, …"
…
unwrapped: 9 of 304 string(s) overflowed (the check caught them)
```

The named failing inputs include **the longest refusals** (`alert red`, both on the transcript and in
the rail's echo) and the longest answers (`status`, `dilithium`). Run on the wrapped implementation
the same assertion passes:

```
$ tests/ship/test_ship_core --textfit
wrapped: 0 of 304 string(s) overflowed (the wrap must return zero)
```

The test's measure is a proportional stand-in (`FitTiny`, in the test), because the engine's true
metrics live in `ext_data/fonts.dat`, game data this repository may not carry. It gives narrow letters
less width than wide ones, which is the property a character count gets wrong; the absolute widths are
proved instead by `lwh_wrap_check`, in the engine, with `UI_ProportionalStringWidth` itself, over the
live strings:

```
$ scripts/computer-check.sh
WRAP: checked 20 string(s), 26 line(s), overflows 0
```

## The commands, and what they returned

```
scripts/computer-check.sh       -> PASS (including "WRAP: checked 20 string(s), 26 line(s), overflows 0",
                                   the refusal's own end a wrapped line of the transcript and the rail's echo)
tests/ship/test_ship_core       -> ship_core: all checks passed (304 real strings, widest line 468 fits its column)
tests/ship/test_ship_core --textfit            -> wrapped:   0 of 304 overflowed
tests/ship/test_ship_core --textfit-unwrapped  -> unwrapped: 9 of 304 overflowed (caught, and named)
```

## What could not be verified

- **Whether the wrapped result *reads* well** — where the breaks fall, whether a wrapped refusal still
  lands. That is the owner's, and it needs the picture: `lwh_computer_refused.tga` and
  `lwh_computer_session.tga`.
- **The pill rows growing**, and the participants line, under content longer than today's: no authored
  string needs it, so that path is exercised by the unit test's synthetic cases, not by a frame.

## Judgement calls, named as calls

1. **The transcript is a scrollback, budgeted in lines.** A single entry can exceed the frame's spare
   height, so the transcript shows the newest whole turns and drops the oldest; it does not shrink the
   text or run a line into the next entry. The full text stays in the log.
2. **The rail's mood column was widened** (its value moved from x=74 to x=34) so `at breaking point`
   fits. It is a small layout change, made because it is the same defect: text cut to fit.
3. **The participants line is left on the bottom strip** it shares with the free-text pill; wrapping
   it to two lines would run into the footer and the engine's version stamp, and it is already
   overlaid by the pill in the frames. Named, not hidden.
4. **The test's measure is a stand-in.** The repository may not carry the engine's font metrics, so
   the unit test proves the wrap's contract with a proportional measure and the engine proves the
   numbers. Both are run; neither is claimed for the other.
5. **The pills and the queue are wrapped even though today's content fits.** The ruling is about the
   mechanism, not this week's strings, so the drop-to-fit path was removed everywhere rather than
   kept in the one place a shorter string happened not to reach it.
