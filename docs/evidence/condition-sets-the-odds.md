# Evidence: condition sets the odds, and stress sets the severity

Date: 2026-10-06. The owner's ruling of 2026-10-06, written in
`docs/failure-is-content.md` section *"Condition sets the odds, and stress sets the severity"*:
**a system's chance of failing is a function of its condition, and this is general**; inside the top
tenth of capability it is nominal; as capability degrades the odds rise; and at severe degradation,
or under severe stress, it lets go visibly at the console the operator holds. Degradation sets the
odds, stress sets the severity, and the severity ladder is the document's own: **degraded, acute,
catastrophic**. The fairness rules are the acceptance test: the risk is visible before the act, the
consequence traces back to what the operator did, and a competent path avoids it.

Game logic lives in `module/ship/` only. No engine change; nothing cvar-gated was needed. Save format
is now **version 42** (the anomaly draw counter, and transporter copies beyond the complement).

## What is built

**The general mechanism** (`module/ship/ship_core.h`, `ship_core.cpp`), a property of every system:

- `SystemCondition(System)` — the capability a system still has, 0..1: its **health** (maintenance
  and parts) capped by the **output** (the power actually reaching it). This is read from fields the
  ship already carries; no new per-system state was needed.
- `AnomalyOdds(condition)` — **0** at `NOMINAL_CONDITION` (0.90, the top tenth) and above; below it
  the chance rises as the square of how far the condition has fallen, up to `ANOMALY_ODDS_MAX`
  (0.85, invented). A well-kept system does what it says, with no consequence.
- `AnomalySeverityFor(condition, stress)` — the ladder: **degraded** (< 0.33), **acute** (< 0.60),
  **catastrophic** at or above. It is driven by the condition first and the load second: a 40% system
  run light is degraded, the same system at battle stations is catastrophic — the document's own
  "misaligned beam ... versus a merge".
- `RollAnomaly(condition, stress, roll)` — the odds first, then the severity, exactly as the ruling
  says. `roll` is the deterministic draw, so a test and the game see the same result.
- `StressNow(Ship)` — the load now: 1.0 at red alert, 0.5 at yellow, 0.15 on a quiet watch.
- `UseSystem(Ship, id, stress, who)` — the general use. It advances the saved draw counter, rolls the
  odds, and on a bad draw **writes the chain to the log** (system, severity, condition, load, and the
  author, who is the operator at the console) and applies the consequence: a scar on the system at
  the least, and at acute or worse **the console lets go at whoever mans it** — canon's exploding
  console, made the visible face of a degraded system at the one place the operator is standing.

**The transporter, worked end to end** (`TransporterConditionLine`, `TransporterRisk`,
`TransporterOutcome`; `TransportAway`/`TransportBack`). The pattern buffer is the condition. The
outcome family of `docs/failure-is-content.md` is mechanised, and each outcome writes to the records:

- **degraded** — a misaligned beam: the traveller arrives hurt;
- **acute** — a mangled arrival: a serious injury, a sickbay problem;
- **catastrophic** — a **copy** (a new record with the same face, an ambiguous legal status, and the
  log says command must decide) or a **merge** (two become one; one record closes, the survivor is
  not either of them). A copy is appended beyond the 141-crew complement and is stored in the save;
  it is bounded at `MAX_DUPLICATES` (16). Neither is a reset button.

