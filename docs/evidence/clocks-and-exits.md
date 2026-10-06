# Evidence — three loose ends, and the three clocks and the two exits

Date: 2026-10-06. Branch: `feat/clocks-and-exits` (cut from `feature/g3-reactive-crew`).
Owner's ruling of 2026-10-06, written into `docs/ship-model.md` under *"The three clocks, and the two
exits"*; this is the implementation, not a redesign. Save format **version 43** (the left-standing mark).

Nothing here is asserted without the command that produced it.

---

## Task A — three loose ends, each verified before it was touched

### A1 — `scripts/check.sh` could never exit 0

**Verified.** On the pinned GDK corpus the eight rejections are exactly the documented set, and the
script exited non-zero for them with no new fault. The eight are the three scripts named in
`docs/evidence/g0-script-compiler.md` (`voy1/scene7.TXT`, `voy1/scene10.TXT`, `voy5/beamstart.TXT`)
and five files that are not scripts (`borg6/setup.txt`, `dn1/start.txt`, `dn1/startbakup.txt`,
`validdirs.txt`, `voy4/ordermunro.bak.txt`).

**Fix.** The corpus pass now carries an explicit `EXPECTED_REJECTIONS` set. It exits **0 when the
rejections are exactly that set**, and non-zero when a file outside the set is rejected, a listed file
now compiles, or there is any read-back failure. Each case is named in the output.

**Observed** (the last line of the run, and the exit status):

```
$ scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts
...
==> compiler corpus pass (every shipped script: compile, then read back)
    2024 files: 2016 compiled and read back, 8 rejected by the compiler, 0 read-back failure(s)
      compile: borg6/setup.txt  |  ...
      ... (all eight named) ...
    the 8 rejections are exactly the known, documented set
$ echo $?
0
```

The check guarding the script pipeline now says something when it fails, because it can say yes when
nothing is wrong.

### A2 — the validator's clean fixture could fail its own negative test

**Verified.** With `hm_temple.map` (a holomatch space, no navigation entities) the clean fixture failed
`E003`, so the baseline was invalid for a reason unrelated to what was under test:

```
$ python3 tools/validator/tests/negative_tests.py \
    --source-map build/gdk/maps/eliteforce_holomatch_maps/hm_temple.map \
    --script-corpus build/gdk/scripts ...
FAIL  clean fixture should pass (rc=1)
E003  ERROR  maps/deck.map: declared inhabited but has no navigation entities
```

**Fix.** `tools/validator/tests/negative_tests.py` now asserts the fixture is clean *before* it is used.
If it is not, it names the source map as an unusable clean fixture, prints the reason, and returns **2**
without running a single seeded case — so a broken baseline can no longer be mistaken for a real
regression, or the reverse.

**Observed:**

```
$ python3 tools/validator/tests/negative_tests.py \
    --source-map build/gdk/maps/eliteforce_holomatch_maps/hm_temple.map \
    --script-corpus build/gdk/scripts ...
FAIL  the clean fixture is not clean: the supplied source map is not a usable clean fixture (hm_temple.map)
      A clean fixture must be built around a space whose declared-inhabited map has navigation entities (a holomatch map has none).
      E003  ERROR  maps/deck.map: declared inhabited but has no navigation entities
$ echo $?
2
```

With a campaign source map the negative tests are unchanged:

```
$ python3 tools/validator/tests/negative_tests.py --source-map build/gdk/maps/brig-map/_brig.map ...
PASS  clean fixture validates with no errors
...
ALL PASS
```

**Regression test:** `tests/tools/test_validator.py` feeds a nav-less map to the harness and asserts it
returns 2 with the fixture message and does not run the seeded cases. It needs no game data or compiler.

### A3 — a save round-trip discrepancy between crew `wounds` and `severity`

**Verified, and it was real.** The observed symptom reproduces, but it is not a field swap in the
serialiser. `Unpack` ends with a derive-on-load `Tick(s, 0.0f)`; `TreatCasualties` ran the
untreated-casualty rule even with zero time elapsed and did `c.wounds = max(c.wounds, c.severity)`. So a
record saved as `wounds=0.15 severity=0.80` came back as `wounds=0.80 severity=0.80` — the throwaway
harness was reading the truth, and a saved run did not replay identically.

**Fix (small, not a redesign).** The untreated branch now runs only when time actually passes
(`hours > 0.0f`). A zero-length tick — the derive-on-load — no longer moves a saved field; every
non-zero tick is unchanged.

**Regression test:** `TestCrewWoundsRoundTrip` in `tests/ship/test_ship_core.cpp` round-trips a
casualty with distinguishable `wounds`, `severity` and `recovery` (no free bed, so the untreated rule
would fire on any tick), asserts each field is bit-identical, and asserts `Pack(back) == blob`. It is
kept, so the question cannot come back silently. Observed:

