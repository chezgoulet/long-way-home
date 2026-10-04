# Prior art: RPG-X

What the longest-lived Elite Force project in the scene already solved, read from its source
(`UberGames/rpgxEF` — the engine fork *with* the multiplayer gamecode, under the same Raven source
licence we build under). Recorded because three of its systems sit directly on our path, and one of
my earlier assumptions about it was wrong.

## What RPG-X is

A total conversion of Elite Force's multiplayer into a Star Trek roleplay environment: released 2004,
still running through The Last Outpost, re-released as a standalone with permission. Its own
description is explicit that it has **no missions, no campaigns, and no goals** — it is a stage.

## Take: the rank and permission model (Track D)

Their gamecode carries a full **rank and permissions system**: `rank` appears 239 times and
`permission` 112, alongside `authorizer`, `rankSets`, `rankNames`, `rankMenuData`, `rankModelData`,
`rankTxt` and `authorizeAddress`.

This is the thing I identified as the MMO's core system — *who may reroute power, who may fire, who may
change course, who may relieve whom* — already implemented, in EF1's own multiplayer language, by people
who ran a crewed ship for two decades. It is the most directly reusable design we have found, and
`authorizeAddress` even suggests a notion of standing authorisation rather than per-action prompts.

## Take: persistence embedded in the game module (Track D, and G3's state budget)

They bundle **SQLite inside the gamecode** — `code/game/sqlite3.c`, `g_sql.c`, `g_sql.h`, and
`code/ui/ui_sql.c`. Character and ship state persist without an external database daemon: the game
module owns its own storage.

That is the shape our per-ship state wants, and it is a working precedent *in this engine family*
rather than a pattern imported from elsewhere. It also suggests how to hold G3's per-NPC state within
a budget: the mechanism for durable state already exists at the point where the state lives.

## Take: the emote and interaction vocabulary (capstone, G4)

`code/ui/ui_emotes.c` — a dedicated emote system. This is the "what can a body *do* here" layer, and it
is exactly what separates an inhabited space from a populated one. Our G3 deliberately excludes new
animation; the capstone will not, and this is the vocabulary to study when it stops excluding it.

## Take: content-limit headroom (Track B and the capstone)

RPG-X's engine fork is described as "ioEF with increased limits", and the actual increase is
**`MAX_CONFIGSTRINGS` 4096** against the 1024 our lineage carries. It also derives `MAX_GENTITIES` and
`MAX_MODELS` from bit widths rather than fixed counts.

**This corrects an assumption I made earlier.** I guessed their limits work was prior art for the
client-count question; it is not — `MAX_CLIENTS` is 64 in rpgxEF too. Their headroom is *content*
capacity, and that is the limit our authoring will meet first: every new shader, model and sound
consumes a configstring long before we run out of players. Worth knowing before the capstone adds
content rather than after.

## Take: the structural lesson about staying alive

Twenty years of continuous life came from **scheduled sessions and a community**, not from features.
That is evidence for what the design already claims: a crew game lives on a calendar. It supports
building the watch and session structure (the `watchbill` component) early rather than treating it as
an MMO-phase nicety.

## Adopted — and where each lands

Owner's decision, 2026-10-04: all four, with the limits item first because it constrains the others.

| taken | lands in | as |
|---|---|---|
| rank and permission model | **Track D** (ship authority) | the starting design for the chain of command, read from `rpgxEF` rather than invented |
| embedded SQLite persistence | **Track D** per-ship state, and **G3**'s per-NPC budget | the storage pattern: state lives with the code that owns it, no external daemon |
| emote / interaction vocabulary | **capstone and G4** | the reference for what a body can do, once G3 stops excluding new animation |
| content headroom | **Track B, before content work** | raise the limits ahead of the capstone, not after it overflows |

### The headroom item, with our numbers beside theirs

| limit | ours (`efcode`) | RPG-X | note |
|---|---|---|---|
| `MAX_CLIENTS` | 64 | 64 | unchanged in both — **this is not a player-count story** |
| `MAX_GENTITIES` | 1024 (`GENTITYNUM_BITS` 10) | 2048 (11 bits) | entities per level |
| `MAX_MODELS` | 256 (fixed) | 512 (`MODELNUM_BITS` 9) | |
| `MAX_SOUNDS` | 256 | 256 | unchanged |
| `MAX_CONFIGSTRINGS` | **1024** | **4096** | the one that fills first |

