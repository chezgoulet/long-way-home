# Evidence — the latent traps: three limits, measured, then classified

Date: 2026-10-11. Branch: `feat/the-bridge-traps`, cut from `testing@0312dca`. Brief: `BRIEF.md`
(the latent traps). Documents: `docs/engine-extension-policy.md`, and the three dated evidence files
the brief quotes — `docs/evidence/engine-content-headroom.md`, `docs/evidence/raise-the-gentity-ceiling.md`,
`docs/evidence/performance-survey.md`, `docs/evidence/s8-the-borg.md`.

Nothing here is asserted without the command that produced it. The command is given before its output.

**The headline, before the numbers.** Of the three, **two are defects and both are fixed** — the SP
bridge's configstring table (a literal 1024 that dropped content with no word) and the two
same-named `cg_public.h` headers whose resolution was silently left to include-path order. The
third — the published decks share shader names, so `RemapShader` cannot address one deck alone — is a
**boundary**: the names are the publisher's content, and the way round is the mechanism the engine
already offers (deck-unique names, which our own generated decks use). A fourth, in the same
evidence, is also a boundary and is priced, not taken: q3map2's `MAX_MAP_VISCLUSTERS`.

The patch series is now **22 patches**; this pass adds `0021` (the configstring table) and `0022`
(the header guard). Both are engine-side and compile-time, so the mode-1 guarantee is structural:
mode 1 compiles both ends from this tree, and mode 2 is a separate prebuilt application this tree
does not build.

## The measurements, and whether each dated claim still holds

| the evidence said | read on this tree | verdict |
|---|---|---|
| `sp_bridge.cpp:113` hard-codes `CS_MAX 1024` | `sp_bridge.cpp:114` — `#define CS_MAX 1024` | **still holds** (the value; the line moved one, comments above it) |
| two same-named headers carry 256 and 1024 | `efgame/src/cgame/cg_public.h:17` = 1024; `EFAndroid-SP/app/jni/efcode/cgame/cg_public.h:32` = 256 | **still holds** |
| the published decks share shader names | 10 decks, 447 distinct shaders, **190 on more than one deck, 15 on all ten** | **still holds** |
| a sixth full-detail re-dress needs more detail aliases or a higher q3map2 cap | ship at **15,321 of 16,384 vis-clusters (93.5%)**; the cap is q3map2's | **still holds** |

The engine was rebuilt from the patched upstream (`cmake --build build-engine`) and the module from
`module/` (`cmake --build ../upstream/efgame/build-linux`); the binary is `build-engine/longwayhome`.
Every run below deletes `baseEF/*.pid` first (a stale pid is the "safe video settings" hang).

## 1. The SP bridge's configstring table — a defect, fixed

### The measurement

```
$ find build/home/baseEF -name '*.pid' -delete
$ SDL_AUDIODRIVER=dummy xvfb-run -a scripts/run-engine.sh --home-dir build/home \
      +set com_hunkMegs 768 +set developer 1 +set s_useOpenAL 0 +set s_initsound 0 +map borg1
EFSP: SP_SpawnServer: content: 104 configstrings of 4096 (bridge table CS_MAX 1024): 46 models of 256, 53 sounds of 256; gentities 561 of 8192
```

Read plainly: the bridge's table is 1024 while `MAX_CONFIGSTRINGS` is 4096. The game's own layout
bound (`CS_MAX` in `bg_public.h`, `CS_PLAYERS + MAX_CLIENTS` = 522 + 1 = **523**) fits under the
literal today, so nothing is dropped by the shipped content — but the table is not derived from
anything, and `I_SetCS` drops an index at or above it with no word. `MAX_MODELS`/`MAX_SOUNDS` are
what bound the *content* (256 each, 46 and 53 spent on `borg1`), so the trap is latent, not live:
it fires the day models or sounds move, and it fires silently. That is a defect in the strict sense
the brief means — a limit that would eat content quietly — and a one-constant one.

### The fix — `patches/0021-bridge-configstring-table.patch`

The table is now `SP_CS_MAX`, derived from `MAX_CONFIGSTRINGS`:

