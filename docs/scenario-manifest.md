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

**Case counts on Linux, and the game's filesystem does not know that.** The GDK ships the same tree
as `foo.txt` and `Foo.TXT`; `glob` is case-sensitive on Linux, so `scripts/**/*.txt` silently drops
every `.TXT` file and `E002` then names the reference rather than the glob that missed it. Declare
the extension case-insensitively — `scripts/**/*.[tT][xX][tT]` — or the corpus will look far more
incomplete than it is. This single trap, on a corpus also missing the GDK's own copy of the tree,
is what inflated "126 script references the GDK corpus lacks" out of a boundary of two
(`docs/evidence/the-126-references.md`).

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
back · `E008` crew section invalid, or inconsistent with the map it names ·
`E009` declaration field invalid — an unknown key, a wrong type, or a value outside its vocabulary ·
`E010` the post-not-name law broken — content names a crew member instead of addressing a post ·
`E011` a line declares no delivery direction, or one not in the engine's set ·
`W001` declared but unreferenced script · `W002` retail asset references (informational) · `W003` checks skipped ·
`W004` space with no navigation, not declared inhabited · `W005` models or sounds approaching an
engine limit · `W006` configstring budget · `W007` crew section advisory (outside the G3 bar, or
more crew than posts).

Exit status is 0 when there are no errors; warnings never fail a build.

## The declaration contract — what a scenario declares

`docs/scenario-atlas.md` says a scenario is *declared*, not thrown: it states its breeding conditions, the
systems it stresses, the staff it needs, the cost to answer or to ignore, its residue, its visibility and its
warmth, the positions it can be experienced from, and the decision it owns — and it is instantiated **through
the post**, never through a named person (`docs/the-entry-point.md`, part three). Those fields are now
manifest keys, validated against the design that owns each one. `tools/validator/declarations.py` is the one
machine form of this contract: the validator and the builder both read it, so they cannot disagree about what
a field is, and `scripts/scenario-check.sh` proves the loader carries every field and no field the schema
does not name.

| key | type | meaning | allowed values | the design line |
|---|---|---|---|---|
| `canon_root` | string | the fiction's own root, the slate entry's name | free text; it may not name a crew member | `docs/scenario-atlas.md:184` (*"each names its canon root"*) |
| `breeding_conditions` | array of `{state, op, value}` | which ship state makes it eligible | `state` ∈ the readable state below; `op` ∈ `<` `<=` `>` `>=` `==` `!=`; `value` a number (or `true`/`false` for `coreless`) | `:70` (*"which ship state makes it eligible"*) |
| `systems` | array of a system id, or `{id, stress?, off?}` | the systems it reads and writes, by our system IDs | the 23 shipped system names (`SPECS[]`, `module/ship/ship_core.cpp`), or `<system>.<arc>` for one of canon's six arcs; the optional `stress`/`off` is the initial load | `:71` (*"the systems it reads and writes, by our system IDs"*) |
| `staff` | array of a post | the posts it requires, so the crew model is the cost | a system id (the crew post vocabulary), one of the nine departments, or a command seat | `:72` (*"posts it requires, so the crew model is the cost"*) |
| `cost.answer`, `cost.ignore` | `{text, effects: [...]}` | what answering spends, and what ignoring lets accrue | each `effects` entry is an effect kind below | `:73`; `:25-27` (*"ignoring something is not failing — it is a different state"*) |
| `residue` | array of a string or an effect | what is permanently different afterwards | a lasting thing in words, or an effect kind | `:73` (*"what is permanently different afterwards"*) |
| `visibility` | array | what the player sees *where they are* | a station (`engineering`, `tactical`, `operations`, `conn`, `sickbay`), a system id, or `deck:<n>` | `:74` (*"what the player sees where they are"*) |
| `warmth` | array of a beat | the human beat it leaves behind | `holodeck`, `quarters`, `funeral`, `promote`, `rations`, `airponics`, `promise`, `manner`, `wall` | `:76` (*"the human beat it leaves behind"*) |
| `positions` | array of `{post, information, authority, responsible_for, decision}` | the positions it can be experienced from | `post` a post; `information` ∈ `log`, `brief`, `console`, `station` or a readable state; `authority` ∈ `operate`, `call_alert`, `command`, `delegate`, `override`, `ood`; `responsible_for` a post or department | `:139-141` (*"the positions it can be experienced from … the information they receive, the authority they hold, the people they are responsible for, and the decision they own"*) |
| `decision` | `{question, options: [{label, cost: [...]}]}` | the scenario's own decision | each option's `cost` is a list of effect kinds | `:184` (*"each names … its decision"*) |
| `lines` | array of `{to, delivery, text}` | the content spoken, delivered | `to` a post; `delivery` ∈ `order`, `report`, `confession`, `condolence`, `flat`; `text` may not name a crew member | `:78-87`; the delivery vocabulary is `docs/gates.md` (*"every line carries a delivery direction"*) |

