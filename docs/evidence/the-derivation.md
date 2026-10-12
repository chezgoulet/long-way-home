# The derivation — the crossing, and the evidence for it

**Pass:** the fourth question of the primitive crossing (`docs/path-to-playtest.md`, *The primitive
crossing*): for every field a scenario must be able to declare, which primitive answers it and how the
engine calls it. The table is `docs/the-derivation.md`. Nothing was built or changed; this is an inventory.

**Host:** static. No engine, no game data. Hooks, console verbs and the test suite were run here; the
rendered surfaces were not.

---

## 1. The public surface is 479 functions, and the register agrees

```sh
python3 tools/hooks/surface.py module/ship/ship_core.h | wc -l
```

```
479
```

```sh
bash scripts/hooks-check.sh
```

```
hooks-check: module/ship/ship_core.h declares 479 public functions; docs/hook-register.md registers 479 hooks
PASS  every public function is registered, and every registered hook exists
```

## 2. The console enumeration agrees with the console, both directions

```sh
bash scripts/api-check.sh
```

```
verbs.py: module/ship/g_ship.cpp answers 133 verb(s); module/ship/console_api.def enumerates 133
WARN  the verb 'breach' is claimed twice and both can be reached (they differ by argument shape)
WARN  the verb 'recall' is claimed twice and both can be reached (they differ by argument shape)
PASS  every console verb is in the ship's API, and every API verb is answered by the console
```

## 3. The scenario manifest carries no field the contract names

```sh
grep -n 'for key in' tools/validator/validate.py
```

```
125:	for key in ("name", "maps", "scripts"):
```

The optional keys are `inhabited`, `asset_roots` and `crew` (`docs/scenario-manifest.md:17-24`). There is no
key for breeding conditions, systems stressed, staff needed, cost, residue, visibility, warmth, positions,
the canon root or the decision. **No declaration reaches any primitive** — the crossing's one-sidedness.

## 4. The three rows marked *ready*, each witnessed independently

**Systems stressed** — the console verbs and the module functions.

```sh
bash scripts/api-check.sh      # PASS, above: on / off / priority / alloc / damage / repair are enumerated
bash scripts/hooks-check.sh    # PASS, above: UseSystem, SetEnabled, SystemIdByName are declared
```

**The authority they hold** — the unit suite, which includes the deck's authority.

```sh
PATH="/home/robot/.cache/lwh-venv/bin:$PATH" bash scripts/test.sh
```

```
==> ship simulation: unit tests
...
ship_core: all checks passed
...
==> api: the console's verbs and the ship's API agree, both directions
PASS  every console verb is in the ship's API, and every API verb is answered by the console
...
all checks passed
```

`tests/ship/test_ship_core.cpp` contains `TestRankAndRoles` (`:3613`), `TestAccessAndAuthority` (`:3678`)
and `TestOodDispatch` (`:8600`) — the last being the reconciliation's own proof that `MayOrder` consults the
officer of the deck (`docs/designs-against-code.md` C-11, fixed 2026-10-12).

**Warmth** — the manner primitive, end to end.

```sh
bash scripts/manner-check.sh
```

```
PASS  the manner is selected from the state (steady and the bad states, each named, with the
      precedence when several are true); raised where the state changes and where the person
      acts; written to the log and handed to the voice; degrades to text with no bank or no
      render; the same state selects the same manner and a save replays it -- and the planted
      state before this pass makes the check fail.
```

Each of the three witnesses is a script that **fails on a planted error** as well as passing on the tree
(`docs/evidence/the-computer-api.md`, `docs/evidence/the-manner.md`, and the register's own falsifications);
a call path verified only by reading would not be evidence.

## 5. The inventory, derived

```sh
python3 tools/hooks/surface.py module/ship/ship_core.h > /tmp/hooks.txt
# the field rows in docs/the-derivation.md name 130 hooks; the complement is 349
```

```
total hooks: 479
named in doc: 130
not named (inventory): 349
  159  4. Readers content can consult
   73  2. Situations the simulation generates on its own
   53  1. Entry points — what content can call
   29  3. Outcomes content can ask for
   14  3b. Orders (docs/orders.md)
   13  3c. The bills (docs/crew-manifest.md)
    8   5. The configurator — the player chooses the start state
```

The 349 are listed in full in `docs/the-derivation.md`.

## 6. What was not run, and the command that would run it

- **The rendered surfaces** (does a station screen show the portfolio; does a warmth beat read as warmth;
  does the post-not-name law hold across authored content): on the playtest host,
  `scripts/run-lwh.sh` and `scripts/run-scenario.sh`, in a logged-in session.
- **The full suite on the branch** was run here (`scripts/test.sh`, exit 0); the checks that need the GDK
  (`scripts/check.sh`) and the game (`scripts/g3-measure.sh`) were not, because the game data is
  read-only on `sasquatch` and this is a static host.
