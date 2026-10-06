# Evidence: the morale-and-fatigue gap, first slice

Date: 2026-10-07. The first of the five owner-approved gaps (`docs/gates.md`): *state* per crew
record, morale and fatigue with their drivers; *control* the watch roster, the galley, the holodeck
and time off; *visible* performance at post; *failure* a crew member who stops caring; *location*
everywhere. *Acceptance:* a bad stretch measurably degrades posts, a good one measurably restores
them, and the log can say why. (The log itself is the fourth gap.)

## What is built

**State.** `CrewMember` gains `morale` (0 broken .. 1 heart in it) beside the fatigue S6 already
kept. Save format is now **version 8**; morale round-trips.

**Drivers** (`UpdateCrew`). Morale eases toward a target over hours, so a bad afternoon is not a bad
week; fatigue accrues on duty and clears with sleep, as before. The target is raised by a meal (if
the replicators run), recreation (if the holodeck does) and sleep, and lowered by a watch at anything
but green, red alert, standing on a deck without air, and the fraction of the crew lost and still
missed. So the roster, the galley, the holodeck and time off are the controls the contract names.

**Effect.** A system's output now uses `staffing` — the post-holders weighted by `Effectiveness(c)` —
rather than a bare head count. A rested, willing person is exactly one hand, so an ordinary watch is
unchanged and the other systems' accounting still holds; only a spent crew member (past half-tired)
or a flagging one (morale below the halfway mark) costs anything, down to a quarter. Retaking a
hijacked system uses the same weighted staffing.

**Visible.** The Sickbay console's readout now ends `MORALE nn%  FATIGUE nn%`, the crew's average
condition (`lwh_ship_medical`).

## What was proven

```
PASS  a spent or disillusioned post is worse; rest restores it, and morale saves
```

`tests/ship/test_ship_core.cpp` (`TestMoraleAndFatigue`): a fresh, rested watch delivers exactly 1.0;
the same hands spent (fatigue 1.0) and disillusioned (morale 0.2) deliver less but never less than a
quarter (tired is not absent), while `manned` still shows the hands are there; rested and willing
again, the post returns to exactly 1.0; an eight-hour watch raises fatigue and green lifts morale;
morale survives Pack/Unpack. The whole suite (every earlier property included) passes, as do
`test.sh`, `scripts/s2-check.sh`, `scripts/s4-check.sh`, `scripts/s4-glance-check.sh`,
`scripts/s4-panel-check.sh` and `scripts/g3-measure.sh`.

## What is left for this gap

- **Manner.** The contract asks for the crew's manner as well as their numbers; the crew layer's
  barks and idles are the place, and are not wired to morale yet.
- **"The log can say why"** waits on the fourth gap (the log as a browsable artifact).
- More drivers the owner may want: warmth, rations, relationships, grief; each is one target term.
