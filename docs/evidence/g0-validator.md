# G0 evidence — scenario validator

Date: 2026-10-04. Gate: **G0, item 3** (asset validator).
Result: **met** — the negative-test criterion passes, and the validator has been run against every
published shipped map.

## The gate criterion

The charter's acceptance for this item is: *negative tests — every seeded, deliberate fault is
caught and correctly named.* `tools/validator/tests/negative_tests.py` builds a clean fixture around a
real shipped map source, confirms it validates with no errors, then seeds one fault at a time:

```
PASS  clean fixture validates with no errors
PASS  E001 unknown entity class: expected E001 naming 'Foo_Unknown_Class' (rc=1)
PASS  E002 missing script: expected E002 naming 'no_such_script' (rc=1)
PASS  E003 map without navigation: expected E003 naming 'empty_room.map' (rc=1)
PASS  E004 unresolved internal asset: expected E004 naming 'absent_shader' (rc=1)
PASS  E005 malformed manifest: expected E005 naming 'scenario.json' (rc=1)
PASS  E006 script fails to compile: expected E006 naming 'deck.txt' (rc=1)
ALL PASS
```

Six fault classes, each asserted twice: that the run fails, and that the report names the offending
object. Fixtures are generated at run time from supplied shipped content; no third-party material is
stored in the repository.

## Real-world run: every published shipped map

A scenario declaring all eleven published map-source archives and the shipped script corpus, checked
with the compiler and reader wired in. Findings, all of them about our inputs rather than about the
maps:

- **4 × E001 unknown entity class** — the two known gaps (`light_generic_street_old`,
  `misc_model_scav_sc_scanner`) reported twice because that manifest globbed maps with two
  overlapping patterns. The validator now de-duplicates expanded paths.
- **126 × E002 script references not declared** — real: maps reference scripts that the Game
  Development Kit's script archive does not contain, which is consistent with the expansion's
  scripts shipping separately from the base corpus. Worth resolving before any expansion work.
- **2 × E003 inhabited but no navigation** — including `voy10.map`, which my own `inhabited`
  declaration wrongly swept in with a broad glob. The validator caught my declaration, not a defect
  in the map. `W004` (73 maps) correctly reports the spaces that have no navigation and do not claim
  to be inhabited: a firing range has no crew walking it, and that is fine.
- **11 × E006 non-script files that fail to compile** — sound tables, configuration and `.bak.txt`
  editor backups swept in by a `**/*.txt` glob. Not scripts; the declaration was too broad.
- **1,015 × W001 declared but unreferenced scripts** — expected when declaring an entire corpus.
- **206 × W002 retail asset references** — informational by design: we hold no retail data, so
  retail paths cannot be verified and asserting otherwise would cry wolf on every real map.

## What this changed in the design

Running the validator against real content, rather than only against fixtures, is what exposed two
genuine design flaws and one documentation gap:

1. **E002 originally resolved references with a filesystem walk under the scenario root.** A scenario
   *declares* its scripts, and that declaration is what a reference must satisfy. Rewritten to use
   the declared set.
2. **Navigation coverage was an assumption, not a declaration.** Some shipped spaces have no waypoint
   entities because nothing walks there. `inhabited` is now an explicit manifest key: `E003` fires
   only for spaces the scenario claims crew will occupy, `W004` reports the rest.
3. **The manifest rules are now written down** in `docs/scenario-manifest.md`, including the two
   footguns this run found: non-recursive globs silently under-declare, and globbing every `.txt`
   pulls in non-scripts.