```c
#define SP_CS_MAX MAX_CONFIGSTRINGS
static char g_configstrings[SP_CS_MAX][1024];
```

so it can never again disagree with the ceiling, and it holds every index the game's layout is
allowed to name. The name is not cosmetic: `bg_public.h`, included for the content blocks, already
defines `CS_MAX` (the game's own layout bound), and the old `#define CS_MAX 1024` silently redefined
it under the build's `-w`. An out-of-range set now says so under `developer`, and an index past the
old literal that lands says so too, so the two states are distinguishable in the engine's own output.
Logging only with `developer`: with it unset (the default) `I_SetCS` stores and drops exactly as
before, and no save changes (the save writes only the non-empty entries it always did).

### Demonstrated — the value survives, and the engine says so

```
$ scripts/bridge-cs-check.sh
==> the bridge's configstring table
    SP_SpawnServer: content: 104 configstrings of 4096 (bridge table SP_CS_MAX 4096): 46 models of 256, 53 sounds of 256; gentities 564 of 8192
    SP_SetConfigstring: index 1500 accepted (bridge table SP_CS_MAX 4096)
SHIP: configstring probe: index 1500 -> "lwh-cs-beyond-1024"
PASS  a configstring numbered past the bridge's old literal 1024 survives the bridge, and the
      engine says so in its own output (SP_SetConfigstring: index 1500 accepted); before patch
      0021 the same run prints an empty read-back -- point --engine at that build to see it fail
```

`g_shipTest 95` (a `developer`-gated harness instrument, `module/ship/g_ship.cpp`, `RunTest`) sets
configstring 1500 — past the old literal — through `gi.SetConfigstring` and reads it back through
`gi.GetConfigstring`; index 1500 is above the game's own layout bound (523), so nothing the game
registers is disturbed. The acceptance's bar is met by this run: an index past the old limit survives
the bridge, and the engine's own line names it accepted.

### The same run, before the fix — the two states are distinguishable

The unfixed engine was built from a worktree with `patches/0001…0020` only
(`git worktree add /tmp/opencode/upstream-0020 0d8942e`; `git apply 0001…0020`;
`cmake -S engine -B /tmp/opencode/build-engine-0020 -DUPSTREAM_DIR=/tmp/opencode/upstream-0020 …`):

```
$ scripts/bridge-cs-check.sh --engine /tmp/opencode/build-engine-0020/longwayhome
==> the bridge's configstring table
    SP_SpawnServer: content: 104 configstrings of 4096 (bridge table CS_MAX 1024): 46 models of 256, 53 sounds of 256; gentities 564 of 8192
SHIP: configstring probe: index 1500 -> ""
FAIL  the bridge's table is not derived from MAX_CONFIGSTRINGS (no 'bridge table SP_CS_MAX 4096')
```

The same source, the same module, the same probe: before the fix the index is dropped and the
read-back is empty; after it the index survives and the engine says it accepted it. `bridge-cs-check.sh`
exits non-zero on the unfixed engine, which is what makes its green mean something.

### A save still round-trips

The table is what the save writes its configstrings from, so the round-trip is re-measured on the
fixed build (the save writes only the non-empty entries; nothing above index 522 exists, so the
bytes are the same):

```
$ … +set developer 1 … +map borg1
EFSP: save: wrote saves/auto.sav (map borg1, t=1000, eAUTO)
EFSP: SP_WriteEntryAutosave: wrote eAUTO level-entry autosave for borg1
$ … +set developer 1 … +load auto
EFSP: load: saves/auto.sav -> map borg1 t=1000 (deferred reload)
EFSP: SP_LoadGame: ge=0x7bfe66e503e0 apiversion=6 gentitySize=1320
EFSP: SP_FinishTransition: done numEnt=561 ps.origin=(104 -936 5)
```

## 2. The two same-named `cg_public.h` — a latent defect, pinned

### The measurement — which translation unit actually gets which

There are two files named `cg_public.h` in the checkout, defining the *same-named* struct
`snapshot_t` at different sizes:

```
$ grep -n 'MAX_ENTITIES_IN_SNAPSHOT' ../upstream/efgame/src/cgame/cg_public.h \
      ../upstream/EFAndroid-SP/app/jni/efcode/cgame/cg_public.h
../upstream/efgame/src/cgame/cg_public.h:17:#define	MAX_ENTITIES_IN_SNAPSHOT	1024
../upstream/EFAndroid-SP/app/jni/efcode/cgame/cg_public.h:32:#define	MAX_ENTITIES_IN_SNAPSHOT	256
```

Which one a translation unit gets is decided by include-path order, not by the source. The bridge's
`#include "../cgame/cg_public.h"` resolves to `…/jni/cgame/cg_public.h` (absent) and then, on the
`-I` list, to `<dir>/../cgame/…` where `<dir>` is the first match. The build's own dependency files
state the resolution, one per translation unit:

```
$ grep -m1 'cgame/cg_public.h' build-engine/CMakeFiles/sp_game_bridge.dir/…/sp_bridge.cpp.o.d
 /home/c/big/git/upstream/efgame/src/game/../cgame/cg_public.h \        # the bridge -> the module's, 1024
$ grep -m1 'cgame/cg_public.h' build-engine/CMakeFiles/longwayhome.dir/…/client/cl_cgame.c.o.d
 /home/c/big/git/upstream/EFAndroid-SP/app/jni/efcode/client/../cgame/cg_public.h \   # the network client -> the engine's, 256
$ grep -m1 'cg_public.h' ../upstream/efgame/build-linux/CMakeFiles/efgame.dir/…/cg_main.cpp.o.d
 /home/c/big/git/upstream/efgame/src/cgame/../game/../cgame/cg_public.h \             # the module's cgame -> the module's, 1024
```

So today: the bridge and the module's cgame share the module's **1024** header (the in-process SP
snapshot), and the engine's network client compiles against its own **256** header (`cl_cgame.c:357`).
They are not interchangeable: the bridge passes a `snapshot_t` to the cgame *in process*
(`CG_GETSNAPSHOT` is a struct copy, no wire), and the module's own header says so — "BOTH .so MUST be
rebuilt together or `snapshot_t` sizes mismatch -> memory corruption" (`efgame/src/cgame/cg_public.h:15`).
Nothing fails today only because the include path happens to resolve the bridge to the module's file.

### Defect or boundary — it is a latent defect, and the fix is a pin

The two *values* legitimately differ (the network client's cap is bound to the wire; the in-process
SP cap is not). The defect is that the resolution is **silent and order-dependent**: point the bridge's
include path at `efcode/cgame` and the engine's 256 header compiles just as happily, silently halving
`snapshot_t` and corrupting the copy — the exact failure the module header warns of, reachable by an
edit to `engine/CMakeLists.txt` that looks innocent. `docs/evidence/performance-survey.md` records it
as "a hazard, not fixed"; a hazard that a one-line edit can trip is worth one line to pin.

### The fix — `patches/0022-pin-the-bridge-cg-header.patch`

A preprocessor guard in `sp_bridge.cpp`, next to the includes, with the two paths named in the
comment:

```c
#if MAX_ENTITIES_IN_SNAPSHOT != 1024
#error "sp_bridge.cpp must compile against the module's cg_public.h (MAX_ENTITIES_IN_SNAPSHOT 1024,
        the in-process SP snapshot), not the engine's network cg_public.h (256): check the
        sp_game_bridge include paths."
#endif
```

Compile-time only: no behaviour, save or wire change.

### The falsification — the real source, twice

`scripts/bridge-header-check.sh` preprocesses the *real* `sp_bridge.cpp` with the bridge's own
include path (the module's header first), then with the engine's cgame directory put ahead of it:

```
$ scripts/bridge-header-check.sh
==> the two headers
    efgame/src/cgame/cg_public.h :#define MAX_ENTITIES_IN_SNAPSHOT 1024
    EFAndroid-SP/app/jni/efcode/cgame/cg_public.h :#define MAX_ENTITIES_IN_SNAPSHOT 256
==> the bridge as built: the module's header resolves first, and the guard passes
==> the wrong header: the engine's cgame ahead of the module's, and the guard fires
    …/sp_bridge.cpp:38:2: error: #error "sp_bridge.cpp must compile against the module's cg_public.h (MAX_ENTITIES_IN_SNAPSHOT 1024, the in-process SP snapshot), not the engine's network cg_public.h (256): check the sp_game_bridge include paths."
PASS  the module's cg_public.h (1024) resolves for the bridge and the guard holds; pointing the
      include path at the engine's network cg_public.h (256) fails the build instead of silently
      halving snapshot_t
```

The second half is the falsification: the guard fails the build for the reason it was added. The
first half is not a substitute for it and is not offered as one.

## 3. The published decks share shader names — a boundary, priced

### The measurement

`gi.RemapShader` addresses a shader **by name**, globally: `RE_RemapShader` remaps every shader
carrying that name (`renderervk/tr_shader.c`, "remap all the shaders with the given name"), and the
comment is the mechanism. The published Virtual Voyager deck sources name their surfaces with
shared names:

```
$ python3 tools/shipmap/shader_census.py --decks build/gdk/maps/eliteforce_virtualvoyager_maps
  10 deck(s), 447 distinct shader(s), 190 named by more than one deck, 15 by all 10
    all 10: common/areaportal
    …
    all 10: hall/hallfloor3
    all 10: voyager/wall2
    …
```

Fifteen shaders, including visible ones the decks draw with (`hall/hallfloor3`, `voyager/wall2`,
`engineering/darkcarpet`, `voyager/basicdark`), are named by **all ten** decks; 190 by more than one.
The stitcher renames *entity* names when they collide but not shader names (`tools/shipmap/stitch.py`),
so the merged map still carries one `hall/hallfloor3` across every deck that uses it. Remap that name
on deck 9 and you remap it on all of them.

Our own decks are the counter-example, and the mechanism the engine *does* support: `gendeck.py`
names each generated deck's four sections per deck (`lwh/deck06wall0` … `lwh/deck06floor`), so 20
section shaders — five decks times four — are each named by exactly one deck:

```
$ python3 tools/shipmap/shader_census.py --decks build/ship/generated \
      --unique-pattern '^lwh/deck[0-9]+(wall[0-9]|floor)$'
    20 shader(s) match '^lwh/deck[0-9]+(wall[0-9]|floor)$', each named by exactly one deck
```

`scripts/deck-shader-check.sh` is both facts plus the renderer's by-name comment.

### Defect or boundary — it is a boundary, and this is why

The shader names are the publisher's content; we do not control them, and nothing we own is broken.
The two ways the evidence names are both larger than a fix:

- **(a) a stitcher pass that gives each deck unique shader names, then a rebake.** The stitcher would
  rewrite every shared face texture to a deck-scoped alias across ~447 names and the ~14,000 faces
  that use them, and the map would have to be recompiled — including `-vis`, which is the lever the
  next boundary is about (the ship is at 93.5% of q3map2's vis-cluster cap, and a per-deck alias
  change is new geometry a person must verify has not moved the vis count past it). It touches the
  published content, the build, and the visibility, for a presentation swap.
- **(b) an engine renderer capability that tags surfaces with a section and filters the remap by
  it.** A new renderer feature and its own patch series — a delta we own forever against
  lilium-voyager/ioEF (`docs/engine-extension-policy.md` rules 1–2), bought to make one presentation
  mode work on decks whose content we do not own.

What the engine's own supported mechanism offers instead is already in and demonstrated: author the
decks you control with deck-unique, section-sized shader names, and remap by name. That is what the
five generated decks do, and `scripts/borg-deck-check.sh` / `scripts/borgfx-check.sh` measure it
turning a deck Borg part by part and back. **The price of (a) is a content rebake under the vis
ceiling; the price of (b) is an engine rebase we carry forever.** Neither is taken here.

## 4. q3map2's vis-cluster cap — a boundary, priced, not taken

The brief's "worth a sentence, not a fix". The next lever for a *sixth* full-detail deck is not an
engine constant at all:

```
$ scripts/viscluster-check.sh
==> the merged ship's visibility lump (BSP lump 16), read from the packed artifact
    voyager.bsp: portalclusters 15321 of 16384 (93.5%), visdata bytes 29416328, clusterbytes 1920
==> the cap itself, in the compiler we run
    MAX_MAP_VISIBILITY exceeded
```

`MAX_MAP_VISCLUSTERS` is `0x4000` in q3map2 (`q3map2.h`), and q3map2 is a binary from a pinned
NetRadiant-custom release (`scripts/fetch-map-tools.sh`, 20260114) — a project we do not own and do
not build. A map past the cap is refused with `MAX_MAP_VISIBILITY exceeded` (its own string, above),
and `docs/evidence/deck07-auxcore.md` records the day it happened: before deck 7's interior became
detail geometry the ship was at 16,737 and `q3map2 -vis` refused the map. **Price of the fork:**
carrying a q3map2 built with a raised `MAX_MAP_VISCLUSTERS` — a third tree to pin, patch and rebuild
beside the engine and the module, and a compiler artefact every map in the programme is baked with,
for a cap that only the largest map in the programme is near. **Not taken.** The mechanism we do have
— authoring copied interiors as `detail` geometry so they add no vis clusters — is what keeps the
ship at 93.5%, and is the lever to reach for first.

## The checks, as PASS lines

```
PASS  a configstring numbered past the bridge's old literal 1024 survives the bridge, and the engine
      says so in its own output (SP_SetConfigstring: index 1500 accepted)
      — scripts/bridge-cs-check.sh; fails on an engine built before 0021 (--engine …/build-engine-0020)
PASS  the module's cg_public.h (1024) resolves for the bridge and the guard holds; pointing the include
      path at the engine's network cg_public.h (256) fails the build instead of silently halving snapshot_t
      — scripts/bridge-header-check.sh
PASS  the published decks share shader names across decks (hall/hallfloor3 and voyager/wall2 on all
      ten), so RemapShader — by name — cannot address one of them alone; the generated decks carry
      deck-unique section shaders
      — scripts/deck-shader-check.sh
PASS  the ship is under q3map2's MAX_MAP_VISCLUSTERS; the next full-detail deck is priced against a
      compiler constant we do not own, not against an engine limit
      — scripts/viscluster-check.sh
PASS  scripts/test.sh exits 0 (22 patches numbered and parsing; the harness case numbers distinct)
PASS  scripts/check.sh exits 0 (2,408 files: 2,394 compiled and read back; the 14 known rejections;
      the 2 unresolved references are exactly the recorded boundary)
PASS  scripts/s5-check.sh exits 0
PASS  scripts/damage-check.sh, emergency-check.sh, fire-check.sh, breach-check.sh exit 0 (the four
      world-effect checks)
```

## What could not be verified

- **A live drop of *shipped* content through the old table.** Nothing in retail or the merged ship
  names a configstring above 1023 (the game's layout bound is 523; models 46, sounds 53–67), so the
  drop is demonstrated with an out-of-range index set from the harness, not by content that exists.
  That is the honest shape of a latent trap: the fix is proved against the limit, not against a
  victim, because the point is that there is no victim yet.
- **The behavior of a deck-6 full re-dress under the vis cap** — unchanged from
  `docs/evidence/engine-content-headroom.md`; deck 6 is still the placeholder, and the vis count in
  §4 is the ship as it stands, not the ship with a sixth re-dress in it.
- **Mode 2 on the wire** was not re-run; `patches/0021` and `patches/0022` touch no wire format (a
  table size and a preprocessor check), and cMod is a separate binary this tree does not build
  (`raise-the-gentity-ceiling.md` established that it never loads our engine or module).

## Judgement calls, named as calls

1. **Item 1 is called a defect and fixed.** The value is latent (no shipped content exceeds 1023
   today), so it could have been called a boundary. It is a defect because the failure mode is
   *silent* content loss with no error and no victim named, the fix is one constant, and the same
   evidence file calls it "the wrong 1024". Deliberately not taken: a behavioural change to content
   registration (the game's `MAX_MODELS`/`MAX_SOUNDS`), which is a protocol fork
   (`raise-the-gentity-ceiling.md`, B2) and not this brief.
2. **The configstring table is sized from `MAX_CONFIGSTRINGS` (4096), not from the game's `CS_MAX`
   (523).** `MAX_CONFIGSTRINGS` is the ceiling the gamestate and the save are already sized against,
   and `bg_public.h` errors out if the layout ever exceeds it, so deriving from it is exactly the
   "cannot disagree with the ceiling" property the trap was about. The cost is a 4 MiB static table
   in `libmain` (from 1 MiB), zeroed once per spawn; it was accepted as the price of a table that
   cannot silently disagree with anything.
3. **The bridge's drop is logged under `developer`, not always.** `docs/engine-extension-policy.md`
   rule 3 wants the unset default to behave exactly as retail; a console line is not behaviour, but
   the pattern in `0017`/`0019` is to gate instrumentation, and it is followed here. The
   consequence — a real drop in a `developer`-off run is still silent — is named here rather than
   smoothed over; the alternative is console noise on every configstring the game drops at level
   init, which it does by design.
4. **Item 2 is called a latent defect, not a documented hazard.** Recording "256 and 1024" in a
   survey does not stop an innocently-reordered include path from compiling the wrong `snapshot_t`;
   the guard does, and it fails the build rather than the save. The two values are left different
   (they must be), and the module's `MAX_ENTITIES_IN_SNAPSHOT` (1024) is left untouched.
5. **Item 3 is a boundary and both fixes are declined, with their price.** The brief said "this one
   may be a boundary"; the measurement says it is, and the two ways round it are a content rebake
   under the vis ceiling and a new renderer capability carried forever. The engine's own mechanism
   (per-deck shader names) is the one we already use.
6. **The harness probe is a new `g_shipTest` value (95), not a new console verb.** The `ship <verb>`
   vocabulary is checked against `tools/…/console_api.def` by `scripts/api-check.sh`, and adding a
   probe verb would extend the game's public surface for a test; a harness case is the established
   instrument (about sixty of them) and is `g_shipTest`-gated, so it never runs in a normal session.

## Environment note — the GDK corpus had to be re-laid

`scripts/check.sh` failed at first on this host: the expected-rejection set (fourteen) names six
files (`_holodeck_*/behaved_*.txt`, `borg3/behaved.txt`) that the *fetch* had not laid down. Commit
`33a0017` changed `scripts/fetch-gdk.sh` to overlay the installer's own 2,408-file script tree on the
Level Scripts archive's 2,024, and the corpus on this host predated it. Re-laying the installer tree
(`unzip build/gdk/raw/_8AEBD20E… -d build/gdk/scripts`) brought the corpus to 2,408; `check.sh` then
reports 2,408 files, 2,394 compiled and read back, 14 rejected — "exactly the known, documented
set" — and exits 0. This is an environment
step, not a repository change; recorded here so the next person does not read the first failure as a
result of this pass.

## Reproducing

- Patches: `scripts/bootstrap-upstream.sh` (22 patches) then `cmake --build build-engine`;
  module: `cmake --build ../upstream/efgame/build-linux`.
- Item 1: `scripts/bridge-cs-check.sh` (and `--engine <build of patches/0001…0020>` to see it fail).
- Item 2: `scripts/bridge-header-check.sh` (needs the upstream checkout).
- Item 3: `scripts/deck-shader-check.sh` (needs `scripts/fetch-gdk.sh` and `scripts/build-ship.sh`).
- Item 4: `scripts/viscluster-check.sh` (needs `scripts/build-ship.sh`).
- Item 3's and 4's numbers, by hand: `python3 tools/shipmap/shader_census.py --decks <dir>`; the
  vis-cluster read is the BSP-lump-16 header of the `.bsp` inside `build/ship/out/longway_voyager.pk3`.
