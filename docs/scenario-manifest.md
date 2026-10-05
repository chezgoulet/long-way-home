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
| `crew` | no | the crew and their posts on one map — see below |

`maps` may be empty when `crew.map` names a map the installation already has: a scenario that crews
an existing deck brings no map of its own.

## Two rules learned the hard way

**Use recursive globs.** `scripts/*.txt` declares only the top level of the scripts directory, so a
map referencing `voy9/intro` fails validation even though the file is sitting right there. Write
`scripts/**/*.txt`. The validator's `E002` catches this, but the message names the reference, not the
glob — read it as "your patterns did not cover this".

**Declare scripts, do not glob everything.** A bare `**/*.txt` over a shipped content tree picks up
sound tables, configuration files, editors' backups (`.bak.txt`) and directory lists. Those are not
ICARUS scripts and correctly fail to compile (`E006`). A scenario says what it contains; the globs
are how it says it.

## The `crew` section — the contract between authoring and the direction layer

The charter fixes this as one documented format: Track B writes it, Track C reads it, and neither
invents fields. `tools/crewgen` owns its rules; the validator calls it (`E008`), and
`crewgen.py build` turns it into the `maps/<map>.crew` file the game module loads.

```json
"crew": {
  "map": "tour/deck04",
  "max": 6,
  "bounds": {"reach_ms": 90000, "ack_ms": 5000, "save_bytes_per_npc": 256},
  "members": [
    {"name": "watch1", "type": "Renner", "at": [-2306, -3192, -3936], "yaw": 142},
    {"name": "Laird"}
  ],
  "posts": [
    {"name": "post1", "at": [-2224, -3192, -3936], "yaw": 145, "holder": "watch1"},
    {"name": "transporter", "navgoal": "transporternav1", "priority": 1}
  ]
}
```

| key | required | meaning |
|---|---|---|
| `map` | yes | the map as the game names it: `tour/deck04`, not `maps/tour/deck04.bsp` |
| `max` | no | most crew the layer will direct (default 10, ceiling 32) |
| `bounds` | no | the numbers the run is judged against: `reach_ms`, `ack_ms`, `stuck_ms`, `stuck_strikes`, `sample_ms`, `coverage_percent`, `save_bytes_per_npc` |
| `members` | no | the crew. Omitted, every Starfleet NPC on the map is crew, up to `max` |
| `posts` | no | the posts. Omitted, every `waypoint_navgoal` on the map is a post |

A **member** is either an NPC the map already places — `{"name": ...}`, matched against its
`NPC_targetname` — or one the layer spawns: `name`, `type` (a type from the game's own NPC table)
and `at`, with an optional `yaw`. Spawned crew are existing characters with their own models and
voices; nothing new is authored.

A **post** has a `name` and exactly one of `at` (a position) or `navgoal` (the targetname of a
`waypoint_navgoal` in the map, whose position it borrows). Optional: `yaw` (the way the holder
faces), `radius` (default 24), `holder` (a member's name or type; without it the nearest free
member takes the post) and `priority` (lower is more important — it decides which post the
director empties to refill another).

With `--data`, the section is also checked against the map itself, read from the installation:
navgoals must exist, types must be in the NPC table, placed members must be placed, and every
authored position must lie within 96 units of a waypoint — off the navigation graph, nothing can
walk there.

`crewgen.py suggest --data DIR --map NAME` proposes a section from the map's waypoints. It is a
starting point: it cannot see doors, and only a measured run (`scripts/g3-measure.sh`) shows
whether each member can actually reach its post.

### The `.crew` file

What `crewgen.py build` writes and the module reads — generated, never edited:

```
crew max 6
bound reach_ms 90000
budget save_bytes_per_npc 256
member watch1 type Renner at -2306 -3192 -3936 yaw 142
member Laird
post post1 at -2224 -3192 -3936 yaw 145 holder watch1
post transporter navgoal transporternav1 priority 1
```

A malformed file is rejected whole, with the line number, and the layer falls back to its
defaults: a half-read configuration is worse than none.

## What the validator checks

`E001` unknown entity class · `E002` script reference not among the declared scripts ·
`E003` inhabited space with no navigation · `E004` scenario-owned asset missing ·
`E005` manifest malformed · `E006` script fails to compile · `E007` compiled stream fails to read
back · `E008` crew section invalid, or inconsistent with the map it names · `W001` declared but
unreferenced script · `W002` retail asset references (informational) · `W003` checks skipped ·
`W004` space with no navigation, not declared inhabited · `W005` models or sounds approaching an
engine limit · `W006` configstring budget · `W007` crew section advisory (outside the G3 bar, or
more crew than posts).

Exit status is 0 when there are no errors; warnings never fail a build.