```
  wounds 0.15 -> 0.15, severity 0.80 -> 0.80, recovery 0.25 -> 0.25
```

---

## Task B — the three clocks and the two exits

`docs/ship-model.md`: accelerated (the player sleeps — incrementally, or all at once), real time (the
normal state), wall clock (a background process keeps the world's time while the player is off duty).
The fourth case is not a clock: exit without a background process and the simulation stops. The line
held is that the world clock cannot be stopped and the simulation can be shut down.

**What is in the model** (`module/ship/ship_core.{h,cpp}`):

- **The sleep state** — `Sleep(Ship &, double shipSeconds)` skips time at the accelerated rate. It is
  an act, not a rate, so it works whatever the clock is configured to, and it advances ship time by
  exactly the interval asked for. `Advance` cuts long steps up, so a sleep converges with a played run.
- **Standing orders across a sleep** — `Sleep` runs the same tick a played interval does, so the
  standing orders (save version 7) are what the ship does while the player sleeps. Nothing new was
  needed here but the proof.
- **The record's mark** — `bool Ship::leftStanding`, saved (version 43) and printed by `Describe`.
  `Suspend` sets it and writes the entry; `CatchUp` writes the continuous return entry. A run shut
  down carries neither, and no time existed.
- **The guard** — `MaySuspend(cfg)` is holodeck-only, alongside `SavesAllowed`; `Suspend` returns false
  and changes nothing under ironman.

**Console and game wiring** (`module/ship/g_ship.cpp`): `ship sleep <hours>` (the sleep state),
`ship suspend` (the exit that leaves the ship standing; ironman refuses), `ship leftstanding` (the
mark); harness `g_shipTest 51`.

### The unit test — convergence, orders, the record, the guard

`tests/ship/test_ship_core.cpp`: `TestSleepAndExits`. Observed output:

```
  slept 6 h to day 0, deuterium 0.998; played the same interval: day 0, deuterium 0.998
  left standing: mark 1, day 0, 4 log entries; shut down: mark 0, day 0
ship_core: all checks passed
```

The same test asserts: a one-jump sleep, twelve incremental sleeps and a played interval converge
(`Describe` and `Pack` are byte-identical for the first two); a deck ordered evacuated stays empty
across a six-hour sleep, a damaged system is seen to, and the night is in the log; the mark is in the
save and survives a round trip; and ironman refuses suspension.

### The game harness

```
$ scripts/clock-check.sh
==> clocks and exits
    clock test: ironman may suspend 0 (must be 0)
    clock test: sleep 6h -> day 0; twelve steps -> day 0; played -> day 0; identical 1
    clock test: after a 6h sleep, deck 11 crew 0 (must be 0), sensors 1.00, log entries 7
    clock test: holodeck suspend -> left standing 1 (must be 1)
PASS  ironman refuses suspension; a sleep converges with a played interval and carries a standing order; holodeck leaves the run marked
```

### The suites

```
$ scripts/test.sh ; echo $?
...
==> tools: unit tests
Ran 45 tests in 0.627s
OK
...
all checks passed
0
```

```
$ scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts ; echo $?
...
ALL PASS                       (validator negative tests)
    the 8 rejections are exactly the known, documented set
0
```

---

## What could not be verified, and why

- **Exit with a real background process.** The distinction between the two exits — the process that
  keeps the world's time and the one that stops the simulation — is the engine's behaviour on quit,
  and it needs a real client, not a headless run. What is proven is the model-level equivalent: the
  mark, the continuous catch-up, the guard, and the convergence. The engine's quit path calling
  `Suspend` is not wired here and is named as the remaining work, not implied to work.
- **The rendered console.** The commands are driven headless; a person at the console reading the
  output is the standing session caveat.

## Judgement calls, named

1. **A3's fix is in the tick, not the serialiser.** The round trip moved `wounds` because the
   derive-on-load zero tick applied a rule that advances a casualty. Guarding that branch with
   `hours > 0.0f` makes a zero tick derive without advancing, which is what "derive-on-load" should
   mean; the serialiser was symmetric. I did not change the (deliberate) coupling of `wounds` to
   `severity` beyond stopping it firing at zero elapsed time.
2. **`CatchUp` logs only when the run was left standing.** The mark is set by `Suspend`, which is the
   act of leaving the ship standing; catch-up on return is its consequence. An ironman run never marks,
   so it never gets the continuous-log entry.
3. **The suspension guard is the `SavesAllowed` guard, in the same place.** Holodeck may suspend;
   ironman may not, because ironman means the ship keeps her own time.
4. **`ship sleep` advances ship time directly**, regardless of the configured clock rate, because
   sleeping is an act rather than a rate; that is what makes an incremental sleep and a one-jump sleep
   provably identical.
