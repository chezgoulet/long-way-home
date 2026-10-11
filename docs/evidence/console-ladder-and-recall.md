# The console ladder — a walk that catches a shadowed verb, and the `recall` fix (audit §2.1)

`docs/audit-whole-project.md`, section 2.1 (the finding this pass closes) and section 3 (why the ladder
is long enough to need a guard), the register pattern of `docs/evidence/the-computer-api.md` and
`tools/hooks/check_register.py`, applied on `feat/the-console-ladder` (cut from `testing`), 2026-10-11.

**Observed, and the command that produced it.** Every output below is a command named beside it, on this
tree (`sasquatch`), after the module was rebuilt — `cmake -S ../upstream/efgame -B
../upstream/efgame/build-linux -DLWH_MODULE_DIR=$PWD/module && cmake --build
../upstream/efgame/build-linux --target efgame --target efui` (exit 0; only the pre-existing
`-Wwrite-strings` warnings). The engine run is headless (`xvfb-run`, `SDL_AUDIODRIVER=dummy`). Nothing
was committed to `main`; no game data, asset or generated file is in the repository.

## The finding

`Svcmd_Ship_f` (`module/ship/g_ship.cpp`, 1,614 lines) claimed `recall` twice in one `if`/`else if`
ladder:

- line 5646 — the **transporter** branch (`ship::TransportBack`), with **no argument guard**;
- line 6021 — the **crew-memory** read (`ship::Recall` / `ship::RecallSource`), guarded `a[0] && b[0]`.

The second could never run. `ship recall <who> <event>` — a request to read what a person remembers —
was silently served by the transporter, which reported the away team home. **And no guard could see
it:** `tools/console/verbs.py` derived the verb set with `sorted(set(...))`, which collapses the
duplicate before the comparison, and `docs/hook-register.md` listed `Recall` and `RecallSource` as
reached from the console. They were not.

## Task A — which branch owns `recall`, and why

**The call: bare `recall` belongs to the transporter; the crew-memory read needs its two arguments.**

The reasons, in the order that decides them:

1. **The Operations console already means the transporter by the bare verb.** The station screen's `R`
   key sends exactly `ship recall` (`module/ui/ui_lwh_engineering.cpp:517`, key labelled `R recall`
   at `:467`). That is the load-bearing caller; the fix must not take the away team's return away from
   it.
2. **The code already says so.** `ship::RecallShuttle`'s branch is annotated *"'recall' is the
   transporter's; a shuttle comes home by docking"* (`g_ship.cpp:6373`).
3. **The API row agrees.** `console_api.def` carried `recall` with station `ops` — an away team's
   return is an Operations act. Reading a crew member's memory is not.
4. **The two claims are different acts**, and a verb that names an act the console performs should mean
   that act bare. A memory read is a query that names its subject, so it is the form that carries
   arguments.

**The fix, and it is the shape `breach` already uses** (two claims distinguished by argument shape, both
reachable): add `&& !a[0]` to the transporter branch, so an argumented `recall` falls past it.

```
-	else if ( !Q_stricmp( cmd, "recall" ) )
+	else if ( !Q_stricmp( cmd, "recall" ) && !a[0] )
```

The memory branch keeps its `a[0] && b[0]` guard, and now says what it did when the crew index is bad
rather than returning silently. `recall <who> <event>` therefore reaches **two different branches**, each
with its own output.

**The console output for both**, from one headless run of a new harness case (`g_shipTest 99`), which
beams a party down, gives crew 3 a memory, then issues the two forms:

```
$ SDL_AUDIODRIVER=dummy timeout 200 xvfb-run -a scripts/run-engine.sh --home-dir build/g3-home \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipTest 99 +map tour/deck04
$ grep '^SHIP:' build/g3-home/console-ladder.out | tail -9
SHIP: transmitter: the pattern buffer is at 100%, nominal: I would send anyone
SHIP: away team of 3 beamed down
SHIP: crew 3 now remembers event 7
SHIP: transmitter: the pattern buffer is at 100%, nominal: I would send anyone
SHIP: the away team is back aboard                        <- `ship recall`      (transporter: line 5679)
SHIP: console ladder: away now 0
SHIP: crew 3 knows event 7 (saw it)                       <- `ship recall 3 7`  (crew memory: line 6061)
SHIP: console ladder test done
```

