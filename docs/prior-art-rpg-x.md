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

The measurement to run before the capstone adds content: log configstring usage on a loaded campaign
map, and find out how much of the 1024 retail SP material already spends. Content that quietly runs out
of configstrings fails in ways that look like missing textures rather than missing capacity.

## Do not take

- **Their engine fork as a base.** We have a better-supported lineage: lilium/cMod, maintained, native
  on Linux, with our single-player path already proven on top of it.
- **Their premise.** No missions and no goals is the opposite of what the capstone is for. We take the
  interaction vocabulary, not the absence of story.
- **Their fragmentation.** RPG-X needed its own client; the modern scene consolidated on cMod. One
  engine, one module boundary, no new fork.
