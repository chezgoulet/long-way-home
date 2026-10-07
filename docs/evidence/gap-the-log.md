# Evidence: the log-as-a-browsable-artifact gap, first slice

Date: 2026-10-07. The fourth of the five owner-approved gaps (`docs/gates.md`): *state* log entries
with time, author, subject and fact; *control* each post reads its own scope, command sees all, the
player can search; *visible* the log itself — retiring the standing assumption in nearly every other
document that something "was recorded"; *location* any console, the ready room, the captain's log.
*Acceptance:* after a run, a player can reconstruct what happened and when from the log alone.

## What is built

**State.** `Ship` gains a bounded `log` of `LogEntry { time, who, scope, what }` — ship time since
midnight of day 0, the author, the subject (a scope for filtering: `bridge`, `engineering`, `hull`,
`command`, `sickbay`, `outside`), and the fact. `LogEvent` writes one and drops the oldest past 128.
Save format is now **version 11**.

**The events that write.** Alert changes; system damage; hull breaches and seals; force fields;
standing orders (priority repair, guards, evacuation, triage); casualties (injured by air, wounded in
a fight, dead, assimilated, and a death for want of a bed); boarding; jumps and torpedo fire. This is
where the documents that said something "was recorded" now actually write it.

**Read and search.** `ship log [count] [scope]` prints the newest first, optionally filtered to one
scope. In the engine, after a short scenario, it reads back exactly:

```
SHIP: --- the log ---
SHIP: day 0 08:02 [bridge] the bridge: condition red
SHIP: day 0 08:02 [hull] damage control: hull breached on deck 9
SHIP: day 0 08:02 [engineering] damage control: sensors damaged
SHIP: day 0 08:02 [sickbay] command: triage: rank first
```

`g_shipTest 16` reproduces it (`scripts/run-engine.sh +set g_ship 1 +set g_shipTest 16 +map
tour/deck04`).

## What was proven

```
PASS  the log says what happened, when, and by whom
```

`tests/ship/test_ship_core.cpp` (`TestLog`): a scenario (red alert, a breach, damage, a triage
order, boarders) leaves entries in time order, each with a non-empty author, scope and fact, and all
five facts are present; the scope filters (at least two `hull` entries); and the log survives a save
and a load in order. The whole suite passes, as do `test.sh`, `scripts/s2-check.sh`,
`scripts/s4-check.sh` and `scripts/s10-check.sh`.

## The screen, the names and the captain's log (2026-10-07)

The three items the first slice left are built and tested (`TestLog`, `TestGapCompletions`,
`scripts/gaps-check.sh`):

- **A browsable screen.** `ui_lwh_log` draws the ship's record newest-first — when, who, scope, what —
  scrollable, reading `lwh_ship_log`. It is the ready-room terminal the contract names.
- **Every author.** The log no longer signs events with a subsystem. `AuthorFor` picks the senior fit
  crew member of the department on duty (damage control is an engineer, an order is the commanding
  officer, a death is the medical officer); a station's name remains only as a fallback when nobody
  of that department is on duty. The crew records carry the names.
- **The captain's log.** `ship captain` prints a summarised artifact in the captain's voice — the day
  and condition, the crew's state, the systems needing attention, the intruders and the contact —
  generated from the ship's state, and `ship captain <text>` writes an authored entry under
  `CommandingOfficer`'s name. It is distinct from the raw feed.

## What is left for this gap

- Nothing in the contract. A physical ready-room terminal (a panel in the world that opens the
  screen) is the ready-room deck-build item, not a gap item.