**What the old behaviour was.** Pre-fix, line 5646 matched every `ship recall …`, so `ship recall 3 7`
called `ship::TransportBack` and printed the away-team line — the wrong answer, confidently. The fix is
the one shape the brief licensed: a differently-shaped collision would be a finding for the owner, not a
licence to re-plumb the ladder. No other verb needed that.

## Task B — the guard walks the ladder; it does not compare sets

`tools/console/verbs.py` now walks `Svcmd_Ship_f`'s top-level `if`/`else if` chain in source order
(`claims()`), rather than deriving a set. For each `Q_stricmp( cmd, "V" )` claim it reads the **argument
shape** the guard imposes: a lower bound on arguments (`a[0]`, `b[0]`, `gi.argc() > first + N`), an upper
bound where the guard forbids an argument (`!a[0]`), and the content tokens a guard adds (`sys >= 0`).
When a verb is claimed more than once it reports, in ladder order, one of two cases:

- **reachable** — the claims differ by argument shape, so each answers an input the other does not. A
  **warning**; the check still passes.
- **shadowed** — an earlier claim matches every argument shape a later claim needs, and would be reached
  first (same chain, or the earlier branch returns). The later claim can never run. A **failure**, naming
  the verb and the shadowed line.

### Demonstration 1 — fails on the pre-fix tree, naming `recall` and the shadowed branch

The pre-fix console is the committed `module/ship/g_ship.cpp` (`git show HEAD:…`), fed to the same tool
without touching the tree:

```
$ git show HEAD:module/ship/g_ship.cpp > /tmp/g_ship_prefix.cpp
$ python3 tools/console/verbs.py check --console /tmp/g_ship_prefix.cpp --register module/ship/console_api.def
verbs.py: /tmp/g_ship_prefix.cpp answers 130 verb(s); module/ship/console_api.def enumerates 130
WARN  the verb 'breach' is claimed twice and both can be reached (they differ by argument shape)
        line 5573: !Q_stricmp( cmd, "breach" ) && a[0] && b[0]  (needs at least 2 argument(s))
        line 5848: !Q_stricmp( cmd, "breach" ) && sys >= 0  (takes any arguments and 0 >= sys)
FAIL  the verb 'recall' is claimed twice, and the claim at line 6021 can never run: line 5646 matches every argument shape it needs
        line 5646: !Q_stricmp( cmd, "recall" )  (takes any arguments)
        line 6021: !Q_stricmp( cmd, "recall" ) && a[0] && b[0]  (needs at least 2 argument(s)) -- shadowed
exit=1
```

### Demonstration 2 — passes after the fix, and reports the two remaining collisions as reachable

```
$ scripts/api-check.sh
verbs.py: module/ship/g_ship.cpp answers 130 verb(s); module/ship/console_api.def enumerates 130
WARN  the verb 'breach' is claimed twice and both can be reached (they differ by argument shape)
        line 5606: !Q_stricmp( cmd, "breach" ) && a[0] && b[0]  (needs at least 2 argument(s))
        line 5888: !Q_stricmp( cmd, "breach" ) && sys >= 0  (takes any arguments and 0 >= sys)
WARN  the verb 'recall' is claimed twice and both can be reached (they differ by argument shape)
        line 5679: !Q_stricmp( cmd, "recall" ) && !a[0]  (takes no arguments)
        line 6061: !Q_stricmp( cmd, "recall" ) && a[0] && b[0]  (needs at least 2 argument(s))
PASS  every console verb is in the ship's API, and every API verb is answered by the console
exit=0
```

### Demonstration 3 — catches a planted shadowing

A second claim of `survey` (already claimed, with no argument distinction) is inserted into a scratch
copy of the fixed console, and the walk names it; the copy is then discarded:

```
$ python3 tools/console/verbs.py check --console /tmp/g_ship_planted.cpp --register module/ship/console_api.def
...
FAIL  the verb 'survey' is claimed twice, and the claim at line 5702 can never run: line 5697 matches every argument shape it needs
        line 5697: !Q_stricmp( cmd, "survey" )  (takes any arguments)
        line 5702: !Q_stricmp( cmd, "survey" )  (takes any arguments) -- shadowed
exit=1
```

### What the walk is, and is not

