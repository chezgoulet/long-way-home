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

## What is left for this gap

- **A browsable screen.** The log is a console query; the contract's "browsable artifact" wants a
  scrollable full-screen log, a ready-room terminal, and the captain's log. The console has the data;
  the screen needs drawing.
- **Every author.** Some events are still written by the subsystem ("damage control") rather than a
  named crew member; the crew records now carry names, so a person can sign them.
- **The captain's log** as an authored, summarised artifact, distinct from the raw feed.