**The effect kinds** a cost or a residue writes, each an existing primitive: `write_off`, `damage_system`,
`shield_hit`, `ignite`, `breach`, `kill`, `sicken`, `promote`, `take_survivors`, `fabricate_parts`,
`fabricate_rations`, `air_source`, `run_holodeck`. An effect that acts on a person acts on a **post** — the
law holds inside an effect too.

**The readable state** a breeding condition may name: `deuterium`, `antimatter`, `batteries`, `torpedoes`,
`spare_parts`, `materials`, `rations`, `probes`, `patients`, `refugees`, `resentment`, `borg_awareness`,
`power_shortfall`, `coreless`, `intruders`, `minutes_of_air`, `deck_life_support`, `system_condition`. These
are the readers `docs/the-derivation.md` lists under *Breeding conditions*.

### The two laws, and why they are checked rather than trusted

- **The post-not-name rule.** Because the player may field an all-fictitious crew (`docs/the-entry-point.md`,
  part three), content addresses a post, a system id or a role; the simulation resolves it to a person. A
  manifest whose content names a crew member — its own declared members, or the canon command crew — is
  **refused** (`E010`). The check is mechanical: the validator knows the manifest's own `crew.members`, and
  the canon names are read from the engine's own seat table rather than copied.
- **The delivery direction per line.** A line declares how it is delivered, from the engine's `Delivery`
  vocabulary (`order` / `report` / `confession` / `condolence` / `flat`), rather than that being assumed. A
  line with no `delivery`, or an unknown one, is **refused** (`E011`). `unmarked` is not authored content:
  where a brief cannot know a direction the loader marks the line, and the synthesis seam refuses it.

### An unknown key is refused

The manifest is closed against the frame keys (`name`, `maps`, `scripts`, `inhabited`, `asset_roots`,
`crew`) and this contract. Any other top-level key is **refused** (`E009`), because a key nothing reads is a
field an author believes in that the game ignores — the failure this pass exists to prevent.

### Where the design is silent, the shape is ours

The design names the fields and their meaning; it does not fix every shape, and where it is silent the shape
is decided here, once, so it can be overruled:

1. **`breeding_conditions` is a list of `{state, op, value}`** rather than a free-text sentence, so it is a
   thing a predicate can read.
2. **`systems` entries may carry an initial load** (`stress`, `off`); the bare id is the systems the scenario
   reads and writes with no initial change.
3. **`positions` is a list of objects with the four sub-fields the design names**, and `post` is the only
   required one.
4. **A cost or residue is a list of effect objects** over the vocabulary above, rather than prose.
5. **`visibility` is a station, a system id or `deck:<n>`** — the three places a person can be standing.

### What the fields reach — the loader, and the evaluator

`module/ship/scenario_core.cpp` parses the compiled declaration and **instantiates what can be
instantiated now**: the systems it stresses (off, or under load), the staff coverage (the two
primitives the derivation found reached by nothing but the tests), the warmth beat, and every position
and line, each **resolved from its post to a person**. It also **reads the breeding conditions and
decides the scenario fires** — the evaluator, `scenario::Evaluate` / `Recruit` / `Resolve` in the same
file. The worked example is `scenarios/deck11-gel-pack/`; the evidence is
`docs/evidence/the-manifest-contract.md` (the loader) and `docs/evidence/the-evaluator.md` (the
evaluator).

The evaluator reads **all 18** readable states. Fifteen are single numbers; three — `system_condition`,
`deck_life_support`, `minutes_of_air` — are **scoped** (each names a system or a deck) and the
`{state, op, value}` tuple carries no subject, so they are read as the ship's worst: the most-damaged
system (its **health**, because the power-capped `SystemCondition` reads 0 for every standby weapon),
the least-supported deck, the deck soonest out of air. That default is ours, recorded here so it can be
overruled: the shape should grow a subject. The design's own breeding facts that no key reaches at all
— a contact, a battle, a beacon, a breach, structural integrity, the clock — are named in the evidence
as the next pass's vocabulary.

### The two authoring laws, at runtime

The validator enforces the post-not-name law and the delivery direction at load (`E010`, `E011`). The
evaluator holds them **at runtime** too: `Recruit` resolves every line's post to whoever holds it (a
canon officer or a fictitious ensign, the content unchanged), and `Resolve` maps each cost and residue
effect onto a primitive, resolving a `post` inside an effect the same way. The scenario's own ledger
entry names the scenario, never a person.

