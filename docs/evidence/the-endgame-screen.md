# Evidence: the endgame screen

**Session 2026-11-08.** Stage E's deferred item: *"an endgame screen (the destinations are reached by
the console and the model)"*, under `docs/endings.md`'s law *"No destination is better than another.
Endings are readings of a life, not rankings of it."*

## What was built

- **The model's rows.** `EndingScreenRows(const Ship&)` in `module/ship/ship_core.{h,cpp}` returns one
  row per destination **in the design's order**, each with the shipped name, reader, lens, whether the
  run has earned it, the table's own offer, and — when a wildcard play is possible — what a play would
  spend. The row has seven fields: destination, open, name, reader, lens, offer, play. **There is no
  rank, score or quality field, and nothing is sorted.**
- **The fixed words are shipped from the model.** `EndingScreenLabel` / `EndingScreenLabels` carry the
  screen's title, subtitle, section heads, approach words, filing question, keys and the last-frame
  line. They are produced inside `EndingToneFindings`, so the neutrality assertion scans the screen as
  well as the destination table and the readings. This is the design's "not in text, in UI, or in the
  log", enforced in one place.
- **The screen.** `module/ui/ui_lwh_ending.cpp` is the UI module's face for the rows: a title, the six
  destinations with reader and state (`EARNED`, `A PLAY`, `NOT OPEN`), the selected row's lens and
  offer/play in a wrapped detail pane, the approach row (`the grind` / `the conduit`, the second only
  when the enemy's knowledge is aboard), Earth's filing row (`FILE THE RECORD` / `HAND IT BACK
  UNREAD`), and the keys. UP/DOWN choose, LEFT/RIGHT turn the approach and the filing question, ENTER
  commits, ESC leaves.
- **It chooses; it does not conclude.** On ENTER the screen calls `UI_ForceMenuOff()` and sends
  `ship ending <id> [nofile] [conduit]` or `ship wildcard <id> [conduit]`. It never draws the reading
  and never draws the log. The arrival plays, the reading is printed, and the official log is the last
  frame — the screen is not the ending, the record is.
- **The way in.** `Ship_Frame` publishes the rows and opens the screen once when `Won` becomes true
  (`endScreenShown`), and `ui_lwh_ending` reopens it. `Publish` fills the `lwh_ship_ending_*` cvars;
  the screen reads only those, so the UI library and the game library keep their existing seam.
- **No save change.** The rows are derived; the screen's cursor, approach and filing choice are host
  state for the one-shot endgame. `SAVE_VERSION` remains **66**.

## Why it is not a score screen, in one sentence

**It lists who would read the record and what each would read — the design's order, no rank or quality
field, no score, nothing sorted — so it asks whose record the run becomes rather than how well the run
scored.**

## The command

```sh
scripts/ending-check.sh
```

It compiles the ship tests on the host, runs the suite, prints `test_ship_core --endings`, then
falsifies five ways. The screen's model and words are in that output:

```text
== the endgame screen
  SCREEN ROWS: 6 in the design's order 0 1 2 3 4 5
  SCREEN OPEN: 1 of 6; a play is offered for 3
  SCREEN RANK FIELD: none (the row is destination, open, name, reader, lens, offer, play)
  SCREEN LABELS: 11, 0 tone findings
  SCREEN EARNED: 6 of 6 open
```

The rank guard and the screen-order falsification, from the same run:

```text
==> endings: falsification four (the screen's rows reversed; the design's order must fail)
    the reversed rows fail the same assertion; the tree passes

==> endings: falsification five (a rank field planted in the screen's row; the rank guard must fail)
    the planted rank field fails the guard; the tree passes
```

The UI module was built against the engine in a separate tree, so the screen compiles and links:

```sh
cmake -S /home/c/big/git/upstream/efgame -B /home/c/big/git/upstream/efgame/build-lwh-endings2 \
  -DLWH_MODULE_DIR=/home/c/big/work/lwh-endings/module
cmake --build /home/c/big/git/upstream/efgame/build-lwh-endings2 -j4
# [100%] Built target efgame ; libefui.so also built
cd /home/c/big/git/upstream/efgame
nm -C build-lwh-endings2/libefui.so | grep LWH_UI_EndingScreens
# 000000000004c780 t LWH_UI_EndingScreens(char const*)
strings build-lwh-endings2/libefui.so | grep -F lwh_ship_ending_title
# lwh_ship_ending_title
```

## PASS lines, and what each is evidence of

- **PASS — the screen is in the design's order.** `SCREEN ROWS: 6 in the design's order 0 1 2 3 4 5`
  is `rows[i].destination == i`; `TestEndingScreen` asserts the order and that the last row is
  `DEST_NO_READER`. The fourth falsification swaps the row's destination id and the same assertion
  prints `5 4 3 2 1 0` and fails.
- **PASS — there is no rank field.** `SCREEN RANK FIELD: none` is the row's seven fields. The check's
  `rank_free` guard reads the `Destination` and `EndingScreenRow` struct bodies and fails on any
  `rank`, `score`, `quality` or `grade` field; the fifth falsification plants `int rank = 0;` in the
  row and the guard fails.
- **PASS — the screen's words are neutral.** `SCREEN LABELS: 11, 0 tone findings` is
  `EndingToneFindings(EndingScreenLabels())`; the labels are the same strings `Publish` sends, so the
  assertion is over what the screen draws, not a hand-copied echo. `TestEndingScreen` also asserts the
  title does not contain the tone word "home".
- **PASS — a place not earned says what a play would spend, and an earned run opens them all.**
  `SCREEN OPEN: 1 of 6; a play is offered for 3` and `SCREEN EARNED: 6 of 6 open` are read from the
  same rows the screen draws.
- **PASS — the screen can compile and link against the engine's UI module.** The symbol
  `LWH_UI_EndingScreens` is in `libefui.so`, and the game module publishes the cvars it reads.

## What could not be run here, and is not claimed

- **The rendered screen was not watched in a graphical session.** This static host compiled and linked
  the module but did not run the engine and photograph the screen. The rows, the order, the absent rank
  field and the neutrality of the words are proven; the *rendered* layout, contrast and legibility are
  not. That is the same boundary the earlier ending evidence recorded for the arrival.
- **No measurement of the screen's legibility was taken.** The wrap is the shared measured wrap
  (`LWH_WrapText`), and every line is measured against its column when the screen draws; the colour
  contrast and the font sizes at 640×480 have not been measured against the accessibility bar, because
  no frame was captured.
- **The screen was not driven by a hand.** `lwh_ending_key` drives it by name for a harness, but no
  engine run was made here.

## Decisions taken where the document leaves a choice

- **The screen lists all six destinations, not only the accrued set.** An unearned place is part of
  the world the run did not reach, and marking it "NOT OPEN" or "A PLAY" is information, not a
  ranking; hiding it would make the accrued set look like the whole set.
- **The approach and the filing question are rows on the same screen.** The approach is the design's
  route, and filing is Earth's last question; putting them beside the destinations keeps the choice in
  one place.
- **The screen closes before the arrival.** A menu pauses single-player simulation, and more
  importantly the design's last frame is the record: leaving the screen up would make the UI the last
  frame instead.