**The instrument** (the brief's item 3). The Operations console now carries
`lwh_ship_transporter`, published from `module/ship/g_ship.cpp` like every other reading, and drawn
on the existing Ops screen (`module/ui/ui_lwh_engineering.cpp`) — no new renderer. The `ship
transport` and `ship recall` commands state it **before the act**:

```
SHIP: transmitter: the pattern buffer is at 100%, nominal: I would send anyone
SHIP: transmitter: the pattern buffer is at 40%, and below 40 I would not send anyone
```

That is the document's shape — *"the pattern buffer is at sixty-two percent, and below forty I would
not send anyone."* It is a read of state and cannot lie.

## What was proven

The measured odds, from twenty thousand draws at each condition (`test_ship_core --risk`):

```
condition  AnomalyOdds   measured/20000   severity light   severity battle
   1.00        0.0000             0          degraded        degraded
   0.90        0.0000             0          degraded        degraded
   0.80        0.0105           211          degraded        degraded
   0.70        0.0420           841          degraded        degraded
   0.60        0.0944          1891          degraded        acute
   0.50        0.1679          3358          degraded        acute
   0.40        0.2623          5248          degraded        catastrophic
   0.30        0.3778          7557          degraded        catastrophic
   0.20        0.5142         10286          acute           catastrophic
   0.10        0.6716         13434          acute           catastrophic
  -0.00        0.8500         17000          acute           catastrophic
```

The top tenth is exactly zero over the run; the odds rise monotonically; and the same degraded
condition takes a worse outcome under load than it does light.

```
PASS  a nominal system is beamed fifty times with no anomaly; the console states the condition before the act;
      the same system degraded and at battle stations lets go, writes the chain to the log, and changes or hurts a record
```

`scripts/condition-check.sh` (one headless run on `tour/deck04`, `g_shipTest 50`, writing under
`build/g3-home` only) produced, in full:

```
SHIP: RISK instrument nominal: the pattern buffer is at 100%, nominal: I would send anyone
SHIP: RISK nominal: 50 beams, 0 anomalies (top tenth)
SHIP: transmitter: the pattern buffer is at 100%, nominal: I would send anyone
SHIP: away team of 3 beamed down
SHIP: transmitter: the pattern buffer is at 100%, nominal: I would send anyone
SHIP: the away team is back aboard
SHIP: RISK instrument degraded: the pattern buffer is at 40%, and below 40 I would not send anyone
SHIP: RISK degraded under red alert: 2 anomaly, last: a copy of Chakotay materialised

SHIP: day 0 08:17  [crew] the transporter room: a second Chakotay materialised: a copy, and not the person
SHIP: day 0 08:17  [sickbay] The Doctor: Crewman 063 was hurt when the transporters console let go
SHIP: day 0 08:17  [engineering] B'Elanna Torres: transporters: catastrophic anomaly at 10% condition under 100% load
SHIP: day 0 08:17  [crew] the transporter room: a second Chakotay materialised: a copy, and not the person
SHIP: day 0 08:17  [sickbay] The Doctor: Crewman 053 was hurt when the transporters console let go
SHIP: day 0 08:17  [engineering] B'Elanna Torres: transporters: catastrophic anomaly at 40% condition under 100% load
```

Read bottom-up: the console stated `40%, and below 40 I would not send anyone`; the beam then produced
a **catastrophic** anomaly at 40% condition under 100% load; the console let go at the operator
(Crewman 053 was hurt); the beam after that was made on the buffer the first had scarred (10%
condition) and produced the copy. The chain — condition, load, severity, who was at the console, what
happened in the records — is all in the record, which is the brief's item 4.

The unit tests, without the game (`tests/ship/test_ship_core.cpp`):

- `TestConditionOdds` — the odds are zero in the top tenth on any draw and rise monotonically; over
  twenty thousand draws the measured frequency tracks `AnomalyOdds`; the ladder is monotonic in both
  condition and load; `RollAnomaly(0.95, 1.0, any)` is always clean; `UseSystem` advances the draw
  counter and writes the chain.
- `TestTransporterAnomaly` — the instrument names the pattern buffer and the 40% threshold; a hundred
  beams out and back at nominal produce no anomaly of any kind and leave the crew whole; the same
  system at 40% under red alert reaches both ends of the ladder, records the chain, and round-trips
  the changed records (a copy beyond the complement) through the save.

`scripts/test.sh` passes (exit status 0):

```
==> ship simulation: unit tests
ship_core: all checks passed
...
all checks passed
```

## Judgement calls, named

- **The merge.** The document says "one record replaces two". The roster's size is invariant (a death
  keeps its record with status `CREW_DEAD`; the post empties and the roster promotes), so the merge is
  represented the model's way: one of the two records closes as dead, the survivor is changed, and the
  log says who is gone and that the one who came back is not either of them. Removal would break every
  index into the roster; this keeps the invariant and is recorded as **our call**.
- **The copy.** A copy is a genuine new record beyond the complement, stored in the save (name and
  type have no seed to come back from), bounded at 16. This is the one place the save format grew a
  variable-length roster; the complement check now accepts `[141, 157]`.
- **Condition = min(health, output).** "Power, maintenance, parts" is read as the worse of the system's
  health and the power actually reaching it, so a starved system and a broken one are both less
  capable. No new per-system field was needed for this.
- **Which system.** The transporter, because the ruling was written about it and its outcome family was
  already specified. It was modelled only as an away-team beam before; it is now worked end to end.
- **The severity thresholds** (0.33, 0.60) and `ANOMALY_ODDS_MAX` are invented, marked `[inv]`.

## What could not be verified

- **The drawn in-world panel and the Ops screen**, visually: `lwh_panel` and `ui_lwh_engineering` are
  compiled (the module builds) and the Ops line is driven by a published cvar, but a person at the
  console has not read it in a logged-in session. The console command output above is the machine
  check.
- **A copy or a merge played by hand**, and the crew's reaction to it (the records change and the log
  says so; the *scene* the document asks for is content).
- **`scripts/check.sh`** is not part of this work. Run against the restored GDK with a campaign
  source map it passes the validator's negative tests and then exits **1** on the compiler corpus:
  `2024 files: 2016 compiled and read back, 8 rejected by the compiler`. That is pre-existing and
  recorded in `docs/gates.md` (the eight rejections are three named syntax errors plus five files
  that are not scripts); it is not touched by these changes. (`hm_temple.map` is not a usable clean
  fixture — a holomatch map has no navigation entities — so `borg1.map` was the source map.)