The walk sees **argument shape and dispatch order**. It does **not** see dead logic *inside* a branch: a
guard satisfied only by a value no input carries, or a branch that returns for every argument it accepts,
is invisible to it. The brief's own limit stands and is restated here: **whether the ladder's other 1,600
lines contain a second unreachable verb of a different shape is not verified** — the walk finds
duplicate claims by shape, not dead branches. Membership both ways is unchanged: the def's duplicate-verb
protection still raises (`tests/tools/test_console_verbs.py::test_a_bad_station_is_reported`), and
`enumerate`/`derive` return the same sets as before (130 verbs).

## Task C — the register and the def

**`docs/hook-register.md`** listed both hooks as reached from the console, and that was false. Task A made
it true, so the rows are left as they read — now accurately — and quoted here after the change
(`docs/hook-register.md:497-498`):

```
| `Recall` | `const CrewMember &who, uint16_t event` | recall | console | — | — |
| `RecallSource` | `const CrewMember &who, uint16_t event` | recall source | console | — | — |
```

`console` is the register's own definition: *"it works, and only the developer console (`ship …`,
`Svcmd_Ship_f`) or the `g_shipTest` harness reaches it"*. `ship recall <who> <event>` now reaches
`ship::Recall` and `ship::RecallSource`, so the claim holds; no row was weakened.

**`module/ship/console_api.def`** — `recall`'s argument hint changed with what it takes (the bare form is
the transporter's and takes none; the argumented form is the memory read):

```
- CONSOLE_VERB("recall", "[party]", "ops")
+ CONSOLE_VERB("recall", "-|<who> <event>", "ops")
```

The station stays `ops`: the row is one per verb, and the Operations console's `R` key is the play
surface for the bare form. **This is a call, recorded:** a single row cannot say "Ops for the bare form,
developer console for the argumented one", and splitting the verb was out of scope for this pass.

**Every other verb the walk reported:** `breach`, claimed twice and **reachable** (a warning, not a
failure) — `breach <deck> <0..1>` is `BreachDeck` and `breach <system>` is the counter-hack
(`g_ship.cpp:5606` and `:5888`), distinguished by argument shape. No other verb is claimed more than
once, and none is shadowed. Nothing else was fixed.

## The checks, run

```
$ scripts/test.sh                                   # exit 0
$ scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts   # exit 0
$ scripts/api-check.sh                              # exit 0 (output above)
$ scripts/hooks-check.sh                            # exit 0
hooks-check: module/ship/ship_core.h declares 441 public functions; docs/hook-register.md registers 441 hooks
PASS  every public function is registered, and every registered hook exists
```

`scripts/test.sh` covers the guard's own tests (`tests/tools/test_console_verbs.py`: the shadow is a
failure, the argument-shape collision is a warning, the planted shadow names the verb, and the real tree
has no shadowed claim and claims `recall` on two reachable branches).

## What I could not verify

- **No second unreachable verb of a different shape.** The walk finds duplicate claims and their
  argument shapes; it cannot see a branch that is unreachable for a reason other than a duplicate claim.
  The other ~1,600 lines of `Svcmd_Ship_f` were not read for that.
- **Runtime reachability of every branch.** The engine demonstration shows the two `recall` branches
  reached through the console; the walk's other findings (membership, the `breach` warning) are static.
- **The pre-fix engine answer itself.** The pre-fix failure is shown by the walk (Demonstration 1), not
  by running a pre-fix binary; the pre-fix branch's behaviour is read from the committed source, which
  the walk names. Rebuilding the pre-fix module to photograph the wrong answer was not done.

## Judgement calls, named

1. **Bare `recall` is the transporter's; the memory read takes its two arguments** (Task A). Reasons: the
   Ops screen's `R` key, the in-code comment, the API row's `ops` station, and the plain reading that a
   bare verb names the act the console performs.
2. **The fix is one guard, `&& !a[0]`,** the same shape as `breach`; no ladder re-plumbing.
3. **The register rows were left, not edited**, because Task A made their `console` claim true — the
   brief's "make it true by Task A if they now are".
4. **The def's hint is `-|<who> <event>` and its station stays `ops`;** one row cannot carry two
   stations, and splitting the verb is out of scope.
5. **A new harness case `g_shipTest 99`** was added to produce console output for both branches, in the
   `docs/evidence/the-computer-api.md` (`g_shipTest 85`) style.
6. **`docs/audit-whole-project.md` and `docs/gates.md` were not edited:** the review is a dated snapshot
   by its own header, and the ledger/register-check lane is a sibling on the other host.