Two constraints the source states, and both bite when raising them:

- **`MAX_MODELS` and `MAX_SOUNDS` ride the network as 8 bits**, and the header says plainly they
  "cannot be blindly increased". Widening them changes the wire format, which is fine here — we build
  both ends — but it is a protocol change, not a constant.
- **`MAX_CONFIGSTRINGS` sizes `stringOffsets[]` in the gamestate against `MAX_GAMESTATE_CHARS 16000`.**
  Raising the count without raising the buffer moves the overflow rather than removing it, so the two
  move together.

### Measured, and it changes the order of work

Counting content registration across **all 106 published map sources** — models and sounds are the
classes that become configstrings per level, so the per-level figure is the one that matters:

| | worst level | median | cap |
|---|---|---|---|
| distinct models | **37** (`deck08`) | 5 | 256 |
| distinct sounds | **14** | 2 | 256 |

Retail is nowhere near the ceiling: no shipped level exceeds 256 models or 256 sounds, and the worst
spends roughly 50 configstrings of 1024.

### Raised anyway, on the owner's call — and here is the resolution of that disagreement

The measurement above argued for instrumenting and waiting. The owner overruled it, correctly: *"We're
running 25-year-old software on modern hardware. Unless there's a compelling reason not to, wouldn't
it make sense to up the limit?"* There was no compelling reason — the measurement answered "retail
does not need this yet", which is a different question from "do not raise it". Since authored content
grows into whatever ceiling exists, raising it before there is content to invalidate is strictly
cheaper than raising it after.

**Raised, in both copies of `q_shared.h` (`efcode/qcommon/` and `efgame/src/game/`):**

| constant | from | to | why it is safe, or what it costs |
|---|---|---|---|
| `MAX_CONFIGSTRINGS` | 1024 | 4096 | no network field references it; ~12 KB more in the struct |
| `MAX_GAMESTATE_CHARS` | 16000 | 64000 | sizes the same struct and must move with it |
| `GENTITYNUM_BITS` | 10 | 11 | **on the wire** (`NETF(otherEntityNum)` et al) — an internal format change, permissible because we build both ends and SP runs over loopback |

`MAX_MODELS` and `MAX_SOUNDS` stay at 256: also 8-bit on the wire, and with the worst shipped level at
37 models and 14 sounds there is no pressure to touch the format a second time.

The entity ceiling was the one worth raising now, because it is the limit with measured pressure:
**`borg1` loads 561 of 1024 entities**, while models sit at 37 of 256. A deck furnished with crew and
props binds entities long before it binds models.

**Verified on the playtest host**, patch series re-applied to both trees, module and engine rebuilt
clean, then run against the retail data:

```
GENTITYNUM_BITS 11   MAX_GENTITIES (1<<GENTITYNUM_BITS)   MAX_CONFIGSTRINGS 4096
21675 files in pk3 files
----- Client Initialization Complete -----
SP: map/transition/use/save/load commands registered
----- finished R_Init -----
ui: SP UI loaded (UI_API_VERSION=2)
```

Nothing regressed, and the SP UI still loads.

**Residual from this change:** saves written by the previous build should be checked once in a session
before being relied on — entity numbers are persisted, and the wire width changed. New saves are the
reference; retail saves were already incompatible.

### The instrument stays, because it is how we find out we were too conservative

The validator counts per-level content registration and warns at 60% of each limit (`W005` for
models/sounds, `W006` for configstrings), with a negative test that seeds a dense level and asserts the
warning is emitted. Across all 106 shipped maps it is silent — correct calibration — and it will speak
up when authored content approaches the new ceiling, rather than failing later as missing textures.

## Do not take

- **Their engine fork as a base.** We have a better-supported lineage: lilium/cMod, maintained, native
  on Linux, with our single-player path already proven on top of it.
- **Their premise.** No missions and no goals is the opposite of what the capstone is for. We take the
  interaction vocabulary, not the absence of story.
- **Their fragmentation.** RPG-X needed its own client; the modern scene consolidated on cMod. One
  engine, one module boundary, no new fork.
