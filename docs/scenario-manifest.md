# The scenario manifest

A scenario is a directory with a `scenario.json` at its root. It is the contract between the
authoring tooling and everything that consumes authored content: what the scenario contains, which
spaces are meant to be inhabited, and what the validator should hold it to.

```json
{
  "name": "deck-9-shift",
  "maps":      ["maps/**/*.map"],
  "scripts":   ["scripts/**/*.txt"],
  "inhabited": ["maps/deck09.map"],
  "asset_roots": ["assets"]
}
```

| key | required | meaning |
|---|---|---|
| `name` | yes | scenario identifier |
| `maps` | yes | globs, relative to the scenario root |
| `scripts` | yes | globs, relative to the scenario root |
| `inhabited` | no | globs for spaces that must have navigation coverage |
| `asset_roots` | no | namespaces the scenario owns (documentation; the validator keys off `assets/`) |

## Two rules learned the hard way

**Use recursive globs.** `scripts/*.txt` declares only the top level of the scripts directory, so a
map referencing `voy9/intro` fails validation even though the file is sitting right there. Write
`scripts/**/*.txt`. The validator's `E002` catches this, but the message names the reference, not the
glob — read it as "your patterns did not cover this".

**Declare scripts, do not glob everything.** A bare `**/*.txt` over a shipped content tree picks up
sound tables, configuration files, editors' backups (`.bak.txt`) and directory lists. Those are not
ICARUS scripts and correctly fail to compile (`E006`). A scenario says what it contains; the globs
are how it says it.

## What the validator checks

`E001` unknown entity class · `E002` script reference not among the declared scripts ·
`E003` inhabited space with no navigation · `E004` scenario-owned asset missing ·
`E005` manifest malformed · `E006` script fails to compile · `E007` compiled stream fails to read
back · `W001` declared but unreferenced script · `W002` retail asset references (informational) ·
`W003` checks skipped · `W004` space with no navigation, not declared inhabited.

Exit status is 0 when there are no errors; warnings never fail a build.
